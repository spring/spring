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


CCobInstance::~CCobInstance(void)
{
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
	Call(COBFN_Killed, args, &CUnitKilledCB, this, NULL);

	//FIXME: moved from Unit.cpp, but seems like a bug?

	// If Killed script returns immediately then returned delayedWreckLevel
	// will have been popped from stack and stored in retCode, so args[1] will
	// have been set to 0 by RealCall, and CCobThread::CheckStack will have
	// warned about stack size mismatch...  Workaround is to not return but to
	// assign to the delayedWreckLevel parameter.

	// If, on the other hand, the Killed script sleeps or waits, then assigning
	// to the delayedWreckLevel parameter won't have any effect, because it
	// does not set retCode.  In this case there must be a
	// 		`return (delayedWreckLevel)'
	// statement.

	unit->delayedWreckLevel = args[1];
}


void CCobInstance::SetDirection(int heading)
{
	Call(COBFN_SetDirection, heading);
}


void CCobInstance::SetSpeed(float speed)
{
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
	Call(COBFN_HitByWeaponId, args, hitByWeaponIdCallback, this, NULL);
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


void CCobInstance::BeginTransport(CUnit* unit)
{
}


int CCobInstance::QueryTransport(CUnit* unit)
{
	return -1;
}


void CCobInstance::TransportPickup(CUnit* unit)
{
}


void CCobInstance::EndTransport()
{
}


void CCobInstance::TransportDrop(CUnit* unit, const float3& pos)
{
}


void CCobInstance::SetMaxReloadTime(int maxReloadMillis)
{
}


void CCobInstance::StartBuilding(int heading, int pitch)
{
}


void CCobInstance::StopBuilding()
{
}


int CCobInstance::QueryNanoPiece()
{
	return -1;
}


int CCobInstance::QueryWeapon(int weaponNum)
{
	return -1;
}


void CCobInstance::AimWeapon(int weaponNum, int heading, int pitch)
{
}


int CCobInstance::AimFromWeapon(int weaponNum)
{
	return -1;
}


void CCobInstance::Shot(int weaponNum)
{
}


bool CCobInstance::BlockShot(int weaponNum, CUnit* targetUnit, bool userTarget)
{
	return false;
}


float CCobInstance::TargetWeight(int weaponNum, CUnit* targetUnit)
{
	return 1.0;
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


void CCobInstance::ShowScriptWarning(const std::string& msg)
{
	GCobEngine.ShowScriptWarning(msg);
}
