/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author	Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef __LUA_AI_IMPL_HANDLER_H__
#define __LUA_AI_IMPL_HANDLER_H__

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

#endif // __LUA_AI_IMPL_HANDLER_H__
