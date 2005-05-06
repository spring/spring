// float3.cpp: implementation of the float3 class.

//

//////////////////////////////////////////////////////////////////////



#include "StdAfx.h"





//////////////////////////////////////////////////////////////////////

// Construction/Destruction

//////////////////////////////////////////////////////////////////////



float float3::maxxpos=2048;

float float3::maxzpos=2048;



bool float3::CheckInBounds()

{

	bool in=true;

	if(x<1){

		x=1;

		in=false;

	}

	if(z<1){

		z=1;

		in=false;

	}

	if(x>maxxpos){

		x=maxxpos;

		in=false;

	}

	if(z>maxzpos){

		z=maxzpos;

		in=false;

	}



	return in;

}