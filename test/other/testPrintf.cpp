/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Log/ILog.h"
//FIXME #include "System/Sync/FPUCheck.h"
#include <stdio.h>


#define BOOST_TEST_MODULE Printf
#include <boost/test/unit_test.hpp>



BOOST_AUTO_TEST_CASE( Printf )
{
	//FIXME good_fpu_init();
	char s[32];

#ifdef _WIN32
	_set_output_format(_TWO_DIGIT_EXPONENT);
	__mingw_sprintf(s, "%.14g", 1e16);
	LOG("%s",s);
	BOOST_CHECK(strcmp(s, "1.0000000272564e+16") == 0);
	__mingw_sprintf(s, "%.14g", 4266.38916015625);
	LOG("%s",s);
	BOOST_CHECK(strcmp(s, "4266.3891601562") == 0); //prints 4266.3891601563
#endif

	sprintf(s, "%.14g", 1e16);
	LOG("%s",s);
	BOOST_CHECK(strcmp(s, "1.0000000272564e+16") == 0);
	__mingw_sprintf(s, "%.14g", 4266.38916015625);
	LOG("%s",s);
	BOOST_CHECK(strcmp(s, "4266.3891601562") == 0);
}
