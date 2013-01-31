/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float3.h"

#define BOOST_TEST_MODULE Float3
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( Float3 )
{
	BOOST_CHECK_MESSAGE(offsetof(float3, x) == 0,             "offset(float3, x) == 0");
	BOOST_CHECK_MESSAGE(offsetof(float3, y) == sizeof(float), "offset(float3, y) == sizeof(float)");
}
