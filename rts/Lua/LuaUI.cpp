/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUI.h"

#include "LuaConfig.h"
#include "LuaInclude.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaArchive.h"
#include "LuaCallInCheck.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstEngine.h"
#include "LuaConstGame.h"
#include "LuaConstPlatform.h"
#include "LuaSyncedRead.h"
#include "LuaInterCall.h"
#include "LuaUnsyncedRead.h"
#include "LuaUICommand.h"
#include "LuaFeatureDefs.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaScream.h"
#include "LuaOpenGL.h"
#include "LuaUtils.h"
#include "LuaVFS.h"
#include "LuaVFSDownload.h"
#include "LuaIO.h"
#include "LuaZip.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSModes.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"
#include "System/Threading/SpringThreading.h"
#include "lib/luasocket/src/luasocket.h"

#include <cstdio>
#include <cctype>

CONFIG(bool, LuaSocketEnabled)
	.defaultValue(true)
	.description("Enable LuaSocket support, allows Lua widgets to make TCP/UDP connections")
	.readOnly(true)
;


CLuaUI* luaUI = nullptr;


/******************************************************************************/
/******************************************************************************/

static spring::mutex m_singleton;

DECL_LOAD_HANDLER(CLuaUI, luaUI)
DECL_FREE_HANDLER(CLuaUI, luaUI)


/******************************************************************************/

CLuaUI::CLuaUI()
: CLuaHandle("LuaUI", LUA_HANDLE_ORDER_UI, true, false)
{
	luaUI = this;

	if (!IsValid())
		return;

	UpdateTeams();

	queuedAction = ACTION_NOVALUE;

	haveShockFront = false;
	shockFrontMinArea  = 0.0f;
	shockFrontMinPower = 0.0f;
	shockFrontDistAdj  = 100.0f;

	const bool luaSocketEnabled = configHandler->GetBool("LuaSocketEnabled");

	const std::string mode = (CLuaHandle::GetDevMode()) ? SPRING_VFS_RAW_FIRST : SPRING_VFS_MOD;
	const std::string file = (CFileHandler::FileExists("luaui.lua", mode) ? "luaui.lua": "LuaUI/main.lua");
	const std::string code = LoadFile(file, mode);

	LOG("LuaUI Entry Point: \"%s\"", file.c_str());
	LOG("LuaSocket Support: %s", (luaSocketEnabled? "enabled": "disabled"));

	if (code.empty()) {
		KillLua();
		return;
	}

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_io);
	LUA_OPEN_LIB(L, luaopen_os);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	LUA_OPEN_LIB(L, luaopen_debug);

	// initialize luasocket
	if (luaSocketEnabled)
		InitLuaSocket(L);

	// setup the lua IO access check functions
	lua_set_fopen(L, LuaIO::fopen);
	lua_set_popen(L, LuaIO::popen, LuaIO::pclose);
	lua_set_system(L, LuaIO::system);
	lua_set_remove(L, LuaIO::remove);
	lua_set_rename(L, LuaIO::rename);

	// remove a few dangerous calls
	lua_getglobal(L, "io");
	lua_pushstring(L, "popen"); lua_pushnil(L); lua_rawset(L, -3);
	lua_pop(L, 1);
	lua_getglobal(L, "os"); {
		lua_pushliteral(L, "exit");      lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "execute");   lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "remove");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "rename");    lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "tmpname");   lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "getenv");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "setlocale"); lua_pushnil(L); lua_rawset(L, -3);
	}
	lua_pop(L, 1); // os

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(L); // into Global

	// load the spring libraries
	if (!LoadCFunctions(L)                                                      ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)              ||
	    !AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushUnsynced)    ||
	    !AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushUnsynced)    ||
	    !AddEntriesToTable(L, "VFS",         LuaArchive::PushEntries)           ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)          ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)        ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)       ||
	    !AddEntriesToTable(L, "Script",      LuaInterCall::PushEntriesUnsynced) ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)            ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)        ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries)      ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedRead::PushEntries)      ||
	    !AddEntriesToTable(L, "Spring",      LuaUICommand::PushEntries)         ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)            ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)           ||
	    !AddEntriesToTable(L, "Engine",      LuaConstEngine::PushEntries)       ||
	    !AddEntriesToTable(L, "Platform",    LuaConstPlatform::PushEntries)     ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)         ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)          ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries)      ||
	    !AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)          ||
	    !AddEntriesToTable(L, "VFS",         LuaVFSDownload::PushEntries)
	) {
		KillLua();
		return;
	}

	lua_settop(L, 0);
	if (!LoadCode(L, code, file)) {
		KillLua();
		return;
	}

	// register for call-ins
	eventHandler.AddClient(this);

	// update extra call-ins
	UpdateCallIn(L, "WorldTooltip");
	UpdateCallIn(L, "MapDrawCmd");

	lua_settop(L, 0);
}


CLuaUI::~CLuaUI()
{
	luaUI = nullptr;
}

void CLuaUI::InitLuaSocket(lua_State* L) {
	std::string code;
	std::string filename = "socket.lua";
	CFileHandler f(filename);

	LUA_OPEN_LIB(L, luaopen_socket_core);

	if (f.LoadStringData(code)) {
		LoadCode(L, code, filename);
	} else {
		LOG_L(L_ERROR, "Error loading %s", filename.c_str());
	}
}

string CLuaUI::LoadFile(const string& name, const std::string& mode) const
{
	CFileHandler f(name, mode);

	string code;
	if (!f.LoadStringData(code))
		code.clear();

	return code;
}


static bool IsDisallowedCallIn(const string& name)
{
	switch (hashString(name.c_str())) {
		case hashString("Explosion"     ): { return true; } break;
		case hashString("DrawUnit"      ): { return true; } break;
		case hashString("DrawFeature"   ): { return true; } break;
		case hashString("DrawShield"    ): { return true; } break;
		case hashString("DrawProjectile"): { return true; } break;
		case hashString("DrawMaterial"  ): { return true; } break;
		default                          : {              } break;
	}

	return false;
}

bool CLuaUI::HasCallIn(lua_State* L, const string& name) const
{
	// never allow these calls
	if (IsDisallowedCallIn(name))
		return false;

	return CLuaHandle::HasCallIn(L, name);
}


void CLuaUI::UpdateTeams()
{
	if (luaUI) {
		luaUI->SetFullCtrl(gs->godMode);
		luaUI->SetCtrlTeam(gs->godMode ? AllAccessTeam :
		                  (gu->spectating ? NoAccessTeam : gu->myTeam));
		luaUI->SetFullRead(gu->spectatingFullView);
		luaUI->SetReadTeam(luaUI->GetFullRead() ? AllAccessTeam : gu->myTeam);
		luaUI->SetReadAllyTeam(luaUI->GetFullRead() ? AllAccessTeam : gu->myAllyTeam);
		luaUI->SetSelectTeam(gu->spectatingFullSelect ? AllAccessTeam : gu->myTeam);
	}
}


/******************************************************************************/
/******************************************************************************/

bool CLuaUI::LoadCFunctions(lua_State* L)
{
	lua_newtable(L);

	REGISTER_LUA_CFUNC(SetShockFrontFactors);

	lua_setglobal(L, "Spring");
	return true;
}


/******************************************************************************/
/******************************************************************************/

bool CLuaUI::ConfigureLayout(const string& command)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false; // the call is not defined

	lua_pushsstring(L, command);

	// call the routine
	return RunCallIn(L, cmdStr, 1, 0);
}



static inline float fuzzRand(float fuzz)
{
	return (1.0f + fuzz) - ((2.0f * fuzz) * guRNG.NextFloat());
}


void CLuaUI::ShockFront(const float3& pos, float power, float areaOfEffect, const float* distMod)
{
	if (!haveShockFront)
		return;
	if (power <= 0.0f)
		return;
	if (areaOfEffect < shockFrontMinArea && distMod == nullptr)
		return;

	float3 gap = (camera->GetPos() - pos);

	const float shockFrontDistMod = this->shockFrontDistAdj;
	float dist = gap.Length() + shockFrontDistMod;

	if ((power /= (dist * dist)) < shockFrontMinPower && distMod == nullptr)
		return;

	LUA_CALL_IN_CHECK(L);

	luaL_checkstack(L, 6, __func__);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L)) {
		haveShockFront = false; //FIXME improve in GameHelper.cpp and pipe instead through eventHandler?
		return; // the call is not defined
	}

	if (!gu->spectatingFullView && !losHandler->InLos(pos, gu->myAllyTeam)) {
		gap.x *= fuzzRand(0.25f);
		gap.y *= fuzzRand(0.25f);
		gap.z *= fuzzRand(0.25f);
		dist = gap.Length() + shockFrontDistMod;
	}
	const float3 dir = (gap / dist); // normalize

	lua_pushnumber(L, power);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);

	// call the routine
	RunCallIn(L, cmdStr, 4, 0);
}


/******************************************************************************/

bool CLuaUI::LayoutButtons(
	int& xButtons,
	int& yButtons,
	const vector<SCommandDescription>& cmds,
	vector<int>& removeCmds,
	vector<SCommandDescription>& customCmds,
	vector<int>& onlyTextureCmds,
	vector<ReStringPair>& reTextureCmds,
	vector<ReStringPair>& reNamedCmds,
	vector<ReStringPair>& reTooltipCmds,
	vector<ReParamsPair>& reParamsCmds,
	spring::unordered_map<int, int>& buttonList,
	string& menuName
) {
	customCmds.clear();
	removeCmds.clear();
	reTextureCmds.clear();
	reNamedCmds.clear();
	reTooltipCmds.clear();
	onlyTextureCmds.clear();
	buttonList.clear();
	menuName = "";

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __func__);
	const int top = lua_gettop(L);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	lua_pushnumber(L, xButtons);
	lua_pushnumber(L, yButtons);
	lua_pushnumber(L, cmds.size());

	if (!BuildCmdDescTable(L, cmds)) {
		lua_settop(L, top);
		return false;
	}

	// call the function
	if (!RunCallIn(L, cmdStr, 4, LUA_MULTRET)) {
		lua_settop(L, top);
		return false;
	}

	// get the results
	const int args = lua_gettop(L) - top;

	if (lua_israwstring(L, -1) &&
	    (string(lua_tostring(L, -1)) == "disabled")) {
		lua_settop(L, top);
		return false; // no warnings
	}
	else if (args != 11) {
		LOG_L(L_WARNING, "LayoutButtons() bad number of return values (%i)", args);
		lua_settop(L, top);
		return false;
	}

	if (!lua_isstring(L, 1)) {
		LOG_L(L_WARNING, "LayoutButtons() bad menuName value");
		lua_settop(L, top);
		return false;
	}
	menuName = string(lua_tostring(L, 1), lua_strlen(L, 1));

	if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		LOG_L(L_WARNING, "LayoutButtons() bad xButtons or yButtons values");
		lua_settop(L, top);
		return false;
	}
	xButtons = lua_toint(L, 2);
	yButtons = lua_toint(L, 3);

	if (!GetLuaIntList(L, 4, removeCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad removeCommands table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaCmdDescList(L, 5, customCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad customCommands table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaIntList(L, 6, onlyTextureCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad onlyTexture table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 7, reTextureCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad reTexture table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 8, reNamedCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad reNamed table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 9, reTooltipCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad reTooltip table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReParamsList(L, 10, reParamsCmds)) {
		LOG_L(L_WARNING, "LayoutButtons() bad reParams table");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaIntMap(L, 11, buttonList)) {
		LOG_L(L_WARNING, "LayoutButtons() bad buttonList table");
		lua_settop(L, top);
		return false;
	}

	lua_settop(L, top);

	return true;
}


bool CLuaUI::BuildCmdDescTable(lua_State* L, const vector<SCommandDescription>& cmds)
{
	lua_createtable(L, cmds.size(), 0);

	for (size_t i = 0; i < cmds.size(); i++) {
		lua_pushnumber(L, i + CMD_INDEX_OFFSET);
		LuaUtils::PushCommandDesc(L, cmds[i]);
		lua_rawset(L, -3);
	}

	return true;
}


bool CLuaUI::GetLuaIntMap(lua_State* L, int index, spring::unordered_map<int, int>& intMap)
{
	const int table = index;
	if (!lua_istable(L, table))
		return false;

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1) || !lua_israwnumber(L, -2)) {
			lua_pop(L, 2);
			return false;
		}
		const int key = lua_toint(L, -2);
		const int value = lua_toint(L, -1) - CMD_INDEX_OFFSET;
		intMap[key] = value;
	}

	return true;
}


bool CLuaUI::GetLuaIntList(lua_State* L, int index, vector<int>& intList)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1)) { // the value  (key is at -2)
			lua_pop(L, 2);
			return false;
		}
		const int value = lua_toint(L, -1) - CMD_INDEX_OFFSET;
		intList.push_back(value);
	}

	return true;
}


bool CLuaUI::GetLuaReStringList(lua_State* L, int index,
                                vector<ReStringPair>& reStringList)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReStringPair rp;
		rp.cmdIndex = lua_toint(L, -2) - CMD_INDEX_OFFSET;
		rp.texture = lua_tostring(L, -1);
		reStringList.push_back(rp);
	}

	return true;
}


bool CLuaUI::GetLuaReParamsList(lua_State* L, int index,
                                vector<ReParamsPair>& reParamsCmds)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_istable(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReParamsPair paramsPair;
		paramsPair.cmdIndex = lua_toint(L, -2) - CMD_INDEX_OFFSET;
		const int paramTable = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
			if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1)) {
				lua_pop(L, 4);
				return false;
			}
			const int paramIndex = lua_toint(L, -2);
			const string paramValue = string(lua_tostring(L, -1), lua_strlen(L, -1));
			paramsPair.params[paramIndex] = paramValue;
		}
		reParamsCmds.push_back(paramsPair);
	}

	return true;
}


bool CLuaUI::GetLuaCmdDescList(lua_State* L, int index, vector<SCommandDescription>& cmdDescs)
{
	const int table = index;
	if (!lua_istable(L, table))
		return false;

	int paramIndex = 1;
	for (lua_rawgeti(L, table, paramIndex);
	     lua_istable(L, -1);
	     lua_pop(L, 1), paramIndex++, lua_rawgeti(L, table, paramIndex)) {
		SCommandDescription cd;
		cd.id   = CMD_INTERNAL;
		cd.type = CMDTYPE_CUSTOM;
		const int cmdDescTable = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, cmdDescTable) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isstring(L, -1)) {
				const string key = StringToLower(string(lua_tostring(L, -2), lua_strlen(L, -2)));
				const string value(lua_tostring(L, -1), lua_strlen(L, -1));
				if (key == "id") {
					cd.id = atoi(value.c_str());
				} else if (key == "type") {
					cd.type = atoi(value.c_str());
				} else if (key == "name") {
					cd.name = value;
				} else if (key == "action") {
					cd.action = value;
				} else if (key == "texture") {
					cd.iconname = value;
				} else if (key == "cursor") {
					cd.mouseicon = value;
				} else if (key == "tooltip") {
					cd.tooltip = value;
				} else if (key == "nwtext") {
					cd.params.push_back(string("$nw$") + value);
				} else if (key == "swtext") {
					cd.params.push_back(string("$sw$") + value);
				} else if (key == "netext") {
					cd.params.push_back(string("$ne$") + value);
				} else if (key == "setext") {
					cd.params.push_back(string("$se$") + value);
				} else {
					LOG_L(L_WARNING, "GetLuaCmdDescList() unknown key (%s)", key.c_str());
				}
			}
			else if (lua_israwstring(L, -2) && lua_istable(L, -1)) {
				const string key = StringToLower(lua_tostring(L, -2));
				if (key != "actions") {
					LOG_L(L_WARNING, "GetLuaCmdDescList() non \"actions\" table: %s",
							key.c_str());
					continue;
				}
				const int actionsTable = lua_gettop(L);
				for (lua_pushnil(L); lua_next(L, actionsTable) != 0; lua_pop(L, 1)) {
					if (lua_isstring(L, -1)) {
						string action = lua_tostring(L, -1);
						if (action[0] != '@') {
							action = "@@" + action;
						}
						cd.params.push_back(action);
					}
				}
			}
			else {
				LOG_L(L_WARNING, "GetLuaCmdDescList() bad entry");
			}
		}
		cmdDescs.push_back(cd);
	}

	if (!lua_isnil(L, -1)) {
		LOG_L(L_WARNING, "GetLuaCmdDescList() entry %i is not a table", paramIndex);
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
// Lua Callbacks
//

int CLuaUI::SetShockFrontFactors(lua_State* L)
{
	luaUI->haveShockFront = true;

	if (lua_isnumber(L, 1))
		luaUI->shockFrontMinArea = std::max(0.0f, lua_tofloat(L, 1));

	if (lua_isnumber(L, 2))
		luaUI->shockFrontMinPower = std::max(0.0f, lua_tofloat(L, 2));

	if (lua_isnumber(L, 3))
		luaUI->shockFrontDistAdj = std::max(1.0f, lua_tofloat(L, 3));

	return 0;
}

/******************************************************************************/
/******************************************************************************/
