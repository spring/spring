#include "StdAfx.h"
#include "myMath.h"
#include "math.h"
#include "mmgr.h"

CR_BIND_STRUCT(float2);
CR_REG_METADATA(float2, (CR_MEMBER(x), CR_MEMBER(y)));

CR_BIND_STRUCT(int2);
CR_REG_METADATA(int2, (CR_MEMBER(x),CR_MEMBER(y)));

float3 headingToVectorTable[1024];

class CMyMath
{
public:
	CMyMath()
	{
		for(int a=0;a<1024;++a){
			float ang=(a-512)*2*PI/1024;
			float3 v;
			v.x=sin(ang);
			v.y=0;
			v.z=cos(ang);
			headingToVectorTable[a]=v;
			headingToVectorTable[a].Normalize();
		}
	}
};


float3 GetVectorFromHAndPExact(short int heading,short int pitch)
{
	float3 ret;
	float h=heading*PI/32768;
	float p=pitch*PI/32768;
	ret.x=sin(h)*cos(p);
	ret.y=sin(p);
	ret.z=cos(h)*cos(p);
	return ret;
}

CMyMath dummyMathObject;

