/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <cmath>
#include "lib/streflop/streflop_cond.h"
#include "System/TimeProfiler.h"
#include "System/FastMath.h"


#define BOOST_TEST_MODULE SQRT
#include <boost/test/unit_test.hpp>


static inline float randf() {
	return rand() / float(RAND_MAX);
}
static inline float RandFloat(const float min, const float max) {
	return min + (max - min) * randf();
}



BOOST_AUTO_TEST_CASE( SQRT )
{
	srand( 0 );
	int iterations = 0xFFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("sqrt_builtin");
		for (auto j=iterations; j>0; --j) {
			hash[0] += fastmath::sqrt_builtin(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("sqrt_sse");
		for (auto j=iterations; j>0; --j) {
			hash[1] += fastmath::sqrt_sse(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("apxsqrt");
		for (auto j=iterations; j>0; --j) {
			hash[2] += fastmath::apxsqrt(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("apxsqrt2");
		for (auto j=iterations; j>0; --j) {
			hash[3] += fastmath::apxsqrt2(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::sqrt");
		for (auto j=iterations; j>0; --j) {
			hash[4] += streflop::sqrt(RandFloat(0.f, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}


BOOST_AUTO_TEST_CASE( ISQRT )
{
	srand( 0 );
	int iterations = 0xFFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("isqrt_nosse");
		for (auto j=iterations; j>0; --j) {
			hash[0] += fastmath::isqrt_nosse(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("isqrt2_nosse");
		for (auto j=iterations; j>0; --j) {
			hash[1] += fastmath::isqrt2_nosse(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("isqrt_sse");
		for (auto j=iterations; j>0; --j) {
			hash[2] += fastmath::isqrt_sse(RandFloat(0.f, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}


BOOST_AUTO_TEST_CASE( SinCos )
{
	srand( 0 );
	int iterations = 0xFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("fastmath::sin");
		for (auto j=iterations; j>0; --j) {
			hash[0] += fastmath::sin(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("std::sin");
		for (auto j=iterations; j>0; --j) {
			hash[1] += std::sin(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::sin");
		for (auto j=iterations; j>0; --j) {
			hash[2] += streflop::sin(RandFloat(0.f, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}


BOOST_AUTO_TEST_CASE( Floor )
{
	srand( 0 );
	int iterations = 0xFFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("fastmath::floor");
		for (auto j=iterations; j>0; --j) {
			hash[0] += fastmath::floor(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("std::floor");
		for (auto j=iterations; j>0; --j) {
			hash[1] += std::floor(RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::floor");
		for (auto j=iterations; j>0; --j) {
			hash[2] += streflop::floor(RandFloat(0.f, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}


BOOST_AUTO_TEST_CASE( Pow )
{
	srand( 0 );
	int iterations = 0xFFFFFF;
	int iterations2 = 0xFFFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("std::pow");
		for (auto j=iterations; j>0; --j) {
			hash[0] += std::pow(RandFloat(0.f, 1e6), RandFloat(0.f, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::pow");
		for (auto j=iterations; j>0; --j) {
			hash[1] += streflop::pow(RandFloat(0.f, 1e6), RandFloat(0.f, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}



BOOST_AUTO_TEST_CASE( Abs )
{
	srand( 0 );
	int iterations = 0xFFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("std::abs");
		for (auto j=iterations; j>0; --j) {
			hash[0] += std::abs(RandFloat(-1e6, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::fabs");
		for (auto j=iterations; j>0; --j) {
			hash[1] += streflop::fabs(RandFloat(-1e6, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}



BOOST_AUTO_TEST_CASE( Tan )
{
	srand( 0 );
	int iterations = 0xFFFFF;

	//typedef float(sqrtfunc*)(float*);
	//std::array

	std::array<float, 5> hash;
	hash.fill(0);

	{
		ScopedOnceTimer foo("std::tan");
		for (auto j=iterations; j>0; --j) {
			hash[0] += std::tan(RandFloat(-1e6, 1e6));
		}
	}
	{
		ScopedOnceTimer foo("streflop::tan");
		for (auto j=iterations; j>0; --j) {
			hash[1] += streflop::tan(RandFloat(-1e6, 1e6));
		}
	}

	bool b = true;
	for (float f: hash) {
		b &= (f == hash[0]);
	}
	BOOST_WARN(b);
}
