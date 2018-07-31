/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaUICommand.h"

#include "LuaInclude.h"

#include "LuaUtils.h"

#include "Game/UnsyncedActionExecutor.h"
#include "Game/SyncedActionExecutor.h"
#include "Game/UnsyncedGameCommands.h"
#include "Game/SyncedGameCommands.h"

/******************************************************************************/
/******************************************************************************/


bool LuaUICommand::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetUICommands);
// 	REGISTER_LUA_CFUNC(RegisterUICommand);
// 	REGISTER_LUA_CFUNC(DeregisterUICommand);
	
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaUICommand::GetUICommands(lua_State* L)
{
	const auto& syncedExecutors = syncedGameCommands->GetSortedActionExecutors();
	const auto& unsyncedExecutors = unsyncedGameCommands->GetSortedActionExecutors();
	
	int count = 0;
	lua_createtable(L, syncedExecutors.size() + unsyncedExecutors.size(), 0);
	for (const auto& pair: syncedExecutors) {
		const ISyncedActionExecutor* exec = pair.second;

		lua_createtable(L, 0, 4);
		HSTR_PUSH_STRING(L, "command",     exec->GetCommand());
		HSTR_PUSH_STRING(L, "description", exec->GetDescription());
		HSTR_PUSH_BOOL(L,   "synced",      exec->IsSynced());
		HSTR_PUSH_BOOL(L,   "cheat",       exec->IsCheatRequired());
		lua_rawseti(L, -2, count++);
	}
	for (const auto& pair: unsyncedExecutors) {
		const IUnsyncedActionExecutor* exec = pair.second;

		lua_createtable(L, 0, 4);
		HSTR_PUSH_STRING(L, "command",     exec->GetCommand());
		HSTR_PUSH_STRING(L, "description", exec->GetDescription());
		HSTR_PUSH_BOOL(L,   "synced",      exec->IsSynced());
		HSTR_PUSH_BOOL(L,   "cheat",       exec->IsCheatRequired());
		lua_rawseti(L, -2, count++);
	}
	return 1;
}

// // TODO: FUTURE
// int LuaUICommand::RegisterUICommand(lua_State* L) 
// {
// 	return 0;
// }
// 
// int LuaUICommand::DeregisterUICommand(lua_State* L) 
// {
// 	return 0;
// }
