/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread.hpp>

#include "LuaInclude.h"
#include "Lua/LuaHandle.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"
#if (!defined(HEADLESS) && !defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI))
	#include "System/Misc/SpringTime.h"
#endif


///////////////////////////////////////////////////////////////////////////
// Custom Lua Mutexes

static std::map<lua_State*, boost::recursive_mutex*> mutexes;
static std::map<lua_State*, bool> coroutines;

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

	if (coroutines.find(L) != coroutines.end()) {
		mutexes.erase(L);
		coroutines.erase(L);
	} else {
		lua_unlock(L);
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

	coroutines[L_child] = true;
	mutexes[L_child] = plcd->luamutex;
}


void LuaMutexLock(lua_State* L)
{
	if (!GetLuaContextData(L)) return; // CLuaParser

	boost::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;

	if (mutex->try_lock())
		return;

	//static int failedLocks = 0;
	//LOG("LuaMutexLock %i", ++failedLocks);
	mutex->lock();
}


void LuaMutexUnlock(lua_State* L)
{
	if (!GetLuaContextData(L)) return; // CLuaParser

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
//

const char* spring_lua_getName(lua_State* L)
{
	auto ld = GetLuaContextData(L);
	if (ld) {
		return ld->owner->GetName().c_str();
	}

	static const char* c = "";
	return c;
}


///////////////////////////////////////////////////////////////////////////
// Custom Memory Allocator
//
// these track allocations across all states
static Threading::AtomicCounterInt64 totalBytesAlloced = 0;
static Threading::AtomicCounterInt64 totalNumLuaAllocs = 0;
static Threading::AtomicCounterInt64 totalLuaAllocTime = 0;

// 64-bit systems are less constrained
static const unsigned int maxAllocedBytes = 768u * 1024u*1024u * (1u + (sizeof(void*) == 8));
static const char* maxAllocFmtStr = "%s: cannot allocate more memory! (%u bytes already used, %u bytes maximum)";

size_t spring_lua_states() { return (mutexes.size() - coroutines.size()); }

void spring_lua_update_context(luaContextData* lcd, size_t osize, size_t nsize) {
	if (lcd == NULL)
		return;

	lcd->maxAllocedBytes = maxAllocedBytes / std::max(size_t(1), spring_lua_states());
	lcd->curAllocedBytes += (nsize - osize);
}

void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	auto lcd = (luaContextData*) ud;

	if (nsize == 0) {
		totalBytesAlloced -= osize;

		spring_lua_update_context(lcd, osize, 0);
		free(ptr);
		return NULL;
	}

	if (lcd != NULL) {
		if ((nsize > osize) && (lcd->curAllocedBytes > lcd->maxAllocedBytes)) {
			LOG_L(L_FATAL, maxAllocFmtStr, (lcd->owner->GetName()).c_str(), lcd->curAllocedBytes, lcd->maxAllocedBytes);

			// better kill Lua than whole engine
			// NOTE: this will trigger luaD_throw --> exit(EXIT_FAILURE)
			return NULL;
		}

		// allow larger allocation limit per state if fewer states
		// but do not allow one single state to soak up all memory
		// TODO: non-uniform distribution of limits per state
		spring_lua_update_context(lcd, osize, nsize);
	}

	#if (!defined(HEADLESS) && !defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI))
	// FIXME:
	//   the Lua lib is compiled with its own makefile and does not inherit these definitions, yet
	//   somehow unitsync compiles (despite not linking against GlobalSynced) but dedserv does not
	//   if gs is referenced
	const spring_time t0 = spring_gettime();
	void* mem = realloc(ptr, nsize);
	const spring_time t1 = spring_gettime();

	totalBytesAlloced += (nsize - osize);
	totalNumLuaAllocs += 1;
	totalLuaAllocTime += (t1 - t0).toMicroSecsi();

	return mem;
	#else
	return (realloc(ptr, nsize));
	#endif
}

void spring_lua_alloc_get_stats(SLuaInfo* info)
{
	info->allocedBytes = totalBytesAlloced;
	info->numLuaAllocs = totalNumLuaAllocs;
	info->luaAllocTime = totalLuaAllocTime;
	info->numLuaStates = mutexes.size() - coroutines.size();
}

void spring_lua_alloc_update_stats(bool clear)
{
	if (clear) {
		totalNumLuaAllocs = 0;
		totalLuaAllocTime = 0;
	}
}

