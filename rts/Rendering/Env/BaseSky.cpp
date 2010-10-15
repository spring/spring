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
#include "Exceptions.h"
#include "LogOutput.h"

CBaseSky* sky = NULL;

CBaseSky::CBaseSky()
	: dynamicSky(false)
	, cloudDensity(0)
	, wireframe(false)
	, fogStart(0)
{
}

CBaseSky::~CBaseSky()
{
}

CBaseSky* CBaseSky::GetSky()
{
	CBaseSky* sky = NULL;

	try {
		if(!mapInfo->atmosphere.skyBox.empty()) {
			sky = new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
		} else if(configHandler->Get("AdvSky", 1)) {
			sky = new CAdvSky();
		}
	} catch (content_error& e) {
		if (e.what()[0] != '\0') {
			logOutput.Print("Error: %s", e.what());
		}
		logOutput.Print("Sky: Fallback to BasicSky.");
		// sky can not be != NULL here
		//delete sky;
	}

	if (!sky) {
		sky = new CBasicSky();
	}

	return sky;
}
