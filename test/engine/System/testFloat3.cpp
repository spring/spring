/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float3.h"
#include "System/float4.h"

#define BOOST_TEST_MODULE Float3
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( Float3 )
{
	BOOST_CHECK_MESSAGE(offsetof(float3, x) == 0,             "offsetof(float3, x) == 0");
	BOOST_CHECK_MESSAGE(offsetof(float3, y) == sizeof(float), "offsetof(float3, y) == sizeof(float)");
	BOOST_CHECK_MESSAGE(sizeof(float3) == 3 * sizeof(float),  "sizeof(float3) == 3 * sizeof(float)");
}

BOOST_AUTO_TEST_CASE( Float34_comparison )
{
	const float nearZero = float3::CMP_EPS*0.1f;
	const float nearOne = 1.0f + nearZero;
	const float big = 100000.0f;
	const float nearBig = big + big*nearZero;

	const float3 f3_Zero(ZeroVector);
	const float3 f3_NearZero(nearZero, nearZero, nearZero);
	const float3 f3_One(OnesVector);
	const float3 f3_NearOne(nearOne, nearOne, nearOne);
	const float3 f3_Big(big, big, big);
	const float3 f3_NearBig(nearBig, nearBig, nearBig);

	const float4 f4_Zero(ZeroVector, 0.0f);
	const float4 f4_NearZero(nearZero, nearZero, nearZero, nearZero);
	const float4 f4_One(OnesVector, 1.0f);
	const float4 f4_NearOne(nearOne, nearOne, nearOne, nearOne);
	const float4 f4_Big(big, big, big, big);
	const float4 f4_NearBig(nearBig, nearBig, nearBig, nearBig);

	BOOST_CHECK(f3_NearZero == f3_Zero);
	BOOST_CHECK(f3_Zero == f3_NearZero);
	BOOST_CHECK(f4_NearZero == f4_Zero);
	BOOST_CHECK(f4_Zero == f4_NearZero);

	BOOST_CHECK(f3_NearOne == f3_One);
	BOOST_CHECK(f3_One == f3_NearOne);
	BOOST_CHECK(f4_NearOne == f4_One);
	BOOST_CHECK(f4_One == f4_NearOne);

	BOOST_CHECK(f3_NearBig == f3_Big);
	BOOST_CHECK(f3_Big == f3_NearBig);
	BOOST_CHECK(f4_NearBig == f4_Big);
	BOOST_CHECK(f4_Big == f4_NearBig);

}
