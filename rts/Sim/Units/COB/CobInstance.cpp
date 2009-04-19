#include "StdAfx.h"
#include "mmgr.h"

#include "CobEngine.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "CobThread.h"

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
#include "Sound/AudioChannel.h"
#include "myMath.h"
#include "Sync/SyncTracer.h"

#endif


// Indices for set/get value
#define ACTIVATION           1  // set or get
#define STANDINGMOVEORDERS   2  // set or get
#define STANDINGFIREORDERS   3  // set or get
#define HEALTH               4  // get (0-100%)
#define INBUILDSTANCE        5  // set or get
#define BUSY                 6  // set or get (used by misc. special case missions like transport ships)
#define PIECE_XZ             7  // get
#define PIECE_Y              8  // get
#define UNIT_XZ              9  // get
#define UNIT_Y              10  // get
#define UNIT_HEIGHT         11  // get
#define XZ_ATAN             12  // get atan of packed x,z coords
#define XZ_HYPOT            13  // get hypot of packed x,z coords
#define ATAN                14  // get ordinary two-parameter atan
#define HYPOT               15  // get ordinary two-parameter hypot
#define GROUND_HEIGHT       16  // get land height, 0 if below water
#define BUILD_PERCENT_LEFT  17  // get 0 = unit is built and ready, 1-100 = How much is left to build
#define YARD_OPEN           18  // set or get (change which plots we occupy when building opens and closes)
#define BUGGER_OFF          19  // set or get (ask other units to clear the area)
#define ARMORED             20  // set or get

/*#define WEAPON_AIM_ABORTED  21
#define WEAPON_READY        22
#define WEAPON_LAUNCH_NOW   23
#define FINISHED_DYING      26
#define ORIENTATION         27*/
#define IN_WATER            28
#define CURRENT_SPEED       29
//#define MAGIC_DEATH         31
#define VETERAN_LEVEL       32
#define ON_ROAD             34

#define MAX_ID                    70
#define MY_ID                     71
#define UNIT_TEAM                 72
#define UNIT_BUILD_PERCENT_LEFT   73
#define UNIT_ALLIED               74
#define MAX_SPEED                 75
#define CLOAKED                   76
#define WANT_CLOAK                77
#define GROUND_WATER_HEIGHT       78 // get land height, negative if below water
#define UPRIGHT                   79 // set or get
#define	POW                       80 // get
#define PRINT                     81 // get, so multiple args can be passed
#define HEADING                   82 // get
#define TARGET_ID                 83 // get
#define LAST_ATTACKER_ID          84 // get
#define LOS_RADIUS                85 // set or get
#define AIR_LOS_RADIUS            86 // set or get
#define RADAR_RADIUS              87 // set or get
#define JAMMER_RADIUS             88 // set or get
#define SONAR_RADIUS              89 // set or get
#define SONAR_JAM_RADIUS          90 // set or get
#define SEISMIC_RADIUS            91 // set or get
#define DO_SEISMIC_PING           92 // get
#define CURRENT_FUEL              93 // set or get
#define TRANSPORT_ID              94 // get
#define SHIELD_POWER              95 // set or get
#define STEALTH                   96 // set or get
#define CRASHING                  97 // set or get, returns whether aircraft isCrashing state
#define CHANGE_TARGET             98 // set, the value it's set to determines the affected weapon
#define CEG_DAMAGE                99 // set
#define COB_ID                   100 // get
#define PLAY_SOUND               101 // get, so multiple args can be passed
#define KILL_UNIT                102 // get KILL_UNIT(unitId, SelfDestruct=true, Reclaimed=false)
#define ALPHA_THRESHOLD          103 // set or get
#define SET_WEAPON_UNIT_TARGET   106 // get (fake set)
#define SET_WEAPON_GROUND_TARGET 107 // get (fake set)
#define SONAR_STEALTH            108 // set or get

// NOTE: [LUA0 - LUA9] are defined in CobThread.cpp as [110 - 119]

#define FLANK_B_MODE             120 // set or get
#define FLANK_B_DIR              121 // set or get, set is through get for multiple args
#define FLANK_B_MOBILITY_ADD     122 // set or get
#define FLANK_B_MAX_DAMAGE       123 // set or get
#define FLANK_B_MIN_DAMAGE       124 // set or get
#define WEAPON_RELOADSTATE       125 // get (with fake set)
#define WEAPON_RELOADTIME        126 // get (with fake set)
#define WEAPON_ACCURACY          127 // get (with fake set)
#define WEAPON_SPRAY             128 // get (with fake set)
#define WEAPON_RANGE             129 // get (with fake set)
#define WEAPON_PROJECTILE_SPEED  130 // get (with fake set)
#define MIN                      131 // get
#define MAX                      132 // get
#define ABS                      133 // get
#define GAME_FRAME               134 // get

// NOTE: shared variables use codes [1024 - 5119]

std::vector<int>  CCobInstance::teamVars[TEAM_VAR_COUNT];
std::vector<int>  CCobInstance::allyVars[ALLY_VAR_COUNT];
int CCobInstance::globalVars[GLOBAL_VAR_COUNT]        =  { 0 };

void CCobInstance::InitVars(int numTeams, int numAllyTeams)
{
	for (int t = 0; t != TEAM_VAR_COUNT; ++t)
	{
		teamVars[t].resize(numTeams,0);
	}
	for (int t = 0; t != ALLY_VAR_COUNT; ++t)
	{
		allyVars[t].resize(numAllyTeams,0);
	}
}

CCobInstance::CCobInstance(CCobFile& _script, CUnit* _unit)
: script(_script), unit(_unit)
{
	staticVars.reserve(script.numStaticVars);
	for (int i = 0; i < script.numStaticVars; ++i) {
		staticVars.push_back(0);
	}

	memset(unitVars,int(0),UNIT_VAR_COUNT);

	MapScriptToModelPieces(unit->localmodel);

	yardOpen = false;
	busy = false;
	smoothAnim = unit->unitDef->smoothAnim;
}

CCobInstance::~CCobInstance(void)
{
	//Can't delete the thread here because that would confuse the scheduler to no end
	//Instead, mark it as dead. It is the function calling Tick that is responsible for delete.
	//Also unregister all callbacks
	for (std::list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		(*i)->state = CCobThread::Dead;
		(*i)->SetCallback(NULL, NULL, NULL);
	}

	// Remove us from possible animation ticking (should only be needed when anims.size() > 0
	GCobEngine.RemoveInstance(this);

	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		//All threads blocking on animations can be killed safely from here since the scheduler does not
		//know about them
		for (std::list<CCobThread *>::iterator j = (*i)->listeners.begin(); j != (*i)->listeners.end(); ++j) {
			delete *j;
		}
		delete *i;
	}
}



void CCobInstance::MapScriptToModelPieces(LocalModel* lmodel)
{
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
}


int CCobInstance::Call(const string &fname)
{
	std::vector<int> x;
	return Call(fname, x, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, vector<int> &args)
{
	return Call(fname, args, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, int p1)
{
	std::vector<int> x;
	x.push_back(p1);
	return Call(fname, x, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, CBCobThreadFinish cb, void *p1, void *p2)
{
	std::vector<int> x;
	return Call(fname, x, cb, p1, p2);
}

int CCobInstance::Call(const string &fname, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = script.getFunctionId(fname);
	if (fn == -1) {
		//logOutput.Print("CobError: unknown function %s called by user", fname.c_str());
		return -1;
	}

	return RealCall(fn, args, cb, p1, p2);
}

int CCobInstance::Call(int id)
{
	std::vector<int> x;
	return Call(id, x, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, int p1)
{
	std::vector<int> x;
	x.push_back(p1);
	return Call(id, x, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, vector<int> &args)
{
	return Call(id, args, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, CBCobThreadFinish cb, void *p1, void *p2)
{
	std::vector<int> x;
	return Call(id, x, cb, p1, p2);
}

int CCobInstance::Call(int id, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = script.scriptIndex[id];
	if (fn == -1) {
		//logOutput.Print("CobError: unknown function index %d called by user", id);
		if(cb){
			(*cb)(0, p1, p2);	//in case the function doesnt exist the callback should still be called
		}
		return -1;
	}

	return RealCall(fn, args, cb, p1, p2);
}


int CCobInstance::RawCall(int fn, vector<int> &args)
{
	return RawCall(fn, args, NULL, NULL, NULL);
}


int CCobInstance::RawCall(int fn, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	if ((fn < 0) || (static_cast<size_t>(fn) >= script.scriptNames.size())) {
		if (cb) {
			// in case the function doesnt exist the callback should still be called
			(*cb)(0, p1, p2);
		}
		return -1;
	}
	return RealCall(fn, args, cb, p1, p2);
}



/**
 * @brief Calls a cob script function
 * @param functionId int cob script function id
 * @param args vector<int> function arguments
 * @param cb CBCobThreadFinish Callback function
 * @param p1 void* callback argument #1
 * @param p2 void* callback argument #2
 * @return 0 if the call terminated. If the caller provides a callback and the thread does not terminate,
 *  it will continue to run. Otherwise it will be killed. Returns 1 in this case.
 */
int CCobInstance::RealCall(int functionId, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	CCobThread *t = new CCobThread(script, this);
	t->Start(functionId, args, false);

#if COB_DEBUG > 0
	if (COB_DEBUG_FILTER)
		logOutput.Print("Calling %s:%s", script.name.c_str(), script.scriptNames[functionId].c_str());
#endif

	int res = t->Tick(30);
	t->CommitAnims(30);

	//Make sure this is run even if the call terminates instantly
	if (cb)
		t->SetCallback(cb, p1, p2);

	if (res == -1) {
		unsigned int i = 0, argc = t->CheckStack(args.size());
		//Retrieve parameter values from stack
		for (; i < argc; ++i)
			args[i] = t->GetStackVal(i);
		//Set erroneous parameters to 0
		for (; i < args.size(); ++i)
			args[i] = 0;
		delete t;
		return 0;
	}
	else {
		//It has already added itself to the correct scheduler (global for sleep, or local for anim)
		return 1;
	}
}


/**
 * @brief Updates move animations
 * @param cur float value to update
 * @param dest float final value
 * @param speed float max increment per tick
 * @return returns 1 if destination was reached, 0 otherwise
 */
int CCobInstance::MoveToward(float &cur, float dest, float speed)
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
int CCobInstance::TurnToward(float &cur, float dest, float speed)
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
int CCobInstance::DoSpin(float &cur, float dest, float &speed, float accel, int divisor)
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
 * @brief Unblocks all threads waiting on an animation
 * @param anim AnimInfo the corresponding animation
 */
void CCobInstance::UnblockAll(struct AnimInfo * anim)
{
	std::list<CCobThread *>::iterator li;

	for (li = anim->listeners.begin(); li != anim->listeners.end(); ++li) {
		//Not sure how to do this more cleanly.. Will probably rewrite it
		if (((*li)->state == CCobThread::WaitMove) ||((*li)->state == CCobThread::WaitTurn)) {
			(*li)->state = CCobThread::Run;
			GCobEngine.AddThread(*li);
		}
		else if ((*li)->state == CCobThread::Dead) {
			delete *li;
		}
		else {
			logOutput.Print("CobError: Turn/move listenener in strange state %d", (*li)->state);
		}
	}
}


/**
 * @brief Called by the engine when we are registered as animating. If we return -1 it means that
 *        there is no longer anything animating
 * @param deltaTime int delta time to update
 * @return 0 if there are still animations going, -1 else
 */
int CCobInstance::Tick(int deltaTime)
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
struct CCobInstance::AnimInfo *CCobInstance::FindAnim(AnimType type, int piece, int axis)
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis))
			return *i;
	}
	return NULL;
}

//Optimize this?
// Returns true if an animation was found and deleted
void CCobInstance::RemoveAnim(AnimType type, int piece, int axis)
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis)) {

			// We need to unblock threads waiting on this animation, otherwise they will be lost in the void
			UnblockAll(*i);

			delete *i;
			anims.erase(i);

			// If this was the last animation, remove from currently animating list
			if (anims.size() == 0) {
				GCobEngine.RemoveInstance(this);
			}
			return;
		}
	}
}

//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CCobInstance::AddAnim(AnimType type, int piece, int axis, int speed, int dest, int accel, bool interpolated)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
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

		//If we were not animating before, inform the engine of this so it can schedule us
		if (anims.size() == 1) {
			GCobEngine.AddInstance(this);
		}
	}

	ai->dest  = destf;
	ai->speed = speedf;
	ai->accel = accelf;
	ai->interpolated = interpolated;
}

void CCobInstance::Spin(int piece, int axis, int speed, int accel)
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

void CCobInstance::StopSpin(int piece, int axis, int decel)
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

void CCobInstance::Turn(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(ATurn, piece, axis, speed, destination, 0, interpolated);
}

void CCobInstance::Move(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(AMove, piece, axis, speed, destination, 0, interpolated);
}

void CCobInstance::MoveNow(int piece, int axis, int destination)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
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

void CCobInstance::TurnNow(int piece, int axis, int destination)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	p->rot[axis] = destination * TAANG2RAD;
	p->updated = true;
	//logOutput.Print("moving %s on axis %d to %d", script.pieceNames[piece].c_str(), axis, destination);
}

void CCobInstance::SetVisibility(int piece, bool visible)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
		return;
	}

	LocalModelPiece* p = pieces[piece];
	if (p->visible != visible) {
		p->visible = visible;
		p->updated = true;
	}
}

void CCobInstance::EmitSfx(int type, int piece)
{
#ifndef _CONSOLE
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber for emit-sfx");
		return;
	}

	if(ph->particleSaturation>1 && type<1024){		//skip adding particles when we have to many (make sure below can be unsynced)
		return;
	}

	float3 relPos(0,0,0);
	float3 relDir(0,1,0);
	if (!GetEmitDirPos(piece, relPos, relDir)) {
		GCobEngine.ShowScriptError("emit-sfx: GetEmitDirPos failed\n");
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
					GCobEngine.ShowScriptError("Invalid explosion generator index for emit-sfx");
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
					GCobEngine.ShowScriptError("Invalid weapon index for emit-sfx");
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
					GCobEngine.ShowScriptError("Invalid weapon index for emit-sfx");
					break;
				}
				// detonate weapon from piece
				const WeaponDef* weaponDef = unit->weapons[index]->weaponDef;
				if (weaponDef->soundhit.getID(0) > 0) {
					Channels::Battle.PlaySample(weaponDef->soundhit.getID(0), unit, weaponDef->soundhit.getVolume(0));
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

void CCobInstance::AttachUnit(int piece, int u)
{
	// -1 is valid, indicates that the unit should be hidden
	if ((piece >= 0) && (!PieceExists(piece))) {
		GCobEngine.ShowScriptError("Invalid piecenumber for attach");
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

void CCobInstance::DropUnit(int u)
{
#ifndef _CONSOLE
	CTransportUnit* tu=dynamic_cast<CTransportUnit*>(unit);

	if(tu && uh->units[u]){
		tu->DetachUnit(uh->units[u]);
	}
#endif
}

//Returns 1 if there was a turn to listen to
int CCobInstance::AddTurnListener(int piece, int axis, CCobThread *listener)
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

int CCobInstance::AddMoveListener(int piece, int axis, CCobThread *listener)
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

void CCobInstance::Signal(int signal)
{
	for (std::list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		if ((signal & (*i)->signalMask) != 0) {
			(*i)->state = CCobThread::Dead;
			//logOutput.Print("Killing a thread %d %d", signal, (*i)->signalMask);
		}
	}
}

//Flags as defined by the cob standard
void CCobInstance::Explode(int piece, int flags)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptError("Invalid piecenumber for explode");
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

void CCobInstance::PlayUnitSound(int snr, int attr)
{
	int sid = script.sounds[snr];
	//logOutput.Print("Playing %d %d %d", snr, attr, sid);
	Channels::UnitReply.PlaySample(sid, unit->pos, unit->speed, attr);
}

void CCobInstance::ShowFlare(int piece)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptError("Invalid piecenumber for show(flare)");
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


int CCobInstance::GetUnitVal(int val, int p1, int p2, int p3, int p4)
{
#ifndef _CONSOLE
	switch(val)
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
	case HEALTH:{
		if (p1 <= 0)
			return (int) ((unit->health/unit->maxHealth)*100.0f);
		CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u == NULL)
			return 0;
		else
			return (int) ((u->health/u->maxHealth)*100.0f);
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
	case PIECE_XZ:{
		if (!PieceExists(p1))
			GCobEngine.ShowScriptError("Invalid piecenumber for get piece_xz");
		float3 relPos = GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return PACKXZ(pos.x, pos.z);}
	case PIECE_Y:{
		if (!PieceExists(p1))
			GCobEngine.ShowScriptError("Invalid piecenumber for get piece_y");
		float3 relPos = GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return (int)(pos.y * COBSCALE);}
	case UNIT_XZ: {
		if (p1 <= 0)
			return PACKXZ(unit->pos.x, unit->pos.z);
		CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u == NULL)
			return PACKXZ(0,0);
		else
			return PACKXZ(u->pos.x, u->pos.z);}
	case UNIT_Y: {
		//logOutput.Print("Unit-y %d", p1);
		if (p1 <= 0)
			return (int)(unit->pos.y * COBSCALE);
		CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u == NULL)
			return 0;
		else
			return (int)(u->pos.y * COBSCALE);}
	case UNIT_HEIGHT:{
		if (p1 <= 0)
			return (int)(unit->radius * COBSCALE);
		CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u == NULL)
			return 0;
		else
			return (int)(u->radius * COBSCALE);}
	case XZ_ATAN:
		return (int)(RAD2TAANG*atan2((float)UNPACKX(p1), (float)UNPACKZ(p1)) + 32768 - unit->heading);
	case XZ_HYPOT:
		return (int)(hypot((float)UNPACKX(p1), (float)UNPACKZ(p1)) * COBSCALE);
	case ATAN:
		return (int)(RAD2TAANG*atan2((float)p1, (float)p2));
	case HYPOT:
		return (int)hypot((float)p1, (float)p2);
	case GROUND_HEIGHT:
		return (int)(ground->GetHeight(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case GROUND_WATER_HEIGHT:
		return (int)(ground->GetHeight2(UNPACKX(p1), UNPACKZ(p1)) * COBSCALE);
	case BUILD_PERCENT_LEFT:
		return (int)((1 - unit->buildProgress) * 100);
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
		return (int)(100*unit->experience);
	case CURRENT_SPEED:
		if (unit->moveType)
			return (int)(unit->speed.Length()*COBSCALE);
		return 0;
	case ON_ROAD:
		return 0;
	case IN_WATER:
		return (unit->pos.y < 0.0f) ? 1 : 0;
	case MAX_ID:
		return uh->MaxUnits()-1;
	case MY_ID:
		return unit->id;
	case UNIT_TEAM:{
		CUnit *u = (p1 >= 0 && p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		return u ? unit->team : 0; }
	case UNIT_ALLIED:{
		CUnit *u = (p1 >= 0 && p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u) return teamHandler->Ally (unit->allyteam, u->allyteam) ? 1 : 0;
		return 0;}
	case UNIT_BUILD_PERCENT_LEFT:{
		CUnit *u = (p1 >= 0 && p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u) return (int)((1 - u->buildProgress) * 100);
		return 0;}
	case MAX_SPEED:
		if(unit->moveType){
			return int(unit->moveType->maxSpeed*COBSCALE);
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
		if (p1 <= 0)
			return unit->heading;
		CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
		if (u == NULL)
			return -1;
		else
			return u->heading;
	}
	case TARGET_ID:
		if (unit->weapons[p1-1]) {
			CWeapon* weapon = unit->weapons[p1-1];
			TargetType tType = weapon->targetType;
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
	case LAST_ATTACKER_ID:
		return unit->lastAttacker?unit->lastAttacker->id:-1;
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
		const CPlasmaRepulser* shield = (CPlasmaRepulser*)unit->shieldWeapon;
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
			const CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
			return (u == NULL) ? -1 : u->unitDef->cobID;
		}
	}
 	case PLAY_SOUND: {
		if ((p1 < 0) || (static_cast<size_t>(p1) >= script.sounds.size())) {
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
			Channels::UnitReply.PlaySample(script.sounds[p1], unit->pos, unit->speed, float(p2) / COBSCALE);
		} else {
			Channels::UnitReply.PlaySample(script.sounds[p1], float(p2) / COBSCALE);
		}
		return 0;
	}
	case SET_WEAPON_UNIT_TARGET: {
		const int weaponID = p1 - 1;
		const int targetID = p2;
		const bool userTarget = !!p3;
		if ((weaponID < 0) || (static_cast<size_t>(weaponID) >= unit->weapons.size())) {
			return 0;
		}
		CWeapon* weapon = unit->weapons[weaponID];
		if (weapon == NULL) { return 0; }
		if ((targetID < 0) || (static_cast<size_t>(targetID) >= uh->MaxUnits())) { return 0; }
		CUnit* target = (targetID == 0) ? NULL : uh->units[targetID];
		return weapon->AttackUnit(target, userTarget) ? 1 : 0;
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
	case FLANK_B_MODE:
		return unit->flankingBonusMode;
	case FLANK_B_DIR:
		switch(p1){
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
		if (p1 >= 0 && p1 < uh->MaxUnits()) {
			CUnit *u = p1 ? uh->units[p1] : unit;
			if (!u) {
				return 0;
			}
			if (u->beingBuilt) u->KillUnit(false, true, NULL); // no explosions and no corpse for units under construction
			else u->KillUnit(p2!=0, p3!=0, NULL);
			return 1;
		}
		return 0;
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
				const CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
				return (u == NULL) ? 0 : u->cob->unitVars[varID];
			}
			else {
				// set the unit var for another unit
				p1 = -p1;
				const CUnit *u = (p1 < uh->MaxUnits()) ? uh->units[p1] : NULL;
				if (u != NULL) {
					u->cob->unitVars[varID] = p2;
					return 1;
				}
				return 0;
			}
		}
		else {
			logOutput.Print("CobError: Unknown get constant %d  (params = %d %d %d %d)",
			                val, p1, p2, p3, p4);
		}
	}
#endif

	return 0;
}

void CCobInstance::SetUnitVal(int val, int param)
{
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
			//logOutput.Print("buildstance %d", param);
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
			if (unit->yardMap != 0x0) {
				// note: if this unit is a factory, engine-controlled
				// OpenYard() and CloseYard() calls can interfere with
				// the yardOpen state (they probably should be removed
				// at some point)
				if (param == 0) {
					if (groundBlockingObjectMap->CanCloseYard(unit)) {
						groundBlockingObjectMap->CloseBlockingYard(unit, unit->yardMap);
						yardOpen = false;
					}
				} else {
					groundBlockingObjectMap->OpenBlockingYard(unit, unit->yardMap);
					yardOpen = true;
				}
			}
			break;
		}
		case BUGGER_OFF: {
			if (param != 0) {
				helper->BuggerOff(unit->pos + unit->frontdir * unit->radius, unit->radius * 1.5f);
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

bool CCobInstance::HasScriptFunction(int id)
{
	return (script.scriptIndex[id] >= 0);
}

void CCobInstance::MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
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

void CCobInstance::TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	if (!PieceExists(piece)) {
		GCobEngine.ShowScriptWarning("Invalid piecenumber");
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


bool CCobInstance::FunctionExist(int id)
{
	if(script.scriptIndex[id]==-1)
		return false;
	return true;
}


int CCobInstance::GetFunctionId(const string& funcName) const
{
	return script.getFunctionId(funcName);
}
