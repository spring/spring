#include "StdAfx.h"
#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "Rendering/GL/myGL.h"
#include "Platform/ConfigHandler.h"
#include "DynWater.h"
#include "mmgr.h"

CBaseWater* water=0;

CBaseWater::CBaseWater(void)
{
	drawReflection=false;
 	noWakeProjectiles=true;
 	drawSolid=false;
}

CBaseWater::~CBaseWater(void)
{
}

CBaseWater* CBaseWater::GetWater()
{
	if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",1)==2 && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"waterdyn.fp")  && ProgramStringIsNative(GL_VERTEX_PROGRAM_ARB,"waterdyn.vp"))
		return new CDynWater;
	else if(GLEW_ARB_fragment_program && configHandler.GetInt("ReflectiveWater",1) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp"))
		return new CAdvWater;
	else
		return new CBasicWater;
}
