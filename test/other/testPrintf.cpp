/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Log/ILog.h"
#include "lib/lua/include/LuaUser.h"
//FIXME #include "System/Sync/FPUCheck.h"

#define __USE_MINGW_ANSI_STDIO 1 // use __mingw_sprintf() for sprintf(), instead of __builtin_sprintf()
#include <stdio.h>
#include <map>

#define BOOST_TEST_MODULE Printf
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( Printf )
{
	//FIXME good_fpu_init();
	bool isWindows = false;
	char s[32];
	constexpr const char* FMT_STRING = "%.14g";
	std::map<float, const char*> testNumbers = {
		{1e16, "1.0000000272564e+16"},
		{4266.38916015625, "4266.3891601562"},
		{1.6, "1.6000000238419"},
	};


#ifdef _WIN32
	isWindows = true;
	_set_output_format(_TWO_DIGIT_EXPONENT);
	for (const auto& p: testNumbers) {
		__mingw_sprintf(s, FMT_STRING, p.first);
		LOG("%s",s);
		BOOST_CHECK(strcmp(s, p.second) == 0);
	}

	for (const auto& p: testNumbers) {
		__builtin_sprintf(s, FMT_STRING, p.first);
		LOG("%s",s);
		BOOST_CHECK((strcmp(s, p.second) == 0) || isWindows); // it's known to not sync `Mingw vs. Linux`
	}
#endif


	for (const auto& p: testNumbers) {
		sprintf(s, FMT_STRING, p.first);
		LOG("%s",s);
		BOOST_CHECK(strcmp(s, p.second) == 0);
	}

	std::map<float, const char*> springTestNumbers = {
		{1e16, "1.0000002e+16"},
		{4266.38916015625, "4266.3889"},
		{1.6, "1.6"},
	};

	for (const auto& p: springTestNumbers) {
		spring_lua_ftoa(p.first, s);
		LOG("%s",s);
		BOOST_CHECK(strcmp(s, p.second) == 0);
	}


}
