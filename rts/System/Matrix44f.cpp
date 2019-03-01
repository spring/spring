/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Matrix44f.h"
#include "System/SpringMath.h"

#include <memory.h>
#include <algorithm>

CR_BIND(CMatrix44f, )

CR_REG_METADATA(CMatrix44f, CR_MEMBER(m))

CMatrix44f::CMatrix44f(const CMatrix44f& mat)
{
	memcpy(&m[0], &mat.m[0], sizeof(CMatrix44f));
}


CMatrix44f::CMatrix44f(const float3 pos, const float3 x, const float3 y, const float3 z)
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
	RotateEulerXYZ(float3(rotX, rotY, rotZ));
}

CMatrix44f::CMatrix44f(const float3 p)
{
	LoadIdentity();
	SetPos(p);
}


bool CMatrix44f::IsOrthoNormal() const
{
	const float3& xdir = GetX();
	const float3& ydir = GetY();
	const float3& zdir = GetZ();

	const float3 dots = {xdir.dot(ydir), ydir.dot(zdir), xdir.dot(zdir)};
	const float3 lens = {xdir.SqLength(), ydir.SqLength(), zdir.SqLength()};

	constexpr float3 epsd = {float3::cmp_eps() *  8.0f, float3::cmp_eps() *  8.0f, float3::cmp_eps() *  8.0f};
	constexpr float3 epsl = {float3::cmp_eps() * 16.0f, float3::cmp_eps() * 16.0f, float3::cmp_eps() * 16.0f};

	return (dots.equals(ZeroVector, epsd)) && (lens.equals(OnesVector, epsl));
}

bool CMatrix44f::IsIdentity() const
{
	constexpr float4 x = {1.0f, 0.0f, 0.0f, 0.0f};
	constexpr float4 y = {0.0f, 1.0f, 0.0f, 0.0f};
	constexpr float4 z = {0.0f, 0.0f, 1.0f, 0.0f};
	constexpr float4 w = {0.0f, 0.0f, 0.0f, 1.0f};
	return (col[0] == x && col[1] == y && col[2] == z && col[3] == w);
}


CMatrix44f& CMatrix44f::RotateX(float angle)
{
	angle = ClampRad(angle);

	#if 0
	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	CMatrix44f rm;
	rm[ 5] = +cr;
	rm[10] = +cr;
	rm[ 9] = +sr;
	rm[ 6] = -sr;

	// neater, but more FLOPS
	*this = Mul(rm);

	#else

	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	float a = m[4];
	m[4] = cr*a - sr*m[8];
	m[8] = sr*a + cr*m[8];

	a = m[5];
	m[5] = cr*a - sr*m[9];
	m[9] = sr*a + cr*m[9];

	a = m[6];
	m[ 6] = cr*a - sr*m[10];
	m[10] = sr*a + cr*m[10];

	a = m[7];
	m[ 7] = cr*a - sr*m[11];
	m[11] = sr*a + cr*m[11];
	#endif

	return *this;
}


CMatrix44f& CMatrix44f::RotateY(float angle)
{
	angle = ClampRad(angle);

	#if 0
	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	CMatrix44f rm;
	rm[ 0] = +cr;
	rm[10] = +cr;
	rm[ 2] = +sr;
	rm[ 8] = -sr;

	*this = Mul(rm);

	#else

	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	float a = m[0];
	m[0] =  cr*a + sr*m[8];
	m[8] = -sr*a + cr*m[8];

	a = m[1];
	m[1] =  cr*a + sr*m[9];
	m[9] = -sr*a + cr*m[9];

	a = m[2];
	m[ 2] =  cr*a + sr*m[10];
	m[10] = -sr*a + cr*m[10];

	a = m[3];
	m[ 3] =  cr*a + sr*m[11];
	m[11] = -sr*a + cr*m[11];
	#endif

	return *this;
}


CMatrix44f& CMatrix44f::RotateZ(float angle)
{
	angle = ClampRad(angle);

	#if 0
	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	CMatrix44f rm;
	rm[0] = +cr;
	rm[5] = +cr;
	rm[4] = +sr;
	rm[1] = -sr;

	*this = Mul(rm);
	#else
	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

	float a = m[0];
	m[0] = cr*a - sr*m[4];
	m[4] = sr*a + cr*m[4];

	a = m[1];
	m[1] = cr*a - sr*m[5];
	m[5] = sr*a + cr*m[5];

	a = m[2];
	m[2] = cr*a - sr*m[6];
	m[6] = sr*a + cr*m[6];

	a = m[3];
	m[3] = cr*a - sr*m[7];
	m[7] = sr*a + cr*m[7];
	#endif

	return *this;
}


CMatrix44f& CMatrix44f::Rotate(float angle, const float3 axis)
{
	const float sr = math::sin(angle);
	const float cr = math::cos(angle);

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


CMatrix44f& CMatrix44f::RotateEulerXYZ(const float3 angles)
{
	// rotate around X first, Y second, Z third (R=R(Z)*R(Y)*R(X))
	if (angles[ANGLE_P] != 0.0f) { RotateX(angles[ANGLE_P]); }
	if (angles[ANGLE_Y] != 0.0f) { RotateY(angles[ANGLE_Y]); }
	if (angles[ANGLE_R] != 0.0f) { RotateZ(angles[ANGLE_R]); }
	return *this;
}

CMatrix44f& CMatrix44f::RotateEulerYXZ(const float3 angles)
{
	// rotate around Y first, X second, Z third (R=R(Z)*R(X)*R(Y))
	if (angles[ANGLE_Y] != 0.0f) { RotateY(angles[ANGLE_Y]); }
	if (angles[ANGLE_P] != 0.0f) { RotateX(angles[ANGLE_P]); }
	if (angles[ANGLE_R] != 0.0f) { RotateZ(angles[ANGLE_R]); }
	return *this;
}

CMatrix44f& CMatrix44f::RotateEulerZXY(const float3 angles)
{
	// rotate around Z first, X second, Y third (R=R(Y)*R(X)*R(Z))
	if (angles[ANGLE_R] != 0.0f) { RotateZ(angles[ANGLE_R]); }
	if (angles[ANGLE_P] != 0.0f) { RotateX(angles[ANGLE_P]); }
	if (angles[ANGLE_Y] != 0.0f) { RotateY(angles[ANGLE_Y]); }
	return *this;
}

CMatrix44f& CMatrix44f::RotateEulerZYX(const float3 angles)
{
	// rotate around Z first, Y second, X third (R=R(X)*R(Y)*R(Z))
	if (angles[ANGLE_R] != 0.0f) { RotateZ(angles[ANGLE_R]); }
	if (angles[ANGLE_Y] != 0.0f) { RotateY(angles[ANGLE_Y]); }
	if (angles[ANGLE_P] != 0.0f) { RotateX(angles[ANGLE_P]); }
	return *this;
}


// multiply M by the scale-matrix (SX, SY, SZ, ST)
//
//  [SX][SY][SZ][ST]
//  ----------------
//  [sx   0   0   0]
//  [ 0  sy   0   0]
//  [ 0   0  sz   0]
//  [ 0   0   0   1]
//
CMatrix44f& CMatrix44f::Scale(const float3 scales)
{
	m[ 0] *= scales.x;
	m[ 1] *= scales.x;
	m[ 2] *= scales.x;
	m[ 3] *= scales.x;

	m[ 4] *= scales.y;
	m[ 5] *= scales.y;
	m[ 6] *= scales.y;
	m[ 7] *= scales.y;

	m[ 8] *= scales.z;
	m[ 9] *= scales.z;
	m[10] *= scales.z;
	m[11] *= scales.z;
	return *this;
}

CMatrix44f& CMatrix44f::Translate(const float x, const float y, const float z)
{
	m[12] += (x*m[0] + y*m[4] + z*m[ 8]);
	m[13] += (x*m[1] + y*m[5] + z*m[ 9]);
	m[14] += (x*m[2] + y*m[6] + z*m[10]);
	m[15] += (x*m[3] + y*m[7] + z*m[11]);
	return *this;
}



__FORCE_ALIGN_STACK__
static inline void MatrixMatrixMultiplySSE(const CMatrix44f& m1, const CMatrix44f& m2, CMatrix44f* mout)
{
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
float4 CMatrix44f::operator* (const float4 v) const
{
	#if 0
	float4 out;
	out.x = m[0] * v.x + m[4] * v.y + m[8 ] * v.z + m[12] * v.w;
	out.y = m[1] * v.x + m[5] * v.y + m[9 ] * v.z + m[13] * v.w;
	out.z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w;
	out.w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w;
	return out;
	#else
	__m128 out;
	out =                 _mm_mul_ps(_mm_loadu_ps(&md[0][0]), _mm_set1_ps(v.x)) ; // or _mm_load1_ps(&v.x)
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[1][0]), _mm_set1_ps(v.y))); // or _mm_load1_ps(&v.y)
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[2][0]), _mm_set1_ps(v.z))); // or _mm_load1_ps(&v.z)
	out = _mm_add_ps(out, _mm_mul_ps(_mm_loadu_ps(&md[3][0]), _mm_set1_ps(v.w))); // or _mm_load1_ps(&v.w)

	const float* fout = reinterpret_cast<float*>(&out);
	return {fout[0], fout[1], fout[2], fout[3]};
	#endif
}


void CMatrix44f::SetUpVector(const float3 up)
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
	std::swap(m[1], m[4]);
	std::swap(m[2], m[8]);
	std::swap(m[6], m[9]);

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
		default: { assert(ei < 4); ai = 0; bi = 1; ci = 2; break; }
	}
	size_t aj, bj, cj;
	switch (ej) {
		case 0: { aj = 1; bj = 2; cj = 3; break; }
		case 1: { aj = 0; bj = 2; cj = 3; break; }
		case 2: { aj = 0; bj = 1; cj = 3; break; }
		default: { assert(ej < 4); aj = 0; bj = 1; cj = 2; break; }
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
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
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



// adapted from stackoverflow.com/questions/18433801/converting-a-3x3-matrix-to-euler-tait-bryan-angles-pitch-yaw-roll
//
// NOTE:
//   this assumes a RIGHT-handed coordinate system, but
//   in Spring all SolidObjects use LEFT-handed systems
//   instead
//
//   therefore (if called on an object's transform matrix)
//   the angles {a,b,c} returned here actually correspond
//   to values as though RotateEulerZYX(-a,-b,-c) had been
//   called, *NOT* RotateEulerXYZ(a,b,c) as in the case of
//   a right-handed matrix
//
//   however, since
//
//     1)           RotateEulerZYX(-a,-b,-c)  == Transpose(RotateEulerXYZ(a,b,c))
//     2) Transpose(RotateEulerZYX(-a,-b,-c)) ==           RotateEulerXYZ(a,b,c)
//
//   we can easily convert them back to left-handed form
//
//   all angles are in radians and returned in PYR order
//
float3 CMatrix44f::GetEulerAnglesRgtHand(float eps) const {
	float3 angles[2];
	float  cosYaw[2] = {0.0f, 0.0f};
	float  rotSum[2] = {0.0f, 0.0f};

	if (Square(eps) > math::fabs(m[0 * 4 + 2] + 1.0f)) {
		// x.z == -1 (yaw=PI/2) means gimbal lock between X and Z
		angles[0][ANGLE_R] =         0.0f;
		angles[0][ANGLE_Y] = math::HALFPI;
		angles[0][ANGLE_P] = (angles[0][ANGLE_R] + math::atan2(m[1 * 4 + 0], m[2 * 4 + 0]));

		return angles[0];
	}

	if (Square(eps) > math::fabs(m[0 * 4 + 2] - 1.0f)) {
		// x.z == 1 (yaw=-PI/2) means gimbal lock between X and Z
		angles[0][ANGLE_R] =            0.0f;
		angles[0][ANGLE_Y] = math::NEGHALFPI;
		angles[0][ANGLE_P] = (-angles[0][ANGLE_R] + math::atan2(-m[1 * 4 + 0], -m[2 * 4 + 0]));

		return angles[0];
	}

	// yaw angles (theta)
	//
	//   angles[i][P] :=   psi := Pitch := X-angle
	//   angles[i][Y] := theta :=   Yaw := Y-angle
	//   angles[i][R] :=   phi :=  Roll := Z-angle
	//
	angles[0][ANGLE_Y] = -math::asin(m[0 * 4 + 2]);
	angles[1][ANGLE_Y] = (math::PI - angles[0][ANGLE_Y]);

	// yaw cosines
	cosYaw[0] = math::cos(angles[0][ANGLE_Y]);
	cosYaw[1] = math::cos(angles[1][ANGLE_Y]);

	// psi angles (pitch)
	angles[0][ANGLE_P] = math::atan2((m[1 * 4 + 2] / cosYaw[0]), (m[2 * 4 + 2] / cosYaw[0]));
	angles[1][ANGLE_P] = math::atan2((m[1 * 4 + 2] / cosYaw[1]), (m[2 * 4 + 2] / cosYaw[1]));
	// phi angles (roll)
	angles[0][ANGLE_R] = math::atan2((m[0 * 4 + 1] / cosYaw[0]), (m[0 * 4 + 0] / cosYaw[0]));
	angles[1][ANGLE_R] = math::atan2((m[0 * 4 + 1] / cosYaw[1]), (m[0 * 4 + 0] / cosYaw[1]));

	rotSum[0] = OnesVector.dot(float3::fabs(angles[0])); // |p0|+|y0|+|r0|
	rotSum[1] = OnesVector.dot(float3::fabs(angles[1])); // |p1|+|y1|+|r1|

	// two solutions exist; choose the "shortest" rotation
	return angles[rotSum[0] > rotSum[1]];
}

float3 CMatrix44f::GetEulerAnglesLftHand(float eps) const {
	CMatrix44f matrix = *this;
	// (A*B*C)^T = (C^T)*(B^T)*(A^T)
	matrix.Transpose();
	return (matrix.GetEulerAnglesRgtHand(eps));
}



CMatrix44f CMatrix44f::PerspProj(float aspect, float thfov, float zn, float zf)
{
	const float t = zn * thfov;
	const float b = -t;
	const float l = b * aspect;
	const float r = t * aspect;

	return (PerspProj(l, r,  b, t,  zn, zf));
}

CMatrix44f CMatrix44f::PerspProj(float l, float r, float b, float t, float zn, float zf)
{
	CMatrix44f proj;
	proj[ 0] = (2.0f * zn) / (r - l);
	proj[ 1] =  0.0f;
	proj[ 2] =  0.0f;
	proj[ 3] =  0.0f;

	proj[ 4] =  0.0f;
	proj[ 5] = (2.0f * zn) / (t - b);
	proj[ 6] =  0.0f;
	proj[ 7] =  0.0f;

	proj[ 8] = (r + l) / (r - l);
	proj[ 9] = (t + b) / (t - b);
	proj[10] = -(zf + zn) / (zf - zn);
	proj[11] = -1.0f;

	proj[12] =   0.0f;
	proj[13] =   0.0f;
	proj[14] = -(2.0f * zf * zn) / (zf - zn);
	proj[15] =   0.0f;

	return proj;
}

CMatrix44f CMatrix44f::OrthoProj(float l, float r, float b, float t, float zn, float zf)
{
	CMatrix44f proj;
	const float tx = -(( r +  l) / ( r -  l));
	const float ty = -(( t +  b) / ( t -  b));
	const float tz = -((zf + zn) / (zf - zn));

	proj[ 0] =  2.0f / (r - l);
	proj[ 1] =  0.0f;
	proj[ 2] =  0.0f;
	proj[ 3] =  0.0f;

	proj[ 4] =  0.0f;
	proj[ 5] =  2.0f / (t - b);
	proj[ 6] =  0.0f;
	proj[ 7] =  0.0f;

	proj[ 8] =  0.0f;
	proj[ 9] =  0.0f;
	proj[10] = -2.0f / (zf - zn);
	proj[11] =  0.0f;

	proj[12] = tx;
	proj[13] = ty;
	proj[14] = tz;
	proj[15] = 1.0f;

	return proj;
}

