#include "StdAfx.h"
#include "Demo.h"

CDemo::CDemo()
{
	demoName = "demos/unnamed.sdf";
	memset(&fileHeader, 0, sizeof(DemoFileHeader));
}
