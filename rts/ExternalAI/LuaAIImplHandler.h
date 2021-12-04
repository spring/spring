/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_AI_IMPL_HANDLER_H
#define LUA_AI_IMPL_HANDLER_H

#include "System/Info.h"

//#include "System/creg/creg_cond.h"

#include <array>
#include <vector>
#include <string>

/**
 * Handles all Lua AI implementation relevant stuff.
 */
class CLuaAIImplHandler
{
//	CR_DECLARE(CLuaAIImplHandler);

public:
	typedef std::vector< std::array<InfoItem, 4> > InfoItemVector;

	/**
	 * Fetcher for the singleton.
	 */
	static CLuaAIImplHandler& GetInstance();

	InfoItemVector LoadInfoItems();
};

#define luaAIImplHandler CLuaAIImplHandler::GetInstance()

#endif // LUA_AI_IMPL_HANDLER_H
