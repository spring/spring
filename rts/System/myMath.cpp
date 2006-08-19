#include "StdAfx.h"
#include "Platform/errorhandler.h"
#include "myMath.h"
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
		// This must be put here too because it's executed before the streflop_init in main().
		// Set single precision floating point math.
		streflop_init<streflop::Simple>();

		for(int a=0;a<1024;++a){
			float ang=(a-512)*2*PI/1024;
			float3 v;
			v.x=sin(ang);
			v.y=0;
			v.z=cos(ang);
			headingToVectorTable[a]=v;
		}
		unsigned checksum = 0;
		for (int a = 0; a < 1024; ++a) {
			for (int b = 0; b < 3; ++b) {
				checksum = 33 * checksum + *(unsigned*) &headingToVectorTable[a][b];
			}
		}
// 		fprintf(stderr, "headingToVectorTable checksum: %08x\n", checksum);
		assert(checksum == 0x617a9968);

		// release mode check
		if (checksum != 0x617a9968)
			handleerror(0, "invalid headingToVectorTable checksum", "Sync Error", 0);
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

float LinePointDist(float3 l1,float3 l2,float3 p)
{
	float3 dir(l2-l1);
	float length=dir.Length();
	if(length==0)
		length=0.1;
	dir/=length;

	float a=(p-l1).dot(dir);
	if(a<0)
		a=0;
	if(a>length)
		a=length;

	float3 p2=p-dir*a;
	return p2.distance(l1);
}

CMyMath dummyMathObject;

