/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <xmmintrin.h> //SSE1

#include "System/Matrix44f.h"
#include "System/float4.h"
#include "System/TimeProfiler.h"
#include "System/Misc/SpringTime.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"


#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"
InitSpringTime ist;

static const int testRuns = 40000000;

#define _noinline __attribute__((__noinline__))
typedef CMatrix44f m44 __attribute__((aligned(16)));
typedef float4 f4 __attribute__((aligned(16)));
static m44 m;
static m44 m_;

// MatrixMultiply1 -- a naive C++ matrix-vector multiplication function.
// It's correct, but that's about the only thing impressive about it.
//
// Performance: ~90 cycles/vector
/*_noinline*/ static float4 MatrixMultiply1(const CMatrix44f& m, const float4& vin)
{
	const float v0 =
		m.md[0][0] * vin.x +
		m.md[1][0] * vin.y +
		m.md[2][0] * vin.z +
		m.md[3][0] * vin.w;
	const float v1 =
		m.md[0][1] * vin.x +
		m.md[1][1] * vin.y +
		m.md[2][1] * vin.z +
		m.md[3][1] * vin.w;
	const float v2 =
		m.md[0][2] * vin.x +
		m.md[1][2] * vin.y +
		m.md[2][2] * vin.z +
		m.md[3][2] * vin.w;
	// w-component matters for float4::operator +=
	// the comparison with MatrixMultiply2() would
	// also not be fair otherwise
	const float v3 =
		m.md[0][3] * vin.x +
		m.md[1][3] * vin.y +
		m.md[2][3] * vin.z +
		m.md[3][3] * vin.w;

	return float4(v0, v1, v2, v3);
}


// MatrixMultiply2 -- a faster version of MatrixMultiply1, still in C++.
//
// Performance: 70 cycles/vector
/*_noinline*/ static void MatrixMultiply2(const CMatrix44f& m, const float4& in, float4* vout)
{
	float4& out = *vout;
	out.x =
		m.md[0][0] * in.x +
		m.md[1][0] * in.y +
		m.md[2][0] * in.z +
		m.md[3][0] * in.w;
	out.y =
		m.md[0][1] * in.x +
		m.md[1][1] * in.y +
		m.md[2][1] * in.z +
		m.md[3][1] * in.w;
	out.z =
		m.md[0][2] * in.x +
		m.md[1][2] * in.y +
		m.md[2][2] * in.z +
		m.md[3][2] * in.w;
	out.w =
		m.md[0][3] * in.x +
		m.md[1][3] * in.y +
		m.md[2][3] * in.z +
		m.md[3][3] * in.w;
}


/*_noinline*/ static void MatrixVectorSSE(const CMatrix44f& m, const float4& vin, float4* vout)
{
	__m128& out = *reinterpret_cast<__m128*>(vout);

	out =                 _mm_mul_ps(_mm_loadu_ps(&m.md[0][0]), _mm_set1_ps(vin.x));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&m.md[1][0]), _mm_set1_ps(vin.y)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&m.md[2][0]), _mm_set1_ps(vin.z)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&m.md[3][0]), _mm_set1_ps(vin.w)));
}


_noinline static void MatrixMatrixMultiply(CMatrix44f* m1, const CMatrix44f& m2)
{
	assert(long(&m1->m[0]) % 16 == 0); // 16byte aligned

	__m128& moutc1 = *reinterpret_cast<__m128*>(&m1->md[0]);
	__m128& moutc2 = *reinterpret_cast<__m128*>(&m1->md[1]);
	__m128& moutc3 = *reinterpret_cast<__m128*>(&m1->md[2]);
	__m128& moutc4 = *reinterpret_cast<__m128*>(&m1->md[3]);

	const __m128 m1c1 = _mm_loadu_ps(&m1->md[0][0]);
	const __m128 m1c2 = _mm_loadu_ps(&m1->md[1][0]);
	const __m128 m1c3 = _mm_loadu_ps(&m1->md[2][0]);
	const __m128 m1c4 = _mm_loadu_ps(&m1->md[3][0]);

	assert(m2.m[3] == 0.0f);
	assert(m2.m[7] == 0.0f);
	//assert(m2.m[11] == 0.0f);

	const __m128 m2i0 = _mm_load1_ps(&m2.m[0]);
	const __m128 m2i1 = _mm_load1_ps(&m2.m[1]);
	const __m128 m2i2 = _mm_load1_ps(&m2.m[2]);
	//const __m128 m2i3 = _mm_load1_ps(&m2.m[3]);
	const __m128 m2i4 = _mm_load1_ps(&m2.m[4]);
	const __m128 m2i5 = _mm_load1_ps(&m2.m[5]);
	const __m128 m2i6 = _mm_load1_ps(&m2.m[6]);
	//const __m128 m2i7 = _mm_load1_ps(&m2.m[7]);
	const __m128 m2i8 = _mm_load1_ps(&m2.m[8]);
	const __m128 m2i9 = _mm_load1_ps(&m2.m[9]);
	const __m128 m2i10 = _mm_load1_ps(&m2.m[10]);
	const __m128 m2i11 = _mm_load1_ps(&m2.m[11]);
	const __m128 m2i12 = _mm_load1_ps(&m2.m[12]);
	const __m128 m2i13 = _mm_load1_ps(&m2.m[13]);
	const __m128 m2i14 = _mm_load1_ps(&m2.m[14]);
	const __m128 m2i15 = _mm_load1_ps(&m2.m[15]);

	moutc1 =                    _mm_mul_ps(m1c1, m2i0);
	moutc2 =                    _mm_mul_ps(m1c1, m2i4);
	moutc3 =                    _mm_mul_ps(m1c1, m2i8);
	moutc4 =                    _mm_mul_ps(m1c1, m2i12);

	moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c2, m2i1));
	moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c2, m2i5));
	moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c2, m2i9));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c2, m2i13));

	moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c3, m2i2));
	moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c3, m2i6));
	moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c3, m2i10));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c3, m2i14));

	//moutc1 = _mm_add_ps(moutc1, _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[3])));
	//moutc2 = _mm_add_ps(moutc2, _mm_mul_ps(m1c4, _mm_load1_ps(&m2.m[7])));
	moutc3 = _mm_add_ps(moutc3, _mm_mul_ps(m1c4, m2i11));
	moutc4 = _mm_add_ps(moutc4, _mm_mul_ps(m1c4, m2i15));
}


_noinline static void MatrixMultSoft(CMatrix44f* m1, const CMatrix44f& m2)
{
	for (int i = 0; i < 4; ++i) {
		const float m10 = m1->m[0+i];
		const float m11 = m1->m[4+i];
		const float m12 = m1->m[8+i];
		const float m13 = m1->m[12+i];
		m1->m[ 0+i] = m2[0]*m10  + m2[1]*m11  + m2[2]*m12  + m2[3]*m13;
		m1->m[ 4+i] = m2[4]*m10  + m2[5]*m11  + m2[6]*m12  + m2[7]*m13;
		m1->m[ 8+i] = m2[8]*m10  + m2[9]*m11  + m2[10]*m12 + m2[11]*m13;
		m1->m[12+i] = m2[12]*m10 + m2[13]*m11 + m2[14]*m12 + m2[15]*m13;
	}
}


_noinline static int TestMMSpring()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: spring (m = m * m2)");
	m44 m1(m_);
	for (int i = 0; i < testRuns; ++i) {
		m1 = m1 * m;
	}
	return HsiehHash(&m1, sizeof(m44), 0);
}

_noinline static int TestMMFpu()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: fpu (m <<= m2)");
	m44 m1(m_);
	for (int i = 0; i < testRuns; ++i) {
		MatrixMultSoft(&m1, m);
	}
	return HsiehHash(&m1, sizeof(m44), 0);
}

_noinline static int TestMMSpring2()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: spring (m <<= m2)");
	m44 m1(m_);
	for (int i = 0; i < testRuns; ++i) {
		m1 <<= m;
	}
	return HsiehHash(&m1, sizeof(m44), 0);
}

_noinline static int TestMMSSE()
{
	ScopedOnceTimer timer("Matrix-Matrix-Mult: sse");
	m44 m1(m_);
	for (int i = 0; i < testRuns; ++i) {
		MatrixMatrixMultiply(&m1, m);
	}
	return HsiehHash(&m1, sizeof(m44), 0);
}

_noinline static int TestSpring()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: spring");
	f4 v_in(1,2,3,1), v_f;
	for (int i = 0; i < testRuns; ++i) {
		v_f += m * v_in;
		v_in+= v_f;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

_noinline static int TestFPU1()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: fpu1");
	f4 v_in(1,2,3,1), v_out, v_f;
	for (int i = 0; i < testRuns; ++i) {
		v_out = MatrixMultiply1(m, v_in);
		v_f += v_out;
		v_in+= v_f;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

_noinline static int TestFPU2()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: fpu2");
	f4 v_in(1,2,3,1), v_out, v_f;
	for (int i = 0; i < testRuns; ++i) {
		MatrixMultiply2(m, v_in, &v_out);
		v_f += v_out;
		v_in+= v_f;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}

_noinline static int TestSSE()
{
	ScopedOnceTimer timer("Matrix-Vector-Mult: sse");
	f4 v_in(1,2,3,1), v_out, v_f;
	for (int i = 0; i < testRuns; ++i) {
		MatrixVectorSSE(m, v_in, &v_out);
		v_f += v_out;
		v_in+= v_f;
	}
	return HsiehHash(&v_f, sizeof(float4), 0);
}


TEST_CASE("Matrix44VectorMultiply")
{

	for (int i = 0; i < 16; ++i) {
		if ((i != 7) && (i != 3)) {
			m[i] = float(i + 1) / 31.5f;
			m_[i] = float(i + 1) / 31.5f;
		}
	}

	const int correctHash = TestSpring();
	CHECK(TestFPU1() == correctHash);
	CHECK(TestFPU2() == correctHash);
	CHECK(TestSSE()  == correctHash);

	spring_clock::PopTickRate();
}

TEST_CASE("Matrix44MatrixMultiply")
{
	for (int i = 0; i < 16; ++i) {
		if ((i != 7) && (i != 3)) {
			m[i] = float(i + 1) / 31.3125f;
		} else {
			m[i] = 0.0f;
		}
	}
}
