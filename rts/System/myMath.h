#ifndef MYMATH_H
#define MYMATH_H

#include "GlobalStuff.h"

#define MaxByAbs(a,b) (abs((a)) > abs((b))) ? (a) : (b);

#define SHORTINT_MAXVALUE	32768


extern float2 headingToVectorTable[1024];

inline short int GetHeadingFromFacing(int facing)
{
	switch (facing) {
		case 0: return 0;		// south
		case 1: return 16384;	// east
		case 2: return  32767;	// north == -32768
		case 3: return -16384;	// west
		default: return 0;
	}
}

inline short int GetHeadingFromVector(float dx, float dz)
{
	float h;
	if (dz != 0) {
		float d = dx / dz;

		if (d > 1) {
			h = (PI / 2) - d / (d * d + 0.28f);
		} else if (d < -1) {
			h = -(PI / 2) - d / (d * d + 0.28f);
		} else {
			h = d / (1 + 0.28f * d * d);
		}

		if (dz < 0) {
			if (dx > 0)
				h += PI;
			else
				h -= PI;
		}
	} else {
		if (dx > 0)
			h = PI / 2;
		else
			h = -PI / 2;
	}

	h *= SHORTINT_MAXVALUE / PI;

	// Prevents h from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
//	if (h > SHORTINT_MAXVALUE) h=SHORTINT_MAXVALUE;
//	return (short int) h;

	int ih = (int) h;
	ih %= SHORTINT_MAXVALUE;
	return (short int) ih;

/*
	float wantedHeading;

	if(dz!=0)
		wantedHeading=atanf(dx/dz);
	else
		wantedHeading=PI;
	if(dz<0)
		wantedHeading+=PI;

	wantedHeading*=32768/PI;

	return wantedHeading;*/
}
struct shortint2{
	short int x,y;
};

//vec should be normalized
inline shortint2 GetHAndPFromVector(const float3& vec)
{
	shortint2 ret;

	// Prevents ret.y from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following 
	// conversion to a short int crashes.
	//this change destroys the whole meaning with using short ints....
	int iy = (int) (streflop::asin(vec.y)*(SHORTINT_MAXVALUE/PI));
	iy %= SHORTINT_MAXVALUE;
	ret.y=(short int) iy;
	ret.x=GetHeadingFromVector(vec.x, vec.z);
	return ret;
}

inline float3 GetVectorFromHeading(short int heading)
{
	float2 v = headingToVectorTable[heading/64+512];
	return float3(v.x,0.0f,v.y);
}

float3 GetVectorFromHAndPExact(short int heading,short int pitch);

inline float3 CalcBeizer(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni=1-i;

	float3 res(p1*ni*ni*ni+p2*3*i*ni*ni+p3*3*i*i*ni+p4*i*i*i);
	return res;
}

float LinePointDist(const float3& l1, const float3& l2, const float3& p);
float3 ClosestPointOnLine(const float3& l1, const float3& l2, const float3& p);

#endif /* MYMATH_H */
