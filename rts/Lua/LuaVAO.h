/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <memory>
#include <vector>

struct lua_State;
class LuaVAOImpl;

class LuaVAOs {
public:
	virtual ~LuaVAOs();
	static int GetVAO(lua_State* L);
	static bool PushEntries(lua_State* L);
	std::vector<std::weak_ptr<LuaVAOImpl>> luaVAOs;
};