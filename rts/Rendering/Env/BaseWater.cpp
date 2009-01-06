#include "StdAfx.h"
#include "mmgr.h"

#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "Rendering/GL/myGL.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "DynWater.h"
#include "RefractWater.h"
#include "Exceptions.h"

CBaseWater* water=0;

CBaseWater::CBaseWater(void)
{
	drawReflection=false;
	drawRefraction=false;
 	noWakeProjectiles=false;
 	drawSolid=false;
	oldwater=NULL;
}

CBaseWater::~CBaseWater(void)
{
	DeleteOldWater(this);
}

void CBaseWater::DeleteOldWater(CBaseWater *water) {
	if(water->oldwater) {
		DeleteOldWater(water->oldwater);
		delete water->oldwater;
		water->oldwater=NULL;
	}
}

CBaseWater* CBaseWater::GetWater(CBaseWater* old)
{
	CBaseWater* water = NULL;
	const int configValue = configHandler.Get("ReflectiveWater",1);
	
	if(water==NULL && configValue==2 && GLEW_ARB_fragment_program && GLEW_ARB_texture_float &&
	   ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"waterDyn.fp")) {
		try {
			water = new CDynWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Dynamic Water failed");
			logOutput.Print("Error: %s", e.what());
		}
	}

	if(water==NULL && configValue==4 && GLEW_ARB_shading_language_100 && GL_ARB_fragment_shader && GL_ARB_vertex_shader) {
		try {
			water = new CBumpWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Bumpmapped Water failed");
			logOutput.Print("Error: %s", e.what());
		}
	}
	
	if(water==NULL && configValue==3 && GLEW_ARB_fragment_program && GLEW_ARB_texture_rectangle){
		try {
			water = new CRefractWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Refractive Water failed");
			logOutput.Print("Error: %s", e.what());
		}
	}
	
	if(water==NULL && configValue!=0 && GLEW_ARB_fragment_program &&
	   ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"water.fp")){
		try {
			water = new CAdvWater;
		} catch (content_error& e) {
			delete water;
			water = NULL;
			logOutput.Print("Loading Reflective Water failed");
			logOutput.Print("Error: %s", e.what());
		}
	}
	if(water==NULL)
		water = new CBasicWater;
	water->oldwater=old;
	return water;
}
