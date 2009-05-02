/* Author: Tobi Vollebregt */
/* heavily based on CobInstance.cpp */

#include "UnitScript.h"
#include "UnitScriptEngine.h"

#ifndef _CONSOLE

#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AirMoveType.h"
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
#include "Sound/Sound.h"
#include "myMath.h"
#include "Sync/SyncTracer.h"

#endif


using std::string;


// GCC needs even pure destructors declared
CUnitScript::IAnimListener::~IAnimListener()
{
}


CUnitScript::CUnitScript(CUnit* unit, const std::vector<int>& scriptIndex)
	: unit(unit)
	, scriptIndex(scriptIndex)
{
	MapScriptToModelPieces(unit->localmodel);
}


CUnitScript::~CUnitScript()
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		//All threads blocking on animations can be killed safely from here since the scheduler does not
		//know about them
		for (std::list<IAnimListener *>::iterator j = (*i)->listeners.begin(); j != (*i)->listeners.end(); ++j) {
			delete *j;
		}
		delete *i;
	}

	// Remove us from possible animation ticking (should only be needed when anims.size() > 0
	GUnitScriptEngine.RemoveInstance(this);
}


void CUnitScript::MapScriptToModelPieces(LocalModel* lmodel)
{
	//TODO: how to get at pieceNames used by script in a generic way?
	/*
	pieces.clear();
	pieces.reserve(script.pieceNames.size());

	std::vector<LocalModelPiece*>& lp = lmodel->pieces;

	for (size_t piecenum=0; piecenum<script.pieceNames.size(); piecenum++) {
		std::string& scriptname = script.pieceNames[piecenum]; // is already in lowercase!

		unsigned int cur;

		//Map this piecename to an index in the script's pieceinfo
		for (cur=0; cur<lp.size(); cur++) {
			if (lp[cur]->name.compare(scriptname) == 0) {
				break;
			}
		}

		//Not found? Try lowercase
		if (cur == lp.size()) {
			for (cur=0; cur<lp.size(); cur++) {
				if (StringToLower(lp[cur]->name).compare(scriptname) == 0) {
					break;
				}
			}
		}

		//Did we find it?
		if (cur < lp.size()) {
			pieces.push_back(lp[cur]);
		} else {
			pieces.push_back(NULL);
			logOutput.Print("CobWarning: Couldn't find a piece named \""+ scriptname +"\" in the model (in "+ script.name +")");
		}
	}
	*/
}


/******************************************************************************/


bool CUnitScript::HasScriptFunction(int id)
{
	return scriptIndex[id] >= 0;
}


bool CUnitScript::FunctionExist(int id)
{
	return scriptIndex[id] >= 0;
}


int CUnitScript::GetFunctionId(const string& funcName) const
{
	//TODO: return script.getFunctionId(funcName);
	return 0;
}


/******************************************************************************/


int CUnitScript::Call(const string &fname)
{
	std::vector<int> x;
	return Call(fname, x, NULL, NULL, NULL);
}


int CUnitScript::Call(const string &fname, vector<int> &args)
{
	return Call(fname, args, NULL, NULL, NULL);
}


int CUnitScript::Call(const string &fname, int p1)
{
	std::vector<int> x;
	x.push_back(p1);
	return Call(fname, x, NULL, NULL, NULL);
}


int CUnitScript::Call(const string &fname, CBCobThreadFinish cb, void *p1, void *p2)
{
	std::vector<int> x;
	return Call(fname, x, cb, p1, p2);
}


int CUnitScript::Call(const string &fname, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = GetFunctionId(fname);
	//TODO: Check that new behaviour of actually calling cb when the function is not defined is right?
	//      (the callback has always been called [when the function is not defined]
	//       in the id-based Call()s, but never in the string based calls.)
	return RealCall(fn, args, cb, p1, p2);
}


int CUnitScript::Call(int id)
{
	std::vector<int> x;
	return Call(id, x, NULL, NULL, NULL);
}


int CUnitScript::Call(int id, int p1)
{
	std::vector<int> x;
	x.push_back(p1);
	return Call(id, x, NULL, NULL, NULL);
}


int CUnitScript::Call(int id, vector<int> &args)
{
	return Call(id, args, NULL, NULL, NULL);
}


int CUnitScript::Call(int id, CBCobThreadFinish cb, void *p1, void *p2)
{
	std::vector<int> x;
	return Call(id, x, cb, p1, p2);
}


int CUnitScript::Call(int id, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	return RealCall(scriptIndex[id], args, cb, p1, p2);
}


int CUnitScript::RawCall(int fn, vector<int> &args)
{
	return RealCall(fn, args, NULL, NULL, NULL);
}


int CUnitScript::RawCall(int fn, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	return RealCall(fn, args, cb, p1, p2);
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
 * @return returns 1 if destination was reached, 0 otherwise
 */
int CUnitScript::MoveToward(float &cur, float dest, float speed)
{
	const float delta = dest - cur;

	if (streflop::fabsf(delta) <= speed) {
		cur = dest;
		return 1;
	}

	if (delta>0.0f) {
		cur += speed;
	} else {
		cur -= speed;
	}

	return 0;
}


/**
 * @brief Updates turn animations
 * @param cur float value to update
 * @param dest float final value
 * @param speed float max increment per tick
 * @return returns 1 if destination was reached, 0 otherwise
 */
int CUnitScript::TurnToward(float &cur, float dest, float speed)
{
	ClampRad(&cur);

	float delta = dest - cur;

	// clamp: -pi .. 0 .. +pi (remainder(x,TWOPI) would do the same but is slower due to streflop)
	if (delta>PI) {
		delta -= TWOPI;
	} else if (delta<=-PI) {
		delta += TWOPI;
	}

	if (streflop::fabsf(delta) <= speed) {
		cur = dest;
		return 1;
	}

	if (delta>0.0f) {
		cur += speed;
	} else {
		cur -= speed;
	}

	return 0;
}


/**
 * @brief Updates spin animations
 * @param cur float value to update
 * @param dest float the final desired speed (NOT the final angle!)
 * @param speed float is updated if it is not equal to dest
 * @param divisor int is the deltatime, it is not added before the call because speed may have to be updated
 * @return 1 if the desired speed is 0 and it is reached, otherwise 0
 */
int CUnitScript::DoSpin(float &cur, float dest, float &speed, float accel, int divisor)
{
	//Check if we are not at the final speed
	if (speed != dest) {
		speed += accel * (30.0f / divisor);   //TA obviously defines accelerations in speed/frame (at 30 fps)
		if (streflop::fabsf(speed) > dest)      // make sure we dont go past desired speed
			speed = dest;
		if ((accel < 0.0f) && (speed == 0.0f))
			return 1;
	}

	cur += (speed / divisor);
	ClampRad(&cur);

	return 0;
}


/**
 * @brief Called by the engine when we are registered as animating. If we return -1 it means that
 *        there is no longer anything animating
 * @param deltaTime int delta time to update
 * @return 0 if there are still animations going, -1 else
 */
int CUnitScript::Tick(int deltaTime)
{
	int done;
	std::list<struct AnimInfo *>::iterator it = anims.begin();
	std::list<struct AnimInfo *>::iterator cur;

	while (it != anims.end()) {
		//Advance it, so we can erase cur safely
		cur = it++;

		done = false;
		pieces[(*cur)->piece]->updated = true;

		switch ((*cur)->type) {
			case AMove:
				done = MoveToward(pieces[(*cur)->piece]->pos[(*cur)->axis], (*cur)->dest, (*cur)->speed / (1000 / deltaTime));
				break;
			case ATurn:
				done = TurnToward(pieces[(*cur)->piece]->rot[(*cur)->axis], (*cur)->dest, (*cur)->speed / (1000 / deltaTime));
				break;
			case ASpin:
				done = DoSpin(pieces[(*cur)->piece]->rot[(*cur)->axis], (*cur)->dest, (*cur)->speed, (*cur)->accel, 1000 / deltaTime);
				break;
		}

		//Tell listeners to unblock?
		if (done) {
			UnblockAll(*cur);
			delete *cur;
			anims.erase(cur);
		}

	}

	if (anims.size() == 0)
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
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis)) {

			// We need to unblock threads waiting on this animation, otherwise they will be lost in the void
			UnblockAll(*i);

			delete *i;
			anims.erase(i);

			// If this was the last animation, remove from currently animating list
			if (anims.size() == 0) {
				GUnitScriptEngine.RemoveInstance(this);
			}
			return;
		}
	}
}


//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CUnitScript::AddAnim(AnimType type, int piece, int axis, int speed, int dest, int accel, bool interpolated)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	// translate cob piece coords into worldcoordinates
	float destf;
	float speedf;
	float accelf;
	if (type == AMove) {
		destf  = pieces[piece]->original->offset[axis];
		if (axis==0) {
			destf -= dest * CORDDIV;
		} else {
			destf += dest * CORDDIV;
		}
		speedf = speed * CORDDIV;
		accelf = accel;
	} else {
		destf  = dest  * TAANG2RAD;
		speedf = speed * TAANG2RAD;
		accelf = accel * TAANG2RAD;
		ClampRad(&destf);
	}

	struct AnimInfo *ai;

	//Turns override spins.. Not sure about the other way around? If so the system should probably be redesigned
	//to only have two types of anims.. turns and moves, with spin as a bool
	if (type != AMove)
		RemoveAnim(type, piece, axis); //todo: optimize, atm RemoveAnim and FindAnim search twice through all anims

	ai = FindAnim(type, piece, axis);
	if (!ai) {
		ai = new struct AnimInfo;
		ai->type = type;
		ai->piece = piece;
		ai->axis = axis;
		anims.push_back(ai);

		// If we were not animating before, inform the engine of this so it can schedule us
		if (anims.size() == 1) {
			GUnitScriptEngine.AddInstance(this);
		}
	}

	ai->dest  = destf;
	ai->speed = speedf;
	ai->accel = accelf;
	ai->interpolated = interpolated;
}


void CUnitScript::Spin(int piece, int axis, int speed, int accel)
{
	struct AnimInfo *ai;
	ai = FindAnim(ASpin, piece, axis);

	//logOutput.Print("Spin called %d %d %d %d", piece, axis, speed, accel);

	//If we are already spinning, we may have to decelerate to the new speed
	if (ai) {
		ai->dest = speed * TAANG2RAD;
		if (accel > 0) {
			if (ai->speed > ai->dest)
				ai->accel = -accel * TAANG2RAD;
			else
				ai->accel = accel * TAANG2RAD;
		}
		else {
			//Go there instantly. Or have a defaul accel?
			ai->speed = speed * TAANG2RAD;
			ai->accel = 0;
		}
	}
	else {
		//No accel means we start at desired speed instantly
		if (accel == 0)
			AddAnim(ASpin, piece, axis, speed, speed, accel);
		else
			AddAnim(ASpin, piece, axis, 0, speed, accel);
	}
}


void CUnitScript::StopSpin(int piece, int axis, int decel)
{
	struct AnimInfo *ai;
	ai = FindAnim(ASpin, piece, axis);
	if (!ai)
		return;

	if (decel == 0) {
		RemoveAnim(ASpin, piece, axis);
	}
	else
		AddAnim(ASpin, piece, axis, ai->speed, 0, -decel);
}


void CUnitScript::Turn(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(ATurn, piece, axis, speed, destination, 0, interpolated);
}


void CUnitScript::Move(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(AMove, piece, axis, speed, destination, 0, interpolated);
}


void CUnitScript::MoveNow(int piece, int axis, int destination)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	p->pos[axis] = pieces[piece]->original->offset[axis];
	if (axis==0) {
		p->pos[axis] -= destination * CORDDIV;
	} else {
		p->pos[axis] += destination * CORDDIV;
	}
	p->updated = true;
}


void CUnitScript::TurnNow(int piece, int axis, int destination)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	p->rot[axis] = destination * TAANG2RAD;
	p->updated = true;
	//logOutput.Print("moving %s on axis %d to %d", script.pieceNames[piece].c_str(), axis, destination);
}


void CUnitScript::SetVisibility(int piece, bool visible)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	if (p->visible != visible) {
		p->visible = visible;
		p->updated = true;
	}
}


void CUnitScript::EmitSfx(int type, int piece)
{
#ifndef _CONSOLE
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber for emit-sfx");
		return;
	}

	if(ph->particleSaturation>1 && type<1024){		//skip adding particles when we have to many (make sure below can be unsynced)
		return;
	}

	float3 relPos(0,0,0);
	float3 relDir(0,1,0);
	if (!GetEmitDirPos(piece, relPos, relDir)) {
		ShowScriptError("emit-sfx: GetEmitDirPos failed\n");
	}

	float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;

	float alpha = 0.3f+gu->usRandFloat()*0.2f;
	float alphaFalloff = 0.004f;
	float fadeupTime=4;

	//Hovers need special care
	if (unit->unitDef->canhover) {
		fadeupTime=8;
		alpha = 0.15f+gu->usRandFloat()*0.2f;
		alphaFalloff = 0.008f;
	}

	//Make sure wakes are only emitted on water
	if ((type >= 2) && (type <= 5)) {
		if (ground->GetApproximateHeight(unit->pos.x, unit->pos.z) > 0){
			return;
		}
	}

	switch (type) {
		case 4:
		case 5:		{	//reverse wake
			//float3 relDir = -GetPieceDirection(piece) * 0.2f;
			relDir *= 0.2f;
			float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
			new CWakeProjectile(pos+gu->usRandVector()*2,dir*0.4f,6+gu->usRandFloat()*4,0.15f+gu->usRandFloat()*0.3f,unit, alpha, alphaFalloff,fadeupTime);
			break;}
		case 3:			//wake 2, in TA it lives longer..
		case 2:		{	//regular ship wake
			//float3 relDir = GetPieceDirection(piece) * 0.2f;
			relDir *= 0.2f;
			float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
			new CWakeProjectile(pos+gu->usRandVector()*2,dir*0.4f,6+gu->usRandFloat()*4,0.15f+gu->usRandFloat()*0.3f,unit, alpha, alphaFalloff,fadeupTime);
			break;}
		case 259:	{	//submarine bubble. does not provide direction through piece vertices..
			float3 pspeed=gu->usRandVector()*0.1f;
			pspeed.y+=0.2f;
			new CBubbleProjectile(pos+gu->usRandVector()*2,pspeed,40+gu->usRandFloat()*30,1+gu->usRandFloat()*2,0.01f,unit,0.3f+gu->usRandFloat()*0.3f);
			break;}
		case 257:	//damaged unit smoke
			new CSmokeProjectile(pos,gu->usRandVector()*0.5f+UpVector*1.1f,60,4,0.5f,unit,0.5f);
			// FIXME -- needs a 'break'?
		case 258:		//damaged unit smoke
			new CSmokeProjectile(pos,gu->usRandVector()*0.5f+UpVector*1.1f,60,4,0.5f,unit,0.6f);
			break;
		case 0:{		//vtol
			//relDir = GetPieceDirection(piece) * 0.2f;
			relDir *= 0.2f;
			float3 dir = unit->frontdir * relDir.z + unit->updir * -fabs(relDir.y) + unit->rightdir * relDir.x;
			CHeatCloudProjectile* hc=new CHeatCloudProjectile(pos, unit->speed*0.7f+dir * 0.5f, 10 + gu->usRandFloat() * 5, 3 + gu->usRandFloat() * 2, unit);
			hc->size=3;
			break;}
		default:
			//logOutput.Print("Unknown sfx: %d", type);
			if (type & 1024)	//emit defined explosiongenerator
			{
				unsigned index = type - 1024;
				if (index >= unit->unitDef->sfxExplGens.size() || unit->unitDef->sfxExplGens[index] == NULL) {
					ShowScriptError("Invalid explosion generator index for emit-sfx");
					break;
				}
				//float3 relDir = -GetPieceDirection(piece) * 0.2f;
				float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
				dir.Normalize();
				unit->unitDef->sfxExplGens[index]->Explosion(pos, unit->cegDamage, 1, unit, 0, 0, dir);
			}
			else if (type & 2048)  //make a weapon fire from the piece
			{
				unsigned index = type - 2048;
				if (index >= unit->weapons.size() || unit->weapons[index] == NULL) {
					ShowScriptError("Invalid weapon index for emit-sfx");
					break;
				}
				//this is very hackish and probably has a lot of side effects, but might be usefull for something
				//float3 relDir =-GetPieceDirection(piece);
				float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
				dir.Normalize();

				float3 targetPos = unit->weapons[index]->targetPos;
				float3 weaponMuzzlePos = unit->weapons[index]->weaponMuzzlePos;

				unit->weapons[index]->targetPos = pos+dir;
				unit->weapons[index]->weaponMuzzlePos = pos;

				unit->weapons[index]->Fire();

				unit->weapons[index]->targetPos = targetPos;
				unit->weapons[index]->weaponMuzzlePos = weaponMuzzlePos;
			}
			else if (type & 4096) {
				unsigned index = type - 4096;
				if (index >= unit->weapons.size() || unit->weapons[index] == NULL) {
					ShowScriptError("Invalid weapon index for emit-sfx");
					break;
				}
				// detonate weapon from piece
				const WeaponDef* weaponDef = unit->weapons[index]->weaponDef;
				if (weaponDef->soundhit.getID(0) > 0) {
					sound->PlaySample(weaponDef->soundhit.getID(0), unit, weaponDef->soundhit.getVolume(0));
				}

				helper->Explosion(
					pos, weaponDef->damages, weaponDef->areaOfEffect, weaponDef->edgeEffectiveness,
					weaponDef->explosionSpeed, unit, true, 1.0f, weaponDef->noSelfDamage, weaponDef->impactOnly, weaponDef->explosionGenerator,
					NULL, float3(0, 0, 0), weaponDef->id
				);
			}
			break;
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
	CTransportUnit* tu=dynamic_cast<CTransportUnit*>(unit);

	if(tu && uh->units[u]){
		//logOutput.Print("attach");
		tu->AttachUnit(uh->units[u],piece);
	}
#endif
}


void CUnitScript::DropUnit(int u)
{
#ifndef _CONSOLE
	CTransportUnit* tu=dynamic_cast<CTransportUnit*>(unit);

	if(tu && uh->units[u]){
		tu->DetachUnit(uh->units[u]);
	}
#endif
}


//Returns 1 if there was a turn to listen to
int CUnitScript::AddTurnListener(int piece, int axis, IAnimListener *listener)
{
	struct AnimInfo *ai;
	ai = FindAnim(ATurn, piece, axis);
	if (ai) {
		ai->listeners.push_back(listener);
		return 1;
	}
	else
		return 0;
}


int CUnitScript::AddMoveListener(int piece, int axis, IAnimListener *listener)
{
	struct AnimInfo *ai;
	ai = FindAnim(AMove, piece, axis);
	if (ai) {
		ai->listeners.push_back(listener);
		return 1;
	}
	else
		return 0;
}


//Flags as defined by the cob standard
void CUnitScript::Explode(int piece, int flags)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber for explode");
		return;
	}

#ifndef _CONSOLE
	float3 pos = GetPiecePos(piece) + unit->pos;

#ifdef TRACE_SYNC
	tracefile << "Cob explosion: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << piece << " " << flags << "\n";
#endif

	// Do an explosion at the location first
	new CHeatCloudProjectile(pos, float3(0, 0, 0), 30, 30, NULL);

	// If this is true, no stuff should fly off
	if (flags & PF_NONE) return;

	// This means that we are going to do a full fledged piece explosion!
	int newflags = 0;
	newflags |= PF_Fall; // if they don't fall they could live forever
	if (flags & PF_Explode) { newflags |= PF_Explode; }
//	if (flags & PF_Fall) { newflags |=  PF_Fall; }
	if ((flags & PF_Smoke) && ph->particleSaturation < 1) { newflags |= PF_Smoke; }
	if ((flags & PF_Fire) && ph->particleSaturation < 0.95f) { newflags |= PF_Fire; }
	if (flags & PF_NoCEGTrail) { newflags |= PF_NoCEGTrail; }

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

	LocalModelPiece* pieceData = pieces[piece];
	if (flags & PF_Shatter) {
		//Shatter

		float pieceChance=1-(ph->currentParticles-(ph->maxParticles-2000))/2000;

		if(pieceData->type == MODELTYPE_3DO){
			/* 3DO */

			S3DOPiece* dl = (S3DOPiece*)pieceData->original;

			for(std::vector<S3DOPrimitive>::iterator pi=dl->prims.begin();pi!=dl->prims.end();++pi){
				if(gu->usRandFloat()>pieceChance || pi->numVertex!=4)
					continue;

				ph->AddFlyingPiece(pos,speed+gu->usRandVector()*2,dl,&*pi);
			}
		} else {
			/* S3O */

			SS3OPiece* cookedPiece = (SS3OPiece*)pieceData->original;

			if (cookedPiece->primitiveType == 0){
				/* GL_TRIANGLES */

				for (size_t i = 0; i < cookedPiece->vertexDrawOrder.size(); i += 3){
					if(gu->usRandFloat()>pieceChance)
						continue;

					SS3OVertex * verts = new SS3OVertex[4];

					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 0]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 2]];

					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			} else if (cookedPiece->primitiveType == 1){
				/* GL_TRIANGLE_STRIP */
				for (size_t i = 2; i < cookedPiece->vertexDrawOrder.size(); i++){
					if(gu->usRandFloat()>pieceChance)
						continue;

					SS3OVertex * verts = new SS3OVertex[4];

					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 2]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 1]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 0]];

					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			} else if (cookedPiece->primitiveType == 2){
				/* GL_QUADS */

				for (size_t i = 0; i < cookedPiece->vertexDrawOrder.size(); i += 4){
					if(gu->usRandFloat()>pieceChance)
						continue;

					SS3OVertex * verts = new SS3OVertex[4];

					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 0]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 2]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 3]];

					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			}
		}
	}
	else {
		if (pieceData->original != NULL) {
			//logOutput.Print("Exploding %s as %d", script.pieceNames[piece].c_str(), dl);
			new CPieceProjectile(pos, speed, pieceData, newflags,unit,0.5f);
		}
	}
#endif
}


void CUnitScript::ShowFlare(int piece)
{
	if (!PieceExists(piece)) {
		ShowScriptError("Invalid piecenumber for show(flare)");
		return;
	}
#ifndef _CONSOLE
	float3 relpos = GetPiecePos(piece);
	float3 pos=unit->pos + unit->frontdir*relpos.z + unit->updir*relpos.y + unit->rightdir*relpos.x;
	float3 dir=unit->lastMuzzleFlameDir;

	float size=unit->lastMuzzleFlameSize;

	new CMuzzleFlame(pos, unit->speed,dir, size);
#endif
}


void CUnitScript::MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	//Make sure we do not overwrite animations of non-interpolated origin
	AnimInfo *ai = FindAnim(AMove, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			//logOutput.Print("Anim move overwrite");
			MoveNow(piece, axis, destination);
			return;
		}
	}

	float cur = pieces[piece]->pos[axis] - pieces[piece]->original->offset[axis];
	if (axis==0) {
		cur = -cur;
	}
	int dist = abs(destination - (int)(cur / CORDDIV));
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	int speed = (dist * timeFactor) / delta;

	//logOutput.Print("SmoothMove %d, %d got %d %d", piece, (int)(cur / CORDDIV), destination, speed);

	Move(piece, axis, speed, destination, true);
}


void CUnitScript::TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		ShowScriptWarning("Invalid piecenumber");
		return;
	}

	AnimInfo *ai = FindAnim(ATurn, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			//logOutput.Print("Anim turn overwrite");
			TurnNow(piece, axis, destination);
			return;
		}
	}

	float cur = pieces[piece]->rot[axis];
	short int dist = abs(destination - (short int)(cur * RAD2TAANG));
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	int speed = (dist * timeFactor) / delta;

	//logOutput.Print("Turnx %d:%d cur %d got %d %d dist %d", piece, axis, cur, destination, speed, dist);

	Turn(piece, axis, speed, destination, true);
}
