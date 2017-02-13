/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobFile.h"
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
#include "System/Util.h"
#include "System/myMath.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncTracer.h"


/******************************************************************************/
/******************************************************************************/

CR_BIND_DERIVED(CCobInstance, CUnitScript, )

CR_REG_METADATA(CCobInstance, (
	CR_MEMBER(staticVars),
	CR_MEMBER(threads),

	//loaded from cobFileHandler
	CR_IGNORED(script),

	CR_POSTLOAD(PostLoad)
))


inline bool CCobInstance::HasFunction(int id) const
{
	return script->scriptIndex[id] >= 0;
}

//Used by creg
CCobInstance::CCobInstance()
	: CUnitScript(nullptr)
	, script(nullptr)
{ }

CCobInstance::CCobInstance(CCobFile* _script, CUnit* _unit)
	: CUnitScript(_unit)
	, script(_script)
{
	Init();
}

void CCobInstance::Init()
{
	assert(script != nullptr);

	staticVars.reserve(script->numStaticVars);
	for (int i = 0; i < script->numStaticVars; ++i) {
		staticVars.push_back(0);
	}

	MapScriptToModelPieces(&unit->localModel);

	hasSetSFXOccupy  = HasFunction(COBFN_SetSFXOccupy);
	hasRockUnit      = HasFunction(COBFN_RockUnit);
	hasStartBuilding = HasFunction(COBFN_StartBuilding);

}

void CCobInstance::PostLoad()
{
	assert(unit != nullptr);
	script = cobFileHandler->GetCobFile(unit->unitDef->scriptName);
	Init();
}


CCobInstance::~CCobInstance()
{
	//this may be dangerous, is it really desired?
	//Destroy();
	// Deleting waiting threads and unregistering all callbacks
	while (!threads.empty()) {
		CCobThread* t = threads.back();
		t->owner = nullptr;
		if (t->IsWaiting()) {
			delete t;
		} else {
			t->state = CCobThread::Dead;
		}
		threads.pop_back();
	}
}


void CCobInstance::MapScriptToModelPieces(LocalModel* lmodel)
{
	std::vector<std::string>& pieceNames = script->pieceNames; // already in lowercase!
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
			pieces.push_back(NULL);

			const char* fmtString = "[%s] could not find piece named \"%s\" (referenced by COB script \"%s\")";
			const char* pieceName = pieceNames[scriptPieceNum].c_str();
			const char* scriptName = script->name.c_str();

			LOG_L(L_WARNING, fmtString, __FUNCTION__, pieceName, scriptName);
		}
	}
}


int CCobInstance::GetFunctionId(const std::string& fname) const
{
	return script->GetFunctionId(fname);
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
	// Calculate the max() of the available weapon reloadtimes
	int maxReloadTime = 0;

	for (vector<CWeapon*>::iterator i = unit->weapons.begin(); i != unit->weapons.end(); ++i) {
		maxReloadTime = std::max(maxReloadTime, (*i)->reloadTime);

		#if 0
		if (dynamic_cast<CBeamLaser*>(*i))
			maxReloadTime = 150; // ???
		#endif
	}

	// convert ticks to milliseconds
	maxReloadTime *= GAME_SPEED;

	#if 0
	// TA does some special handling depending on weapon count, Spring != TA
	if (unit->weapons.size() > 1) {
		maxReloadTime = std::max(maxReloadTime, 3000);
	}
	#endif

	Call(COBFN_Create);
	Call(COBFN_SetMaxReloadTime, maxReloadTime);
}



void CCobInstance::Killed()
{
	vector<int> args;
	args.reserve(2);
	args.push_back((int) (unit->recentDamage / unit->maxHealth * 100));
	args.push_back(0);

	Call(COBFN_Killed, args, CBKilled, 0, nullptr);
}


void CCobInstance::WindChanged(float heading, float speed)
{
	Call(COBFN_SetSpeed, (int)(speed * 3000.0f));
	Call(COBFN_SetDirection, short(heading * RAD2TAANG));
}


void CCobInstance::ExtractionRateChanged(float speed)
{
	Call(COBFN_SetSpeed, (int)(speed * 500.0f));
	if (unit->activated) {
		Call(COBFN_Go);
	}
}


void CCobInstance::RockUnit(const float3& rockDir)
{
	vector<int> args;
	args.reserve(2);
	args.push_back(rockDir.z);
	args.push_back(rockDir.x);

	Call(COBFN_RockUnit, args);
}


void CCobInstance::HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	vector<int> args;
	args.reserve(4);
	args.push_back(hitDir.z);
	args.push_back(hitDir.x);

	if (HasFunction(COBFN_HitByWeaponId)) {
		const WeaponDef* wd = weaponDefHandler->GetWeaponDefByID(weaponDefId);

		args.push_back((wd != nullptr)? wd->tdfId : -1);
		args.push_back((int)(100 * inoutDamage));

		int weaponHitMod = 1;

		Call(COBFN_HitByWeaponId, args, CBNone, 0, &weaponHitMod);

		inoutDamage *= weaponHitMod * 0.01f;
	} else {
		Call(COBFN_HitByWeapon, args);
	}
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
	args.reserve(2);
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
	args.reserve(2);
	args.push_back(unit->id);
	args.push_back(PACKXZ(pos.x, pos.z));
	Call(COBFN_TransportDrop, args);
}


void CCobInstance::StartBuilding(float heading, float pitch)
{
	vector<int> args;
	args.reserve(2);
	args.push_back(short(heading * RAD2TAANG));
	args.push_back(short(pitch * RAD2TAANG));
	Call(COBFN_StartBuilding, args);
}


int CCobInstance::QueryNanoPiece()
{
	vector<int> args(1, -1);
	Call(COBFN_QueryNanoPiece, args);
	return args[0];
}


int CCobInstance::QueryBuildInfo()
{
	vector<int> args(1, -1);
	Call(COBFN_QueryBuildInfo, args);
	return args[0];
}


int CCobInstance::QueryWeapon(int weaponNum)
{
	vector<int> args(1, -1);
	Call(COBFN_QueryPrimary + COBFN_Weapon_Funcs * weaponNum, args);
	return args[0];
}


void CCobInstance::AimWeapon(int weaponNum, float heading, float pitch)
{
	vector<int> args;
	args.reserve(2);
	args.push_back(short(heading * RAD2TAANG));
	args.push_back(short(pitch * RAD2TAANG));
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weaponNum, args, CBAimWeapon, weaponNum, nullptr);
}


void CCobInstance::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	vector<int> args;
	args.reserve(2);
	args.push_back(0); // compat with AimWeapon (same script is called)
	args.push_back(0);
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weapon->weaponNum, args, CBAimShield, weapon->weaponNum, nullptr);
}


int CCobInstance::AimFromWeapon(int weaponNum)
{
	vector<int> args(1, -1);
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
	args.reserve(3);

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
	args.reserve(2);

	args.push_back(unitID);
	args.push_back(COBSCALE); // arg[1], for the return value
	                          // the default is 1.0

	Call(COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum, args);

	return (float)args[1] / (float)COBSCALE;
}

void CCobInstance::AnimFinished(AnimType type, int piece, int axis)
{
	for (CCobThread* t: threads) {
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
int CCobInstance::RealCall(int functionId, vector<int>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{

	if (functionId < 0 || size_t(functionId) >= script->scriptNames.size()) {
		if (retCode != nullptr)
			*retCode = -1;

		if (cb != CBNone) {
			// in case the function does not exist the callback should
			// still be called; -1 is the default CobThread return code
			ThreadCallback(cb, -1, cbParam);
		}
		return -1;
	}


	CCobThread* thread = new CCobThread(this);
	thread->Start(functionId, args, false);

	//LOG_L(L_DEBUG, "Calling %s:%s", script->name.c_str(), script->scriptNames[functionId].c_str());

	const bool res = thread->Tick();

	// Make sure this is run even if the call terminates instantly
	if (cb != CBNone)
		thread->SetCallback(cb, cbParam);

	if (!res) {
		// thread died already after one tick
		// NOTE:
		//   the StartMoving callin now takes an argument which means
		//   there will be a mismatch between the number of arguments
		//   passed in (1) and the number returned (0) as of 95.0 -->
		//   prevent error-spam
		unsigned int i = 0, argc = thread->CheckStack(args.size(), functionId != script->scriptIndex[COBFN_StartMoving]);

		// Retrieve parameter values from stack
		for (; i < argc; ++i)
			args[i] = thread->GetStackVal(i);

		// Set erroneous parameters to 0
		for (; i < args.size(); ++i)
			args[i] = 0;

		// dtor runs the callback
		if (retCode != nullptr)
			*retCode = thread->GetRetCode();
		delete thread;
		return 0;
	}

	// thread has already added itself to the correct
	// scheduler (global for sleep, or local for anim)
	return 1;
}


/******************************************************************************/


int CCobInstance::Call(const std::string& fname)
{
	vector<int> x;
	return Call(fname, x, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, std::vector<int>& args)
{
	return Call(fname, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, int arg1)
{
	vector<int> x;
	x.reserve(1);
	x.push_back(arg1);
	return Call(fname, x, CBNone, 0, nullptr);
}

int CCobInstance::Call(const std::string& fname, std::vector<int>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	//TODO: Check that new behaviour of actually calling cb when the function is not defined is right?
	//      (the callback has always been called [when the function is not defined]
	//       in the id-based Call()s, but never in the string based calls.)
	return RealCall(GetFunctionId(fname), args, cb, cbParam, retCode);
}


int CCobInstance::Call(int id)
{
	vector<int> x;
	return Call(id, x, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, int arg1)
{
	vector<int> x;
	x.reserve(1);
	x.push_back(arg1);
	return Call(id, x, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::vector<int>& args)
{
	return Call(id, args, CBNone, 0, nullptr);
}

int CCobInstance::Call(int id, std::vector<int>& args, ThreadCallbackType cb, int cbParam, int* retCode)
{
	return RealCall(script->scriptIndex[id], args, cb, cbParam, retCode);
}


void CCobInstance::RawCall(int fn)
{
	vector<int> x;
	RealCall(fn, x, CBNone, 0, nullptr);
}

int CCobInstance::RawCall(int fn, std::vector<int> &args)
{
	return RealCall(fn, args, CBNone, 0, nullptr);
}

void CCobInstance::ThreadCallback(ThreadCallbackType type, int retCode, int cbParam)
{
	switch(type) {
		// note: this callback is always called, even if Killed does not exist
		// however, retCode is only set if the function has a return statement
		// (otherwise its value is -1 regardless of Killed being present which
		// means *no* wreck will be spawned)
		case CBKilled:
			unit->deathScriptFinished = true;
			unit->delayedWreckLevel = retCode;
			break;
		case CBAimWeapon:
			unit->weapons[cbParam]->angleGood = (retCode == 1);
			break;
		case CBAimShield:
			static_cast<CPlasmaRepulser*>(unit->weapons[cbParam])->SetEnabled(retCode != 0);
			break;
		default:
			assert(false);
		}
}

/******************************************************************************/
/******************************************************************************/


void CCobInstance::Signal(int signal)
{
	for (CCobThread* t: threads) {
		if ((signal & t->signalMask) != 0) {
			t->state = CCobThread::Dead;
			//LOG_L(L_DEBUG, "Killing a thread %d %d", signal, (*i)->signalMask);
		}
	}
}


void CCobInstance::PlayUnitSound(int snr, int attr)
{
	Channels::UnitReply->PlaySample(script->sounds[snr], unit->pos, unit->speed, attr);
}


void CCobInstance::ShowScriptError(const std::string& msg)
{
	cobEngine->ShowScriptError(msg);
}
