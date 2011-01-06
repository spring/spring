/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobInstance.cpp */

#include "UnitScript.h"

#include "CobFile.h"
#include "CobInstance.h"
#include "UnitScriptEngine.h"

#ifndef _CONSOLE

#include <SDL_timer.h>
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/BubbleProjectile.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/MuzzleFlame.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/WakeProjectile.h"
#include "Sim/Projectiles/Unsynced/WreckProjectile.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Sound/IEffectChannel.h"
#include "myMath.h"
#include "FastMath.h"
#include "Sync/SyncTracer.h"
#include "CobDefines.h"

#endif


std::vector< std::vector<int> > CUnitScript::teamVars;
std::vector< std::vector<int> > CUnitScript::allyVars;
int CUnitScript::globalVars[GLOBAL_VAR_COUNT] =  { 0 };


void CUnitScript::InitVars(int numTeams, int numAllyTeams)
{
	teamVars.resize(numTeams, std::vector<int>());
	for (int t = 0; t < numTeams; t++) {
		teamVars[t].resize(TEAM_VAR_COUNT, 0);
	}

	allyVars.resize(numAllyTeams, std::vector<int>());
	for (int t = 0; t < numAllyTeams; t++) {
		allyVars[t].resize(ALLY_VAR_COUNT, 0);
	}
}


CUnitScript::CUnitScript(CUnit* unit, const std::vector<LocalModelPiece*>& pieces)
	: unit(unit)
	, yardOpen(false)
	, busy(false)
	, hasSetSFXOccupy(false)
	, hasRockUnit(false)
	, hasStartBuilding(false)
	, pieces(pieces)
{
	memset(unitVars, int(0), UNIT_VAR_COUNT);
}


CUnitScript::~CUnitScript()
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		// anim listeners are not owned by the anim in general, so don't delete them here
		delete *i;
	}

	// Remove us from possible animation ticking
	if (!anims.empty()) {
		GUnitScriptEngine.RemoveInstance(this);
	}
}


/******************************************************************************/


/**
 * @brief Unblocks all threads waiting on an animation
 * @param anim AnimInfo the corresponding animation
 */
void CUnitScript::UnblockAll(struct AnimInfo * anim)
{
	std::list<IAnimListener *>::iterator li;

	for (li = anim->listeners.begin(); li != anim->listeners.end(); ++li) {
		(*li)->AnimFinished(anim->type, anim->piece, anim->axis);
	}
}


/**
 * @brief Updates move animations
 * @param cur float value to update
 * @param dest float final value
 * @param speed float max increment per tick
 * @return returns true if destination was reached, false otherwise
 */
bool CUnitScript::MoveToward(float &cur, float dest, float speed)
{
	const float delta = dest - cur;

	if (streflop::fabsf(delta) <= speed) {
		cur = dest;
		return true;
	}

	if (delta>0.0f) {
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
bool CUnitScript::TurnToward(float &cur, float dest, float speed)
{
	float delta = dest - cur;

	// clamp: -pi .. 0 .. +pi (remainder(x,TWOPI) would do the same but is slower due to streflop)
	if (delta>PI) {
		delta -= TWOPI;
	} else if (delta<=-PI) {
		delta += TWOPI;
	}

	if (streflop::fabsf(delta) <= speed) {
		cur = dest;
		return true;
	}

	if (delta>0.0f) {
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
bool CUnitScript::DoSpin(float &cur, float dest, float &speed, float accel, int divisor)
{
	const float delta = dest - speed;

	// Check if we are not at the final speed and
	// make sure we dont go past desired speed
	if (streflop::fabsf(delta) <= accel) {
		speed = dest;
		if (speed == 0.0f)
			return true;
	}
	else {
		if (delta > 0.0f) {
			//TA obviously defines accelerations in speed/frame (at 30 fps)
			speed += accel * (30.0f / divisor);
		} else {
			speed -= accel * (30.0f / divisor);
		}
	}

	cur += (speed / divisor);
	ClampRad(&cur);

	return false;
}


/**
 * @brief Called by the engine when we are registered as animating. If we return -1 it means that
 *        there is no longer anything animating
 * @param deltaTime int delta time to update
 * @return 0 if there are still animations going, -1 else
 */
int CUnitScript::Tick(int deltaTime)
{
	std::vector<struct AnimInfo *> remove;

	for (std::list<struct AnimInfo *>::iterator it = anims.begin(); it != anims.end(); ) {
		struct AnimInfo *ai = *it;

		bool done = false;

		switch (ai->type) {
			case AMove:
				done = MoveToward(pieces[ai->piece]->pos[ai->axis], ai->dest, ai->speed / (1000 / deltaTime));
				break;
			case ATurn:
				done = TurnToward(pieces[ai->piece]->rot[ai->axis], ai->dest, ai->speed / (1000 / deltaTime));
				break;
			case ASpin:
				done = DoSpin(pieces[ai->piece]->rot[ai->axis], ai->dest, ai->speed, ai->accel, 1000 / deltaTime);
				break;
		}

		// Queue for removal (UnblockAll may add new anims)
		if (done) {
			remove.push_back(ai);
			it = anims.erase(it);
		}
		else {
			++it;
		}
	}

	//Tell listeners to unblock?
	for (std::vector<struct AnimInfo *>::iterator it = remove.begin(); it != remove.end(); ++it) {
		UnblockAll(*it);
		delete *it;
	}

	if (anims.empty())
		return -1;
	else
		return 0;
}


//Optimize this?
//Returns anims list
struct CUnitScript::AnimInfo *CUnitScript::FindAnim(AnimType type, int piece, int axis)
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis))
			return *i;
	}
	return NULL;
}


//Optimize this?
// Returns true if an animation was found and deleted
void CUnitScript::RemoveAnim(AnimType type, int piece, int axis)
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		struct AnimInfo *ai = *i;

		if ((ai->type == type) && (ai->piece == piece) && (ai->axis == axis)) {
			anims.erase(i);

			// We need to unblock threads waiting on this animation, otherwise they will be lost in the void
			UnblockAll(ai);

			delete ai;

			// If this was the last animation, remove from currently animating list
			if (anims.empty()) {
				GUnitScriptEngine.RemoveInstance(this);
			}
			return;
		}
	}
}


//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CUnitScript::AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel, bool interpolated)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	float destf;
	if (type == AMove) {
		destf = pieces[piece]->original->offset[axis] + dest;
	} else {
		destf = dest;
		if (type == ATurn) {
			ClampRad(&destf);
		}
	}

	struct AnimInfo *ai;

	//Turns override spins.. Not sure about the other way around? If so the system should probably be redesigned
	//to only have two types of anims.. turns and moves, with spin as a bool
	//todo: optimize, atm RemoveAnim and FindAnim search twice through all anims
	if (type == ATurn)
		RemoveAnim(ASpin, piece, axis);
	if (type == ASpin)
		RemoveAnim(ATurn, piece, axis);

	ai = FindAnim(type, piece, axis);
	if (!ai) {
		// If we were not animating before, inform the engine of this so it can schedule us
		if (anims.empty()) {
			GUnitScriptEngine.AddInstance(this);
		}

		ai = new struct AnimInfo;
		ai->type = type;
		ai->piece = piece;
		ai->axis = axis;
		anims.push_back(ai);
	}

	ai->dest  = destf;
	ai->speed = speed;
	ai->accel = accel;
	ai->interpolated = interpolated;
}


void CUnitScript::Spin(int piece, int axis, float speed, float accel)
{
	struct AnimInfo *ai;
	ai = FindAnim(ASpin, piece, axis);

	//If we are already spinning, we may have to decelerate to the new speed
	if (ai) {
		ai->dest = speed;
		if (accel > 0) {
			ai->accel = accel;
		}
		else {
			//Go there instantly. Or have a defaul accel?
			ai->speed = speed;
			ai->accel = 0;
		}
	}
	else {
		//No accel means we start at desired speed instantly
		if (accel <= 0)
			AddAnim(ASpin, piece, axis, speed, speed, 0);
		else
			AddAnim(ASpin, piece, axis, 0, speed, accel);
	}
}


void CUnitScript::StopSpin(int piece, int axis, float decel)
{
	if (decel <= 0) {
		RemoveAnim(ASpin, piece, axis);
	}
	else {
		struct AnimInfo *ai;
		ai = FindAnim(ASpin, piece, axis);
		if (!ai)
			return;

		ai->dest = 0;
		ai->accel = decel;
	}
}


void CUnitScript::Turn(int piece, int axis, float speed, float destination, bool interpolated)
{
	AddAnim(ATurn, piece, axis, speed, destination, 0, interpolated);
}


void CUnitScript::Move(int piece, int axis, float speed, float destination, bool interpolated)
{
	AddAnim(AMove, piece, axis, speed, destination, 0, interpolated);
}


void CUnitScript::MoveNow(int piece, int axis, float destination)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	p->pos[axis] = pieces[piece]->original->offset[axis] + destination;
}


void CUnitScript::TurnNow(int piece, int axis, float destination)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	p->rot[axis] = destination;
}


void CUnitScript::SetVisibility(int piece, bool visible)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	if (p->visible != visible) {
		p->visible = visible;
	}
}


void CUnitScript::EmitSfx(int sfxType, int piece)
{
#ifndef _CONSOLE
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber for emit-sfx");
		return;
	}

	if (ph->particleSaturation > 1.0f && sfxType < SFX_CEG) {
		// skip adding (unsynced!) particles when we have too many
		return;
	}

	// Make sure wakes are only emitted on water
	if ((sfxType >= SFX_WAKE) && (sfxType <= SFX_REVERSE_WAKE_2)) {
		if (ground->GetApproximateHeight(unit->pos.x, unit->pos.z) > 0.0f) {
			return;
		}
	}

	float3 relPos = ZeroVector;
	float3 relDir = UpVector;

	if (!GetEmitDirPos(piece, relPos, relDir)) {
		ShowScriptError("emit-sfx: GetEmitDirPos failed");
		return;
	}

	relDir.SafeNormalize();

	const float3 pos =
		unit->pos +
		unit->frontdir * relPos.z +
		unit->updir    * relPos.y +
		unit->rightdir * relPos.x;
	const float3 dir =
		unit->frontdir * relDir.z +
		unit->updir    * relDir.y +
		unit->rightdir * relDir.x;

	float alpha = 0.3f + gu->usRandFloat() * 0.2f;
	float alphaFalloff = 0.004f;
	float fadeupTime = 4;

	// Hovers need special care
	if (unit->unitDef->canhover) {
		fadeupTime = 8.0f;
		alpha = 0.15f + gu->usRandFloat() * 0.2f;
		alphaFalloff = 0.008f;
	}

	switch (sfxType) {
		case SFX_REVERSE_WAKE:
		case SFX_REVERSE_WAKE_2: {  //reverse wake
			new CWakeProjectile(
				pos + gu->usRandVector() * 2.0f,
				dir * 0.4f,
				6.0f + gu->usRandFloat() * 4.0f,
				0.15f + gu->usRandFloat() * 0.3f,
				unit,
				alpha, alphaFalloff, fadeupTime
			);
			break;
		}

		case SFX_WAKE_2:  //wake 2, in TA it lives longer..
		case SFX_WAKE: {  //regular ship wake
			new CWakeProjectile(
				pos + gu->usRandVector() * 2.0f,
				dir * 0.4f,
				6.0f + gu->usRandFloat() * 4.0f,
				0.15f + gu->usRandFloat() * 0.3f,
				unit,
				alpha, alphaFalloff, fadeupTime
			);
			break;
		}

		case SFX_BUBBLE: {  //submarine bubble. does not provide direction through piece vertices..
			float3 pspeed = gu->usRandVector() * 0.1f;
				pspeed.y += 0.2f;

			new CBubbleProjectile(
				pos + gu->usRandVector() * 2.0f,
				pspeed,
				40.0f + gu->usRandFloat() * 30.0f,
				1.0f + gu->usRandFloat() * 2.0f,
				0.01f,
				unit,
				0.3f + gu->usRandFloat() * 0.3f
			);
		} break;

		case SFX_WHITE_SMOKE:  //damaged unit smoke
			new CSmokeProjectile(pos, gu->usRandVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, unit, 0.5f);
			break;
		case SFX_BLACK_SMOKE:  //damaged unit smoke
			new CSmokeProjectile(pos, gu->usRandVector() * 0.5f + UpVector * 1.1f, 60, 4, 0.5f, unit, 0.6f);
			break;
		case SFX_VTOL: {
			const float3 speed =
				unit->speed    * 0.7f +
				unit->frontdir * 0.5f *       relDir.z  +
				unit->updir    * 0.5f * -fabs(relDir.y) +
				unit->rightdir * 0.5f *       relDir.x;

			CHeatCloudProjectile* hc = new CHeatCloudProjectile(
				pos,
				speed,
				10 + gu->usRandFloat() * 5,
				3 + gu->usRandFloat() * 2,
				unit
			);
			hc->size = 3;
			break;
		}
		default: {
			if (sfxType & SFX_CEG) {
				// emit defined explosiongenerator
				const unsigned index = sfxType - SFX_CEG;
				if (index >= unit->unitDef->sfxExplGens.size() || unit->unitDef->sfxExplGens[index] == NULL) {
					ShowScriptError("Invalid explosion generator index for emit-sfx");
					break;
				}

				IExplosionGenerator* explGen = unit->unitDef->sfxExplGens[index];
				explGen->Explosion(0, pos, unit->cegDamage, 1, unit, 0, 0, dir);
			}
			else if (sfxType & SFX_FIRE_WEAPON) {
				// make a weapon fire from the piece
				const unsigned index = sfxType - SFX_FIRE_WEAPON;
				if (index >= unit->weapons.size() || unit->weapons[index] == NULL) {
					ShowScriptError("Invalid weapon index for emit-sfx");
					break;
				}

				CWeapon* weapon = unit->weapons[index];

				const float3 targetPos = weapon->targetPos;
				const float3 weaponMuzzlePos = weapon->weaponMuzzlePos;

				// don't override the weapon's target position
				// if it was not set internally (so that force-
				// fire keeps working as expected)
				if (!weapon->haveUserTarget) {
					weapon->targetPos = pos + dir;
				}

				weapon->weaponMuzzlePos = pos;
				weapon->Fire();
				weapon->weaponMuzzlePos = weaponMuzzlePos;

				weapon->targetPos = targetPos;
			}
			else if (sfxType & SFX_DETONATE_WEAPON) {
				const unsigned index = sfxType - SFX_DETONATE_WEAPON;
				if (index >= unit->weapons.size() || unit->weapons[index] == NULL) {
					ShowScriptError("Invalid weapon index for emit-sfx");
					break;
				}

				// detonate weapon from piece
				const WeaponDef* weaponDef = unit->weapons[index]->weaponDef;

				if (weaponDef->soundhit.getID(0) > 0) {
					Channels::Battle.PlaySample(weaponDef->soundhit.getID(0), unit, weaponDef->soundhit.getVolume(0));
				}

				helper->Explosion(
					pos,
					weaponDef->damages,
					weaponDef->areaOfEffect,
					weaponDef->edgeEffectiveness,
					weaponDef->explosionSpeed,
					unit,
					true,
					1.0f,
					weaponDef->noSelfDamage,
					weaponDef->impactOnly,
					weaponDef->explosionGenerator,
					NULL,
					ZeroVector,
					weaponDef->id
				);
			}
		} break;
	}


#endif
}


void CUnitScript::AttachUnit(int piece, int u)
{
	// -1 is valid, indicates that the unit should be hidden
	if ((piece >= 0) && (!PieceExists(piece))) {
		ShowScriptError("Invalid piecenumber for attach");
		return;
	}

#ifndef _CONSOLE
	CTransportUnit* tu = dynamic_cast<CTransportUnit*>(unit);

	if (tu && uh->units[u]) {
		tu->AttachUnit(uh->units[u], piece);
	}
#endif
}


void CUnitScript::DropUnit(int u)
{
#ifndef _CONSOLE
	CTransportUnit* tu = dynamic_cast<CTransportUnit*>(unit);

	if (tu && uh->units[u]) {
		tu->DetachUnit(uh->units[u]);
	}
#endif
}


//Returns true if there was an animation to listen to
bool CUnitScript::AddAnimListener(AnimType type, int piece, int axis, IAnimListener *listener)
{
	struct AnimInfo *ai;
	ai = FindAnim(type, piece, axis);
	if (ai) {
		ai->listeners.push_back(listener);
		return true;
	}
	else
		return false;
}


//Flags as defined by the cob standard
void CUnitScript::Explode(int piece, int flags)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber for explode");
		return;
	}

#ifndef _CONSOLE
	const float3 pos = GetPiecePos(piece) + unit->pos;

#ifdef TRACE_SYNC
	tracefile << "Cob explosion: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << piece << " " << flags << "\n";
#endif

	if (!(flags & PF_NoHeatCloud)) {
		// Do an explosion at the location first
		new CHeatCloudProjectile(pos, float3(0, 0, 0), 30, 30, NULL);
	}

	// If this is true, no stuff should fly off
	if (flags & PF_NONE) return;

	// This means that we are going to do a full fledged piece explosion!
	float3 baseSpeed = unit->speed + unit->residualImpulse * 0.5f;
	float sql = baseSpeed.SqLength();

	if (sql > 9) {
		const float l  = sqrt(sql);
		const float l2 = 3 + sqrt(l - 3);
		baseSpeed *= (l2 / l);
	}
	float3 speed((0.5f-gs->randFloat()) * 6.0f, 1.2f + gs->randFloat() * 5.0f, (0.5f - gs->randFloat()) * 6.0f);
	if (unit->pos.y - ground->GetApproximateHeight(unit->pos.x, unit->pos.z) > 15) {
		speed.y = (0.5f - gs->randFloat()) * 6.0f;
	}
	speed += baseSpeed;
	if (speed.SqLength() > 12*12) {
		speed = speed.Normalize() * 12;
	}

	/* TODO Push this back. Don't forget to pass the team (color).  */

	if (flags & PF_Shatter) {
		Shatter(piece, pos, speed);
	}
	else {
		LocalModelPiece* pieceData = pieces[piece];

		if (pieceData->original != NULL) {
			int newflags = PF_Fall; // if they don't fall they could live forever
			if (flags & PF_Explode) { newflags |= PF_Explode; }
			//if (flags & PF_Fall) { newflags |=  PF_Fall; }
			if ((flags & PF_Smoke) && ph->particleSaturation < 1) { newflags |= PF_Smoke; }
			if ((flags & PF_Fire) && ph->particleSaturation < 0.95f) { newflags |= PF_Fire; }
			if (flags & PF_NoCEGTrail) { newflags |= PF_NoCEGTrail; }

			//logOutput.Print("Exploding %s as %d", script.pieceNames[piece].c_str(), dl);
			new CPieceProjectile(pos, speed, pieceData, newflags,unit,0.5f);
		}
	}
#endif
}


void CUnitScript::Shatter(int piece, const float3& pos, const float3& speed)
{
	const LocalModelPiece* lmp = pieces[piece];
	const S3DModelPiece* omp = lmp->original;
	const float pieceChance = 1.0f - (ph->currentParticles - (ph->maxParticles - 2000)) / 2000.0f;

	omp->Shatter(pieceChance, unit->model->textureType, unit->team, pos, speed);
}


void CUnitScript::ShowFlare(int piece)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber for show(flare)");
		return;
	}
#ifndef _CONSOLE
	const float3 relpos = GetPiecePos(piece);
	const float3 pos =
		unit->pos +
		unit->frontdir * relpos.z +
		unit->updir    * relpos.y +
		unit->rightdir * relpos.x;
	const float3 dir = unit->lastMuzzleFlameDir;
	const float size = unit->lastMuzzleFlameSize;

	new CMuzzleFlame(pos, unit->speed, dir, size);
#endif
}


void CUnitScript::MoveSmooth(int piece, int axis, float destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	//Make sure we do not overwrite animations of non-interpolated origin
	AnimInfo *ai = FindAnim(AMove, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			MoveNow(piece, axis, destination);
			return;
		}
	}

	float cur = pieces[piece]->pos[axis] - pieces[piece]->original->offset[axis];
	float dist = streflop::fabsf(destination - cur);
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	float speed = (dist * timeFactor) / delta;

	Move(piece, axis, speed, destination, true);
}


void CUnitScript::TurnSmooth(int piece, int axis, float destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber");
		return;
	}

	AnimInfo *ai = FindAnim(ATurn, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			TurnNow(piece, axis, destination);
			return;
		}
	}

	// not sure the ClampRad() call is necessary here
	float cur = ClampRad(pieces[piece]->rot[axis]);
	float dist = streflop::fabsf(destination - cur);
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	float speed = (dist * timeFactor) / delta;

	Turn(piece, axis, speed, destination, true);
}


/******************************************************************************/


int CUnitScript::GetUnitVal(int val, int p1, int p2, int p3, int p4)
{
	// may happen in case one uses Spring.GetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (!unit) {
		ShowScriptError("Error: no unit (in GetUnitVal)");
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

		const CUnit* u = uh->GetUnit(p1);

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
			ShowScriptError("Invalid piecenumber for get piece_xz");
			break;
		}
		float3 relPos = GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return PACKXZ(pos.x, pos.z);
	}
	case PIECE_Y: {
		if (!PieceExists(p1)) {
			ShowScriptError("Invalid piecenumber for get piece_y");
			break;
		}
		float3 relPos = GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return int(pos.y * COBSCALE);
	}
	case UNIT_XZ: {
		if (p1 <= 0)
			return PACKXZ(unit->pos.x, unit->pos.z);

		const CUnit* u = uh->GetUnit(p1);

		if (u == NULL)
			return PACKXZ(0, 0);
		else
			return PACKXZ(u->pos.x, u->pos.z);
	}
	case UNIT_Y: {
		if (p1 <= 0)
			return int(unit->pos.y * COBSCALE);

		const CUnit* u = uh->GetUnit(p1);

		if (u == NULL)
			return 0;
		else
			return int(u->pos.y * COBSCALE);
	}
	case UNIT_HEIGHT: {
		if (p1 <= 0)
			return int(unit->radius * COBSCALE);

		const CUnit* u = uh->GetUnit(p1);

		if (u == NULL)
			return 0;
		else
			return int(u->radius * COBSCALE);
	}
	case XZ_ATAN:
		return int(RAD2TAANG*atan2((float)UNPACKX(p1), (float)UNPACKZ(p1)) + 32768 - unit->heading);
	case XZ_HYPOT:
		return int(hypot((float)UNPACKX(p1), (float)UNPACKZ(p1)) * COBSCALE);
	case ATAN:
		return int(RAD2TAANG*atan2((float)p1, (float)p2));
	case HYPOT:
		return int(hypot((float)p1, (float)p2));
	case GROUND_HEIGHT:
		return int(ground->GetHeightAboveWater(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case GROUND_WATER_HEIGHT:
		return int(ground->GetHeightReal(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case BUILD_PERCENT_LEFT:
		return int((1.0f - unit->buildProgress) * 100);

	case YARD_OPEN:
		if (yardOpen)
			return 1;
		else
			return 0;
	case BUGGER_OFF:
		break;
	case ARMORED:
		if (unit->armoredState)
			return 1;
		else
			return 0;
	case VETERAN_LEVEL:
		return int(100 * unit->experience);
	case CURRENT_SPEED:
		if (unit->moveType)
			return int(unit->speed.Length() * COBSCALE);
		return 0;
	case ON_ROAD:
		return 0;
	case IN_WATER:
		return (unit->pos.y < 0.0f) ? 1 : 0;
	case MAX_ID:
		return uh->MaxUnits()-1;
	case MY_ID:
		return unit->id;

	case UNIT_TEAM: {
		const CUnit* u = uh->GetUnit(p1);
		return (u != NULL)? unit->team : 0;
	}
	case UNIT_ALLIED: {
		const CUnit* u = uh->GetUnit(p1);

		if (u != NULL) {
			return teamHandler->Ally(unit->allyteam, u->allyteam) ? 1 : 0;
		}

		return 0;
	}
	case UNIT_BUILD_PERCENT_LEFT: {
		const CUnit* u = uh->GetUnit(p1);

		if (u != NULL) {
			return int((1.0f - u->buildProgress) * 100);
		}

		return 0;
	}
	case MAX_SPEED:
		if (unit->moveType) {
			return int(unit->moveType->maxSpeed * COBSCALE);
		}
		break;
	case REVERSING:
		if (unit->moveType) {
			CGroundMoveType* gmt = dynamic_cast<CGroundMoveType*>(unit->moveType);
			return ((gmt != NULL)? int(gmt->IsReversing()): 0);
		}
		break;
	case CLOAKED:
		return !!unit->isCloaked;
	case WANT_CLOAK:
		return !!unit->wantCloak;
	case UPRIGHT:
		return !!unit->upright;
	case POW:
		return int(pow(((float)p1)/COBSCALE,((float)p2)/COBSCALE)*COBSCALE);
	case PRINT:
		logOutput.Print("Value 1: %d, 2: %d, 3: %d, 4: %d", p1, p2, p3, p4);
		break;
	case HEADING: {
		if (p1 <= 0) {
			return unit->heading;
		}

		const CUnit* u = uh->GetUnit(p1);

		if (u != NULL) {
			return u->heading;
		}

		return -1;
	}
	case TARGET_ID: {
		if (unit->weapons[p1 - 1]) {
			const CWeapon* weapon = unit->weapons[p1 - 1];
			const TargetType tType = weapon->targetType;

			if (tType == Target_Unit)
				return unit->weapons[p1 - 1]->targetUnit->id;
			else if (tType == Target_None)
				return -1;
			else if (tType == Target_Pos)
				return -2;
			else // Target_Intercept
				return -3;
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

	case CURRENT_FUEL:
		return int(unit->currentFuel * float(COBSCALE));
	case TRANSPORT_ID:
		return unit->transporter?unit->transporter->id:-1;

	case SHIELD_POWER: {
		if (unit->shieldWeapon == NULL) {
			return -1;
		}
		const CPlasmaRepulser* shield = (CPlasmaRepulser*) unit->shieldWeapon;
		return int(shield->curPower * float(COBSCALE));
	}

	case STEALTH: {
		return unit->stealth ? 1 : 0;
	}
	case SONAR_STEALTH: {
		return unit->sonarStealth ? 1 : 0;
	}
	case CRASHING:
		return !!unit->crashing;
	case ALPHA_THRESHOLD: {
		return int(unit->alphaThreshold * 255);
	}

	case COB_ID: {
		if (p1 <= 0) {
			return unit->unitDef->cobID;
		} else {
			const CUnit* u = uh->GetUnit(p1);
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
				if (!loshandler->InAirLos(unit->pos,gu->myAllyTeam)) { return 0; }
				break;
			case 1:		//LOS
				if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS)) { return 0; }
				break;
			case 2:		//ALOS or radar
				if (!(loshandler->InAirLos(unit->pos,gu->myAllyTeam) || unit->losStatus[gu->myAllyTeam] & (LOS_INRADAR))) { return 0; }
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
			Channels::UnitReply.PlaySample(script->sounds[p1], unit->pos, unit->speed, float(p2) / COBSCALE);
		} else {
			Channels::UnitReply.PlaySample(script->sounds[p1], float(p2) / COBSCALE);
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
		CUnit* target = (targetID > 0)? uh->GetUnit(targetID): NULL;
		return (weapon->AttackUnit(target, userTarget) ? 1 : 0);
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

		return weapon->AttackGround(pos, userTarget) ? 1 : 0;
	}
	case MIN:
		return std::min(p1, p2);
	case MAX:
		return std::max(p1, p2);
	case ABS:
		return abs(p1);
	case KSIN:
		return int(1024*streflop::sinf(TAANG2RAD*(float)p1));
	case KCOS:
		return int(1024*streflop::cosf(TAANG2RAD*(float)p1));
	case KTAN:
		return int(1024*streflop::tanf(TAANG2RAD*(float)p1));
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
		CUnit* u = (p1 > 0)? uh->GetUnit(p1): this->unit;

		if (u == NULL) {
			return 0;
		}

		if (u->beingBuilt) {
			// no explosions and no corpse for units under construction
			u->KillUnit(false, true, NULL);
		} else {
			u->KillUnit(p2 != 0, p3 != 0, NULL);
		}

		return 1;
	}
	case WEAPON_RELOADSTATE: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return unit->weapons[p1-1]->reloadStatus;
		}
		else if (p1 < 0 && p1 >= (0 - unit->weapons.size())) {
			int old = unit->weapons[-p1-1]->reloadStatus;
			unit->weapons[-p1-1]->reloadStatus = p2;
			return old;
		}
		else {
			return -1;
		}
	}
	case WEAPON_RELOADTIME: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return unit->weapons[p1-1]->reloadTime;
		}
		else if (p1 < 0 && p1 >= 0 - unit->weapons.size()) {
			int old = unit->weapons[-p1-1]->reloadTime;
			unit->weapons[-p1-1]->reloadTime = p2;
			return old;
		}
		else {
			return -1;
		}
	}
	case WEAPON_ACCURACY: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return int(unit->weapons[p1-1]->accuracy * COBSCALE);
		}
		else if (p1 < 0 && p1 >= (0 - unit->weapons.size())) {
			int old = int(unit->weapons[-p1-1]->accuracy * COBSCALE);
			unit->weapons[-p1-1]->accuracy = float(p2) / COBSCALE;
			return old;
		}
		else {
			return -1;
		}
	}
	case WEAPON_SPRAY: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return int(unit->weapons[p1-1]->sprayAngle * COBSCALE);
		}
		else if (p1 < 0 && p1 >= (0 - unit->weapons.size())) {
			int old = int(unit->weapons[-p1-1]->sprayAngle * COBSCALE);
			unit->weapons[-p1-1]->sprayAngle = float(p2) / COBSCALE;
			return old;
		}
		else {
			return -1;
		}
	}
	case WEAPON_RANGE: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return int(unit->weapons[p1-1]->range * COBSCALE);
		}
		else if (p1 < 0 && p1 >= (0 - unit->weapons.size())) {
			int old = int(unit->weapons[-p1-1]->range * COBSCALE);
			unit->weapons[-p1-1]->range = float(p2) / COBSCALE;
			return old;
		}
		else {
			return -1;
		}
	}
	case WEAPON_PROJECTILE_SPEED: {
		if (p1 > 0 && static_cast<size_t>(p1) <= unit->weapons.size()) {
			return int(unit->weapons[p1-1]->projectileSpeed * COBSCALE);
		}
		else if (p1 < 0 && p1 >= (0 - unit->weapons.size())) {
			int old = int(unit->weapons[-p1-1]->projectileSpeed * COBSCALE);
			unit->weapons[-p1-1]->projectileSpeed = float(p2) / COBSCALE;
			return old;
		}
		else {
			return -1;
		}
	}
	case GAME_FRAME: {
		return gs->frameNum;
	}
	default:
		if ((val >= GLOBAL_VAR_START) && (val <= GLOBAL_VAR_END)) {
			return globalVars[val - GLOBAL_VAR_START];
		}
		else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
			return teamVars[unit->team][val - TEAM_VAR_START];
		}
		else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
			return allyVars[unit->allyteam][val - ALLY_VAR_START];
		}
		else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
			const int varID = val - UNIT_VAR_START;

			if (p1 == 0) {
				return unitVars[varID];
			}
			else if (p1 > 0) {
				// get the unit var for another unit
				const CUnit* u = uh->GetUnit(p1);

				if (u != NULL && u->script != NULL) {
					return u->script->unitVars[varID];
				}
			}
			else {
				// set the unit var for another unit
				p1 = -p1;

				CUnit* u = uh->GetUnit(p1);

				if (u != NULL && u->script != NULL) {
					u->script->unitVars[varID] = p2;
					return 1;
				}
			}
			return 0;
		}
		else {
			logOutput.Print("CobError: Unknown get constant %d  (params = %d %d %d %d)",
			                val, p1, p2, p3, p4);
		}
	}
#endif

	return 0;
}


void CUnitScript::SetUnitVal(int val, int param)
{
	// may happen in case one uses Spring.SetUnitCOBValue (Lua) on a unit with CNullUnitScript
	if (!unit) {
		ShowScriptError("Error: no unit (in SetUnitVal)");
		return;
	}

#ifndef _CONSOLE
	switch(val) {
		case ACTIVATION: {
			if(unit->unitDef->onoffable) {
				Command c;
				c.id = CMD_ONOFF;
				c.params.push_back(param == 0 ? 0 : 1);
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
				Command c;
				c.id = CMD_MOVE_STATE;
				c.params.push_back(param);
				unit->commandAI->GiveCommand(c);
			}
			break;
		}
		case STANDINGFIREORDERS: {
			if (param >= 0 && param <= 2) {
				Command c;
				c.id = CMD_FIRE_STATE;
				c.params.push_back(param);
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
			if (unit->curYardMap != 0) {
				// note: if this unit is a factory, engine-controlled
				// OpenYard() and CloseYard() calls can interfere with
				// the yardOpen state (they probably should be removed
				// at some point)
				if (param == 0) {
					if (groundBlockingObjectMap->CanCloseYard(unit)) {
						groundBlockingObjectMap->CloseBlockingYard(unit, unit->curYardMap);
						yardOpen = false;
					}
				} else {
					groundBlockingObjectMap->OpenBlockingYard(unit, unit->curYardMap);
					yardOpen = true;
				}
			}
			break;
		}
		case BUGGER_OFF: {
			if (param != 0) {
				helper->BuggerOff(unit->pos + unit->frontdir * unit->radius, unit->radius * 1.5f, true, false, unit->team, NULL);
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
			if (unit->moveType && param > 0) {
				// find the first CMD_SET_WANTED_MAX_SPEED and modify it if need be
				for (CCommandQueue::iterator it = unit->commandAI->commandQue.begin();
						it != unit->commandAI->commandQue.end(); ++it) {
					Command &c = *it;
					if (c.id == CMD_SET_WANTED_MAX_SPEED && c.params[0] == unit->maxSpeed) {
						c.params[0] = param/(float)COBSCALE;
						break;
					}
				}
				unit->moveType->SetMaxSpeed(param/(float)COBSCALE);
				unit->maxSpeed = param/(float)COBSCALE;
			}
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
			unit->SetDirectionFromHeading();
			break;
		}
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
			unit->ChangeSensorRadius(&unit->radarRadius, param);
			break;
		}
		case JAMMER_RADIUS: {
			unit->ChangeSensorRadius(&unit->jammerRadius, param);
			break;
		}
		case SONAR_RADIUS: {
			unit->ChangeSensorRadius(&unit->sonarRadius, param);
			break;
		}
		case SONAR_JAM_RADIUS: {
			unit->ChangeSensorRadius(&unit->sonarJamRadius, param);
			break;
		}
		case SEISMIC_RADIUS: {
			unit->ChangeSensorRadius(&unit->seismicRadius, param);
			break;
		}
		case CURRENT_FUEL: {
			unit->currentFuel = param / (float) COBSCALE;
			break;
		}
		case SHIELD_POWER: {
			if (unit->shieldWeapon != NULL) {
				CPlasmaRepulser* shield = (CPlasmaRepulser*)unit->shieldWeapon;
				shield->curPower = std::max(0.0f, float(param) / float(COBSCALE));
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
			if(dynamic_cast<CAirMoveType*>(unit->moveType)){
				if(!!param){
					((CAirMoveType*)unit->moveType)->SetState(AAirMoveType::AIRCRAFT_CRASHING);
				} else {
					unit->crashing=false;
					((CAirMoveType*)unit->moveType)->aircraftState=AAirMoveType::AIRCRAFT_TAKEOFF;
					((CAirMoveType*)unit->moveType)->SetState(AAirMoveType::AIRCRAFT_FLYING);
				}
			}
			break;
		}
		case CHANGE_TARGET: {
			unit->weapons[param - 1]->avoidTarget = true;
			break;
		}
		case ALPHA_THRESHOLD: {
			unit->alphaThreshold = float(param) / 255.0f;
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
				globalVars[val - GLOBAL_VAR_START] = param;
			}
			else if ((val >= TEAM_VAR_START) && (val <= TEAM_VAR_END)) {
				teamVars[unit->team][val - TEAM_VAR_START] = param;
			}
			else if ((val >= ALLY_VAR_START) && (val <= ALLY_VAR_END)) {
				allyVars[unit->allyteam][val - ALLY_VAR_START] = param;
			}
			else if ((val >= UNIT_VAR_START) && (val <= UNIT_VAR_END)) {
				unitVars[val - UNIT_VAR_START] = param;
			}
			else {
				logOutput.Print("CobError: Unknown set constant %d", val);
			}
		}
	}
#endif
}


/******************************************************************************/
/******************************************************************************/

#ifndef _CONSOLE

void CUnitScript::BenchmarkScript(CUnitScript* script)
{
	const int duration = 10000; // millisecs

	const unsigned start = SDL_GetTicks();
	unsigned end = start;
	int count = 0;

	while ((end - start) < duration) {
		for (int i = 0; i < 10000; ++i) {
			script->QueryWeapon(0);
		}
		++count;
		end = SDL_GetTicks();
	}

	logOutput.Print("%d0000 calls in %u ms -> %.0f calls/second",
	                count, end - start, float(count) * (10000 / (duration / 1000)));
}


void CUnitScript::BenchmarkScript(const string& unitname)
{
	std::list<CUnit*>::iterator ui = uh->activeUnits.begin();
	for (; ui != uh->activeUnits.end(); ++ui) {
		CUnit* unit = *ui;
		if (unit->unitDef->name == unitname) {
			BenchmarkScript(unit->script);
			return;
		}
	}
}

#endif

/******************************************************************************/
/******************************************************************************/

int CUnitScript::ScriptToModel(int scriptnum) const {
	const LocalModelPiece* p = GetLocalModelPiece(scriptnum);

	if (p == NULL) return -1;

	int i = 0;
	const std::vector<LocalModelPiece*>& modelpieces = unit->localmodel->pieces;
	for (std::vector<LocalModelPiece*>::const_iterator pm = modelpieces.begin(); pm != modelpieces.end(); pm++, i++) {
		if (p == *pm) return i;
	}
	return -1;
};
