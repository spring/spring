#ifndef MYMATH_H
#define MYMATH_H
/*pragma once removed*/

#include "GlobalStuff.h"

#define MaxByAbs(a,b) (abs((a)) > abs((b))) ? (a) : (b);


extern float3 headingToVectorTable[1024];

inline short int GetHeadingFromVector(float dx,float dz)
{
	float h;
	if(dz!=0){
		float d=dx/dz;
		if(d > 1){
			h=(PI/2) - d/(d*d + 0.28);
		} else if (d < -1){
			h=-(PI/2) - d/(d*d + 0.28);
		}else{
			h=d/(1 + 0.28 * d*d);
		}

		if(h<0)
			h+=PI;
	} else {
		if(dx>0)
			h=PI/2;
		else
			h=-PI/2;
	}

	h*=32768/PI;
	return (short int) h;
/*
	float wantedHeading;

	if(dz!=0)
		wantedHeading=atanf(dx/dz);
	else
		wantedHeading=PI;
	if(h<0)
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
	float h;
	if(vec.z!=0){
		float d=vec.x/vec.z;
		if(d > 1){
			h=(PI/2) - d/(d*d + 0.28);
		} else if (d < -1){
			h=-(PI/2) - d/(d*d + 0.28);
		}else{
			h=d/(1 + 0.28 * d*d);
		}

		if(vec.z<0)
			h+=PI;
	} else {
		if(vec.x>0)
			h=PI;
		else
			h=-PI;
	}

//	h+=PI;
	h*=32768/PI;
	ret.x=(short int) h;

	ret.y=(short int) (asin(vec.y)*(32768/PI));

	return ret;
}

inline float3 GetVectorFromHeading(short int heading)
{
	return headingToVectorTable[heading/64+512];
}

float3 GetVectorFromHAndPExact(short int heading,short int pitch);

inline float3 CalcBeizer(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni=1-i;

	float3 res(p1*ni*ni*ni+p2*3*i*ni*ni+p3*3*i*i*ni+p4*i*i*i);
	return res;
}


unsigned int NextPwr2(unsigned int num);
/*
Gives the number of 1:s in a given bitmask.
*/
int CountBits(register unsigned int bitmask);

#endif /* MYMATH_H */
