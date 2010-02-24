/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Resource.h"

#include <float.h>

CR_BIND(CResource, );

CR_REG_METADATA(CResource, (
				CR_MEMBER(name),
				CR_MEMBER(optimum),
				CR_MEMBER(extractorRadius),
				CR_MEMBER(maxWorth)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResource::CResource():
		name("UNNAMED_RESOURCE"),
		optimum(FLT_MAX),
		extractorRadius(0.0f),
		maxWorth(0.0f)
{
}

CResource::~CResource()
{
}
