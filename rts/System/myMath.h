#ifndef MYMATH_H
#define MYMATH_H

#include "GlobalStuff.h"

#define MaxByAbs(a,b) (abs((a)) > abs((b))) ? (a) : (b);

#define SHORTINT_MAXVALUE	32767

extern float3 headingToVectorTable[1024];

inline short int GetHeadingFromVector(float dx,float dz)
{
	float h;
	//printf("(%.2f,%.2f)\n",dx,dz);
	if(dz!=0){
		float d=dx/dz;
		if(d > 1){
			h=(PI/2) - d/(d*d + 0.28);
		} else if (d < -1){
			h=-(PI/2) - d/(d*d + 0.28);
		}else{
			h=d/(1 + 0.28 * d*d);
		}
		if (dz<0) {
		  if (dx>0)
		    h+=PI;
		  else
		    h-=PI;
		}

	} else {
		if(dx>0)
			h=PI/2;
		else
			h=-PI/2;
	}

	h*=SHORTINT_MAXVALUE/PI;

	// Prevents h from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following 
	// conversion to a short int crashes.
	if (h > SHORTINT_MAXVALUE) h=SHORTINT_MAXVALUE;
	return (short int) h;
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
	h*=SHORTINT_MAXVALUE/PI;
	
	// Prevents h from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following 
	// conversion to a short int crashes.
	if (h > SHORTINT_MAXVALUE) h=SHORTINT_MAXVALUE;
	ret.x=(short int) h;
	ret.y=(short int) (asin(vec.y)*(SHORTINT_MAXVALUE/PI));
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

#endif /* MYMATH_H */
