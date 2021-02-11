/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobFile.h"
#include "CobFileHandler.h"
#include "CobInstance.h"
#include "CobThread.h"

#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/TeamHandler.h"
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
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/StringUtil.h"
#include "System/SpringMath.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncTracer.h"


/******************************************************************************/
/******************************************************************************/

CR_BIND_DERIVED(CCobInstance, CUnitScript, )

CR_REG_METADATA(CCobInstance, (
	// loaded from cobFileHandler
	CR_IGNORED(cobFile),

	CR_MEMBER(staticVars),
	CR_MEMBER(threadIDs),

	CR_POSTLOAD(PostLoad)
))


inline bool CCobInstance::HasFunction(int id) const
{
	return (cobFile->scriptIndex.size() > id && cobFile->scriptIndex[id] >= 0);
}



void CCobInstance::Init()
{
	assert(cobFile != nullptr);

	staticVars.clear();
	staticVars.resize(cobFile->numStaticVars, 0);

	MapScriptToModelPieces(&unit->localModel);

	hasSetSFXOccupy  = HasFunction(COBFN_SetSFXOccupy);
	hasRockUnit      = HasFunction(COBFN_RockUnit);
	hasStartBuilding = HasFunction(COBFN_StartBuilding);

}

void CCobInstance::PostLoad()
{
	assert(unit != nullptr);
	assert(cobFile == nullptr);

	cobFile = cobFileHandler->GetCobFile(unit->unitDef->scriptName);

	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);

		t->cobInst = this;
		t->cobFile = cobFile;
	}

	Init();
}


CCobInstance::~CCobInstance()
{
	// this may be dangerous, is it really desired?
	// Destroy();

	// delete our threads, make sure callbacks do not run
	while (!threadIDs.empty()) {
		CCobThread* t = cobEngine->GetThread(threadIDs.back());

		t->MakeGarbage();
		cobEngine->RemoveThread(t->GetID());

		threadIDs.pop_back();
	}

	cobEngine->SanityCheckThreads(this);
}


void CCobInstance::MapScriptToModelPieces(LocalModel* lmodel)
{
	std::vector<std::string>& pieceNames = cobFile->pieceNames; // already in lowercase!
	std::vector<LocalModelPiece>& lmodelPieces = lmodel->pieces;

	pieces.clear();
	pieces.reserve(pieceNames.size());

	// clear the default assumed 1:1 mapping
	for (size_t lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
		lmodelPieces[lmodelPieceNum].SetScriptPieceIndex(-1);
	}
	for (size_t scriptPieceNum = 0; scriptPieceNum < pieceNames.size(); scriptPieceNum++) {
		unsigned int lmodelPieceNum;

		// Map this piecename to an index in the script's pieceinfo
		for (lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
			if (lmodelPieces[lmodelPieceNum].original->name.compare(pieceNames[scriptPieceNum]) == 0) {
				break;
			}
		}

		// Not found? Try lowercase
		if (lmodelPieceNum == lmodelPieces.size()) {
			for (lmodelPieceNum = 0; lmodelPieceNum < lmodelPieces.size(); lmodelPieceNum++) {
				if (StringToLower(lmodelPieces[lmodelPieceNum].original->name).compare(pieceNames[scriptPieceNum]) == 0) {
					break;
				}
			}
		}

		// Did we find it?
		if (lmodelPieceNum < lmodelPieces.size()) {
			lmodelPieces[lmodelPieceNum].SetScriptPieceIndex(scriptPieceNum);
			pieces.push_back(&lmodelPieces[lmodelPieceNum]);
		} else {
			pieces.push_back(nullptr);

			const char* fmtString = "[%s] could not find piece named \"%s\" (referenced by COB script \"%s\")";
			const char* pieceName = pieceNames[scriptPieceNum].c_str();
			const char* scriptName = cobFile->name.c_str();

			LOG_L(L_WARNING, fmtString, __FUNCTION__, pieceName, scriptName);
		}
	}
}


int CCobInstance::GetFunctionId(const std::string& fname) const
{
	return cobFile->GetFunctionId(fname);
}


bool CCobInstance::HasBlockShot(int weaponNum) const
{
	return HasFunction(COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum);
}


bool CCobInstance::HasTargetWeight(int weaponNum) const
{
	return HasFunction(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum);
}


/******************************************************************************/
/******************************************************************************/


void CCobInstance::Create()
{
	// calculate maximum reload-time of the available weapons
	int maxReloadTime = 0;

	for (const CWeapon* w: unit->weapons) {
		maxReloadTime = std::max(maxReloadTime, w->reloadTime);
	}

	// convert ticks to milliseconds
	maxReloadTime *= GAME_SPEED;

	Call(COBFN_Create);
	Call(COBFN_SetMaxReloadTime, maxReloadTime);
}



void CCobInstance::Killed()
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(unit->recentDamage / unit->maxHealth * 100);
	callinArgs[2] = 0;

	Call(COBFN_Killed, callinArgs, CBKilled, 0, nullptr);
}


void CCobInstance::WindChanged(float heading, float speed)
{
	Call(COBFN_SetSpeed, int(speed * 3000.0f));
	Call(COBFN_SetDirection, short(heading * RAD2TAANG));
}


void CCobInstance::ExtractionRateChanged(float speed)
{
	Call(COBFN_SetSpeed, int(speed * 500.0f));

	if (!unit->activated)
		return;

	Call(COBFN_Go);
}


void CCobInstance::RockUnit(const float3& rockDir)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(rockDir.z);
	callinArgs[2] = int(rockDir.x);

	Call(COBFN_RockUnit, callinArgs);
}


void CCobInstance::HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = int(hitDir.z);
	callinArgs[2] = int(hitDir.x);

	if (HasFunction(COBFN_HitByWeaponId)) {
		const WeaponDef* wd = weaponDefHandler->GetWeaponDefByID(weaponDefId);

		callinArgs[0] = 4;
		callinArgs[3] = ((wd != nullptr)? wd->tdfId : -1);
		callinArgs[4] = int(100 * inoutDamage);
		// weaponHitMod, not an actual arg
		callinArgs[MAX_COB_ARGS] = 1;

		Call(COBFN_HitByWeaponId, callinArgs, CBNone, 0, &callinArgs[MAX_COB_ARGS]);

		inoutDamage *= (callinArgs[MAX_COB_ARGS] * 0.01f);
	} else {
		Call(COBFN_HitByWeapon, callinArgs);
	}
}


void CCobInstance::SetSFXOccupy(int curTerrainType)
{
	Call(COBFN_SetSFXOccupy, curTerrainType);
}


void CCobInstance::QueryLandingPads(std::vector<int>& outPieces)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = MAX_COB_ARGS; // default max pad count

	if (HasFunction(COBFN_QueryLandingPadCount)) {
		// query the count
		Call(COBFN_QueryLandingPadCount, callinArgs);

		// setup count as first arg for next query
		callinArgs[1] = callinArgs[0];
		callinArgs[0] = 1;
	}

	const int maxPadCount = std::min(callinArgs[1], int(MAX_COB_ARGS));

	// query the pieces
	Call(COBFN_QueryLandingPad, callinArgs);

	outPieces.clear();
	outPieces.resize(maxPadCount);
	outPieces.assign(callinArgs.begin(), callinArgs.begin() + maxPadCount);
}


void CCobInstance::BeginTransport(const CUnit* unit)
{
	// COB uses model height to identify units
	Call(COBFN_BeginTransport, int(unit->model->height * 65536));
}


int CCobInstance::QueryTransport(const CUnit* unit)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = 0;
	callinArgs[2] = int(unit->model->height * 65536);

	Call(COBFN_QueryTransport, callinArgs);
	return callinArgs[0];
}


void CCobInstance::TransportPickup(const CUnit* unit)
{
	// here COB uses unitIDs instead of model height
	Call(COBFN_TransportPickup, unit->id);
}


void CCobInstance::TransportDrop(const CUnit* unit, const float3& pos)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = unit->id;
	callinArgs[2] = PACKXZ(pos.x, pos.z);

	Call(COBFN_TransportDrop, callinArgs);
}


void CCobInstance::StartBuilding(float heading, float pitch)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = short(heading * RAD2TAANG);
	callinArgs[2] = short(  pitch * RAD2TAANG);

	Call(COBFN_StartBuilding, callinArgs);
}


int CCobInstance::QueryNanoPiece()
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryNanoPiece, callinArgs);
	return callinArgs[0];
}


int CCobInstance::QueryBuildInfo()
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryBuildInfo, callinArgs);
	return callinArgs[0];
}


int CCobInstance::QueryWeapon(int weaponNum)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_QueryPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs);
	return callinArgs[0];
}


void CCobInstance::AimWeapon(int weaponNum, float heading, float pitch)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = short(heading * RAD2TAANG);
	callinArgs[2] = short(  pitch * RAD2TAANG);

	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs, CBAimWeapon, weaponNum, nullptr);
}


void CCobInstance::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = 0; // compat with AimWeapon (same script is called)

	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weapon->weaponNum, callinArgs, CBAimShield, weapon->weaponNum, nullptr);
}


int CCobInstance::AimFromWeapon(int weaponNum)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] =  1;
	callinArgs[1] = -1;

	Call(COBFN_AimFromPrimary + COBFN_Weapon_Funcs * weaponNum, callinArgs);
	return callinArgs[0];
}


void CCobInstance::Shot(int weaponNum)
{
	Call(COBFN_Shot + COBFN_Weapon_Funcs * weaponNum, 0); // why the 0 argument?
}


bool CCobInstance::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 3;
	callinArgs[1] = (targetUnit != nullptr)? targetUnit->id : 0;
	callinArgs[2] = 0; // return value; default is to not block the shot
	callinArgs[3] = userTarget;

	Call(COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum, callinArgs);

	return (callinArgs[1] != 0);
}


float CCobInstance::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 2;
	callinArgs[1] = (targetUnit != nullptr)? targetUnit->id : 0;
	callinArgs[2] = COBSCALE; // return value; default is 1.0

	Call(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum, callinArgs);

	return (callinArgs[1] * 1.0f / COBSCALE);
}

void CCobInstance::AnimFinished(AnimType type, int piece, int axis)
{
	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);
		t->AnimFinished(type, piece, axis);
	}
}


void CCobInstance::Destroy() { Call(COBFN_Destroy); }
void CCobInstance::StartMoving(bool reversing) { Call(COBFN_StartMoving, reversing); }
void CCobInstance::StopMoving() { Call(COBFN_StopMoving); }
void CCobInstance::StartUnload() { Call(COBFN_StartUnload); }
void CCobInstance::EndTransport() { Call(COBFN_EndTransport); }
void CCobInstance::StartBuilding() { Call(COBFN_StartBuilding); }
void CCobInstance::StopBuilding() { Call(COBFN_StopBuilding); }
void CCobInstance::Falling() { Call(COBFN_Falling); }
void CCobInstance::Landed() { Call(COBFN_Landed); }
void CCobInstance::Activate() { Call(COBFN_Activate); }
void CCobInstance::Deactivate() { Call(COBFN_Deactivate); }
void CCobInstance::MoveRate(int curRate) { Call(COBFN_MoveRate0 + curRate); }
void CCobInstance::FireWeapon(int weaponNum) { Call(COBFN_FirePrimary + COBFN_Weapon_Funcs * weaponNum); }
void CCobInstance::EndBurst(int weaponNum) { Call(COBFN_EndBurst + COBFN_Weapon_Funcs * weaponNum); }


/******************************************************************************/


/**
 * @brief Calls a cob script function
 * @param functionId int cob script function id
 * @param args vector<int> function arguments
 * @param cb ThreadCallbackType Callback action
 * @param cbParam int callback argument
 * @return 0 if the call terminated. If the caller provides a callback and the thread does not terminate,
 *  it will continue to run. Otherwise it will be killed. Returns 1 in this case.
 */
int CCobInstance::RealCall(int functionId, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	int ret = -1;

	if (size_t(functionId) >= cobFile->scriptNames.size()) {
		if (retCode != nullptr)
			*retCode = ret;

		// in case the function does not exist the callback should
		// still be called; -1 is the default CobThread return code
		if (cb != CBNone)
			ThreadCallback(cb, -1, cbParam);

		return ret;
	}

	// LOG_L(L_DEBUG, "Calling %s:%s", cobFile->name.c_str(), cobFile->scriptNames[functionId].c_str());

	const int threadID = cobEngine->AddThread(std::move(CCobThread(this)));
	CCobThread* newThread = cobEngine->GetThread(threadID);


	// make sure this is run even if the call terminates instantly
	if (cb != CBNone)
		newThread->SetCallback(cb, cbParam);

	newThread->Start(functionId, 0, args, false);

	if ((ret = newThread->Tick()) == 0) {
		// thread died already after one tick
		// NOTE:
		//   ticking can trigger recursion, for example FireWeapon ->
		//   Call(COBFN_FirePrimary + N*) -> EmitSFX -> FireWeapon ->
		//   ...
		//
		//   the StartMoving callin now takes an argument which means
		//   there will be a mismatch between the number of arguments
		//   passed in (1) and the number returned (0) as of 95.0, so
		//   prevent error-spam by checking functionId
		//
		//   args[0] holds the number of input args
		const unsigned int numArgs = args[0];
		const unsigned int retArgs = newThread->CheckStack(numArgs, functionId != cobFile->scriptIndex[COBFN_StartMoving]);

		// retrieve output parameter values from stack
		for (unsigned int i = 0, n = std::min(retArgs, MAX_COB_ARGS); i < n; ++i)
			args[i] = newThread->GetStackVal(i);

		// set erroneous parameters to 0
		for (unsigned int i = std::min(retArgs, MAX_COB_ARGS); i < numArgs; ++i)
			args[i] = 0;

		// dtor runs the callback
		if (retCode != nullptr)
			*retCode = newThread->GetRetCode();

		cobEngine->RemoveThread(newThread->GetID());
	}

	// handle any spawned threads
	cobEngine->AddQueuedThreads();
	return ret;
}


/******************************************************************************/


int CCobInstance::Call(const std::string& fname)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	return Call(fname, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	return Call(fname, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, int arg1)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = arg1;

	return Call(fname, callinArgs, CBNone, 0, nullptr);
}



int CCobInstance::Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	//TODO: Check that new behaviour of actually calling cb when the function is not defined is right?
	//      (the callback has always been called [when the function is not defined]
	//       in the id-based Call()s, but never in the string based calls.)
	return RealCall(GetFunctionId(fname), args, cb, cbParam, retCode);
}



int CCobInstance::Call(int id)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	return Call(id, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, int arg1)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs;

	callinArgs[0] = 1;
	callinArgs[1] = arg1;

	return Call(id, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	return Call(id, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	return RealCall(cobFile->scriptIndex[id], args, cb, cbParam, retCode);
}


void CCobInstance::RawCall(int fn)
{
	std::array<int, 1 + MAX_COB_ARGS> callinArgs = {{0}};

	RealCall(fn, callinArgs, CBNone, 0, nullptr);
}

int CCobInstance::RawCall(int fn, std::array<int, 1 + MAX_COB_ARGS>& args)
{
	return RealCall(fn, args, CBNone, 0, nullptr);
}


void CCobInstance::ThreadCallback(ThreadCallbackType type, int retCode, int cbParam)
{
	switch (type) {
		// note: this callback is always called, even if Killed does not exist
		// however, retCode is only set if the function has a return statement
		// (otherwise its value is -1 regardless of Killed being present which
		// means *no* wreck will be spawned)
		case CBKilled: {
			unit->KilledScriptFinished(retCode);
		} break;
		case CBAimWeapon: {
			unit->weapons[cbParam]->AimScriptFinished(retCode == 1);
		} break;
		case CBAimShield: {
			static_cast<CPlasmaRepulser*>(unit->weapons[cbParam])->SetEnabled(retCode != 0);
		} break;
		default: {
			assert(false);
		} break;
	}
}

/******************************************************************************/
/******************************************************************************/


void CCobInstance::Signal(int signal)
{
	for (int threadID: threadIDs) {
		CCobThread* t = cobEngine->GetThread(threadID);

		if ((signal & t->GetSignalMask()) == 0)
			continue;

		t->SetState(CCobThread::Dead);
	}
}


void CCobInstance::PlayUnitSound(int snr, int attr)
{
	Channels::UnitReply->PlaySample(cobFile->sounds[snr], unit->pos, unit->speed, attr);
}


void CCobInstance::ShowScriptError(const std::string& msg)
{
	cobEngine->ShowScriptError(msg);
}
