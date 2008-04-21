#include "StdAfx.h"
// LuaUtils.cpp: implementation of the CLuaUtils class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUtils.h"
#include <set>
#include <cctype>

#include "LogOutput.h"


static       int depth = 0;
static const int maxDepth = 256;


/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index);


static inline int PosLuaIndex(lua_State* src, int index)
{
	if (index > 0) {
		return index;
	} else {
		return (lua_gettop(src) + index + 1);
	}
}


static bool CopyPushData(lua_State* dst, lua_State* src, int index)
{
	if (lua_isboolean(src, index)) {
		lua_pushboolean(dst, lua_toboolean(src, index));
	}
	else if (lua_israwnumber(src, index)) {
		lua_pushnumber(dst, lua_tonumber(src, index));
	}
	else if (lua_israwstring(src, index)) {
		lua_pushstring(dst, lua_tostring(src, index));
	}
	else if (lua_istable(src, index)) {
		CopyPushTable(dst, src, index);
	}
	else {
		lua_pushnil(dst); // unhandled type
		return false;
	}
	return true;
}


static bool CopyPushTable(lua_State* dst, lua_State* src, int index)
{
	if (depth > maxDepth) {
		lua_pushnil(dst); // push something
		return false;
	}

	depth++;
	lua_newtable(dst);
	const int table = PosLuaIndex(src, index);
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		CopyPushData(dst, src, -2); // copy the key
		CopyPushData(dst, src, -1); // copy the value
		lua_rawset(dst, -3);
	}
	depth--;

	return true;
}


int LuaUtils::CopyData(lua_State* dst, lua_State* src, int count)
{
	const int srcTop = lua_gettop(src);
	const int dstTop = lua_gettop(dst);
	if (srcTop < count) {
		return 0;
	}
	lua_checkstack(dst, dstTop + count); // FIXME: not enough for table chains

	depth = 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		CopyPushData(dst, src, i);
	}
	lua_settop(dst, dstTop + count);

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
		printf("  %i: type = %s (%p)", i, luaL_typename(L, i), lua_topointer(L, i));
		const int type = lua_type(L, i);
		if (type == LUA_TSTRING) {
			printf("\t\t%s\n", lua_tostring(L, i));
		} else if (type == LUA_TNUMBER) {
			printf("\t\t%f\n", lua_tonumber(L, i));
		} else if (type == LUA_TBOOLEAN) {
			printf("\t\t%s\n", lua_toboolean(L, i) ? "true" : "false");
		} else {
			printf("\n");
		}
	}
}


/******************************************************************************/
/******************************************************************************/


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
				}
			}
		}
	}
	else {
		luaL_error(L, "%s(): bad options", caller);
	}
}


void LuaUtils::ParseCommand(lua_State* L, const char* caller,
                            int idIndex, Command& cmd)
{
	// cmdID
	if (!lua_isnumber(L, idIndex)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	cmd.id = (int)lua_tonumber(L, idIndex);

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
			const float value = (float)lua_tonumber(L, -1);
			cmd.params.push_back(value);
		}
	}

	// options
	ParseCommandOptions(L, caller, (idIndex + 2), cmd);

	// NOTE: should do some sanity checking?
}


void LuaUtils::ParseCommandTable(lua_State* L, const char* caller,
                                 int table, Command& cmd)
{
	// cmdID
	lua_rawgeti(L, table, 1);
	if (!lua_isnumber(L, -1)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	cmd.id = (int)lua_tonumber(L, -1);
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
			const float value = (float)lua_tonumber(L, -1);
			cmd.params.push_back(value);
		}
	}
	lua_pop(L, 1);

	// options
	lua_rawgeti(L, table, 3);
	ParseCommandOptions(L, caller, lua_gettop(L), cmd);
	lua_pop(L, 1);

	// NOTE: should do some sanity checking?
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
		Command cmd;
		ParseCommandTable(L, caller, lua_gettop(L), cmd);
		commands.push_back(cmd);
	}
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
		lua_pushstring(L, nextKey.c_str()); // push the key
		lua_pushvalue(L, 3);                // copy the key
		lua_gettable(L, 1);                 // get the value
		return 2;
	}

	// all internal parameters use strings as keys
	if (lua_isstring(L, 2)) {
		const char* key = lua_tostring(L, 2);
		ParamMap::const_iterator it = paramMap.find(key);
		if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
			// last key was an internal parameter
			it++;
			while ((it != paramMap.end()) && (it->second.type == READONLY_TYPE)) {
				it++; // skip read-only parameters
			}
			if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
				// next key is an internal parameter
				const string& nextKey = it->first;
				lua_pushstring(L, nextKey.c_str()); // push the key
				lua_pushvalue(L, 3);                // copy the key
				lua_gettable(L, 1);                 // get the value (proxied)
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


int LuaUtils::Echo(lua_State* L)
{
	// copied from lua/src/lib/lbaselib.c
	string msg = "";
	const int args = lua_gettop(L); // number of arguments

	lua_getglobal(L, "tostring");

	for (int i = 1; i <= args; i++) {
		const char *s;
		lua_pushvalue(L, -1);     // function to be called
		lua_pushvalue(L, i);      // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if (s == NULL) {
			return luaL_error(L, "`tostring' must return a string to `print'");
		}
		if (i > 1) {
			msg += ", ";
		}
		msg += s;
		lua_pop(L, 1);            // pop result
	}
	logOutput.Print(msg);

	if ((args != 1) || !lua_istable(L, 1)) {
		return 0;
	}

	// print solo tables (array style)
	msg = "TABLE: ";
	bool first = true;
	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {  // only numeric keys
			const char *s;
			lua_pushvalue(L, -3);     // function to be called
			lua_pushvalue(L, -2	);    // value to print
			lua_call(L, 1, 1);
			s = lua_tostring(L, -1);  // get result
			if (s == NULL) {
				return luaL_error(L, "`tostring' must return a string to `print'");
			}
			if (!first) {
				msg += ", ";
			}
			msg += s;
			first = false;
			lua_pop(L, 1);            // pop result
		}
	}
	logOutput.Print(msg);

	return 0;
}


/******************************************************************************/
/******************************************************************************/
