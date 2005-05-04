#include "stdafx.h"
#include "mymath.h"
#include "math.h"
//#include "mmgr.h"


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


/*
Calculates the number of 1:s in a given bitmask.
*/
int CountBits(register unsigned int w)  {
	const unsigned int all1   = ~0;
	const unsigned int mask1h = all1 /  3  << 1;
	const unsigned int mask2l = all1 /  5;
	const unsigned int mask4l = all1 / 17;
	w -= (mask1h & w) >> 1;
	w = (w & mask2l) + ((w>>2) & mask2l);
	w = w + (w >> 4) & mask4l;
	w += w >> 8;
	w += w >> 16;
	return w & 0xff;
}
