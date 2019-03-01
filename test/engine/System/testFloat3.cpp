/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include "System/float3.h"
#include "System/float4.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

InitSpringTime ist;


static inline float randf() {
	return rand() / float(RAND_MAX);
}
static inline float RandFloat(const float min, const float max) {
	return min + (max - min) * randf();
}


static inline bool equals_legacy(const float3& f1, const float3& f2)
{
	return math::fabs(f1.x - f2.x) <= float3::cmp_eps() * math::fabs(f1.x)
		&& math::fabs(f1.y - f2.y) <= float3::cmp_eps() * math::fabs(f1.y)
		&& math::fabs(f1.z - f2.z) <= float3::cmp_eps() * math::fabs(f1.z);
}


static inline bool equals_new(const float3& f1, const float3& f2)
{
	return (epscmp(f1.x, f2.x, float3::cmp_eps()) && epscmp(f1.y, f2.y, float3::cmp_eps()) && epscmp(f1.z, f2.z, float3::cmp_eps()));
}


static inline bool epscmp2(const float& f) { return (f <= (float3::cmp_eps() * f)); }
static inline bool equals_distance(const float3& f1, const float3& f2)
{
	return ((f1.x == f2.x) && (f1.y == f2.y) && (f1.z == f2.z))
		|| epscmp2(f1.SqDistance(f2));
}


static inline bool equals_sse(const float3& f1, const float3& f2)
{
	// same as equals_new() just with SSE
	__m128 eq;
	__m128 m1 = _mm_set_ps(f1[0], f1[1], f1[2], 0.f);
	__m128 m2 = _mm_set_ps(f2[0], f2[1], f2[2], 0.f);
	eq = _mm_cmpeq_ps(m1, m2);
	if ((eq[0] != 0) && (eq[1] != 0) && (eq[2] != 0))
		return true;

	static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
	static const __m128 eps = _mm_set1_ps(float3::cmp_eps());
	static const __m128 ones = _mm_set1_ps(1.f);
	__m128 am1 = _mm_andnot_ps(sign_mask, m1);
	__m128 am2 = _mm_andnot_ps(sign_mask, m2);
	__m128 right = _mm_add_ps(am1, am2);
	right = _mm_add_ps(right, ones);
	right = _mm_mul_ps(right, eps);
	__m128 left = _mm_sub_ps(m1, m2);
	left = _mm_andnot_ps(sign_mask, left);

	eq = _mm_cmple_ps(left, right);
	return ((eq[0] != 0) && (eq[1] != 0) && (eq[2] != 0));
}



TEST_CASE("Float3")
{
	CHECK(offsetof(float3, x) == 0);
	CHECK(offsetof(float3, y) == sizeof(float));
	CHECK(sizeof(float3) == 3 * sizeof(float));
}

TEST_CASE("Float34_comparison")
{
	const float nearZero = float3::cmp_eps()*0.1f;
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

	CHECK(f3_NearZero == f3_Zero);
	CHECK(f3_Zero == f3_NearZero);
	CHECK(f4_NearZero == f4_Zero);
	CHECK(f4_Zero == f4_NearZero);

	CHECK(f3_NearOne == f3_One);
	CHECK(f3_One == f3_NearOne);
	CHECK(f4_NearOne == f4_One);
	CHECK(f4_One == f4_NearOne);

	CHECK(f3_NearBig == f3_Big);
	CHECK(f3_Big == f3_NearBig);
	CHECK(f4_NearBig == f4_Big);
	CHECK(f4_Big == f4_NearBig);
}


TEST_CASE("Float34_comparison_Performance")
{
	srand( 0 );
	std::array<float3, 100> v;
	for (float3& f: v) {
		f.x = RandFloat(0.f, 5000.f);
		f.y = RandFloat(0.f, 5000.f);
		f.z = RandFloat(0.f, 5000.f);
	}
	auto w = v;
	auto u = v;
	for (int i=0; i<u.size(); ++i) {
		if (i % 2 == 0)
			u[i+1] = u[i];
	}

	const std::int64_t iterations = 100000000;
	int b[5] = {false, false, false, false, false};

	LOG("float3:");
	{
		ScopedOnceTimer foo(" float::operator==() (  0% equality)");
		for (auto j=iterations; j>0; --j) {
			b[0] ^= (v[j % v.size()] == v[(j+1) % v.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() ( 50% equality)");
		for (auto j=iterations; j>0; --j) {
			b[0] ^= (u[j % u.size()] == u[(j+1) % u.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() (100% equality)");
		for (auto j=iterations; j>0; --j) {
			b[0] ^= (v[j % v.size()] == w[j % v.size()]) * j;
		}
	}

	LOG("legacy:");
	{
		ScopedOnceTimer foo(" float::operator==() (  0% equality)");
		for (auto j=iterations; j>0; --j) {
			b[1] ^= equals_legacy(v[j % v.size()], v[(j+1) % v.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() ( 50% equality)");
		for (auto j=iterations; j>0; --j) {
			b[1] ^= equals_legacy(u[j % u.size()], u[(j+1) % u.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() (100% equality)");
		for (auto j=iterations; j>0; --j) {
			b[1] ^= equals_legacy(v[j % v.size()], w[j % v.size()]) * j;
		}
	}

	LOG("new (inlined):");
	{
		ScopedOnceTimer foo(" float::operator==() (  0% equality)");
		for (auto j=iterations; j>0; --j) {
			b[2] ^= equals_new(v[j % v.size()], v[(j+1) % v.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() ( 50% equality)");
		for (auto j=iterations; j>0; --j) {
			b[2] ^= equals_new(u[j % u.size()], u[(j+1) % u.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() (100% equality)");
		for (auto j=iterations; j>0; --j) {
			b[2] ^= equals_new(v[j % v.size()], w[j % v.size()]) * j;
		}
	}

	LOG("new (SSE impl):");
	{
		ScopedOnceTimer foo(" float::operator==() (  0% equality)");
		for (auto j=iterations; j>0; --j) {
			b[3] ^= equals_sse(v[j % v.size()], v[(j+1) % v.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() ( 50% equality)");
		for (auto j=iterations; j>0; --j) {
			b[3] ^= equals_sse(u[j % u.size()], u[(j+1) % u.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() (100% equality)");
		for (auto j=iterations; j>0; --j) {
			b[3] ^= equals_sse(v[j % v.size()], w[j % v.size()]) * j;
		}
	}

	LOG("distance:");
	{
		ScopedOnceTimer foo(" float::operator==() (  0% equality)");
		for (auto j=iterations; j>0; --j) {
			b[4] ^= equals_distance(v[j % v.size()], v[(j+1) % v.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() ( 50% equality)");
		for (auto j=iterations; j>0; --j) {
			b[4] ^= equals_distance(u[j % u.size()], u[(j+1) % u.size()]) * j;
		}
	}
	{
		ScopedOnceTimer foo(" float::operator==() (100% equality)");
		for (auto j=iterations; j>0; --j) {
			b[4] ^= equals_distance(v[j % v.size()], w[j % v.size()]) * j;
		}
	}

	CHECK(((b[0] == b[1]) && (b[2] == b[3]) && (b[1] == b[2]) && (b[3] == b[4])));
}
