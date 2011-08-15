/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_AI_IMPL_HANDLER_H
#define LUA_AI_IMPL_HANDLER_H

#include "System/Info.h"

//#include "System/creg/creg_cond.h"

#include <vector>
#include <string>

struct Info;

/**
 * Handles all Lua AI implementation relevant stuff.
 */
class CLuaAIImplHandler
{
//	CR_DECLARE(CLuaAIImplHandler);

	CLuaAIImplHandler();
	~CLuaAIImplHandler();

public:
	/**
	 * Fetcher for the singleton.
	 */
	static CLuaAIImplHandler& GetInstance();

	std::vector< std::vector<InfoItem> > LoadInfos();
};

#define luaAIImplHandler CLuaAIImplHandler::GetInstance()

#endif // LUA_AI_IMPL_HANDLER_H
