/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <array>
#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include "lib/streflop/streflop_cond.h"

#include "LuaInclude.h"
#include "Game/GameVersion.h"
#include "Lua/LuaHandle.h"
#include "System/myMath.h"
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

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
static inline int sprintf64(char* dst, boost::int64_t x) { return sprintf(dst, "%I64d", x); }
#pragma GCC diagnostic pop
#else
static inline int sprintf64(char* dst, long int x)      { return sprintf(dst, "%ld", x); }
static inline int sprintf64(char* dst, long long int x) { return sprintf(dst, "%lld", x); }
#endif

// excluding mantissa, a float has a rest int-precision of: 2^24 = 16,777,216
// int numbers in that range are 100% exact, and don't suffer float precision issues
static constexpr int MAX_PRECISE_DIGITS_IN_FLOAT = std::numeric_limits<float>::digits10;
static constexpr auto SPRING_FLOAT_MAX = std::numeric_limits<float>::max();
static constexpr auto SPRING_INT64_MAX = std::numeric_limits<boost::int64_t>::max();

static constexpr std::array<double, 11> v = {
	1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10
};


static constexpr inline double Pow10d(unsigned i)
{
	return (i<v.size()) ? v[i] : std::pow(double(10), i);
}


static const inline int FastLog10(const float f)
{
	assert(f != 0.0f); // log10(0) = -inf
	if (f>=1.f && f<(SPRING_INT64_MAX >> 1)) {
		const boost::int64_t i = f;
		int log10 = 0;
		boost::int64_t n = 10;
		while (i >= n) {
			++log10;
			n *= 10;
		}
		return log10;
	}
	return std::floor(std::log10(f));
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



static inline int PrintIntPart(char* buf, float f, const bool carrierBit = false)
{
#ifdef WIN32
	if (f < (std::numeric_limits<int>::max() - carrierBit)) {
		return sprintf(buf, "%d", int(f) + carrierBit);
	} else
#endif
	if (f < (SPRING_INT64_MAX - carrierBit)) {
		return sprintf64(buf, boost::int64_t(f) + carrierBit); // much faster than printing a float!
	} else {
		return sprintf(buf, "%1.0f", f + carrierBit);
	}
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

	const auto old = buf;

	assert(digits <= 15);
	assert(digits <= std::numeric_limits<boost::int64_t>::digits10);
	const boost::int64_t i = double(f) * Pow10d(digits) + 0.5;
	char s[16];
	const int len = sprintf64(s, i);
	if (len < digits) {
		memset(buf, '0', digits - len);
		buf += digits - len;
	}
	memcpy(buf, s, len);
	buf += len;

	// removing trailing zeros
	precision = std::max(1, precision);
	while (buf[-1] == '0' && (buf - old) > precision) --buf;
	buf[0] = '\0';

	streflop::streflop_init<streflop::Simple>();
	return (buf - old);
}


static inline bool HandleRounding(float* fractF, int log10, int charsInStdNotation, int nDigits, bool scienceNotation, int precision)
{
	// We handle here the case when rounding in the
	// fract part carries into the integer part.
	// We don't handle the fract rounding itself!

	int iDigits = 1;
	if (!scienceNotation) {
		if (log10 >= 0) {
			iDigits = charsInStdNotation;
		}
	}

	int fDigits = std::max(0, nDigits - (iDigits + 1)); // excluding dot
	if (precision >= 0) {
		fDigits = precision;
	}

	// 1 -> 0.95   -%.1f-> 1.0
	// 2 -> 0.995  -%.2f-> 1.00
	// 3 -> 0.9995 -%.3f-> 1.000
	const float roundLimit = 1.f - 0.5f * std::pow(0.1f, fDigits);
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
			*endBuf = '.'; ++endBuf;
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
		e10 = log10;
		nDigits -= 4; // space needed for "e+01"
		f *= std::pow(10.f, -e10);
	}
	const bool scienceNotation = (e10 != 0);

	float truncF;
	float fractF = std::modf(f, &truncF);
	const bool carrierBit = HandleRounding(&fractF, log10, charsInStdNotation, nDigits, scienceNotation, precision);

	const int iDigits = PrintIntPart(buf, truncF, carrierBit);
	if (scienceNotation)
		assert(iDigits == 1);
	nDigits -= iDigits;
	buf += iDigits;

	if (precision >= 0)
		nDigits = precision + 1; //+1 for dot
	if ((nDigits > 1) && (scienceNotation || fractF != 0 || precision > 0)) {
		buf[0] = '.';
		++buf;
		--nDigits;

		const int fDigits = PrintFractPart(buf, fractF, nDigits, precision);
		assert(fDigits >= 1);
		buf += fDigits;
	}

	if (scienceNotation) {
		sprintf(buf, "e%+02d", e10);
	}
}


void spring_lua_format(float f, const char* fmt, char* buf)
{
	if (fmt[0] == '\0')
		return spring_lua_ftoa(f, buf);

	// handles `%(sign)(width)(.precision)f`, i.e. %+10.2f

	char bufC[128];
	char* buf2 = bufC;

	// sign
	if (fmt[0] == '+' || fmt[0] == ' ') {
		if (!std::signbit(f)) { // use signbit() cause < doesn't work with nans
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
	if (dotPos != nullptr) {
		fmt = dotPos + 1;
		precision = Clamp(atoi(fmt), 0, 15);
	}

	// convert the float
	spring_lua_ftoa(f, buf2, precision);

	// right align the number when `width` is given
	const int len = strlen(bufC);
	if (len < width) {
		memset(buf, ' ', width - len);
		buf += width - len;
	}

	// copy the float string into dst
	memcpy(buf, bufC, len+1);
}
