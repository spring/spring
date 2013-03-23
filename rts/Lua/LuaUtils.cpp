/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"

#include <set>
#include <cctype>
#include <zlib.h>
#include <boost/cstdint.hpp>
#include <string.h>


#include "LuaUtils.h"

#include "System/Log/ILog.h"
#include "System/Util.h"
#include "LuaConfig.h"
#include <boost/thread/recursive_mutex.hpp>

static const int maxDepth = 256;
int backupSize = 0;


boost::recursive_mutex luaprimmutex, luasecmutex;

boost::recursive_mutex* getLuaMutex(bool userMode, bool primary) {
#if (LUA_MT_OPT & LUA_MUTEX)
	if(userMode)
		return new boost::recursive_mutex();
	else // LuaGaia & LuaRules will share mutexes to avoid deadlocks during XCalls etc.
		return primary ? &luaprimmutex : &luasecmutex;
#else
		return &luaprimmutex;
#endif
}

/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth);


static inline int PosLuaIndex(lua_State* src, int index)
{
	if (index > 0) {
		return index;
	} else {
		return (lua_gettop(src) + index + 1);
	}
}


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth)
{
	const int type = lua_type(src, index);
	switch (type) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, lua_toboolean(src, index));
			break;
		}
		case LUA_TNUMBER: {
			lua_pushnumber(dst, lua_tonumber(src, index));
			break;
		}
		case LUA_TSTRING: {
			size_t len;
			const char* data = lua_tolstring(src, index, &len);
			lua_pushlstring(dst, data, len);
			break;
		}
		case LUA_TTABLE: {
			CopyPushTable(dst, src, index, depth);
			break;
		}
		default: {
			lua_pushnil(dst); // unhandled type
			return false;
		}
	}
	return true;
}


static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth)
{
	if (depth++ > maxDepth) {
		lua_pushnil(dst); // push something
		return false;
	}

	lua_newtable(dst);
	const int table = PosLuaIndex(src, index);
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		CopyPushData(dst, src, -2, depth); // copy the key
		CopyPushData(dst, src, -1, depth); // copy the value
		lua_rawset(dst, -3);
	}

	return true;
}


int LuaUtils::CopyData(lua_State* dst, lua_State* src, int count)
{
	const int srcTop = lua_gettop(src);
	const int dstTop = lua_gettop(dst);
	if (srcTop < count) {
		return 0;
	}
	lua_checkstack(dst, count); // FIXME: not enough for table chains

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		CopyPushData(dst, src, i, 0);
	}
	lua_settop(dst, dstTop + count);

	return count;
}

/******************************************************************************/
/******************************************************************************/

static bool BackupData(LuaUtils::DataDump &d, lua_State* src, int index, int depth);
static bool RestoreData(const LuaUtils::DataDump &d, lua_State* dst, int depth);
static bool BackupTable(LuaUtils::DataDump &d, lua_State* src, int index, int depth);
static bool RestoreTable(const LuaUtils::DataDump &d, lua_State* dst, int depth);


static bool BackupData(LuaUtils::DataDump &d, lua_State* src, int index, int depth) {
	++backupSize;
	const int type = lua_type(src, index);
	d.type = type;
	switch (type) {
		case LUA_TBOOLEAN: {
			d.bol = lua_toboolean(src, index);
			break;
		}
		case LUA_TNUMBER: {
			d.num = lua_tonumber(src, index);
			break;
		}
		case LUA_TSTRING: {
			size_t len = 0;
			const char* data = lua_tolstring(src, index, &len);
			if (len > 0) {
				d.str.resize(len);
				memcpy(&d.str[0], data, len);
			}
			break;
		}
		case LUA_TTABLE: {
			if(!BackupTable(d, src, index, depth))
				d.type = LUA_TNIL;
			break;
		}
		default: {
			d.type = LUA_TNIL;
			break;
		}
	}
	return true;
}

static bool RestoreData(const LuaUtils::DataDump &d, lua_State* dst, int depth) {
	const int type = d.type;
	switch (type) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, d.bol);
			break;
		}
		case LUA_TNUMBER: {
			lua_pushnumber(dst, d.num);
			break;
		}
		case LUA_TSTRING: {
			lua_pushlstring(dst, d.str.c_str(), d.str.size());
			break;
		}
		case LUA_TTABLE: {
			RestoreTable(d, dst, depth);
			break;
		}
		default: {
			lua_pushnil(dst);
			break;
		}
	}
	return true;
}

static bool BackupTable(LuaUtils::DataDump &d, lua_State* src, int index, int depth) {
	if (depth++ > maxDepth)
		return false;

	const int table = PosLuaIndex(src, index);
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		LuaUtils::DataDump dk, dv;
		BackupData(dk, src, -2, depth);
		BackupData(dv, src, -1, depth);
		d.table.push_back(std::pair<LuaUtils::DataDump, LuaUtils::DataDump>(dk ,dv));
	}

	return true;
}

static bool RestoreTable(const LuaUtils::DataDump &d, lua_State* dst, int depth) {
	if (depth++ > maxDepth) {
		lua_pushnil(dst);
		return false;
	}

	lua_newtable(dst);
	for (std::vector<std::pair<LuaUtils::DataDump, LuaUtils::DataDump> >::const_iterator i = d.table.begin(); i != d.table.end(); ++i) {
		RestoreData((*i).first, dst, depth);
		RestoreData((*i).second, dst, depth);
		lua_rawset(dst, -3);
	}

	return true;
}


int LuaUtils::Backup(std::vector<LuaUtils::DataDump> &backup, lua_State* src, int count) {
	const int srcTop = lua_gettop(src);
	if (srcTop < count)
		return 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		backup.push_back(DataDump());
		BackupData(backup.back(), src, i, 0);
	}

	return count;
}


int LuaUtils::Restore(const std::vector<LuaUtils::DataDump> &backup, lua_State* dst) {
	const int dstTop = lua_gettop(dst);
	int count = backup.size();
	lua_checkstack(dst, count); // FIXME: not enough for table chains

	for (std::vector<DataDump>::const_iterator i = backup.begin(); i != backup.end(); ++i) {
		RestoreData(*i, dst, 0);
	}
	lua_settop(dst, dstTop + count);

	return count;
}


int LuaUtils::ShallowBackup(std::vector<LuaUtils::ShallowDataDump> &backup, lua_State* src, int count) {
	const int srcTop = lua_gettop(src);
	if (srcTop < count)
		return 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;

	for(int i = startIndex; i <= endIndex; ++i) {
		const int type = lua_type(src, i);
		ShallowDataDump sdd;
		sdd.type = type;
		switch (type) {
			case LUA_TBOOLEAN: {
				sdd.data.bol = lua_toboolean(src, i);
				break;
			}
			case LUA_TNUMBER: {
				sdd.data.num = lua_tonumber(src, i);
				break;
			}
			case LUA_TSTRING: {
				size_t len = 0;
				const char* data = lua_tolstring(src, i, &len);
				sdd.data.str = new std::string;
				if (len > 0) {
					sdd.data.str->resize(len);
					memcpy(&(*sdd.data.str)[0], data, len);
				}
				break;
			}
			case LUA_TNIL: {
				break;
			}
			default: {
				LOG_L(L_WARNING, "ShallowBackup: Invalid type for argument %d", i);
				break; // nil
			}
		}
		backup.push_back(sdd);
	}

	return count;
}


int LuaUtils::ShallowRestore(const std::vector<LuaUtils::ShallowDataDump> &backup, lua_State* dst) {
	int count = backup.size();
	lua_checkstack(dst, count);

	for (int d = 0; d < count; ++d) {
		const ShallowDataDump &sdd = backup[d];
		switch (sdd.type) {
			case LUA_TBOOLEAN: {
				lua_pushboolean(dst, sdd.data.bol);
				break;
			}
			case LUA_TNUMBER: {
				lua_pushnumber(dst, sdd.data.num);
				break;
			}
			case LUA_TSTRING: {
				lua_pushlstring(dst, sdd.data.str->c_str(), sdd.data.str->size());
				delete sdd.data.str;
				break;
			}
			case LUA_TNIL: {
				lua_pushnil(dst);
				break;
			}
			default: {
				lua_pushnil(dst);
				LOG_L(L_WARNING, "ShallowRestore: Invalid type for argument %d", d + 1);
				break; // unhandled type
			}
		}
	}

	return count;
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


void LuaUtils::PushCurrentFuncEnv(lua_State* L, const char* caller)
{
	PushCurrentFunc(L, caller);
	PushFunctionEnv(L, caller, -1);
	lua_remove(L, -2); // remove the function
}

/******************************************************************************/
/******************************************************************************/

static int lowerKeysTable = 0;


static bool LowerKeysCheck(lua_State* L, int table)
{
	bool used = false;
	lua_pushvalue(L, table);
	lua_rawget(L, lowerKeysTable);
	if (lua_isnil(L, -1)) {
		used = false;
		lua_pushvalue(L, table);
		lua_pushboolean(L, true);
		lua_rawset(L, lowerKeysTable);
	}
	lua_pop(L, 1);
	return used;
}


static bool LowerKeysReal(lua_State* L, int depth)
{
	lua_checkstack(L, lowerKeysTable + 8 + (depth * 3));

	const int table = lua_gettop(L);
	if (LowerKeysCheck(L, table)) {
		return true;
	}

	// a new table for changed values
	const int changed = table + 1;
	lua_newtable(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			LowerKeysReal(L, depth + 1);
		}
		if (lua_israwstring(L, -2)) {
			const string rawKey = lua_tostring(L, -2);
			const string lowerKey = StringToLower(rawKey);
			if (rawKey != lowerKey) {
				// removed the mixed case entry
				lua_pushvalue(L, -2); // the key
				lua_pushnil(L);
				lua_rawset(L, table);
				// does the lower case key alread exist in the table?
				lua_pushsstring(L, lowerKey);
				lua_rawget(L, table);
				if (lua_isnil(L, -1)) {
					// lower case does not exist, add it to the changed table
					lua_pushsstring(L, lowerKey);
					lua_pushvalue(L, -3); // the value
					lua_rawset(L, changed);
				}
				lua_pop(L, 1);
			}
		}
	}

	// copy the changed values into the table
	for (lua_pushnil(L); lua_next(L, changed) != 0; lua_pop(L, 1)) {
		lua_pushvalue(L, -2); // copy the key to the top
		lua_pushvalue(L, -2); // copy the value to the top
		lua_rawset(L, table);
	}

	lua_pop(L, 1); // pop the changed table

	return true;
}


bool LuaUtils::LowerKeys(lua_State* L, int table)
{
	if (!lua_istable(L, table)) {
		return false;
	}

	// table of processed tables
	lowerKeysTable = lua_gettop(L) + 1;
	lua_checkstack(L, lowerKeysTable + 2);
	lua_newtable(L);

	lua_pushvalue(L, table); // push the table onto the top of the stack

	LowerKeysReal(L, 0);

	lua_pop(L, 2); // the lowered table, and the check table

	return true;
}


/******************************************************************************/
/******************************************************************************/

// copied from lua/src/lauxlib.cpp:luaL_checkudata()
void* LuaUtils::GetUserData(lua_State* L, int index, const string& type)
{
	const char* tname = type.c_str();
	void *p = lua_touserdata(L, index);
	if (p != NULL) {                               // value is a userdata?
		if (lua_getmetatable(L, index)) {            // does it have a metatable?
			lua_getfield(L, LUA_REGISTRYINDEX, tname); // get correct metatable
			if (lua_rawequal(L, -1, -2)) {             // the correct mt?
				lua_pop(L, 2);                           // remove both metatables
				return p;
			}
		}
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/

void LuaUtils::PrintStack(lua_State* L)
{
	const int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		LOG_L(L_ERROR, "  %i: type = %s (%p)", i, luaL_typename(L, i), lua_topointer(L, i));
		const int type = lua_type(L, i);
		if (type == LUA_TSTRING) {
			LOG_L(L_ERROR, "\t\t%s\n", lua_tostring(L, i));
		} else if (type == LUA_TNUMBER) {
			LOG_L(L_ERROR, "\t\t%f\n", lua_tonumber(L, i));
		} else if (type == LUA_TBOOLEAN) {
			LOG_L(L_ERROR, "\t\t%s\n", lua_toboolean(L, i) ? "true" : "false");
		} else {
			LOG_L(L_ERROR, "\n");
		}
	}
}


/******************************************************************************/
/******************************************************************************/

// from LuaShaders.cpp
int LuaUtils::ParseIntArray(lua_State* L, int index, int* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_toint(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}

/*
// from LuaShaders.cpp (index unused)
int LuaUtils::ParseFloatArray(lua_State* L, int index, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}
*/

// from LuaUnsyncedCtrl.cpp
int LuaUtils::ParseFloatArray(lua_State* L, int index, float* array, int size)
{
	if (!lua_istable(L, index)) {
		return -1;
	}
	const int table = (index > 0) ? index : (lua_gettop(L) + index + 1);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}

void LuaUtils::ParseCommandOptions(lua_State* L, const char* caller,
                                   int index, Command& cmd)
{
	if (lua_isnumber(L, index)) {
		cmd.options = (unsigned char)lua_tonumber(L, index);
	}
	else if (lua_istable(L, index)) {
		const int optionTable = index;
		for (lua_pushnil(L); lua_next(L, optionTable) != 0; lua_pop(L, 1)) {
			if (lua_israwnumber(L, -2)) { // avoid 'n'
				if (!lua_isstring(L, -1)) {
					luaL_error(L, "%s(): bad option table entry", caller);
				}
				const string value = lua_tostring(L, -1);
				if (value == "right") {
					cmd.options |= RIGHT_MOUSE_KEY;
				} else if (value == "alt") {
					cmd.options |= ALT_KEY;
				} else if (value == "ctrl") {
					cmd.options |= CONTROL_KEY;
				} else if (value == "shift") {
					cmd.options |= SHIFT_KEY;
				} else if (value == "meta") {
					cmd.options |= META_KEY;
				}
			}
		}
	}
	else {
		luaL_error(L, "%s(): bad options", caller);
	}
}


Command LuaUtils::ParseCommand(lua_State* L, const char* caller, int idIndex)
{
	// cmdID
	if (!lua_isnumber(L, idIndex)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	const int id = lua_toint(L, idIndex);
	Command cmd(id);

	// params
	const int paramTable = (idIndex + 1);
	if (!lua_istable(L, paramTable)) {
		luaL_error(L, "%s(): bad param table", caller);
	}
	for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): bad param table entry", caller);
			}
			const float value = lua_tofloat(L, -1);
			cmd.PushParam(value);
		}
	}

	// options
	ParseCommandOptions(L, caller, (idIndex + 2), cmd);

	// XXX should do some sanity checking?

	return cmd;
}


Command LuaUtils::ParseCommandTable(lua_State* L, const char* caller, int table)
{
	// cmdID
	lua_rawgeti(L, table, 1);
	if (!lua_isnumber(L, -1)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	const int id = lua_toint(L, -1);
	Command cmd(id);
	lua_pop(L, 1);

	// params
	lua_rawgeti(L, table, 2);
	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s(): bad param table", caller);
	}
	const int paramTable = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): bad param table entry", caller);
			}
			const float value = lua_tofloat(L, -1);
			cmd.PushParam(value);
		}
	}
	lua_pop(L, 1);

	// options
	lua_rawgeti(L, table, 3);
	ParseCommandOptions(L, caller, lua_gettop(L), cmd);
	lua_pop(L, 1);

	// XXX should do some sanity checking?

	return cmd;
}


void LuaUtils::ParseCommandArray(lua_State* L, const char* caller,
                                 int table, vector<Command>& commands)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing command array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1)) {
			continue;
		}
		Command cmd = ParseCommandTable(L, caller, lua_gettop(L));
		commands.push_back(cmd);
	}
}


int LuaUtils::ParseFacing(lua_State* L, const char* caller, int index)
{
	if (lua_israwnumber(L, index)) {
		return std::max(0, std::min(3, lua_toint(L, index)));
	}
	else if (lua_israwstring(L, index)) {
		const string dir = StringToLower(lua_tostring(L, index));
		if (dir == "s") { return 0; }
		if (dir == "e") { return 1; }
		if (dir == "n") { return 2; }
		if (dir == "w") { return 3; }
		if (dir == "south") { return 0; }
		if (dir == "east")  { return 1; }
		if (dir == "north") { return 2; }
		if (dir == "west")  { return 3; }
		luaL_error(L, "%s(): bad facing string", caller);
	}
	luaL_error(L, "%s(): bad facing parameter", caller);
	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUtils::Next(const ParamMap& paramMap, lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2); // create a 2nd argument if there isn't one

	// internal parameters first
	if (lua_isnil(L, 2)) {
		const string& nextKey = paramMap.begin()->first;
		lua_pushsstring(L, nextKey); // push the key
		lua_pushvalue(L, 3);         // copy the key
		lua_gettable(L, 1);          // get the value
		return 2;
	}

	// all internal parameters use strings as keys
	if (lua_isstring(L, 2)) {
		const char* key = lua_tostring(L, 2);
		ParamMap::const_iterator it = paramMap.find(key);
		if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
			// last key was an internal parameter
			++it;
			while ((it != paramMap.end()) && (it->second.type == READONLY_TYPE)) {
				++it; // skip read-only parameters
			}
			if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
				// next key is an internal parameter
				const string& nextKey = it->first;
				lua_pushsstring(L, nextKey); // push the key
				lua_pushvalue(L, 3);         // copy the key
				lua_gettable(L, 1);          // get the value (proxied)
				return 2;
			}
			// start the user parameters,
			// remove the internal key and push a nil
			lua_settop(L, 1);
			lua_pushnil(L);
		}
	}

	// user parameter
	if (lua_next(L, 1)) {
		return 2;
	}

	// end of the line
	lua_pushnil(L);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static std::string getprintf_msg(lua_State* L, int index)
{
	// copied from lua/src/lib/lbaselib.c
	string msg = "";
	const int args = lua_gettop(L); // number of arguments

	lua_getglobal(L, "tostring");

	for (int i = index; i <= args; i++) {
		const char* s;
		lua_pushvalue(L, -1);     // function to be called
		lua_pushvalue(L, i);      // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if (i > index) {
			msg += ", ";
		}
		msg += s;
		lua_pop(L, 1);            // pop result
	}

	if ((args != index) || !lua_istable(L, index)) {
		return msg;
	}

	// print solo tables (array style)
	msg = "TABLE: ";
	bool first = true;
	for (lua_pushnil(L); lua_next(L, index) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {  // only numeric keys
			const char *s;
			lua_pushvalue(L, -3);    // function to be called
			lua_pushvalue(L, -2);    // value to print
			lua_call(L, 1, 1);
			s = lua_tostring(L, -1);  // get result
			if (!first) {
				msg += ", ";
			}
			msg += s;
			first = false;
			lua_pop(L, 1);            // pop result
		}
	}

	return msg;
}


int LuaUtils::Echo(lua_State* L)
{
	const std::string msg = getprintf_msg(L, 1);
	LOG("%s", msg.c_str());
	return 0;
}


bool LuaUtils::PushLogEntries(lua_State* L)
{
#define PUSH_LOG_LEVEL(cmd) LuaPushNamedNumber(L, #cmd, LOG_LEVEL_ ## cmd)
	PUSH_LOG_LEVEL(DEBUG);
	PUSH_LOG_LEVEL(INFO);
	PUSH_LOG_LEVEL(WARNING);
	PUSH_LOG_LEVEL(ERROR);
	PUSH_LOG_LEVEL(FATAL);
	return true;
}


/*-
	Logs a msg to the logfile / console
	@param loglevel loglevel that will be used for the message
	@param msg string to be logged
	@fn Spring.Log(string logsection, int loglevel, ...)
	@fn Spring.Log(string logsection, string loglevel, ...)
*/
int LuaUtils::Log(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args < 2)
		return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");
	if (args < 3)
		return 0;

	const std::string section = luaL_checkstring(L, 1);

	int loglevel;
	if (lua_israwnumber(L, 2)) {
		loglevel = lua_tonumber(L, 2);
	}
	else if (lua_israwstring(L, 2)) {
		std::string loglvlstr = lua_tostring(L, 2);
		StringToLowerInPlace(loglvlstr);
		if (loglvlstr == "debug") {
			loglevel = LOG_LEVEL_DEBUG;
		}
		else if (loglvlstr == "info") {
			loglevel = LOG_LEVEL_INFO;
		}
		else if (loglvlstr == "warning") {
			loglevel = LOG_LEVEL_WARNING;
		}
		else if (loglvlstr == "error") {
			loglevel = LOG_LEVEL_ERROR;
		}
		else if (loglvlstr == "fatal") {
			loglevel = LOG_LEVEL_FATAL;
		}
		else {
			return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");
		}
	}
	else {
		return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");
	}

	const std::string msg = getprintf_msg(L, 3);
	LOG_SI(section.c_str(), loglevel, "%s", msg.c_str());
	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUtils::ZlibCompress(lua_State* L)
{
	size_t inLen;
	const char* inData = luaL_checklstring(L, 1, &inLen);

	long unsigned bufsize = compressBound(inLen);
	std::vector<boost::uint8_t> compressed(bufsize, 0);
	const int error = compress(&compressed[0], &bufsize, (const boost::uint8_t*)inData, inLen);
	if (error == Z_OK)
	{
		lua_pushlstring(L, (const char*)&compressed[0], bufsize);
		return 1;
	}
	else
	{
		return luaL_error(L, "Error while compressing");
	}
}

int LuaUtils::ZlibDecompress(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args < 1)
		return luaL_error(L, "ZlibCompress: missign data argument");
	size_t inLen;
	const char* inData = luaL_checklstring(L, 1, &inLen);

	long unsigned bufsize = 65000;
	if (args > 1 && lua_isnumber(L, 2))
		bufsize = std::max(lua_toint(L, 2), 0);

	std::vector<boost::uint8_t> uncompressed(bufsize, 0);
	const int error = uncompress(&uncompressed[0], &bufsize, (const boost::uint8_t*)inData, inLen);
	if (error == Z_OK)
	{
		lua_pushlstring(L, (const char*)&uncompressed[0], bufsize);
		return 1;
	}
	else
	{
		return luaL_error(L, "Error while decompressing");
	}
}

/******************************************************************************/
/******************************************************************************/

int LuaUtils::tobool(lua_State* L)
{
	return 1;
}


int LuaUtils::isnil(lua_State* L)
{
	lua_pushboolean(L, lua_isnoneornil(L, 1));
	return 1;
}


int LuaUtils::isbool(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TBOOLEAN);
	return 1;
}


int LuaUtils::isnumber(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TNUMBER);
	return 1;
}


int LuaUtils::isstring(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TSTRING);
	return 1;
}


int LuaUtils::istable(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TTABLE);
	return 1;
}


int LuaUtils::isthread(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TTHREAD);
	return 1;
}


int LuaUtils::isfunction(lua_State* L)
{
	lua_pushboolean(L, lua_type(L, 1) == LUA_TFUNCTION);
	return 1;
}


int LuaUtils::isuserdata(lua_State* L)
{
	const int type = lua_type(L, 1);
	lua_pushboolean(L, (type == LUA_TUSERDATA) ||
	                   (type == LUA_TLIGHTUSERDATA));
	return 1;
}


/******************************************************************************/
/******************************************************************************/

#define DEBUG_TABLE "debug"
#define DEBUG_FUNC "traceback"

int LuaUtils::PushDebugTraceback(lua_State *L)
{
	lua_getglobal(L, DEBUG_TABLE);
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, DEBUG_FUNC);
		if (!lua_isfunction(L, -1)) {
			return 0;
		}
		lua_remove(L, -2);
	} else {
		lua_pop(L, 1);
		static const LuaHashString traceback("traceback");
		if (!traceback.GetRegistryFunc(L)) {
			return 0;
		}
	}

	return lua_gettop(L);
}

/******************************************************************************/
/******************************************************************************/

void LuaUtils::PushStringVector(lua_State* L, const vector<string>& vec)
{
	lua_createtable(L, vec.size(), 0);
	for (size_t i = 0; i < vec.size(); i++) {
		lua_pushnumber(L, (int) (i + 1));
		lua_pushsstring(L, vec[i]);
		lua_rawset(L, -3);
	}
}

/******************************************************************************/
/******************************************************************************/

void LuaUtils::PushCommandDesc(lua_State* L, const CommandDescription& cd)
{
	const int numParams = cd.params.size();
	const int numTblKeys = 12;

	lua_checkstack(L, 1 + 1 + 1 + 1);
	lua_createtable(L, 0, numTblKeys);

	HSTR_PUSH_NUMBER(L, "id",          cd.id);
	HSTR_PUSH_NUMBER(L, "type",        cd.type);
	HSTR_PUSH_STRING(L, "name",        cd.name);
	HSTR_PUSH_STRING(L, "action",      cd.action);
	HSTR_PUSH_STRING(L, "tooltip",     cd.tooltip);
	HSTR_PUSH_STRING(L, "texture",     cd.iconname);
	HSTR_PUSH_STRING(L, "cursor",      cd.mouseicon);
	HSTR_PUSH_BOOL(L,   "hidden",      cd.hidden);
	HSTR_PUSH_BOOL(L,   "disabled",    cd.disabled);
	HSTR_PUSH_BOOL(L,   "showUnique",  cd.showUnique);
	HSTR_PUSH_BOOL(L,   "onlyTexture", cd.onlyTexture);

	HSTR_PUSH(L, "params");

	lua_createtable(L, 0, numParams);

	for (int p = 0; p < numParams; p++) {
		lua_pushsstring(L, cd.params[p]);
		lua_rawseti(L, -2, p + 1);
	}

	// CmdDesc["params"] = {[1] = "string1", [2] = "string2", ...}
	lua_settable(L, -3);
}

