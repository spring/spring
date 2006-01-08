#include "StdAfx.h"
#include "myMath.h"
#include "math.h"
#include "mmgr.h"


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
#if 0
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
#else
	/*
	 * This is faster, and runs in parallel
	 */
	const int S[] = {1, 2, 4, 8, 16};
	const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
	int c = w;
	c = ((c >> S[0]) & B[0]) + (c & B[0]);
	c = ((c >> S[1]) & B[1]) + (c & B[1]);
	c = ((c >> S[2]) & B[2]) + (c & B[2]);
	c = ((c >> S[3]) & B[3]) + (c & B[3]);
	c = ((c >> S[4]) & B[4]) + (c & B[4]);
	return c;
#endif
}

unsigned int NextPwr2(unsigned int x)
{
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return ++x;
}
