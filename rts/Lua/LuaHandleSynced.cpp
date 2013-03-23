/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <set>
#include <cctype>


#include "LuaHandleSynced.h"

#include "LuaInclude.h"

#include "LuaCallInCheck.h"
#include "LuaUtils.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstCOB.h"
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
#include "LuaZip.h"

#include "System/Config/ConfigHandler.h"
#include "Game/WordCompletion.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/LuaUnitScript.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"


static const LuaHashString unsyncedStr("UNSYNCED");

LuaRulesParams::Params  CLuaHandleSynced::gameParams;
LuaRulesParams::HashMap CLuaHandleSynced::gameParamsMap;



/******************************************************************************/
/******************************************************************************/

CLuaHandleSynced::CLuaHandleSynced(const string& _name, int _order)
: CLuaHandle(_name, _order, false),
  teamsLocked(false)
{
	UpdateThreading();
	SetAllowChanges(false, true);
}


CLuaHandleSynced::~CLuaHandleSynced()
{
	// kill all unitscripts running in this handle
	CLuaUnitScript::HandleFreed(this);

	watchUnitDefs.clear();
	watchFeatureDefs.clear();
	watchWeaponDefs.clear();
}


/******************************************************************************/


void CLuaHandleSynced::UpdateThreading() {
	int mtl = globalConfig->GetMultiThreadLua();
	useDualStates = (mtl == MT_LUA_DUAL_EXPORT || mtl == MT_LUA_DUAL || mtl == MT_LUA_DUAL_ALL || mtl == MT_LUA_DUAL_UNMANAGED);
	singleState = (mtl == MT_LUA_NONE || mtl == MT_LUA_SINGLE || mtl == MT_LUA_SINGLE_BATCH);
	copyExportTable = (mtl == MT_LUA_DUAL_EXPORT);
	useEventBatch = false;
	purgeCallsFromSyncedBatch = !singleState && (mtl != MT_LUA_DUAL_UNMANAGED);
}


void CLuaHandleSynced::Init(const string& syncedFile,
                            const string& unsyncedFile,
                            const string& modes)
{
	if (!IsValid())
		return;

	if (GetFullCtrl()) {
		watchUnitDefs.resize(unitDefHandler->unitDefs.size() + 1, false);
		watchFeatureDefs.resize(featureHandler->GetFeatureDefs().size(), false);
		watchWeaponDefs.resize(weaponDefHandler->weaponDefs.size(), false);
	}

	const string syncedCode   = LoadFile(syncedFile, modes);
	const string unsyncedCode = LoadFile(unsyncedFile, modes);
	if (syncedCode.empty() && unsyncedCode.empty()) {
		KillLua();
		return;
	}

	BEGIN_ITERATE_LUA_STATES();

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
	if (!SyncifyRandomFuncs(L)) {
		KillLua();
		return;
	}

	SetAllowChanges(true, true);
	SetSynced(true, true);

	const bool haveSynced = (SingleState() || L == L_Sim) && SetupSynced(L, syncedCode, syncedFile);
	if (!IsValid())
		return;

	SetAllowChanges(false, true);
	SetSynced(false, true);

	const bool haveUnsynced = (SingleState() || L == L_Draw) && SetupUnsynced(L, unsyncedCode, unsyncedFile);
	if (!IsValid())
		return;

	SetSynced(true, true);
	SetAllowChanges(true, true);

	if (!haveSynced && !haveUnsynced) {
		KillLua();
		return;
	}

	// register for call-ins
	eventHandler.AddClient(this);

	END_ITERATE_LUA_STATES();
}


bool CLuaHandleSynced::SetupSynced(lua_State *L, const string& code, const string& filename)
{
	if (!IsValid() || code.empty()) {
		return false;
	}

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	HSTR_PUSH(L, "EXPORT");
	lua_newtable(L);
	lua_rawset(L, -3);

	AddBasicCalls(L); // into Global

	lua_pushliteral(L, "Script");
	lua_rawget(L, -2);
	LuaPushNamedCFunc(L, "AddActionFallback",    AddSyncedActionFallback);
	LuaPushNamedCFunc(L, "RemoveActionFallback", RemoveSyncedActionFallback);
	LuaPushNamedCFunc(L, "UpdateCallIn",         CallOutSyncedUpdateCallIn);
	LuaPushNamedCFunc(L, "GetWatchUnit",         GetWatchUnitDef);
	LuaPushNamedCFunc(L, "SetWatchUnit",         SetWatchUnitDef);
	LuaPushNamedCFunc(L, "GetWatchFeature",      GetWatchFeatureDef);
	LuaPushNamedCFunc(L, "SetWatchFeature",      SetWatchFeatureDef);
	LuaPushNamedCFunc(L, "GetWatchWeapon",       GetWatchWeaponDef);
	LuaPushNamedCFunc(L, "SetWatchWeapon",       SetWatchWeaponDef);
	lua_pop(L, 1);

	// add the custom file loader
	LuaPushNamedCFunc(L, "SendToUnsynced", SendToUnsynced);

	LuaPushNamedCFunc(L, "loadstring", LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam", CallAsTeam);

	LuaPushNamedNumber(L, "COBSCALE", COBSCALE);

	// load our libraries  (LuaSyncedCtrl overrides some LuaUnsyncedCtrl entries)
	if (!AddEntriesToTable(L, "VFS",         LuaVFS::PushSynced)           ||
		!AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushSynced) ||
		!AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushSynced) ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaSyncedCall::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedCtrl::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries) ||
	    !AddEntriesToTable(L, "COB",         LuaConstCOB::PushEntries)     ||
	    !AddEntriesToTable(L, "SFX",         LuaConstSFX::PushEntries)     ||
	    !AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)
	) {
		KillLua();
		return false;
	}

	// add code from the sub-class
	if (!AddSyncedCode(L)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);

	if (!LoadCode(L, code, filename)) {
		KillLua();
		return false;
	}

	return true;
}


bool CLuaHandleSynced::SetupUnsynced(lua_State *L, const string& code, const string& filename)
{
	if (!IsValid() || code.empty()) {
		return false;
	}

	// make the UNSYNCED table
	unsyncedStr.Push(L);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	unsyncedStr.GetRegistry(L);

	AddBasicCalls(L); // into UNSYNCED

	// remove Script.Kill()
	lua_pushliteral(L, "Script");
	lua_rawget(L, -2);
	lua_pushliteral(L, "Kill");
	lua_pushnil(L);
	lua_rawset(L, -3);
	LuaPushNamedCFunc(L, "UpdateCallIn", CallOutUnsyncedUpdateCallIn);
	lua_pop(L, 1);

	lua_pushliteral(L, "_G");
	unsyncedStr.GetRegistry(L);
	lua_rawset(L, -3);

	LuaPushNamedCFunc(L, "loadstring",   LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam",   CallAsTeam);

	// load our libraries
	if (!LuaSyncedTable::PushEntries(L)                                    ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileReader::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileWriter::PushUnsynced) ||
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
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries) ||
	    !AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)
	) {
		KillLua();
		return false;
	}

	lua_pushliteral(L, "math"); lua_newtable(L);
	lua_getglobal(L, "math"); LightCopyTable(L, -2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	lua_pushliteral(L, "table"); lua_newtable(L);
	lua_getglobal(L, "table"); LightCopyTable(L, -2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	lua_pushliteral(L, "string"); lua_newtable(L);
	lua_getglobal(L, "string"); LightCopyTable(L, -2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	lua_pushliteral(L, "coroutine"); lua_newtable(L);
	lua_getglobal(L, "coroutine"); LightCopyTable(L, -2, -1); lua_pop(L, 1);
	lua_rawset(L, -3);

	if (!CopyRealRandomFuncs(L)) {
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
		"unpack",         "select",
		"getmetatable",   "setmetatable",
		"rawequal",       "rawget",       "rawset",
		"getfenv",        "setfenv",
		"pcall",          "xpcall",
		"_VERSION",
		NULL
	};
	for (const char** l = labels; *l != NULL; l++) {
		CopyGlobalToUnsynced(L, *l);
	}

	// add code from the sub-class
	unsyncedStr.GetRegistry(L);
	if (!AddUnsyncedCode(L)) {
		KillLua();
		return false;
	}
	lua_settop(L, 0);

	if (!LoadUnsyncedCode(L, code, filename)) {
		KillLua();
		return false;
	}

	if (!SetupUnsyncedFunction(L, "RecvFromSynced")      ||
	    !SetupUnsyncedFunction(L, "Update")              ||
	    !SetupUnsyncedFunction(L, "DrawGenesis")         ||
	    !SetupUnsyncedFunction(L, "DrawWorld")           ||
	    !SetupUnsyncedFunction(L, "DrawWorldPreUnit")    ||
	    !SetupUnsyncedFunction(L, "DrawWorldShadow")     ||
	    !SetupUnsyncedFunction(L, "DrawWorldReflection") ||
	    !SetupUnsyncedFunction(L, "DrawWorldRefraction") ||
	    !SetupUnsyncedFunction(L, "DrawScreenEffects")   ||
	    !SetupUnsyncedFunction(L, "DrawScreen")          ||
	    !SetupUnsyncedFunction(L, "DrawInMiniMap")) {
		return false;
	}

	return true;
}


bool CLuaHandleSynced::SyncifyRandomFuncs(lua_State *L)
{
	// adjust the math.random() and math.randomseed() calls
	lua_getglobal(L, "math");
	if (!lua_istable(L, -1)) {
		LOG_L(L_WARNING, "lua.math does not exist");
		return false;
	}

	// copy the original random() into the registry
	lua_pushliteral(L, "random");
	lua_pushliteral(L, "random");
	lua_rawget(L, -3); // math table
	lua_rawset(L, LUA_REGISTRYINDEX);

	// copy the original randomseed() into the registry
	lua_pushliteral(L, "randomseed");
	lua_pushliteral(L, "randomseed");
	lua_rawget(L, -3); // math table
	lua_rawset(L, LUA_REGISTRYINDEX);

	// install our custom random()
	lua_pushliteral(L, "random");
	lua_pushcfunction(L, SyncedRandom);
	lua_rawset(L, -3);

	// remove randomseed()
	lua_pushliteral(L, "randomseed");
	lua_pushnil(L);
	lua_rawset(L, -3);

	lua_pop(L, 1); // pop the global math table

	return true;
}


bool CLuaHandleSynced::CopyRealRandomFuncs(lua_State *L)
{
	lua_pushliteral(L, "math");
	lua_rawget(L, -2);

	lua_pushliteral(L, "random");
	lua_pushliteral(L, "random");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawset(L, -3);

	lua_pushliteral(L, "randomseed");
	lua_pushliteral(L, "randomseed");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawset(L, -3);

	lua_pop(L, 1); // remove 'math'

	return true;
}


bool CLuaHandleSynced::SetupUnsyncedFunction(lua_State *L, const char* funcName)
{
	// copy the function from UNSYNCED into
	// the registry, and setfenv() it to UNSYNCED
	lua_settop(L, 0);
	unsyncedStr.GetRegistry(L);
	if (!lua_istable(L, -1)) {
		lua_settop(L, 0);
// FIXME		LOG_L(L_ERROR, "missing UNSYNCED table for %s", name.c_str());
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
		LOG_L(L_WARNING, "%s in %s is not a function",
				funcName, GetName().c_str());
		return false;
	}
	lua_pushvalue(L, unsynced);
	lua_setfenv(L, -2);

	lua_rawset(L, LUA_REGISTRYINDEX);

	lua_settop(L, 0);
	return true;
}


bool CLuaHandleSynced::CopyGlobalToUnsynced(lua_State *L, const char* name)
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


bool CLuaHandleSynced::LightCopyTable(lua_State *L, int dstIndex, int srcIndex)
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


bool CLuaHandleSynced::LoadUnsyncedCode(lua_State *L, const string& code, const string& debug)
{
	lua_settop(L, 0);

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		LOG_L(L_ERROR, "error = %i, %s, %s",
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

	SetRunning(L, true);
	error = lua_pcall(L, 0, 0, 0);
	SetRunning(L, false);

	if (error != 0) {
		LOG_L(L_ERROR, "error = %i, %s, %s",
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


bool CLuaHandleSynced::HasCallIn(lua_State *L, const string& name)
{
	if (!IsValid()) {
		return false;
	}

	static const std::string unsyncedNames[] = {
		"DrawUnit",
		"DrawFeature",
		"DrawShield",
		"RecvSkirmishAIMessage",
		"RecvFromSynced",
	};

	// unsynced call-ins in REGISTRY
	int tableIndex = LUA_REGISTRYINDEX;
	bool unsynced = false;

	for (unsigned int n = 0; n < (sizeof(unsyncedNames) / sizeof(std::string)); n++) {
		if (name == unsyncedNames[n]) {
			unsynced = true; break;
		}
	}

	// synced call-ins in GLOBAL
	if (!unsynced && !eventHandler.IsUnsynced(name))
		tableIndex = LUA_GLOBALSINDEX;

	bool haveFunc = true;
	lua_settop(L, 0);
	lua_pushsstring(L, name);
	lua_gettable(L, tableIndex);
	if (!lua_isfunction(L, -1)) {
		haveFunc = false;
	}
	lua_settop(L, 0);

	return haveFunc;
}


bool CLuaHandleSynced::SyncedUpdateCallIn(lua_State *L, const string& name)
{
	if ((name == "RecvFromSynced") ||
	    eventHandler.IsUnsynced(name)) {
		return false;
	}
	if (HasCallIn(L, name)) {
		eventHandler.InsertEvent(this, name);
	} else {
		eventHandler.RemoveEvent(this, name);
	}
	return true;
}


bool CLuaHandleSynced::UnsyncedUpdateCallIn(lua_State *L, const string& name)
{
	if ((name != "RecvFromSynced") &&
	    !eventHandler.IsUnsynced(name)) {
		  return false;
	}
	if (name != "RecvFromSynced") {
		if (HasCallIn(L, name)) {
			eventHandler.InsertEvent(this, name);
		} else {
			eventHandler.RemoveEvent(this, name);
		}
	}
	SetupUnsyncedFunction(L, name.c_str());
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Call-ins (first two are unused)
//

#if 0
bool CLuaHandleSynced::Initialize(const string& syncData)
{
	LUA_CALL_IN_CHECK(L, true);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("Initialize");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true;
	}

	int errfunc = SetupTraceback(L) ? -2 : 0;
	LOG("Initialize errfunc=%d", errfunc);

	lua_pushsstring(L, syncData);

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

	LUA_CALL_IN_CHECK(L, syncData);
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
#endif


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
	LUA_CALL_IN_CHECK(L, true);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("GotChatMsg");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushsstring(L, msg);
	lua_pushnumber(L, playerID);

	// call the routine
	RunCallIn(cmdStr, 2, 0);

	return true;
}


/******************************************************************************/



void CLuaHandleSynced::RecvFromSynced(lua_State *srcState, int args)
{
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

	if (!cmdStr.GetRegistryFunc(L))
		return; // the call is not defined
	lua_insert(L, 1); // place the function

	// call the routine
	SetAllowChanges(false);
	SetHandleSynced(L, false);

	lua_State* L_Prev = ForceUnsyncedState();
	RunCallIn(cmdStr, args, 0);
	RestoreState(L_Prev);

	SetHandleSynced(L, true);
	SetAllowChanges(true);
}


bool CLuaHandleSynced::RecvLuaMsg(const string& msg, int playerID)
{
	//FIXME: is there a reason to disallow gamestate changes in RecvLuaMsg?
	const bool prevAllowChanges = GetAllowChanges();
	SetAllowChanges(false);

	const bool retval = CLuaHandle::RecvLuaMsg(msg, playerID);

	SetAllowChanges(prevAllowChanges);

	return retval;
}


/******************************************************************************/
/******************************************************************************/
//
// Custom Call-in
//

bool CLuaHandleSynced::HasSyncedXCall(const string& funcName)
{
	SELECT_LUA_STATE();

	if (L != L_Sim)
		return false;

	GML_DRCMUTEX_LOCK(lua); // HasSyncedXCall

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushsstring(L, funcName); // push the function name
	lua_rawget(L, -2);                   // get the function
	const bool haveFunc = lua_isfunction(L, -1);
	lua_pop(L, 2);
	return haveFunc;
}


bool CLuaHandleSynced::HasUnsyncedXCall(lua_State* srcState, const string& funcName)
{
	SELECT_UNSYNCED_LUA_STATE();

	GML_DRCMUTEX_LOCK(lua); // HasUnsyncedXCall

	unsyncedStr.GetRegistry(L); // push the UNSYNCED table
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushsstring(L, funcName); // push the function name
	lua_rawget(L, -2);                   // get the function
	const bool haveFunc = lua_isfunction(L, -1);
	lua_pop(L, 2);
	return haveFunc;
}


int CLuaHandleSynced::XCall(lua_State* L, lua_State* srcState, const string& funcName)
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
	SELECT_LUA_STATE();

	if (L != L_Sim)
		return 0;

	GML_DRCMUTEX_LOCK(lua); // SyncedXCall

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	const int retval = XCall(L, srcState, funcName);
	ASSERT_SYNCED(retval);
	return retval;
}


int CLuaHandleSynced::UnsyncedXCall(lua_State* srcState, const string& funcName)
{
	SELECT_UNSYNCED_LUA_STATE();

	GML_DRCMUTEX_LOCK(lua); // UnsyncedXCall

	const bool prevSynced = GetHandleSynced(L);
	SetHandleSynced(L, false);
	unsyncedStr.GetRegistry(L); // push the UNSYNCED table
	const int retval = XCall(L, srcState, funcName);
	SetHandleSynced(L, prevSynced);
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
		if (lower > upper) {
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


int CLuaHandleSynced::CallAsTeam(lua_State* L)
{
	CLuaHandleSynced* lhs = GetSyncedHandle(L);
	if (lhs->teamsLocked) {
		luaL_error(L, "CallAsTeam() called when teams are locked");
	}
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// save the current access
	const bool prevFullCtrl    = GetHandleFullCtrl(L);
	const bool prevFullRead    = GetHandleFullRead(L);
	const int prevCtrlTeam     = GetHandleCtrlTeam(L);
	const int prevReadTeam     = GetHandleReadTeam(L);
	const int prevReadAllyTeam = GetHandleReadAllyTeam(L);
	const int prevSelectTeam   = GetHandleSelectTeam(L);

	// parse the new access
	if (lua_isnumber(L, 1)) {
		const int teamID = lua_toint(L, 1);
		if ((teamID < MinSpecialTeam) || (teamID >= teamHandler->ActiveTeams())) {
			luaL_error(L, "Bad teamID in SetCtrlTeam");
		}
		// ctrl
		SetHandleCtrlTeam(L, teamID);
		SetHandleFullCtrl(L, GetHandleCtrlTeam(L) == CEventClient::AllAccessTeam);
		// read
		SetHandleReadTeam(L, teamID);
		SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID));
		SetHandleFullRead(L, GetHandleReadAllyTeam(L) == CEventClient::AllAccessTeam);
		// select
		SetHandleSelectTeam(L, teamID);
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
				SetHandleCtrlTeam(L, teamID);
				SetHandleFullCtrl(L, GetHandleCtrlTeam(L) == CEventClient::AllAccessTeam);
			}
			else if (key == "read") {
				SetHandleReadTeam(L, teamID);
				SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID));
				SetHandleFullRead(L, GetHandleReadAllyTeam(L) == CEventClient::AllAccessTeam);
			}
			else if (key == "select") {
				SetHandleSelectTeam(L, teamID);
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// call the function
	const int funcArgs = lua_gettop(L) - 2;

	// protected call so that the permissions are always reverted
	const int error = lua_pcall(L, funcArgs, LUA_MULTRET, 0);

	// revert the permissions
	SetHandleFullCtrl(L, prevFullCtrl);
	SetHandleFullRead(L, prevFullRead);
	SetHandleCtrlTeam(L, prevCtrlTeam);
	SetHandleReadTeam(L, prevReadTeam);
	SetHandleReadAllyTeam(L, prevReadAllyTeam);
	SetHandleSelectTeam(L, prevSelectTeam);

	if (error != 0) {
		LOG_L(L_ERROR, "error = %i, %s, %s",
				error, "CallAsTeam", lua_tostring(L, -1));
		lua_error(L);
	}

	return lua_gettop(L) - 1;	// the teamID/table is still on the stack
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

	CLuaHandleSynced* lhs = GetSyncedHandle(L);
	lhs->textCommands[cmd] = lua_tostring(L, 2);
	wordCompletion->AddWord(cmdRaw, true, false, false);
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

	CLuaHandleSynced* lhs = GetSyncedHandle(L);

	map<string, string>::iterator it = lhs->textCommands.find(cmd);
	if (it != lhs->textCommands.end()) {
		lhs->textCommands.erase(it);
		wordCompletion->RemoveWord(cmdRaw);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


/******************************************************************************/

#define GetWatchDef(DefType)                                          \
	int CLuaHandleSynced::GetWatch ## DefType ## Def(lua_State* L) {  \
		CLuaHandleSynced* lhs = GetSyncedHandle(L);                   \
		const unsigned int defID = luaL_checkint(L, 1);               \
		if (defID >= lhs->watch ## DefType ## Defs.size()) {          \
			return 0;                                                 \
		}                                                             \
		lua_pushboolean(L, lhs->watch ## DefType ## Defs[defID]);     \
		return 1;                                                     \
	}

#define SetWatchDef(DefType)                                          \
	int CLuaHandleSynced::SetWatch ## DefType ## Def(lua_State* L) {  \
		CLuaHandleSynced* lhs = GetSyncedHandle(L);                   \
		const unsigned int defID = luaL_checkint(L, 1);               \
		if (defID >= lhs->watch ## DefType ## Defs.size()) {          \
			return 0;                                                 \
		}                                                             \
		if (!lua_isboolean(L, 2)) {                                   \
			luaL_error(L, "Incorrect arguments to %s", __FUNCTION__); \
			return 0;                                                 \
		}                                                             \
		lhs->watch ## DefType ## Defs[defID] = lua_toboolean(L, 2);   \
		return 0;                                                     \
	}

GetWatchDef(Unit)
GetWatchDef(Feature)
GetWatchDef(Weapon)

SetWatchDef(Unit)
SetWatchDef(Feature)
SetWatchDef(Weapon)

#undef GetWatchDef
#undef SetWatchDef

/******************************************************************************/
/******************************************************************************/
