#include "StdAfx.h"
#include <assert.h>
#include "Platform/errorhandler.h"
#include "myMath.h"
#include "mmgr.h"


float2 headingToVectorTable[NUM_HEADINGS];

class CMyMath
{
public:
	CMyMath()
	{
#ifdef STREFLOP_H
		// This must be put here too because it's executed before the streflop_init in main().
		// Set single precision floating point math.
		streflop_init<streflop::Simple>();
#endif

		for(int a=0;a<NUM_HEADINGS;++a){
			float ang=(a-(NUM_HEADINGS/2))*2*PI/NUM_HEADINGS;
			float2 v;
			v.x=sin(ang);
			v.y=cos(ang);
			headingToVectorTable[a]=v;
		}
		unsigned checksum = 0;
		for (int a = 0; a < NUM_HEADINGS; ++a) {
			checksum = 33 * checksum + *(unsigned*)&headingToVectorTable[a].x;
			checksum *= 33;
			checksum = 33 * checksum + *(unsigned*)&headingToVectorTable[a].y;
		}
#ifdef STREFLOP_H
/*		if (checksum != HEADING_CHECKSUM)
			handleerror(0, "invalid headingToVectorTable checksum", "Sync Error", 0);*/
#endif
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

float LinePointDist(const float3& l1, const float3& l2, const float3& p)
{
	float3 dir(l2-l1);
	float length=dir.Length();
	if(length==0)
		length=0.1f;
	dir/=length;

	float a=(p-l1).dot(dir);
	if(a<0)
		a=0;
	if(a>length)
		a=length;

	float3 p2=p-dir*a;
	return p2.distance(l1);
}

/**
 * @brief calculate closest point on linepiece from l1 to l2
 * Note, this clamps the returned point to a position between l1 and l2.
 */
float3 ClosestPointOnLine(const float3& l1, const float3& l2, const float3& p)
{
	float3 dir(l2-l1);
	float3 pdir(p-l1);
	float length = dir.Length();
	if (fabs(length) < 1e-4f)
		return l1;
	float c = dir.dot(pdir) / length;
	if (c < 0) c = 0;
	if (c > length) c = length;
	return l1 + dir * (c / length);
}

CMyMath dummyMathObject;

