#include "StdAfx.h"
#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "myGL.h"
#include "ConfigHandler.h"
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
	if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",1) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp"))
		return new CAdvWater;
	else
		return new CBasicWater;
}
