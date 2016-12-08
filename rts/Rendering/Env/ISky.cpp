/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ISky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "SkyBox.h"
#include "Game/TraceRay.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

CONFIG(bool, AdvSky).defaultValue(true).headlessValue(false).defaultValue(false).description("Enables High Resolution Clouds.");

ISky* sky = nullptr;

ISky::ISky()
	: wireframe(false)
	, dynamicSky(false)
	, skyColor(mapInfo->atmosphere.skyColor)
	, sunColor(mapInfo->atmosphere.sunColor)
	, cloudColor(mapInfo->atmosphere.cloudColor)
	, fogColor(mapInfo->atmosphere.fogColor)
	, fogStart(mapInfo->atmosphere.fogStart)
	, fogEnd(mapInfo->atmosphere.fogEnd)
	, cloudDensity(mapInfo->atmosphere.cloudDensity)
	, skyLight(nullptr)
// 	, cloudDensity(0.0f)
{
	skyLight = new ISkyLight();
}

ISky::~ISky()
{
	SafeDelete(skyLight);
}



void ISky::SetupFog() {

	if (globalRendering->drawFog) {
		glEnable(GL_FOG);
	} else {
		glDisable(GL_FOG);
	}

	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE,   GL_LINEAR);
	glFogf(GL_FOG_START,  globalRendering->viewRange * fogStart);
	glFogf(GL_FOG_END,    globalRendering->viewRange * fogEnd);
	glFogf(GL_FOG_DENSITY, 1.0f);
}



ISky* ISky::GetSky()
{
	ISky* sky = nullptr;

	try {
		if (!mapInfo->atmosphere.skyBox.empty()) {
			sky = new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
		} else if (configHandler->GetBool("AdvSky")) {
			sky = new CAdvSky();
		}
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "[%s] error: %s (falling back to BasicSky)", __FUNCTION__, ex.what());

		SafeDelete(sky);
	}

	if (sky == nullptr)
		sky = new CBasicSky();

	return sky;
}

bool ISky::SunVisible(const float3 pos) const {
	const CUnit* hitUnit = nullptr;
	const CFeature* hitFeature = nullptr;

	// cast a ray *toward* the sun from <pos>
	// sun is visible if no terrain blocks it
	const float3& sunDir = skyLight->GetLightDir();
	const float sunDist = TraceRay::GuiTraceRay(pos, sunDir, globalRendering->viewRange, nullptr, hitUnit, hitFeature, false, true, false);

	return (sunDist < 0.0f || sunDist >= globalRendering->viewRange);
}

