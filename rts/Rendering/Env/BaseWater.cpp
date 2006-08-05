#include "StdAfx.h"
#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "Rendering/GL/myGL.h"
#include "Platform/ConfigHandler.h"
#include "DynWater.h"
#include "RefractWater.h"
#include "mmgr.h"

CBaseWater* water=0;

CBaseWater::CBaseWater(void)
{
	drawReflection=false;
	drawRefraction=false;
 	noWakeProjectiles=false;
 	drawSolid=false;
}

CBaseWater::~CBaseWater(void)
{
}

CBaseWater* CBaseWater::GetWater()
{
	if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",3)==2 && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"waterDyn.fp") && GLEW_ARB_texture_float)
		return new CDynWater;
	else if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",3)==3 && GLEW_ARB_texture_rectangle)
		return new CRefractWater;
	else if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",3) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp"))
		return new CAdvWater;
	else
		return new CBasicWater;
}
