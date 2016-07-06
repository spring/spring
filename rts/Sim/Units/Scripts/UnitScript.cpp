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
#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncTracer.h"

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
	if (HaveAnimations())
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

	if (delta > 0.0f) {
		cur += speed;
	} else {
		cur -= speed;
	}

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

	// clamp: -pi .. 0 .. +pi (remainder(x,TWOPI) would do the same but is slower due to streflop)
	if (delta > PI) {
		delta -= TWOPI;
	} else if (delta<=-PI) {
		delta += TWOPI;
	}

	if (math::fabsf(delta) <= speed) {
		cur = dest;
		return true;
	}

	if (delta > 0.0f) {
		cur += speed;
	} else {
		cur -= speed;
	}

	ClampRad(&cur);

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
bool CUnitScript::DoSpin(float& cur, float dest, float &speed, float accel, int divisor)
{
	const float delta = dest - speed;

	// Check if we are not at the final speed and
	// make sure we dont go past desired speed
	if (math::fabsf(delta) <= accel) {
		speed = dest;
		if (speed == 0.0f)
			return true;
	}
	else {
		if (delta > 0.0f) {
			// accelerations are defined in speed/frame (at GAME_SPEED fps)
			speed += accel * (float(GAME_SPEED) / divisor);
		} else {
			speed -= accel * (float(GAME_SPEED) / divisor);
		}
	}

	cur += (speed / divisor);
	ClampRad(&cur);

	return false;
}



void CUnitScript::TickAnims(int deltaTime, AnimType type, std::vector<AnimInfo>& doneAnims) {
	switch (type) {
		case AMove: {
			int i = 0;
			while (i < anims[type].size()) {
				AnimInfo& ai = anims[type][i];
				const int piece = ai.piece;

				// NOTE: we should not need to copy-and-set here, because
				// MoveToward/TurnToward/DoSpin modify pos/rot by reference
				float3 pos = pieces[piece]->GetPosition();

				if (MoveToward(pos[ai.axis], ai.dest, ai.speed / (1000 / deltaTime))) {
					ai.done = true;
					if (ai.hasWaiting)
						doneAnims.push_back(ai);

					ai = anims[type].back();
					anims[type].pop_back();
				} else {
					++i;
				}

				pieces[piece]->SetPosition(pos);
			}
		} break;

		case ATurn: {
			int i = 0;
			while (i < anims[type].size()) {
				AnimInfo& ai = anims[type][i];
				const int piece = ai.piece;
				float3 rot = pieces[piece]->GetRotation();

				if (TurnToward(rot[ai.axis], ai.dest, ai.speed / (1000 / deltaTime))) {
					ai.done = true;
					if (ai.hasWaiting)
						doneAnims.push_back(ai);

					ai = anims[type].back();
					anims[type].pop_back();
				} else {
					++i;
				}

				pieces[piece]->SetRotation(rot);
			}
		} break;

		case ASpin: {
			int i = 0;
			while (i < anims[type].size()) {
				AnimInfo& ai = anims[type][i];
				const int piece = ai.piece;
				float3 rot = pieces[piece]->GetRotation();

				if (DoSpin(rot[ai.axis], ai.dest, ai.speed, ai.accel, 1000 / deltaTime)) {
					ai.done = true;
					if (ai.hasWaiting)
						doneAnims.push_back(ai);

					ai = anims[type].back();
					anims[type].pop_back();
				} else {
					++i;
				}

				pieces[piece]->SetRotation(rot);
			}
		} break;

		default: {
		} break;
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
	static std::vector<AnimInfo> doneAnims[AMove + 1];

	for (int animType = ATurn; animType <= AMove; animType++) {
		TickAnims(deltaTime, AnimType(animType), doneAnims[animType]);
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
	for (auto it = anims[type].begin(); it != anims[type].end(); ++it) {
		if (((*it).piece == piece) && ((*it).axis == axis))
			return it;
	}

	return anims[type].end();
}

void CUnitScript::RemoveAnim(AnimType type, const AnimContainerTypeIt& animInfoIt)
{
	if (animInfoIt == anims[type].end())
		return;

	AnimInfo& ai = *animInfoIt;

	//! We need to unblock threads waiting on this animation, otherwise they will be lost in the void
	//! NOTE: AnimFinished might result in new anims being added
	if (ai.hasWaiting)
		AnimFinished(type, ai.piece, ai.axis);

	ai = anims[type].back();
	anims[type].pop_back();

	// If this was the last animation, remove from currently animating list
	// FIXME: this could be done in a cleaner way
	if (!HaveAnimations()) {
		unitScriptEngine->RemoveInstance(this);
	}
}


//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CUnitScript::AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("Invalid piecenumber");
		return;
	}

	float destf = 0.0f;

	if (type == AMove) {
		destf = pieces[piece]->original->offset[axis] + dest;
	} else {
		destf = dest;
		if (type == ATurn) {
			ClampRad(&destf);
		}
	}

	AnimContainerTypeIt animInfoIt;
	AnimInfo* ai = NULL;
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
		if (!HaveAnimations()) {
			unitScriptEngine->AddInstance(this);
		}

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

	//If we are already spinning, we may have to decelerate to the new speed
	if (animInfoIt != anims[ASpin].end()) {
		AnimInfo* ai = &(*animInfoIt);
		ai->dest = speed;

		if (accel > 0) {
			ai->accel = accel;
		} else {
			//Go there instantly. Or have a defaul accel?
			ai->speed = speed;
			ai->accel = 0;
		}
	} else {
		//No accel means we start at desired speed instantly
		if (accel <= 0)
			AddAnim(ASpin, piece, axis, speed, speed, 0);
		else
			AddAnim(ASpin, piece, axis, 0, speed, accel);
	}
}


void CUnitScript::StopSpin(int piece, int axis, float decel)
{
	auto animInfoIt = FindAnim(ASpin, piece, axis);

	if (decel <= 0) {
		RemoveAnim(ASpin, animInfoIt);
	} else {
		if (animInfoIt == anims[ASpin].end())
			return;

		AnimInfo* ai = &(*animInfoIt);
		ai->dest = 0;
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
		ShowUnitScriptError("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];

	float3 pos = p->GetPosition();
	pos[axis] = pieces[piece]->original->offset[axis] + destination;

	p->SetPosition(pos);
}


void CUnitScript::TurnNow(int piece, int axis, float destination)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("Invalid piecenumber");
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
		ShowUnitScriptError("Invalid piecenumber");
		return;
	}

	pieces[piece]->scriptSetVisible = visible;
}

void CUnitScript::EmitSfx(int sfxType, int piece)
{
#ifndef _CONSOLE
	if (!PieceExists(piece)) {
		ShowUnitScriptError("Invalid piecenumber for emit-sfx");
		return;
	}

	if (projectileHandler->GetParticleSaturation() > 1.0f && sfxType < SFX_CEG) {
		// skip adding (unsynced!) particles when we have too many
		return;
	}

	// Make sure wakes are only emitted on water
	if ((sfxType >= SFX_WAKE) && (sfxType <= SFX_REVERSE_WAKE_2)) {
		if (CGround::GetApproximateHeight(unit->pos.x, unit->pos.z) > 0.0f) {
			return;
		}
	}

	float3 relPos = ZeroVector;
	float3 relDir = UpVector;

	if (!GetEmitDirPos(piece, relPos, relDir)) {
		ShowUnitScriptError("emit-sfx: GetEmitDirPos failed");
		return;
	}

	relDir.SafeNormalize();

	const float3 pos = unit->GetObjectSpacePos(relPos);
	const float3 dir = unit->GetObjectSpaceVec(relDir);

	float alpha = 0.3f + gu->RandFloat() * 0.2f;
	float alphaFalloff = 0.004f;
	float fadeupTime = 4;

	const UnitDef* ud = unit->unitDef;
	const MoveDef* md = unit->moveDef;

	// hovercraft need special care
	if (md != NULL && md->speedModClass == MoveDef::Hover) {
		fadeupTime = 8.0f;
		alpha = 0.15f + gu->RandFloat() * 0.2f;
		alphaFalloff = 0.008f;
	}

	switch (sfxType) {
		case SFX_REVERSE_WAKE:
		case SFX_REVERSE_WAKE_2: {  //reverse wake
			new CWakeProjectile(
				unit,
				pos + gu->RandVector() * 2.0f,
				dir * 0.4f,
				6.0f + gu->RandFloat() * 4.0f,
				0.15f + gu->RandFloat() * 0.3f,
				alpha, alphaFalloff, fadeupTime
			);
			break;
		}

		case SFX_WAKE_2:  //wake 2, in TA it lives longer..
		case SFX_WAKE: {  //regular ship wake
			new CWakeProjectile(
				unit,
				pos + gu->RandVector() * 2.0f,
				dir * 0.4f,
				6.0f + gu->RandFloat() * 4.0f,
				0.15f + gu->RandFloat() * 0.3f,
				alpha, alphaFalloff, fadeupTime
			);
			break;
		}

		case SFX_BUBBLE: {  //submarine bubble. does not provide direction through piece vertices..
			float3 pspeed = gu->RandVector() * 0.1f;
				pspeed.y += 0.2f;

			new CBubbleProjectile(
				unit,
				pos + gu->RandVector() * 2.0f,
				pspeed,
				40.0f + gu->RandFloat() * GAME_SPEED,
				1.0f + gu->RandFloat() * 2.0f,
				0.01f,
				0.3f + gu->RandFloat() * 0.3f
			);
		} break;

		case SFX_WHITE_SMOKE:  //damaged unit smoke
			new CSmokeProjectile(unit, pos, gu->RandVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, 0.5f);
			break;
		case SFX_BLACK_SMOKE:  //damaged unit smoke
			new CSmokeProjectile(unit, pos, gu->RandVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, 0.6f);
			break;
		case SFX_VTOL: {
			const float3 speed =
				unit->speed    * 0.7f +
				unit->frontdir * 0.5f *       relDir.z  +
				unit->updir    * 0.5f * -math::fabs(relDir.y) +
				unit->rightdir * 0.5f *       relDir.x;

			CHeatCloudProjectile* hc = new CHeatCloudProjectile(
				unit,
				pos,
				speed,
				10 + gu->RandFloat() * 5,
				3 + gu->RandFloat() * 2
			);
			hc->size = 3;
			break;
		}
		default: {
			if (sfxType & SFX_CEG) {
				// emit defined explosion-generator (can only be custom, not standard)
				// index is made valid by callee, an ID of -1 means CEG failed to load
				explGenHandler->GenExplosion(ud->GetModelExplosionGeneratorID(sfxType - SFX_CEG), pos, dir, unit->cegDamage, 1.0f, 0.0f, unit, NULL);
			}
			else if (sfxType & SFX_FIRE_WEAPON) {
				// make a weapon fire from the piece
				const unsigned index = sfxType - SFX_FIRE_WEAPON;
				if (index >= unit->weapons.size()) {
					ShowUnitScriptError("Invalid weapon index for emit-sfx");
					break;
				}
				CWeapon* w = unit->weapons[index];
				const SWeaponTarget origTarget = w->GetCurrentTarget();
				const float3 origWeaponMuzzlePos = w->weaponMuzzlePos;
				w->SetAttackTarget(SWeaponTarget(pos + dir));
				w->weaponMuzzlePos = pos;
				w->Fire(true);
				w->weaponMuzzlePos = origWeaponMuzzlePos;
				bool origRestored = w->Attack(origTarget);
				assert(origRestored);
			}
			else if (sfxType & SFX_DETONATE_WEAPON) {
				const unsigned index = sfxType - SFX_DETONATE_WEAPON;
				if (index >= unit->weapons.size()) {
					ShowUnitScriptError("Invalid weapon index for emit-sfx");
					break;
				}

				// detonate weapon from piece
				const CWeapon* weapon = unit->weapons[index];
				const WeaponDef* weaponDef = weapon->weaponDef;

				CExplosionParams params = {
					pos,
					ZeroVector,
					*weapon->damages,
					weaponDef,
					unit,                              // owner
					NULL,                              // hitUnit
					NULL,                              // hitFeature
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


#endif
}


void CUnitScript::AttachUnit(int piece, int u)
{
	// -1 is valid, indicates that the unit should be hidden
	if ((piece >= 0) && (!PieceExists(piece))) {
		ShowUnitScriptError("Invalid piecenumber for attach");
		return;
	}

#ifndef _CONSOLE
	if (unit->unitDef->IsTransportUnit() && unitHandler->units[u]) {
		unit->AttachUnit(unitHandler->units[u], piece);
	}
#endif
}


void CUnitScript::DropUnit(int u)
{
#ifndef _CONSOLE
	if (unit->unitDef->IsTransportUnit() && unitHandler->units[u]) {
		unit->DetachUnit(unitHandler->units[u]);
	}
#endif
}


//Returns true if there was an animation to listen to
bool CUnitScript::NeedsWait(AnimType type, int piece, int axis)
{
	auto animInfoIt = FindAnim(type, piece, axis);

	if (animInfoIt != anims[type].end()) {
		AnimInfo* ai = &(*animInfoIt);

		if (!ai->done) {
			ai->hasWaiting = true;
			return true;
		}

		// if the animation is already finished, listening for
		// it just adds some overhead since either the current
		// or the next Tick will remove it and call UnblockAll
		// (which calls AnimFinished for each listener)
		//
		// we could notify the listener here, but a cleaner way
		// is to treat the animation as if it did not exist and
		// simply disregard the WaitFor* (no side-effects)
		//
		// if (ai->hasWaiting)
		// 		AnimFinished(ai->type, ai->piece, ai->axis);
	}

	return false;
}


//Flags as defined by the cob standard
void CUnitScript::Explode(int piece, int flags)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("Invalid piecenumber for explode");
		return;
	}

#ifndef _CONSOLE
	const float3 relPos = GetPiecePos(piece);
	const float3 absPos = unit->GetObjectSpacePos(relPos);

#ifdef TRACE_SYNC
	tracefile << "Cob explosion: ";
	tracefile << absPos.x << " " << absPos.y << " " << absPos.z << " " << piece << " " << flags << "\n";
#endif

	if (!(flags & PF_NoHeatCloud)) {
		// Do an explosion at the location first
		new CHeatCloudProjectile(nullptr, absPos, ZeroVector, 30, 30);
	}

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
	explSpeed.x = (0.5f - gs->randFloat()) * 6.0f;
	explSpeed.y = 1.2f + (gs->randFloat() * 5.0f);
	explSpeed.z = (0.5f - gs->randFloat()) * 6.0f;

	if (unit->pos.y - CGround::GetApproximateHeight(unit->pos.x, unit->pos.z) > 15)
		explSpeed.y = (0.5f - gs->randFloat()) * 6.0f;

	if (baseSpeed.SqLength() > 9.0f) {
		const float l  = baseSpeed.Length();
		const float l2 = 3.0f + math::sqrt(l - 3.0f);
		baseSpeed *= (l2 / l);
	}

	explSpeed += baseSpeed;

	const float partSat = projectileHandler->GetParticleSaturation();

	int newFlags = 0;
	newFlags |= (PF_Explode    *  ((flags & PF_Explode   ) != 0)                    );
	newFlags |= (PF_Smoke      * (((flags & PF_Smoke     ) != 0) && partSat < 0.95f));
	newFlags |= (PF_Fire       * (((flags & PF_Fire      ) != 0) && partSat < 0.95f));
	newFlags |= (PF_NoCEGTrail *  ((flags & PF_NoCEGTrail) != 0)                    );
	newFlags |= (PF_Recursive  *  ((flags & PF_Recursive ) != 0)                    );

	new CPieceProjectile(unit, pieces[piece], absPos, explSpeed, newFlags, 0.5f);
#endif
}


void CUnitScript::Shatter(int piece, const float3& pos, const float3& speed)
{
	const LocalModelPiece* lmp = pieces[piece];
	const S3DModelPiece* omp = lmp->original;

	if (!omp->HasGeometryData())
		return;

	const float pieceChance = 1.0f - (projectileHandler->GetCurrentParticles() - (projectileHandler->maxParticles - 2000)) / 2000.0f;
	if (pieceChance > 0.0f) {
		const CMatrix44f m = unit->GetTransformMatrix() * lmp->GetModelSpaceMatrix();
		omp->Shatter(pieceChance, unit->model->type, unit->model->textureType, unit->team, pos, speed, m);
	}
}


void CUnitScript::ShowFlare(int piece)
{
	if (!PieceExists(piece)) {
		ShowUnitScriptError("Invalid piecenumber for show(flare)");
		return;
	}
#ifndef _CONSOLE
	const float3 relPos = GetPiecePos(piece);
	const float3 absPos = unit->GetObjectSpacePos(relPos);
	const float3 dir = unit->lastMuzzleFlameDir;
	const float size = unit->lastMuzzleFlameSize;

	new CMuzzleFlame(absPos, unit->speed, dir, size);
#endif
}


/******************************************************************************/
int CUnitScript::GetUnitVal(int val, int p1, int p2, int p3, int p4)
{
	// may happen in case one uses Spring.GetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (!unit) {
		ShowUnitScriptError("no unit (in GetUnitVal)");
		return 0;
	}

#ifndef _CONSOLE
	switch (val)
	{
	case ACTIVATION:
		if (unit->activated)
			return 1;
		else
			return 0;
		break;
	case STANDINGMOVEORDERS:
		return unit->moveState;
		break;
	case STANDINGFIREORDERS:
		return unit->fireState;
		break;
	case HEALTH: {
		if (p1 <= 0)
			return int((unit->health / unit->maxHealth) * 100.0f);

		const CUnit* u = unitHandler->GetUnit(p1);

		if (u == NULL)
			return 0;
		else
			return int((u->health / u->maxHealth) * 100.0f);
	}
	case INBUILDSTANCE:
		if (unit->inBuildStance)
			return 1;
		else
			return 0;
	case BUSY:
		if (busy)
			return 1;
		else
			return 0;
		break;
	case PIECE_XZ: {
		if (!PieceExists(p1)) {
			ShowUnitScriptError("Invalid piecenumber for get piece_xz");
			break;
		}
		const float3 relPos = GetPiecePos(p1);
		const float3 absPos = unit->GetObjectSpacePos(relPos);
		return PACKXZ(absPos.x, absPos.z);
	}
	case PIECE_Y: {
		if (!PieceExists(p1)) {
			ShowUnitScriptError("Invalid piecenumber for get piece_y");
			break;
		}
		const float3 relPos = GetPiecePos(p1);
		const float3 absPos = unit->GetObjectSpacePos(relPos);
		return int(absPos.y * COBSCALE);
	}
	case UNIT_XZ: {
		if (p1 <= 0)
			return PACKXZ(unit->pos.x, unit->pos.z);

		const CUnit* u = unitHandler->GetUnit(p1);

		if (u == NULL)
			return PACKXZ(0, 0);
		else
			return PACKXZ(u->pos.x, u->pos.z);
	}
	case UNIT_Y: {
		if (p1 <= 0)
			return int(unit->pos.y * COBSCALE);

		const CUnit* u = unitHandler->GetUnit(p1);

		if (u == NULL)
			return 0;
		else
			return int(u->pos.y * COBSCALE);
	}
	case UNIT_HEIGHT: {
		if (p1 <= 0)
			return int(unit->radius * COBSCALE);

		const CUnit* u = unitHandler->GetUnit(p1);

		if (u == NULL)
			return 0;
		else
			return int(u->radius * COBSCALE);
	}
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
		return unitHandler->MaxUnits()-1;
	case MY_ID:
		return unit->id;

	case UNIT_TEAM: {
		const CUnit* u = unitHandler->GetUnit(p1);
		return (u != NULL)? unit->team : 0;
	}
	case UNIT_ALLIED: {
		const CUnit* u = unitHandler->GetUnit(p1);

		if (u != NULL) {
			return teamHandler->Ally(unit->allyteam, u->allyteam) ? 1 : 0;
		}

		return 0;
	}
	case UNIT_BUILD_PERCENT_LEFT: {
		const CUnit* u = unitHandler->GetUnit(p1);
		if (u != NULL) {
			return int((1.0f - u->buildProgress) * 100);
		}
		return 0;
	}
	case MAX_SPEED: {
		return int(unit->moveType->GetMaxSpeed() * COBSCALE);
	} break;
	case REVERSING: {
		CGroundMoveType* gmt = dynamic_cast<CGroundMoveType*>(unit->moveType);
		return ((gmt != NULL)? int(gmt->IsReversing()): 0);
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
		if (p1 <= 0) {
			return unit->heading;
		}

		const CUnit* u = unitHandler->GetUnit(p1);

		if (u != NULL) {
			return u->heading;
		}
		return -1;
	}
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
	}

	case LAST_ATTACKER_ID:
		return unit->lastAttacker? unit->lastAttacker->id: -1;
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

	case DO_SEISMIC_PING:
		float pingSize;
		if (p1 == 0) {
			pingSize = unit->seismicSignature;
		} else {
			pingSize = p1;
		}
		unit->DoSeismicPing(pingSize);
		break;

	case CURRENT_FUEL: //deprecated
		return 0;
	case TRANSPORT_ID:
		return unit->transporter?unit->transporter->id:-1;

	case SHIELD_POWER: {
		const CWeapon* shield = unit->shieldWeapon;

		if (shield == NULL)
			return -1;

		return int(static_cast<const CPlasmaRepulser*>(shield)->GetCurPower() * float(COBSCALE));
	}

	case STEALTH: {
		return unit->stealth ? 1 : 0;
	}
	case SONAR_STEALTH: {
		return unit->sonarStealth ? 1 : 0;
	}
	case CRASHING:
		return !!unit->IsCrashing();

	case COB_ID: {
		if (p1 <= 0) {
			return unit->unitDef->cobID;
		} else {
			const CUnit* u = unitHandler->GetUnit(p1);
			return ((u == NULL)? -1 : u->unitDef->cobID);
		}
	}

 	case PLAY_SOUND: {
 		// FIXME: this can currently only work for CCobInstance, because Lua can not get sound IDs
 		// (however, for Lua scripts there is already LuaUnsyncedCtrl::PlaySoundFile)
 		CCobInstance* cob = dynamic_cast<CCobInstance*>(this);
 		if (cob == NULL) {
 			return 1;
 		}
 		const CCobFile* script = cob->GetScriptAddr();
 		if (script == NULL) {
 			return 1;
 		}
		if ((p1 < 0) || (static_cast<size_t>(p1) >= script->sounds.size())) {
			return 1;
		}
		switch (p3) {	//who hears the sound
			case 0:		//ALOS
				if (!losHandler->InAirLos(unit,gu->myAllyTeam)) { return 0; }
				break;
			case 1:		//LOS
				if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS)) { return 0; }
				break;
			case 2:		//ALOS or radar
				if (!(losHandler->InAirLos(unit,gu->myAllyTeam) || unit->losStatus[gu->myAllyTeam] & (LOS_INRADAR))) { return 0; }
				break;
			case 3:		//LOS or radar
				if (!(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR))) { return 0; }
				break;
			case 4:		//everyone
				break;
			case 5:		//allies
				if (unit->allyteam != gu->myAllyTeam) { return 0; }
				break;
			case 6:		//team
				if (unit->team != gu->myTeam) { return 0; }
				break;
			case 7:		//enemies
				if (unit->allyteam == gu->myAllyTeam) { return 0; }
				break;
		}
		if (p4 == 0) {
			Channels::General->PlaySample(script->sounds[p1], unit->pos, unit->speed, float(p2) / COBSCALE);
		} else {
			Channels::General->PlaySample(script->sounds[p1], float(p2) / COBSCALE);
		}
		return 0;
	}
	case SET_WEAPON_UNIT_TARGET: {
		const unsigned int weaponID = p1 - 1;
		const unsigned int targetID = p2;
		const bool userTarget = !!p3;

		if (weaponID >= unit->weapons.size()) {
			return 0;
		}

		CWeapon* weapon = unit->weapons[weaponID];

		if (weapon == NULL) {
			return 0;
		}

		//! if targetID is 0, just sets weapon->haveUserTarget
		//! to false (and targetType to None) without attacking
		CUnit* target = (targetID > 0)? unitHandler->GetUnit(targetID): NULL;
		return (weapon->Attack(SWeaponTarget(target, userTarget)) ? 1 : 0);
	}
	case SET_WEAPON_GROUND_TARGET: {
		const int weaponID = p1 - 1;
		const float3 pos = float3(float(UNPACKX(p2)),
		                          float(p3) / float(COBSCALE),
		                          float(UNPACKZ(p2)));
		const bool userTarget = !!p4;
		if ((weaponID < 0) || (static_cast<size_t>(weaponID) >= unit->weapons.size())) {
			return 0;
		}
		CWeapon* weapon = unit->weapons[weaponID];
		if (weapon == NULL) { return 0; }

		return weapon->Attack(SWeaponTarget(pos, userTarget)) ? 1 : 0;
	}
	case MIN:
		return std::min(p1, p2);
	case MAX:
		return std::max(p1, p2);
	case ABS:
		return abs(p1);
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
			case 4: unit->flankingBonusDir.x = (p2/(float)COBSCALE); return 0;
			case 5: unit->flankingBonusDir.y = (p2/(float)COBSCALE); return 0;
			case 6: unit->flankingBonusDir.z = (p2/(float)COBSCALE); return 0;
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
		//! ID 0 is reserved for the script's owner
		CUnit* u = (p1 > 0)? unitHandler->GetUnit(p1): this->unit;

		if (u == NULL) {
			return 0;
		}

		if (u->beingBuilt) {
			// no explosions and no corpse for units under construction
			u->KillUnit(NULL, false, true);
		} else {
			u->KillUnit(NULL, p2 != 0, p3 != 0);
		}

		return 1;
	}


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
	}
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
	}
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
	}
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
	}
	case WEAPON_RANGE: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->range * COBSCALE);

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->range * COBSCALE;
			w->range = float(p2) / COBSCALE;
			return old;
		}

		return -1;
	}
	case WEAPON_PROJECTILE_SPEED: {
		const int np1 = -p1;

		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size())
			return int(unit->weapons[p1 - 1]->projectileSpeed * COBSCALE);

		if (np1 > 0 && static_cast<size_t>(np1) <= unit->weapons.size()) {
			CWeapon* w = unit->weapons[np1 - 1];
			const int old = w->projectileSpeed * COBSCALE;
			w->projectileSpeed = float(p2) / COBSCALE;
			return old;
		}

		return -1;
	}


	case GAME_FRAME: {
		return gs->frameNum;
	}
	default:
		if ((val >= GLOBAL_VAR_START) && (val <= GLOBAL_VAR_END)) {
			ShowUnitScriptError("cob global vars are deprecated");
			return 0;
		}
		else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
			ShowUnitScriptError("cob team vars are deprecated");
			return 0;
		}
		else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
			ShowUnitScriptError("cob allyteam vars are deprecated");
			return 0;
		}
		else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
			ShowUnitScriptError("cob unit vars are deprecated");
			return 0;
		}
		else {
			ShowUnitScriptError("CobError: Unknown get constant " + IntToString(val) + " (params = " + IntToString(p1) + " " +
			IntToString(p2) + " " + IntToString(p3) + " " + IntToString(p4) + ")");
		}
	}
#endif

	return 0;
}


void CUnitScript::SetUnitVal(int val, int param)
{
	// may happen in case one uses Spring.SetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (!unit) {
		ShowUnitScriptError("Error: no unit (in SetUnitVal)");
		return;
	}

#ifndef _CONSOLE
	switch (val) {
		case ACTIVATION: {
			if(unit->unitDef->onoffable) {
				Command c(CMD_ONOFF, 0, (param == 0) ? 0 : 1);
				unit->commandAI->GiveCommand(c);
			}
			else {
				if(param == 0) {
					unit->Deactivate();
				}
				else {
					unit->Activate();
				}
			}
			break;
		}
		case STANDINGMOVEORDERS: {
			if (param >= 0 && param <= 2) {
				Command c(CMD_MOVE_STATE, 0, param);
				unit->commandAI->GiveCommand(c);
			}
			break;
		}
		case STANDINGFIREORDERS: {
			if (param >= 0 && param <= 2) {
				Command c(CMD_FIRE_STATE, 0, param);
				unit->commandAI->GiveCommand(c);
			}
			break;
		}
		case HEALTH: {
			break;
		}
		case INBUILDSTANCE: {
			unit->inBuildStance = (param != 0);
			break;
		}
		case BUSY: {
			busy = (param != 0);
			break;
		}
		case PIECE_XZ: {
			break;
		}
		case PIECE_Y: {
			break;
		}
		case UNIT_XZ: {
			break;
		}
		case UNIT_Y: {
			break;
		}
		case UNIT_HEIGHT: {
			break;
		}
		case XZ_ATAN: {
			break;
		}
		case XZ_HYPOT: {
			break;
		}
		case ATAN: {
			break;
		}
		case HYPOT: {
			break;
		}
		case GROUND_HEIGHT: {
			break;
		}
		case GROUND_WATER_HEIGHT: {
			break;
		}
		case BUILD_PERCENT_LEFT: {
			break;
		}
		case YARD_OPEN: {
			if (unit->blockMap != NULL) {
				// note: if this unit is a factory, engine-controlled
				// OpenYard() and CloseYard() calls can interfere with
				// the yardOpen state (they probably should be removed
				// at some point)
				if (param == 0) {
					if (groundBlockingObjectMap->CanCloseYard(unit)) {
						groundBlockingObjectMap->CloseBlockingYard(unit);
					}
				} else {
					if (groundBlockingObjectMap->CanOpenYard(unit)) {
						groundBlockingObjectMap->OpenBlockingYard(unit);
					}
				}
			}
			break;
		}
		case BUGGER_OFF: {
			if (param != 0) {
				CGameHelper::BuggerOff(unit->pos + unit->frontdir * unit->radius, unit->radius * 1.5f, true, false, unit->team, NULL);
			}
			break;
		}
		case ARMORED: {
			if (param) {
				unit->curArmorMultiple = unit->armoredMultiple;
			} else {
				unit->curArmorMultiple = 1;
			}
			unit->armoredState = (param != 0);
			break;
		}
		case VETERAN_LEVEL: {
			unit->experience = param * 0.01f;
			break;
		}
		case MAX_SPEED: {
			// interpret negative values as non-persistent changes
			unit->commandAI->SetScriptMaxSpeed(std::max(param, -param) / float(COBSCALE), (param >= 0));
			break;
		}
		case CLOAKED: {
			unit->wantCloak = !!param;
			break;
		}
		case WANT_CLOAK: {
			unit->wantCloak = !!param;
			break;
		}
		case UPRIGHT: {
			unit->upright = !!param;
			break;
		}
		case HEADING: {
			unit->heading = param % COBSCALE;
			unit->UpdateDirVectors(!unit->upright);
			unit->UpdateMidAndAimPos();
		} break;
		case LOS_RADIUS: {
			unit->ChangeLos(param, unit->realAirLosRadius);
			unit->realLosRadius = param;
			break;
		}
		case AIR_LOS_RADIUS: {
			unit->ChangeLos(unit->realLosRadius, param);
			unit->realAirLosRadius = param;
			break;
		}
		case RADAR_RADIUS: {
			unit->radarRadius = param;
			break;
		}
		case JAMMER_RADIUS: {
			unit->jammerRadius = param;
			break;
		}
		case SONAR_RADIUS: {
			unit->sonarRadius = param;
			break;
		}
		case SONAR_JAM_RADIUS: {
			unit->sonarJamRadius = param;
			break;
		}
		case SEISMIC_RADIUS: {
			unit->seismicRadius = param;
			break;
		}
		case CURRENT_FUEL: { //deprecated
			break;
		}
		case SHIELD_POWER: {
			if (unit->shieldWeapon != NULL) {
				CPlasmaRepulser* shield = static_cast<CPlasmaRepulser*>(unit->shieldWeapon);
				shield->SetCurPower(std::max(0.0f, float(param) / float(COBSCALE)));
			}
			break;
		}
		case STEALTH: {
			unit->stealth = !!param;
			break;
		}
		case SONAR_STEALTH: {
			unit->sonarStealth = !!param;
			break;
		}
		case CRASHING: {
			AAirMoveType* amt = dynamic_cast<AAirMoveType*>(unit->moveType);
			if (amt != NULL) {
				if (!!param) {
					amt->SetState(AAirMoveType::AIRCRAFT_CRASHING);
				} else {
					amt->SetState(AAirMoveType::AIRCRAFT_FLYING);
				}
			}
			break;
		}
		case CHANGE_TARGET: {
			if (param <                     0) { return; }
			if (param >= unit->weapons.size()) { return; }

			unit->weapons[param]->avoidTarget = true;
			break;
		}
		case CEG_DAMAGE: {
			unit->cegDamage = param;
			break;
		}
		case FLANK_B_MODE:
			unit->flankingBonusMode = param;
			break;
		case FLANK_B_MOBILITY_ADD:
			unit->flankingBonusMobilityAdd = (param / (float)COBSCALE);
			break;
		case FLANK_B_MAX_DAMAGE: {
			float mindamage = unit->flankingBonusAvgDamage - unit->flankingBonusDifDamage;
			unit->flankingBonusAvgDamage = (param / (float)COBSCALE + mindamage)*0.5f;
			unit->flankingBonusDifDamage = (param / (float)COBSCALE - mindamage)*0.5f;
			break;
		 }
		case FLANK_B_MIN_DAMAGE: {
			float maxdamage = unit->flankingBonusAvgDamage + unit->flankingBonusDifDamage;
			unit->flankingBonusAvgDamage = (maxdamage + param / (float)COBSCALE)*0.5f;
			unit->flankingBonusDifDamage = (maxdamage - param / (float)COBSCALE)*0.5f;
			break;
		}
		default: {
			if ((val >= GLOBAL_VAR_START) && (val <= GLOBAL_VAR_END)) {
				ShowUnitScriptError("cob global vars are deprecated");
			}
			else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
				ShowUnitScriptError("cob team vars are deprecated");
			}
			else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
				ShowUnitScriptError("cob allyteam vars are deprecated");
			}
			else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
				ShowUnitScriptError("cob unit vars are deprecated");
			}
			else {
				ShowUnitScriptError("CobError: Unknown set constant " + IntToString(val));
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
	ShowScriptError(unit ? error + std::string(" ") + IntToString(unit->id) + std::string(" of type ") + unit->unitDef->name
						 : error + std::string(" -> Unit doesn't have a script!"));
}

