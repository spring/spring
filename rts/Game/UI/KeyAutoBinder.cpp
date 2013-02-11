/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>

#include "KeyAutoBinder.h"

#include "LuaInclude.h"

#include "KeyBindings.h"
#include "KeyBindingsLog.h"
#include "Sim/Misc/Team.h"
#include "Lua/LuaDefs.h"
#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaConstGame.h"
#include "Lua/LuaUnitDefs.h"
#include "Lua/LuaWeaponDefs.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Util.h"

#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_KEY_BINDINGS


static const string UnitDefsName    = "UnitDefs";
static const string ReqFuncName     = "HasReqs";
static const string SortFuncName    = "IsBetter";
static const string CompareFuncName = "Compare";


#ifndef _WIN32
static const string endlStr = "\n";
#else
static const string endlStr = "\r\n";
#endif


/******************************************************************************/

CKeyAutoBinder::CKeyAutoBinder()
	: CLuaHandle("KeyAutoBinder", 1234, false)
{
	if (!IsValid())
		return;

	BEGIN_ITERATE_LUA_STATES();

	LoadCompareFunc(L);

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	LUA_OPEN_LIB(L, luaopen_debug);
	//LUA_OPEN_LIB(L, luaopen_io);
	//LUA_OPEN_LIB(L, luaopen_os);
	//LUA_OPEN_LIB(L, luaopen_package);

	// load the spring libraries
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (!AddEntriesToTable(L, "Game",               LuaConstGame::PushEntries)  ||
	    !AddEntriesToTable(L, UnitDefsName.c_str(), LuaUnitDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "WeaponDefs",         LuaWeaponDefs::PushEntries)) {
		LOG_L(L_ERROR, "Failed loading Lua libraries");
	}

	lua_settop(L, 0);

	END_ITERATE_LUA_STATES();
}


CKeyAutoBinder::~CKeyAutoBinder()
{
}


string CKeyAutoBinder::LoadFile(const string& filename) const
{
	CFileHandler f(filename, SPRING_VFS_RAW);

	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


bool CKeyAutoBinder::LoadCode(lua_State *L, const string& code, const string& debug)
{
	if (!IsValid())
		return false;

	lua_settop(L, 0);

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		LOG_L(L_ERROR, "Loading: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	SetRunning(L, true); // just for the sake of consistency
	error = lua_pcall(L, 0, 0, 0);
	SetRunning(L, false);
	if (error != 0) {
		LOG_L(L_ERROR, "Running: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


bool CKeyAutoBinder::LoadCompareFunc(lua_State *L)
{
	std::stringstream code(endlStr);

	// handy sorting comparison routine
	code << "function " << CompareFuncName << "(a, b)"   << endlStr;
	code << "  if (type(a) == \"number\") then"          << endlStr;
	code << "    if (a > b) then return  1.0; end"       << endlStr;
	code << "    if (a < b) then return -1.0; end"       << endlStr;
	code << "  elseif (type(a) == \"boolean\") then"     << endlStr;
	code << "    if (a and not b) then return  1.0; end" << endlStr;
	code << "    if (not a and b) then return -1.0; end" << endlStr;
	code << "  elseif (type(a) == \"string\") then"      << endlStr;
	code << "    if (a > b) then return  1.0; end"       << endlStr;
	code << "    if (a < b) then return -1.0; end"       << endlStr;
	code << "  end"                                      << endlStr;
	code << "  return 0.0"                               << endlStr;
	code << "end"                                        << endlStr;

	const std::string codeStr = code.str();

	LOG_L(L_DEBUG, "%s", codeStr.c_str());

	if (!LoadCode(L, codeStr, CompareFuncName + "()")) {
		return false;
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/

static CKeyAutoBinder* thisBinder = NULL;
static lua_State* tmpLuaState = NULL;


bool CKeyAutoBinder::UnitDefHolder::operator<(const UnitDefHolder& m) const
{
	return thisBinder->IsBetter(tmpLuaState, udID, m.udID);
}


/******************************************************************************/

bool CKeyAutoBinder::BindBuildType(const string& keystr,
                                   const vector<string>& requirements,
                                   const vector<string>& sortCriteria,
                                   const vector<string>& chords)
{
	if (!IsValid())
		return false;

	SELECT_LUA_STATE();

	lua_settop(L, 0);

	const string reqCall = MakeRequirementCall(requirements);
	const string sortCall = MakeSortCriteriaCall(sortCriteria);

	if (LOG_IS_ENABLED(L_DEBUG)) {
		LOG_L(L_DEBUG, "--reqCall(%s):", keystr.c_str());
		LOG_L(L_DEBUG, "%s", reqCall.c_str());
		LOG_L(L_DEBUG, "--sortCall(%s):", keystr.c_str());
		LOG_L(L_DEBUG, "%s", sortCall.c_str());
	}

	if (!LoadCode(L, reqCall,  keystr + ":" + ReqFuncName + "()") ||
	    !LoadCode(L, sortCall, keystr + ":" + SortFuncName + "()")) {
		return false;
	}

	vector<UnitDefHolder> defs;

	const std::map<std::string, int>& udMap = unitDefHandler->unitDefIDsByName;
	std::map<std::string, int>::const_iterator udIt;
	for (udIt = udMap.begin(); udIt != udMap.end(); ++udIt) {
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(udIt->second);
		if (ud == NULL) {
	  	continue;
		}
		if (HasRequirements(L, ud->id)) {
			UnitDefHolder udh(ud->id, ud);
			defs.push_back(udh);
		}
	}

	// sort the results
	tmpLuaState = L;
	thisBinder = this;
	sort(defs.begin(), defs.end());

	// add the bindings
	const string bindStr = "bind";
	for (unsigned int i = 0; i < defs.size(); i++) {
		const string lowerName = StringToLower(defs[i].ud->name);
		const string action = string("buildunit_") + lowerName;
		keyBindings->ExecuteCommand(bindStr + " " + keystr + " " + action);
		if ((i < chords.size()) && (StringToLower(chords[i]) != "none")) {
			keyBindings->ExecuteCommand(bindStr + " " + chords[i] + " " + action);
		}
		LOG_L(L_DEBUG, "auto-%s %s %s",
				bindStr.c_str(), keystr.c_str(), action.c_str());
	}

	return true;
}


string CKeyAutoBinder::ConvertBooleanSymbols(const string& text) const
{
	string newText = text;
	newText = StringReplace(newText, "&&", " and ");
	newText = StringReplace(newText, "||", " or ");
	newText = StringReplace(newText, "!",  " not ");
	return newText;
}


string CKeyAutoBinder::AddUnitDefPrefix(const string& text,
                                        const string& prefix) const
{
	std::stringstream result("");
	const char* end = text.c_str();

	while (end[0] != 0) {
		const char* c = end;
		while ((c[0] != 0) && !isalpha(c[0]) && (c[0] != '_')) { c++; }
		result << string(end, c - end).c_str();
		if (c[0] == 0) {
			break;
		}
		const char* start = c;
		while ((c[0] != 0) && (isalnum(c[0]) || (c[0] == '_'))) { c++; }
		string word(start, c - start);
		if (LuaUnitDefs::IsDefaultParam(word)) {
			result << prefix << "." << word;
		} else if ((word == "custom") && (c[0] == '.')) {
			result << prefix;
		} else {
			result << word;
		}
		end = c;
	}

	return result.str();
}


string CKeyAutoBinder::MakeRequirementCall(const vector<string>& requirements)
{
	std::stringstream code("");

	code << "function " << ReqFuncName << "(thisID)" << endlStr;

	const int count = (int)requirements.size();

	if (count <= 0) {
		code << "return false" << endlStr;
		code << "end" << endlStr;
		return code.str();
	}

	code << "local this = " << UnitDefsName << "[thisID]" << endlStr;

	if (StringToLower(requirements[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code << requirements[i] << endlStr;
		}
		code << "end" << endlStr;
		return code.str();
	}

	code << "return ";
	const int lastReq = (count - 1);
	for (int i = 0; i < count; i++) {
		code << "(";
		code << requirements[i];
		code << ")";
		if (i != lastReq) {
			code << " and ";
		}
	}
	code << endlStr << "end" << endlStr;

	return ConvertBooleanSymbols(AddUnitDefPrefix(code.str(), "this"));
}


string CKeyAutoBinder::MakeSortCriteriaCall(const vector<string>& sortCriteria)
{
	std::stringstream code("");

	code << "function " << SortFuncName << "(thisID, thatID)" << endlStr;

	const int count = (int)sortCriteria.size();

	if (count <= 0) {
		code << "return false" << endlStr;
		code << "end" << endlStr;
		return code.str();
	}

	code << "local this = " << UnitDefsName << "[thisID]" << endlStr;
	code << "local that = " << UnitDefsName << "[thatID]" << endlStr;

	if (StringToLower(sortCriteria[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code << sortCriteria[i] << endlStr;
		}
		code << "end" << endlStr;
		return code.str();
	}

	for (int i = 0; i < count; i++) {
		const string natural = ConvertBooleanSymbols(sortCriteria[i]);
		const string thisStr = AddUnitDefPrefix(natural, "this");
		const string thatStr = AddUnitDefPrefix(natural, "that");
		code << "local test = " << CompareFuncName << "(" << thisStr << ", " << thatStr << ")" << endlStr;
		code << "if (test ~= 0.0) then return (test > 0.0) end" << endlStr;
	}
	code << "return false" << endlStr;
	code << "end" << endlStr;

	return ConvertBooleanSymbols(code.str());
}


bool CKeyAutoBinder::HasRequirements(lua_State* L, int unitDefID)
{
	lua_getglobal(L, ReqFuncName.c_str());
	lua_pushnumber(L, unitDefID);
	const int error = lua_pcall(L, 1, 1, 0);
	if (error != 0) {
		LOG_L(L_ERROR, "Running %s(%i)\n  %s",
				ReqFuncName.c_str(), unitDefID, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}


bool CKeyAutoBinder::IsBetter(lua_State* L, int thisDefID, int thatDefID)
{
	lua_getglobal(L, SortFuncName.c_str());
	lua_pushnumber(L, thisDefID);
	lua_pushnumber(L, thatDefID);
	const int error = lua_pcall(L, 2, 1, 0);
	if (error != 0) {
		LOG_L(L_ERROR, "Running %s(%i, %i)\n  %s",
				SortFuncName.c_str(), thisDefID, thatDefID, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}
