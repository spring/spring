
#include <map>
#include "lua.h"
#include <boost/thread/recursive_mutex.hpp>
#include "System/Platform/Threading.h"

/*boost::recursive_mutex* getLuaMutex(bool userMode, bool primary) {
#if (LUA_MT_OPT & LUA_MUTEX)
	if(userMode)
		return new boost::recursive_mutex();
	else // LuaGaia & LuaRules will share mutexes to avoid deadlocks during XCalls etc.
		return primary ? &luaprimmutex : &luasecmutex;
#else
		return &luaprimmutex;
#endif
}*/

std::map<lua_State*, boost::recursive_mutex*> mutexes;
std::map<lua_State*, bool> isCoroutine;

void LuaCreateMutex(lua_State* L)
{
	//L_New->luamutex = getLuaMutex(bool userMode, bool primary);
	mutexes[L] = new boost::recursive_mutex();
}
void LuaMutexLock(lua_State* L)
{
	mutexes[L]->lock(); //FIXME count (un)locks?
}
void LuaMutexUnlock(lua_State* L)
{
	mutexes[L]->unlock(); //FIXME count (un)locks?
}
void LuaLinkMutex(lua_State* L_parent, lua_State* L_child)
{
	isCoroutine[L_child] = true;
	mutexes[L_child] = mutexes[L_parent];
}
void LuaDestroyMutex(lua_State* L)
{
	if (!L)
		return;

	if (isCoroutine.find(L) != isCoroutine.end()) {
		mutexes.erase(L);
		isCoroutine.erase(L);
	} else {
		assert(isCoroutine.find(L) == isCoroutine.end());
		assert(mutexes.find(L) != mutexes.end());
		//delete mutexes[L];
		mutexes.erase(L);
		isCoroutine.erase(L);
		//TODO erase all children too?
	}
}

static Threading::AtomicCounterInt64 allocedCur = 0;

void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void)ud;

	if (nsize == 0) {
		allocedCur -= osize;
		free(ptr);
		return NULL;
	}

	allocedCur -= osize;
	allocedCur += nsize;
	return realloc(ptr, nsize);
}

void spring_lua_alloc_get_stats(int* allocedBytes)
{
	*allocedBytes = allocedCur;
}
