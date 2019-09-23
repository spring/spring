/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobInstance.cpp */
#include "UnitScript.h"

#include "CobDefines.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "UnitScriptEngine.h"

#ifndef _CONSOLE

#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/MuzzleFlame.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/WakeProjectile.h"
#include "Rendering/Env/Particles/Classes/WreckProjectile.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Sound/ISoundChannels.h"

#endif

CR_BIND_INTERFACE(CUnitScript)

CR_REG_METADATA(CUnitScript, (
	CR_MEMBER(unit),
	CR_MEMBER(busy),
	CR_MEMBER(anims),

	//Populated by children
	CR_IGNORED(pieces),
	CR_IGNORED(hasSetSFXOccupy),
	CR_IGNORED(hasRockUnit),
	CR_IGNORED(hasStartBuilding)
))

CR_BIND(CUnitScript::AnimInfo,)

CR_REG_METADATA_SUB(CUnitScript, AnimInfo,(
	CR_MEMBER(axis),
	CR_MEMBER(piece),
	CR_MEMBER(speed),
	CR_MEMBER(dest),
	CR_MEMBER(accel),
	CR_MEMBER(done),
	CR_MEMBER(hasWaiting)
))


CUnitScript::CUnitScript(CUnit* unit)
	: unit(unit)
	, busy(false)
	, hasSetSFXOccupy(false)
	, hasRockUnit(false)
	, hasStartBuilding(false)
{ }


CUnitScript::~CUnitScript()
{
	// Remove us from possible animation ticking
	if (!HaveAnimations())
		return;

	unitScriptEngine->RemoveInstance(this);
}


/******************************************************************************/


/**
 * @brief Updates move animations
 * @param cur float value to update
 * @param dest float final value
 * @param speed float max increment per tick
 * @return returns true if destination was reached, false otherwise
 */
bool CUnitScript::MoveToward(float& cur, float dest, float speed)
{
	const float delta = dest - cur;

	if (math::fabsf(delta) <= speed) {
		cur = dest;
		return true;
	}

	cur += (speed * Sign(delta));
	return false;
}


/**
 * @brief Updates turn animations
 * @param cur float value to update
 * @param dest float final value
 * @param speed float max increment per tick
 * @return returns true if destination was reached, false otherwise
 */
bool CUnitScript::TurnToward(float& cur, float dest, float speed)
{
	float delta = dest - cur;

	// clamp: -pi .. 0 .. +pi (fmod(x,TWOPI) would do the same but is slower due to streflop)
	if (delta > math::PI) {
		delta -= math::TWOPI;
	} else if (delta <= -math::PI) {
		delta += math::TWOPI;
	}

	if (math::fabsf(delta) <= speed) {
		cur = dest;
		return true;
	}

	cur = ClampRad(cur + speed * Sign(delta));
	return false;
}


/**
 * @brief Updates spin animations
 * @param cur float value to update
 * @param dest float the final desired speed (NOT the final angle!)
 * @param speed float is updated if it is not equal to dest
 * @param divisor int is the deltatime, it is not added before the call because speed may have to be updated
 * @return true if the desired speed is 0 and it is reached, false otherwise
 */
bool CUnitScript::DoSpin(float& cur, float dest, float& speed, float accel, int divisor)
{
	const float delta = dest - speed;

	// Check if we are not at the final speed and
	// make sure we do not go past desired speed
	if (math::fabsf(delta) <= accel) {
		if ((speed = dest) == 0.0f)
			return true;
	} else {
		// accelerations are defined in speed/frame (at GAME_SPEED fps)
		speed += (accel * (GAME_SPEED * 1.0f / divisor) * Sign(delta));
	}

	cur = ClampRad(cur + (speed / divisor));
	return false;
}



void CUnitScript::TickAnims(int tickRate, const TickAnimFunc& tickAnimFunc, AnimContainerType& liveAnims, AnimContainerType& doneAnims) {
	for (size_t i = 0; i < liveAnims.size(); ) {
		AnimInfo& ai = liveAnims[i];
		LocalModelPiece& lmp = *pieces[ai.piece];

		if ((ai.done |= (this->*tickAnimFunc)(tickRate, lmp, ai))) {
			if (ai.hasWaiting)
				doneAnims.push_back(ai);

			ai = liveAnims.back();
			liveAnims.pop_back();
			continue;
		}

		++i;
	}
}

/**
 * @brief Called by the engine when we are registered as animating.
          If we return false there are no active animations left.
 * @param deltaTime int delta time to update
 * @return true if there are still active animations
 */
bool CUnitScript::Tick(int deltaTime)
{
	// vector of indexes of finished animations,
	// so we can get rid of them in constant time
	static AnimContainerType doneAnims[AMove + 1];
	// tick-functions; these never change address
	static constexpr TickAnimFunc tickAnimFuncs[AMove + 1] = {&CUnitScript::TickTurnAnim, &CUnitScript::TickSpinAnim, &CUnitScript::TickMoveAnim};

	for (int animType = ATurn; animType <= AMove; animType++) {
		TickAnims(1000 / deltaTime, tickAnimFuncs[animType], anims[animType], doneAnims[animType]);
	}

	// Tell listeners to unblock, and remove finished animations from the unit/script.
	for (int animType = ATurn; animType <= AMove; animType++) {
		for (AnimInfo& ai: doneAnims[animType]) {
			AnimFinished((AnimType) animType, ai.piece, ai.axis);
		}

		doneAnims[animType].clear();
	}

	return (HaveAnimations());
}



CUnitScript::AnimContainerTypeIt CUnitScript::FindAnim(AnimType type, int piece, int axis)
{
	const auto& pred = [&](const AnimInfo& ai) { return (ai.piece == piece && ai.axis == axis); };
	const auto& iter = std::find_if(anims[type].begin(), anims[type].end(), pred);
	return iter;
}

void CUnitScript::RemoveAnim(AnimType type, const AnimContainerTypeIt& animInfoIt)
{
	if (animInfoIt == anims[type].end())
		return;

	AnimInfo& ai = *animInfoIt;

	// We need to unblock threads waiting on this animation, otherwise they will be lost in the void
	// NOTE: AnimFinished might result in new anims being added
	if (ai.hasWaiting)
		AnimFinished(type, ai.piece, ai.axis);

	ai = anims[type].back();
	anims[type].pop_back();

	// If this was the last animation, remove from currently animating list
	// FIXME: this could be done in a cleaner way
	if (HaveAnimations())
		return;

	unitScriptEngine->RemoveInstance(this);
}


//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CUnitScript::AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::AddAnim] invalid script piece index");
		return;
	}

	float destf = 0.0f;

	if (type == AMove) {
		destf = pieces[piece]->original->offset[axis] + dest;
	} else {
		// clamp destination (angle) for turn-anims
		destf = mix(dest, ClampRad(dest), type == ATurn);
	}

	AnimContainerTypeIt animInfoIt;
	AnimInfo* ai = nullptr;
	AnimType overrideType = ANone;

	// first find an animation of a type we override
	// Turns override spins.. Not sure about the other way around? If so
	// the system should probably be redesigned to only have two types of
	// anims (turns and moves), with spin as a bool
	switch (type) {
		case ATurn: {
			overrideType = ASpin;
			animInfoIt = FindAnim(overrideType, piece, axis);
		} break;
		case ASpin: {
			overrideType = ATurn;
			animInfoIt = FindAnim(overrideType, piece, axis);
		} break;
		case AMove: {
			// ensure we never remove an animation of this type
			overrideType = AMove;
			animInfoIt = anims[overrideType].end();
		} break;
		default: {
		} break;
	}
	assert(overrideType >= 0);

	if (animInfoIt != anims[overrideType].end())
		RemoveAnim(overrideType, animInfoIt);

	// now find an animation of our own type
	animInfoIt = FindAnim(type, piece, axis);

	if (animInfoIt == anims[type].end()) {
		// If we were not animating before, inform the engine of this so it can schedule us
		// FIXME: this could be done in a cleaner way
		if (!HaveAnimations())
			unitScriptEngine->AddInstance(this);

		anims[type].emplace_back();
		ai = &anims[type].back();
		ai->piece = piece;
		ai->axis = axis;
	} else {
		ai = &(*animInfoIt);
	}

	ai->dest  = destf;
	ai->speed = speed;
	ai->accel = accel;
	ai->done = false;
}


void CUnitScript::Spin(int piece, int axis, float speed, float accel)
{
	auto animInfoIt = FindAnim(ASpin, piece, axis);

	// if we are already spinning, we may have to decelerate to the new speed
	if (animInfoIt != anims[ASpin].end()) {
		AnimInfo* ai = &(*animInfoIt);
		ai->dest = speed;

		if (accel > 0.0f) {
			ai->accel = accel;
		} else {
			// Go there instantly. Or have a defaul accel?
			ai->speed = speed;
			ai->accel = 0.0f;
		}

		return;
	}

	// no acceleration means we start at desired speed instantly
	if (accel <= 0.0f) {
		AddAnim(ASpin, piece, axis,  speed, speed, 0.0f);
	} else {
		AddAnim(ASpin, piece, axis,   0.0f, speed, accel);
	}
}


void CUnitScript::StopSpin(int piece, int axis, float decel)
{
	auto animInfoIt = FindAnim(ASpin, piece, axis);

	if (decel <= 0.0f) {
		RemoveAnim(ASpin, animInfoIt);
	} else {
		if (animInfoIt == anims[ASpin].end())
			return;

		AnimInfo* ai = &(*animInfoIt);
		ai->dest = 0.0f;
		ai->accel = decel;
	}
}


void CUnitScript::Turn(int piece, int axis, float speed, float destination)
{
	AddAnim(ATurn, piece, axis, std::max(speed, -speed), destination, 0);
}


void CUnitScript::Move(int piece, int axis, float speed, float destination)
{
	AddAnim(AMove, piece, axis, std::max(speed, -speed), destination, 0);
}


void CUnitScript::MoveNow(int piece, int axis, float destination)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::MoveNow] invalid script piece index");
		return;
	}

	LocalModelPiece* p = pieces[piece];

	float3 pos = p->GetPosition();
	float3 ofs = p->original->offset;

	pos[axis] = ofs[axis] + destination;

	p->SetPosition(pos);
}


void CUnitScript::TurnNow(int piece, int axis, float destination)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::TurnNow] invalid script piece index");
		return;
	}

	LocalModelPiece* p = pieces[piece];

	float3 rot = p->GetRotation();
	rot[axis] = destination;

	p->SetRotation(rot);
}


void CUnitScript::SetVisibility(int piece, bool visible)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::SetVisibility] invalid script piece index");
		return;
	}

	pieces[piece]->scriptSetVisible = visible;
}


bool CUnitScript::EmitSfx(int sfxType, int sfxPiece)
{
#ifndef _CONSOLE
	if (!PieceExists(sfxPiece)) {
		ShowUnitScriptError("[US::EmitSFX] invalid script piece index");
		return false;
	}

	float3 relPos = ZeroVector;
	float3 relDir = UpVector;

	if (!GetEmitDirPos(sfxPiece, relPos, relDir)) {
		ShowUnitScriptError("[US::EmitSFX] invalid model piece index");
		return false;
	}

	return (EmitRelSFX(sfxType, relPos, relDir.SafeNormalize()));
#endif
}

bool CUnitScript::EmitRelSFX(int sfxType, const float3& relPos, const float3& relDir)
{
	// convert piece-space {pos,dir} to unit-space
	return (EmitAbsSFX(sfxType, unit->GetObjectSpacePos(relPos), unit->GetObjectSpaceVec(relDir), relDir));
}

bool CUnitScript::EmitAbsSFX(int sfxType, const float3& absPos, const float3& absDir, const float3& relDir)
{
	// skip adding (non-CEG) particles when we have too many
	if (sfxType < SFX_CEG && projectileHandler.GetParticleSaturation() > 1.0f)
		return false;

	// make sure wakes are only emitted on water
	if (sfxType >= SFX_WAKE && sfxType <= SFX_REVERSE_WAKE_2 && !unit->IsInWater())
		return false;

	float wakeAlphaStart = 0.3f + guRNG.NextFloat() * 0.2f;
	float wakeAlphaDecay = 0.004f;
	float wakeFadeupTime = 4.0f;

	const UnitDef* ud = unit->unitDef;
	const MoveDef* md = unit->moveDef;

	// hovercraft need special care (?)
	if (md != nullptr && md->speedModClass == MoveDef::Hover) {
		wakeAlphaStart = 0.15f + guRNG.NextFloat() * 0.2f;
		wakeAlphaDecay = 0.008f;
		wakeFadeupTime = 8.0f;
	}

	switch (sfxType) {
		// reverse ship wake
		case SFX_REVERSE_WAKE:
		case SFX_REVERSE_WAKE_2: {
			projMemPool.alloc<CWakeProjectile>(
				unit,
				absPos + guRNG.NextVector() * 2.0f,
				absDir * -0.4f,
				6.0f + guRNG.NextFloat() * 4.0f,
				0.15f + guRNG.NextFloat() * 0.3f,
				wakeAlphaStart, wakeAlphaDecay, wakeFadeupTime
			);
		} break;

		// regular ship wake
		case SFX_WAKE_2:
		case SFX_WAKE: {
			projMemPool.alloc<CWakeProjectile>(
				unit,
				absPos + guRNG.NextVector() * 2.0f,
				absDir * 0.4f,
				6.0f + guRNG.NextFloat() * 4.0f,
				0.15f + guRNG.NextFloat() * 0.3f,
				wakeAlphaStart, wakeAlphaDecay, wakeFadeupTime
			);
		} break;

		// submarine bubble; does not provide direction through piece vertices
		case SFX_BUBBLE: {
			const float3 pspeed = guRNG.NextVector() * 0.1f;

			projMemPool.alloc<CBubbleProjectile>(
				unit,
				absPos + guRNG.NextVector() * 2.0f,
				pspeed + UpVector * 0.2f,
				40.0f + guRNG.NextFloat() * GAME_SPEED,
				1.0f + guRNG.NextFloat() * 2.0f,
				0.01f,
				0.3f + guRNG.NextFloat() * 0.3f
			);
		} break;

		// damaged unit smoke
		case SFX_WHITE_SMOKE: {
			projMemPool.alloc<CSmokeProjectile>(unit, absPos, guRNG.NextVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, 0.5f);
		} break;
		case SFX_BLACK_SMOKE: {
			projMemPool.alloc<CSmokeProjectile>(unit, absPos, guRNG.NextVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, 0.6f);
		} break;

		case SFX_VTOL: {
			const float4 scale = {0.5f, 0.5f, 0.5f, 0.7f};
			const float3 speed =
				unit->speed    * scale.w +
				unit->frontdir * scale.z *             relDir.z  +
				unit->updir    * scale.y * -math::fabs(relDir.y) +
				unit->rightdir * scale.x *             relDir.x;

			CHeatCloudProjectile* hc = projMemPool.alloc<CHeatCloudProjectile>(
				unit,
				absPos,
				speed,
				10.0f + guRNG.NextFloat() * 5.0f,
				3.0f + guRNG.NextFloat() * 2.0f
			);

			hc->size = 3;
		} break;

		default: {
			if ((sfxType & SFX_GLOBAL) != 0) {
				// emit defined explosion-generator (can only be custom, not standard)
				// index is made valid by callee, an ID of -1 means CEG failed to load
				explGenHandler.GenExplosion(sfxType - SFX_GLOBAL, absPos, absDir, unit->cegDamage, 1.0f, 0.0f, unit, nullptr);
				return true;
			}
			if ((sfxType & SFX_CEG) != 0) {
				// emit defined explosion-generator (can only be custom, not standard)
				// index is made valid by callee, an ID of -1 means CEG failed to load
				explGenHandler.GenExplosion(ud->GetModelExpGenID(sfxType - SFX_CEG), absPos, absDir, unit->cegDamage, 1.0f, 0.0f, unit, nullptr);
				return true;
			}

			if ((sfxType & SFX_FIRE_WEAPON) != 0) {
				// make a weapon fire from the piece
				const unsigned int index = sfxType - SFX_FIRE_WEAPON;

				if (index >= unit->weapons.size()) {
					ShowUnitScriptError("[US::EmitSFX::FIRE_WEAPON] invalid weapon index");
					break;
				}

				CWeapon* w = unit->weapons[index];

				const SWeaponTarget origTarget = w->GetCurrentTarget();
				const SWeaponTarget emitTarget = {absPos + absDir};

				const float3 origWeaponMuzzlePos = w->weaponMuzzlePos;

				w->SetAttackTarget(emitTarget);
				w->weaponMuzzlePos = absPos;
				w->Fire(true);
				w->weaponMuzzlePos = origWeaponMuzzlePos;

				const bool origRestored = w->Attack(origTarget);
				assert(origRestored);
				return true;
			}

			if ((sfxType & SFX_DETONATE_WEAPON) != 0) {
				const unsigned index = sfxType - SFX_DETONATE_WEAPON;

				if (index >= unit->weapons.size()) {
					ShowUnitScriptError("[US::EmitSFX::DETONATE_WEAPON] invalid weapon index");
					break;
				}

				// detonate weapon from piece
				const CWeapon* weapon = unit->weapons[index];
				const WeaponDef* weaponDef = weapon->weaponDef;

				const CExplosionParams params = {
					absPos,
					ZeroVector,
					*weapon->damages,
					weaponDef,
					unit,                              // owner
					nullptr,                           // hitUnit
					nullptr,                           // hitFeature
					weapon->damages->craterAreaOfEffect,
					weapon->damages->damageAreaOfEffect,
					weapon->damages->edgeEffectiveness,
					weapon->damages->explosionSpeed,
					1.0f,                              // gfxMod
					weaponDef->impactOnly,
					weaponDef->noSelfDamage,           // ignoreOwner
					true,                              // damageGround
					-1u                                // projectileID
				};

				helper->Explosion(params);
			}
		} break;
	}

	return true;
}


void CUnitScript::AttachUnit(int piece, int u)
{
	// -1 is valid, indicates that the unit should be hidden
	if ((piece >= 0) && (!PieceExists(piece))) {
		ShowUnitScriptError("[US::AttachUnit] invalid script piece index");
		return;
	}

#ifndef _CONSOLE
	CUnit* tgtUnit = unitHandler.GetUnit(u);

	if (tgtUnit == nullptr || !unit->unitDef->IsTransportUnit())
		return;

	unit->AttachUnit(tgtUnit, piece);
#endif
}


void CUnitScript::DropUnit(int u)
{
#ifndef _CONSOLE
	CUnit* tgtUnit = unitHandler.GetUnit(u);

	if (tgtUnit == nullptr || !unit->unitDef->IsTransportUnit())
		return;

	unit->DetachUnit(tgtUnit);
#endif
}


//Returns true if there was an animation to listen to
bool CUnitScript::NeedsWait(AnimType type, int piece, int axis)
{
	auto animInfoIt = FindAnim(type, piece, axis);

	if (animInfoIt == anims[type].end())
		return false;

	AnimInfo& ai = *animInfoIt;

	// if the animation is already finished, listening for
	// it just adds some overhead since either the current
	// or the next Tick will remove it and call UnblockAll
	// (which calls AnimFinished for each listener)
	//
	// we could notify the listener here, but a cleaner way
	// is to treat the animation as if it did not exist and
	// simply disregard the WaitFor* (no side-effects)
	//
	// if (ai.hasWaiting)
	// 		AnimFinished(ai.type, ai.piece, ai.axis);
	if (ai.done)
		return false;

	return (ai.hasWaiting = true);
}


//Flags as defined by the cob standard
void CUnitScript::Explode(int piece, int flags)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::Explode] invalid script piece index");
		return;
	}

#ifndef _CONSOLE
	const float3 relPos = GetPiecePos(piece);
	const float3 absPos = unit->GetObjectSpacePos(relPos);

	// do an explosion at the location first
	if (!(flags & PF_NoHeatCloud))
		projMemPool.alloc<CHeatCloudProjectile>(nullptr, absPos, ZeroVector, 30, 30);

	// If this is true, no stuff should fly off
	if (flags & PF_NONE)
		return;

	if (pieces[piece]->original == nullptr)
		return;

	if (flags & PF_Shatter) {
		Shatter(piece, absPos, unit->speed);
		return;
	}

	// This means that we are going to do a full fledged piece explosion!
	float3 baseSpeed = unit->speed;
	float3 explSpeed;
	explSpeed.x = (0.5f -  gsRNG.NextFloat()) * 6.0f;
	explSpeed.y =  1.2f + (gsRNG.NextFloat()  * 5.0f);
	explSpeed.z = (0.5f -  gsRNG.NextFloat()) * 6.0f;

	if (unit->pos.y - CGround::GetApproximateHeight(unit->pos.x, unit->pos.z) > 15)
		explSpeed.y = (0.5f - gsRNG.NextFloat()) * 6.0f;

	if (baseSpeed.SqLength() > 9.0f) {
		const float l  = baseSpeed.Length();
		const float l2 = 3.0f + math::sqrt(l - 3.0f);
		baseSpeed *= (l2 / l);
	}

	const float partSat = projectileHandler.GetParticleSaturation();

	unsigned int newFlags = 0;
	newFlags |= (PF_Explode    *  ((flags & PF_Explode   ) != 0)                    );
	newFlags |= (PF_Smoke      * (((flags & PF_Smoke     ) != 0) && partSat < 0.95f));
	newFlags |= (PF_Fire       * (((flags & PF_Fire      ) != 0) && partSat < 0.95f));
	newFlags |= (PF_NoCEGTrail *  ((flags & PF_NoCEGTrail) != 0)                    );
	newFlags |= (PF_Recursive  *  ((flags & PF_Recursive ) != 0)                    );

	projMemPool.alloc<CPieceProjectile>(unit, pieces[piece], absPos, explSpeed + baseSpeed, newFlags, 0.5f);
#endif
}


void CUnitScript::Shatter(int piece, const float3& pos, const float3& speed)
{
	const S3DModel* mdl = unit->model;
	const LocalModelPiece* lmp = pieces[piece];
	const S3DModelPiece* omp = lmp->original;

	if (!omp->HasGeometryData())
		return;

	const float pieceChance = 1.0f - (projectileHandler.GetCurrentParticles() - (projectileHandler.maxParticles - 2000)) / 2000.0f;

	if (pieceChance <= 0.0f)
		return;

	omp->Shatter(mdl, unit->team, pieceChance, pos, speed, unit->GetTransformMatrix() * lmp->GetModelSpaceMatrix());
}


void CUnitScript::ShowFlare(int piece)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("[US::ShowFlare] invalid script piece index");
		return;
	}
#ifndef _CONSOLE
	const float3 relPos = GetPiecePos(piece);
	const float3 absPos = unit->GetObjectSpacePos(relPos);

	projMemPool.alloc<CMuzzleFlame>(absPos, unit->speed, unit->lastMuzzleFlameDir, unit->lastMuzzleFlameSize);
#endif
}


/******************************************************************************/
int CUnitScript::GetUnitVal(int val, int p1, int p2, int p3, int p4)
{
	// may happen in case one uses Spring.GetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (unit == nullptr) {
		ShowUnitScriptError("[US::GetUnitVal] invoked for null-scripted unit");
		return 0;
	}

#ifndef _CONSOLE
	switch (val) {
	case ACTIVATION:
		return (unit->activated);
	case STANDINGMOVEORDERS:
		return unit->moveState;
		break;
	case STANDINGFIREORDERS:
		return unit->fireState;
		break;

	case HEALTH: {
		if (p1 <= 0)
			return int((unit->health / unit->maxHealth) * 100.0f);

		const CUnit* u = unitHandler.GetUnit(p1);

		if (u == nullptr)
			return 0;

		return int((u->health / u->maxHealth) * 100.0f);
	} break;

	case INBUILDSTANCE: {
		return (unit->inBuildStance);
	} break;
	case BUSY: {
		return (busy);
	} break;

	case PIECE_XZ: {
		if (!PieceExists(p1)) {
			ShowUnitScriptError("[US::GetUnitVal::PIECE_XZ] invalid script piece index");
			break;
		}
		const float3 relPos = GetPiecePos(p1);
		const float3 absPos = unit->GetObjectSpacePos(relPos);
		return PACKXZ(absPos.x, absPos.z);
	} break;
	case PIECE_Y: {
		if (!PieceExists(p1)) {
			ShowUnitScriptError("[US::GetUnitVal::PIECE_Y] invalid script piece index");
			break;
		}
		const float3 relPos = GetPiecePos(p1);
		const float3 absPos = unit->GetObjectSpacePos(relPos);
		return int(absPos.y * COBSCALE);
	} break;

	case UNIT_XZ: {
		if (p1 <= 0)
			return PACKXZ(unit->pos.x, unit->pos.z);

		const CUnit* u = unitHandler.GetUnit(p1);

		if (u == nullptr)
			return PACKXZ(0, 0);

		return PACKXZ(u->pos.x, u->pos.z);
	} break;
	case UNIT_Y: {
		if (p1 <= 0)
			return int(unit->pos.y * COBSCALE);

		const CUnit* u = unitHandler.GetUnit(p1);

		if (u == nullptr)
			return 0;

		return int(u->pos.y * COBSCALE);
	} break;
	case UNIT_HEIGHT: {
		if (p1 <= 0)
			return int(unit->radius * COBSCALE);

		const CUnit* u = unitHandler.GetUnit(p1);

		if (u == nullptr)
			return 0;

		return int(u->radius * COBSCALE);
	} break;

	case XZ_ATAN:
		return int(RAD2TAANG*math::atan2((float)UNPACKX(p1), (float)UNPACKZ(p1)) + 32768 - unit->heading);
	case XZ_HYPOT:
		return int(math::hypot((float)UNPACKX(p1), (float)UNPACKZ(p1)) * COBSCALE);
	case ATAN:
		return int(RAD2TAANG*math::atan2((float)p1, (float)p2));
	case HYPOT:
		return int(math::hypot((float)p1, (float)p2));
	case GROUND_HEIGHT:
		return int(CGround::GetHeightAboveWater(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case GROUND_WATER_HEIGHT:
		return int(CGround::GetHeightReal(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case BUILD_PERCENT_LEFT:
		return int((1.0f - unit->buildProgress) * 100);

	case YARD_OPEN:
		return (unit->yardOpen) ? 1 : 0;
	case BUGGER_OFF:
		break;
	case ARMORED:
		return (unit->armoredState) ? 1 : 0;
	case VETERAN_LEVEL:
		return int(100 * unit->experience);
	case CURRENT_SPEED:
		return int(unit->speed.w * COBSCALE);
	case ON_ROAD:
		return 0;
	case IN_WATER:
		return (unit->IsInWater());
	case MAX_ID:
		return unitHandler.MaxUnits()-1;
	case MY_ID:
		return unit->id;

	case UNIT_TEAM: {
		const CUnit* u = unitHandler.GetUnit(p1);
		return (u != nullptr)? unit->team : 0;
	} break;
	case UNIT_ALLIED: {
		const CUnit* u = unitHandler.GetUnit(p1);

		if (u != nullptr)
			return teamHandler.Ally(unit->allyteam, u->allyteam) ? 1 : 0;

		return 0;
	} break;

	case UNIT_BUILD_PERCENT_LEFT: {
		const CUnit* u = unitHandler.GetUnit(p1);
		if (u != nullptr)
			return int((1.0f - u->buildProgress) * 100);

		return 0;
	} break;
	case MAX_SPEED: {
		return int(unit->moveType->GetMaxSpeed() * COBSCALE);
	} break;
	case REVERSING: {
		CGroundMoveType* gmt = dynamic_cast<CGroundMoveType*>(unit->moveType);
		return ((gmt != nullptr)? int(gmt->IsReversing()): 0);
	} break;
	case CLOAKED:
		return !!unit->isCloaked;
	case WANT_CLOAK:
		return !!unit->wantCloak;
	case UPRIGHT:
		return !!unit->upright;
	case POW:
		return int(math::pow(((float)p1)/COBSCALE,((float)p2)/COBSCALE)*COBSCALE);
	case PRINT:
		LOG("Value 1: %d, 2: %d, 3: %d, 4: %d", p1, p2, p3, p4);
		break;
	case HEADING: {
		if (p1 <= 0)
			return unit->heading;

		const CUnit* u = unitHandler.GetUnit(p1);

		if (u != nullptr)
			return u->heading;

		return -1;
	} break;
	case TARGET_ID: {
		if (size_t(p1 - 1) < unit->weapons.size()) {
			const CWeapon* w = unit->weapons[p1 - 1];
			auto curTarget = w->GetCurrentTarget();
			switch (curTarget.type) {
				case Target_Unit:      return curTarget.unit->id;
				case Target_None:      return -1;
				case Target_Pos:       return -2;
				case Target_Intercept: return -3;
			}
		}
		return -4; // weapon does not exist
	} break;

	case LAST_ATTACKER_ID:
		return ((unit->lastAttacker != nullptr)? unit->lastAttacker->id: -1);
	case LOS_RADIUS:
		return unit->realLosRadius;
	case AIR_LOS_RADIUS:
		return unit->realAirLosRadius;
	case RADAR_RADIUS:
		return unit->radarRadius;
	case JAMMER_RADIUS:
		return unit->jammerRadius;
	case SONAR_RADIUS:
		return unit->sonarRadius;
	case SONAR_JAM_RADIUS:
		return unit->sonarJamRadius;
	case SEISMIC_RADIUS:
		return unit->seismicRadius;

	case DO_SEISMIC_PING: {
		unit->DoSeismicPing((p1 == 0)? unit->seismicSignature: p1);
	} break;

	case CURRENT_FUEL: //deprecated
		return 0;
	case TRANSPORT_ID:
		return ((unit->transporter != nullptr)? unit->transporter->id: -1);

	case SHIELD_POWER: {
		const CWeapon* shield = unit->shieldWeapon;

		if (shield == nullptr)
			return -1;

		return int(static_cast<const CPlasmaRepulser*>(shield)->GetCurPower() * float(COBSCALE));
	} break;

	case STEALTH: {
		return unit->stealth;
	} break;
	case SONAR_STEALTH: {
		return unit->sonarStealth;
	} break;

	case CRASHING:
		return (unit->IsCrashing());

	case COB_ID: {
		if (p1 <= 0)
			return unit->unitDef->cobID;

		const CUnit* u = unitHandler.GetUnit(p1);
		return ((u == nullptr)? -1 : u->unitDef->cobID);
	} break;

 	case PLAY_SOUND: {
 		// FIXME: this currently only works for CCobInstance, because Lua can not get sound IDs
 		// (however, for Lua scripts there is already LuaUnsyncedCtrl::PlaySoundFile)
 		CCobInstance* cobInst = dynamic_cast<CCobInstance*>(this);

 		if (cobInst == nullptr)
 			return 1;

 		const CCobFile* cobFile = cobInst->GetFile();

 		if (cobFile == nullptr)
 			return 1;

		if ((p1 < 0) || (static_cast<size_t>(p1) >= cobFile->sounds.size()))
			return 1;

		// who hears the sound
		switch (p3) {
			case 0: {
				// ALOS
				if (!losHandler->InAirLos(unit, gu->myAllyTeam))
					return 0;
			} break;
			case 1: {
				// LOS
				if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS))
					return 0;
			} break;
			case 2: {
				// ALOS or radar
				if (!(losHandler->InAirLos(unit, gu->myAllyTeam) || unit->losStatus[gu->myAllyTeam] & (LOS_INRADAR)))
					return 0;
			} break;
			case 3: {
				// LOS or radar
				if (!(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)))
					return 0;
			} break;
			case 4: {
				// everyone
			} break;
			case 5: {
				// allies
				if (unit->allyteam != gu->myAllyTeam)
					return 0;
			} break;
			case 6: {
				// team
				if (unit->team != gu->myTeam)
					return 0;
			} break;
			case 7: {
				// enemies
				if (unit->allyteam == gu->myAllyTeam)
					return 0;
			} break;
			default: {
			} break;
		}

		if (p4 == 0) {
			Channels::General->PlaySample(cobFile->sounds[p1], unit->pos, unit->speed, float(p2) / COBSCALE);
		} else {
			Channels::General->PlaySample(cobFile->sounds[p1], float(p2) / COBSCALE);
		}

		return 0;
	} break;

	case SET_WEAPON_UNIT_TARGET: {
		const unsigned int weaponID = p1 - 1;
		const unsigned int targetID = p2;
		const bool userTarget = !!p3;

		if (weaponID >= unit->weapons.size())
			return 0;

		CWeapon* weapon = unit->weapons[weaponID];

		if (weapon == nullptr)
			return 0;

		// if targetID is 0, just sets weapon->haveUserTarget
		// to false (and targetType to None) without attacking
		CUnit* target = (targetID > 0)? unitHandler.GetUnit(targetID): nullptr;
		return (weapon->Attack(SWeaponTarget(target, userTarget)));
	} break;

	case SET_WEAPON_GROUND_TARGET: {
		const unsigned int weaponID = p1 - 1;

		const float3 pos = float3(float(UNPACKX(p2)),
		                          float(p3) / float(COBSCALE),
		                          float(UNPACKZ(p2)));
		const bool userTarget = !!p4;

		if (weaponID >= unit->weapons.size())
			return 0;

		CWeapon* weapon = unit->weapons[weaponID];

		if (weapon == nullptr)
			return 0;

		return weapon->Attack(SWeaponTarget(pos, userTarget)) ? 1 : 0;
	} break;

	case MIN:
		return std::min(p1, p2);
	case MAX:
		return std::max(p1, p2);
	case ABS:
		return std::abs(p1);
	case KSIN:
		return int(1024*math::sinf(TAANG2RAD*(float)p1));
	case KCOS:
		return int(1024*math::cosf(TAANG2RAD*(float)p1));
	case KTAN:
		return int(1024*math::tanf(TAANG2RAD*(float)p1));
	case SQRT:
		return int(math::sqrt((float)p1));

	case FLANK_B_MODE:
		return unit->flankingBonusMode;
	case FLANK_B_DIR:
		switch (p1) {
			case 1: return int(unit->flankingBonusDir.x * COBSCALE);
			case 2: return int(unit->flankingBonusDir.y * COBSCALE);
			case 3: return int(unit->flankingBonusDir.z * COBSCALE);
			case 4: unit->flankingBonusDir.x = (p2 / (float)COBSCALE); return 0;
			case 5: unit->flankingBonusDir.y = (p2 / (float)COBSCALE); return 0;
			case 6: unit->flankingBonusDir.z = (p2 / (float)COBSCALE); return 0;
			case 7: unit->flankingBonusDir = float3(p2/(float)COBSCALE, p3/(float)COBSCALE, p4/(float)COBSCALE).Normalize(); return 0;
			default: return(-1);
		}
	case FLANK_B_MOBILITY_ADD:
		return int(unit->flankingBonusMobilityAdd * COBSCALE);
	case FLANK_B_MAX_DAMAGE:
		return int((unit->flankingBonusAvgDamage + unit->flankingBonusDifDamage) * COBSCALE);
	case FLANK_B_MIN_DAMAGE:
		return int((unit->flankingBonusAvgDamage - unit->flankingBonusDifDamage) * COBSCALE);

	case KILL_UNIT: {
		// ID 0 is reserved for the script's owner
		CUnit* u = (p1 > 0)? unitHandler.GetUnit(p1): this->unit;

		if (u == nullptr)
			return 0;

		if (u->beingBuilt) {
			// no explosions and no corpse for units under construction
			u->KillUnit(nullptr, false, true);
		} else {
			u->KillUnit(nullptr, p2 != 0, p3 != 0);
		}

		return 1;
	} break;


	case WEAPON_RELOADSTATE: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return unit->weapons[p1 - 1]->reloadStatus;

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->reloadStatus;
			w->reloadStatus = p2;
			return old;
		}

		return -1;
	} break;

	case WEAPON_RELOADTIME: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return unit->weapons[p1 - 1]->reloadTime;

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->reloadTime;
			w->reloadTime = p2;
			return old;
		}

		return -1;
	} break;

	case WEAPON_ACCURACY: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->accuracyError * COBSCALE);

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->accuracyError * COBSCALE;
			w->accuracyError = float(p2) / COBSCALE;
			return old;
		}

		return -1;
	} break;

	case WEAPON_SPRAY: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->sprayAngle * COBSCALE);

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->sprayAngle * COBSCALE;
			w->sprayAngle = float(p2) / COBSCALE;
			return old;
		}

		return -1;
	} break;

	case WEAPON_RANGE: {
		// get (+)
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->range * COBSCALE);

		// set (-)
		if (-p1 > 0 && static_cast<size_t>(-p1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[-p1 - 1];

			const float  wr = w->range * COBSCALE;
			const float nwr = float(p2) / COBSCALE;

			w->UpdateRange(nwr);
			return int(wr);
		}

		return -1;
	} break;

	case WEAPON_PROJECTILE_SPEED: {
		// get (+)
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->projectileSpeed * COBSCALE);

		// set (-)
		if (-p1 > 0 && static_cast<size_t>(-p1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[-p1 - 1];

			const float  wps = w->projectileSpeed * COBSCALE;
			const float nwps = float(p2) / COBSCALE;

			w->UpdateProjectileSpeed(nwps);
			return int(wps);
		}

		return -1;
	} break;


	case GAME_FRAME: {
		return gs->frameNum;
	} break;

	default:
		if ((val >= GLOBAL_VAR_START) && (val <= GLOBAL_VAR_END)) {
			ShowUnitScriptError("[US::GetUnitVal] COB global vars are deprecated");
			return 0;
		}
		else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
			ShowUnitScriptError("[US::GetUnitVal] COB team vars are deprecated");
			return 0;
		}
		else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
			ShowUnitScriptError("[US::GetUnitVal] COB allyteam vars are deprecated");
			return 0;
		}
		else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
			ShowUnitScriptError("[US::GetUnitVal] COB unit vars are deprecated");
			return 0;
		}
		else {
			ShowUnitScriptError("[US::GetUnitVal] CobError: unknown get-constant " + IntToString(val) + " (params = " + IntToString(p1) + " " +
			IntToString(p2) + " " + IntToString(p3) + " " + IntToString(p4) + ")");
		}
	}
#endif

	return 0;
}


void CUnitScript::SetUnitVal(int val, int param)
{
	// may happen in case one uses Spring.SetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (unit == nullptr) {
		ShowUnitScriptError("[US::SetUnitVal] invoked for null-scripted unit");
		return;
	}

#ifndef _CONSOLE
	switch (val) {
		case ACTIVATION: {
			if (unit->unitDef->onoffable) {
				Command c(CMD_ONOFF, 0, (param == 0) ? 0 : 1);
				unit->commandAI->GiveCommand(c);
			} else {
				if (param == 0) {
					unit->Deactivate();
				} else {
					unit->Activate();
				}
			}
		} break;

		case STANDINGMOVEORDERS: {
			if (param >= 0 && param <= MOVESTATE_ROAM) {
				Command c(CMD_MOVE_STATE, 0, param);
				unit->commandAI->GiveCommand(c);
			}
		} break;

		case STANDINGFIREORDERS: {
			if (param >= 0 && param <= FIRESTATE_FIREATNEUTRAL) {
				Command c(CMD_FIRE_STATE, 0, param);
				unit->commandAI->GiveCommand(c);
			}
		} break;

		case HEALTH: {
		} break;
		case INBUILDSTANCE: {
			unit->inBuildStance = (param != 0);
		} break;

		case BUSY: {
			busy = (param != 0);
		} break;

		case PIECE_XZ: {
		} break;

		case PIECE_Y: {
		} break;

		case UNIT_XZ: {
		} break;

		case UNIT_Y: {
		} break;

		case UNIT_HEIGHT: {
		} break;

		case XZ_ATAN: {
		} break;

		case XZ_HYPOT: {
		} break;

		case ATAN: {
		} break;

		case HYPOT: {
		} break;

		case GROUND_HEIGHT: {
		} break;

		case GROUND_WATER_HEIGHT: {
		} break;

		case BUILD_PERCENT_LEFT: {
		} break;

		case YARD_OPEN: {
			if (unit->GetBlockMap() != nullptr) {
				// note: if this unit is a factory, engine-controlled
				// OpenYard() and CloseYard() calls can interfere with
				// the yardOpen state (they probably should be removed
				// at some point)
				if (param == 0) {
					if (groundBlockingObjectMap.CanCloseYard(unit))
						groundBlockingObjectMap.CloseBlockingYard(unit);

				} else {
					if (groundBlockingObjectMap.CanOpenYard(unit))
						groundBlockingObjectMap.OpenBlockingYard(unit);

				}
			}
		} break;

		case BUGGER_OFF: {
			if (param != 0)
				CGameHelper::BuggerOff(unit->pos + unit->frontdir * unit->radius, unit->radius * 1.5f, true, false, unit->team, nullptr);

		} break;

		case ARMORED: {
			unit->curArmorMultiple = mix(1.0f, unit->armoredMultiple, param != 0);
			unit->armoredState = (param != 0);
		} break;

		case VETERAN_LEVEL: {
			unit->experience = param * 0.01f;
		} break;

		case MAX_SPEED: {
			if (param >= 0) {
				unit->moveType->SetMaxSpeed(param / float(COBSCALE));
			} else {
				// temporarily (until the next command is issued) change unit's speed
				unit->moveType->SetWantedMaxSpeed(-param / float(COBSCALE));
			}
		} break;

		case CLOAKED: {
			unit->wantCloak = !!param;
		} break;

		case WANT_CLOAK: {
			unit->wantCloak = !!param;
		} break;

		case UPRIGHT: {
			unit->upright = !!param;
		} break;

		case HEADING: {
			unit->SetHeading(param % COBSCALE, !unit->upright && unit->IsOnGround(), false);
		} break;
		case LOS_RADIUS: {
			unit->ChangeLos(param, unit->realAirLosRadius);
			unit->realLosRadius = param;
		} break;

		case AIR_LOS_RADIUS: {
			unit->ChangeLos(unit->realLosRadius, param);
			unit->realAirLosRadius = param;
		} break;

		case RADAR_RADIUS: {
			unit->radarRadius = param;
		} break;

		case JAMMER_RADIUS: {
			unit->jammerRadius = param;
		} break;

		case SONAR_RADIUS: {
			unit->sonarRadius = param;
		} break;

		case SONAR_JAM_RADIUS: {
			unit->sonarJamRadius = param;
		} break;

		case SEISMIC_RADIUS: {
			unit->seismicRadius = param;
		} break;

		case CURRENT_FUEL: { // deprecated
		} break;

		case SHIELD_POWER: {
			if (unit->shieldWeapon != nullptr) {
				CPlasmaRepulser* shield = static_cast<CPlasmaRepulser*>(unit->shieldWeapon);
				shield->SetCurPower(std::max(0.0f, float(param) / float(COBSCALE)));
			}
		} break;

		case STEALTH: {
			unit->stealth = !!param;
		} break;

		case SONAR_STEALTH: {
			unit->sonarStealth = !!param;
		} break;

		case CRASHING: {
			AAirMoveType* amt = dynamic_cast<AAirMoveType*>(unit->moveType);
			if (amt != nullptr) {
				if (!!param) {
					amt->SetState(AAirMoveType::AIRCRAFT_CRASHING);
				} else {
					amt->SetState(AAirMoveType::AIRCRAFT_FLYING);
				}
			}
		} break;

		case CHANGE_TARGET: {
			if (param <                     0) { return; }
			if (param >= unit->weapons.size()) { return; }

			unit->weapons[param]->avoidTarget = true;
		} break;

		case CEG_DAMAGE: {
			unit->cegDamage = param;
		} break;

		case FLANK_B_MODE: {
			unit->flankingBonusMode = param;
		} break;
		case FLANK_B_MOBILITY_ADD: {
			unit->flankingBonusMobilityAdd = (param / (float)COBSCALE);
		} break;
		case FLANK_B_MAX_DAMAGE: {
			float mindamage = unit->flankingBonusAvgDamage - unit->flankingBonusDifDamage;
			unit->flankingBonusAvgDamage = (param / (float)COBSCALE + mindamage)*0.5f;
			unit->flankingBonusDifDamage = (param / (float)COBSCALE - mindamage)*0.5f;
		} break;

		case FLANK_B_MIN_DAMAGE: {
			float maxdamage = unit->flankingBonusAvgDamage + unit->flankingBonusDifDamage;
			unit->flankingBonusAvgDamage = (maxdamage + param / (float)COBSCALE)*0.5f;
			unit->flankingBonusDifDamage = (maxdamage - param / (float)COBSCALE)*0.5f;
		} break;

		default: {
			if ((val >= GLOBAL_VAR_START) && (val <= GLOBAL_VAR_END)) {
				ShowUnitScriptError("[US::SetUnitVal] COB global vars are deprecated");
			}
			else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
				ShowUnitScriptError("[US::SetUnitVal] COB team vars are deprecated");
			}
			else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
				ShowUnitScriptError("[US::SetUnitVal] COB allyteam vars are deprecated");
			}
			else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
				ShowUnitScriptError("[US::SetUnitVal] COB unit vars are deprecated");
			}
			else {
				ShowUnitScriptError("[US::SetUnitVal] CobError: unknown set-constant " + IntToString(val));
			}
		}
	}
#endif
}

/******************************************************************************/
/******************************************************************************/

int CUnitScript::ScriptToModel(int scriptPieceNum) const {
	if (!PieceExists(scriptPieceNum))
		return -1;

	const LocalModelPiece* smp = GetScriptLocalModelPiece(scriptPieceNum);

	return (smp->GetLModelPieceIndex());
}

int CUnitScript::ModelToScript(int lmodelPieceNum) const {
	LocalModel& lm = unit->localModel;

	if (!lm.HasPiece(lmodelPieceNum))
		return -1;

	const LocalModelPiece* lmp = lm.GetPiece(lmodelPieceNum);

	return (lmp->GetScriptPieceIndex());
}

void CUnitScript::ShowUnitScriptError(const std::string& error)
{
	if (unit == nullptr) {
		ShowScriptError("unitID=null error=\"" + error + "\"");
	} else {
		ShowScriptError("unitID=" + IntToString(unit->id) + " defName=" + unit->unitDef->name + " error=\"" + error + "\"");
	}
}

