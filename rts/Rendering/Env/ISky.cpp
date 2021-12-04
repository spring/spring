/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ISky.h"
#include "NullSky.h"
#include "LuaSky.h"
#include "SkyBox.h"
#include "Game/Camera.h"
#include "Game/TraceRay.h"
#include "Map/MapInfo.h"
#include "Rendering/GL/myGL.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

ISky* sky = nullptr;

ISky::ISky()
	: skyColor(mapInfo->atmosphere.skyColor)
	, sunColor(mapInfo->atmosphere.sunColor)
	, cloudColor(mapInfo->atmosphere.cloudColor)
	, fogColor(mapInfo->atmosphere.fogColor)
	, fogStart(mapInfo->atmosphere.fogStart)
	, fogEnd(mapInfo->atmosphere.fogEnd)
	, wireFrameMode(false)
{
}


ISky* ISky::GetSky()
{
	ISky* sky = nullptr;

	try {
		if (!mapInfo->atmosphere.skyBox.empty()) {
			sky = new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
		} else {
			sky = new CLuaSky();
		}
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "[%s] error: %s (falling back to NullSky)", __func__, ex.what());

		spring::SafeDelete(sky);
	}

	if (sky == nullptr)
		sky = new CNullSky();

	return sky;
}

bool ISky::SunVisible(const float3 pos) const {
	const CUnit* hitUnit = nullptr;
	const CFeature* hitFeature = nullptr;

	// cast a ray *toward* the sun from <pos>
	// sun is visible if no terrain blocks it
	const float3& sunDir = skyLight.GetLightDir();
	const float sunDist = TraceRay::GuiTraceRay(pos, sunDir, camera->GetFarPlaneDist(), nullptr, hitUnit, hitFeature, false, true, false);

	return (sunDist < 0.0f || sunDist >= camera->GetFarPlaneDist());
}

