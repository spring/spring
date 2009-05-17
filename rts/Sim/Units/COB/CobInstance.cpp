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
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Sound/AudioChannel.h"
#include "myMath.h"
#include "Sync/SyncTracer.h"

#endif


/******************************************************************************/
/******************************************************************************/


CCobInstance::CCobInstance(CCobFile& _script, CUnit* _unit)
	: CUnitScript(_unit, _script.scriptIndex, pieces)
	, script(_script)
	, smoothAnim(unit->unitDef->smoothAnim)
{
	staticVars.reserve(script.numStaticVars);
	for (int i = 0; i < script.numStaticVars; ++i) {
		staticVars.push_back(0);
	}

	MapScriptToModelPieces(unit->localmodel);
}


CCobInstance::~CCobInstance()
{
	for (std::list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		//All threads blocking on animations can be killed safely from here since the scheduler does not
		//know about them
		for (std::list<IAnimListener *>::iterator j = (*i)->listeners.begin(); j != (*i)->listeners.end(); ++j) {
			delete *j;
		}
		// the anims are deleted in ~CUnitScript
	}

	//Can't delete the thread here because that would confuse the scheduler to no end
	//Instead, mark it as dead. It is the function calling Tick that is responsible for delete.
	//Also unregister all callbacks
	for (std::list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		(*i)->state = CCobThread::Dead;
		(*i)->SetCallback(NULL, NULL, NULL);
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


int CCobInstance::GetFunctionId(const std::string& fname) const
{
	return script.GetFunctionId(fname);
}




/******************************************************************************/
/******************************************************************************/


void CCobInstance::Create()
{
	// Calculate the max() of the available weapon reloadtimes
	int relMax = 0;
	for (vector<CWeapon*>::iterator i = unit->weapons.begin(); i != unit->weapons.end(); ++i) {
		if ((*i)->reloadTime > relMax)
			relMax = (*i)->reloadTime;
		if (dynamic_cast<CBeamLaser*>(*i))
			relMax = 150;
	}

	// convert ticks to milliseconds
	relMax *= 30;

	// TA does some special handling depending on weapon count
	if (unit->weapons.size() > 1) {
		relMax = std::max(relMax, 3000);
	}

	Call(COBFN_Create);
	Call(COBFN_SetMaxReloadTime, relMax);
}


// Called when a unit's Killed script finishes executing
static void CUnitKilledCB(int retCode, void* p1, void* p2)
{
	CUnit* self = (CUnit*) p1;
	self->deathScriptFinished = true;
	self->delayedWreckLevel = retCode;
}


void CCobInstance::Killed()
{
	vector<int> args;
	args.push_back((int) (unit->recentDamage / unit->maxHealth * 100));
	args.push_back(0);
	Call(COBFN_Killed, args, &CUnitKilledCB, unit, NULL);
	unit->delayedWreckLevel = args[1];
}


void CCobInstance::SetDirection(float heading)
{
	Call(COBFN_SetDirection, short(heading * RAD2TAANG));
}


void CCobInstance::SetSpeed(float speed, float cob_mult)
{
	Call(COBFN_SetSpeed, (int)(speed * cob_mult));
}


void CCobInstance::RockUnit(const float3& rockDir)
{
	vector<int> rockAngles;
	rockAngles.push_back((int)(500 * rockDir.z));
	rockAngles.push_back((int)(500 * rockDir.x));
	Call(COBFN_RockUnit, rockAngles);
}


void CCobInstance::HitByWeapon(const float3& hitDir)
{
	vector<int> args;
	args.push_back((int)(500 * hitDir.z));
	args.push_back((int)(500 * hitDir.x));
	Call(COBFN_HitByWeapon, args);
}


// ugly hack to get return value of HitByWeaponId script
static float weaponHitMod; //fraction of weapondamage to use when hit by weapon
static void hitByWeaponIdCallback(int retCode, void *p1, void *p2) { weaponHitMod = retCode*0.01f; }


void CCobInstance::HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage)
{
	vector<int> args;

	args.push_back((int)(500 * hitDir.z));
	args.push_back((int)(500 * hitDir.x));

	if (weaponDefId != -1) {
		args.push_back(weaponDefHandler->weaponDefs[weaponDefId].tdfId);
	} else {
		args.push_back(-1);
	}

	args.push_back((int)(100 * inout_damage));

	weaponHitMod = 1.0f;
	Call(COBFN_HitByWeaponId, args, hitByWeaponIdCallback, NULL, NULL);
	inout_damage *= weaponHitMod; // weaponHitMod gets set in callback function
}


void CCobInstance::SetSFXOccupy(int curTerrainType)
{
	Call(COBFN_SetSFXOccupy, curTerrainType);
}


void CCobInstance::QueryLandingPads(std::vector<int>& out_pieces)
{
	int maxPadCount = 16; // default max pad count

	if (HasFunction(COBFN_QueryLandingPadCount)) {
		vector<int> args;
		args.push_back(maxPadCount);
		Call(COBFN_QueryLandingPadCount, args);
		maxPadCount = args[0];
	}

	for (int i = 0; i < maxPadCount; i++) {
		out_pieces.push_back(-1);
	}

	Call(COBFN_QueryLandingPad, out_pieces);
}


void CCobInstance::BeginTransport(const CUnit* unit)
{
	// yes, COB is silly, while it handles integers fine it uses model height to identify units
	Call(COBFN_BeginTransport, (int)(unit->model->height*65536));
}


int CCobInstance::QueryTransport(const CUnit* unit)
{
	vector<int> args;
	args.push_back(0);
	args.push_back((int)(unit->model->height*65536));
	Call(COBFN_QueryTransport, args);
	return args[0];
}


void CCobInstance::TransportPickup(const CUnit* unit)
{
	// funny, now it uses unitIDs instead of model height
	Call(COBFN_TransportPickup, unit->id);
}


void CCobInstance::TransportDrop(const CUnit* unit, const float3& pos)
{
	vector<int> args;
	args.push_back(unit->id);
	args.push_back(PACKXZ(pos.x, pos.z));
	Call(COBFN_TransportDrop, args);
}


void CCobInstance::StartBuilding(float heading, float pitch)
{
	vector<int> args;
	args.push_back(short(heading * RAD2TAANG));
	args.push_back(short(pitch * RAD2TAANG));
	Call(COBFN_StartBuilding, args);
}


int CCobInstance::QueryNanoPiece()
{
	vector<int> args(1, 0);
	Call(COBFN_QueryNanoPiece, args);
	return args[0];
}


int CCobInstance::QueryBuildInfo()
{
	vector<int> args(1, 0);
	Call(COBFN_QueryBuildInfo, args);
	return args[0];
}


int CCobInstance::QueryWeapon(int weaponNum)
{
	vector<int> args(1, 0);
	Call(COBFN_QueryPrimary + COBFN_Weapon_Funcs * weaponNum, args);
	return args[0];
}


// Called when unit's AimWeapon script finished executing
static void ScriptCallback(int retCode, void* p1, void* p2)
{
	if (retCode == 1) {
		((CWeapon*)p1)->angleGood = true;
	}
}

void CCobInstance::AimWeapon(int weaponNum, float heading, float pitch)
{
	vector<int> args;
	args.push_back(short(heading * RAD2TAANG));
	args.push_back(short(pitch * RAD2TAANG));
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weaponNum, args, ScriptCallback, unit->weapons[weaponNum], NULL);
}


// Called when unit's AimWeapon script finished executing (for shield weapon)
static void ShieldScriptCallback(int retCode, void* p1, void* p2)
{
	((CPlasmaRepulser*)p1)->isEnabled = !!retCode;
}

void CCobInstance::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	vector<int> args;
	args.push_back(0); // compat with AimWeapon (same script is called)
	args.push_back(0);
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weapon->weaponNum, args, ShieldScriptCallback, weapon, 0);
}


int CCobInstance::AimFromWeapon(int weaponNum)
{
	vector<int> args(1, 0);
	Call(COBFN_AimFromPrimary + COBFN_Weapon_Funcs * weaponNum, args);
	return args[0];
}


void CCobInstance::Shot(int weaponNum)
{
	Call(COBFN_Shot + COBFN_Weapon_Funcs * weaponNum, 0); // why the 0 argument?
}


bool CCobInstance::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	const int unitID = targetUnit ? targetUnit->id : 0;

	vector<int> args;

	args.push_back(unitID);
	args.push_back(0); // arg[1], for the return value
	                   // the default is to not block the shot
	args.push_back(userTarget);

	Call(COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum, args);

	return !!args[1];
}


float CCobInstance::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	const int unitID = targetUnit ? targetUnit->id : 0;

	vector<int> args;

	args.push_back(unitID);
	args.push_back(COBSCALE); // arg[1], for the return value
	                          // the default is 1.0

	Call(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum, args);

	return (float)args[1] / (float)COBSCALE;
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
	if (functionId < 0 || size_t(functionId) >= script.scriptNames.size()) {
		if (cb) {
			// in case the function doesnt exist the callback should still be called
			(*cb)(0, p1, p2);
		}
		return -1;
	}

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


/******************************************************************************/


int CCobInstance::Call(const string &fname)
{
	vector<int> x;
	return Call(fname, x, NULL, NULL, NULL);
}


int CCobInstance::Call(const string &fname, vector<int> &args)
{
	return Call(fname, args, NULL, NULL, NULL);
}


int CCobInstance::Call(const string &fname, int p1)
{
	vector<int> x;
	x.push_back(p1);
	return Call(fname, x, NULL, NULL, NULL);
}


int CCobInstance::Call(const string &fname, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = GetFunctionId(fname);
	//TODO: Check that new behaviour of actually calling cb when the function is not defined is right?
	//      (the callback has always been called [when the function is not defined]
	//       in the id-based Call()s, but never in the string based calls.)
	return RealCall(fn, args, cb, p1, p2);
}


int CCobInstance::Call(int id)
{
	vector<int> x;
	return Call(id, x, NULL, NULL, NULL);
}


int CCobInstance::Call(int id, int p1)
{
	vector<int> x;
	x.push_back(p1);
	return Call(id, x, NULL, NULL, NULL);
}


int CCobInstance::Call(int id, vector<int> &args)
{
	return Call(id, args, NULL, NULL, NULL);
}


int CCobInstance::Call(int id, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	return RealCall(scriptIndex[id], args, cb, p1, p2);
}


void CCobInstance::RawCall(int fn)
{
	vector<int> x;
	RealCall(fn, x, NULL, NULL, NULL);
}


int CCobInstance::RawCall(int fn, vector<int> &args)
{
	return RealCall(fn, args, NULL, NULL, NULL);
}


/******************************************************************************/
/******************************************************************************/


void CCobInstance::Signal(int signal)
{
	for (std::list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		if ((signal & (*i)->signalMask) != 0) {
			(*i)->state = CCobThread::Dead;
			//logOutput.Print("Killing a thread %d %d", signal, (*i)->signalMask);
		}
	}
}


void CCobInstance::PlayUnitSound(int snr, int attr)
{
	int sid = script.sounds[snr];
	//logOutput.Print("Playing %d %d %d", snr, attr, sid);
	Channels::UnitReply.PlaySample(sid, unit->pos, unit->speed, attr);
}


void CCobInstance::ShowScriptError(const std::string& msg)
{
	GCobEngine.ShowScriptError(msg);
}
