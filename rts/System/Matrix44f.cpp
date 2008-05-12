#include "StdAfx.h"
#include "Matrix44f.h"
#include <memory.h>
#include "mmgr.h"

CR_BIND(CMatrix44f, );

CR_REG_METADATA(CMatrix44f, CR_MEMBER(m));


CMatrix44f::CMatrix44f(void)
{
	LoadIdentity();
}


CMatrix44f::CMatrix44f(const float3& pos,const float3& x,const float3& y,const float3& z)
{
	m[0]  = x.x;   m[1]  = x.y;   m[2]  = x.z;   m[3]  = 0.0f;
	m[4]  = y.x;   m[5]  = y.y;   m[6]  = y.z;   m[7]  = 0.0f;
	m[8]  = z.x;   m[9]  = z.y;   m[10] = z.z;   m[11] = 0.0f;
	m[12] = pos.x; m[13] = pos.y; m[14] = pos.z; m[15] = 1.0f;
}


CMatrix44f::CMatrix44f(const CMatrix44f& n)
{
	m[0]  = n[0];   m[1]  = n[1];   m[2]  = n[2];   m[3]  = n[3];
	m[4]  = n[4];   m[5]  = n[5];   m[6]  = n[6];   m[7]  = n[7];
	m[8]  = n[8];   m[9]  = n[9];   m[10] = n[10];  m[11] = n[11];
	m[12] = n[12];  m[13] = n[13];  m[14] = n[14];  m[15] = n[15];
}


CMatrix44f::~CMatrix44f(void)
{
}


void CMatrix44f::LoadIdentity()
{
	m[0]  = m[5]  = m[10] = m[15] = 1.0f;

	m[1]  = m[2]  = m[3]  = 0.0f;
	m[4]  = m[6]  = m[7]  = 0.0f;
	m[8]  = m[9]  = m[11] = 0.0f;
	m[12] = m[13] = m[14] = 0.0f;
//	memset(m, 0, 16 * sizeof(float));
//	m[0] = m[5] = m[10] = m[15] = 1.0f;
}


void CMatrix44f::RotateX(float rad)
{
/*
	const float sr = sin(rad);
	const float cr = cos(rad);

	CMatrix44f rm;
	rm[5]  = +cr;
	rm[10] = +cr;
	rm[9]  = +sr;
	rm[6]  = -sr;

	*this=Mul(rm);
*/
	const float sr = sin(rad);
	const float cr = cos(rad);

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
	const float sr = sin(rad);
	const float cr = cos(rad);

	CMatrix44f rm;
	rm[0]  = +cr;
	rm[10] = +cr;
	rm[2]  = +sr;
	rm[8]  = -sr;

	*this = Mul(rm);
*/
	const float sr = sin(rad);
	const float cr = cos(rad);

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
	const float sr = sin(rad);
	const float cr = cos(rad);

	CMatrix44f rm;
	rm[0] = +cr;
	rm[5] = +cr;
	rm[4] = +sr;
	rm[1] = -sr;

	*this = Mul(rm);
*/
	const float sr = sin(rad);
	const float cr = cos(rad);

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


void CMatrix44f::Translate(float x, float y, float z)
{
/*
	CMatrix44f tm;

	tm[12] = x;
	tm[13] = y;
	tm[14] = z;

	*this = Mul(tm);
*/

	m[12] += x*m[0] + y*m[4] + z*m[8];
	m[13] += x*m[1] + y*m[5] + z*m[9];
	m[14] += x*m[2] + y*m[6] + z*m[10];
	m[15] += x*m[3] + y*m[7] + z*m[11];
}


void CMatrix44f::Translate(const float3& pos)
{
/*
	CMatrix44f tm;

	tm[12] = pos.x;
	tm[13] = pos.y;
	tm[14] = pos.z;

	*this = Mul(tm);
*/

	const float x=pos.x;
	const float y=pos.y;
	const float z=pos.z;
	m[12] += x*m[0] + y*m[4] + z*m[8];
	m[13] += x*m[1] + y*m[5] + z*m[9];
	m[14] += x*m[2] + y*m[6] + z*m[10];
	m[15] += x*m[3] + y*m[7] + z*m[11];
}


CMatrix44f CMatrix44f::Mul(const CMatrix44f& m2) const 
{
	CMatrix44f res;

	float m20, m21, m22, m23;

	for(int i = 0; i < 16; i += 4){
		m20 = m2[i];
		m21 = m2[i+1];
		m22 = m2[i+2];
		m23 = m2[i+3];

		res[i]   = m[0]*m20 + m[4]*m21 + m[8]*m22  + m[12]*m23;
		res[i+1] = m[1]*m20 + m[5]*m21 + m[9]*m22  + m[13]*m23;
		res[i+2] = m[2]*m20 + m[6]*m21 + m[10]*m22 + m[14]*m23;
		res[i+3] = m[3]*m20 + m[7]*m21 + m[11]*m22 + m[15]*m23;
	}
/*
	for(int x=0;x<4;++x){
		for(int y=0;y<4;++y){
			res[x*4+y]=0;
			for(int a=0;a<4;++a){
				res[x*4+y]+=m[a*4+y]*other[x*4+a];
			}
		}
	}
/*/
	return res;
}


float3 CMatrix44f::Mul(const float3& vect) const
{
	const float v0(vect[0]), v1(vect[1]), v2(vect[2]);
	return float3(	v0*m[0] + v1*m[4] + v2*m[8] + m[12],
				v0*m[1] + v1*m[5] + v2*m[9] + m[13],
				v0*m[2] + v1*m[6] + v2*m[10] + m[14]);
}


void CMatrix44f::SetUpVector(float3& up)
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


void CMatrix44f::Rotate(float rad, float3& axis)
{
	const float sr = sin(rad);
	const float cr = cos(rad);

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

// note: assumes this matrix only
// does translation and rotation
CMatrix44f& CMatrix44f::InvertInPlace()
{
	// transpose the rotation
	float tmp = 0.0f;
	tmp = m[1]; m[1] = m[4]; m[4] = tmp;
	tmp = m[2]; m[2] = m[8]; m[8] = tmp;
	tmp = m[6]; m[6] = m[9]; m[9] = tmp;

	// get the inverse translation
	float3 t(-m[12], -m[13], -m[14]);

	// do the actual inversion
	m[12] = t.x * m[0] + t.y * m[4] + t.z * m[ 8];
	m[13] = t.x * m[1] + t.y * m[5] + t.z * m[ 9];
	m[14] = t.x * m[2] + t.y * m[6] + t.z * m[10];

	return *this;
}

// note: assumes this matrix only
// does translation and rotation;
// m.Mul(m.Invert()) is identity
CMatrix44f CMatrix44f::Invert() const
{
	CMatrix44f mInv(*this);

	// transpose the rotation
	mInv[1] = m[4]; mInv[4] = m[1];
	mInv[2] = m[8]; mInv[8] = m[2];
	mInv[6] = m[9]; mInv[9] = m[6];

	// get the inverse translation
	float3 t(-m[12], -m[13], -m[14]);

	// do the actual inversion
	mInv[12] = t.x * mInv[0] + t.y * mInv[4] + t.z * mInv[ 8];
	mInv[13] = t.x * mInv[1] + t.y * mInv[5] + t.z * mInv[ 9];
	mInv[14] = t.x * mInv[2] + t.y * mInv[6] + t.z * mInv[10];

	return mInv;
}
