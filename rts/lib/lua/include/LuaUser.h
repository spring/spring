/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_USER_H
#define SPRING_LUA_USER_H

extern void LuaCreateMutex(lua_State * L);
extern void LuaDestroyMutex(lua_State * L);
extern void LuaLinkMutex(lua_State* L_parent, lua_State* L_child);
extern void LuaMutexLock(lua_State * L);
extern void LuaMutexUnlock(lua_State * L);
extern void LuaMutexYield(lua_State * L);

extern void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize);
extern void spring_lua_alloc_get_stats(int* allocedBytes);

#endif // SPRING_LUA_USER_H
