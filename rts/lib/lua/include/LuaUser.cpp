/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <boost/thread.hpp>

#include "LuaInclude.h"
#include "Game/GameVersion.h"
#include "Lua/LuaHandle.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringMutex.h"
#include "System/Log/ILog.h"
#if (!defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI))
	#include "System/Misc/SpringTime.h"
#endif


///////////////////////////////////////////////////////////////////////////
// Custom Lua Mutexes

static std::map<lua_State*, spring::recursive_mutex*> mutexes;
static std::map<lua_State*, bool> coroutines;

static spring::recursive_mutex* GetLuaMutex(lua_State* L)
{
	assert(!mutexes[L]);
	return new spring::recursive_mutex();
}



void LuaCreateMutex(lua_State* L)
{
	#if (ENABLE_USERSTATE_LOCKS == 0)
	// if LoadingMT=1, everything runs in the game-load thread (on startup)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

	luaContextData* lcd = GetLuaContextData(L);
	if (!lcd) return; // CLuaParser
	assert(lcd);

	spring::recursive_mutex* mutex = GetLuaMutex(L);
	lcd->luamutex = mutex;
	mutexes[L] = mutex;
}


void LuaDestroyMutex(lua_State* L)
{
	#if (ENABLE_USERSTATE_LOCKS == 0)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

	if (!GetLuaContextData(L)) return; // CLuaParser
	assert(GetLuaContextData(L));

	if (coroutines.find(L) != coroutines.end()) {
		mutexes.erase(L);
		coroutines.erase(L);
	} else {
		lua_unlock(L);
		assert(mutexes.find(L) != mutexes.end());
		spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
		assert(mutex);
		delete mutex;
		mutexes.erase(L);
		//TODO erase all related coroutines too?
	}
}


void LuaLinkMutex(lua_State* L_parent, lua_State* L_child)
{
	#if (ENABLE_USERSTATE_LOCKS == 0)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

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
	#if (ENABLE_USERSTATE_LOCKS == 0)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

	if (!GetLuaContextData(L)) return; // CLuaParser

	spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;

	if (mutex->try_lock())
		return;

	//static int failedLocks = 0;
	//LOG("LuaMutexLock %i", ++failedLocks);
	mutex->lock();
}


void LuaMutexUnlock(lua_State* L)
{
	#if (ENABLE_USERSTATE_LOCKS == 0)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

	if (!GetLuaContextData(L)) return; // CLuaParser

	spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
	mutex->unlock();
}


void LuaMutexYield(lua_State* L)
{
	#if (ENABLE_USERSTATE_LOCKS == 0)
	//assert(Threading::IsMainThread() || Threading::IsGameLoadThread() || SpringVersion::IsUnitsync());
	return;
	#endif

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

static const unsigned int maxAllocedBytes = 768u * 1024u*1024u;
static const char* maxAllocFmtStr = "%s: cannot allocate more memory! (%u bytes already used, %u bytes maximum)";


void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	auto lcd = (luaContextData*) ud;

	if (nsize == 0) {
		totalBytesAlloced -= osize;
		free(ptr);
		return NULL;
	}

	if ((nsize > osize) && (totalBytesAlloced > maxAllocedBytes)) {
		// better kill Lua than whole engine
		// NOTE: this will trigger luaD_throw --> exit(EXIT_FAILURE)
		LOG_L(L_FATAL, maxAllocFmtStr, (lcd->owner->GetName()).c_str(), (unsigned int) totalBytesAlloced, maxAllocedBytes);
		return NULL;
	}

	#if (!defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI))
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

//////////////////////////////////////////////////////////
////// Custom synced float to string
//////////////////////////////////////////////////////////
void spring_lua_ftoa(float f, char *buf)
{
	//get rid of integers
	if ((float) (int) f == f) {
		sprintf(buf,"%d",(int) f);
		return;
	}

	if (f < 0.0f) {
		f = -f;
		buf[0] = '-';
		++buf;
	}

	int nDigits = 8;
	int e = 0;

	while (f >= 10.0f) {
		f /= 10;
		e += 1;
	}
	while (f < 1.0f) {
		f *= 10;
		e -= 1;
	}



	int pointPos = (e < 10 && e > 0) ? (1 + e) : 1;

	int pos = 0;

	int lastRealDigit = 0;
	while (nDigits > 0) {
		int intPart = f;
		f = f - intPart;
		assert(intPart >= 0 && intPart < 10);
		buf[pos] = intPart + '0';
		if (intPart != 0 || pos < pointPos) {
			lastRealDigit = pos;
		}

		f *= 10;
		++pos;
		--nDigits;
		if (pos == pointPos) {
			buf[pos] = '.';
			++pos;
		}
	}
	//Round
	if (f > 5.0f) {
		pos -= 1;
		while (pos >= 0) {
			if (buf[pos] == '9') {
				buf[pos] = '0';
				if (pos > pointPos) {
					lastRealDigit = pos - 1;
				}
			} else if (buf[pos] == '.') {
				lastRealDigit = pos - 1;
			} else {
				buf[pos] += 1;
				if (pos > pointPos) {
					lastRealDigit = pos;
				}
				break;
			}

			--pos;
		}
		if (pos < 0) {
			//Recalculate exponent and point position
			buf[0] = '1';
			buf[pointPos] = '0';
			e += 1;
			pointPos = (e < 10 && e > 0) ? (1 + e) : 1;
			buf[pointPos] = '.';
		}

	}

	if (e >= 10) {
		sprintf(buf + lastRealDigit + 1, "e+%02d", e);
		lastRealDigit += 4;
	} else if (e < 0) {
		sprintf(buf + lastRealDigit + 1, "e%03d", e);
		lastRealDigit += 4;
	}
	buf[lastRealDigit + 1] = '\0';
}
