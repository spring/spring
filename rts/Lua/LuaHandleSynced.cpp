#include "StdAfx.h"
// LuaHandleSynced.cpp: implementation of the CLuaHandleSynced class.
//
//////////////////////////////////////////////////////////////////////

#include <set>
#include <cctype>

#include "mmgr.h"

#include "LuaHandleSynced.h"

#include "LuaInclude.h"

#include "LuaCallInCheck.h"
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
#include "LuaScream.h"
#include "LuaOpenGL.h"
#include "LuaVFS.h"

#include "Game/Game.h"
#include "Game/WordCompletion.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/LuaUnitScript.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "EventHandler.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"


static const LuaHashString unsyncedStr("UNSYNCED");


#if (LUA_VERSION_NUM < 500)
#  define LUA_OPEN_LIB(L, lib) lib(L)
#else
#  define LUA_OPEN_LIB(L, lib) \
     lua_pushcfunction((L), lib); \
     lua_pcall((L), 0, 0, 0);
#endif


/******************************************************************************/
/******************************************************************************/

CLuaHandleSynced::CLuaHandleSynced(const string& _name, int _order, const string& msgPrefix)
: CLuaHandle(_name, _order, false),
  messagePrefix(msgPrefix),
  allowChanges(false),
  allowUnsafeChanges(false),
  teamsLocked(false)
{
	printTracebacks = true;
}


CLuaHandleSynced::~CLuaHandleSynced()
{
	// kill all unitscripts running in this handle
	CLuaUnitScript::HandleFreed(this);
}


/******************************************************************************/

void CLuaHandleSynced::Init(const string& syncedFile,
                            const string& unsyncedFile,
                            const string& modes)
{
	if (L == NULL) {
		return;
	}

	if (fullCtrl) {
		// numWeaponDefs has an extra slot
		for (int w = 0; w <= weaponDefHandler->numWeaponDefs; w++) {
			watchWeapons.push_back(false);
		}
	}

	const string syncedCode   = LoadFile(syncedFile, modes);
	const string unsyncedCode = LoadFile(unsyncedFile, modes);
	if (syncedCode.empty() && unsyncedCode.empty()) {
		KillLua();
		return;
	}

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	//LUA_OPEN_LIB(L, luaopen_io);
	//LUA_OPEN_LIB(L, luaopen_os);
	//LUA_OPEN_LIB(L, luaopen_package);
	//LUA_OPEN_LIB(L, luaopen_debug);

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

	synced = true;

	const bool haveSynced = SetupSynced(syncedCode, syncedFile);
	if (L == NULL) {
		SetActiveHandle(origHandle);
		return;
	}

	synced = false;

	const bool haveUnsynced = SetupUnsynced(unsyncedCode, unsyncedFile);
	if (L == NULL) {
		SetActiveHandle(origHandle);
		return;
	}

	synced = true;

	if (!haveSynced && !haveUnsynced) {
		KillLua();
		SetActiveHandle(origHandle);
		return;
	}

	// register for call-ins
	eventHandler.AddClient(this);

	SetActiveHandle(origHandle);
}


bool CLuaHandleSynced::SetupSynced(const string& code, const string& filename)
{
	if ((L == NULL) || code.empty()) {
		return false;
	}

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(); // into Global

	lua_pushstring(L, "Script");
	lua_rawget(L, -2);
	LuaPushNamedCFunc(L, "AddActionFallback",    AddSyncedActionFallback);
	LuaPushNamedCFunc(L, "RemoveActionFallback", RemoveSyncedActionFallback);
	LuaPushNamedCFunc(L, "UpdateCallIn",         CallOutSyncedUpdateCallIn);
	LuaPushNamedCFunc(L, "GetWatchWeapon",       GetWatchWeapon);
	LuaPushNamedCFunc(L, "SetWatchWeapon",       SetWatchWeapon);
	lua_pop(L, 1);

	// add the custom file loader
	LuaPushNamedCFunc(L, "SendToUnsynced", SendToUnsynced);

	LuaPushNamedCFunc(L, "loadstring", LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam", CallAsTeam);

	LuaPushNamedCFunc(L, "AllowUnsafeChanges", AllowUnsafeChanges);

	LuaPushNamedNumber(L, "COBSCALE", COBSCALE);

	// load our libraries  (LuaSyncedCtrl overrides some LuaUnsyncedCtrl entries)
	if (!AddEntriesToTable(L, "VFS",         LuaVFS::PushSynced)           ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaSyncedCall::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedCtrl::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
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
	LuaPushNamedCFunc(L, "UpdateCallIn", CallOutUnsyncedUpdateCallIn);
	lua_pop(L, 1);

	lua_pushstring(L, "_G");
	unsyncedStr.GetRegistry(L);
	lua_rawset(L, -3);

	LuaPushNamedCFunc(L, "loadstring",   LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam",   CallAsTeam);

	// load our libraries
	if (!LuaSyncedTable::PushEntries(L)                                    ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaUnsyncedCall::PushEntries) ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)       ||
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
	    !SetupUnsyncedFunction("DrawGenesis")         ||
	    !SetupUnsyncedFunction("DrawWorld")           ||
	    !SetupUnsyncedFunction("DrawWorldPreUnit")    ||
	    !SetupUnsyncedFunction("DrawWorldShadow")     ||
	    !SetupUnsyncedFunction("DrawWorldReflection") ||
	    !SetupUnsyncedFunction("DrawWorldRefraction") ||
	    !SetupUnsyncedFunction("DrawScreenEffects")   ||
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
//FIXME		logOutput.Print("ERROR: missing UNSYNCED table for %s", name.c_str());
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
		logOutput.Print("%s in %s is not a function", funcName, GetName().c_str());
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
		if (!lua_israwstring(L, -2)) { // key must be a string
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

string CLuaHandleSynced::LoadFile(const string& filename,
                                  const string& modes) const
{
	string vfsModes;
	if (devMode) {
		vfsModes = SPRING_VFS_RAW;
	}
	vfsModes += modes;
	CFileHandler f(filename, vfsModes);
	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


bool CLuaHandleSynced::HasCallIn(const string& name)
{
	if (L == NULL) {
		return false;
	}

	int tableIndex;
	if ((name != "DrawUnit") &&
	    (name != "AICallIn") &&
	    (name != "RecvFromSynced") &&
	    !eventHandler.IsUnsynced(name)) {
		tableIndex = LUA_GLOBALSINDEX;  // synced call-ins in GLOBAL
	} else {
		tableIndex = LUA_REGISTRYINDEX; // unsynced call-ins in REGISTRY
	}

	bool haveFunc = true;
	lua_settop(L, 0);
	lua_pushstring(L, name.c_str());
	lua_gettable(L, tableIndex);
	if (!lua_isfunction(L, -1)) {
		haveFunc = false;
	}
	lua_settop(L, 0);

	return haveFunc;
}


bool CLuaHandleSynced::SyncedUpdateCallIn(const string& name)
{
	if ((name == "RecvFromSynced") ||
	    eventHandler.IsUnsynced(name)) {
		return false;
	}
	if (HasCallIn(name)) {
		eventHandler.InsertEvent(this, name);
	} else {
		eventHandler.RemoveEvent(this, name);
	}
	return true;
}


bool CLuaHandleSynced::UnsyncedUpdateCallIn(const string& name)
{
	if ((name != "RecvFromSynced") &&
	    !eventHandler.IsUnsynced(name)) {
		  return false;
	}
	if (name != "RecvFromSynced") {
		if (HasCallIn(name)) {
			eventHandler.InsertEvent(this, name);
		} else {
			eventHandler.RemoveEvent(this, name);
		}
	}
	SetupUnsyncedFunction(name.c_str());
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Call-ins
//

bool CLuaHandleSynced::Initialize(const string& syncData)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("Initialize");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true;
	}

	int errfunc = SetupTraceback() ? -2 : 0;
	logOutput.Print("Initialize errfunc=%d\n", errfunc);

	lua_pushlstring(L, syncData.c_str(), syncData.size());

	// call the routine
	if (!RunCallInTraceback(cmdStr, 1, 1, errfunc)) {
		return false;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return true;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


string CLuaHandleSynced::GetSyncData()
{
	string syncData;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("GetSyncData");
	if (!cmdStr.GetGlobalFunc(L)) {
		return syncData;
	}

	// call the routine
	if (!RunCallIn(cmdStr, 0, 1)) {
		return syncData;
	}

	// get the result
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return syncData;
	}
	const int len = lua_strlen(L, -1);
	const char* c = lua_tostring(L, -1);
	syncData.resize(len);
	for (int i = 0; i < len; i++) {
		syncData[i] = c[i];
	}
	lua_pop(L, 1);

	return syncData;
}


void CLuaHandleSynced::GameFrame(int frameNumber)
{
	if (killMe) {
		string msg = GetName();
		if (!killMsg.empty()) {
			msg += ": " + killMsg;
		}
		logOutput.Print("Disabled %s\n", msg.c_str());
		delete this;
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr("GameFrame");
	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) lua_pop(L, 1);
		return;
	}

	lua_pushnumber(L, frameNumber); // 6 day roll-over

	// call the routine
	allowChanges = true;
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
	allowChanges = allowUnsafeChanges;

	return;
}


bool CLuaHandleSynced::SyncedActionFallback(const string& msg, int playerID)
{
	string cmd = msg;
	const string::size_type pos = cmd.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}
	if (textCommands.find(cmd) == textCommands.end()) {
		return false;
	}
	GotChatMsg(msg.substr(1), playerID); // strip the '.'
	return true;
}


bool CLuaHandleSynced::GotChatMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("GotChatMsg");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushstring(L, msg.c_str());
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
	//LUA_CALL_IN_CHECK(L); -- not valid here
	static const LuaHashString cmdStr("RecvFromSynced");
	if (!cmdStr.GetRegistryFunc(L)) {
		return; // the call is not defined
	}
	lua_insert(L, 1); // place the function

	// call the routine
	const bool prevAllowChanges = allowChanges;
	allowChanges = false;
	synced = false;

	RunCallIn(cmdStr, args, 0);

	synced = true;
	allowChanges = prevAllowChanges;

	return;
}


bool CLuaHandleSynced::RecvLuaMsg(const string& msg, int playerID)
{
	const bool prevAllowChanges = allowChanges;
	allowChanges = false;

	const bool retval = CLuaHandle::RecvLuaMsg(msg, playerID);

	allowChanges = prevAllowChanges;

	return retval;
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
	const bool haveFunc = lua_isfunction(L, -1);
	lua_pop(L, 2);
	return haveFunc;
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
	const bool haveFunc = lua_isfunction(L, -1);
	lua_pop(L, 2);
	return haveFunc;
}


int CLuaHandleSynced::XCall(lua_State* srcState, const string& funcName)
{
	// expecting an environment table
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}

	// push the function
	const LuaHashString funcHash(funcName);
	funcHash.Push(L);  // push the function name
	lua_rawget(L, -2); // get the function
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 0;
	}
	lua_remove(L, -2);

	const int top = lua_gettop(L) - 1; // do not count the function

	const bool diffStates = (srcState != L);

	int retCount;

	if (!diffStates) {
		lua_insert(L, 1); // move the function to the beginning
		// call the function
		if (!RunCallIn(funcHash, top, LUA_MULTRET)) {
			return 0;
		}
		retCount = lua_gettop(L);
	}
	else {
		const int srcCount = lua_gettop(srcState);

		LuaUtils::CopyData(L, srcState, srcCount);

		// call the function
		if (!RunCallIn(funcHash, srcCount, LUA_MULTRET)) {
			return 0;
		}
		retCount = lua_gettop(L) - top;

		lua_settop(srcState, 0);
		if (retCount > 0) {
			LuaUtils::CopyData(srcState, L, retCount);
		}
		lua_settop(L, top);
	}

	return retCount;
}


int CLuaHandleSynced::SyncedXCall(lua_State* srcState, const string& funcName)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	const int retval = XCall(srcState, funcName);
	return retval;
}


int CLuaHandleSynced::UnsyncedXCall(lua_State* srcState, const string& funcName)
{
	const bool prevSynced = synced;
	synced = false;
	unsyncedStr.GetRegistry(L); // push the UNSYNCED table
	const int retval = XCall(srcState, funcName);
	synced = prevSynced;
	return retval;
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandleSynced::SyncedRandom(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 0) {
		lua_pushnumber(L, gs->randFloat());
	}
	else if ((args == 1) && lua_isnumber(L, 1)) {
		const int maxn = lua_toint(L, 1);
		if (maxn < 1) {
			luaL_error(L, "error: too small upper limit (%d) given to math.random(), should be >= 1 {synced}", maxn);
		}
		lua_pushnumber(L, 1 + (gs->randInt() % maxn));
	}
	else if ((args == 2) && lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
		const int lower = lua_toint(L, 1);
		const int upper = lua_toint(L, 2);
		if (lower >= upper) {
			luaL_error(L, "Empty interval in math.random() {synced}");
		}
		const float diff  = (upper - lower);
		const float r = gs->randFloat(); // [0,1], not [0,1) ?
		int value = lower + (int)(r * (diff + 1));
		value = std::max(lower, std::min(upper, value));
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
	if (lua_istable(L, 3)) {
		lua_pushvalue(L, 3);
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __FUNCTION__);
	}
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
		if (!lua_isnil(L, i)    &&
		    !lua_isnumber(L, i) &&
		    !lua_isstring(L, i) &&
		    !lua_isboolean(L, i)) {
			luaL_error(L, "Incorrect data type for SendToUnsynced(), arg %d", i);
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
		const int teamID = lua_toint(L, 1);
		if ((teamID < MinSpecialTeam) || (teamID >= teamHandler->ActiveTeams())) {
			luaL_error(L, "Bad teamID in SetCtrlTeam");
		}
		// ctrl
		lhs->ctrlTeam = teamID;
		lhs->fullCtrl = (lhs->ctrlTeam == CEventClient::AllAccessTeam);
		// read
		lhs->readTeam = teamID;
		lhs->readAllyTeam = (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID);
		lhs->fullRead = (lhs->readAllyTeam == CEventClient::AllAccessTeam);
		activeFullRead     = lhs->fullRead;
		activeReadAllyTeam = lhs->readAllyTeam;
		// select
		lhs->selectTeam = teamID;
	}
	else if (lua_istable(L, 1)) {
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
				continue;
			}
			const string key = lua_tostring(L, -2);
			const int teamID = lua_toint(L, -1);
			if ((teamID < MinSpecialTeam) || (teamID >= teamHandler->ActiveTeams())) {
				luaL_error(L, "Bad teamID in SetCtrlTeam");
			}

			if (key == "ctrl") {
				lhs->ctrlTeam = teamID;
				lhs->fullCtrl = (lhs->ctrlTeam == CEventClient::AllAccessTeam);
			}
			else if (key == "read") {
				lhs->readTeam = teamID;
				lhs->readAllyTeam = (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID);
				lhs->fullRead = (lhs->readAllyTeam == CEventClient::AllAccessTeam);
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


int CLuaHandleSynced::AddSyncedActionFallback(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 2) || !lua_isstring(L, 1) ||  !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to AddActionFallback()");
	}
	string cmdRaw  = lua_tostring(L, 1);
	cmdRaw = "/" + cmdRaw;

	string cmd = cmdRaw;
	const string::size_type pos = cmdRaw.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	CLuaHandleSynced* lhs = GetActiveHandle();
	lhs->textCommands[cmd] = lua_tostring(L, 2);
	game->wordCompletion->AddWord(cmdRaw, true, false, false);
	lua_pushboolean(L, true);
	return 1;
}


int CLuaHandleSynced::RemoveSyncedActionFallback(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to RemoveActionFallback()");
	}
	string cmdRaw  = lua_tostring(L, 1);
	cmdRaw = "/" + cmdRaw;

	string cmd = cmdRaw;
	const string::size_type pos = cmdRaw.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	CLuaHandleSynced* lhs = GetActiveHandle();

	map<string, string>::iterator it = lhs->textCommands.find(cmd);
	if (it != lhs->textCommands.end()) {
		lhs->textCommands.erase(it);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	// FIXME -- word completion should also be removed
	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/

int CLuaHandleSynced::GetWatchWeapon(lua_State* L)
{
	CLuaHandleSynced* lhs = GetActiveHandle();
	const int weaponID = luaL_checkint(L, 1);
	if ((weaponID < 0) || (weaponID >= (int)lhs->watchWeapons.size())) {
		return 0;
	}
	lua_pushboolean(L, lhs->watchWeapons[weaponID]);
	return 1;
}


int CLuaHandleSynced::SetWatchWeapon(lua_State* L)
{
	CLuaHandleSynced* lhs = GetActiveHandle();
	const int weaponID = luaL_checkint(L, 1);
	if ((weaponID < 0) || (weaponID >= (int)lhs->watchWeapons.size())) {
		return 0;
	}
	if (!lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetWatchWeapon()");
	}
	lhs->watchWeapons[weaponID] = lua_toboolean(L, 2);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
