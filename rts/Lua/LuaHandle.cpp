/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandle.h"

#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaUI.h"

#include "LuaCallInCheck.h"
#include "LuaEventBatch.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaBitOps.h"
#include "LuaMathExtra.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/Protocol/BaseNetProtocol.h"
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
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Rectangle.h"
#include "System/ScopedFPUSettings.h"
#include "System/Log/ILog.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/FileHandler.h"

#include "LuaInclude.h"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>


#include <string>

bool CLuaHandle::devMode = false;
bool CLuaHandle::modUICtrl = true;


/******************************************************************************/
/******************************************************************************/

void CLuaHandle::PushTracebackFuncToRegistry(lua_State* L)
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
	: CEventClient(_name, _order, false)
	, userMode   (_userMode)
	, killMe     (false)
	, callinErrors(0)
{
	D.owner = this;
	L = LUA_OPEN(&D);

	// needed for engine traceback
	PushTracebackFuncToRegistry(L);
}


CLuaHandle::~CLuaHandle()
{
	eventHandler.RemoveClient(this);

	// KillLua() must be called before dtor!!!
	assert(!IsValid());
}


void CLuaHandle::KillLua()
{
	if (IsValid()) {
		// 1. unlink from eventHandler, so no new events are getting triggered
		eventHandler.RemoveClient(this);
		//FIXME when multithreaded lua is enabled, wait for all running events to finish (possible via a mutex?)

		// 2. shutdown
		Shutdown();

		// 3. delete the lua_State
		SetHandleRunning(L, true);
		LUA_CLOSE(L);
		//SetHandleRunning(L, false); --nope, the state is deleted
		L = NULL;
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
	lua_settop(L, 0);

	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	int loadError = 0;
	int callError = 0;
	bool ret = true;

	if ((loadError = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str())) == 0) {
		SetHandleRunning(L, true);
		callError = lua_pcall(L, 0, 0, 0);
		SetHandleRunning(L, false);
		assert(!IsHandleRunning(L));

		if (callError != 0) {
			LOG_L(L_ERROR, "Lua LoadCode pcall error = %i, %s, %s", callError, debug.c_str(), lua_tostring(L, -1));
			lua_pop(L, 1);
			ret = false;
		}
	} else {
		LOG_L(L_ERROR, "Lua LoadCode loadbuffer error = %i, %s, %s", loadError, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		ret = false;
	}

	return ret;
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CheckStack()
{
	if (!IsValid())
		return;

	const int top = lua_gettop(L);
	if (top != 0) {
		LOG_L(L_WARNING, "%s stack check: top = %i", GetName().c_str(), top);
		lua_settop(L, 0);
	}
	assert(!IsRunning());
}


int CLuaHandle::XCall(lua_State* srcState, const string& funcName)
{
	const int top = lua_gettop(L);

	// push the function
	const LuaHashString funcHash(funcName);
	if (!funcHash.GetGlobalFunc(L)) {
		LOG_L(L_WARNING, "Tried to call non-linked Script.%s.%s()", GetName().c_str(), funcName.c_str());
		return 0;
	}

	int retCount;

	if (srcState == L) {
		// call the function
		if (!RunCallIn(L, funcHash, top, LUA_MULTRET)) {
			return 0;
		}
		retCount = lua_gettop(L);
	}
	else {
		const int srcCount = lua_gettop(srcState);

		LuaUtils::CopyData(L, srcState, srcCount);

		const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
		LuaOpenGL::SetDrawingEnabled(L, LuaOpenGL::IsDrawingEnabled(srcState));

		// call the function
		const bool failed = !RunCallIn(L, funcHash, srcCount, LUA_MULTRET);

		LuaOpenGL::SetDrawingEnabled(L, origDrawingState);

		if (failed)
			return 0;

		retCount = lua_gettop(L) - top;

		if (retCount > 0) {
			LuaUtils::CopyData(srcState, L, retCount);
		}
		lua_settop(L, top);
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::RunCallInTraceback(
	lua_State* L,
	const LuaHashString* hs,
	int inArgs,
	int outArgs,
	int errFuncIndex,
	std::string& tracebackMsg,
	bool popErrorFunc
) {
	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	struct ScopedLuaCall {
	public:
		ScopedLuaCall(
			CLuaHandle* handle,
			lua_State* state,
			const LuaHashString* func,
			int _nInArgs,
			int _nOutArgs,
			int _errFuncIdx,
			bool _popErrFunc
		)
			: luaState(state)
			, luaHandle(handle)

			, nInArgs(_nInArgs)
			, nOutArgs(_nOutArgs)
			, errFuncIdx(_errFuncIdx)
			, popErrFunc(_popErrFunc)
		{
			const bool oldRun = CLuaHandle::IsHandleRunning(luaState);
			handle->SetHandleRunning(state, true);

			GLMatrixStateTracker& matTracker = GetLuaContextData(state)->glMatrixTracker;
			MatrixStateData prevMatState = matTracker.PushMatrixState();
			LuaOpenGL::InitMatrixState(state, func);

			top = lua_gettop(state);
			// note1: disable GC outside of this scope to prevent sync errors and similar
			// note2: we collect garbage now in its own callin "CollectGarbage"
			// lua_gc(L, LUA_GCRESTART, 0);
			error = lua_pcall(state, nInArgs, nOutArgs, errFuncIdx);
			// only run GC inside of "SetHandleRunning(L, true) ... SetHandleRunning(L, false)"!
			lua_gc(state, LUA_GCSTOP, 0);

			LuaOpenGL::CheckMatrixState(state, func, error);
			matTracker.PopMatrixState(prevMatState);

			handle->SetHandleRunning(state, false);
			assert(oldRun == CLuaHandle::IsHandleRunning(luaState));
		}

		~ScopedLuaCall() {
			assert(!popErrFunc); // deprecated!
			if (popErrFunc) {
				lua_remove(luaState, errFuncIdx);
			}
		}

		void CheckFixStack(std::string& trace) {
			// note: assumes error-handler has not been popped yet (!)
			const int outArgs = (lua_gettop(luaState) - (GetTop() - 1)) + nInArgs;

			if (GetError() == 0) {
				if (nOutArgs != LUA_MULTRET) {
					if (outArgs != nOutArgs) {
						LOG_L(L_ERROR, "Internal Lua error: %d return values, %d expected", outArgs, nOutArgs);
						if (outArgs > nOutArgs)
							lua_pop(luaState, outArgs - nOutArgs);
					}
				} else {
					if (outArgs < 0) {
						LOG_L(L_ERROR, "Internal Lua error: stack corrupted");
					}
				}
			} else {
				const int dbgOutArgs = 1; // the traceback string

				if (outArgs > dbgOutArgs) {
					LOG_L(L_ERROR, "Internal Lua error: %i too many elements on the stack", outArgs - dbgOutArgs);
					lua_pop(luaState, outArgs - dbgOutArgs); // only leave traceback str on the stack
				} else if (outArgs < dbgOutArgs) {
					LOG_L(L_ERROR, "Internal Lua error: stack corrupted");
				}

				trace += "[Internal Lua error: Traceback failure] ";
				trace += luaL_optstring(luaState, -1, "[No traceback returned]");
				lua_pop(luaState, 1); // pop traceback string

				// log only errors that lead to a crash
				luaHandle->callinErrors += (GetError() == LUA_ERRRUN);
			}
		}

		int GetTop() const { return top; }
		int GetError() const { return error; }

	private:
		lua_State* luaState;
		CLuaHandle* luaHandle;

		int nInArgs;
		int nOutArgs;
		int errFuncIdx;
		bool popErrFunc;

		int top;
		int error;
	};

	// TODO: use closure so we do not need to copy args
	ScopedLuaCall call(this, L, hs, inArgs, outArgs, errFuncIndex, popErrorFunc);
	call.CheckFixStack(tracebackMsg);

	return (call.GetError());
}


bool CLuaHandle::RunCallInTraceback(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc)
{
	std::string traceback;
	const int error = RunCallInTraceback(L, &hs, inArgs, outArgs, errFuncIndex, traceback, popErrFunc);

	if (error != 0) {
		LOG_L(L_ERROR, "%s::RunCallIn: error = %i, %s, %s", GetName().c_str(),
				error, hs.GetString().c_str(), traceback.c_str());
		return false;
	}
	return true;
}


/******************************************************************************/

void CLuaHandle::Shutdown()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("Shutdown");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}


bool CLuaHandle::GotChatMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("GotChatMsg");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	lua_pushsstring(L, msg);
	lua_pushnumber(L, playerID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool processed = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return processed;
}


void CLuaHandle::Load(IArchive* archive)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("Load");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	// Load gets ZipFileReader userdatum as single argument
	LuaZipFileReader::PushNew(L, "", archive);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


bool CLuaHandle::HasCallIn(lua_State* L, const string& name)
{
	if (!IsValid())
		return false;

	if (name == "CollectGarbage")
		return true;

	//FIXME should be equal to below, but somehow it isn't and doesn't work as expected!?
// 	lua_getglobal(L, name.c_str());
// 	const bool found = !lua_isfunction(L, -1);
// 	lua_pop(L, 1);

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushsstring(L, name); // push the function name
	lua_rawget(L, -2);        // get the function
	const bool found = lua_isfunction(L, -1);
	lua_pop(L, 2);

	return found;
}


bool CLuaHandle::UpdateCallIn(lua_State* L, const string& name)
{
	if (HasCallIn(L, name)) {
		eventHandler.InsertEvent(this, name);
	} else {
		eventHandler.RemoveEvent(this, name);
	}
	return true;
}


void CLuaHandle::GamePreload()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("GamePreload");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::GameStart()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("GameStart");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::GameOver(const std::vector<unsigned char>& winningAllyTeams)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("GameOver");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_createtable(L, winningAllyTeams.size(), 0);
	for (unsigned int i = 0; i < winningAllyTeams.size(); i++) {
		lua_pushnumber(L, winningAllyTeams[i]);
		lua_rawseti(L, -2, i + 1);
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::GamePaused(int playerID, bool paused)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("GamePaused");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);
	lua_pushboolean(L, paused);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
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

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("GameFrame");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::GameID(const unsigned char* gameID, unsigned int numBytes)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	const LuaHashString cmdStr("GameID");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushlstring(L, reinterpret_cast<const char*>(gameID), numBytes);

	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::TeamDied(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("TeamDied");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::TeamChanged(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("TeamChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerChanged(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("PlayerChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerAdded(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("PlayerAdded");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerRemoved(int playerID, int reason)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("PlayerRemoved");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);
	lua_pushnumber(L, reason);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************/

inline void CLuaHandle::UnitCallIn(const LuaHashString& hs, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!hs.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(L, hs, 3, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitCreated");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	RunCallInTraceback(L, cmdStr, args, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitFinished(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitFinished");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitFromFactory");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallInTraceback(L, cmdStr, 6, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitTaken");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitGiven(const CUnit* unit, int oldTeam, int newTeam)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitGiven");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, newTeam);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitIdle(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitIdle");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitCommand(const CUnit* unit, const Command& command)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 11, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitCommand");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	lua_pushnumber(L, command.GetID());

	//FIXME: perhaps we should push the table version rather than the bitfield directly
	lua_pushnumber(L, command.options);

	// push the params list
	LuaUtils::PushCommandParamsTable(L, command, false);

	lua_pushnumber(L, command.tag);

	// call the routine
	RunCallInTraceback(L, cmdStr, 7, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitCmdDone(const CUnit* unit, const Command& command)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitCmdDone");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, command.GetID());
	lua_pushnumber(L, command.tag);
	// push the params list
	LuaUtils::PushCommandParamsTable(L, command, false);
	// push the options table
	LuaUtils::PushCommandOptionsTable(L, command, false);

	// call the routine
	RunCallInTraceback(L, cmdStr, 7, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 11, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	int argCount = 5;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);

	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, weaponDefID);
		lua_pushnumber(L, projectileID);
		argCount += 2;

		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			argCount += 3;
		}
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitExperience(const CUnit* unit, float oldExperience)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitExperience");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, oldExperience);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitHarvestStorageFull(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitHarvestStorageFull");
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __FUNCTION__);
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
	RunCallIn(L, cmdStr, GetHandleFullRead(L) ? 7 : 4, 0);
}


/******************************************************************************/

void CLuaHandle::LosCallIn(const LuaHashString& hs,
                           const CUnit* unit, int allyTeam)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __FUNCTION__);
	if (!hs.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->team);
	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(L, hs, GetHandleFullRead(L) ? 4 : 2, 0);
}


void CLuaHandle::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs("UnitEnteredRadar");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs("UnitEnteredLos");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftRadar(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs("UnitLeftRadar");
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs("UnitLeftLos");
	LosCallIn(hs, unit, allyTeam);
}


/******************************************************************************/

void CLuaHandle::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitLoaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitUnloaded(const CUnit* unit, const CUnit* transport)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("UnitUnloaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, 5, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************/

void CLuaHandle::UnitEnteredWater(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitEnteredWater");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitEnteredAir(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitEnteredAir");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftWater(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitLeftWater");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftAir(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitLeftAir");
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::UnitCloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitCloaked");
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitDecloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr("UnitDecloaked");
	UnitCallIn(cmdStr, unit);
}



void CLuaHandle::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (!watchUnitDefs[collider->unitDef->id]) return;
	if (!watchUnitDefs[collidee->unitDef->id]) return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr("UnitUnitCollision");
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);
	lua_pushboolean(L, false);

	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (watchFeatureDefs.empty()) return;
	if (!watchUnitDefs[collider->unitDef->id]) return;
	if (!watchFeatureDefs[collidee->def->id]) return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr("UnitFeatureCollision");
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);
	lua_pushboolean(L, false);

	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::UnitMoveFailed(const CUnit* unit)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty()) return;
	if (!watchUnitDefs[unit->unitDef->id]) return;

	static const LuaHashString cmdStr("UnitMoveFailed");
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);
	static const LuaHashString cmdStr("FeatureCreated");

	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr("FeatureDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::FeatureDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 11, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	int argCount = 4;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->def->id);
	lua_pushnumber(L, feature->team);
	lua_pushnumber(L, damage);

	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, weaponDefID); argCount += 1;
		lua_pushnumber(L, projectileID); argCount += 1;

		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			argCount += 3;
		}
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
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

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr("ProjectileCreated");

	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, p->id);
	lua_pushnumber(L, ((owner != NULL)? owner->id: -1));
	lua_pushnumber(L, ((wd != NULL)? wd->id: -1));

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
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

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);

	static const LuaHashString cmdStr("ProjectileDestroyed");

	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	lua_pushnumber(L, p->id);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
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

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __FUNCTION__);

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
	if (!RunCallIn(L, cmdStr, (owner == NULL) ? 4 : 5, 1))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


void CLuaHandle::StockpileChanged(const CUnit* unit,
                                  const CWeapon* weapon, int oldCount)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __FUNCTION__);

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
	RunCallIn(L, cmdStr, 6, 0);
}



bool CLuaHandle::RecvLuaMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 8, __FUNCTION__);

	static const LuaHashString cmdStr("RecvLuaMsg");
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, msg); // allows embedded 0's
	lua_pushnumber(L, playerID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
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


void CLuaHandle::Save(zipFile archive)
{
	// LuaUI does not get this call-in
	if (GetUserMode()) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __FUNCTION__);
	static const LuaHashString cmdStr("Save");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	// Save gets ZipFileWriter userdatum as single argument
	LuaZipFileWriter::PushNew(L, "", archive);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}


void CLuaHandle::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __FUNCTION__);
	static const LuaHashString cmdStr("UnsyncedHeightMapUpdate");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, rect.x1);
	lua_pushnumber(L, rect.z1);
	lua_pushnumber(L, rect.x2);
	lua_pushnumber(L, rect.z2);

	// call the routine
	RunCallIn(L, cmdStr, 4, 0);
}


void CLuaHandle::Update()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __FUNCTION__);
	static const LuaHashString cmdStr("Update");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	// call the routine
	RunCallIn(L, cmdStr, 0, 0);
}


void CLuaHandle::ViewResize()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("ViewResize");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	RunCallIn(L, cmdStr, 1, 0);
}


bool CLuaHandle::DefaultCommand(const CUnit* unit,
                                const CFeature* feature, int& cmd)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("DefaultCommand");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	if (!RunCallIn(L, cmdStr, args, 1))
		return false;

	if (!lua_isnumber(L, 1)) {
		lua_pop(L, 1);
		return false;
	}

	cmd = lua_toint(L, -1);
	lua_pop(L, 1);
	return true;
}


void CLuaHandle::RunDrawCallIn(const LuaHashString& hs)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __FUNCTION__);
	if (!hs.GetGlobalFunc(L)) {
		return;
	}

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, hs, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawGenesis()
{
	static const LuaHashString cmdStr("DrawGenesis");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawWorld()
{
	static const LuaHashString cmdStr("DrawWorld");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawWorldPreUnit()
{
	static const LuaHashString cmdStr("DrawWorldPreUnit");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawWorldShadow()
{
	static const LuaHashString cmdStr("DrawWorldShadow");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawWorldReflection()
{
	static const LuaHashString cmdStr("DrawWorldReflection");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawWorldRefraction()
{
	static const LuaHashString cmdStr("DrawWorldRefraction");
	RunDrawCallIn(cmdStr);
}


void CLuaHandle::DrawScreen()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("DrawScreen");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawScreenEffects()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("DrawScreenEffects");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaHandle::DrawInMiniMap()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("DrawInMiniMap");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


void CLuaHandle::GameProgress(int frameNum )
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __FUNCTION__);
	static const LuaHashString cmdStr("GameProgress");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}


/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::KeyPress(int key, bool isRepeat)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __FUNCTION__);
	static const LuaHashString cmdStr("KeyPress");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key); //FIXME return scancode here?

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	HSTR_PUSH_BOOL(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushnumber(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::KeyRelease(int key)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("KeyRelease");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	HSTR_PUSH_BOOL(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushnumber(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::TextInput(const std::string& utf8)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __FUNCTION__);
	static const LuaHashString cmdStr("TextInput");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushsstring(L, utf8);
	//lua_pushnumber(L, UTF8toUTF32(utf8));

	// call the function
	if (!RunCallIn(L, cmdStr, 1, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MousePress(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("MousePress");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


void CLuaHandle::MouseRelease(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("MouseRelease");
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	RunCallIn(L, cmdStr, 3, 0);
}


bool CLuaHandle::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __FUNCTION__);
	static const LuaHashString cmdStr("MouseMove");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MouseWheel(bool up, float value)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("MouseWheel");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushboolean(L, up);
	lua_pushnumber(L, value);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::JoystickEvent(const std::string& event, int val1, int val2)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);
	const LuaHashString cmdStr(event);
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, val1);
	lua_pushnumber(L, val2);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::IsAbove(int x, int y)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("IsAbove");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


string CLuaHandle::GetTooltip(int x, int y)
{
	if (!CheckModUICtrl()) {
		return "";
	}
	LUA_CALL_IN_CHECK(L, "");
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("GetTooltip");
	if (!cmdStr.GetGlobalFunc(L)) {
		return ""; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return "";

	const string retval = luaL_optsstring(L, -1, "");
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::CommandNotify(const Command& cmd)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("CommandNotify");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	// push the command id
	lua_pushnumber(L, cmd.GetID());

	// push the params list
	LuaUtils::PushCommandParamsTable(L, cmd, false);
	// push the options table
	LuaUtils::PushCommandOptionsTable(L, cmd, false);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::AddConsoleLine(const string& msg, const string& section, int level)
{
	if (!CheckModUICtrl()) {
		return true; // FIXME?
	}

	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("AddConsoleLine");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushsstring(L, msg);
	lua_pushnumber(L, level);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 0))
		return false;

	return true;
}



bool CLuaHandle::GroupChanged(int groupID)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __FUNCTION__);
	static const LuaHashString cmdStr("GroupChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, groupID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 1, 0))
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
	luaL_checkstack(L, 6, __FUNCTION__);
	static const LuaHashString cmdStr("WorldTooltip");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	if (!RunCallIn(L, cmdStr, args, 1))
		return "";

	const string retval = luaL_optstring(L, -1, "");
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
	luaL_checkstack(L, 9, __FUNCTION__);
	static const LuaHashString cmdStr("MapDrawCmd");
	if (!cmdStr.GetGlobalFunc(L)) {
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
	if (!RunCallIn(L, cmdStr, args, 1))
		return false;

	// take the event?
	const bool retval = luaL_optboolean(L, -1, false);
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
	luaL_checkstack(L, 5, __FUNCTION__);
	static const LuaHashString cmdStr("GameSetup");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false;
	}

	lua_pushsstring(L, state);

	lua_pushboolean(L, ready);

	lua_newtable(L);
	map<int, string>::const_iterator it;
	for (it = playerStates.begin(); it != playerStates.end(); ++it) {
		lua_pushsstring(L, it->second);
		lua_rawseti(L, -2, it->first);
	}

	// call the routine
	if (!RunCallIn(L, cmdStr, 3, 2))
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
	luaL_checkstack(L, 4, __FUNCTION__);

	static const LuaHashString cmdStr("RecvSkirmishAIMessage");

	// <this> is either CLuaRules* or CLuaUI*,
	// but the AI call-in is always unsynced!
	if (!cmdStr.GetGlobalFunc(L)) {
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

	if (!RunCallIn(L, cmdStr, argCount, 1))
		return NULL;

	if (lua_isstring(L, -1))
		outData = lua_tolstring(L, -1, NULL);

	lua_pop(L, 1);
	return outData;
}

/******************************************************************************/
/******************************************************************************/

CONFIG(float, MaxLuaGarbageCollectionTime ).defaultValue(1000.0f / GAME_SPEED).minimumValue(1.0f).description("in MilliSecs");
CONFIG(  int, MaxLuaGarbageCollectionSteps).defaultValue(10000               ).minimumValue(1   );

void CLuaHandle::CollectGarbage()
{
	//LOG("CLuaHandle::CollectGarbage %s", GetName().c_str());

	lua_gc(L, LUA_GCSTOP, 0); // don't collect garbage outside of this function

	static const float maxLuaGarbageCollectTime  = configHandler->GetFloat("MaxLuaGarbageCollectionTime" );
	static const   int maxLuaGarbageCollectSteps = configHandler->GetInt  ("MaxLuaGarbageCollectionSteps");

	// kilobytes --> megabytes (note: total footprint INCLUDING garbage)
	const int luaMemFootPrint = lua_gc(L, LUA_GCCOUNT, 0) / 1024;

	// 25MB --> 20usecs, 100MB --> 100usecs (30x per second)
	const float rawRunTime = luaMemFootPrint * (0.02f + 0.08f * smoothstep(25, 100, luaMemFootPrint));
	const float maxRunTime = std::min(rawRunTime, maxLuaGarbageCollectTime);

	const spring_time startTime = spring_gettime();
	const spring_time endTime = startTime + spring_msecs(maxRunTime);

	// collect garbage until time runs out or the maximum
	// number of steps is exceeded, whichever comes first

	static int gcsteps = 10;
	int numLuaGarbageCollectIters = 0;

	assert(!IsRunning());
	SetHandleRunning(L, true);

	while ((numLuaGarbageCollectIters++ < maxLuaGarbageCollectSteps) && (spring_gettime() < endTime)) {
		if (lua_gc(L, LUA_GCSTEP, gcsteps)) {
			// garbage-collection finished
			break;
		}
	}

	SetHandleRunning(L, false);

	if (gcsteps > 1) {
		// runtime optimize number of steps to process in a batch
		const float avgTimePerLoopIter = (spring_gettime() - startTime).toMilliSecsf() / numLuaGarbageCollectIters;

		if (avgTimePerLoopIter > (maxLuaGarbageCollectTime * 0.150f)) gcsteps--;
		if (avgTimePerLoopIter < (maxLuaGarbageCollectTime * 0.075f)) gcsteps++;
	}
}

/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::AddBasicCalls(lua_State *L)
{
	HSTR_PUSH(L, "Script");
	lua_newtable(L); {
		HSTR_PUSH_CFUNC(L, "Kill",            KillActiveHandle);
		HSTR_PUSH_CFUNC(L, "UpdateCallIn",    CallOutUpdateCallIn);
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
	}
	lua_rawset(L, -3);

	// extra math utilities
	lua_getglobal(L, "math");
	LuaBitOps::PushEntries(L);
	LuaMathExtra::PushEntries(L);
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


int CLuaHandle::CallOutUpdateCallIn(lua_State* L)
{

	const string name = luaL_checkstring(L, 1);
	CLuaHandle* lh = GetHandle(L);
	lh->UpdateCallIn(L, name);
	return 0;


}


/******************************************************************************/
/******************************************************************************/

