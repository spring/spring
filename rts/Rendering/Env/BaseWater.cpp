#include "StdAfx.h"
#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "Rendering/GL/myGL.h"
#include "Platform/ConfigHandler.h"
#include "LogOutput.h"
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
	CBaseWater* water = NULL;
	const int configValue = configHandler.GetInt("ReflectiveWater",1);
	
	if(configValue==2 && GLEW_ARB_fragment_program && GLEW_ARB_texture_float &&
	   ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"waterDyn.fp")) {
		try {
			water = SAFE_NEW CDynWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Dynamic Water failed");
			logOutput.Print("Error: %s", e.what());
		}
		if (water) {
			return water;
		}
	}

	if(configValue==4 && GLEW_ARB_shading_language_100) {
		try {
			water = SAFE_NEW CBumpWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Bumpmapped Water failed");
			logOutput.Print("Error: %s", e.what());
		}
		if (water) {
			return water;
		}
	}
	
	if(configValue==3 && GLEW_ARB_fragment_program && GLEW_ARB_texture_rectangle){
		try {
			water = SAFE_NEW CRefractWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Refractive Water failed");
			logOutput.Print("Error: %s", e.what());
		}
		if (water) {
			return water;
		}
	}
	
	if(configValue!=0 && GLEW_ARB_fragment_program &&
	   ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp")){
		try {
			water = SAFE_NEW CAdvWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Reflective Water failed");
			logOutput.Print("Error: %s", e.what());
		}
		if (water) {
			return water;
		}
	}
	
	return SAFE_NEW CBasicWater;
}
