/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Matrix44f.h"
#include <memory.h>
#include <algorithm>

CR_BIND(CMatrix44f, )

CR_REG_METADATA(CMatrix44f, CR_MEMBER(m))


CMatrix44f::CMatrix44f()
{
	LoadIdentity();
}

CMatrix44f::CMatrix44f(const CMatrix44f& mat)
{
	memcpy(&m[0], &mat.m[0], sizeof(CMatrix44f));
}


CMatrix44f::CMatrix44f(const float3& pos, const float3& x, const float3& y, const float3& z)
{
	// column-major!
	m[0] = x.x;   m[4] = y.x;   m[ 8] = z.x;   m[12] = pos.x;
	m[1] = x.y;   m[5] = y.y;   m[ 9] = z.y;   m[13] = pos.y;
	m[2] = x.z;   m[6] = y.z;   m[10] = z.z;   m[14] = pos.z;
	m[3] = 0.0f;  m[7] = 0.0f;  m[11] = 0.0f;  m[15] = 1.0f;
}

CMatrix44f::CMatrix44f(const float rotX, const float rotY, const float rotZ)
{
	LoadIdentity();
	RotateX(rotX);
	RotateY(rotY);
	RotateZ(rotZ);
}

CMatrix44f::CMatrix44f(const float3& p)
{
	LoadIdentity();
	SetPos(p);
}


int CMatrix44f::IsOrthoNormal(float eps) const
{
	const float3& xdir = GetX();
	const float3& ydir = GetY();
	const float3& zdir = GetZ();

	if (math::fabs(xdir.dot(ydir)) > eps) { return 1; }
	if (math::fabs(ydir.dot(zdir)) > eps) { return 2; }
	if (math::fabs(xdir.dot(zdir)) > eps) { return 3; }

	if (xdir.SqLength() < (1.0f - eps) || xdir.SqLength() > (1.0f + eps)) { return 4; }
	if (ydir.SqLength() < (1.0f - eps) || ydir.SqLength() > (1.0f + eps)) { return 5; }
	if (zdir.SqLength() < (1.0f - eps) || zdir.SqLength() > (1.0f + eps)) { return 6; }

	return 0;
}

int CMatrix44f::IsIdentity(float eps) const
{
	#define ABS(i) math::fabs(m[i])
	if (ABS( 0) > (1.0f + eps) || ABS( 1) > (       eps) || ABS( 2) > (       eps) || ABS( 3) > (       eps)) return 1; // 1st col
	if (ABS( 4) > (       eps) || ABS( 5) > (1.0f + eps) || ABS( 6) > (       eps) || ABS( 7) > (       eps)) return 2; // 2nd col
	if (ABS( 8) > (       eps) || ABS( 9) > (       eps) || ABS(10) > (1.0f + eps) || ABS(11) > (       eps)) return 3; // 3rd col
	if (ABS(12) > (       eps) || ABS(13) > (       eps) || ABS(14) > (       eps) || ABS(15) > (1.0f + eps)) return 4; // 4th col
	#undef ABS
	return 0;
}

CMatrix44f& CMatrix44f::LoadIdentity()
{
	m[ 0] = m[ 5] = m[10] = m[15] = 1.0f;

	m[ 1] = m[ 2] = m[ 3] = 0.0f;
	m[ 4] = m[ 6] = m[ 7] = 0.0f;
	m[ 8] = m[ 9] = m[11] = 0.0f;
	m[12] = m[13] = m[14] = 0.0f;

	return *this;
}


CMatrix44f& CMatrix44f::RotateX(float rad)
{
/*
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	CMatrix44f rm;
	rm[5]  = +cr;
	rm[10] = +cr;
	rm[9]  = +sr;
	rm[6]  = -sr;

	*this=Mul(rm);
*/
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	float a=m[4];
	m[4] = cr*a - sr*m[8];
	m[8] = sr*a + cr*m[8];

	a=m[5];
	m[5] = cr*a - sr*m[9];
	m[9] = sr*a + cr*m[9];

	a=m[6];
	m[6]  = cr*a - sr*m[10];
	m[10] = sr*a + cr*m[10];

	a=m[7];
	m[7]  = cr*a - sr*m[11];
	m[11] = sr*a + cr*m[11];

	return *this;
}


CMatrix44f& CMatrix44f::RotateY(float rad)
{
/*
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	CMatrix44f rm;
	rm[0]  = +cr;
	rm[10] = +cr;
	rm[2]  = +sr;
	rm[8]  = -sr;

	*this = Mul(rm);
*/
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	float a=m[0];
	m[0] =  cr*a + sr*m[8];
	m[8] = -sr*a + cr*m[8];

	a=m[1];
	m[1] =  cr*a + sr*m[9];
	m[9] = -sr*a + cr*m[9];

	a=m[2];
	m[2]  =  cr*a + sr*m[10];
	m[10] = -sr*a + cr*m[10];

	a=m[3];
	m[3]  =  cr*a + sr*m[11];
	m[11] = -sr*a + cr*m[11];

	return *this;
}


CMatrix44f& CMatrix44f::RotateZ(float rad)
{
/*
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	CMatrix44f rm;
	rm[0] = +cr;
	rm[5] = +cr;
	rm[4] = +sr;
	rm[1] = -sr;

	*this = Mul(rm);
*/
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	float a=m[0];
	m[0] = cr*a - sr*m[4];
	m[4] = sr*a + cr*m[4];

	a=m[1];
	m[1] = cr*a - sr*m[5];
	m[5] = sr*a + cr*m[5];

	a=m[2];
	m[2] = cr*a - sr*m[6];
	m[6] = sr*a + cr*m[6];

	a=m[3];
	m[3] = cr*a - sr*m[7];
	m[7] = sr*a + cr*m[7];

	return *this;
}


CMatrix44f& CMatrix44f::Rotate(float rad, const float3& axis)
{
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	for (int a = 0; a < 3; ++a) {
		// a=0: x, a=1: y, a=2: z
		float3 v(m[a*4], m[a*4 + 1], m[a*4 + 2]);

		// project the rotation axis onto x/y/z (va)
		// find component orthogonal to projection (vp)
		// find another vector orthogonal to that (vp2)
		// --> orthonormal basis, va is forward vector
		float3 va(axis * v.dot(axis));
		float3 vp(v - va);
		float3 vp2(axis.cross(vp));

		// rotate vp (around va), set x/y/z to va plus this
		float3 vpnew(vp*cr + vp2*sr);
		float3 vnew(va + vpnew);

		m[a*4    ] = vnew.x;
		m[a*4 + 1] = vnew.y;
		m[a*4 + 2] = vnew.z;
	}

	return *this;
}


CMatrix44f& CMatrix44f::Scale(const float3 scales)
{
	m[ 0] *= scales.x;
	m[ 1] *= scales.y;
	m[ 2] *= scales.z;

	m[ 4] *= scales.x;
	m[ 5] *= scales.y;
	m[ 6] *= scales.z;

	m[ 8] *= scales.x;
	m[ 9] *= scales.y;
	m[10] *= scales.z;

	m[12] *= scales.x;
	m[13] *= scales.y;
	m[14] *= scales.z;

	return *this;
}


CMatrix44f& CMatrix44f::Translate(const float x, const float y, const float z)
{
	m[12] += x*m[0] + y*m[4] + z*m[ 8];
	m[13] += x*m[1] + y*m[5] + z*m[ 9];
	m[14] += x*m[2] + y*m[6] + z*m[10];
	m[15] += x*m[3] + y*m[7] + z*m[11];
	return *this;
}


void CMatrix44f::SetPos(const float3& pos)
{
	const float x = pos.x;
	const float y = pos.y;
	const float z = pos.z;

	m[12] = x*m[0] + y*m[4] + z*m[ 8];
	m[13] = x*m[1] + y*m[5] + z*m[ 9];
	m[14] = x*m[2] + y*m[6] + z*m[10];
	m[15] = x*m[3] + y*m[7] + z*m[11];
}


__FORCE_ALIGN_STACK__
static inline void MatrixMatrixMultiplySSE(const CMatrix44f& m1, const CMatrix44f& m2, CMatrix44f* mout)
{
	//SCOPED_TIMER("CMatrix44f::MM-Mul");

	const __m128 m1c1 = _mm_loadu_ps(&m1.md[0][0]);
	const __m128 m1c2 = _mm_loadu_ps(&m1.md[1][0]);
	const __m128 m1c3 = _mm_loadu_ps(&m1.md[2][0]);
	const __m128 m1c4 = _mm_loadu_ps(&m1.md[3][0]);

	// an optimization we assume
	assert(m2.m[3] == 0.0f);
	assert(m2.m[7] == 0.0f);
	// assert(m2.m[11] == 0.0f); in case of a gluPerspective it's -1

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

	__m128 moutc1, moutc2, moutc3, moutc4;
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

	_mm_storeu_ps(&mout->md[0][0], moutc1);
	_mm_storeu_ps(&mout->md[1][0], moutc2);
	_mm_storeu_ps(&mout->md[2][0], moutc3);
	_mm_storeu_ps(&mout->md[3][0], moutc4);
}


CMatrix44f CMatrix44f::operator* (const CMatrix44f& m2) const
{
	CMatrix44f mout;
	MatrixMatrixMultiplySSE(*this, m2, &mout);
	return mout;
}


CMatrix44f& CMatrix44f::operator>>= (const CMatrix44f& m2)
{
	MatrixMatrixMultiplySSE(m2, *this, this);
	return (*this);
}


CMatrix44f& CMatrix44f::operator<<= (const CMatrix44f& m2)
{
	MatrixMatrixMultiplySSE(*this, m2, this);
	return (*this);
}

__FORCE_ALIGN_STACK__
float3 CMatrix44f::operator* (const float3& v) const
{
	//SCOPED_TIMER("CMatrix44f::Mul");

	/*return float3(
		m[0] * v.x + m[4] * v.y + m[8 ] * v.z + m[12],
		m[1] * v.x + m[5] * v.y + m[9 ] * v.z + m[13],
		m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]);*/

	__m128 out;
	out =                 _mm_mul_ps(_mm_loadu_ps(&md[0][0]), _mm_load1_ps(&v.x));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[1][0]), _mm_load1_ps(&v.y)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[2][0]), _mm_load1_ps(&v.z)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[3][0]), _mm_set1_ps(1.0f)));

	const float* fout = reinterpret_cast<float*>(&out);
	return float3(fout[0], fout[1], fout[2]);
}

__FORCE_ALIGN_STACK__
float4 CMatrix44f::operator* (const float4& v) const
{
	__m128 out;
	out =                 _mm_mul_ps(_mm_loadu_ps(&md[0][0]), _mm_load1_ps(&v.x));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[1][0]), _mm_load1_ps(&v.y)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[2][0]), _mm_load1_ps(&v.z)));
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[3][0]), _mm_load1_ps(&v.w)));

	const float* fout = reinterpret_cast<float*>(&out);
	return float4(fout[0], fout[1], fout[2], fout[3]);
}


void CMatrix44f::SetUpVector(const float3& up)
{
	float3 zdir(m[8], m[9], m[10]);
	float3 xdir(zdir.cross(up));

	xdir.Normalize();
	zdir = up.cross(xdir);

	m[ 0] = xdir.x;
	m[ 1] = xdir.y;
	m[ 2] = xdir.z;

	m[ 4] = up.x;
	m[ 5] = up.y;
	m[ 6] = up.z;

	m[ 8] = zdir.x;
	m[ 9] = zdir.y;
	m[10] = zdir.z;
}


CMatrix44f& CMatrix44f::Transpose()
{
	std::swap(md[0][1], md[1][0]);
	std::swap(md[0][2], md[2][0]);
	std::swap(md[0][3], md[3][0]);

	std::swap(md[1][2], md[2][1]);
	std::swap(md[1][3], md[3][1]);

	std::swap(md[2][3], md[3][2]);
	return *this;
}


//! note: assumes this matrix only
//! does translation and rotation
CMatrix44f& CMatrix44f::InvertAffineInPlace()
{
	//! transpose the rotation
	float tmp;
	tmp = m[1]; m[1] = m[4]; m[4] = tmp;
	tmp = m[2]; m[2] = m[8]; m[8] = tmp;
	tmp = m[6]; m[6] = m[9]; m[9] = tmp;

	//! get the inverse translation
	const float3 t(-m[12], -m[13], -m[14]);

	//! do the actual inversion
	m[12] = t.x * m[0] + t.y * m[4] + t.z * m[ 8];
	m[13] = t.x * m[1] + t.y * m[5] + t.z * m[ 9];
	m[14] = t.x * m[2] + t.y * m[6] + t.z * m[10];

	return *this;
}


CMatrix44f CMatrix44f::InvertAffine() const
{
	CMatrix44f mInv(*this);
	mInv.InvertAffineInPlace();
	return mInv;
}


template<typename T>
static inline T CalculateCofactor(const T m[4][4], const int ei, const int ej)
{
	size_t ai, bi, ci;
	switch (ei) {
		case 0: { ai = 1; bi = 2; ci = 3; break; }
		case 1: { ai = 0; bi = 2; ci = 3; break; }
		case 2: { ai = 0; bi = 1; ci = 3; break; }
		case 3: { ai = 0; bi = 1; ci = 2; break; }
	}
	size_t aj, bj, cj;
	switch (ej) {
		case 0: { aj = 1; bj = 2; cj = 3; break; }
		case 1: { aj = 0; bj = 2; cj = 3; break; }
		case 2: { aj = 0; bj = 1; cj = 3; break; }
		case 3: { aj = 0; bj = 1; cj = 2; break; }
	}

	const T val =
		(m[ai][aj] * ((m[bi][bj] * m[ci][cj]) - (m[bi][cj] * m[ci][bj]))) +
		(m[ai][bj] * ((m[bi][cj] * m[ci][aj]) - (m[bi][aj] * m[ci][cj]))) +
		(m[ai][cj] * ((m[bi][aj] * m[ci][bj]) - (m[bi][bj] * m[ci][aj])));

	if (((ei + ej) & 1) == 0) {
		return +val;
	} else {
		return -val;
	}
}


//! generalized inverse for non-orthonormal 4x4 matrices
//! A^-1 = (1 / det(A)) (C^T)_{ij} = (1 / det(A)) C_{ji}
bool CMatrix44f::InvertInPlace()
{
	float cofac[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			cofac[i][j] = CalculateCofactor(md, i, j);
		}
	}

	const float det =
		(md[0][0] * cofac[0][0]) +
		(md[0][1] * cofac[0][1]) +
		(md[0][2] * cofac[0][2]) +
		(md[0][3] * cofac[0][3]);

	if (det == 0.0f) {
		//! singular matrix, set to identity?
		LoadIdentity();
		return false;
	}

	const float scale = 1.0f / det;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			//! (adjoint / determinant)
			//! (note the transposition in 'cofac')
			md[i][j] = cofac[j][i] * scale;
		}
	}

	return true;
}


//! generalized inverse for non-orthonormal 4x4 matrices
//! A^-1 = (1 / det(A)) (C^T)_{ij} = (1 / det(A)) C_{ji}
CMatrix44f CMatrix44f::Invert(bool* status) const
{
	CMatrix44f mat;
	CMatrix44f& cofac = mat;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			cofac.md[i][j] = CalculateCofactor(md, i, j);
		}
	}

	const float det =
		(md[0][0] * cofac.md[0][0]) +
		(md[0][1] * cofac.md[0][1]) +
		(md[0][2] * cofac.md[0][2]) +
		(md[0][3] * cofac.md[0][3]);

	if (det == 0.0f) {
		//! singular matrix, set to identity?
		mat.LoadIdentity();
		if (status) *status = false;
		return mat;
	}

	mat *= 1.f / det;
	mat.Transpose();

	if (status) *status = true;
	return mat;
}
