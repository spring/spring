/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaParser.h"

#include <algorithm>
#include <climits>

#include "lib/streflop/streflop_cond.h"

#include "System/float3.h"
#include "System/float4.h"
#include "LuaInclude.h"

#include "LuaConstGame.h"
#include "LuaConstEngine.h"
#include "LuaIO.h"
#include "LuaVFS.h"
#include "LuaUtils.h"

#include "Sim/Misc/GlobalSynced.h" // gsRNG
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Misc/SpringTime.h"
#include "System/ContainerUtil.h"
#include "System/TimeProfiler.h"
#include "System/ScopedFPUSettings.h"
#include "System/StringUtil.h"

LuaParser* GetLuaParser(lua_State* L) {
	assert(GetLuaContextData(L)->parser != nullptr);
	return GetLuaContextData(L)->parser;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaParser
//

LuaParser::LuaParser(const std::string& _fileName, const std::string& _fileModes, const std::string& _accessModes, const boolean& synced, const boolean& setup)
	: fileName(_fileName)
	, fileModes(_fileModes)
	, accessModes(_accessModes)

	, D(false, false)

	, initDepth(0)
	, rootRef(LUA_NOREF)
	, currentRef(LUA_NOREF)

	, lowerKeys(true)
	, lowerCppKeys(true)
{
	// be on the safe side
	// D.synced = synced.b;
	D.synced = true;
	D.parser = this;

	if (!setup.b)
		return;

	SetupLua(synced.b, false);
}

LuaParser::LuaParser(const std::string& _textChunk, const std::string& _accessModes, int, const boolean& synced, const boolean& setup)
	: textChunk(_textChunk)
	, accessModes(_accessModes)

	, D(false, false)

	, initDepth(0)
	, rootRef(LUA_NOREF)
	, currentRef(LUA_NOREF)

	, lowerKeys(true)
	, lowerCppKeys(true)
{
	// be on the safe side
	// D.synced = synced.b;
	D.synced = true;
	D.parser = this;

	if (!setup.b)
		return;

	SetupLua(synced.b, false);
}


LuaParser::~LuaParser()
{
	// prevent crashes in glDelete* calls since LuaParser
	// might be constructed by multiple different threads
	D.Clear();

	if (L != nullptr)
		LUA_CLOSE(&L);

	// invalidate leftover tables if parser is destroyed first
	for (LuaTable* table: tables) {
		table->parser  = nullptr;
		table->L       = nullptr;
		table->isValid = false;
		table->refnum  = LUA_NOREF;
	}
}


void LuaParser::SetupLua(bool isSyncedCtxt, bool isDefsParser)
{
	if ((L = LUA_OPEN(&D)) == nullptr)
		return;

	SetupEnv(isSyncedCtxt, isDefsParser);
}

void LuaParser::SetupEnv(bool isSyncedCtxt, bool isDefsParser)
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
	lua_pushnil(L); lua_setglobal(L, "newproxy"); // not sync-safe cause of __gc

	{
		lua_getglobal(L, "math");
		if (isSyncedCtxt) {
			LuaPushNamedCFunc(L, "random", Random);
			LuaPushNamedCFunc(L, "randomseed", RandomSeed);
		} else {
			LuaPushNamedCFunc(L, "random", DummyRandom);
			LuaPushNamedCFunc(L, "randomseed", DummyRandomSeed);
		}
		lua_pop(L, 1); // pop "math"
	}

	AddFunc("DontMessWithMyCase", DontMessWithMyCase);

	GetTable("Spring");
	AddFunc("Echo", LuaUtils::Echo);
	AddFunc("Log", LuaUtils::Log);
	AddFunc("TimeCheck", TimeCheck);
	EndTable();

	GetTable("Script");
	AddFunc("IsEngineMinVersion", LuaUtils::IsEngineMinVersion);
	EndTable();

	#if (!defined(UNITSYNC) && !defined(DEDICATED))
	if (isDefsParser) {
		// irrelevant for most LuaParsers except defsParser
		// adding it to others can cause infinite recursion
		// (e.g. when scanning generated virtual archives)
		GetTable("Game");
		LuaConstGame::PushEntries(L);
		EndTable();
	}
	#endif

	GetTable("Engine");
	LuaConstEngine::PushEntries(L);
	EndTable();

	GetTable("VFS");
	#if (!defined(UNITSYNC) && !defined(DEDICATED))
	// no LuaConstVFS, but this will do
	LuaVFS::PushCommon(L);
	#endif
	AddFunc("DirList",    DirList);
	AddFunc("SubDirs",    SubDirs);
	AddFunc("Include",    Include);
	AddFunc("LoadFile",   LoadFile);
	AddFunc("FileExists", FileExists);
	EndTable();

	GetTable("LOG");
	LuaUtils::PushLogEntries(L);
	EndTable();
}


/******************************************************************************/

bool LuaParser::Execute()
{
	if (!IsValid()) {
		errorLog = "could not initialize Lua library";
		return false;
	}

	assert(rootRef == LUA_NOREF);
	assert(initDepth == 0);
	initDepth = -1;

	std::string code;
	std::string codeLabel;

	if (!textChunk.empty()) {
		code = textChunk;
		codeLabel = "text chunk";
	}
	else if (!fileName.empty()) {
		CFileHandler fh(codeLabel = fileName, fileModes);

		if (!fh.LoadStringData(code)) {
			errorLog = "could not open file: " + fileName;

			LUA_CLOSE(&L);
			return false;
		}
	}
	else {
		errorLog = "invalid format or empty file";

		LUA_CLOSE(&L);
		return false;
	}

	char errorBuf[4096] = {0};
	int errorNum = 0;

	if ((errorNum = luaL_loadbuffer(L, code.c_str(), code.size(), codeLabel.c_str())) != 0) {
		SNPRINTF(errorBuf, sizeof(errorBuf), "[loadbuf] error %d (\"%s\") in %s", errorNum, lua_tostring(L, -1), codeLabel.c_str());
		LUA_CLOSE(&L);

		errorLog = errorBuf;
		return false;
	}

	{
		// LuaParser::Execute can be called concurrently by the
		// game-load and (e.g.) assimp model preloading threads

		// do not signal floating point exceptions in user Lua code
		ScopedDisableFpuExceptions fe;

		if ((errorNum = lua_pcall(L, 0, 1, 0)) != 0) {
			SNPRINTF(errorBuf, sizeof(errorBuf), "[pcall] error %d (\"%s\") in %s", errorNum, lua_tostring(L, -1), fileName.c_str());
			LUA_CLOSE(&L);

			errorLog = errorBuf;
			return false;
		}

		#if 0
		if (!lua_istable(L, 1)) {
			errorLog = "no return table from " + fileName;

			LUA_CLOSE(&L);
			return false;
		}
		#else
		// make not returning a table (which may or may not be intentional)
		// equivalent to returning an empty one; leave it to callers whether
		// they consider this an error
		if (!lua_istable(L, 1)) {
			lua_pop(L, lua_gettop(L));
			lua_newtable(L);

			errorLog = "no return table from " + fileName;
		}
		#endif

		if (lowerKeys)
			LuaUtils::LowerKeys(L, 1);

		LuaUtils::CheckTableForNaNs(L, 1, fileName);
	}

	rootRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_settop(L, 0);

	return (valid = true);
}


void LuaParser::AddTable(LuaTable* tbl) { spring::VectorInsertUnique(tables, tbl); }
void LuaParser::RemoveTable(LuaTable* tbl) { spring::VectorErase(tables, tbl); }


LuaTable LuaParser::GetRoot()
{
	return LuaTable(this);
}


/******************************************************************************/

void LuaParser::PushParam()
{
	if (!IsValid() || (initDepth < 0))
		return;

	if (initDepth > 0) {
		lua_rawset(L, -3);
	} else {
		lua_rawset(L, LUA_GLOBALSINDEX);
	}
}


void LuaParser::GetTable(const std::string& name, bool overwrite)
{
	if (!IsValid() || (initDepth < 0))
		return;

	lua_pushsstring(L, name);

	if (overwrite) {
		lua_newtable(L);
	} else {
		lua_pushsstring(L, name);
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
	if (!IsValid() || (initDepth < 0))
		return;

	lua_pushnumber(L, index);

	if (overwrite) {
		lua_newtable(L);
	} else {
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
	if (!IsValid() || (initDepth < 0))
		return;
	assert(initDepth > 0);
	initDepth--;
	PushParam();
}


/******************************************************************************/

void LuaParser::AddFunc(const std::string& key, int (*func)(lua_State*))
{
	if (!IsValid() || (initDepth < 0))
		return;
	if (func == nullptr)
		return;

	// can not use this; initDepth check in PushParam
	// LuaPushNamedCFunc(L, key, func);
	lua_pushsstring(L, key);
	lua_pushcfunction(L, func);

	// t[k] = v
	PushParam();
}


void LuaParser::AddInt(const std::string& key, int value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushsstring(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddBool(const std::string& key, bool value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushsstring(L, key);
	lua_pushboolean(L, value);
	PushParam();
}


void LuaParser::AddFloat(const std::string& key, float value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushsstring(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddString(const std::string& key, const std::string& value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushsstring(L, key);
	lua_pushsstring(L, value);
	PushParam();
}


/******************************************************************************/

void LuaParser::AddFunc(int key, int (*func)(lua_State*))
{
	if (!IsValid() || (initDepth < 0))
		return;
	if (func == nullptr)
		return;

	lua_pushnumber(L, key);
	lua_pushcfunction(L, func);

	PushParam();
}


void LuaParser::AddInt(int key, int value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushnumber(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddBool(int key, bool value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushnumber(L, key);
	lua_pushboolean(L, value);
	PushParam();
}


void LuaParser::AddFloat(int key, float value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushnumber(L, key);
	lua_pushnumber(L, value);
	PushParam();
}


void LuaParser::AddString(int key, const std::string& value)
{
	if (!IsValid() || (initDepth < 0))
		return;
	lua_pushnumber(L, key);
	lua_pushsstring(L, value);
	PushParam();
}


/******************************************************************************/
/******************************************************************************/
//
//  call-outs
//

int LuaParser::TimeCheck(lua_State* L)
{
	#if (!defined(UNITSYNC) && !defined(DEDICATED))
	if (!lua_isstring(L, 1) || !lua_isfunction(L, 2))
		luaL_error(L, "Invalid arguments to TimeCheck('string', func, ...)");

	{
		ScopedOnceTimer timer(lua_tostring(L, 1));

		lua_remove(L, 1);

		if (lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0) != 0) {
			const std::string errmsg = lua_tostring(L, -1);

			lua_pop(L, 1);
			luaL_error(L, errmsg.c_str());
		}
	}

	return lua_gettop(L);
	#else
	return 0;
	#endif
}


/******************************************************************************/

// seeding makes little sense inside LuaParser execution
int LuaParser::RandomSeed(lua_State* L) { return (DummyRandomSeed(L)); }
int LuaParser::Random(lua_State* L)
{
	// both US and DS depend on LuaParser via MapParser, etc
	#if (!defined(UNITSYNC) && !defined(DEDICATED))
	lua_pushnumber(L, gsRNG.NextFloat());
	return 1;
	#else
	return (DummyRandom(L));
	#endif
}

int LuaParser::DummyRandomSeed(lua_State* L) { return 0; }
int LuaParser::DummyRandom(lua_State* L) { return 0; }

/******************************************************************************/

int LuaParser::DirList(lua_State* L)
{
	const LuaParser* currentParser = GetLuaParser(L);

	const std::string& dir = luaL_checkstring(L, 1);

	if (!LuaIO::IsSimplePath(dir))
		return 0;

	const std::string& pat = luaL_optstring(L, 2, "*");
	const std::string& modes = CFileHandler::AllowModes(luaL_optstring(L, 3, currentParser->accessModes.c_str()), currentParser->accessModes);

	LuaUtils::PushStringVector(L, CFileHandler::DirList(dir, pat, modes));
	return 1;
}


int LuaParser::SubDirs(lua_State* L)
{
	const LuaParser* currentParser = GetLuaParser(L);

	const std::string& dir = luaL_checkstring(L, 1);

	if (!LuaIO::IsSimplePath(dir))
		return 0;

	const std::string& pat = luaL_optstring(L, 2, "*");
	const std::string& modes = CFileHandler::AllowModes(luaL_optstring(L, 3, currentParser->accessModes.c_str()), currentParser->accessModes);

	LuaUtils::PushStringVector(L, CFileHandler::SubDirs(dir, pat, modes));
	return 1;
}

/******************************************************************************/

int LuaParser::Include(lua_State* L)
{
	const LuaParser* currentParser = GetLuaParser(L);

	// filename [, fenv]
	const std::string& filename = luaL_checkstring(L, 1);

	if (!LuaIO::IsSimplePath(filename))
		luaL_error(L, "bad pathname");

	const std::string& modes = CFileHandler::AllowModes(luaL_optstring(L, 3, currentParser->accessModes.c_str()), currentParser->accessModes);

	CFileHandler fh(filename, modes);
	if (!fh.FileExists()) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Include() file missing '%s'\n", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	std::string code;
	if (!fh.LoadStringData(code)) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Include() could not load '%s'\n", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	int error = luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str());
	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n", error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	// set the chunk's fenv to the current fenv, or a user table
	if (lua_istable(L, 2)) {
		lua_pushvalue(L, 2); // user fenv
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __func__);
	}

	// set the include fenv to the current function's fenv
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "Include(): error with setfenv");
	}

	const int paramTop = lua_gettop(L) - 1;

	error = lua_pcall(L, 0, LUA_MULTRET, 0);

	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n", error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	#if 0
	spring::VectorInsertUnique(currentParser->accessedFiles, StringToLower(filename), true);
	#endif
	return (lua_gettop(L) - paramTop);
}


/******************************************************************************/

int LuaParser::LoadFile(lua_State* L)
{
	const LuaParser* currentParser = GetLuaParser(L);

	const std::string& filename = luaL_checkstring(L, 1);

	if (!LuaIO::IsSimplePath(filename))
		return 0;

	const std::string& modes = CFileHandler::AllowModes(luaL_optstring(L, 2, currentParser->accessModes.c_str()), currentParser->accessModes);

	CFileHandler fh(filename, modes);
	if (!fh.FileExists()) {
		lua_pushnil(L);
		lua_pushstring(L, "missing file");
		return 2;
	}
	std::string data;
	if (!fh.LoadStringData(data)) {
		lua_pushnil(L);
		lua_pushstring(L, "could not load data");
		return 2;
	}
	lua_pushstring(L, data.c_str());

	#if 0
	spring::VectorInsertUnique(currentParser->accessedFiles, StringToLower(filename), true);
	#endif
	return 1;
}


int LuaParser::FileExists(lua_State* L)
{
	const LuaParser* currentParser = GetLuaParser(L);

	const std::string& filename = luaL_checkstring(L, 1);

	if (!LuaIO::IsSimplePath(filename))
		return 0;

	lua_pushboolean(L, CFileHandler::FileExists(filename, currentParser->accessModes));
	return 1;
}


int LuaParser::DontMessWithMyCase(lua_State* L)
{
	LuaParser* currentParser = GetLuaParser(L);

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
  parser(nullptr),
  L(nullptr),
  refnum(LUA_NOREF)
{
}


LuaTable::LuaTable(LuaParser* _parser)
{
	assert(_parser != nullptr);

	isValid = _parser->IsValid();
	path    = "ROOT";
	parser  = _parser;
	L       = parser->L;
	refnum  = parser->rootRef;

	parser->AddTable(this);

	if (PushTable()) {
		lua_pushvalue(L, -1); // copy
		refnum = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
	 	refnum = LUA_NOREF;
	}
	isValid = (refnum != LUA_NOREF);
}


LuaTable::LuaTable(const LuaTable& tbl)
{
	parser = tbl.parser;
	L      = tbl.L;
	path   = tbl.path;

	if (parser != nullptr)
		parser->AddTable(this);

	if (tbl.PushTable()) {
		lua_pushvalue(L, -1); // copy
		refnum = luaL_ref(L, LUA_REGISTRYINDEX);
	} else {
		refnum = LUA_NOREF;
	}
	isValid = (refnum != LUA_NOREF);
}


LuaTable& LuaTable::operator=(const LuaTable& tbl)
{
	if (parser != nullptr && (refnum != LUA_NOREF) && (parser->currentRef == refnum)) {
		lua_settop(L, 0);
		parser->currentRef = LUA_NOREF;
	}

	if (parser != tbl.parser) {
		if (parser != nullptr)
			parser->RemoveTable(this);

		if (L != nullptr && (refnum != LUA_NOREF))
			luaL_unref(L, LUA_REGISTRYINDEX, refnum);

		if (tbl.parser != nullptr)
			tbl.parser->AddTable(this);

		parser = tbl.parser;
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

	if (!PushTable())
		return subTable;

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


LuaTable LuaTable::SubTable(const std::string& mixedKey) const
{
	const std::string key = !((parser != nullptr)? parser->lowerCppKeys : true) ? mixedKey : StringToLower(mixedKey);

	LuaTable subTable;
	subTable.path = path + "." + key;

	if (!PushTable())
		return subTable;

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


LuaTable LuaTable::SubTableExpr(const std::string& expr) const
{
	if (expr.empty())
		return LuaTable(*this);

	if (!isValid)
		return LuaTable();

	std::string::size_type endPos;
	LuaTable nextTable;

	if (expr[0] == '[') { // numeric key
		if ((endPos = expr.find(']')) == std::string::npos)
			return LuaTable(); // missing brace

		const char* startPtr = expr.c_str() + 1; // skip the '['
		char* endPtr;
		const int index = strtol(startPtr, &endPtr, 10);

		if (endPtr == startPtr)
			return LuaTable(); // invalid index

		endPos++; // eat the ']'
		nextTable = SubTable(index);
	} else { // string key
		if ((endPos = expr.find_first_of(".[")) == std::string::npos)
			return SubTable(expr);

		nextTable = SubTable(expr.substr(0, endPos));
	}

	if (expr[endPos] == '.') {
		endPos++; // eat the dot
	}
	return nextTable.SubTableExpr(expr.substr(endPos));
}


LuaTable::~LuaTable()
{
	if (parser != nullptr)
		parser->RemoveTable(this);

	if (L != nullptr && (refnum != LUA_NOREF)) {
		luaL_unref(L, LUA_REGISTRYINDEX, refnum);

		if (parser != nullptr && (parser->currentRef == refnum)) {
			lua_settop(L, 0);
			parser->currentRef = LUA_NOREF;
		}
	}
}


/******************************************************************************/

bool LuaTable::PushTable() const
{
	if (!isValid)
		return false;

	if ((refnum != LUA_NOREF) && (parser->currentRef == refnum)) {
		if (!lua_istable(L, -1)) {
			LOG_L(L_ERROR, "Internal Error: LuaTable::PushTable() = %s", path.c_str());
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


bool LuaTable::PushValue(const std::string& mixedKey) const
{
	const std::string key = !(parser ? parser->lowerCppKeys : true) ? mixedKey : StringToLower(mixedKey);

	if (!PushTable())
		return false;

	const int top = lua_gettop(L);

	if (key.find('.') != std::string::npos) {
		// nested key (e.g. "subtable.subsub.mahkey")
		size_t lastpos = 0;
		size_t dotpos = key.find('.');

		lua_pushvalue(L, -1);
		do {
			const std::string subTableName = key.substr(lastpos, dotpos);
			lastpos = dotpos + 1;
			dotpos = key.find('.', lastpos);

			lua_pushsstring(L, subTableName);
			lua_gettable(L, -2);
			if (!lua_istable(L, -1)) {
				lua_pop(L, 2);
				const int newtop = lua_gettop(L);
				assert(newtop == top);
				return false;
			}
			lua_remove(L, -2);
		} while (dotpos != std::string::npos);

		const std::string keyname = key.substr(lastpos);

		// try as string
		lua_pushsstring(L, keyname);
		lua_gettable(L, -2);
		if (!lua_isnoneornil(L, -1)) {
			lua_remove(L, -2);
			const int newtop = lua_gettop(L);
			assert(newtop == top + 1);
			return true;
		}

		// try as integer
		bool failed;
		int i = StringToInt(keyname, &failed);
		if (failed) {
			lua_pop(L, 2);
			const int newtop = lua_gettop(L);
			assert(newtop == top);
			return false;
		}
		lua_pop(L, 1); // pop nil
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		if (!lua_isnoneornil(L, -1)) {
			lua_remove(L, -2);
			const int newtop = lua_gettop(L);
			assert(newtop == top + 1);
			return true;
		}
		lua_pop(L, 2);
		const int newtop = lua_gettop(L);
		assert(newtop == top);
		return false;
	}

	lua_pushsstring(L, key);
	lua_gettable(L, -2);
	if (!lua_isnoneornil(L, -1)) {
		const int newtop = lua_gettop(L);
		assert(newtop == top + 1);
		return true;
	}
	lua_pop(L, 1);
	const int newtop = lua_gettop(L);
	assert(newtop == top);
	return false;
}


/******************************************************************************/
/******************************************************************************/
//
//  Key existance testing
//

bool LuaTable::KeyExists(int key) const
{
	if (!PushValue(key))
		return false;

	lua_pop(L, 1);
	return true;
}


bool LuaTable::KeyExists(const std::string& key) const
{
	if (!PushValue(key))
		return false;

	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Value types
//

LuaTable::DataType LuaTable::GetType(int key) const
{
	if (!PushValue(key))
		return NIL;

	const int type = lua_type(L, -1);
	lua_pop(L, 1);

	switch (type) {
		case LUA_TBOOLEAN: return BOOLEAN;
		case LUA_TNUMBER:  return NUMBER;
		case LUA_TSTRING:  return STRING;
		case LUA_TTABLE:   return TABLE;
		default:           return NIL;
	}
}


LuaTable::DataType LuaTable::GetType(const std::string& key) const
{
	if (!PushValue(key))
		return NIL;

	const int type = lua_type(L, -1);
	lua_pop(L, 1);

	switch (type) {
		case LUA_TBOOLEAN: return BOOLEAN;
		case LUA_TNUMBER:  return NUMBER;
		case LUA_TSTRING:  return STRING;
		case LUA_TTABLE:   return TABLE;
		default:           return NIL;
	}
}


/******************************************************************************/
/******************************************************************************/
//
//  Object lengths
//

int LuaTable::GetLength() const
{
	if (!PushTable())
		return 0;

	return lua_objlen(L, -1);
}


int LuaTable::GetLength(int key) const
{
	if (!PushValue(key))
		return 0;

	const int len = lua_objlen(L, -1);
	lua_pop(L, 1);
	return len;
}


int LuaTable::GetLength(const std::string& key) const
{
	if (!PushValue(key))
		return 0;

	const int len = lua_objlen(L, -1);
	lua_pop(L, 1);
	return len;
}


/******************************************************************************/
/******************************************************************************/
//
//  Key list functions
//

bool LuaTable::GetKeys(std::vector<int>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2))
			continue;

		data.push_back(lua_toint(L, -2));
	}

	std::stable_sort(data.begin(), data.end());
	return true;
}

bool LuaTable::GetKeys(std::vector<std::string>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2))
			data.emplace_back(lua_tostring(L, -2));
	}

	std::stable_sort(data.begin(), data.end());
	return true;
}



bool LuaTable::GetPairs(std::vector<std::pair<int, std::string>>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1))
			continue;

		if (lua_isstring(L, -1)) {
			data.emplace_back(lua_toint(L, -2), lua_tostring(L, -1));
			continue;
		}
		if (lua_isboolean(L, -1)) {
			data.emplace_back(lua_toint(L, -2), lua_toboolean(L, -1) ? "1" : "0");
			continue;
		}
	}

	using T = std::remove_reference<decltype(data)>::type;
	using P = T::value_type;

	std::stable_sort(data.begin(), data.end(), [](const P& a, const P& b) { return (a.first < b.first); });
	return true;
}

bool LuaTable::GetPairs(std::vector<std::pair<std::string, float>>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
			continue;

		data.emplace_back(lua_tostring(L, -2), lua_tonumber(L, -1));
	}

	using T = std::remove_reference<decltype(data)>::type;
	using P = T::value_type;

	std::stable_sort(data.begin(), data.end(), [](const P& a, const P& b) { return (a.first < b.first); });
	return true;
}

bool LuaTable::GetPairs(std::vector<std::pair<std::string, std::string>>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		if (lua_isstring(L, -1)) { // includes numbers
			data.emplace_back(lua_tostring(L, -2), lua_tostring(L, -1));
			continue;
		}
		if (lua_isboolean(L, -1)) {
			data.emplace_back(lua_tostring(L, -2), lua_toboolean(L, -1) ? "1" : "0");
			continue;
		}
	}

	using T = std::remove_reference<decltype(data)>::type;
	using P = T::value_type;

	std::stable_sort(data.begin(), data.end(), [](const P& a, const P& b) { return (a.first < b.first); });
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Map functions
//

bool LuaTable::GetMap(spring::unordered_map<int, float>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_isnumber(L, -1))
			continue;

		data[lua_toint(L, -2)] = lua_tonumber(L, -1);
	}

	return true;
}

bool LuaTable::GetMap(spring::unordered_map<int, std::string>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1))
			continue;

		if (lua_isstring(L, -1)) {
			data[lua_toint(L, -2)] = lua_tostring(L, -1);
			continue;
		}
		if (lua_isboolean(L, -1)) {
			data[lua_toint(L, -2)] = lua_toboolean(L, -1) ? "1" : "0";
			continue;
		}
	}

	return true;
}

bool LuaTable::GetMap(spring::unordered_map<std::string, float>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
			continue;

		data[lua_tostring(L, -2)] = lua_tonumber(L, -1);
	}

	return true;
}

bool LuaTable::GetMap(spring::unordered_map<std::string, std::string>& data) const
{
	if (!PushTable())
		return false;

	const int table = lua_gettop(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		if (lua_isstring(L, -1)) { // includes numbers
			data[lua_tostring(L, -2)] = lua_tostring(L, -1);
			continue;
		}
		if (lua_isboolean(L, -1)) {
			data[lua_tostring(L, -2)] = lua_toboolean(L, -1) ? "1" : "0";
			continue;
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
	value = lua_tonumber(L, -1);
	if (unlikely(value == 0) && !lua_isnumber(L, -1) && !lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
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
		if (sscanf(lua_tostring(L, index), "%f %f %f", &value.x, &value.y, &value.z) == 3)
			return true;
	}

	return false;
}

static bool ParseFloat4(lua_State* L, int index, float4& value)
{
	if (lua_istable(L, index)) {
		const int table = (index > 0) ? index : lua_gettop(L) + index + 1;
		if (ParseTableFloat(L, table, 1, value.x) &&
		    ParseTableFloat(L, table, 2, value.y) &&
		    ParseTableFloat(L, table, 3, value.z) &&
		    ParseTableFloat(L, table, 4, value.w)) {
			return true;
		}
	}
	else if (lua_isstring(L, index)) {
		if (sscanf(lua_tostring(L, index), "%f %f %f %f", &value.x, &value.y, &value.z, &value.w) == 4)
			return true;
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
		value = (lua_tonumber(L, index) != 0.0f);
		return true;
	}
	else if (lua_isstring(L, index)) {
		const std::string str = StringToLower(lua_tostring(L, index));
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

int LuaTable::Get(const std::string& key, int def) const
{
	if (!PushValue(key))
		return def;

	const int value = lua_toint(L, -1);
	if (unlikely(value == 0) && !lua_isnumber(L, -1) && !lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


bool LuaTable::Get(const std::string& key, bool def) const
{
	if (!PushValue(key))
		return def;

	bool value;
	if (!ParseBoolean(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


float LuaTable::Get(const std::string& key, float def) const
{
	if (!PushValue(key))
		return def;

	const float value = lua_tonumber(L, -1);
	if (unlikely(value == 0.f) && !lua_isnumber(L, -1) && !lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}



float3 LuaTable::Get(const std::string& key, const float3& def) const
{
	if (!PushValue(key))
		return def;

	float3 value;
	if (!ParseFloat3(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}

float4 LuaTable::Get(const std::string& key, const float4& def) const
{
	if (!PushValue(key))
		return def;

	float4 value;
	if (!ParseFloat4(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}



std::string LuaTable::Get(const std::string& key, const std::string& def) const
{
	if (!PushValue(key))
		return def;

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const std::string value = lua_tostring(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/
/******************************************************************************/
//
//  Number key functions
//

int LuaTable::Get(int key, int def) const
{
	if (!PushValue(key))
		return def;

	const int value = lua_toint(L, -1);
	if (unlikely(value == 0) && !lua_isnumber(L, -1) && !lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


bool LuaTable::Get(int key, bool def) const
{
	if (!PushValue(key))
		return def;

	bool value;
	if (!ParseBoolean(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}


float LuaTable::Get(int key, float def) const
{
	if (!PushValue(key))
		return def;

	const float value = lua_tonumber(L, -1);
	if (unlikely(value == 0) && !lua_isnumber(L, -1) && !lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}



float3 LuaTable::Get(int key, const float3& def) const
{
	if (!PushValue(key))
		return def;

	float3 value;
	if (!ParseFloat3(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}

float4 LuaTable::Get(int key, const float4& def) const
{
	if (!PushValue(key)) {
		return def;
	}
	float4 value;
	if (!ParseFloat4(L, -1, value)) {
		lua_pop(L, 1);
		return def;
	}
	lua_pop(L, 1);
	return value;
}



std::string LuaTable::Get(int key, const std::string& def) const
{
	if (!PushValue(key))
		return def;

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return def;
	}
	const std::string value = lua_tostring(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/
/******************************************************************************/
//
//  Number key functions
//

float3 LuaTable::GetFloat3(int key, const float3& def) const
{
	return Get(key, def);
}


float4 LuaTable::GetFloat4(int key, const float4& def) const
{
	return Get(key, def);
}


float3 LuaTable::GetFloat3(const std::string& key, const float3& def) const
{
	return Get(key, def);
}


float4 LuaTable::GetFloat4(const std::string& key, const float4& def) const
{
	return Get(key, def);
}



/******************************************************************************/
/******************************************************************************/
