/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <boost/thread/recursive_mutex.hpp>

#include "lua.h"
#include "LuaInclude.h"
#include "Lua/LuaHandle.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"

///////////////////////////////////////////////////////////////////////////
// Custom Lua Mutexes

static std::map<lua_State*, boost::recursive_mutex*> mutexes;
static std::map<lua_State*, bool> isCoroutine;

static boost::recursive_mutex* GetLuaMutex(lua_State* L)
{
	assert(!mutexes[L]);
	return new boost::recursive_mutex();
}



void LuaCreateMutex(lua_State* L)
{
	luaContextData* lcd = GetLuaContextData(L);
	if (!lcd) return; // CLuaParser
	assert(lcd);

	boost::recursive_mutex* mutex = GetLuaMutex(L);
	lcd->luamutex = mutex;
	mutexes[L] = mutex;
}


void LuaDestroyMutex(lua_State* L)
{
	if (!GetLuaContextData(L)) return; // CLuaParser
	assert(GetLuaContextData(L));

	if (isCoroutine.find(L) != isCoroutine.end()) {
		mutexes.erase(L);
		isCoroutine.erase(L);
	} else {
		lua_unlock(L);
		assert(isCoroutine.find(L) == isCoroutine.end());
		assert(mutexes.find(L) != mutexes.end());
		boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
		assert(mutex);
		delete mutex;
		mutexes.erase(L);
		//TODO erase all related coroutines too?
	}
}


void LuaLinkMutex(lua_State* L_parent, lua_State* L_child)
{
	luaContextData* plcd = GetLuaContextData(L_parent);
	assert(plcd);

	luaContextData* clcd = GetLuaContextData(L_child);
	assert(clcd);

	assert(plcd == clcd);

	isCoroutine[L_child] = true;
	mutexes[L_child] = plcd->luamutex;
}


void LuaMutexLock(lua_State* L)
{
	if (!GetLuaContextData(L)) return; // CLuaParser
	assert(GetLuaContextData(L));
	boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;

	if (mutex->try_lock()) return;
	//static int failedLocks = 0;
	//LOG("LuaMutexLock %i", ++failedLocks);
	mutex->lock();
}


void LuaMutexUnlock(lua_State* L)
{
	if (!GetLuaContextData(L)) return; // CLuaParser
	assert(GetLuaContextData(L));
	boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
	mutex->unlock();
}


void LuaMutexYield(lua_State* L)
{
	assert(GetLuaContextData(L));
	/*mutexes[L]->unlock();
	if (!mutexes[L]->try_lock()) {
		// only yield if another thread is waiting for the mutex
		boost::this_thread::yield();
		mutexes[L]->lock();
	}*/

	static int count = 0;
	bool y = false;
	if (count-- <= 0) { y = true; count = 30; }
	LuaMutexUnlock(L);
	if (y) boost::this_thread::yield();
	LuaMutexLock(L);
}


///////////////////////////////////////////////////////////////////////////
// Custom Memory Allocator

static Threading::AtomicCounterInt64 allocedCur = 0;

void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	//(luaContextData*)ud;

	if (nsize == 0) {
		allocedCur -= osize;
		free(ptr);
		return NULL;
	}

	allocedCur += nsize - osize;
	assert(allocedCur < 512*1024*1024);
	return realloc(ptr, nsize);
}

void spring_lua_alloc_get_stats(SLuaInfo* info)
{
	info->allocedBytes = allocedCur;
	info->numStates = mutexes.size() - isCoroutine.size();
}
