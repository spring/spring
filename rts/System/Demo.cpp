/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Demo.h"

CDemo::CDemo()
{
	demoName = "demos/unnamed.sdf";
	memset(&fileHeader, 0, sizeof(DemoFileHeader));
}
