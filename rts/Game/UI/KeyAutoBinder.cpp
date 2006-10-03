#include "StdAfx.h"
// KeyAutoBinder.cpp: implementation of the CKeyAutoBinder class.
//
//////////////////////////////////////////////////////////////////////

#include "KeyAutoBinder.h"

#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}
#include "LuaState.h"
#include "KeyBindings.h"
#include "SimpleParser.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/LogOutput.h"

static string IntToString(int value);
static string BoolToString(bool value);
static string FloatToString(float value);
static string SafeString(const string& text);
static string StringReplace(const string& text,
                            const string& from, const string& to);
static string GetCategoryTableFromBits(unsigned int bits);
static string GetCategoryTableFromString(const string& text);


// FIXME -- use 'em
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
{
	LoadCompareFunc();
}


CKeyAutoBinder::~CKeyAutoBinder()
{
}


bool CKeyAutoBinder::LoadCode(const string& code, const string& debug)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("ERROR: KeyAutoBinder: Loading: %s\n",
		                lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		logOutput.Print("ERROR: KeyAutoBinder: Running: %s\n",
		                lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	return true;
}


bool CKeyAutoBinder::LoadCompareFunc()
{
	string code = endlStr;

	// handy sorting comparison routine
	code += "function Compare(a, b)"                     + endlStr;
	code += "  if (type(a) == \"number\") then"          + endlStr;
	code += "    if (a > b) then return  1.0; end"       + endlStr;
	code += "    if (a < b) then return -1.0; end"       + endlStr;
	code += "  elseif (type(a) == \"boolean\") then"     + endlStr;
	code += "    if (a and not b) then return  1.0; end" + endlStr;
	code += "    if (not a and b) then return -1.0; end" + endlStr;
	code += "  elseif (type(a) == \"string\") then"      + endlStr;
	code += "    if (a > b) then return  1.0; end"       + endlStr;
	code += "    if (a < b) then return -1.0; end"       + endlStr;
	code += "  end"                                      + endlStr;
	code += "  return 0.0"                               + endlStr;
	code += "end"                                        + endlStr;

	if (keyBindings->GetDebug() > 1) {
		logOutput.Print(code);
	}

	if (!LoadCode(code, "Compare()")) {
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
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	const string reqCall = MakeRequirementCall(requirements);
	const string sortCall = MakeSortCriteriaCall(sortCriteria);

	if (keyBindings->GetDebug() > 1) {  
		logOutput.Print("--reqCall(%s):\n", keystr.c_str());
		logOutput.Print(reqCall);
		logOutput.Print("--sortCall(%s):\n", keystr.c_str());
		logOutput.Print(sortCall);
	}

	if (!LoadCode(reqCall,  keystr + ":HasReqs()") ||
	    !LoadCode(sortCall, keystr + ":IsBetter()")) {
		return false;
	}

	vector<UnitDefHolder> defs;

	// find the unit definitions that meet the requirements	
	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
		const UnitDef& ud = unitDefHandler->unitDefs[uit->second];
		if (HasRequirements(L, uit->second)) {
			UnitDefHolder udh;
			udh.ud = &ud;
			udh.udID = uit->second;
			defs.push_back(udh);
		}
	}

	// sort the results
	tmpLuaState = L;
	thisBinder = this;
	sort(defs.begin(), defs.end());

	// add the bindings
	for (unsigned int i = 0; i < defs.size(); i++) {
		const string bindStr = "bind";
		const string lowerName = StringToLower(defs[i].ud->name);
		const string action = string("buildunit_") + lowerName;
		keyBindings->Command(bindStr + " " + keystr + " " + action);
		if ((i < chords.size()) && (StringToLower(chords[i]) != "none")) {
			keyBindings->Command(bindStr + " " + chords[i] + " " + action);
		}
		if (keyBindings->GetDebug() > 0) {
			string msg = "auto-";
			msg += bindStr + " " + keystr + " " + action;
			logOutput.Print(msg);
		}
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
	string result;
	const char* end = text.c_str();
	
	while (end[0] != 0) {
		const char* c = end;
		while ((c[0] != 0) && !isalpha(c[0]) && (c[0] != '_')) { c++; }
		result += string(end, c - end);
		if (c[0] == 0) {
			break;
		}
		const char* start = c;
		while ((c[0] != 0) && (isalnum(c[0]) || (c[0] == '_'))) { c++; }
		string word(start, c - start);
		const set<string>& unitDefParams = LUASTATE.GetUnitDefParams();
		if (unitDefParams.find(word) != unitDefParams.end()) {
			result += prefix + "." + word;
		} else if ((word == "custom") && (c[0] == '.')) {
			result += prefix;
		} else {
			result += word;
		}
		end = c;
	}
		
	return result;
}


string CKeyAutoBinder::MakeRequirementCall(const vector<string>& requirements)
{
	string code = "";

	code += "function HasReqs(thisID)" + endlStr;
	
	const int count = (int)requirements.size();
	
	if (count <= 0) {
		code += "return false" + endlStr;
		code += "end" + endlStr;
		return code;
	}

	code += "local this = UnitDefs[thisID]" + endlStr;
	
	if (StringToLower(requirements[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code += requirements[i] + endlStr;
		}
		code += "end" + endlStr;
		return code;
	}
	
	code += "return ";
	const int lastReq = (count - 1);
	for (int i = 0; i < count; i++) {
		code += "(";
		code += requirements[i];
		code += ")";
		if (i != lastReq) {
			code += " and ";
		}
	}
	code += endlStr + "end" + endlStr;
	
	return ConvertBooleanSymbols(AddUnitDefPrefix(code, "this"));
}


string CKeyAutoBinder::MakeSortCriteriaCall(const vector<string>& sortCriteria)
{
	string code = "";

	code += "function IsBetter(thisID, thatID)" + endlStr;
	
	const int count = (int)sortCriteria.size();

	if (count <= 0) {
		code += "return false" + endlStr;
		code += "end" + endlStr;
		return code;
	}

	code += "local this = UnitDefs[thisID]" + endlStr;
	code += "local that = UnitDefs[thatID]" + endlStr;

	if (StringToLower(sortCriteria[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code += sortCriteria[i] + endlStr;
		}
		code += "end" + endlStr;
		return code;
	}
	
	for (int i = 0; i < count; i++) {
		const string natural = ConvertBooleanSymbols(sortCriteria[i]);
		const string thisStr = AddUnitDefPrefix(natural, "this");
		const string thatStr = AddUnitDefPrefix(natural, "that");
		code += "local test = Compare(" + thisStr + ", " + thatStr + ")" + endlStr;
		code += "if (test ~= 0.0) then return (test > 0.0) end" + endlStr;
	}
	code += "return false" + endlStr;
	code += "end" + endlStr;
	
	return ConvertBooleanSymbols(code);
}


bool CKeyAutoBinder::HasRequirements(lua_State* L, int unitDefID)
{
	lua_getglobal(L, "HasReqs");
	lua_pushnumber(L, unitDefID);
	const int error = lua_pcall(L, 1, 1, 0);
	if (error != 0) {
		logOutput.Print("ERROR: KeyAutoBinder: Running HasReqs(%i)\n  %s\n",
		                unitDefID, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}


bool CKeyAutoBinder::IsBetter(lua_State* L, int thisDefID, int thatDefID)
{
	lua_getglobal(L, "IsBetter");
	lua_pushnumber(L, thisDefID);
	lua_pushnumber(L, thatDefID);
	const int error = lua_pcall(L, 2, 1, 0);
	if (error != 0) {
		logOutput.Print("ERROR: KeyAutoBinder: Running IsBetter(%i, %i)\n  %s\n",
		                thisDefID, thatDefID, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/

static string IntToString(int value)
{
	char buf[32];
	SNPRINTF(buf, sizeof(buf), "%i", value);
	return string(buf);
}


static string BoolToString(bool value)
{
	if (value) {
		return string("true");
	}
	return string("false");
}


static string FloatToString(float value)
{
	char buf[32];
	SNPRINTF(buf, sizeof(buf), "%f", value);
	return string(buf);
}


static string SafeString(const string& text)
{
	const string noSlash =  StringReplace(text,    "\\", "\\\\");
	const string noQuote =  StringReplace(noSlash, "\"", "\\\"");
	const string quote = string("\"");
	return (quote + noQuote + quote);
}


static string StringReplace(const string& text,
                            const string& from, const string& to)
{
	string working = text;
	string::size_type pos = 0;
	while (true) {
		pos = working.find(from, pos);
		if (pos == string::npos) {
			break;
		}
		string tmp = working.substr(0, pos);
		tmp += to;
		tmp += working.substr(pos + from.size(), string::npos);
		pos += to.size();
		working = tmp;
	}
	return working;
}


static string GetCategoryTable(const vector<string> cats)
{
	string table = "{";
	const int catCount = (int)cats.size();
	for (int i = 0; i < catCount; i++) {
		table += " ";
		table += SafeString(StringToLower(cats[i]));
		if (i != (catCount - 1)) {
			table += ",";
		} else {
			table += " ";
		}
	}
	table += "}";
	return table;
}


static string GetCategoryTableFromBits(unsigned int bits)
{
	vector<string> cats = CCategoryHandler::Instance()->GetCategoryNames(bits);
	return GetCategoryTable(cats);
}


static string GetCategoryTableFromString(const string& text)
{
	vector<string> cats = SimpleParser::Tokenize(text, 0);
	return GetCategoryTable(cats);
}


/******************************************************************************/
