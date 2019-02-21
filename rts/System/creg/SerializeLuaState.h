/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_LUA_TYPES_H
#define CR_LUA_TYPES_H

#include "creg_cond.h"
#include "LuaInclude.h"

namespace creg {
	void SerializeLuaState(creg::ISerializer* s, lua_State** L);
	void SerializeLuaThread(creg::ISerializer* s, lua_State** L);
	void RegisterCFunction(const char* name, lua_CFunction f);
	void AutoRegisterCFunctions(const std::string& handle, lua_State* L);
	void CopyLuaContext(lua_State* L);
}

#endif //CR_BASIC_TYPES_H
