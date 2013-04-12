/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <xmmintrin.h>

#include "System/Matrix44f.h"
#include "System/float4.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"



#define BOOST_TEST_MODULE Matrix44f
#include <boost/test/unit_test.hpp>


// MatrixMultiply1 -- a naive C++ matrix-vector multiplication function.
// It's correct, but that's about the only thing impressive about it.
//
// Performance: ~90 cycles/vector
float4 MatrixMultiply1(CMatrix44f &m, float4 &vin)
{
	float v0 =  m.md[0][0]*vin[0] + m.md[0][1]*vin[1] +
			m.md[0][2]*vin[2] + m.md[0][3]*vin[3];
	float v1 =  m.md[1][0]*vin[0] + m.md[1][1]*vin[1] +
			m.md[1][2]*vin[2] + m.md[1][3]*vin[3];
	float v2 =  m.md[2][0]*vin[0] + m.md[2][1]*vin[1] +
			m.md[2][2]*vin[2] + m.md[2][3]*vin[3];
	float v3 =  m.md[3][0]*vin[0] + m.md[3][1]*vin[1] +
			m.md[3][2]*vin[6] + m.md[3][3]*vin[3];
	return float4(v0,v1,v2,v3);
}


// MatrixMultiply2 -- a faster version of MatrixMultiply1, still in C++.
//
// Performance: 70 cycles/vector
void MatrixMultiply2(CMatrix44f &m, float4 *vin, float4 *vout)
{
	float4& in  = *vin;
	float4& out = *vout;
	out[0] =  m.md[0][0]*in[0] + m.md[0][1]*in[1] +
			m.md[0][2]*in[2] + m.md[0][3]*in[3];
	out[1] =  m.md[1][0]*in[0] + m.md[1][1]*in[1] +
			m.md[1][2]*in[2] + m.md[1][3]*in[3];
	out[2] =  m.md[2][0]*in[0] + m.md[2][1]*in[1] +
			m.md[2][2]*in[2] + m.md[2][3]*in[3];
	out[3] =  m.md[3][0]*in[0] + m.md[3][1]*in[1] +
			m.md[3][2]*in[2] + m.md[3][3]*in[3];
}


void MatrixMultiply3(const CMatrix44f& m, const float4 vin, float4* vout)
{
	__m128& out = *reinterpret_cast<__m128*>(vout);

	const __m128& mx = *reinterpret_cast<const __m128*>(&m.md[0]);
	const __m128& my = *reinterpret_cast<const __m128*>(&m.md[1]);
	const __m128& mz = *reinterpret_cast<const __m128*>(&m.md[2]);
	const __m128& mw = *reinterpret_cast<const __m128*>(&m.md[3]);

	out =                 _mm_mul_ps(mx, _mm_load1_ps(&vin.x));
	out = _mm_add_ps(out, _mm_mul_ps(my, _mm_load1_ps(&vin.y)));
	out = _mm_add_ps(out, _mm_mul_ps(mz, _mm_load1_ps(&vin.z)));
	out = _mm_add_ps(out, _mm_mul_ps(mw, _mm_load1_ps(&vin.w)));
}

void MatrixMatrixMultiply(const CMatrix44f& m1, const CMatrix44f& m2, CMatrix44f* mout)
{
	__m128& moutc1 = *reinterpret_cast<__m128*>(&mout->md[0]);
	__m128& moutc2 = *reinterpret_cast<__m128*>(&mout->md[1]);
	__m128& moutc3 = *reinterpret_cast<__m128*>(&mout->md[2]);
	__m128& moutc4 = *reinterpret_cast<__m128*>(&mout->md[3]);

	const __m128& m1c1 = *reinterpret_cast<const __m128*>(&m1.md[0]);
	const __m128& m1c2 = *reinterpret_cast<const __m128*>(&m1.md[1]);
	const __m128& m1c3 = *reinterpret_cast<const __m128*>(&m1.md[2]);
	const __m128& m1c4 = *reinterpret_cast<const __m128*>(&m1.md[3]);

	assert(m2.m[3] == 0.0f);
	assert(m2.m[7] == 0.0f);
	assert(m2.m[11] == 0.0f);

	moutc1 =                    _mm_mul_ps(m1c1, _mm_load1_ps(&m2.m[0]));
	moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c1, _mm_load1_ps(&m2.m[1])));
	moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c1, _mm_load1_ps(&m2.m[2])));
	//moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c1, _mm_load1_ps(&m2.m[3])));

	moutc2 =                    _mm_mul_ps(m1c2, _mm_load1_ps(&m2.m[4]));
	moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c2, _mm_load1_ps(&m2.m[5])));
	moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c2, _mm_load1_ps(&m2.m[6])));
	//moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c2, _mm_load1_ps(&m2.m[7])));

	moutc3 =                    _mm_mul_ps(m1c3, _mm_load1_ps(&m2.m[8]));
	moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c3, _mm_load1_ps(&m2.m[9])));
	moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c3, _mm_load1_ps(&m2.m[10])));
	//moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c3, _mm_load1_ps(&m2.m[11])));

	moutc4 =                    _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[12]));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[13])));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[14])));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[15])));
}

typedef CMatrix44f m44 __attribute__((aligned(16)));
typedef float4 f4 __attribute__((aligned(16)));

static const int testRuns = 4000000;

static inline int TestMMSpring()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: spring (m = m * m2)");
	m44 m1, m2;
	m1.RotateX(1.0f);
	m1.RotateX(2.0f);
	m1.RotateX(3.0f);
	f4 v_in(1,2,3,4), v_f;
	for (int i=0; i<testRuns; ++i) {
		m1 = m1 * m2;
	}
	v_f = m1.Mul(v_in);
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestMMSpring2()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: spring (m *= m2)");
	m44 m1, m2;
	m1.RotateX(1.0f);
	m1.RotateX(2.0f);
	m1.RotateX(3.0f);
	f4 v_in(1,2,3,4), v_f;
	for (int i=0; i<testRuns; ++i) {
		m1 *= m2;
	}
	v_f = m1.Mul(v_in);
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestMMSSE()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: sse");
	m44 m1, m2, mout;
	m1.RotateX(1.0f);
	m1.RotateX(2.0f);
	m1.RotateX(3.0f);
	f4 v_in(1,2,3,4), v_f;
	for (int i=0; i<testRuns; ++i) {
		MatrixMatrixMultiply(m1, m2, &mout);
		m1 = mout;
	}
	v_f = m1.Mul(v_in);
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestSpring()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: spring");
	m44 m;
	f4 v_in(1,2,3,4), v_f;
	for (int i=0; i<testRuns; ++i) {
		v_f += m.Mul(v_in);
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestFPU1()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: fpu1");
	m44 m;
	f4 v_in(1,2,3,4), v_out, v_f;
	for (int i=0; i<testRuns; ++i) {
		v_out = MatrixMultiply1(m, v_in);
		v_f += v_out;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestFPU2()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: fpu2");
	m44 m;
	f4 v_in(1,2,3,4), v_out, v_f;
	for (int i=0; i<testRuns; ++i) {
		MatrixMultiply2(m, &v_in, &v_out);
		v_f += v_out;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

static inline int TestSSE()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: sse");
	m44 m;
	f4 v_in(1,2,3,4), v_out, v_f;
	for (int i=0; i<testRuns; ++i) {
		MatrixMultiply3(m, v_in, &v_out);
		v_f += v_out;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

BOOST_AUTO_TEST_CASE( Matrix44VectorMultiply )
{
	const int correctHash = TestSpring();
	BOOST_CHECK(TestFPU1()   == correctHash);
	BOOST_CHECK(TestFPU2()   == correctHash);
	BOOST_CHECK(TestSSE()    == correctHash);
}

BOOST_AUTO_TEST_CASE( Matrix44MatrixMultiply )
{
	const int correctHash = TestMMSpring();
	BOOST_CHECK(TestMMSpring2() == correctHash);
	BOOST_CHECK(TestMMSSE()     == correctHash);
}
