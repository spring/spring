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

static boost::recursive_mutex luaprimmutex, luasecmutex;
static std::map<lua_State*, boost::recursive_mutex*> mutexes;
static std::map<lua_State*, bool> coroutines;
//static luaContextData baseLuaContextData;

static boost::recursive_mutex* GetLuaMutex(bool userMode, bool primary)
{
#if (LUA_MT_OPT & LUA_MUTEX)
	if (userMode)
		return new boost::recursive_mutex();
	else // LuaGaia & LuaRules will share mutexes to avoid deadlocks during XCalls etc.
		return primary ? &luaprimmutex : &luasecmutex;
#else
	return &luaprimmutex; //FIXME all luaStates share the same mutex???
#endif
}



void LuaCreateMutex(lua_State* L)
{
	if (!GetLuaContextData(L)) return;

	luaContextData* lcd = GetLuaContextData(L);

	//FIXME when using Lua's lua_lock system it might be a bit inefficient to do a null check each call,
	//      so better link to a dummy one instead.
	//      Problem is that luaContextData links a lot rendering related files (LuaTextures, LuaFBOs, ...)
	//      which aren't linked in unitsync.
	/*if (!lcd) {
		G(L)->ud = &baseLuaContextData;
		lcd = &baseLuaContextData;
	}*/

	boost::recursive_mutex* mutex = GetLuaMutex((lcd->owner) ? true : lcd->owner->GetUserMode(), lcd->primary);
	lcd->luamutex = mutex;
	mutexes[L] = mutex;
}


void LuaDestroyMutex(lua_State* L)
{
	if (!GetLuaContextData(L)) return;

	if (!L)
		return;

	if (coroutines.find(L) != coroutines.end()) {
		mutexes.erase(L);
		coroutines.erase(L);
	} else {
		assert(coroutines.find(L) == coroutines.end());
		assert(mutexes.find(L) != mutexes.end());
		boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
		if ((mutex != &luaprimmutex) && (mutex != &luasecmutex))
			delete mutex;
		mutexes.erase(L);
		//TODO erase all related coroutines too?
	}
}


void LuaLinkMutex(lua_State* L_parent, lua_State* L_child)
{
	if (!GetLuaContextData(L_parent)) return;

	coroutines[L_child] = true;
	mutexes[L_child] = mutexes[L_parent];
}


void LuaMutexLock(lua_State* L)
{
	if (!GetLuaContextData(L)) return;
	boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;

	if (mutex->try_lock()) return;
	//static int failedLocks = 0;
	//LOG("LuaMutexLock %i", ++failedLocks);
	mutex->lock();
}


void LuaMutexUnlock(lua_State* L)
{
	if (!GetLuaContextData(L)) return;
	boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
	mutex->unlock();
}


void LuaMutexYield(lua_State* L)
{
	if (!GetLuaContextData(L)) return;
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

static Threading::AtomicCounterInt64 bytesAlloced = 0;
static Threading::AtomicCounterInt64 numLuaAllocs = 0;
static Threading::AtomicCounterInt64 luaAllocTime = 0;

void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	if (nsize == 0) {
		bytesAlloced -= osize;
		free(ptr);
		return NULL;
	}

	#if (!defined(HEADLESS) && !defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI))
	// FIXME:
	//   the Lua lib is compiled with its own makefile and does not inherit these definitions, yet
	//   somehow unitsync compiles (despite not linking against GlobalSynced) but dedserv does not
	//   if gs is referenced
	const spring_time t0 = spring_gettime();
	void* mem = realloc(ptr, nsize);
	const spring_time t1 = spring_gettime();

	bytesAlloced += (nsize - osize);
	numLuaAllocs += 1;
	luaAllocTime += (t1 - t0).toMicroSecsi();

	return mem;
	#else
	return (realloc(ptr, nsize));
	#endif
}

void spring_lua_alloc_get_stats(SLuaInfo* info)
{
	info->allocedBytes = bytesAlloced;
	info->numLuaAllocs = numLuaAllocs;
	info->luaAllocTime = luaAllocTime;
	info->numLuaStates = mutexes.size() - coroutines.size();
}

void spring_lua_alloc_update_stats(bool clear)
{
	if (clear) {
		numLuaAllocs = 0;
		luaAllocTime = 0;
	}
}

