/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Demo.h"

#include <cstring>

CDemo::CDemo():
	demoName("demos/unnamed.sdfz")
{
	memset(&fileHeader, 0, sizeof(DemoFileHeader));
}
