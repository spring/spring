/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "BaseSky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "SkyBox.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"

IBaseSky* sky = NULL;

IBaseSky::IBaseSky()
	: wireframe(false)
	, dynamicSky(false)
	, fogStart(0.0f)
	, skyLight(NULL)
	, cloudDensity(0.0f)
{
	SetLight(configHandler->Get("DynamicSun", false));
}

IBaseSky::~IBaseSky()
{
	delete skyLight;
}



void IBaseSky::SetFog() {
	if (globalRendering->drawFog) {
		glEnable(GL_FOG);
	} else {
		glDisable(GL_FOG);
	}

	glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	glFogi(GL_FOG_MODE,   GL_LINEAR);
	glFogf(GL_FOG_START,  globalRendering->viewRange * mapInfo->atmosphere.fogStart);
	glFogf(GL_FOG_END,    globalRendering->viewRange * mapInfo->atmosphere.fogEnd);
	glFogf(GL_FOG_DENSITY, 1.0f);
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
