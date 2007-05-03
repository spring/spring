#include "StdAfx.h"
// LuaHandleSynced.cpp: implementation of the CLuaHandleSynced class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaHandleSynced.h"
#include <set>
#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaCallInHandler.h"
#include "LuaUtils.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstGame.h"
#include "LuaSyncedCall.h"
#include "LuaSyncedCtrl.h"
#include "LuaSyncedRead.h"
#include "LuaSyncedTable.h"
#include "LuaUnsyncedCall.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaUnsyncedRead.h"
#include "LuaFeatureDefs.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaOpenGL.h"
#include "LuaVFS.h"

#include "Game/command.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/COB/CobInstance.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"


static const LuaHashString unsyncedStr("UNSYNCED");


/******************************************************************************/
/******************************************************************************/

CLuaHandleSynced::CLuaHandleSynced(const string& name, int order,
                                   LuaCobCallback callback,
                                   const string& msgPrefix)
: messagePrefix(msgPrefix),
  CLuaHandle(name, order, false, callback),
  allowChanges(false),
  allowUnsafeChanges(false),
  teamsLocked(false)
{
}


CLuaHandleSynced::~CLuaHandleSynced()
{
}


void CLuaHandleSynced::KillLua()
{
	if (L != NULL) {
		lua_close(L);
		L = NULL;
	}
}


/******************************************************************************/

string CLuaHandleSynced::LoadMapFile(const string& filename) const
{
	return LoadFile(filename);
}


string CLuaHandleSynced::LoadModFile(const string& filename) const
{
	return LoadFile(filename);
}


/******************************************************************************/

void CLuaHandleSynced::Init(const string& syncedFile,
                            const string& unsyncedFile)
{
	if (L == NULL) {
		return;
	}
	
	const string syncedCode   = LoadFile(syncedFile);
	const string unsyncedCode = LoadFile(unsyncedFile);
	if (syncedCode.empty() && unsyncedCode.empty()) {
		KillLua();
		return;
	}
	
	// load the standard libraries
	luaopen_base(L);
	luaopen_math(L);
	luaopen_table(L);
	luaopen_string(L);

	// delete some dangerous functions
	lua_pushnil(L); lua_setglobal(L, "dofile");
	lua_pushnil(L); lua_setglobal(L, "loadfile");
	lua_pushnil(L); lua_setglobal(L, "loadlib");
	lua_pushnil(L); lua_setglobal(L, "loadstring"); // replaced
	lua_pushnil(L); lua_setglobal(L, "require");
	lua_pushnil(L); lua_setglobal(L, "rawequal");
	lua_pushnil(L); lua_setglobal(L, "rawget");
	lua_pushnil(L); lua_setglobal(L, "rawset");
//	lua_pushnil(L); lua_setglobal(L, "getfenv");
//	lua_pushnil(L); lua_setglobal(L, "setfenv");
	lua_pushnil(L); lua_setglobal(L, "newproxy");
	lua_pushnil(L); lua_setglobal(L, "gcinfo");
	lua_pushnil(L); lua_setglobal(L, "collectgarbage");
	
	// use gs->randFloat() for the synchronized code, and disable randomseed()
	// (this first copies the original functions to the registry for unsynced)
	if (!SyncifyRandomFuncs()) {
		KillLua();
		return;
	}

	CLuaHandle* origHandle = activeHandle;
	SetActiveHandle();

	const bool haveSynced = SetupSynced(syncedCode, syncedFile);
	if (L == NULL) {
		SetActiveHandle(origHandle);
		return;
	}

	const bool haveUnsynced = SetupUnsynced(unsyncedCode, unsyncedFile);
	if (L == NULL) {
		SetActiveHandle(origHandle);
		return;
	}

	if (!haveSynced && !haveUnsynced) {
		KillLua();
		SetActiveHandle(origHandle);
		return;
	}

	// register for call-ins
	luaCallIns.AddHandle(this);

	SetActiveHandle(origHandle);
}


bool CLuaHandleSynced::SetupSynced(const string& code, const string& filename)
{
	if ((L == NULL) || code.empty()) {
		return false;
	}

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(); // into Global

	// add the custom file loader
	LuaPushNamedCFunc(L, "SendToUnsynced", SendToUnsynced);

	LuaPushNamedCFunc(L, "loadstring", LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam", CallAsTeam);

	LuaPushNamedCFunc(L, "AllowUnsafeChanges", AllowUnsafeChanges);

	LuaPushNamedNumber(L, "COBSCALE", COBSCALE);
	
	// load our libraries
	if (!AddEntriesToTable(L, "VFS",         LuaVFS::PushSynced)           ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaSyncedCall::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedCtrl::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries)) {
		KillLua();
		return false;
	}

	// add code from the sub-class
	if (!AddSyncedCode()) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);

	if (!LoadCode(code, filename)) {
		KillLua();
		return false;
	}

	return true;
}


bool CLuaHandleSynced::SetupUnsynced(const string& code, const string& filename)
{
	if ((L == NULL) || code.empty()) {
		return false;
	}

	// make the UNSYNCED table
	unsyncedStr.Push(L);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	unsyncedStr.GetRegistry(L);

	AddBasicCalls(); // into UNSYNCED

	// remove Script.Kill()
	lua_pushstring(L, "Script");
	lua_rawget(L, -2);
	lua_pushstring(L, "Kill");
	lua_pushnil(L);
	lua_rawset(L, -3);
	lua_pop(L, 1);

	lua_pushstring(L, "_G");
	unsyncedStr.GetRegistry(L);
	lua_rawset(L, -3);

	LuaPushNamedCFunc(L, "loadstring",   LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam",   CallAsTeam);
	LuaPushNamedCFunc(L, "UpdateCallIn", UpdateCallIn);

	// load our libraries
	if (!LuaSyncedTable::PushEntries(L)                                    ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaUnsyncedCall::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedRead::PushEntries) ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)       ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)      ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries)) {
		KillLua();
		return false;
	}

	lua_pushstring(L, "math"); lua_newtable(L);
	lua_getglobal(L, "math"); LightCopyTable(-2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	lua_pushstring(L, "table"); lua_newtable(L);
	lua_getglobal(L, "table"); LightCopyTable(-2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	lua_pushstring(L, "string"); lua_newtable(L);
	lua_getglobal(L, "string"); LightCopyTable(-2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	if (!CopyRealRandomFuncs()) {
		KillLua();
		return false;
	}
	
	lua_settop(L, 0);

	// note the absence of loadstring() -- global access
	const char* labels[] = {
		"assert",         "error",
		"print",
		"next",           "pairs",        "ipairs",
		"tonumber",       "tostring",     "type",
		"collectgarbage", "gcinfo",
		"unpack",
		"getmetatable",   "setmetatable",
		"rawequal",       "rawget",       "rawset",
		"getfenv",        "setfenv",
		"pcall",          "xpcall",
		"_VERSION",
		NULL
	};
	for (const char** l = labels; *l != NULL; l++) {
		CopyGlobalToUnsynced(*l);
	}

	// add code from the sub-class
	unsyncedStr.GetRegistry(L);
	if (!AddUnsyncedCode()) {
		KillLua();
		return false;
	}
	lua_settop(L, 0);

	if (!LoadUnsyncedCode(code, filename)) {
		KillLua();
		return false;
	}

	if (!SetupUnsyncedFunction("RecvFromSynced")      ||
	    !SetupUnsyncedFunction("Update")              ||
	    !SetupUnsyncedFunction("DrawWorld")           ||
	    !SetupUnsyncedFunction("DrawWorldShadow")     ||
	    !SetupUnsyncedFunction("DrawWorldReflection") ||
	    !SetupUnsyncedFunction("DrawWorldRefraction") ||
	    !SetupUnsyncedFunction("DrawScreen")          ||
	    !SetupUnsyncedFunction("DrawInMiniMap")) {
		return false;
	}

	return true;
}


bool CLuaHandleSynced::SyncifyRandomFuncs()
{
	// adjust the math.random() and math.randomseed() calls
	lua_getglobal(L, "math");
	if (!lua_istable(L, -1)) {
		logOutput.Print("lua.math does not exist\n");
		return false;
	}

	// copy the original random() into the registry
	lua_pushstring(L, "random");
	lua_pushstring(L, "random");
	lua_rawget(L, -3); // math table
	lua_rawset(L, LUA_REGISTRYINDEX);
	
	// copy the original randomseed() into the registry
	lua_pushstring(L, "randomseed");
	lua_pushstring(L, "randomseed");
	lua_rawget(L, -3); // math table
	lua_rawset(L, LUA_REGISTRYINDEX);
	
	// install our custom random()
	lua_pushstring(L, "random");
	lua_pushcfunction(L, SyncedRandom);
	lua_rawset(L, -3);

	// remove randomseed()
	lua_pushstring(L, "randomseed");
	lua_pushnil(L);
	lua_rawset(L, -3);

	lua_pop(L, 1); // pop the global math table

	return true;
}


bool CLuaHandleSynced::CopyRealRandomFuncs()
{
	lua_pushstring(L, "math");
	lua_rawget(L, -2);

	lua_pushstring(L, "random");
	lua_pushstring(L, "random");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawset(L, -3);

	lua_pushstring(L, "randomseed");
	lua_pushstring(L, "randomseed");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawset(L, -3);

	lua_pop(L, 1); // remove 'math'

	return true;	
}


bool CLuaHandleSynced::SetupUnsyncedFunction(const char* funcName)
{
	// copy the function from UNSYNCED into
	// the registry, and setfenv() it to UNSYNCED
	lua_settop(L, 0);
	unsyncedStr.GetRegistry(L);
	if (!lua_istable(L, -1)) {
		lua_settop(L, 0);
		logOutput.Print("ERROR: missing UNSYNCED table for %s", name.c_str());
		return false;
	}
	const int unsynced = lua_gettop(L);

	lua_pushstring(L, funcName);

	lua_pushstring(L, funcName);
	lua_gettable(L, unsynced);
	if (lua_isnil(L, -1)) {
		lua_rawset(L, LUA_REGISTRYINDEX);
		lua_settop(L, 0);
		return true;
	}
	else if (!lua_isfunction(L, -1)) {
		lua_settop(L, 0);
		logOutput.Print("%s in %s is not a function", funcName, name.c_str());
		return false;
	}
	lua_pushvalue(L, unsynced);
	lua_setfenv(L, -2);

	lua_rawset(L, LUA_REGISTRYINDEX);

	lua_settop(L, 0);
	return true;
}


bool CLuaHandleSynced::CopyGlobalToUnsynced(const char* name)
{
	lua_settop(L, 0);
	unsyncedStr.GetRegistry(L);
	const int unsynced = lua_gettop(L);
	
	lua_pushstring(L, name);
	lua_getglobal(L, name);
	lua_rawset(L, unsynced);
	
	lua_settop(L, 0);
	return true;
}


bool CLuaHandleSynced::LightCopyTable(int dstIndex, int srcIndex)
{
	// use positive indices
	if (dstIndex < 0) { dstIndex = lua_gettop(L) + dstIndex + 1; }
	if (srcIndex < 0) { srcIndex = lua_gettop(L) + srcIndex + 1; }

	if (!lua_istable(L, dstIndex) || !lua_istable(L, srcIndex)) {
		return false;
	}

	for (lua_pushnil(L); lua_next(L, srcIndex) != 0; lua_pop(L, 1)) {
		if (!lua_isstring(L, -2)) { // key must be a string
			continue;
		}
		if (lua_isstring(L, -1)   ||
				lua_isnumber(L, -1)   ||
				lua_isboolean(L, -1)  ||
				lua_isfunction(L, -1)) {
			lua_pushvalue(L, -2); // copy the key
			lua_pushvalue(L, -2); // copy the value
			lua_rawset(L, dstIndex);
		}
	}

	return true;
}


bool CLuaHandleSynced::LoadUnsyncedCode(const string& code, const string& debug)
{
	lua_settop(L, 0);

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_settop(L, 0);
		return false;
	}

	unsyncedStr.GetRegistry(L);
	if (!lua_istable(L, -1)) {
		lua_settop(L, 0);
		return false;
	}
	lua_setfenv(L, -2);
	
	CLuaHandle* orig = activeHandle;
	SetActiveHandle();
	error = lua_pcall(L, 0, 0, 0);
	SetActiveHandle(orig);

	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_settop(L, 0);
		return false;
	}

	return true;
}


/******************************************************************************/

string CLuaHandleSynced::LoadFile(const string& filename) const
{
	CFileHandler::VFSmode vfsMode;
	if (devMode) {
		vfsMode = CFileHandler::AnyFS;
	} else {
		vfsMode = CFileHandler::OnlyArchiveFS;
	}
	CFileHandler f(filename, vfsMode);
	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


bool CLuaHandleSynced::HasCallIn(const string& callInName)
{
	if (L == NULL) {
		return false;
	}

	int tableIndex;
	if ((callInName != "Update") &&
	    (callInName.find("Draw") != 0)) {
		tableIndex = LUA_GLOBALSINDEX;  // synced call-ins in GLOBAL
	} else {
		tableIndex = LUA_REGISTRYINDEX; // unsynced call-ins in REGISTRY
	}

	bool haveFunc = true;
	lua_settop(L, 0);
	lua_pushstring(L, callInName.c_str());
	lua_gettable(L, tableIndex);
	if (!lua_isfunction(L, -1)) {
		haveFunc = false;
	}
	lua_settop(L, 0);

	return haveFunc;
}


/******************************************************************************/

bool CLuaHandleSynced::Initialize(const string& syncData)
{
	lua_settop(L, 0);

	static const LuaHashString cmdStr("Initialize");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return true;
	}

	lua_pushlstring(L, syncData.c_str(), syncData.size());
	
	// call the routine
	if (!RunCallIn(cmdStr, 1, 1)) {
		return false;
	}

	// get the results
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isboolean(L, -1)) {
		lua_pop(L, args);
		return true;
	}

	return lua_toboolean(L, -1);
}


string CLuaHandleSynced::GetSyncData()
{
	string syncData;
	
	lua_settop(L, 0);

	static const LuaHashString cmdStr("GetSyncData");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return syncData;
	}

	// call the routine
	if (!RunCallIn(cmdStr, 0, 1)) {
		return syncData;
	}

	// get the result
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pop(L, args);
		return syncData;
	}
	const int len = lua_strlen(L, 1);
	const char* c = lua_tostring(L, 1);
	syncData.resize(len);
	for (int i = 0; i < len; i++) {
		syncData[i] = c[i];
	}

	return syncData;
}


void CLuaHandleSynced::SendCallbacks()
{
	const int count = (int)cobCallbackEntries.size();
	for (int cb = 0; cb < count; cb++) {
		static const LuaHashString cmdStr("CobCallback");
		if (!cmdStr.GetGlobalFunc(L)) {
			lua_settop(L, 0);
			return;
		}
		const CobCallbackData& cbd = cobCallbackEntries[cb];
		lua_pushnumber(L, cbd.retCode);
		lua_pushnumber(L, cbd.unitID);
		lua_pushnumber(L, cbd.floatData);
		// call the routine
		RunCallIn(cmdStr, 3, 0);
		lua_settop(L, 0);
	}
	cobCallbackEntries.clear();
}


void CLuaHandleSynced::GameFrame(int frameNumber)
{
	if (killMe) {
		string msg = name;
		if (!killMsg.empty()) {
			msg += ": " + killMsg;
		}
		logOutput.Print("Disabled %s\n", msg.c_str());
		delete this;
		return;
	}

	lua_settop(L, 0);
	static const LuaHashString cmdStr("GameFrame");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return;
	}


	lua_pushnumber(L, frameNumber); // 6 day roll-over
	
	// call the routine
	allowChanges = true;
	RunCallIn(cmdStr, 1, 0);
	allowChanges = allowUnsafeChanges;

	return;
}


bool CLuaHandleSynced::GotChatMsg(const string& msg, int playerID)
{
	// is the message for this script?
	if (msg.find(messagePrefix) != 0) {
		return true;
	}
	string text = msg.substr(messagePrefix.size());
	
	lua_settop(L, 0);

	static const LuaHashString cmdStr("GotChatMsg");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return true; // the call is not defined
	}

	lua_pushstring(L, text.c_str());
	lua_pushnumber(L, playerID);

	// call the routine
	allowChanges = true;
	RunCallIn(cmdStr, 2, 0);
	allowChanges = allowUnsafeChanges;

	return true;
}


/******************************************************************************/

void CLuaHandleSynced::RecvFromSynced(int args)
{
	static const LuaHashString cmdStr("RecvFromSynced");
	if (!cmdStr.GetRegistryFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}
	lua_insert(L, 1); // place the function

	// call the routine
	const bool prevAllowChanges = allowChanges;
	allowChanges = false;
	RunCallIn(cmdStr, args, 0);
	allowChanges = prevAllowChanges;

	return;
}


/******************************************************************************/
/******************************************************************************/
//
// Custom Call-in
//

bool CLuaHandleSynced::HasSyncedXCall(const string& funcName)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushstring(L, funcName.c_str()); // push the function name
	lua_rawget(L, -2);                   // get the function
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return false;
	}
	lua_pop(L, 2);
	return true;	
}


bool CLuaHandleSynced::HasUnsyncedXCall(const string& funcName)
{
	unsyncedStr.GetRegistry(L); // push the UNSYNCED table
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushstring(L, funcName.c_str()); // push the function name
	lua_rawget(L, -2);                   // get the function
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return false;
	}
	lua_pop(L, 2);
	return true;	
}


int CLuaHandleSynced::SyncedXCall(lua_State* srcState, const string& funcName)
{
	const bool diffStates = (srcState != L);
	const int argCount = lua_gettop(srcState);
	const LuaHashString cmdStr(funcName);

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, -1)) {
		lua_settop(L, 0);
		return 0;
	}
	cmdStr.Push(L);             // push the function name
	lua_rawget(L, -2);          // get the function
	if (!lua_isfunction(L, -1)) {
		lua_settop(L, 0);
		return 0;
	}
	lua_insert(L, 1); // move the function to the beginning
	lua_pop(L, 1);    // pop the GLOBAL table

	if (diffStates) {
		LuaUtils::CopyData(L, srcState, argCount);
	}

	// call the function
	if (!RunCallIn(cmdStr, argCount, 1)) {
		return 0;
	}

	const int retCount = lua_gettop(L);
	if ((retCount > 0) && diffStates) {
		lua_settop(srcState, 0);
		LuaUtils::CopyData(srcState, L, retCount);
	}

	return retCount;
}


int CLuaHandleSynced::UnsyncedXCall(lua_State* srcState, const string& funcName)
{
	const bool diffStates = (srcState != L);
	const int argCount = lua_gettop(srcState);
	const LuaHashString cmdStr(funcName);

	unsyncedStr.GetRegistry(L); // push the UNSYNCED table
	if (!lua_istable(L, -1)) {
		lua_settop(L, 0);
		return 0;
	}
	cmdStr.Push(L);             // push the function name
	lua_rawget(L, -2);          // get the function
	if (!lua_isfunction(L, -1)) {
		lua_settop(L, 0);
		return 0;
	}
	lua_insert(L, 1); // move the function to the beginning
	lua_pop(L, 1);    // pop the UNSYNCED table

	if (diffStates) {
		LuaUtils::CopyData(L, srcState, argCount);
	}

	// call the function
	if (!RunCallIn(cmdStr, argCount, 1)) {
		return 0;
	}

	const int retCount = lua_gettop(L);
	if ((retCount > 0) && diffStates) {
		lua_settop(srcState, 0);
		LuaUtils::CopyData(srcState, L, retCount);
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/

static void PushCurrentFunc(lua_State* L, const char* caller)
{
	// get the current function
	lua_Debug ar;
	if (lua_getstack(L, 1, &ar) == 0) {
		luaL_error(L, "%s() lua_getstack() error", caller);
	}
	if (lua_getinfo(L, "f", &ar) == 0) {
		luaL_error(L, "%s() lua_getinfo() error", caller);
	}
	if (!lua_isfunction(L, -1)) {
		luaL_error(L, "%s() invalid current function", caller);
	}
}


static void PushFunctionEnv(lua_State* L, const char* caller, int funcIndex)
{
	lua_getfenv(L, funcIndex);
	lua_pushliteral(L, "__fenv");
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1); // there is no fenv proxy
	} else {
		lua_remove(L, -2); // remove the orig table, leave the proxy
	}

	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s() invalid fenv", caller);
	}
}


static void PushCurrentFuncEnv(lua_State* L, const char* caller)
{
	PushCurrentFunc(L, caller);
	PushFunctionEnv(L, caller, -1);
	lua_remove(L, -2); // remove the function
}


/******************************************************************************/

int CLuaHandleSynced::SyncedRandom(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 0) {
		lua_pushnumber(L, gs->randFloat());
	}
	else if ((args == 1) && lua_isnumber(L, 1)) {
		const int maxn = (int)lua_tonumber(L, 1);
		if (maxn < 1) {
			luaL_error(L, "small integer(%i) given to math.random() {synced}", maxn);
		}
		lua_pushnumber(L, 1 + (gs->randInt() % maxn));
	}
	else if ((args == 2) && lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
		const int lower = (int)lua_tonumber(L, 1);
		const int upper = (int)lua_tonumber(L, 2);
		if (lower >= upper) {
			luaL_error(L, "Empty interval in math.random() {synced}");
		}
		const float diff  = (upper - lower);
		const float r = gs->randFloat(); // [0,1], not [0,1) ?
		int value = lower + (int)(r * (diff + 1));
		value = max(lower, min(upper, value));
		lua_pushnumber(L, value);
	}
	else {
		luaL_error(L, "Incorrect arguments to math.random() {synced}");
	}
	return 1;
}


int CLuaHandleSynced::LoadStringData(lua_State* L)
{
	size_t len;
	const char *str    = luaL_checklstring(L, 1, &len);
	const char *chunkname = luaL_optstring(L, 2, str);
	int status = luaL_loadbuffer(L, str, len, chunkname);
	if (status != 0) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2; // nil, then the error message
	}
	// set the chunk's fenv to the current fenv
	PushCurrentFuncEnv(L, __FUNCTION__);
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "loadstring(): error with setfenv");
	}
	return 1;
}


int CLuaHandleSynced::SendToUnsynced(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args <= 0) {
		luaL_error(L, "Incorrect arguments to SendToUnsynced()");
	}
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i) &&
		    !lua_isstring(L, i) &&
		    !lua_isboolean(L, i)) {
			luaL_error(L, "Incorrect data type for SendToUnsynced(), arg %i", i);
		}
	}
	CLuaHandleSynced* lhs = GetActiveHandle();
	lhs->RecvFromSynced(args);
	return 0;
}


int CLuaHandleSynced::CallAsTeam(lua_State* L)
{
	CLuaHandleSynced* lhs = GetActiveHandle();
	if (lhs->teamsLocked) {
		luaL_error(L, "CallAsTeam() called when teams are locked");
	}
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// save the current access
	const bool prevFullCtrl    = lhs->fullCtrl;
	const bool prevFullRead    = lhs->fullRead;
	const int prevCtrlTeam     = lhs->ctrlTeam;
	const int prevReadTeam     = lhs->readTeam;
	const int prevReadAllyTeam = lhs->readAllyTeam;
	const int prevSelectTeam   = lhs->selectTeam;

	// parse the new access
	if (lua_isnumber(L, 1)) {
		const int teamID = (int)lua_tonumber(L, 1);
		if ((teamID < MinSpecialTeam) || (teamID >= gs->activeTeams)) {
			luaL_error(L, "Bad teamID in SetCtrlTeam");
		}
		// ctrl
		lhs->ctrlTeam = teamID;
		lhs->fullCtrl = (lhs->ctrlTeam == AllAccessTeam);
		// read
		lhs->readTeam = teamID;
		lhs->readAllyTeam = (teamID < 0) ? teamID : gs->AllyTeam(teamID);
		lhs->fullRead = (lhs->readAllyTeam == AllAccessTeam);
		activeFullRead     = lhs->fullRead;
		activeReadAllyTeam = lhs->readAllyTeam;
		// select
		lhs->selectTeam = teamID;
	}
	else if (lua_istable(L, 1)) {
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_isstring(L, -2) || !lua_isnumber(L, -1)) {
				continue;
			}
			const string key = lua_tostring(L, -2);
			const int teamID = (int)lua_tonumber(L, -1);
			if ((teamID < MinSpecialTeam) || (teamID >= gs->activeTeams)) {
				luaL_error(L, "Bad teamID in SetCtrlTeam");
			}

			if (key == "ctrl") {
				lhs->ctrlTeam = teamID;
				lhs->fullCtrl = (lhs->ctrlTeam == AllAccessTeam);
			}
			else if (key == "read") {
				lhs->readTeam = teamID;
				lhs->readAllyTeam = (teamID < 0) ? teamID : gs->AllyTeam(teamID);
				lhs->fullRead = (lhs->readAllyTeam == AllAccessTeam);
				activeFullRead     = lhs->fullRead;
				activeReadAllyTeam = lhs->readAllyTeam;
			}
			else if (key == "select") {
				lhs->selectTeam = teamID;
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// call the function
	const int funcArgs = lua_gettop(L) - 2;

	// protected call so that the permissions are always reverted
	const int error = lua_pcall(lhs->L, funcArgs, LUA_MULTRET, 0);

	// revert the permissions
	lhs->fullCtrl      = prevFullCtrl;
	lhs->fullRead      = prevFullRead;
	lhs->ctrlTeam      = prevCtrlTeam;
	lhs->readTeam      = prevReadTeam;
	lhs->readAllyTeam  = prevReadAllyTeam;
	lhs->selectTeam    = prevSelectTeam;
	activeFullRead     = prevFullRead;
	activeReadAllyTeam = prevReadAllyTeam;

	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n",
		                error, "CallAsTeam", lua_tostring(L, -1));
		lua_error(L);
	}

	return lua_gettop(L) - 1;	// the teamID/table is still on the stack
}


int CLuaHandleSynced::UpdateCallIn(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to UpdateCallIn()");
	}
	const string funcName = lua_tostring(L, 1);
	if ((funcName != "RecvFromSynced")      &&
	    (funcName != "Update")              &&
	    (funcName != "DrawWorld")           &&
	    (funcName != "DrawWorldShadow")     &&
	    (funcName != "DrawWorldReflection") &&
	    (funcName != "DrawWorldRefraction") &&
	    (funcName != "DrawScreen")          &&
	    (funcName != "DrawInMiniMap")) {
		luaL_error(L, "UpdateCallIn() can not change %s()", funcName.c_str());
	}
	CLuaHandleSynced* lhs = GetActiveHandle();
	lhs->SetupUnsyncedFunction(funcName.c_str());
	return 0;
}	


int CLuaHandleSynced::AllowUnsafeChanges(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to AllowUnsafeChanges()");
	}
	const string magic = lua_tostring(L, 1);
	const bool value = (magic == "USE AT YOUR OWN PERIL");	
	CLuaHandleSynced* lhs = GetActiveHandle();
	lhs->allowChanges = value;
	lhs->allowUnsafeChanges = value;
	return 0;
}


/******************************************************************************/
/******************************************************************************/
