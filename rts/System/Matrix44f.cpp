#include "Matrix44f.h"
#include "math.h"
#include <memory.h>
#include "mmgr.h"

CR_BIND_STRUCT(CMatrix44f);

CR_REG_METADATA(CMatrix44f, CR_MEMBER(m));

/*
float globalArf=15;
extern void arfurf();
extern void TestDenormal(float f);
*/
CMatrix44f::CMatrix44f(void)
{
	// 4x4 identity matrix
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.;
}

CMatrix44f::CMatrix44f(const float3& pos,const float3& x,const float3& y,const float3& z)
{
	m[0] = x.x, m[1] = x.y, m[2] = x.z, m[3] = 0.;
	m[4] = y.x, m[5] = y.y, m[6] = y.z, m[7] = 0.;
	m[8] = z.x, m[9] = z.y, m[10] = z.z, m[11] = 0.;
	m[12] = pos.x , m[13] = pos.y, m[14] = pos.z, m[15] = 1.;
}

CMatrix44f::~CMatrix44f(void)
{
}

void CMatrix44f::RotateX(float rad)
{
	const float sr=sin(rad);
	const float cr=cos(rad);

	CMatrix44f rm;
	rm[5]=cr;
	rm[10]=cr;
	rm[9]=sr;
	rm[6]=-sr;

	*this=Mul(rm);
}

void CMatrix44f::RotateY(float rad)
{
	const float sr=sin(rad);
	const float cr=cos(rad);

	CMatrix44f rm;
	rm[0]=cr;
	rm[10]=cr;
	rm[2]=sr;
	rm[8]=-sr;

	*this=Mul(rm);
}

void CMatrix44f::RotateZ(float rad)
{
	const float sr=sin(rad);
	const float cr=cos(rad);

	CMatrix44f rm;
	rm[0]=cr;
	rm[5]=cr;
	rm[4]=sr;
	rm[1]=-sr;

	*this=Mul(rm);
}

void CMatrix44f::Translate(float x, float y, float z)
{
	CMatrix44f tm;

	tm[12]=x;
	tm[13]=y;
	tm[14]=z;

	*this=Mul(tm);
}

void CMatrix44f::Translate(const float3& pos)
{
	CMatrix44f tm;

	tm[12]=pos.x;
	tm[13]=pos.y;
	tm[14]=pos.z;

	*this=Mul(tm);
}

CMatrix44f CMatrix44f::Mul(const CMatrix44f& m2) const 
{
	CMatrix44f res;

/*
	for(int x=0;x<4;++x){
		for(int y=0;y<4;++y){
			res[x*4+y]=0;
			for(int a=0;a<4;++a){
				res[x*4+y]+=m[a*4+y]*other[x*4+a];
			}
		}
	}
/*/	float m20, m21, m22, m23;

	for(int i = 0; i < 16; i += 4){
		m20 = m2[i], m21 = m2[i+1], m22 = m2[i+2], m23 = m2[i+3];
		res[i]   = m[0]*m20 + m[4]*m21 + m[8]*m22  + m[12]*m23;
		res[i+1] = m[1]*m20 + m[5]*m21 + m[9]*m22  + m[13]*m23;
		res[i+2] = m[2]*m20 + m[6]*m21 + m[10]*m22 + m[14]*m23;
		res[i+3] = m[3]*m20 + m[7]*m21 + m[11]*m22 + m[15]*m23;
	}/**/
	return res;
}

//Only uses the 3x3 matrix :)
float3 CMatrix44f::Mul(const float3& vect) const
{
/*	float3 res;

	for (int x = 0; x < 3; ++x) {
		res[x] = 0;
		for (int y = 0; y < 3; ++y) {
			res[x] += vect[y] * m[x * 4 + y];
		}
	}

	return res;
/*/	const float v0(vect[0]), v1(vect[1]), v2(vect[2]);
	return float3(	v0*m[0] + v1*m[1] + v2*m[2],
					v0*m[4] + v1*m[5] + v2*m[6],
					v0*m[8] + v1*m[9] + v2*m[10]);/**/
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
	const float sr=sin(rad);
	const float cr=cos(rad);

	for(int a=0;a<3;++a){
		float3 v(m[a*4],m[a*4+1],m[a*4+2]);

		float3 va(axis*v.dot(axis));
		float3 vp(v-va);
		float3 vp2(axis.cross(vp));

		float3 vpnew(vp*cr+vp2*sr);
		float3 vnew(va+vpnew);

		m[a*4]=vnew.x;
		m[a*4+1]=vnew.y;
		m[a*4+2]=vnew.z;
	}

	
}

