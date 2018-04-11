/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_LUA_TYPES_H
#define CR_LUA_TYPES_H

#include "creg_cond.h"
#include "LuaInclude.h"

namespace creg {
	void SerializeLuaState(creg::ISerializer* s, lua_State** L);
	void RegisterCFunction(const char* name, lua_CFunction f);
	void SetLuaContext(void* context, lua_Alloc frealloc, lua_CFunction panic);
}

#endif //CR_BASIC_TYPES_H
