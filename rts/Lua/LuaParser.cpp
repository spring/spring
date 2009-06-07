#include "StdAfx.h"
// LuaParser.cpp: implementation of the LuaParser class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaParser.h"

#include <algorithm>
#include <limits.h>
#include <boost/regex.hpp>

#include "mmgr.h"

#include "LuaInclude.h"

#include "LuaIO.h"
#include "LuaUtils.h"

#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Util.h"
#include "System/myTime.h"


#if (LUA_VERSION_NUM < 500)
#  define LUA_OPEN_LIB(L, lib) lib(L)
#else
#  define LUA_OPEN_LIB(L, lib) \
     lua_pushcfunction((L), lib); \
     lua_pcall((L), 0, 0, 0);
#endif


LuaParser* LuaParser::currentParser = NULL;


/******************************************************************************/
/******************************************************************************/
//
//  LuaParser
//

LuaParser::LuaParser(const string& _fileName,
                     const string& _fileModes,
                     const string& _accessModes)
: fileName(_fileName),
  fileModes(_fileModes),
  textChunk(""),
  accessModes(_accessModes),
  valid(false),
  initDepth(0),
  rootRef(LUA_NOREF),
  currentRef(LUA_NOREF),
  lowerKeys(true),
  lowerCppKeys(true)
{
	L = lua_open();

	if (L != NULL) {
		SetupEnv();
	}
}


LuaParser::LuaParser(const string& _textChunk,
                     const string& _accessModes)
: fileName(""),
  fileModes(""),
  textChunk(_textChunk),
  accessModes(_accessModes),
  valid(false),
  initDepth(0),
  rootRef(LUA_NOREF),
  currentRef(LUA_NOREF),
  lowerKeys(true),
  lowerCppKeys(true)
{
	L = lua_open();

	if (L != NULL) {
		SetupEnv();
	}
}


LuaParser::~LuaParser()
{
	if (L != NULL) {
		lua_close(L);
	}
	set<LuaTable*>::iterator it;
	for (it = tables.begin(); it != tables.end(); ++it) {
		LuaTable& table = **it;
		table.parser  = NULL;
		table.L       = NULL;
		table.isValid = false;
		table.refnum  = LUA_NOREF;
	}
}


void LuaParser::SetupEnv()
{
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	//LUA_OPEN_LIB(L, luaopen_io);
	//LUA_OPEN_LIB(L, luaopen_os);
	//LUA_OPEN_LIB(L, luaopen_package);
	//LUA_OPEN_LIB(L, luaopen_debug);

	// delete some dangerous/unsynced functions
	lua_pushnil(L); lua_setglobal(L, "dofile");
	lua_pushnil(L); lua_setglobal(L, "loadfile");
	lua_pushnil(L); lua_setglobal(L, "loadlib");
	lua_pushnil(L); lua_setglobal(L, "require");
	lua_pushnil(L); lua_setglobal(L, "gcinfo");
	lua_pushnil(L); lua_setglobal(L, "collectgarbage");

	// FIXME: replace "random" as in LuaHandleSynced (can write your own for now)
	lua_getglobal(L, "math");
	lua_pushstring(L, "random");     lua_pushnil(L); lua_rawset(L, -3);
	lua_pushstring(L, "randomseed"); lua_pushnil(L); lua_rawset(L, -3);
	lua_pop(L, 1); // pop "math"

	AddFunc("DontMessWithMyCase", DontMessWithMyCase);

	GetTable("Spring");
	AddFunc("Echo", Echo);
	AddFunc("TimeCheck", TimeCheck);
	EndTable();

	GetTable("VFS");
	AddFunc("DirList",    DirList);
	AddFunc("Include",    Include);
	AddFunc("LoadFile",   LoadFile);
	AddFunc("FileExists", FileExists);
	EndTable();
}


/******************************************************************************/

bool LuaParser::Execute()
{
	if (L == NULL) {
		errorLog = "could not initialize LUA library";
		return false;
	}

	rootRef = LUA_NOREF;

	assert(initDepth == 0);
	initDepth = -1;

	string code;
	string codeLabel;
	if (!textChunk.empty()) {
		code = textChunk;
		codeLabel = "text chunk";
	}
	else if (!fileName.empty()) {
		codeLabel = fileName;
		CFileHandler fh(fileName, fileModes);
		if (!fh.LoadStringData(code)) {
			errorLog = "could not open file: " + fileName;
			lua_close(L);
			L = NULL;
			return false;
		}
	}
	else {
		errorLog = "no source file or text";
		lua_close(L);
		L = NULL;
		return false;
	}

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), codeLabel.c_str());
	if (error != 0) {
		errorLog = lua_tostring(L, -1);
		logOutput.Print("error = %i, %s, %s\n",
		                error, codeLabel.c_str(), errorLog.c_str());
		lua_close(L);
		L = NULL;
		return false;
	}

	currentParser = this;

	error = lua_pcall(L, 0, 1, 0);

	currentParser = NULL;

	if (error != 0) {
		errorLog = lua_tostring(L, -1);
		logOutput.Print("error = %i, %s, %s\n",
		                error, fileName.c_str(), errorLog.c_str());
		lua_close(L);
		L = NULL;
		return false;
	}

	if (!lua_istable(L, 1)) {
		errorLog = "missing return table from " + fileName + "\n";
		logOutput.Print("missing return table from %s\n", fileName.c_str());
		lua_close(L);
		L = NULL;
		return false;
	}

	if (lowerKeys) {
		LuaUtils::LowerKeys(L, 1);
	}

	rootRef = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_settop(L, 0);

	valid = true;

	return true;
}


void LuaParser::AddTable(LuaTable* tbl)
{
	tables.insert(tbl);
}


void LuaParser::RemoveTable(LuaTable* tbl)
{
	tables.erase(tbl);
}


LuaTable LuaParser::GetRoot()
{
	return LuaTable(this);
}


/******************************************************************************/

void LuaParser::PushParam()
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	if (initDepth > 0) {
		lua_rawset(L, -3);
	} else {
		lua_rawset(L, LUA_GLOBALSINDEX);
	}
}


void LuaParser::GetTable(const string& name, bool overwrite)
{
	if ((L == NULL) || (initDepth < 0)) { return; }

	lua_pushstring(L, name.c_str());

	if (overwrite) {
		lua_newtable(L);
	}
	else {
		lua_pushstring(L, name.c_str());
		lua_gettable(L, (initDepth == 0) ? LUA_GLOBALSINDEX : -3);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
		}
	}

	initDepth++;
}


void LuaParser::GetTable(int index, bool overwrite)
{
	if ((L == NULL) || (initDepth < 0)) { return; }

	lua_pushnumber(L, index);

	if (overwrite) {
		lua_newtable(L);
	}
	else {
		lua_pushnumber(L, index);
		lua_gettable(L, (initDepth == 0) ? LUA_GLOBALSINDEX : -3);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
		}
	}

	initDepth++;
}


void LuaParser::EndTable()
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	assert(initDepth > 0);
	initDepth--;
	PushParam();
}


/******************************************************************************/

void LuaParser::AddFunc(const string& key, int (*func)(lua_State*))
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	if (func == NULL) { return; }
	lua_pushstring(L, key.c_str());
	lua_pushcfunction(L, func);
	PushParam();
}


void LuaParser::AddInt(const string& key, int value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddBool(const string& key, bool value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushstring(L, key.c_str());
	lua_pushboolean(L, value);
	PushParam();
}


void LuaParser::AddFloat(const string& key, float value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddString(const string& key, const string& value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	PushParam();
}


/******************************************************************************/

void LuaParser::AddFunc(int key, int (*func)(lua_State*))
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	if (func == NULL) { return; }
	lua_pushnumber(L, key);
	lua_pushcfunction(L, func);
	PushParam();
}


void LuaParser::AddInt(int key, int value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushnumber(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddBool(int key, bool value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushnumber(L, key);
	lua_pushboolean(L, value);
	PushParam();
}


void LuaParser::AddFloat(int key, float value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushnumber(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddString(int key, const string& value)
{
	if ((L == NULL) || (initDepth < 0)) { return; }
	lua_pushnumber(L, key);
	lua_pushstring(L, value.c_str());
	PushParam();
}


/******************************************************************************/
/******************************************************************************/
//
//  call-outs
//

int LuaParser::Echo(lua_State* L)
{
	return LuaUtils::Echo(L);
}


int LuaParser::TimeCheck(lua_State* L)
{
	if (!lua_isstring(L, 1) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Invalid arguments to TimeCheck('string', func, ...)");
	}
	const string name = lua_tostring(L, 1);
	lua_remove(L, 1);
	const spring_time startTime = spring_gettime();
	const int error = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
	if (error != 0) {
		const string errmsg = lua_tostring(L, -1);
		lua_pop(L, 1);
		luaL_error(L, errmsg.c_str());
	}
	const spring_time endTime = spring_gettime();
	const float elapsed = 1.0e-3f * (float)(spring_tomsecs(endTime - startTime));
	logOutput.Print("%s %f", name.c_str(), elapsed);
	return lua_gettop(L);
}


/******************************************************************************/

int LuaParser::DirList(lua_State* L)
{
	if (currentParser == NULL) {
		luaL_error(L, "invalid call to DirList() after execution");
	}

	const string dir = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(dir)) {
		return 0;
	}
	const string pat = luaL_optstring(L, 2, "*");
	string modes = luaL_optstring(L, 3, currentParser->accessModes.c_str());
	modes = CFileHandler::AllowModes(modes, currentParser->accessModes);

	const vector<string> files = CFileHandler::DirList(dir, pat, modes);

	lua_newtable(L);
	int count = 0;
	vector<string>::const_iterator fi;
	for (fi = files.begin(); fi != files.end(); ++fi) {
		count++;
		lua_pushnumber(L, count);
		lua_pushstring(L, fi->c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


/******************************************************************************/

int LuaParser::Include(lua_State* L)
{
	if (currentParser == NULL) {
		luaL_error(L, "invalid call to Include() after execution");
	}

	// filename [, fenv]
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		luaL_error(L, "bad pathname");
	}
	string modes = luaL_optstring(L, 3, currentParser->accessModes.c_str());
	modes = CFileHandler::AllowModes(modes, currentParser->accessModes);

	CFileHandler fh(filename, modes);
	if (!fh.FileExists()) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf),
		         "Include() file missing '%s'\n", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	string code;
	if (!fh.LoadStringData(code)) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf),
		         "Include() could not load '%s'\n", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	int error = luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str());
	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	// set the chunk's fenv to the current fenv, or a user table
	if (lua_istable(L, 2)) {
		lua_pushvalue(L, 2); // user fenv
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __FUNCTION__);
	}

	// set the include fenv to the current function's fenv
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "Include(): error with setfenv");
	}

	const int paramTop = lua_gettop(L) - 1;

	error = lua_pcall(L, 0, LUA_MULTRET, 0);

	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	currentParser->accessedFiles.insert(StringToLower(filename));

	return lua_gettop(L) - paramTop;
}


/******************************************************************************/

int LuaParser::LoadFile(lua_State* L)
{
	if (currentParser == NULL) {
		luaL_error(L, "invalid call to LoadFile() after execution");
	}

	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		return 0;
	}
	string modes = luaL_optstring(L, 2, currentParser->accessModes.c_str());
	modes = CFileHandler::AllowModes(modes, currentParser->accessModes);

	CFileHandler fh(filename, modes);
	if (!fh.FileExists()) {
		lua_pushnil(L);
		lua_pushstring(L, "missing file");
		return 2;
	}
	string data;
	if (!fh.LoadStringData(data)) {
		lua_pushnil(L);
		lua_pushstring(L, "could not load data");
		return 2;
	}
	lua_pushstring(L, data.c_str());

	currentParser->accessedFiles.insert(StringToLower(filename));

	return 1;
}


int LuaParser::FileExists(lua_State* L)
{
	if (currentParser == NULL) {
		luaL_error(L, "invalid call to FileExists() after execution");
	}
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		return 0;
	}
	CFileHandler fh(filename, currentParser->accessModes);
	lua_pushboolean(L, fh.FileExists());
	return 1;
}


int LuaParser::DontMessWithMyCase(lua_State* L)
{
	if (currentParser == NULL) {
		luaL_error(L, "invalid call to DontMessWithMyCase() after execution");
	}
	currentParser->SetLowerKeys(lua_toboolean(L, 1));
	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaTable
//

LuaTable::LuaTable()
: path(""),
  isValid(false),
  parser(NULL),
  L(NULL),
  refnum(LUA_NOREF)
{
}


LuaTable::LuaTable(LuaParser* _parser)
{
	assert(_parser != NULL);

	isValid = _parser->IsValid();
	path    = "ROOT";
	parser  = _parser;
	L       = parser->L;
	refnum  = parser->rootRef;

	if (PushTable()) {
		lua_pushvalue(L, -1); // copy
		refnum = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
	 	refnum = LUA_NOREF;
	}
	isValid = (refnum != LUA_NOREF);

	parser->AddTable(this);
}


LuaTable::LuaTable(const LuaTable& tbl)
{
	parser = tbl.parser;
	L      = tbl.L;
	path   = tbl.path;

	if (tbl.PushTable()) {
		lua_pushvalue(L, -1); // copy
		refnum = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
		refnum = LUA_NOREF;
	}
	isValid = (refnum != LUA_NOREF);

	if (parser) {
		parser->AddTable(this);
	}
}


LuaTable& LuaTable::operator=(const LuaTable& tbl)
{
	if (parser && (refnum != LUA_NOREF) && (parser->currentRef == refnum)) {
		lua_settop(L, 0);
		parser->currentRef = LUA_NOREF;
	}

	if (parser != tbl.parser) {
		if (parser != NULL) {
			parser->RemoveTable(this);
		}
		if (L && (refnum != LUA_NOREF)) {
			luaL_unref(L, LUA_REGISTRYINDEX, refnum);
		}
		parser = tbl.parser;
		if (parser != NULL) {
			parser->AddTable(this);
		}
	}

	L    = tbl.L;
	path = tbl.path;

	if (tbl.PushTable()) {
		lua_pushvalue(L, -1); // copy
		refnum = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
		refnum = LUA_NOREF;
	}

	isValid = (refnum != LUA_NOREF);

	return *this;
}


LuaTable LuaTable::SubTable(int key) const
{
	LuaTable subTable;
	char buf[32];
	SNPRINTF(buf, 32, "[%i]", key);
	subTable.path = path + buf;

	if (!PushTable()) {
		return subTable;
	}

	lua_pushnumber(L, key);
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return subTable;
	}

	subTable.parser  = parser;
	subTable.L       = L;
	subTable.refnum  = luaL_ref(L, LUA_REGISTRYINDEX);
	subTable.isValid = (subTable.refnum != LUA_NOREF);

	parser->AddTable(&subTable);

	return subTable;
}


LuaTable LuaTable::SubTable(const string& mixedKey) const
{
	
	const string key = !(parser ? parser->lowerCppKeys : true) ? mixedKey : StringToLower(mixedKey);

	LuaTable subTable;
	subTable.path = path + "." + key;

	if (!PushTable()) {
		return subTable;
	}

	lua_pushstring(L, key.c_str());
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return subTable;
	}

	subTable.parser  = parser;
	subTable.L       = L;
	subTable.refnum  = luaL_ref(L, LUA_REGISTRYINDEX);
	subTable.isValid = (subTable.refnum != LUA_NOREF);

	parser->AddTable(&subTable);

	return subTable;
}


LuaTable LuaTable::SubTableExpr(const string& expr) const
{
	if (expr.empty()) {
		return LuaTable(*this);
	}
	if (!isValid) {
		return LuaTable();
	}

	string::size_type endPos;
	LuaTable nextTable;

	if (expr[0] == '[') { // numeric key
    endPos = expr.find(']');
    if (endPos == string::npos) {
      return LuaTable(); // missing brace
    }
    const char* startPtr = expr.c_str() + 1; // skip the '['
    char* endPtr;
    const int index = strtol(startPtr, &endPtr, 10);
    if (endPtr == startPtr) {
      return LuaTable(); // invalid index
    }
    endPos++; // eat the ']'
    nextTable = SubTable(index);
	}
	else { // string key
		endPos = expr.find_first_of(".[");
		if (endPos == string::npos) {
			return SubTable(expr);
		}
		nextTable = SubTable(expr.substr(0, endPos));
	}

	if (expr[endPos] == '.') {
		endPos++; // eat the dot
	}
	return nextTable.SubTableExpr(expr.substr(endPos));
}


LuaTable::~LuaTable()
{
	if (L && (refnum != LUA_NOREF)) {
		luaL_unref(L, LUA_REGISTRYINDEX, refnum);
		if (parser && (parser->currentRef == refnum)) {
			lua_settop(L, 0);
			parser->currentRef = LUA_NOREF;
		}
	}
	if (parser) {
		parser->RemoveTable(this);
	}
}


/******************************************************************************/

bool LuaTable::PushTable() const
{
	if (!isValid) {
		return false;
	}

	if ((refnum != LUA_NOREF) && (parser->currentRef == refnum)) {
		if (!lua_istable(L, -1)) {
			logOutput.Print("Internal Error: LuaTable::PushTable() = %s\n",
			                path.c_str());
			parser->currentRef = LUA_NOREF;
			lua_settop(L, 0);
			return false;
		}
		return true;
	}

	lua_settop(L, 0);

	lua_rawgeti(L, LUA_REGISTRYINDEX, refnum);
	if (!lua_istable(L, -1)) {
		isValid = false;
		parser->currentRef = LUA_NOREF;
		lua_settop(L, 0);
		return false;
	}

	parser->currentRef = refnum;

	return true;
}


bool LuaTable::PushValue(int key) const
{
	if (!PushTable()) {
		return false;
	}
	lua_pushnumber(L, key);
	lua_gettable(L, -2);
	if (lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	return true;
}


bool LuaTable::PushValue(const string& mixedKey) const
{
	const string key = !(parser ? parser->lowerCppKeys : true) ? mixedKey : StringToLower(mixedKey);
	if (!PushTable()) {
		return false;
	}
	lua_pushstring(L, key.c_str());
	lua_gettable(L, -2);
	if (lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Key existance testing
//

bool LuaTable::KeyExists(int key) const
{
	if (!PushValue(key)) {
		return false;
	}
	lua_pop(L, 1);
	return true;
}


bool LuaTable::KeyExists(const string& key) const
{
	if (!PushValue(key)) {
		return false;
	}
	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Value types
//

int LuaTable::GetType(int key) const
{
	if (!PushValue(key)) {
		return -1;
	}
	const int type = lua_type(L, -1);
	lua_pop(L, 1);
	return type;
}


int LuaTable::GetType(const string& key) const
{
	if (!PushValue(key)) {
		return -1;
	}
	const int type = lua_type(L, -1);
	lua_pop(L, 1);
	return type;
}


/******************************************************************************/
/******************************************************************************/
//
//  Object lengths
//

int LuaTable::GetLength() const
{
	if (!PushTable()) {
		return 0;
	}
	return lua_objlen(L, -1);
}


int LuaTable::GetLength(int key) const
{
	if (!PushValue(key)) {
		return 0;
	}
	const int len = lua_objlen(L, -1);
	lua_pop(L, 1);
	return len;
}


int LuaTable::GetLength(const string& key) const
{
	if (!PushValue(key)) {
		return 0;
	}
	const int len = lua_objlen(L, -1);
	lua_pop(L, 1);
	return len;
}


/******************************************************************************/
/******************************************************************************/
//
//  Key list functions
//

bool LuaTable::GetKeys(vector<int>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			const int value = lua_toint(L, -2);
			data.push_back(value);
		}
	}
	std::sort(data.begin(), data.end());
	return true;
}


bool LuaTable::GetKeys(vector<string>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const string value = lua_tostring(L, -2);
			data.push_back(value);
		}
	}
	std::sort(data.begin(), data.end());
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Map functions
//

bool LuaTable::GetMap(map<int, float>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {
			const int   key   =   lua_toint(L, -2);
			const float value = lua_tofloat(L, -1);
			data[key] = value;
		}
	}
	return true;
}


bool LuaTable::GetMap(map<int, string>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isstring(L, -1)) {
			const int    key   = lua_toint(L, -2);
			const string value = lua_tostring(L, -1);
			data[key] = value;
		}
	}
	return true;
}


bool LuaTable::GetMap(map<string, float>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
			const string key   = lua_tostring(L, -2);
			const float  value = lua_tofloat(L, -1);
			data[key] = value;
		}
	}
	return true;
}


bool LuaTable::GetMap(map<string, string>& data) const
{
	if (!PushTable()) {
		return false;
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2) && lua_isstring(L, -1)) {
			const string key   = lua_tostring(L, -2);
			const string value = lua_tostring(L, -1);
			data[key] = value;
		}
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing utilities
//

static bool ParseTableFloat(lua_State* L,
                            int tableIndex, int index, float& value)
{
	lua_pushnumber(L, index);
	lua_gettable(L, tableIndex);
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	value = lua_tofloat(L, -1);
	lua_pop(L, 1);
	return true;
}


static bool ParseFloat3(lua_State* L, int index, float3& value)
{
	if (lua_istable(L, index)) {
		const int table = (index > 0) ? index : lua_gettop(L) + index + 1;
		if (ParseTableFloat(L, table, 1, value.x) &&
		    ParseTableFloat(L, table, 2, value.y) &&
		    ParseTableFloat(L, table, 3, value.z)) {
			return true;
		}
	}
	else if (lua_isstring(L, index)) {
		const int count = sscanf(lua_tostring(L, index), "%f %f %f",
		                         &value.x, &value.y, &value.z);
		if (count == 3) {
			return true;
		}
	}
	return false;
}


static bool ParseBoolean(lua_State* L, int index, bool& value)
{
	if (lua_isboolean(L, index)) {
		value = lua_toboolean(L, index);
		return true;
	}
	else if (lua_isnumber(L, index)) {
		value = (lua_tofloat(L, index) != 0.0f);
		return true;
	}
	else if (lua_isstring(L, index)) {
		const string str = StringToLower(lua_tostring(L, index));
		if ((str == "1") || (str == "true")) {
			value = true;
			return true;
		}
		if ((str == "0") || (str == "false")) {
			value = false;
			return true;
		}
	}
	return false;
}


/******************************************************************************/
/******************************************************************************/
//
//  String key functions
//

int LuaTable::GetInt(const string& key, int def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const int value = lua_toint(L, -1);
	lua_pop(L, 1);
	return value;
}


bool LuaTable::GetBool(const string& key, bool def) const
{
	if (!PushValue(key)) {
		return def;
	}
	bool value;
	if (!ParseBoolean(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


float LuaTable::GetFloat(const string& key, float def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const float value = lua_tofloat(L, -1);
	lua_pop(L, 1);
	return value;
}


float3 LuaTable::GetFloat3(const string& key, const float3& def) const
{
	if (!PushValue(key)) {
		return def;
	}
	float3 value;
	if (!ParseFloat3(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


string LuaTable::GetString(const string& key, const string& def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const string value = lua_tostring(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/
/******************************************************************************/
//
//  Number key functions
//

int LuaTable::GetInt(int key, int def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const int value = lua_toint(L, -1);
	lua_pop(L, 1);
	return value;
}


bool LuaTable::GetBool(int key, bool def) const
{
	if (!PushValue(key)) {
		return def;
	}
	bool value;
	if (!ParseBoolean(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


float LuaTable::GetFloat(int key, float def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const float value = lua_tofloat(L, -1);
	lua_pop(L, 1);
	return value;
}


float3 LuaTable::GetFloat3(int key, const float3& def) const
{
	if (!PushValue(key)) {
		return def;
	}
	float3 value;
	if (!ParseFloat3(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


string LuaTable::GetString(int key, const string& def) const
{
	if (!PushValue(key)) {
		return def;
	}
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const string value = lua_tostring(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/
/******************************************************************************/
