#include "stdafx.h"
#include "basewater.h"
#include "basicwater.h"
#include "advwater.h"
#include "mygl.h"
#include "reghandler.h"
//#include "mmgr.h"

CBaseWater* water=0;

CBaseWater::CBaseWater(void)
{
	drawReflection=false;
}

CBaseWater::~CBaseWater(void)
{
}

CBaseWater* CBaseWater::GetWater()
{
	if(GLEW_ARB_fragment_program && regHandler.GetInt("ReflectiveWater",1) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp"))
		return new CAdvWater;
	else
		return new CBasicWater;
}
