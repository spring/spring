/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "lib/streflop/streflop_cond.h"
#include "lib/lua/include/LuaUser.h"


#define __USE_MINGW_ANSI_STDIO 1 // use __mingw_sprintf() for sprintf(), instead of __builtin_sprintf()
#include <stdio.h>
#include <vector>
#include <limits>
#include <cmath>


#define BOOST_TEST_MODULE Printf
#include <boost/test/unit_test.hpp>


static char s[128];
constexpr const char* FMT_STRING = "%.14g";


static inline float randf() {
	return rand() / float(RAND_MAX);
}
static inline float RandFloat(const float min, const float max) {
	// we work with large floats here (1e22 etc.) we cannot blend them without overflows, so do it a bit differently
	if (rand() & 1) {
		return max * randf();
	}
	return min * randf();
}
static inline float RandInt() {
	return rand() - rand();
}


static const std::vector<std::pair<float, const char*>> testNumbers = {
	// precision
	{1200.12345, "1200.12341"},
	{4266.38345015625, "4266.3833"},
	{1.6, "1.60000002"},
	{0.016, "0.016"},
	{-2, "-2"},
	{-1.0000003, "-1.0000004"},
	{math::acos(-1.f), "3.14159274" /*3.141592653589793*/}, // PI

	// large/extreme numbers
	{7.5e-08, "0.00000008"},
	{7e-16, "7.0e-16"},
	{1e-16, "1.0e-16"},
	{1e16, "1.0e+16"},
	{1000000000, "1000000000"},
	{10000000000, "1.0e+10"},
	{1e22, "1.0e+22"}, // larger than max int64!
	{std::pow(2, 24), "16777216"},
	{std::numeric_limits<int>::max(), "2147483648"},
	{std::numeric_limits<float>::max(), "3.4028e+38"},
	{std::numeric_limits<float>::min(), "1.1755e-38"},

	// infinite numbers
	{std::numeric_limits<float>::quiet_NaN(), "nan"},
// some clang versions have difficulties with nan
// https://llvm.org/bugs/show_bug.cgi?id=16666
// https://stackoverflow.com/a/32224172
#ifndef __clang__
	{-std::numeric_limits<float>::quiet_NaN(), "-nan"},
#endif
	{std::numeric_limits<float>::infinity(), "inf"},
	{-std::numeric_limits<float>::infinity(), "-inf"}
};





BOOST_AUTO_TEST_CASE( SpringFormat )
{
	streflop::streflop_init<streflop::Simple>();

	// printf & spring output should be equal when a precision is specified

	LOG("");
	LOG("--------------------");
	LOG("-- spring_lua_format");
	LOG("%40s %40s", "[spring]", "[printf]");

	const std::vector<const char*> fmts = {
		"11.3", "+7.1" , " 3.5", ".0", ".15"
	};

	char s1[128];
	char s2[128];
	auto CHECK_FORMAT = [&](const float f, const char* fmt) {
		spring_lua_format(f, fmt, s1);
		sprintf(s2, (std::string("%") + fmt + "f").c_str(), f);
		LOG("%40s %40s", s1, s2);

#ifdef WIN32
		if (!std::isnan(f)) // window's printf doesn't support signed NaNs
#endif
		BOOST_CHECK(strcmp(s1, s2) == 0);
	};

	// test #1
	for (const auto& fmt: fmts) {
		LOG("format: %%%sf", fmt);
		for (const auto& p: testNumbers) {
			CHECK_FORMAT(p.first, fmt);
		}
	}

	// test #2
	srand( 0 );
	for (int j=25000; j>0; --j) {
		const float f = RandFloat(1e-22, 1e22);
		sprintf(s1, "%+.5f", f);
		spring_lua_format(f, "+.5", s2);
		BOOST_CHECK(strcmp(s1, s2) == 0);
	}
}


BOOST_AUTO_TEST_CASE( Printf )
{
	LOG("");
	LOG("----------------------");
	LOG("-- printf comparisions");
#ifdef _WIN32
	LOG("\n__mingw_sprintf:");
	_set_output_format(_TWO_DIGIT_EXPONENT);
	for (const auto& p: testNumbers) {
		__mingw_sprintf(s, FMT_STRING, p.first);
		LOG("%20s [%s]", s, p.second);
		BOOST_WARN(strcmp(s, p.second) == 0);
	}

	LOG("\n__builtin_sprintf:");
	for (const auto& p: testNumbers) {
		__builtin_sprintf(s, FMT_STRING, p.first);
		LOG("%20s [%s]", s, p.second);
		BOOST_WARN(strcmp(s, p.second) == 0); // it's known to not sync `Mingw vs. Linux`
	}
#endif

	LOG("\nsprintf:");
	for (const auto& p: testNumbers) {
		sprintf(s, FMT_STRING, p.first);
		LOG("%20s [%s]", s, p.second);
		BOOST_WARN(strcmp(s, p.second) == 0); // not sync Mingw vs Linux
	}

	LOG("\nspring_lua_ftoa:");
	for (const auto& p: testNumbers) {
		spring_lua_ftoa(p.first, s);
		LOG("%20s [%s]", s, p.second);
		BOOST_CHECK(strcmp(s, p.second) == 0);
	}
}


BOOST_AUTO_TEST_CASE( Roundings )
{
	LOG("");
	LOG("----------------");
	LOG("-- roundings");

	static const std::vector<std::tuple<float, const char*, const char*>> testNumbers = {
		std::make_tuple(0.9996f, "1.000", ".3"),
		std::make_tuple(0.9995f, "1.000", ".3"),
		std::make_tuple(0.99945f, "0.999", ".3"),
		std::make_tuple(1999999.0f, "1999999", ".0"),
		std::make_tuple(1999999.7f, "2000000", ".0"),
		std::make_tuple(-1999999.0f, "-1999999", ".0"),
		std::make_tuple(-1999999.7f, "-2000000", ".0")
	};

	for (const auto& p: testNumbers) {
		spring_lua_format(std::get<0>(p), std::get<2>(p), s);
		LOG("%f -\"%%%sf\"-> %s [%s]", std::get<0>(p), std::get<2>(p), s, std::get<1>(p));
		BOOST_CHECK(strcmp(s, std::get<1>(p)) == 0);
	}
}


BOOST_AUTO_TEST_CASE( Precision )
{
	LOG("");
	LOG("----------------");
	LOG("-- precision");

	char s1[128];
	char s2[128];
	char s3[128];
	auto CHECK_PRECISION = [&](const float f) {
		sprintf(s1, FMT_STRING, f);
		const double floatPrintf = atof(s1);
		sprintf(s3, "%.8f", f);
		const double floatPrintf2 = atof(s3);

		spring_lua_ftoa(f, s2);
		const double floatSpring = atof(s2);

		const double diffPrintf = std::abs(f - floatPrintf);
		const double diffPrintf2 = std::abs(f - floatPrintf2);
		const double diffSpring = std::abs(f - floatSpring);
		const double maxDiff = std::abs(f * 1e-4);

		bool inPrecisionRange = false;
		inPrecisionRange |= (diffSpring <= maxDiff);
		inPrecisionRange |= (diffSpring <= (diffPrintf * 1.5f));

		// spring_lua_ftoa hasn't used scientific notation,
		// but sprintf might have (check the non-scientific sprintf one then)
		if (strchr(s2, 'e') == nullptr)
			inPrecisionRange |= (diffSpring <= (diffPrintf2 * 1.5f));

		if (!inPrecisionRange) {
			LOG_L(L_ERROR, "number=%e maxError=%e printf=%s [error=%e] spring=%s [error=%e]", f, maxDiff, s1, diffPrintf, s2, diffSpring);
		}
		BOOST_CHECK(inPrecisionRange);
	};

	for (const auto& p: testNumbers) {
		if (!std::isfinite(p.first))
			continue;

		CHECK_PRECISION(p.first);
	}

	srand( 0 );
	for (int j=25000; j>0; --j) {
		CHECK_PRECISION(RandFloat(1e-5, 1e5));
	}
}


BOOST_AUTO_TEST_CASE( Performance )
{
	LOG("");
	LOG("----------------");
	LOG("-- performance");

	const int iterations = 1000000;

	// small floats
	{
		srand( 0 );
		ScopedOnceTimer foo("printf() - floats");
		for (int j=iterations; j>0; --j) {
			sprintf(s, FMT_STRING, randf() * 10000);
		}
	}
	{
		srand( 0 );
		ScopedOnceTimer foo("spring_lua_ftoa() - floats");
		for (int j=iterations; j>0; --j) {
			spring_lua_ftoa(randf() * 10000, s);
		}
	}

	// large floats
	{
		srand( 0 );
		ScopedOnceTimer foo("printf() - large floats");
		for (int j=iterations; j>0; --j) {
			sprintf(s, FMT_STRING, RandFloat(1e-22, 1e22));
		}
	}
	{
		srand( 0 );
		ScopedOnceTimer foo("spring_lua_ftoa() - large floats");
		for (int j=iterations; j>0; --j) {
			spring_lua_ftoa(RandFloat(1e-22, 1e22), s);
		}
	}

	// integers
	{
		srand( 0 );
		ScopedOnceTimer foo("printf() - ints");
		for (int j=iterations; j>0; --j) {
			sprintf(s, FMT_STRING, RandInt());
		}
	}
	{
		srand( 0 );
		ScopedOnceTimer foo("spring_lua_ftoa() - ints");
		for (int j=iterations; j>0; --j) {
			spring_lua_ftoa(RandInt(), s);
		}
	}
}
