/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BaseSky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "Rendering/GL/myGL.h"
#include "ConfigHandler.h"
#include "SkyBox.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

CBaseSky* sky=0;

CBaseSky::CBaseSky(void)
{
	wireframe = false;
	dynamicSky = false;
	cloudDensity = 0;
	fogStart = 0;
}

CBaseSky::~CBaseSky(void)
{
}

CBaseSky* CBaseSky::GetSky()
{
	if(!mapInfo->atmosphere.skyBox.empty())
		return new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
	else if(GLEW_ARB_fragment_program && configHandler->Get("AdvSky",1) && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB,"ARB/clouds.fp"))
		return new CAdvSky();
	else
		return new CBasicSky();
}
