/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BaseSky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "ConfigHandler.h"
#include "SkyBox.h"
#include "Map/MapInfo.h"
#include "Exceptions.h"
#include "LogOutput.h"

IBaseSky* sky = NULL;

IBaseSky::IBaseSky()
	: wireframe(false)
	, dynamicSky(false)
	, fogStart(0.0f)
	, skyLight(NULL)
	, cloudDensity(0.0f)
{
	SetLight(configHandler->Get("DynamicSun", true));
}

IBaseSky::~IBaseSky()
{
	delete skyLight;
}



IBaseSky* IBaseSky::GetSky()
{
	IBaseSky* sky = NULL;

	try {
		if (!mapInfo->atmosphere.skyBox.empty()) {
			sky = new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
		} else if (configHandler->Get("AdvSky", true)) {
			sky = new CAdvSky();
		}
	} catch (content_error& e) {
		logOutput.Print("[%s] error: %s (falling back to BasicSky)", __FUNCTION__, e.what());
		delete sky;
		sky = NULL;
	}

	if (sky == NULL) {
		sky = new CBasicSky();
	}

	return sky;
}

void IBaseSky::SetLight(bool dynamic) {
	delete skyLight;

	if (dynamic)
		skyLight = new DynamicSkyLight();
	else
		skyLight = new StaticSkyLight();
}
