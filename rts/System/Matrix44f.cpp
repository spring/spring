/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Matrix44f.h"
#include <memory.h>

CR_BIND(CMatrix44f, );

CR_REG_METADATA(CMatrix44f, CR_MEMBER(m));


CMatrix44f::CMatrix44f()
{
	LoadIdentity();
}

CMatrix44f::CMatrix44f(const CMatrix44f& mat)
{
	for (int i = 0; i < 16; i += 4) {
		m[i    ] = mat[i    ];
		m[i + 1] = mat[i + 1];
		m[i + 2] = mat[i + 2];
		m[i + 3] = mat[i + 3];
	}
}


CMatrix44f::CMatrix44f(const float3& pos, const float3& x, const float3& y, const float3& z)
{
	// column-major!
	m[0]  = x.x;   m[4]  = y.x;   m[8]  = z.x;   m[12] = pos.x;
	m[1]  = x.y;   m[5]  = y.y;   m[9]  = z.y;   m[13] = pos.y;
	m[2]  = x.z;   m[6]  = y.z;   m[10] = z.z;   m[14] = pos.z;
	m[3]  = 0.0f;  m[7]  = 0.0f;  m[11] = 0.0f;  m[15] = 1.0f;
}


CMatrix44f::CMatrix44f(const float& rotX, const float& rotY, const float& rotZ)
{
	LoadIdentity();
	RotateX(rotX);
	RotateY(rotY);
	RotateZ(rotZ);
}


CMatrix44f::CMatrix44f(const float3& pos)
{
	m[0]  = m[5]  = m[10] = m[15] = 1.0f;

	m[1]  = m[2]  = m[3]  = 0.0f;
	m[4]  = m[6]  = m[7]  = 0.0f;
	m[8]  = m[9]  = m[11] = 0.0f;

	m[12] = pos.x; m[13] = pos.y; m[14] = pos.z;
}


void CMatrix44f::LoadIdentity()
{
	m[0]  = m[5]  = m[10] = m[15] = 1.0f;

	m[1]  = m[2]  = m[3]  = 0.0f;
	m[4]  = m[6]  = m[7]  = 0.0f;
	m[8]  = m[9]  = m[11] = 0.0f;
	m[12] = m[13] = m[14] = 0.0f;
}


void CMatrix44f::RotateX(float rad)
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
}


void CMatrix44f::RotateY(float rad)
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
}


void CMatrix44f::RotateZ(float rad)
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
}


void CMatrix44f::Rotate(float rad, const float3& axis)
{
	const float sr = math::sin(rad);
	const float cr = math::cos(rad);

	for(int a=0;a<3;++a){
		float3 v(m[a*4],m[a*4+1],m[a*4+2]);

		float3 va(axis*v.dot(axis));
		float3 vp(v-va);
		float3 vp2(axis.cross(vp));

		float3 vpnew(vp*cr+vp2*sr);
		float3 vnew(va+vpnew);

		m[a*4]     = vnew.x;
		m[a*4 + 1] = vnew.y;
		m[a*4 + 2] = vnew.z;
	}
}


void CMatrix44f::Scale(const float3 scales)
{
	m[0]  *= scales.x;
	m[1]  *= scales.y;
	m[2]  *= scales.z;

	m[4]  *= scales.x;
	m[5]  *= scales.y;
	m[6]  *= scales.z;

	m[8]  *= scales.x;
	m[9]  *= scales.y;
	m[10] *= scales.z;

	m[12] *= scales.x;
	m[13] *= scales.y;
	m[14] *= scales.z;
}


void CMatrix44f::Translate(float x, float y, float z)
{
	m[12] += x*m[0] + y*m[4] + z*m[8];
	m[13] += x*m[1] + y*m[5] + z*m[9];
	m[14] += x*m[2] + y*m[6] + z*m[10];
	m[15] += x*m[3] + y*m[7] + z*m[11];
}


void CMatrix44f::Translate(const float3& pos)
{
	const float& x=pos.x;
	const float& y=pos.y;
	const float& z=pos.z;
	m[12] += x*m[0] + y*m[4] + z*m[8];
	m[13] += x*m[1] + y*m[5] + z*m[9];
	m[14] += x*m[2] + y*m[6] + z*m[10];
	m[15] += x*m[3] + y*m[7] + z*m[11];
}


void CMatrix44f::SetPos(float x, float y, float z)
{
	m[12] = x*m[0] + y*m[4] + z*m[8];
	m[13] = x*m[1] + y*m[5] + z*m[9];
	m[14] = x*m[2] + y*m[6] + z*m[10];
	m[15] = x*m[3] + y*m[7] + z*m[11];
}


void CMatrix44f::SetPos(const float3& pos)
{
	const float& x=pos.x;
	const float& y=pos.y;
	const float& z=pos.z;
	m[12] = x*m[0] + y*m[4] + z*m[8];
	m[13] = x*m[1] + y*m[5] + z*m[9];
	m[14] = x*m[2] + y*m[6] + z*m[10];
	m[15] = x*m[3] + y*m[7] + z*m[11];
}


CMatrix44f CMatrix44f::operator* (const CMatrix44f& m2) const
{
	CMatrix44f res(*this);
	res *= m2;
	return res;
}


CMatrix44f& CMatrix44f::operator*= (const CMatrix44f& m2)
{
	float m10, m11, m12, m13;

	for(int i = 0; i < 16; i += 4){
		m10 = m[i];
		m11 = m[i+1];
		m12 = m[i+2];
		m13 = m[i+3];

		m[i]   = m2[0]*m10 + m2[4]*m11 + m2[8]*m12  + m2[12]*m13;
		m[i+1] = m2[1]*m10 + m2[5]*m11 + m2[9]*m12  + m2[13]*m13;
		m[i+2] = m2[2]*m10 + m2[6]*m11 + m2[10]*m12 + m2[14]*m13;
		m[i+3] = m2[3]*m10 + m2[7]*m11 + m2[11]*m12 + m2[15]*m13;
	}

	return (*this);
}


float3 CMatrix44f::Mul(const float3& vect) const
{
	const float v0(vect[0]), v1(vect[1]), v2(vect[2]);
	return float3(
		v0*m[0] + v1*m[4] + v2*m[8] + m[12],
		v0*m[1] + v1*m[5] + v2*m[9] + m[13],
		v0*m[2] + v1*m[6] + v2*m[10] + m[14]);
}


void CMatrix44f::SetUpVector(const float3& up)
{
	float3 zdir(m[8],m[9],m[10]);
	float3 xdir(zdir.cross(up));
	xdir.Normalize();
	zdir=up.cross(xdir);
	m[0]=xdir.x;
	m[1]=xdir.y;
	m[2]=xdir.z;

	m[4]=up.x;
	m[5]=up.y;
	m[6]=up.z;

	m[8]=zdir.x;
	m[9]=zdir.y;
	m[10]=zdir.z;
}


void CMatrix44f::Transpose()
{
	std::swap(md[0][1], md[1][0]);
	std::swap(md[0][2], md[2][0]);
	std::swap(md[0][3], md[3][0]);

	std::swap(md[1][2], md[2][1]);
	std::swap(md[1][3], md[3][1]);

	std::swap(md[2][3], md[3][2]);
}


//! note: assumes this matrix only
//! does translation and rotation
CMatrix44f& CMatrix44f::InvertAffineInPlace()
{
	//! transpose the rotation
	float tmp = 0.0f;
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
static inline T CalculateCofactor(const T m[4][4], int ei, int ej)
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
