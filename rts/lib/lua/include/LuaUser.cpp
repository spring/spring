/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <cinttypes>

#include "lib/streflop/streflop_cond.h"

#include "LuaInclude.h"
#include "Lua/LuaAllocState.h"
#include "Lua/LuaHandle.h"
#include "Lua/LuaMemPool.h"

#include "System/GlobalRNG.h"
#include "System/SpringMath.h"

#if (ENABLE_USERSTATE_LOCKS != 0)
	#include "System/UnorderedMap.hpp"
	#include "System/Threading/SpringThreading.h"
#endif

#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"

#if defined(DEDICATED) || defined(UNITSYNC) || defined(BUILDING_AI)
#error liblua should be built only once!
#endif




///////////////////////////////////////////////////////////////////////////
// Custom (Unsynced) Random Number Generator

static CGlobalUnsyncedRNG lguRNG;

int spring_lua_unsynced_rand(lua_State* L) {
	const lua_Number r = lguRNG.NextFloat();

	switch (lua_gettop(L)) {
		case 0: {
			lua_pushnumber(L, r);
		} break;
		case 1: {
			const int l = 1;
			const int u = luaL_checkint(L, 1);

			luaL_argcheck(L, 1 <= u, 1, "[spring_lua_unsynced_rand(1, upper)] empty interval");
			lua_pushnumber(L, std::floor(r * (u - l + 1)) + l);
		} break;
		case 2: {
			const int l = luaL_checkint(L, 1);
			const int u = luaL_checkint(L, 2);

			luaL_argcheck(L, l <= u, 2, "[spring_lua_unsynced_rand(lower, upper)] empty interval");
			lua_pushnumber(L, std::floor(r * (u - l + 1)) + l);
		} break;
		default: {
			return luaL_error(L, "[spring_lua_unsynced_rand] wrong number of arguments");
		} break;
	}

	return 1;
}

int spring_lua_unsynced_srand(lua_State* L) {
	if (L == nullptr) {
		lguRNG.Seed(CGlobalUnsyncedRNG::rng_val_type(&L)); // startup
	} else {
		lguRNG.Seed(luaL_checkint(L, 1));
	}

	return 0;
}




///////////////////////////////////////////////////////////////////////////
// Custom Lua Mutexes

#if (ENABLE_USERSTATE_LOCKS != 0)
static spring::unsynced_map<lua_State*, bool> coroutines;
static spring::unsynced_map<lua_State*, spring::recursive_mutex*> mutexes;

static spring::recursive_mutex* GetLuaMutex(lua_State* L)
{
	assert(mutexes[L] == nullptr);
	return new spring::recursive_mutex();
}
#endif


void LuaCreateMutex(lua_State* L)
{
#if (ENABLE_USERSTATE_LOCKS != 0)
	luaContextData* lcd = GetLuaContextData(L);

	if (lcd == nullptr)
		return; // CLuaParser

	assert(lcd != nullptr);

	spring::recursive_mutex* mutex = GetLuaMutex(L);
	lcd->luamutex = mutex;
	mutexes[L] = mutex;
#endif
}

void LuaDestroyMutex(lua_State* L)
{
#if (ENABLE_USERSTATE_LOCKS != 0)
	if (GetLuaContextData(L) == nullptr)
		return; // CLuaParser

	assert(GetLuaContextData(L) != nullptr);

	if (coroutines.find(L) != coroutines.end()) {
		mutexes.erase(L);
		coroutines.erase(L);
		return;
	}

	lua_unlock(L);
	assert(mutexes.find(L) != mutexes.end());
	spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
	assert(mutex);
	delete mutex;
	mutexes.erase(L);
	//TODO erase all related coroutines too?
#endif
}


void LuaLinkMutex(lua_State* L_parent, lua_State* L_child)
{
#if (ENABLE_USERSTATE_LOCKS != 0)
	luaContextData* plcd = GetLuaContextData(L_parent);
	luaContextData* clcd = GetLuaContextData(L_child);

	assert(plcd != nullptr);
	assert(clcd != nullptr);
	assert(plcd == clcd);

	coroutines[L_child] = true;
	mutexes[L_child] = plcd->luamutex;
#endif
}


void LuaMutexLock(lua_State* L)
{
#if (ENABLE_USERSTATE_LOCKS != 0)

	if (GetLuaContextData(L) == nullptr)
		return; // CLuaParser

	spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;

	if (mutex->try_lock())
		return;

	mutex->lock();
#endif
}

void LuaMutexUnlock(lua_State* L)
{
#if (ENABLE_USERSTATE_LOCKS != 0)
	if (GetLuaContextData(L) == nullptr)
		return; // CLuaParser

	spring::recursive_mutex* mutex = GetLuaContextData(L)->luamutex;
	mutex->unlock();
#endif
}


void LuaMutexYield(lua_State* L)
{
#if (ENABLE_USERSTATE_LOCKS != 0)
	assert(GetLuaContextData(L));
	/*mutexes[L]->unlock();
	if (!mutexes[L]->try_lock()) {
		// only yield if another thread is waiting for the mutex
		spring::this_thread::yield();
		mutexes[L]->lock();
	}*/

	static int count = 0;

	if (count-- <= 0)
		count = 30;

	LuaMutexUnlock(L);

	if (count == 30)
		spring::this_thread::yield();

	LuaMutexLock(L);
#endif
}




///////////////////////////////////////////////////////////////////////////
//

static const char* spring_lua_get_handle_name(const CLuaHandle* h) {
	return ((h != nullptr)? (h->GetName()).c_str(): "<null>");
}

const char* spring_lua_get_handle_name(lua_State* L)
{
	const luaContextData* lcd = GetLuaContextData(L);

	if (lcd != nullptr)
		return (spring_lua_get_handle_name(lcd->owner));

	return "";
}




///////////////////////////////////////////////////////////////////////////
// Custom Memory Allocator
//
static constexpr const char* maxAllocFmtStr = "[%s][handle=%s][OOM] synced=%d {alloced,maximum}={%u,%u}bytes\n";

// tracks allocations across all states
static SLuaAllocState gLuaAllocState = {{0}, {0}, {0}, {0}};
static SLuaAllocError gLuaAllocError = {};

void spring_lua_alloc_log_error(const luaContextData* lcd)
{
	const CLuaHandle* lho = lcd->owner;

	const char* lhn = spring_lua_get_handle_name(lho);
	const char* fmt = maxAllocFmtStr;

	SLuaAllocState& s = gLuaAllocState;
	SLuaAllocError& e = gLuaAllocError;

	if (e.msgPtr == nullptr)
		e.msgPtr = &e.msgBuf[0];

	// append to buffer until it fills up or get_error is called
	e.msgPtr += SNPRINTF(e.msgPtr, sizeof(e.msgBuf) - (e.msgPtr - &e.msgBuf[0]), fmt, __func__, lhn, lcd->synced, uint32_t(s.allocedBytes), SLuaAllocState::maxAllocedBytes);
}

void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	luaContextData* lcd = static_cast<luaContextData*>(ud);
	SLuaAllocState* las = &lcd->allocState;
	LuaMemPool* lmp = lcd->memPool;

	gLuaAllocState.allocedBytes -= osize;
	gLuaAllocState.allocedBytes += nsize;
	las->allocedBytes -= osize;
	las->allocedBytes += nsize;

	if (nsize == 0) {
		// deallocation; must return NULL
		lmp->Free(ptr, osize);
		return nullptr;
	}

	if ((nsize > osize) && (gLuaAllocState.allocedBytes.load() > SLuaAllocState::maxAllocedBytes)) {
		// (re)allocation
		// better kill Lua than whole engine; instant desync if synced handle
		// NOTE: this will trigger luaD_throw, which calls exit(EXIT_FAILURE)
		spring_lua_alloc_log_error(lcd);
		return nullptr;
	}

	// ptr is NULL if and only if osize is zero
	// behaves like realloc when nsize!=0 and osize!=0 (ptr != NULL)
	// behaves like malloc when nsize!=0 and osize==0 (ptr == NULL)
	const spring_time t0 = spring_gettime();
	void* mem = lmp->Realloc(ptr, nsize, osize);
	const spring_time t1 = spring_gettime();

	gLuaAllocState.numLuaAllocs += 1;
	gLuaAllocState.luaAllocTime += (t1 - t0).toMicroSecsi();
	las->numLuaAllocs += 1;
	las->luaAllocTime += (t1 - t0).toMicroSecsi();

	return mem;
}

void spring_lua_alloc_get_stats(SLuaAllocState* state)
{
	state->allocedBytes.store(gLuaAllocState.allocedBytes.load());
	state->numLuaAllocs.store(gLuaAllocState.numLuaAllocs.load());
	state->luaAllocTime.store(gLuaAllocState.luaAllocTime.load());

#if (ENABLE_USERSTATE_LOCKS != 0)
	state->numLuaStates.store(mutexes.size() - coroutines.size();
#else
	state->numLuaStates.store(LuaMemPool::GetPoolCount());
#endif
}

bool spring_lua_alloc_skip_gc(float gcLoadMult)
{
	// randomly skip a GC cycle with probability 1 - (weighted memory load ratio)
	const float rawLoadRatio = float(gLuaAllocState.allocedBytes.load()) / float(SLuaAllocState::maxAllocedBytes);
	const float modLoadRatio = gcLoadMult * rawLoadRatio;
	return (lguRNG.NextFloat() > modLoadRatio);
}

bool spring_lua_alloc_get_error(SLuaAllocError* error)
{
	if (gLuaAllocError.msgBuf[0] == 0)
		return false;

	// copy and clear
	std::memcpy(error->msgBuf, gLuaAllocError.msgBuf, sizeof(error->msgBuf));
	std::memset(gLuaAllocError.msgBuf, 0, sizeof(gLuaAllocError.msgBuf));

	gLuaAllocError.msgPtr = &gLuaAllocError.msgBuf[0];
	return true;
}

void spring_lua_alloc_update_stats(int clearStatsFrame)
{
	gLuaAllocState.numLuaAllocs.store(gLuaAllocState.numLuaAllocs * (1 - clearStatsFrame));
	gLuaAllocState.luaAllocTime.store(gLuaAllocState.luaAllocTime * (1 - clearStatsFrame));
}






//////////////////////////////////////////////////////////
////// Custom synced float to string
//////////////////////////////////////////////////////////

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
static inline int sprintf64(char* dst, std::int64_t x) { return sprintf(dst, "%I64d", x); }
#pragma GCC diagnostic pop
#else
static inline int sprintf64(char* dst, long int x)      { return sprintf(dst, "%ld", x); }
static inline int sprintf64(char* dst, long long int x) { return sprintf(dst, "%lld", x); }
#endif

// excluding mantissa, a float has a rest int-precision of: 2^24 = 16,777,216
// int numbers in that range are 100% exact, and don't suffer float precision issues
// static constexpr int MAX_PRECISE_DIGITS_IN_FLOAT = std::numeric_limits<float>::digits10;
// static constexpr auto SPRING_FLOAT_MAX = std::numeric_limits<float>::max();
static constexpr auto SPRING_INT64_MAX = std::numeric_limits<std::int64_t>::max();

static constexpr std::array<double, 11> v = {
	{1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10}
};


static constexpr inline double Pow10d(unsigned i)
{
	return (i < v.size()) ? v[i] : std::pow(double(10), i);
}

static const inline int FastLog10(const float f)
{
	assert(f != 0.0f); // log10(0) = -inf

	if (f < 1.0f || f >= (SPRING_INT64_MAX >> 1))
		return std::floor(std::log10(f));

	const std::int64_t i = f;
	      std::int64_t n = 10;

	int log10 = 0;
	while (i >= n) {
		++log10;
		n *= 10;
	}

	return log10;
}


static constexpr inline int GetDigitsInStdNotation(const int log10)
{
	// log10(0.01) = -2  (4 chars)
	// log10(0.1)  = -1  (3 chars)
	// log10(1)    = 0   (1 char)
	// log10(10)   = 1   (2 chars)
	// log10(100)  = 2   (3 chars)
	return (log10 >= 0) ? (log10 + 1) : (-log10 + 2);
}



static inline int PrintIntPart(char* buf, float f, const bool roundingCarryBit = false)
{
#ifdef WIN32
	if (f < (std::numeric_limits<int>::max() - roundingCarryBit)) {
		return sprintf(buf, "%d", int(f) + roundingCarryBit);
	} else
#endif
	if (f < (SPRING_INT64_MAX - roundingCarryBit))
		return sprintf64(buf, std::int64_t(f) + roundingCarryBit); // much faster than printing a float!

	return sprintf(buf, "%1.0f", f + roundingCarryBit);
}

static inline int PrintFractPart(char* buf, float f, int digits, int precision)
{
	//XXX: Hacks, with streflop enabled we limit the FPU normally to 32bit float
	//     and doing any double math will use floats math then!
	//     But here we need the precision of doubles, so switch the FPU to it just
	//     for this casting.
	//Note: We are still in synced code, so even these doubles need to sync!
	//     Also performance seems to be unaffected by switching the FPU mode.
	streflop::streflop_init<streflop::Double>();

	const char* old = buf;
	char s[16];

	assert(digits <= 15);
	assert(digits <= std::numeric_limits<std::int64_t>::digits10);

	const std::int64_t i = double(f) * Pow10d(digits) + 0.5;
	const int len = sprintf64(s, i);

	if (len < digits) {
		memset(buf, '0', digits - len);
		buf += digits - len;
	}

	memcpy(buf, s, len);
	buf += len;

	// removing trailing zeros
	precision = std::max(1, precision);
	while (buf[-1] == '0' && (buf - old) > precision)
		--buf;
	buf[0] = '\0';

	streflop::streflop_init<streflop::Simple>();
	assert((buf - old) >= 1);
	return (buf - old);
}


static inline bool HandleRounding(float* fractF, int log10, int charsInStdNotation, int nDigits, int precision, bool useScientificNot)
{
	// We handle here the case when rounding in the
	// fract part carries into the integer part.
	// We don't handle the fract rounding itself!
	// fDigits excludes the dot when precision is < 0
	const int iDigits = mix(1, charsInStdNotation, (!useScientificNot && log10 >= 0));
	const int fDigits = mix(std::max(0, nDigits - (iDigits + 1)), precision, (precision >= 0));

	// check fractional part against the rounding limit
	// 1 -> 0.95   -%.1f-> 1.0
	// 2 -> 0.995  -%.2f-> 1.00
	// 3 -> 0.9995 -%.3f-> 1.000
	const float roundLimit = 1.0f - 0.5f * std::pow(0.1f, fDigits);

	if (*fractF >= roundLimit) {
		*fractF = 0.0f;
		return true;
	}

	return false;
}


void spring_lua_ftoa(float f, char* buf, int precision)
{
	static constexpr int MAX_DIGITS = 10;
	static_assert(MAX_DIGITS > 6, "must have enough room for at least 1.0e+23");

	// get rid of integers
	int x = f;
	if (float(x) == f) {
		sprintf(buf, "%i", x);

		if (precision > 0) {
			char* endBuf = strchr(buf, '\0');
			*endBuf = '.';
			++endBuf;
			memset(endBuf, '0', precision);
			endBuf[precision] = '\0';
		}

		return;
	}


	int nDigits = MAX_DIGITS;

	if (std::signbit(f)) { // use signbit() cause < doesn't work with nans
		f = -f;
		buf[0] = '-';
		++buf;
		--nDigits;
	}

	if (std::isinf(f)) {
		strcpy(buf, "inf");
		return;
	}
	if (std::isnan(f)) {
		strcpy(buf, "nan");
		return;
	}

	int e10 = 0;
	const int log10 = FastLog10(f);
	const int charsInStdNotation = GetDigitsInStdNotation(log10);

	if ((charsInStdNotation > nDigits) && (precision == -1)) {
		nDigits -= 4; // space needed for "e+01"
		f *= std::pow(10.0f, -(e10 = log10));
	}

	float truncF;
	float fractF = std::modf(f, &truncF);

	const bool useScientificNot = (e10 != 0);
	const bool roundingCarryBit = HandleRounding(&fractF, log10, charsInStdNotation, nDigits, precision, useScientificNot);

	int iDigits = PrintIntPart(buf, truncF, roundingCarryBit);

	if (useScientificNot) {
		assert(iDigits != 2 || fractF == 0.0f);

		e10 += (iDigits == 2);
		iDigits = mix(iDigits, 1, iDigits == 2);

		assert(iDigits == 1);
	}

	nDigits -= iDigits;
	nDigits = mix(nDigits, precision + 1, precision >= 0); // add 1 for dot if precision is positive
	buf += iDigits;

	if ((nDigits > 1) && (useScientificNot || fractF != 0 || precision > 0)) {
		buf[0] = '.';

		++buf;
		--nDigits;

		buf += PrintFractPart(buf, fractF, nDigits, precision);
	}

	if (!useScientificNot)
		return;

	sprintf(buf, "e%+02d", e10);
}


void spring_lua_format(float f, const char* fmt, char* buf)
{
	if (fmt[0] == '\0')
		return spring_lua_ftoa(f, buf);

	// handles `%(sign)(width)(.precision)f`, i.e. %+10.2f
	char bufC[128];
	char* buf2 = bufC;

	// insert sign; f might be NaN so check with signbit()
	if (fmt[0] == '+' || fmt[0] == ' ') {
		if (!std::signbit(f)) {
			buf2[0] = fmt[0];
			++buf2;
		}
		++fmt;
	}

	// width
	const int width = atoi(fmt);

	// precision
	int precision = -1;
	const char* dotPos = strchr(fmt, '.');

	if (dotPos != nullptr)
		precision = Clamp(atoi(fmt = dotPos + 1), 0, 15);


	// convert the float
	spring_lua_ftoa(f, buf2, precision);

	// right align the number when `width` is given
	const int len = strlen(bufC);
	if (len < width) {
		memset(buf, ' ', width - len);
		buf += (width - len);
	}

	// copy the float string into dst
	memcpy(buf, bufC, len + 1);
}

