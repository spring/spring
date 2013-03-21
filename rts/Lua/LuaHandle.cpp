/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandle.h"

#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaUI.h"

#include "LuaCallInCheck.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaBitOps.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/BaseNetProtocol.h" // FIXME: for MAPDRAW_*
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Rectangle.h"
#include "System/ScopedFPUSettings.h"
#include "System/Log/ILog.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/FileHandler.h"

#include "LuaInclude.h"

#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include <string>


bool CLuaHandle::devMode = false;
bool CLuaHandle::modUICtrl = true;
bool CLuaHandle::useDualStates = false;


/******************************************************************************/
/******************************************************************************/

static void PushTracebackFuncToRegistry(lua_State* L)
{
	LUA_OPEN_LIB(L, luaopen_debug);
		HSTR_PUSH(L, "traceback");
		LuaUtils::PushDebugTraceback(L);
		lua_rawset(L, LUA_REGISTRYINDEX);
	// We only need the debug.traceback function, the others are unsafe for syncing.
	// Later CLuaHandle implementations decide themselves if they want to reload the lib or not (LuaUI does).
	LUA_UNLOAD_LIB(L, LUA_DBLIBNAME);
}


CLuaHandle::CLuaHandle(const string& _name, int _order, bool _userMode)
	: CEventClient(_name, _order, false) // FIXME
	, userMode   (_userMode)
	, killMe     (false)
	, callinErrors(0)
{
	UpdateThreading();

	SetSynced(false, true);
	D_Sim.owner = this;
	L_Sim = LUA_OPEN(&D_Sim, GetUserMode(), true);
	D_Draw.owner = this;
	L_Draw = LUA_OPEN(&D_Draw, GetUserMode(), false);

	// needed for engine traceback
	PushTracebackFuncToRegistry(L_Sim);
	PushTracebackFuncToRegistry(L_Draw);
}


CLuaHandle::~CLuaHandle()
{
	eventHandler.RemoveClient(this);

	// free the lua state
	KillLua();

	for(int i = 0; i < delayedCallsFromSynced.size(); ++i) {
		DelayDataDump& ddp = delayedCallsFromSynced[i];
		for(int d = 0; d < ddp.data.size(); ++d) {
			LuaUtils::ShallowDataDump& sdd = ddp.data[d];
			if(sdd.type == LUA_TSTRING)
				delete sdd.data.str;
		}
	}
	delayedCallsFromSynced.clear();
}


void CLuaHandle::UpdateThreading() {
	int mtl = globalConfig->GetMultiThreadLua();
	useDualStates = (mtl == MT_LUA_DUAL_EXPORT || mtl == MT_LUA_DUAL || mtl == MT_LUA_DUAL_ALL || mtl == MT_LUA_DUAL_UNMANAGED);
	singleState = (mtl != MT_LUA_DUAL_ALL && mtl != MT_LUA_DUAL_UNMANAGED);
	copyExportTable = false;
	useEventBatch = singleState && (mtl != MT_LUA_NONE && mtl != MT_LUA_SINGLE);
	purgeCallsFromSyncedBatch = useDualStates && (mtl != MT_LUA_DUAL_UNMANAGED);
}


void CLuaHandle::KillLua()
{
	if (L_Draw != NULL) {
		lua_State* L_Old = L_Sim;
		L_Sim = L_Draw;
		SetRunning(L_Draw, true);
		LUA_CLOSE(L_Draw);
		//SetRunning(L_Draw, false); --nope, the state is deleted
		L_Draw = NULL;
		L_Sim = L_Old;
	}
	if (L_Sim != NULL) {
		lua_State* L_Old = L_Draw;
		L_Draw = L_Sim;
		SetRunning(L_Sim, true);
		LUA_CLOSE(L_Sim);
		//SetRunning(L_Sim, false); --nope, the state is deleted
		L_Sim = NULL;
		L_Draw = L_Old;
	}
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::KillActiveHandle(lua_State* L)
{
	CLuaHandle* ah = GetHandle(L);
	if (ah != NULL) {
		const int args = lua_gettop(L);
		if ((args >= 1) && lua_isstring(L, 1)) {
			ah->killMsg = lua_tostring(L, 1);
		}
		// get rid of us next GameFrame call
		ah->killMe = true;
	}
	return 0;
}


/******************************************************************************/

bool CLuaHandle::AddEntriesToTable(lua_State* L, const char* name,
                                   bool (*entriesFunc)(lua_State*))
{
	const int top = lua_gettop(L);
	lua_pushstring(L, name);
	lua_rawget(L, -2);
	if (lua_istable(L, -1)) {
		bool success = entriesFunc(L);
		lua_settop(L, top);
		return success;
	}

	// make a new table
	lua_pop(L, 1);
	lua_pushstring(L, name);
	lua_newtable(L);
	if (!entriesFunc(L)) {
		lua_settop(L, top);
		return false;
	}
	lua_rawset(L, -3);

	lua_settop(L, top);
	return true;
}


bool CLuaHandle::LoadCode(lua_State *L, const string& code, const string& debug)
{
	GML_DRCMUTEX_LOCK(lua); // LoadCode

	lua_settop(L, 0);

	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	int loadError = 0;
	int callError = 0;
	bool ret = true;

	if ((loadError = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str())) == 0) {
		SetRunning(L, true);

		if ((callError = lua_pcall(L, 0, 0, 0)) != 0) {
			LOG_L(L_ERROR, "Lua LoadCode pcall error = %i, %s, %s", loadError, debug.c_str(), lua_tostring(L, -1));
			lua_pop(L, 1);
			ret = false;
		}

		SetRunning(L, false);
	} else {
		LOG_L(L_ERROR, "Lua LoadCode loadbuffer error = %i, %s, %s", callError, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		ret = false;
	}

	return ret;
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CheckStack()
{
	// FIXME WTF this has NOTHING to do with the stack! esp. it should be called AFTER the stack was checked
	ExecuteCallsFromSynced(false);
	ExecuteUnitEventBatch();
	ExecuteFeatEventBatch();
	ExecuteObjEventBatch();
	ExecuteProjEventBatch();
	ExecuteFrameEventBatch();
	ExecuteLogEventBatch();

	SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // CheckStack - avoid bogus errors due to concurrency

	const int top = lua_gettop(L);
	if (top != 0) {
		LOG_L(L_WARNING, "%s stack check: top = %i", GetName().c_str(), top);
		lua_settop(L, 0);
	}
}


void CLuaHandle::RecvFromSynced(lua_State *srcState, int args) {
	SELECT_UNSYNCED_LUA_STATE();

#if ((LUA_MT_OPT & LUA_STATE) && (LUA_MT_OPT & LUA_MUTEX))
	if (!SingleState() && srcState != L) { // Sim thread sends to unsynced --> delay it
		DelayRecvFromSynced(srcState, args);
		return;
	}
	// Draw thread, delayed already, execute it
#endif

	static const LuaHashString cmdStr("RecvFromSynced");
	//LUA_CALL_IN_CHECK(L); -- not valid here

	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined
	lua_insert(L, 1); // place the function

	// call the routine
	lua_State* L_Prev = ForceUnsyncedState();
	RunCallIn(cmdStr, args, 0);
	RestoreState(L_Prev);
}


void CLuaHandle::DelayRecvFromSynced(lua_State* srcState, int args) {
	DelayDataDump ddmp;

	if(CopyExportTable()) {
		HSTR_PUSH(srcState, "EXPORT");
		lua_rawget(srcState, LUA_GLOBALSINDEX);

		if (lua_istable(srcState, -1))
			LuaUtils::Backup(ddmp.dump, srcState, 1);
		lua_pop(srcState, 1);
	}

	LuaUtils::ShallowBackup(ddmp.data, srcState, args);

	GML_STDMUTEX_LOCK(scall);

	delayedCallsFromSynced.push_back(DelayDataDump());

	DelayDataDump& ddb = delayedCallsFromSynced.back();
	ddb.data.swap(ddmp.data);
	ddb.dump.swap(ddmp.dump);
	ddb.xcall = false;
}


int CLuaHandle::SendToUnsynced(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args <= 0) {
		luaL_error(L, "Incorrect arguments to SendToUnsynced()");
	}

	static const int supportedTypes =
		  (1 << LUA_TNIL)
		| (1 << LUA_TBOOLEAN)
		| (1 << LUA_TNUMBER)
		| (1 << LUA_TSTRING)
	;

	for (int i = 1; i <= args; i++) {
		const int t = (1 << lua_type(L, i));
		if (!(t & supportedTypes)) {
			luaL_error(L, "Incorrect data type for SendToUnsynced(), arg %d", i);
		}
	}
	CLuaHandle* lh = GetHandle(L);
	lh->RecvFromSynced(L, args);

	return 0;
}


bool CLuaHandle::ExecuteCallsFromSynced(bool forced) {
#if (LUA_MT_OPT & LUA_MUTEX)
	if ((SingleState() && (this != luaUI)) || (forced && !PurgeCallsFromSyncedBatch()))
#endif
		return false;

	GML_THRMUTEX_LOCK(obj, GML_DRAW); // ExecuteCallsFromSynced

	std::vector<DelayDataDump> drfs;
	{
		GML_STDMUTEX_LOCK(scall); // ExecuteCallsFromSynced

		if(delayedCallsFromSynced.empty())
			return false;

		delayedCallsFromSynced.swap(drfs);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteCallsFromSynced
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteCallsFromSynced
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteCallsFromSynced

	SELECT_UNSYNCED_LUA_STATE(); // ExecuteCallsFromSynced
	GML_DRCMUTEX_LOCK(lua);

	for (int i = 0; i < drfs.size(); ++i) {
		DelayDataDump &ddp = drfs[i];

#if (LUA_MT_OPT & LUA_STATE)
		if (!ddp.xcall) {
			if (CopyExportTable() && ddp.dump.size() > 0) {
				HSTR_PUSH(L, "UNSYNCED");
				lua_rawget(L, LUA_REGISTRYINDEX);

				HSTR_PUSH(L, "SYNCED");
				lua_rawget(L, -2);
				if (lua_getmetatable(L, -1)) {
					HSTR_PUSH(L, "realTable");
					lua_rawget(L, -2);
					if (lua_istable(L, -1)) {
						HSTR_PUSH(L, "EXPORT");
						LuaUtils::Restore(ddp.dump, L);
						lua_rawset(L, -3);
					}
					lua_pop(L, 2);
				}
				lua_pop(L, 2);
			}

			int ddsize = ddp.data.size();
			if (ddsize > 0) {
				LuaUtils::ShallowRestore(ddp.data, L);
				lua_checkstack(L, 2);
				RecvFromSynced(L, ddsize);
			}
		}
		else
#endif // (LUA_MT_OPT & LUA_STATE)
		{
			if (ddp.data.size() == 1) {
				LuaUtils::ShallowDataDump sdd = ddp.data[0];
				if (sdd.type == LUA_TSTRING) {
					const LuaHashString funcHash(*sdd.data.str);
					delete sdd.data.str;
					if (funcHash.GetGlobalFunc(L)) {
						const int top = lua_gettop(L) - 1;

						LuaUtils::Restore(ddp.dump, L);

						lua_State* L_Prev = ForceUnsyncedState();
						RunCallIn(funcHash, ddp.dump.size(), LUA_MULTRET);
						RestoreState(L_Prev);

						lua_settop(L, top);
					}
				}
			}
		}
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::SetupTraceback(lua_State *L)
{
	return LuaUtils::PushDebugTraceback(L);
}


int CLuaHandle::RunCallInTraceback(const LuaHashString* hs, int inArgs, int outArgs, int errfuncIndex, std::string& traceback)
{
	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	SELECT_LUA_STATE();
	SetRunning(L, true);
	// disable GC outside of this scope to prevent sync errors and similar
	lua_gc(L, LUA_GCRESTART, 0);
	MatrixStateData prevMSD = L->lcd->PushMatrixState();
	LuaOpenGL::InitMatrixState(L, hs);
	const int error = lua_pcall(L, inArgs, outArgs, errfuncIndex);
	LuaOpenGL::CheckMatrixState(L, hs, error);
	L->lcd->PopMatrixState(prevMSD);
	lua_gc(L, LUA_GCSTOP, 0);

	SetRunning(L, false);

	if (error == 0) {
		// pop the error handler
		if (errfuncIndex != 0) {
			lua_remove(L, errfuncIndex);
		}
	} else {
		traceback = lua_tostring(L, -1);
		lua_pop(L, 1);
		if (errfuncIndex != 0)
			lua_remove(L, errfuncIndex);
		// log only errors that lead to a crash
		callinErrors += (error == 2);
	}

	return error;
}


bool CLuaHandle::RunCallInTraceback(const LuaHashString& hs, int inArgs, int outArgs, int errfuncIndex)
{
	std::string traceback;
	const int error = RunCallInTraceback(&hs, inArgs, outArgs, errfuncIndex, traceback);

	if (error != 0) {
		LOG_L(L_ERROR, "%s::RunCallIn: error = %i, %s, %s", GetName().c_str(),
				error, hs.GetString().c_str(), traceback.c_str());
		return false;
	}
	return true;
}


int CLuaHandle::RunCallIn(int inArgs, int outArgs, std::string& errormessage)
{
	return RunCallInTraceback(NULL, inArgs, outArgs, 0, errormessage);
}

/******************************************************************************/

void CLuaHandle::Shutdown()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("Shutdown");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
}

void CLuaHandle::Load(IArchive* archive)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("Load");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	// Load gets ZipFileReader userdatum as single argument
	LuaZipFileReader::PushNew(L, "", archive);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}

void CLuaHandle::GamePreload()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("GamePreload");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
}

void CLuaHandle::GameStart()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("GameStart");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
}

void CLuaHandle::GameOver(const std::vector<unsigned char>& winningAllyTeams)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("GameOver");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_createtable(L, winningAllyTeams.size(), 0);
	for (unsigned int i = 0; i < winningAllyTeams.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, winningAllyTeams[i]);
		lua_rawset(L, -3);
	}

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::GamePaused(int playerID, bool paused)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("GamePaused");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);
	lua_pushboolean(L, paused);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
}


void CLuaHandle::GameFrame(int frameNum)
{
	if (killMe) {
		string msg = GetName();
		if (!killMsg.empty()) {
			msg += ": " + killMsg;
		}
		LOG("[%s] disabled %s", __FUNCTION__, msg.c_str());
		delete this;
		return;
	}

	LUA_FRAME_BATCH_PUSH(frameNum);
	LUA_CALL_IN_CHECK(L);
	if (CopyExportTable())
		DelayRecvFromSynced(L, 0); // Copy _G.EXPORT --> SYNCED.EXPORT once a game frame
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("GameFrame");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::GameID(const unsigned char* gameID, unsigned int numBytes)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	const LuaHashString cmdStr("GameID");
	const int errFunc = SetupTraceback(L);

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errFunc != 0) {
			lua_pop(L, 1);
		}
		return;
	}

	lua_pushlstring(L, reinterpret_cast<const char*>(gameID), numBytes);

	RunCallInTraceback(cmdStr, 1, 0, errFunc);
}


void CLuaHandle::TeamDied(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("TeamDied");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::TeamChanged(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("TeamChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::PlayerChanged(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("PlayerChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::PlayerAdded(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("PlayerAdded");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
}


void CLuaHandle::PlayerRemoved(int playerID, int reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("PlayerRemoved");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);
	lua_pushnumber(L, reason);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
}


/******************************************************************************/

inline void CLuaHandle::UnitCallIn(const LuaHashString& hs, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	int errfunc = SetupTraceback(L);

	if (!hs.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(hs, 3, 0, errfunc);
}


void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_CREATED, unit, builder);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitCreated");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (builder != NULL) {
		lua_pushnumber(L, builder->id);
	}

	int args = (builder != NULL) ? 4 : 3;
	// call the routine
	RunCallInTraceback(cmdStr, args, 0, errfunc);
}


void CLuaHandle::UnitFinished(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_FINISHED, unit);
	static const LuaHashString cmdStr("UnitFinished");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_FROM_FACTORY, unit, factory, userOrders);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitFromFactory");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallInTraceback(cmdStr, 6, 0, errfunc);
}


void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_DESTROYED, unit, attacker);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	int argCount = 3;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (GetHandleFullRead(L) && (attacker != NULL)) {
		lua_pushnumber(L, attacker->id);
		lua_pushnumber(L, attacker->unitDef->id);
		lua_pushnumber(L, attacker->team);
		argCount += 3;
	}

	// call the routine
	RunCallInTraceback(cmdStr, argCount, 0, errfunc);
}


void CLuaHandle::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_TAKEN, unit, oldTeam, newTeam);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitTaken");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);

	// call the routine
	RunCallInTraceback(cmdStr, 4, 0, errfunc);
}


void CLuaHandle::UnitGiven(const CUnit* unit, int oldTeam, int newTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_GIVEN, unit, oldTeam, newTeam);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitGiven");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, newTeam);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallInTraceback(cmdStr, 4, 0, errfunc);
}


void CLuaHandle::UnitIdle(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_IDLE, unit);
	static const LuaHashString cmdStr("UnitIdle");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitCommand(const CUnit* unit, const Command& command)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_COMMAND, unit, command);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitCommand");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	lua_pushnumber(L, command.GetID());
	lua_pushnumber(L, command.options);

	const vector<float> &params = command.params;
	lua_createtable(L, params.size(), 0);
	for (unsigned int i = 0; i < params.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, params[i]);
		lua_rawset(L, -3);
	}

	// call the routine
	RunCallInTraceback(cmdStr, 6, 0, errfunc);
}


void CLuaHandle::UnitCmdDone(const CUnit* unit, int cmdID, int cmdTag)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_CMD_DONE, unit, cmdID, cmdTag);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitCmdDone");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, cmdID);
	lua_pushnumber(L, cmdTag);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
}


void CLuaHandle::UnitDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_DAMAGED, unit, attacker, damage, weaponDefID, paralyzer);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitDamaged");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	int argCount = 5;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, weaponDefID);
		argCount += 1;

		// TODO: add to batch
		// lua_pushnumber(L, projectileID);
		// argCount += 1;

		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			argCount += 3;
		}
	}

	// call the routine
	RunCallInTraceback(cmdStr, argCount, 0, errfunc);
}


void CLuaHandle::UnitExperience(const CUnit* unit, float oldExperience)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_EXPERIENCE, unit, oldExperience);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitExperience");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, oldExperience);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
}


/******************************************************************************/

void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_SEISMIC_PING, unit, allyTeam, pos, strength);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);
	int readAllyTeam = GetHandleReadAllyTeam(L);
	if ((readAllyTeam >= 0) && (unit->losStatus[readAllyTeam] & LOS_INLOS)) {
		return; // don't need to see this ping
	}

	static const LuaHashString cmdStr("UnitSeismicPing");
	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, strength);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->id);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(cmdStr, GetHandleFullRead(L) ? 7 : 4, 0);
}


/******************************************************************************/

void CLuaHandle::LosCallIn(const LuaHashString& hs,
                           const CUnit* unit, int allyTeam)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	if (!hs.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->team);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(hs, GetHandleFullRead(L) ? 4 : 2, 0);
}


void CLuaHandle::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_ENTERED_RADAR, unit, allyTeam);
	static const LuaHashString hs("UnitEnteredRadar");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_ENTERED_LOS, unit, allyTeam);
	static const LuaHashString hs("UnitEnteredLos");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftRadar(const CUnit* unit, int allyTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_LEFT_RADAR, unit, allyTeam);
	static const LuaHashString hs("UnitLeftRadar");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_LEFT_LOS, unit, allyTeam);
	static const LuaHashString hs("UnitLeftLos");
	LosCallIn(hs, unit, allyTeam);
}


/******************************************************************************/

void CLuaHandle::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_LOADED, unit, transport);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitLoaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
}


void CLuaHandle::UnitUnloaded(const CUnit* unit, const CUnit* transport)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_UNLOADED, unit, transport);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("UnitUnloaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
}


/******************************************************************************/

void CLuaHandle::UnitEnteredWater(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_ENTERED_WATER, unit);
	static const LuaHashString cmdStr("UnitEnteredWater");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitEnteredAir(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_ENTERED_AIR, unit);
	static const LuaHashString cmdStr("UnitEnteredAir");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftWater(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_LEFT_WATER, unit);
	static const LuaHashString cmdStr("UnitLeftWater");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftAir(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_LEFT_AIR, unit);
	static const LuaHashString cmdStr("UnitLeftAir");
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::UnitCloaked(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_CLOAKED, unit);
	static const LuaHashString cmdStr("UnitCloaked");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitDecloaked(const CUnit* unit)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_DECLOAKED, unit);
	static const LuaHashString cmdStr("UnitDecloaked");
	UnitCallIn(cmdStr, unit);
}



void CLuaHandle::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (!watchUnitDefs[collider->unitDef->id]) return;
	if (!watchUnitDefs[collidee->unitDef->id]) return;

	LUA_UNIT_BATCH_PUSH(,UNIT_UNIT_COLLISION, collider, collidee);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	static const LuaHashString cmdStr("UnitUnitCollision");
	const int errFunc = SetupTraceback(L);

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errFunc != 0)
			lua_pop(L, 1);
		return;
	}

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);
	lua_pushboolean(L, collidee->crushKilled);

	RunCallInTraceback(cmdStr, 3, 0, errFunc);
}

void CLuaHandle::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (watchFeatureDefs.empty()) return;
	if (!watchUnitDefs[collider->unitDef->id]) return;
	if (!watchFeatureDefs[collidee->def->id]) return;

	LUA_OBJ_BATCH_PUSH(UNIT_FEAT_COLLISION, collider, collidee);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	static const LuaHashString cmdStr("UnitFeatureCollision");
	const int errFunc = SetupTraceback(L);

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errFunc != 0)
			lua_pop(L, 1);
		return;
	}

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);
	lua_pushboolean(L, collidee->crushKilled);

	RunCallInTraceback(cmdStr, 3, 0, errFunc);
}

void CLuaHandle::UnitMoveFailed(const CUnit* unit)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (!watchUnitDefs[unit->unitDef->id]) return;

	LUA_UNIT_BATCH_PUSH(,UNIT_MOVE_FAILED, unit);
	static const LuaHashString cmdStr("UnitMoveFailed");
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	LUA_FEAT_BATCH_PUSH(FEAT_CREATED, feature);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	const int errfunc = SetupTraceback(L);
	static const LuaHashString cmdStr("FeatureCreated");

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
}


void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	LUA_FEAT_BATCH_PUSH(FEAT_DESTROYED, feature);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback(L);

	static const LuaHashString cmdStr("FeatureDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) // remove error handler
			lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
}


/******************************************************************************/

void CLuaHandle::ProjectileCreated(const CProjectile* p)
{
	// if empty, we are not a LuaHandleSynced
	if (watchWeaponDefs.empty()) return;

	if (!p->synced) return;
	if (!p->weapon && !p->piece) return;

	const CUnit* owner = p->owner();
	const CWeaponProjectile* wp = p->weapon? static_cast<const CWeaponProjectile*>(p): NULL;
	const WeaponDef* wd = p->weapon? wp->GetWeaponDef(): NULL;

	// if this weapon-type is not being watched, bail
	if (p->weapon && (wd == NULL || !watchWeaponDefs[wd->id]))
		return;

	LUA_PROJ_BATCH_PUSH(PROJ_CREATED, p);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	static const LuaHashString cmdStr("ProjectileCreated");

	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, p->id);
	lua_pushnumber(L, ((owner != NULL)? owner->id: -1));
	lua_pushnumber(L, ((wd != NULL)? wd->id: -1));

	// call the routine
	RunCallIn(cmdStr, 3, 0);
}


void CLuaHandle::ProjectileDestroyed(const CProjectile* p)
{
	// if empty, we are not a LuaHandleSynced
	if (watchWeaponDefs.empty()) return;

	if (!p->synced) return;
	if (!p->weapon && !p->piece) return;
	if (p->weapon) {
		const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);
		const WeaponDef* wd = wp->GetWeaponDef();

		// if this weapon-type is not being watched, bail
		if (wd == NULL || !watchWeaponDefs[wd->id]) return;
	}

	LUA_PROJ_BATCH_PUSH(PROJ_DESTROYED, p);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	static const LuaHashString cmdStr("ProjectileDestroyed");

	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, p->id);

	// call the routine
	RunCallIn(cmdStr, 1, 0);
}

/******************************************************************************/

bool CLuaHandle::Explosion(int weaponDefID, int projectileID, const float3& pos, const CUnit* owner)
{
	// piece-projectile collision (*ALL* other
	// explosion events pass valid weaponDefIDs)
	if (weaponDefID < 0) return false;

	// if empty, we are not a LuaHandleSynced
	if (watchWeaponDefs.empty()) return false;
	if (!watchWeaponDefs[weaponDefID]) return false;

	LUA_UNIT_BATCH_PUSH(false, UNIT_EXPLOSION, weaponDefID, pos, owner);
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("Explosion");
	if (!cmdStr.GetGlobalFunc(L))
		return false; // the call is not defined

	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	if (owner != NULL) {
		lua_pushnumber(L, owner->id);
	}

	// call the routine
	if (!RunCallIn(cmdStr, (owner == NULL) ? 4 : 5, 1))
		return false;

	// get the results
	if (!lua_isboolean(L, -1)) {
		LOG_L(L_WARNING, "%s() bad return value", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


void CLuaHandle::StockpileChanged(const CUnit* unit,
                                  const CWeapon* weapon, int oldCount)
{
	LUA_UNIT_BATCH_PUSH(,UNIT_STOCKPILE_CHANGED, unit, weapon, oldCount);
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr("StockpileChanged");
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, weapon->weaponNum);
	lua_pushnumber(L, oldCount);
	lua_pushnumber(L, weapon->numStockpiled);

	// call the routine
	RunCallIn(cmdStr, 6, 0);
}


void CLuaHandle::ExecuteUnitEventBatch() {
	if(!UseEventBatch()) return;

	GML_THRMUTEX_LOCK(obj, GML_DRAW); // ExecuteUnitEventBatch

	std::vector<LuaUnitEvent> lueb;
	{
		GML_STDMUTEX_LOCK(ulbatch);

		if(luaUnitEventBatch.empty())
			return;

		luaUnitEventBatch.swap(lueb);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteUnitEventBatch
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteUnitEventBatch
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteUnitEventBatch

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteUnitEventBatch

	if (Threading::IsSimThread())
		Threading::SetBatchThread(false);
	for (std::vector<LuaUnitEvent>::iterator i = lueb.begin(); i != lueb.end(); ++i) {
		LuaUnitEvent &e = *i;
		switch (e.id) {
			case UNIT_FINISHED:
				UnitFinished(e.unit1);
				break;
			case UNIT_CREATED:
				UnitCreated(e.unit1, e.unit2);
				break;
			case UNIT_FROM_FACTORY:
				UnitFromFactory(e.unit1, e.unit2, e.bool1);
				break;
			case UNIT_DESTROYED:
				UnitDestroyed(e.unit1, e.unit2);
				break;
			case UNIT_TAKEN:
				UnitTaken(e.unit1, e.int1, e.int2);
				break;
			case UNIT_GIVEN:
				UnitGiven(e.unit1, e.int1, e.int2);
				break;
			case UNIT_IDLE:
				UnitIdle(e.unit1);
				break;
			case UNIT_COMMAND:
				UnitCommand(e.unit1, *e.cmd1);
				break;
			case UNIT_CMD_DONE:
				UnitCmdDone(e.unit1, e.int1, e.int2);
				break;
			case UNIT_DAMAGED:
				UnitDamaged(e.unit1, e.unit2, e.float1, e.int1, -1, e.bool1);
				break;
			case UNIT_EXPERIENCE:
				UnitExperience(e.unit1, e.float1);
				break;
			case UNIT_SEISMIC_PING:
				UnitSeismicPing(e.unit1, e.int1, e.cmd1->GetPos(0), e.float1);
				break;
			case UNIT_ENTERED_RADAR:
				UnitEnteredRadar(e.unit1, e.int1);
				break;
			case UNIT_ENTERED_LOS:
				UnitEnteredLos(e.unit1, e.int1);
				break;
			case UNIT_LEFT_RADAR:
				UnitLeftRadar(e.unit1, e.int1);
				break;
			case UNIT_LEFT_LOS:
				UnitLeftLos(e.unit1, e.int1);
				break;
			case UNIT_LOADED:
				UnitLoaded(e.unit1, e.unit2);
				break;
			case UNIT_UNLOADED:
				UnitUnloaded(e.unit1, e.unit2);
				break;
			case UNIT_ENTERED_WATER:
				UnitEnteredWater(e.unit1);
				break;
			case UNIT_ENTERED_AIR:
				UnitEnteredAir(e.unit1);
				break;
			case UNIT_LEFT_WATER:
				UnitLeftWater(e.unit1);
				break;
			case UNIT_LEFT_AIR:
				UnitLeftAir(e.unit1);
				break;
			case UNIT_CLOAKED:
				UnitCloaked(e.unit1);
				break;
			case UNIT_DECLOAKED:
				UnitDecloaked(e.unit1);
				break;
			case UNIT_MOVE_FAILED:
				UnitMoveFailed(e.unit1);
				break;
			case UNIT_EXPLOSION:
				Explosion(e.int1, -1, e.cmd1->GetPos(0), e.unit1);
				break;
			case UNIT_UNIT_COLLISION:
				UnitUnitCollision(e.unit1, e.unit2);
				break;
			case UNIT_STOCKPILE_CHANGED:
				StockpileChanged(e.unit1, e.weapon2, e.int1);
				break;
			default:
				LOG_L(L_ERROR, "%s: Invalid Event %d", __FUNCTION__, e.id);
				break;
		}
	}
	if (Threading::IsSimThread())
		Threading::SetBatchThread(true);
}


void CLuaHandle::ExecuteFeatEventBatch() {
	if(!UseEventBatch()) return;

	GML_THRMUTEX_LOCK(obj, GML_DRAW); // ExecuteFeatEventBatch

	std::vector<LuaFeatEvent> lfeb;
	{
		GML_STDMUTEX_LOCK(flbatch);

		if(luaFeatEventBatch.empty())
			return;

		luaFeatEventBatch.swap(lfeb);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteFeatEventBatch
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteFeatEventBatch
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteFeatEventBatch

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteFeatEventBatch

	if(Threading::IsSimThread())
		Threading::SetBatchThread(false);
	for(std::vector<LuaFeatEvent>::iterator i = lfeb.begin(); i != lfeb.end(); ++i) {
		LuaFeatEvent &e = *i;
		switch(e.id) {
			case FEAT_CREATED:
				FeatureCreated(e.feat1);
				break;
			case FEAT_DESTROYED:
				FeatureDestroyed(e.feat1);
				break;
			default:
				LOG_L(L_ERROR, "%s: Invalid Event %d", __FUNCTION__, e.id);
				break;
		}
	}
	if(Threading::IsSimThread())
		Threading::SetBatchThread(true);
}


void CLuaHandle::ExecuteObjEventBatch() {
	if(!UseEventBatch()) return;

	GML_THRMUTEX_LOCK(obj, GML_DRAW); // ExecuteObjEventBatch

	std::vector<LuaObjEvent> loeb;
	{
		GML_STDMUTEX_LOCK(olbatch);

		if(luaObjEventBatch.empty())
			return;

		luaObjEventBatch.swap(loeb);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteObjEventBatch
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteObjEventBatch
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteObjEventBatch

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteObjEventBatch

	if(Threading::IsSimThread())
		Threading::SetBatchThread(false);
	for(std::vector<LuaObjEvent>::iterator i = loeb.begin(); i != loeb.end(); ++i) {
		LuaObjEvent &e = *i;
		switch(e.id) {
			case UNIT_FEAT_COLLISION:
				UnitFeatureCollision(e.unit, e.feat);
				break;
			default:
				LOG_L(L_ERROR, "%s: Invalid Event %d", __FUNCTION__, e.id);
				break;
		}
	}
	if(Threading::IsSimThread())
		Threading::SetBatchThread(true);
}


void CLuaHandle::ExecuteProjEventBatch() {
	if(!UseEventBatch()) return;

//	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteProjEventBatch
//	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteProjEventBatch
	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteProjEventBatch

	std::vector<LuaProjEvent> lpeb;
	{
		GML_STDMUTEX_LOCK(plbatch);

		if(luaProjEventBatch.empty())
			return;

		luaProjEventBatch.swap(lpeb);
	}

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteProjEventBatch

	if(Threading::IsSimThread())
		Threading::SetBatchThread(false);
	for(std::vector<LuaProjEvent>::iterator i = lpeb.begin(); i != lpeb.end(); ++i) {
		LuaProjEvent &e = *i;
		switch(e.id) {
			case PROJ_CREATED:
				ProjectileCreated(e.proj1);
				break;
			case PROJ_DESTROYED:
				ProjectileDestroyed(e.proj1);
				break;
			default:
				LOG_L(L_ERROR, "%s: Invalid Event %d", __FUNCTION__, e.id);
				break;
		}
	}
	if(Threading::IsSimThread())
		Threading::SetBatchThread(true);
}


void CLuaHandle::ExecuteFrameEventBatch() {
	if(!UseEventBatch()) return;

	std::vector<int> lgeb;
	{
		GML_STDMUTEX_LOCK(glbatch);

		if(luaFrameEventBatch.empty())
			return;

		luaFrameEventBatch.swap(lgeb);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteFrameEventBatch
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteFrameEventBatch
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteFrameEventBatch

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteFrameEventBatch

	if(Threading::IsSimThread())
		Threading::SetBatchThread(false);
	for(std::vector<int>::iterator i = lgeb.begin(); i != lgeb.end(); ++i) {
		GameFrame(*i);
	}
	if(Threading::IsSimThread())
		Threading::SetBatchThread(true);
}


void CLuaHandle::ExecuteLogEventBatch() {
	if (!UseEventBatch()) return;

	std::vector<LuaLogEvent> lmeb;
	{
		GML_STDMUTEX_LOCK(mlbatch);

		if (luaLogEventBatch.empty()) {
			return;
		}

		luaLogEventBatch.swap(lmeb);
	}

	GML_THRMUTEX_LOCK(unit, GML_DRAW); // ExecuteLogEventBatch
	GML_THRMUTEX_LOCK(feat, GML_DRAW); // ExecuteLogEventBatch
//	GML_THRMUTEX_LOCK(proj, GML_DRAW); // ExecuteLogEventBatch

	GML_SELECT_LUA_STATE();
	GML_DRCMUTEX_LOCK(lua); // ExecuteLogEventBatch

	if (Threading::IsSimThread()) {
		Threading::SetBatchThread(false);
	}
	for (std::vector<LuaLogEvent>::iterator i = lmeb.begin(); i != lmeb.end(); ++i) {
		LuaLogEvent& e = *i;
		switch (e.id) {
			case ADD_CONSOLE_LINE:
				AddConsoleLine(e.msg, e.section, e.level);
				break;
			default:
				LOG_L(L_ERROR, "%s: Invalid Event %d", __FUNCTION__, e.id);
				break;
		}
	}
	if (Threading::IsSimThread()) {
		Threading::SetBatchThread(true);
	}
}


bool CLuaHandle::RecvLuaMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr("RecvLuaMsg");
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, msg); // allow embedded 0's
	lua_pushnumber(L, playerID);

	// call the routine
	if (!RunCallIn(cmdStr, 2, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


/******************************************************************************/

void CLuaHandle::HandleLuaMsg(int playerID, int script, int mode, const std::vector<boost::uint8_t>& data)
{
	std::string msg;
	msg.resize(data.size());
	std::copy(data.begin(), data.end(), msg.begin());
	if (script == LUA_HANDLE_ORDER_UI) {
		if (luaUI) {
			bool sendMsg = false;
			if (mode == 0) {
				sendMsg = true;
			}
			else if (mode == 's') {
				sendMsg = gu->spectating;
			}
			else if (mode == 'a') {
				const CPlayer* player = playerHandler->Player(playerID);
				if (player == NULL) {
					return;
				}
				if (gu->spectatingFullView) {
					sendMsg = true;
				}
				else if (player->spectator) {
					sendMsg = gu->spectating;
				} else {
					const int msgAllyTeam = teamHandler->AllyTeam(player->team);
					sendMsg = teamHandler->Ally(msgAllyTeam, gu->myAllyTeam);
				}
			}
			if (sendMsg) {
				luaUI->RecvLuaMsg(msg, playerID);
			}
		}
	}
	else if (script == LUA_HANDLE_ORDER_GAIA) {
		if (luaGaia) {
			luaGaia->RecvLuaMsg(msg, playerID);
		}
	}
	else if (script == LUA_HANDLE_ORDER_RULES) {
		if (luaRules) {
			luaRules->RecvLuaMsg(msg, playerID);
		}
	}
}


/******************************************************************************/

inline bool CLuaHandle::PushUnsyncedCallIn(lua_State *L, const LuaHashString& hs)
{
	// LuaUI keeps these call-ins in the Global table,
	// the synced handles keep them in the Registry table
	if (GetUserMode()) {
		return hs.GetGlobalFunc(L);
	} else {
		return hs.GetRegistryFunc(L);
	}
}


void CLuaHandle::Save(zipFile archive)
{
	// LuaUI does not get this call-in
	if (GetUserMode()) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("Save");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	// Save gets ZipFileWriter userdatum as single argument
	LuaZipFileWriter::PushNew(L, "", archive);

	// call the routine
	RunCallInUnsynced(cmdStr, 1, 0);
}


void CLuaHandle::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("UnsyncedHeightMapUpdate");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	lua_pushnumber(L, rect.x1);
	lua_pushnumber(L, rect.z1);
	lua_pushnumber(L, rect.x2);
	lua_pushnumber(L, rect.z2);

	// call the routine
	RunCallInUnsynced(cmdStr, 4, 0);
}


void CLuaHandle::Update()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("Update");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);
}


void CLuaHandle::ViewResize()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("ViewResize");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY; //! origin BOTTOMLEFT

	lua_newtable(L);
	LuaPushNamedNumber(L, "screenSizeX", globalRendering->screenSizeX);
	LuaPushNamedNumber(L, "screenSizeY", globalRendering->screenSizeY);
	LuaPushNamedNumber(L, "screenPosX",  0.0f);
	LuaPushNamedNumber(L, "screenPosY",  0.0f);
	LuaPushNamedNumber(L, "windowSizeX", globalRendering->winSizeX);
	LuaPushNamedNumber(L, "windowSizeY", globalRendering->winSizeY);
	LuaPushNamedNumber(L, "windowPosX",  globalRendering->winPosX);
	LuaPushNamedNumber(L, "windowPosY",  winPosY_bl);
	LuaPushNamedNumber(L, "viewSizeX",   globalRendering->viewSizeX);
	LuaPushNamedNumber(L, "viewSizeY",   globalRendering->viewSizeY);
	LuaPushNamedNumber(L, "viewPosX",    globalRendering->viewPosX);
	LuaPushNamedNumber(L, "viewPosY",    globalRendering->viewPosY);

	// call the routine
	RunCallInUnsynced(cmdStr, 1, 0);
}


bool CLuaHandle::DefaultCommand(const CUnit* unit,
                                const CFeature* feature, int& cmd)
{
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DefaultCommand");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false;
	}

	int args = 0;
	if (unit) {
		HSTR_PUSH(L, "unit");
		lua_pushnumber(L, unit->id);
		args = 2;
	}
	else if (feature) {
		HSTR_PUSH(L, "feature");
		lua_pushnumber(L, feature->id);
		args = 2;
	}
/* FIXME
	else if (groundPos) {
		HSTR_PUSH(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		HSTR_PUSH(L, "selection");
		args = 1;
	}
*/

	// call the routine
	if (!RunCallInUnsynced(cmdStr, args, 1))
		return false;

	if (!lua_isnumber(L, 1)) {
		lua_pop(L, 1);
		return false;
	}

	cmd = lua_toint(L, -1);
	lua_pop(L, 1);
	return true;
}




void CLuaHandle::DrawGenesis()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawGenesis");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawWorld()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawWorld");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawWorldPreUnit()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawWorldPreUnit");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawWorldShadow()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawWorldShadow");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawWorldReflection()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawWorldReflection");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawWorldRefraction()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("DrawWorldRefraction");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawScreen()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DrawScreen");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawScreenEffects()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DrawScreenEffects");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawInMiniMap()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DrawInMiniMap");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	const int xSize = minimap->GetSizeX();
	const int ySize = minimap->GetSizeY();

	if ((xSize < 1) || (ySize < 1)) {
		lua_pop(L, 1);
		return;
	}

	lua_pushnumber(L, xSize);
	lua_pushnumber(L, ySize);

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


void CLuaHandle::GameProgress(int frameNum )
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("GameProgress");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return;
	}

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInUnsynced(cmdStr, 1, 0);
}


/******************************************************************************/
/******************************************************************************/


bool CLuaHandle::KeyPress(unsigned short key, bool isRepeat)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("KeyPress");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!keyInput->GetKeyState(SDLK_LALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!keyInput->GetKeyState(SDLK_LCTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!keyInput->GetKeyState(SDLK_LMETA));
	HSTR_PUSH_BOOL(L, "shift", !!keyInput->GetKeyState(SDLK_LSHIFT));

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));

	lua_pushnumber(L, keyInput->GetCurrentKeyUnicodeChar());

	// call the function
	if (!RunCallInUnsynced(cmdStr, 5, 1))
		return false;

	// const int args = lua_gettop(L); unused
	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::KeyRelease(unsigned short key)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("KeyRelease");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!keyInput->GetKeyState(SDLK_LALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!keyInput->GetKeyState(SDLK_LCTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!keyInput->GetKeyState(SDLK_LMETA));
	HSTR_PUSH_BOOL(L, "shift", !!keyInput->GetKeyState(SDLK_LSHIFT));

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));

	lua_pushnumber(L, keyInput->GetCurrentKeyUnicodeChar());

	// call the function
	if (!RunCallInUnsynced(cmdStr, 4, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MousePress(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("MousePress");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


int CLuaHandle::MouseRelease(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("MouseRelease");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1))
		return false;

	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return -1;
	}
	const int retval = lua_toint(L, -1) - 1;
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("MouseMove");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 5, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MouseWheel(bool up, float value)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("MouseWheel");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushboolean(L, up);
	lua_pushnumber(L, value);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::JoystickEvent(const std::string& event, int val1, int val2)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);
	const LuaHashString cmdStr(event);
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, val1);
	lua_pushnumber(L, val2);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::IsAbove(int x, int y)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("IsAbove");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1))
		return false;

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


string CLuaHandle::GetTooltip(int x, int y)
{
	if (!CheckModUICtrl()) {
		return "";
	}
	LUA_CALL_IN_CHECK(L, "");
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("GetTooltip");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return ""; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1))
		return "";

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return "";
	}
	const string retval = lua_tostring(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::ConfigCommand(const string& command)
{
	if (!CheckModUICtrl()) {
		return true; // FIXME ?
	}
	LUA_CALL_IN_CHECK(L, true);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("ConfigureLayout");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return true; // the call is not defined
	}

	lua_pushsstring(L, command);

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 1, 0))
		return false;

	return true;
}


bool CLuaHandle::CommandNotify(const Command& cmd)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("CommandNotify");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined
	}

	// push the command id
	lua_pushnumber(L, cmd.GetID());

	// push the params list
	lua_createtable(L, cmd.params.size(), 0);
	for (int p = 0; p < (int)cmd.params.size(); p++) {
		lua_pushnumber(L, cmd.params[p]);
		lua_rawseti(L, -2, p + 1);
	}

	// push the options table
	lua_createtable(L, 0, 6);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	HSTR_PUSH_BOOL(L, "alt",   !!(cmd.options & ALT_KEY));
	HSTR_PUSH_BOOL(L, "ctrl",  !!(cmd.options & CONTROL_KEY));
	HSTR_PUSH_BOOL(L, "shift", !!(cmd.options & SHIFT_KEY));
	HSTR_PUSH_BOOL(L, "right", !!(cmd.options & RIGHT_MOUSE_KEY));
	HSTR_PUSH_BOOL(L, "meta",  !!(cmd.options & META_KEY));

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1))
		return false;

	// get the results
	if (!lua_isboolean(L, -1)) {
		LOG_L(L_WARNING, "%s() bad return value", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::AddConsoleLine(const string& msg, const string& section, int level)
{
	if (!CheckModUICtrl()) {
		return true; // FIXME?
	}
	LUA_LOG_BATCH_PUSH(true, ADD_CONSOLE_LINE, msg, level, section);
	LUA_CALL_IN_CHECK(L, true);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("AddConsoleLine");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return true; // the call is not defined
	}

	lua_pushsstring(L, msg);
	// FIXME: makes no sense now, but *gets might expect this
	lua_pushnumber(L, 0); // priority XXX replace 0 with level?

	// call the function
	if (!RunCallIn(cmdStr, 2, 0))
		return false;

	return true;
}



bool CLuaHandle::GroupChanged(int groupID)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("GroupChanged");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, groupID);

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 1, 0))
		return false;

	return true;
}



string CLuaHandle::WorldTooltip(const CUnit* unit,
                                const CFeature* feature,
                                const float3* groundPos)
{
	if (!CheckModUICtrl()) {
		return "";
	}
	LUA_CALL_IN_CHECK(L, "");
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("WorldTooltip");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return ""; // the call is not defined
	}

	int args;
	if (unit) {
		HSTR_PUSH(L, "unit");
		lua_pushnumber(L, unit->id);
		args = 2;
	}
	else if (feature) {
		HSTR_PUSH(L, "feature");
		lua_pushnumber(L, feature->id);
		args = 2;
	}
	else if (groundPos) {
		HSTR_PUSH(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		HSTR_PUSH(L, "selection");
		args = 1;
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, args, 1))
		return "";

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return "";
	}
	const string retval = lua_tostring(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MapDrawCmd(int playerID, int type,
                            const float3* pos0,
                            const float3* pos1,
                            const string* label)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 9);
	static const LuaHashString cmdStr("MapDrawCmd");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false; // the call is not defined
	}

	int args;

	lua_pushnumber(L, playerID);

	if (type == MAPDRAW_POINT) {
		HSTR_PUSH(L, "point");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushsstring(L, *label);
		args = 6;
	}
	else if (type == MAPDRAW_LINE) {
		HSTR_PUSH(L, "line");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, pos1->x);
		lua_pushnumber(L, pos1->y);
		lua_pushnumber(L, pos1->z);
		args = 8;
	}
	else if (type == MAPDRAW_ERASE) {
		HSTR_PUSH(L, "erase");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, 100.0f);  // radius
		args = 6;
	}
	else {
		LOG_L(L_WARNING, "Unknown MapDrawCmd() type: %i", type);
		lua_pop(L, 2); // pop the function and playerID
		return false;
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, args, 1))
		return false;

	// take the event?
	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::GameSetup(const string& state, bool& ready,
                           const map<int, string>& playerStates)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("GameSetup");
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return false;
	}

	lua_pushsstring(L, state);

	lua_pushboolean(L, ready);

	lua_newtable(L);
	map<int, string>::const_iterator it;
	for (it = playerStates.begin(); it != playerStates.end(); ++it) {
		lua_pushnumber(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 3, 2))
		return false;

	if (lua_isboolean(L, -2)) {
		if (lua_toboolean(L, -2)) {
			if (lua_isboolean(L, -1)) {
				ready = lua_toboolean(L, -1);
			}
			lua_pop(L, 2);
			return true;
		}
	}
	lua_pop(L, 2);
	return false;
}



const char* CLuaHandle::RecvSkirmishAIMessage(int aiTeam, const char* inData, int inSize)
{
	LUA_CALL_IN_CHECK(L, NULL);
	lua_checkstack(L, 4);

	static const LuaHashString cmdStr("RecvSkirmishAIMessage");

	// <this> is either CLuaRules* or CLuaUI*,
	// but the AI call-in is always unsynced!
	if (!PushUnsyncedCallIn(L, cmdStr)) {
		return NULL;
	}

	lua_pushnumber(L, aiTeam);

	int argCount = 1;
	const char* outData = NULL;

	if (inData != NULL) {
		if (inSize < 0) {
			inSize = strlen(inData);
		}
		lua_pushlstring(L, inData, inSize);
		argCount = 2;
	}

	if (!RunCallIn(cmdStr, argCount, 1))
		return NULL;

	if (lua_isstring(L, -1))
		outData = lua_tolstring(L, -1, NULL);

	lua_pop(L, 1);
	return outData;
}


/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::AddBasicCalls(lua_State *L)
{
	HSTR_PUSH(L, "Script");
	lua_newtable(L); {
		HSTR_PUSH_CFUNC(L, "Kill",            KillActiveHandle);
		HSTR_PUSH_CFUNC(L, "GetName",         CallOutGetName);
		HSTR_PUSH_CFUNC(L, "GetSynced",       CallOutGetSynced);
		HSTR_PUSH_CFUNC(L, "GetFullCtrl",     CallOutGetFullCtrl);
		HSTR_PUSH_CFUNC(L, "GetFullRead",     CallOutGetFullRead);
		HSTR_PUSH_CFUNC(L, "GetCtrlTeam",     CallOutGetCtrlTeam);
		HSTR_PUSH_CFUNC(L, "GetReadTeam",     CallOutGetReadTeam);
		HSTR_PUSH_CFUNC(L, "GetReadAllyTeam", CallOutGetReadAllyTeam);
		HSTR_PUSH_CFUNC(L, "GetSelectTeam",   CallOutGetSelectTeam);
		HSTR_PUSH_CFUNC(L, "GetGlobal",       CallOutGetGlobal);
		HSTR_PUSH_CFUNC(L, "GetRegistry",     CallOutGetRegistry);
		HSTR_PUSH_CFUNC(L, "GetCallInList",   CallOutGetCallInList);
		// special team constants
		HSTR_PUSH_NUMBER(L, "NO_ACCESS_TEAM",  CEventClient::NoAccessTeam);
		HSTR_PUSH_NUMBER(L, "ALL_ACCESS_TEAM", CEventClient::AllAccessTeam);
//FIXME		LuaArrays::PushEntries(L);
	}
	lua_rawset(L, -3);

	// extra math utilities
	lua_getglobal(L, "math");
	LuaBitOps::PushEntries(L);
	lua_pop(L, 1);

	return true;
}


int CLuaHandle::CallOutGetName(lua_State* L)
{
	lua_pushsstring(L, GetHandle(L)->GetName());
	return 1;
}


int CLuaHandle::CallOutGetSynced(lua_State* L)
{
	lua_pushboolean(L, GetHandleSynced(L));
	return 1;
}


int CLuaHandle::CallOutGetFullCtrl(lua_State* L)
{
	lua_pushboolean(L, GetHandleFullCtrl(L));
	return 1;
}


int CLuaHandle::CallOutGetFullRead(lua_State* L)
{
	lua_pushboolean(L, GetHandleFullRead(L));
	return 1;
}


int CLuaHandle::CallOutGetCtrlTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleCtrlTeam(L));
	return 1;
}


int CLuaHandle::CallOutGetReadTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleReadTeam(L));
	return 1;
}


int CLuaHandle::CallOutGetReadAllyTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleReadAllyTeam(L));
	return 1;
}


int CLuaHandle::CallOutGetSelectTeam(lua_State* L)
{
	lua_pushnumber(L, GetHandleSelectTeam(L));
	return 1;
}


int CLuaHandle::CallOutGetGlobal(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_GLOBALSINDEX);
		return 1;
	}
	return 0;
}


int CLuaHandle::CallOutGetRegistry(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		return 1;
	}
	return 0;
}


int CLuaHandle::CallOutGetCallInList(lua_State* L)
{
	vector<string> list;
	eventHandler.GetEventList(list);
	lua_createtable(L, 0, list.size());
	for (unsigned int i = 0; i < list.size(); i++) {
		lua_pushsstring(L, list[i]);
		lua_newtable(L); {
			lua_pushliteral(L, "unsynced");
			lua_pushboolean(L, eventHandler.IsUnsynced(list[i]));
			lua_rawset(L, -3);
			lua_pushliteral(L, "controller");
			lua_pushboolean(L, eventHandler.IsController(list[i]));
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	return 1;
}


int CLuaHandle::CallOutSyncedUpdateCallIn(lua_State* L)
{
	if(!Threading::IsSimThread())
		return 0; // FIXME: If this can be called from a non-sim context, this code is insufficient
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to UpdateCallIn()");
	}
	const string name = lua_tostring(L, 1);
	CLuaHandle* lh = GetHandle(L);
	lh->SyncedUpdateCallIn(lh->GetActiveState(), name);
	return 0;
}


int CLuaHandle::CallOutUnsyncedUpdateCallIn(lua_State* L)
{
	if(Threading::IsSimThread())
		return 0; // FIXME: If this can be called from a sim context, this code is insufficient
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to UpdateCallIn()");
	}
	const string name = lua_tostring(L, 1);
	CLuaHandle* lh = GetHandle(L);
	lh->UnsyncedUpdateCallIn(lh->GetActiveState(), name);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

