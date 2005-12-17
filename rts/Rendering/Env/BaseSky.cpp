#include "StdAfx.h"
#include "BaseSky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "Rendering/GL/myGL.h"
#include "Platform/ConfigHandler.h"
#include "SkyBox.h"
#include "Sim/Map/ReadMap.h"
//#include "mmgr.h"

CBaseSky* sky=0;

CBaseSky::CBaseSky(void)
{
}

CBaseSky::~CBaseSky(void)
{
}

CBaseSky* CBaseSky::GetSky()
{

	if(!readmap->skyBox.empty())
		return new CSkyBox("maps/" + readmap->skyBox);
	else if(GLEW_ARB_fragment_program && configHandler.GetInt("AdvSky",1) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"clouds.fp"))
		return new CAdvSky();
	else
		return new CBasicSky();
}
