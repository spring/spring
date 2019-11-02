/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandle.h"

#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaUI.h"

#include "LuaCallInCheck.h"
#include "LuaConfig.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaBitOps.h"
#include "LuaMathExtra.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/Protocol/NetProtocol.h"
#include "Game/UI/KeySet.h"
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
#include "System/creg/SerializeLuaState.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/Rectangle.h"
#include "System/ScopedFPUSettings.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Input/KeyInput.h"
#include "System/Platform/SDL1_keysym.h"

#include "LuaInclude.h"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>


#include <string>


CONFIG(float, LuaGarbageCollectionMemLoadMult).defaultValue(1.33f).minimumValue(1.0f).maximumValue(100.0f);
CONFIG(float, LuaGarbageCollectionRunTimeMult).defaultValue(5.0f).minimumValue(1.0f).description("in milliseconds");


static spring::unsynced_set<const luaContextData*>    SYNCED_LUAHANDLE_CONTEXTS;
static spring::unsynced_set<const luaContextData*>  UNSYNCED_LUAHANDLE_CONTEXTS;
const  spring::unsynced_set<const luaContextData*>*          LUAHANDLE_CONTEXTS[2] = {&UNSYNCED_LUAHANDLE_CONTEXTS, &SYNCED_LUAHANDLE_CONTEXTS};

bool CLuaHandle::devMode = false;


/******************************************************************************/
/******************************************************************************/

void CLuaHandle::PushTracebackFuncToRegistry(lua_State* L)
{
	SPRING_LUA_OPEN_LIB(L, luaopen_debug);
		HSTR_PUSH(L, "traceback");
		LuaUtils::PushDebugTraceback(L);
		lua_rawset(L, LUA_REGISTRYINDEX);
	// We only need the debug.traceback function, the others are unsafe for syncing.
	// Later CLuaHandle implementations decide themselves if they want to reload the lib or not (LuaUI does).
	LUA_UNLOAD_LIB(L, LUA_DBLIBNAME);
}


static void LUA_INSERT_CONTEXT(const luaContextData* D, const spring::unsynced_set<const luaContextData*>* S) {
	const_cast<  spring::unsynced_set<const luaContextData*>*  >(S)->insert(D);
}
static void LUA_ERASE_CONTEXT(const luaContextData* D, const spring::unsynced_set<const luaContextData*>* S) {
	const_cast<  spring::unsynced_set<const luaContextData*>*  >(S)->erase(D);
}

static int handlepanic(lua_State* L)
{
	throw content_error(luaL_optsstring(L, 1, "lua paniced"));
}



CLuaHandle::CLuaHandle(const string& _name, int _order, bool _userMode, bool _synced)
	: CEventClient(_name, _order, _synced)
	, userMode(_userMode)
	, killMe(false)
	// no shared pool for LuaIntro to protect against LoadingMT=1
	// do not use it for LuaMenu either; too many blocks allocated
	// by *other* states end up not being recycled which presently
	// forces clearing the shared pool on reload
	, D(_name != "LuaIntro" && name != "LuaMenu", true)
{
	D.owner = this;
	D.synced = _synced;

	D.gcCtrl.baseMemLoadMult = configHandler->GetFloat("LuaGarbageCollectionMemLoadMult");
	D.gcCtrl.baseRunTimeMult = configHandler->GetFloat("LuaGarbageCollectionRunTimeMult");

	L = LUA_OPEN(&D);
	L_GC = lua_newthread(L);

	LUA_INSERT_CONTEXT(&D, LUAHANDLE_CONTEXTS[D.synced]);

	luaL_ref(L, LUA_REGISTRYINDEX);

	// needed for engine traceback
	PushTracebackFuncToRegistry(L);

	// prevent lua from calling c's exit()
	lua_atpanic(L, handlepanic);
}


CLuaHandle::~CLuaHandle()
{
	// KillLua() must be called before us!
	assert(!IsValid());
	assert(!eventHandler.HasClient(this));
}


// can be called from a handler constructor or FreeHandler
// we care about calling Shutdown only in the latter case!
void CLuaHandle::KillLua(bool inFreeHandler)
{
	// 1. unlink from eventHandler, so no new events are getting triggered
	//FIXME when multithreaded lua is enabled, wait for all running events to finish (possible via a mutex?)
	eventHandler.RemoveClient(this);

	if (!IsValid())
		return;

	// 2. shutdown
	if (inFreeHandler)
		Shutdown();

	// 3. delete the lua_State
	//
	// must be done here: if called from a ctor, we want the
	// state to become non-valid so that LoadHandler returns
	// false and FreeHandler runs next
	LUA_ERASE_CONTEXT(&D, LUAHANDLE_CONTEXTS[D.synced]);
	LUA_CLOSE(&L);
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::KillActiveHandle(lua_State* L)
{
	CLuaHandle* ah = GetHandle(L);

	if (ah != nullptr) {
		if ((lua_gettop(L) >= 1) && lua_isstring(L, 1))
			ah->killMsg = lua_tostring(L, 1);

		// get rid of us next GameFrame call
		ah->killMe = true;

		// don't process any further events
		eventHandler.RemoveClient(ah);
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


/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CheckStack()
{
	if (!IsValid())
		return;

	const int top = lua_gettop(L);
	if (top != 0) {
		LOG_L(L_WARNING, "[LuaHandle::%s] %s stack-top = %i", __func__, name.c_str(), top);
		lua_settop(L, 0);
	}
}


int CLuaHandle::XCall(lua_State* srcState, const char* funcName)
{
	const int top = lua_gettop(L);

	// push the function
	const LuaHashString funcHash(funcName);
	if (!funcHash.GetGlobalFunc(L)) {
		LOG_L(L_WARNING, "[LuaHandle::%s] tried to cross-call unlinked Script.%s.%s()", __func__, name.c_str(), funcName);
		return 0;
	}

	int retCount;

	if (srcState == L) {
		lua_insert(L, 1); // move the function to the beginning

		// call the function
		if (!RunCallIn(L, funcHash, top, LUA_MULTRET))
			return 0;

		retCount = lua_gettop(L);
	} else {
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

		lua_settop(srcState, 0); // pop all passed arguments on caller stack
		if (retCount > 0)
			LuaUtils::CopyData(srcState, L, retCount); // push the new returned arguments on caller stack

		lua_settop(L, top); // revert the callee stack
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::RunCallInTraceback(
	lua_State* L,
	const LuaHashString* hs,
	std::string* ts,
	int inArgs,
	int outArgs,
	int errFuncIndex,
	bool popErrorFunc
) {
	// do not signal floating point exceptions in user Lua code
	ScopedDisableFpuExceptions fe;

	struct ScopedLuaCall {
	public:
		ScopedLuaCall(
			CLuaHandle* handle,
			lua_State* state,
			const char* func,
			int _nInArgs,
			int _nOutArgs,
			int _errFuncIdx,
			bool _popErrFunc
		)
			: luaState(state)
			, luaHandle(handle)
			, luaFunc(func)

			, nInArgs(_nInArgs)
			, nOutArgs(_nOutArgs)
			, errFuncIdx(_errFuncIdx)
			, popErrFunc(_popErrFunc)
		{
			handle->SetHandleRunning(state, true); // inc
			const bool canDraw = LuaOpenGL::IsDrawingEnabled(state);

			SMatrixStateData prevMatState;
			GLMatrixStateTracker& matTracker = GetLuaContextData(state)->glMatrixTracker;

			if (canDraw) {
				prevMatState = matTracker.PushMatrixState();
				LuaOpenGL::InitMatrixState(state, luaFunc);
			}

			top = lua_gettop(state);
			// note1: disable GC outside of this scope to prevent sync errors and similar
			// note2: we collect garbage now in its own callin "CollectGarbage"
			// lua_gc(L, LUA_GCRESTART, 0);
			error = lua_pcall(state, nInArgs, nOutArgs, errFuncIdx);
			// only run GC inside of "SetHandleRunning(L, true) ... SetHandleRunning(L, false)"!
			lua_gc(state, LUA_GCSTOP, 0);

			if (canDraw) {
				LuaOpenGL::CheckMatrixState(state, luaFunc, error);
				matTracker.PopMatrixState(prevMatState);
			}

			handle->SetHandleRunning(state, false); // dec
		}

		~ScopedLuaCall() {
			assert(!popErrFunc); // deprecated!
			if (popErrFunc) {
				lua_remove(luaState, errFuncIdx);
			}
		}

		void CheckFixStack(std::string& traceStr) {
			// note: assumes error-handler has not been popped yet (!)
			const int curTop = lua_gettop(luaState);
			const int outArgs = (curTop - (GetTop() - 1)) + nInArgs;

			if (GetError() == 0) {
				if (nOutArgs != LUA_MULTRET) {
					if (outArgs != nOutArgs) {
						LOG_L(L_ERROR, "[SLC::%s] %d ret-vals but %d expected for callin %s", __func__, outArgs, nOutArgs, luaFunc);

						if (outArgs > nOutArgs)
							lua_pop(luaState, outArgs - nOutArgs);
					}
				} else {
					// should not be reachable without getting a LUA_ERR*
					if (outArgs < 0) {
						LOG_L(L_ERROR, "[SLC::%s] %d ret-vals (top={%d,%d} args=%d) for callin %s, corrupt stack", __func__, outArgs, curTop, GetTop(), nInArgs, luaFunc);
					}
				}
			} else {
				// traceback string is optionally left on the stack
				// might also have been popped in case of underflow
				constexpr int dbgOutArgs = 1;

				if (outArgs > dbgOutArgs) {
					LOG_L(L_ERROR, "[SLC::%s] %i excess values on stack for callin %s", __func__, outArgs - dbgOutArgs, luaFunc);
					// only leave traceback string on the stack, popped below
					lua_pop(luaState, outArgs - dbgOutArgs);
				} else if (outArgs < dbgOutArgs) {
					LOG_L(L_ERROR, "[SLC::%s] %d ret-vals (top={%d,%d} args=%d) for callin %s, corrupt stack", __func__, outArgs, curTop, GetTop(), nInArgs, luaFunc);
					// make the pop() below valid
					lua_pushnil(luaState);
				}

				traceStr.append("[Internal Lua error: Call failure] ");
				traceStr.append(luaL_optstring(luaState, -1, "[No traceback returned]"));

				// pop traceback string
				lua_pop(luaState, dbgOutArgs);

				// log only errors that lead to a crash
				luaHandle->callinErrors += (GetError() == LUA_ERRRUN);

				// catch clients that desync due to corrupted data
				if (GetError() == LUA_ERRRUN && GetHandleSynced(luaState))
					CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, traceStr);
			}
		}

		int GetTop() const { return top; }
		int GetError() const { return error; }

	private:
		lua_State* luaState;
		CLuaHandle* luaHandle;
		const char* luaFunc;

		int nInArgs;
		int nOutArgs;
		int errFuncIdx;
		bool popErrFunc;

		int top;
		int error;
	};

	// TODO: use closure so we do not need to copy args
	ScopedLuaCall call(this, L, (hs != nullptr)? hs->GetString(): "LUS::?", inArgs, outArgs, errFuncIndex, popErrorFunc);
	call.CheckFixStack(*ts);

	return (call.GetError());
}


bool CLuaHandle::RunCallInTraceback(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc)
{
	std::string traceStr;
	const int error = RunCallInTraceback(L, &hs, &traceStr, inArgs, outArgs, errFuncIndex, popErrFunc);

	if (error == 0)
		return true;

	const auto& hn = GetName();
	const char* hsn = hs.GetString();
	const char* es = LuaErrorString(error);

	LOG_L(L_ERROR, "[%s::%s] error=%i (%s) callin=%s trace=%s", hn.c_str(), __func__, error, es, hsn, traceStr.c_str());

	if (error == LUA_ERRMEM) {
		// try to free some memory so other lua states can alloc again
		CollectGarbage(true);
		// kill the entire handle next frame
		KillActiveHandle(L);
	}

	return false;
}

/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::LoadCode(lua_State* L, const string& code, const string& debug)
{
	lua_settop(L, 0);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	const int error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());

	if (error != 0) {
		LOG_L(L_ERROR, "[%s::%s] error=%i (%s) debug=%s msg=%s", name.c_str(), __func__, error, LuaErrorString(error), debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	static const LuaHashString cmdStr(__func__);

	// call Initialize immediately after load
	return (RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false));
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::Shutdown()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}


bool CLuaHandle::GotChatMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	bool processed = false;
	if (cmdStr.GetGlobalFunc(L)) {
		lua_pushsstring(L, msg);
		lua_pushnumber(L, playerID);

		// call the routine
		if (RunCallIn(L, cmdStr, 2, 1)) {
			processed = luaL_optboolean(L, -1, false);
			lua_pop(L, 1);
		}
	}

	if (!processed && (this == luaUI)) {
		processed = luaUI->ConfigureLayout(msg); //FIXME deprecated
	}
	return processed;
}


void CLuaHandle::Load(IArchive* archive)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// Load gets ZipFileReader userdatum as single argument
	LuaZipFileReader::PushNew(L, "", archive);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


bool CLuaHandle::HasCallIn(lua_State* L, const string& name) const
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
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::GameStart()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallInTraceback(L, cmdStr, 0, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::GameOver(const std::vector<unsigned char>& winningAllyTeams)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);
	lua_pushboolean(L, paused);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::GameFrame(int frameNum)
{
	if (killMe) {
		const std::string msg = GetName() + ((!killMsg.empty())? ": " + killMsg: "");

		LOG("[%s] disabled %s", __func__, msg.c_str());
		delete this;
		return;
	}

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::GameID(const unsigned char* gameID, unsigned int numBytes)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	char buf[33];

	SNPRINTF(buf, sizeof(buf),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			gameID[ 0], gameID[ 1], gameID[ 2], gameID[ 3], gameID[ 4], gameID[ 5], gameID[ 6], gameID[ 7],
			gameID[ 8], gameID[ 9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
	lua_pushstring(L, buf);

	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::TeamDied(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::TeamChanged(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerChanged(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerAdded(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::PlayerRemoved(int playerID, int reason)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, playerID);
	lua_pushnumber(L, reason);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************/

inline void CLuaHandle::UnitCallIn(const LuaHashString& hs, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!hs.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(L, hs, 3, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (builder != nullptr)
		lua_pushnumber(L, builder->id);

	// call the routine
	RunCallInTraceback(L, cmdStr, (builder != nullptr)? 4: 3, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitFinished(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallInTraceback(L, cmdStr, 6, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitReverseBuilt(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int argCount = 3 + 3;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	if (GetHandleFullRead(L) && (attacker != nullptr)) {
		lua_pushnumber(L, attacker->id);
		lua_pushnumber(L, attacker->unitDef->id);
		lua_pushnumber(L, attacker->team);
	} else {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 7, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 7, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, newTeam);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitIdle(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitCommand(const CUnit* unit, const Command& command, int playerNum, bool fromSynced, bool fromLua)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 1 + 7 + 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int argc = LuaUtils::PushUnitAndCommand(L, unit, command);

	lua_pushnumber(L, playerNum);
	lua_pushboolean(L, fromSynced);
	lua_pushboolean(L, fromLua);

	// call the routine
	RunCallInTraceback(L, cmdStr, argc + 3, 0, traceBack.GetErrFuncIdx(), false);
}


void CLuaHandle::UnitCmdDone(const CUnit* unit, const Command& command)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	LuaUtils::PushUnitAndCommand(L, unit, command);

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
	luaL_checkstack(L, 11, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	int argCount = 7;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	// these two do not count as information leaks
	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, projectileID);

	if (attacker != nullptr && GetHandleFullRead(L)) {
		lua_pushnumber(L, attacker->id);
		lua_pushnumber(L, attacker->unitDef->id);
		lua_pushnumber(L, attacker->team);
		argCount += 3;
	}

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::UnitStunned(
	const CUnit* unit,
	bool stunned)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushboolean(L, stunned);

	// call the routine
	RunCallInTraceback(L, cmdStr, 4, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::UnitExperience(const CUnit* unit, float oldExperience)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);
	int readAllyTeam = GetHandleReadAllyTeam(L);
	if ((readAllyTeam >= 0) && (unit->losStatus[readAllyTeam] & LOS_INLOS)) {
		return; // don't need to see this ping
	}

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 6, __func__);
	if (!hs.GetGlobalFunc(L))
		return;

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
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftRadar(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__func__);
	LosCallIn(hs, unit, allyTeam);
}


/******************************************************************************/

void CLuaHandle::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitEnteredAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftWater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitLeftAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


/******************************************************************************/

void CLuaHandle::UnitCloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::UnitDecloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}



bool CLuaHandle::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	// if empty, we are not a LuaHandleSynced
	if (watchUnitDefs.empty())
		return false;

	if (!watchUnitDefs[collider->unitDef->id])
		return false;
	if (!watchUnitDefs[collidee->unitDef->id])
		return false;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);

	RunCallInTraceback(L, cmdStr, 2, 1, traceBack.GetErrFuncIdx(), false);

	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}

bool CLuaHandle::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	// if empty, we are not a LuaHandleSynced (and must always return false)
	if (watchUnitDefs.empty())
		return false;
	if (watchFeatureDefs.empty())
		return false;

	if (!watchUnitDefs[collider->unitDef->id])
		return false;
	if (!watchFeatureDefs[collidee->def->id])
		return false;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, collider->id);
	lua_pushnumber(L, collidee->id);

	RunCallInTraceback(L, cmdStr, 2, 1, traceBack.GetErrFuncIdx(), false);

	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}

void CLuaHandle::UnitMoveFailed(const CUnit* unit)
{
	// if empty, we are not a LuaHandleSynced (and must always return false)
	if (watchUnitDefs.empty())
		return;
	if (!watchUnitDefs[unit->unitDef->id])
		return;

	static const LuaHashString cmdStr(__func__);
	UnitCallIn(cmdStr, unit);
}


void CLuaHandle::RenderUnitDestroyed(const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 9, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	const int argCount = 3;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(L, cmdStr, argCount, 0, traceBack.GetErrFuncIdx(), false);
}


/******************************************************************************/

void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 11, __func__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
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

		if (attacker != nullptr) {
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
	if (watchProjectileDefs.empty())
		return;

	if (!p->weapon && !p->piece)
		return;

	assert(p->synced);

	const CUnit* owner = p->owner();
	const CWeaponProjectile* wp = p->weapon? static_cast<const CWeaponProjectile*>(p): nullptr;
	const WeaponDef* wd = p->weapon? wp->GetWeaponDef(): nullptr;

	// if this weapon-type is not being watched, bail
	if (p->weapon && (wd == nullptr || !watchProjectileDefs[wd->id]))
		return;
	if (p->piece && !watchProjectileDefs[watchProjectileDefs.size() - 1])
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, p->id);
	lua_pushnumber(L, ((owner != nullptr)? owner->id: -1));
	lua_pushnumber(L, ((wd != nullptr)? wd->id: -1));

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
}


void CLuaHandle::ProjectileDestroyed(const CProjectile* p)
{
	// if empty, we are not a LuaHandleSynced
	if (watchProjectileDefs.empty())
		return;

	if (!p->weapon && !p->piece)
		return;

	assert(p->synced);

	if (p->weapon) {
		const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);
		const WeaponDef* wd = wp->GetWeaponDef();

		// if this weapon-type is not being watched, bail
		if (wd == nullptr || !watchProjectileDefs[wd->id])
			return;
	}
	if (p->piece && !watchProjectileDefs[watchProjectileDefs.size() - 1])
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, p->id);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}

/******************************************************************************/

bool CLuaHandle::Explosion(int weaponDefID, int projectileID, const float3& pos, const CUnit* owner)
{
	// piece-projectile collision (*ALL* other
	// explosion events pass valid weaponDefIDs)
	if (weaponDefID < 0)
		return false;

	// if empty, we are not a LuaHandleSynced
	if (watchExplosionDefs.empty())
		return false;
	if (!watchExplosionDefs[weaponDefID])
		return false;

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, weaponDefID);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	if (owner != nullptr) {
		lua_pushnumber(L, owner->id);
	} else {
		lua_pushnil(L); // for backward compatibility
	}
	lua_pushnumber(L, projectileID);

	// call the routine
	if (!RunCallIn(L, cmdStr, 6, 1))
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
	luaL_checkstack(L, 8, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, weapon->weaponNum + LUA_WEAPON_BASE_INDEX);
	lua_pushnumber(L, oldCount);
	lua_pushnumber(L, weapon->numStockpiled);

	// call the routine
	RunCallIn(L, cmdStr, 6, 0);
}



bool CLuaHandle::RecvLuaMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 8, __func__);

	static const LuaHashString cmdStr(__func__);
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

void CLuaHandle::HandleLuaMsg(int playerID, int script, int mode, const std::vector<std::uint8_t>& data)
{
	std::string msg;
	msg.resize(data.size());
	std::copy(data.begin(), data.end(), msg.begin());

	switch (script) {
		case LUA_HANDLE_ORDER_UI: {
			if (luaUI != nullptr) {
				bool sendMsg = false;

				switch (mode) {
					case 0: { sendMsg = true; } break;
					case 's': { sendMsg = gu->spectating; } break;
					case 'a': {
						const CPlayer* player = playerHandler.Player(playerID);

						if (player == nullptr)
							return;

						if (gu->spectatingFullView) {
							sendMsg = true;
						} else if (player->spectator) {
							sendMsg = gu->spectating;
						} else {
							const int msgAllyTeam = teamHandler.AllyTeam(player->team);
							sendMsg = teamHandler.Ally(msgAllyTeam, gu->myAllyTeam);
						}
					} break;
				}

				if (sendMsg)
					luaUI->RecvLuaMsg(msg, playerID);
			}
		} break;

		case LUA_HANDLE_ORDER_GAIA: {
			if (luaGaia != nullptr)
				luaGaia->RecvLuaMsg(msg, playerID);
		} break;

		case LUA_HANDLE_ORDER_RULES: {
			if (luaRules != nullptr)
				luaRules->RecvLuaMsg(msg, playerID);
		} break;
	}
}


/******************************************************************************/


void CLuaHandle::Save(zipFile archive)
{
	// LuaUI does not get this call-in
	if (GetUserMode())
		return;

	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// Save gets ZipFileWriter userdatum as single argument
	LuaZipFileWriter::PushNew(L, "", archive);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}


void CLuaHandle::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallIn(L, cmdStr, 0, 0);
}


void CLuaHandle::ViewResize()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

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

void CLuaHandle::SunChanged()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	// call the routine
	RunCallIn(L, cmdStr, 0, 0);
}

bool CLuaHandle::DefaultCommand(const CUnit* unit,
                                const CFeature* feature, int& cmd)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	if (unit) {
		HSTR_PUSH(L, "unit");
		lua_pushnumber(L, unit->id);
	}
	else if (feature) {
		HSTR_PUSH(L, "feature");
		lua_pushnumber(L, feature->id);
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
	}
	lua_pushnumber(L, cmd);

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
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	if (!lua_isnumber(L, -1)) {
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
	luaL_checkstack(L, 2, __func__);
	if (!hs.GetGlobalFunc(L))
		return;

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, hs, 0, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}

#define DRAW_CALLIN(name)                     \
void CLuaHandle::name()                       \
{                                             \
	static const LuaHashString cmdStr(#name); \
	RunDrawCallIn(cmdStr);                    \
}


DRAW_CALLIN(DrawGenesis)
DRAW_CALLIN(DrawWater)
DRAW_CALLIN(DrawSky)
DRAW_CALLIN(DrawSun)
DRAW_CALLIN(DrawGrass)
DRAW_CALLIN(DrawTrees)
DRAW_CALLIN(DrawWorld)
DRAW_CALLIN(DrawWorldPreUnit)
DRAW_CALLIN(DrawWorldPreParticles)
DRAW_CALLIN(DrawWorldShadow)
DRAW_CALLIN(DrawWorldReflection)
DRAW_CALLIN(DrawWorldRefraction)
DRAW_CALLIN(DrawGroundPreForward)
DRAW_CALLIN(DrawGroundPostForward)
DRAW_CALLIN(DrawGroundPreDeferred)
DRAW_CALLIN(DrawGroundPostDeferred)
DRAW_CALLIN(DrawUnitsPostDeferred)
DRAW_CALLIN(DrawFeaturesPostDeferred)

inline void CLuaHandle::DrawScreenCommon(const LuaHashString& cmdStr)
{
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, false);
}

void CLuaHandle::DrawScreen()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}


void CLuaHandle::DrawScreenEffects()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}

void CLuaHandle::DrawScreenPost()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);

	DrawScreenCommon(cmdStr);
}


void CLuaHandle::DrawInMiniMap()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


void CLuaHandle::DrawInMiniMapBackground()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());

	const bool origDrawingState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);

	LuaOpenGL::SetDrawingEnabled(L, origDrawingState);
}


void CLuaHandle::GameProgress(int frameNum)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 3, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, frameNum);

	// call the routine
	RunCallIn(L, cmdStr, 1, 0);
}

void CLuaHandle::Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 1 + 1 + 3, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, pingTag);
	lua_pushnumber(L, pktSendTime.toMilliSecsf());
	lua_pushnumber(L, pktRecvTime.toMilliSecsf());

	// call the routine
	RunCallIn(L, cmdStr, 3, 0);
}


/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::KeyPress(int key, bool isRepeat)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);

	// if the call is not defined, do not take the event
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	//FIXME we should never had started using directly SDL consts, somaeday we should weakly force lua-devs to fix their code
	lua_pushinteger(L, SDL21_keysyms(key));

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	HSTR_PUSH_BOOL(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushinteger(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::KeyRelease(int key)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushinteger(L, SDL21_keysyms(key));

	lua_createtable(L, 0, 4);
	HSTR_PUSH_BOOL(L, "alt",   !!KeyInput::GetKeyModState(KMOD_ALT));
	HSTR_PUSH_BOOL(L, "ctrl",  !!KeyInput::GetKeyModState(KMOD_CTRL));
	HSTR_PUSH_BOOL(L, "meta",  !!KeyInput::GetKeyModState(KMOD_GUI));
	HSTR_PUSH_BOOL(L, "shift", !!KeyInput::GetKeyModState(KMOD_SHIFT));

	CKeySet ks(key, false);
	lua_pushsstring(L, ks.GetString(true));
	lua_pushinteger(L, 0); //FIXME remove, was deprecated utf32 char (now uses TextInput for that)

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::TextInput(const std::string& utf8)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, utf8);
	//lua_pushnumber(L, UTF8toUTF32(utf8));

	// call the function
	if (!RunCallIn(L, cmdStr, 1, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::TextEditing(const std::string& utf8, unsigned int start, unsigned int length)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, utf8);
	lua_pushinteger(L, start);
	lua_pushinteger(L, length);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MousePress(int x, int y, int button)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

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
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	RunCallIn(L, cmdStr, 3, 0);
}


bool CLuaHandle::MouseMove(int x, int y, int dx, int dy, int button)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 7, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

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
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushboolean(L, up);
	lua_pushnumber(L, value);

	// call the function
	if (!RunCallIn(L, cmdStr, 2, 1))
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::IsAbove(int x, int y)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

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
	LUA_CALL_IN_CHECK(L, "");
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return "";

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
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

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
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true;

	lua_pushsstring(L, msg);
	lua_pushnumber(L, level);

	// call the function
	return RunCallIn(L, cmdStr, 2, 0);
}



bool CLuaHandle::GroupChanged(int groupID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, groupID);

	// call the routine
	return RunCallIn(L, cmdStr, 1, 0);
}



string CLuaHandle::WorldTooltip(const CUnit* unit,
                                const CFeature* feature,
                                const float3* groundPos)
{
	LUA_CALL_IN_CHECK(L, "");
	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return "";

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
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 9, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

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
                           const std::vector< std::pair<int, std::string> >& playerStates)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushsstring(L, state);
	lua_pushboolean(L, ready);

	lua_newtable(L);

	for (const auto& playerState: playerStates) {
		lua_pushsstring(L, playerState.second);
		lua_rawseti(L, -2, playerState.first);
	}

	// call the routine
	if (!RunCallIn(L, cmdStr, 3, 2))
		return false;

	if (lua_isboolean(L, -2)) {
		if (lua_toboolean(L, -2)) {
			// only allow ready-state change if Lua takes the event
			if (lua_isboolean(L, -1))
				ready = lua_toboolean(L, -1);

			lua_pop(L, 2);
			return true;
		}
	}
	lua_pop(L, 2);
	return false;
}



const char* CLuaHandle::RecvSkirmishAIMessage(int aiTeam, const char* inData, int inSize, size_t* outSize)
{
	LUA_CALL_IN_CHECK(L, nullptr);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);

	// <this> is either CLuaRules* or CLuaUI*,
	// but the AI call-in is always unsynced!
	if (!cmdStr.GetGlobalFunc(L))
		return nullptr;

	lua_pushnumber(L, aiTeam);

	int argCount = 1;
	const char* outData = nullptr;

	if (inData != nullptr) {
		if (inSize < 0)
			inSize = strlen(inData);

		lua_pushlstring(L, inData, inSize);
		argCount = 2;
	}

	if (!RunCallIn(L, cmdStr, argCount, 1))
		return nullptr;

	if (lua_isstring(L, -1))
		outData = lua_tolstring(L, -1, outSize);

	lua_pop(L, 1);
	return outData;
}

void CLuaHandle::DownloadQueued(int ID, const string& archiveName, const string& archiveType)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushsstring(L, archiveName);
	lua_pushsstring(L, archiveType);

	// call the routine
	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::DownloadStarted(int ID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 1, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::DownloadFinished(int ID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 1, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 1, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::DownloadFailed(int ID, int errorID)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushinteger(L, errorID);

	// call the routine
	RunCallInTraceback(L, cmdStr, 2, 0, traceBack.GetErrFuncIdx(), false);
}

void CLuaHandle::DownloadProgress(int ID, long downloaded, long total)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 3, __func__);

	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushinteger(L, ID);
	lua_pushnumber(L, downloaded);
	lua_pushnumber(L, total);

	// call the routine
	RunCallInTraceback(L, cmdStr, 3, 0, traceBack.GetErrFuncIdx(), false);
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CollectGarbage(bool forced)
{
	const float gcMemLoadMult = D.gcCtrl.baseMemLoadMult;
	const float gcRunTimeMult = D.gcCtrl.baseRunTimeMult;

	if (!forced && spring_lua_alloc_skip_gc(gcMemLoadMult))
		return;

	LUA_CALL_IN_CHECK_NAMED(L, (GetLuaContextData(L)->synced)? "Lua::CollectGarbage::Synced": "Lua::CollectGarbage::Unsynced");

	lua_lock(L_GC);
	SetHandleRunning(L_GC, true);

	// note: total footprint INCLUDING garbage, in KB
	int  gcMemFootPrint = lua_gc(L_GC, LUA_GCCOUNT, 0);
	int  gcItersInBatch = 0;
	int& gcStepsPerIter = D.gcCtrl.numStepsPerIter;

	// if gc runs at a fixed rate, the upper limit to base runtime will
	// quickly be reached since Lua's footprint can easily exceed 100MB
	// and OOM exceptions become a concern when catching up
	// OTOH if gc is tied to sim-speed the increased number of calls can
	// mean too much time is spent on it, must weigh the per-call period
	const float gcSpeedFactor = Clamp(gs->speedFactor * (1 - gs->PreSimFrame()) * (1 - gs->paused), 1.0f, 50.0f);
	const float gcBaseRunTime = smoothstep(10.0f, 100.0f, gcMemFootPrint / 1024);
	const float gcLoopRunTime = Clamp((gcBaseRunTime * gcRunTimeMult) / gcSpeedFactor, D.gcCtrl.minLoopRunTime, D.gcCtrl.maxLoopRunTime);

	const spring_time startTime = spring_gettime();
	const spring_time   endTime = startTime + spring_msecs(gcLoopRunTime);

	// perform GC cycles until time runs out or iteration-limit is reached
	while (forced || (gcItersInBatch < D.gcCtrl.itersPerBatch && spring_gettime() < endTime)) {
		gcItersInBatch++;

		if (!lua_gc(L_GC, LUA_GCSTEP, gcStepsPerIter))
			continue;

		// garbage-collection cycle finished
		const int gcMemFootPrintNow = lua_gc(L_GC, LUA_GCCOUNT, 0);
		const int gcMemFootPrintDif = gcMemFootPrintNow - gcMemFootPrint;

		gcMemFootPrint = gcMemFootPrintNow;

		// early-exit if cycle didn't free any memory
		if (gcMemFootPrintDif == 0)
			break;
	}

	// don't collect garbage outside of CollectGarbage
	lua_gc(L_GC, LUA_GCSTOP, 0);
	SetHandleRunning(L_GC, false);
	lua_unlock(L_GC);


	const spring_time finishTime = spring_gettime();

	if (gcStepsPerIter > 1 && gcItersInBatch > 0) {
		// runtime optimize number of steps to process in a batch
		const float avgLoopIterTime = (finishTime - startTime).toMilliSecsf() / gcItersInBatch;

		gcStepsPerIter -= (avgLoopIterTime > (gcRunTimeMult * 0.150f));
		gcStepsPerIter += (avgLoopIterTime < (gcRunTimeMult * 0.075f));
		gcStepsPerIter  = Clamp(gcStepsPerIter, D.gcCtrl.minStepsPerIter, D.gcCtrl.maxStepsPerIter);
	}

	eventHandler.DbgTimingInfo(TIMING_GC, startTime, finishTime);
}

/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::AddBasicCalls(lua_State* L)
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
		HSTR_PUSH_CFUNC(L, "IsEngineMinVersion", CallOutIsEngineMinVersion);
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


int CLuaHandle::CallOutIsEngineMinVersion(lua_State* L)
{
	return (LuaUtils::IsEngineMinVersion(L));
}


int CLuaHandle::CallOutGetCallInList(lua_State* L)
{
	std::vector<std::string> eventList;
	eventHandler.GetEventList(eventList);
	lua_createtable(L, 0, eventList.size());
	for (const auto& event : eventList) {
		lua_pushsstring(L, event);
		lua_newtable(L); {
			lua_pushliteral(L, "unsynced");
			lua_pushboolean(L, eventHandler.IsUnsynced(event));
			lua_rawset(L, -3);
			lua_pushliteral(L, "controller");
			lua_pushboolean(L, eventHandler.IsController(event));
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
