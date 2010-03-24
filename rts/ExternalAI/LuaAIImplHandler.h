/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LUA_AI_IMPL_HANDLER_H_
#define _LUA_AI_IMPL_HANDLER_H_

//#include "ExternalAI/SkirmishAIKey.h"
#include "System/Info.h"

//#include "creg/creg_cond.h"

#include <vector>
#include <string>

struct Info;

/**
 * Handles all Lua AI implementation relevant stuff.
 */
class CLuaAIImplHandler
{
private:
//	CR_DECLARE(CLuaAIImplHandler);

	CLuaAIImplHandler();
	~CLuaAIImplHandler();

public:
	/**
	 * Fetcher for the singleton.
	 */
	static CLuaAIImplHandler& GetInstance();

	std::vector< std::vector<InfoItem> > LoadInfos();

private:
	static CLuaAIImplHandler* mySingleton;
};

#define luaAIImplHandler CLuaAIImplHandler::GetInstance()

#endif // _LUA_AI_IMPL_HANDLER_H_
