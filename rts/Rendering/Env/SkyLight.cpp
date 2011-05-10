/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "SkyLight.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/BaseSky.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/ConfigHandler.h"
#include "System/myMath.h"


StaticSkyLight::StaticSkyLight() {
	const CMapInfo::light_t& light = mapInfo->light;

	lightIntensity = 1.0f;
	groundShadowDensity = light.groundShadowDensity;
	unitShadowDensity = light.unitShadowDensity;

	lightDir = light.sunDir;
	lightDir.w = 0.0f;
}



DynamicSkyLight::DynamicSkyLight()
	: luaControl(false)
	, updateNeeded(true)
{
	const CMapInfo::light_t& light = mapInfo->light;

	lightIntensity = 1.0f;
	groundShadowDensity = light.groundShadowDensity;
	unitShadowDensity = light.unitShadowDensity;
	orbitMinSunHeight = configHandler->Get("DynamicSunMinElevation", 0.1f); //FIXME mapinfo option???
	orbitMinSunHeight = Clamp(orbitMinSunHeight, -1.0f, 1.0f);

	// light.sunDir has already been normalized (in 3D) in MapInfo
	SetLightParams(light.sunDir, light.sunStartAngle, light.sunOrbitTime);
}

void DynamicSkyLight::Update() {
	if (!luaControl) {
		UpdateSunDir();

		lightIntensity = math::sqrt(Clamp(lightDir.y, 0.0f, 1.0f));

		const float shadowDensity = std::min(1.0f, lightIntensity * shadowDensityFactor); //FIXME why min(1.)? shouldn't the shadow fade away in the night?
		const CMapInfo::light_t& light = mapInfo->light;

		groundShadowDensity = shadowDensity * light.groundShadowDensity;
		unitShadowDensity   = shadowDensity * light.unitShadowDensity;
	}

	if (updateNeeded) {
		sky->UpdateSunDir();
		unitDrawer->UpdateSunDir();
		readmap->GetGroundDrawer()->UpdateSunDir();
		groundDecals->UpdateSunDir();
	}
}



bool DynamicSkyLight::SetLightDir(const float4& newLightDir) {
	if (newLightDir != lightDir) {
		updateNeeded = (lightDir.dot(newLightDir) < 0.95f);

		lightDir = newLightDir;
		lightDir.w = 0.0f;
		return true;
	}

	return false;
}

float4 DynamicSkyLight::CalculateSunPos(const float startAngle) const {
	const float gameSeconds = gs->frameNum / (float)GAME_SPEED;
	const float angularVelocity = 2.0f * PI / sunOrbitTime;

	const float sunAng  = startAngle - initialSunAngle - angularVelocity * gameSeconds;
	const float4 sunPos = sunRotation.Mul(float3(sunOrbitRad * cos(sunAng), sunOrbitHeight, sunOrbitRad * sin(sunAng)));

	return sunPos;
}

bool DynamicSkyLight::UpdateSunDir() {
	const float4 newDir = CalculateSunPos(sunStartAngle).ANormalize();
	return SetLightDir(newDir);
}


void DynamicSkyLight::SetLightParams(float4 newLightDir, float startAngle, float orbitTime) {
	newLightDir.ANormalize();

	sunStartAngle = PI + startAngle; //FIXME WHY +PI?
	sunOrbitTime = orbitTime;
	initialSunAngle = GetRadFromXY(newLightDir.x, newLightDir.z);

	//FIXME This function really really needs comments about what it does!
	if (newLightDir.w == FLT_MAX) {
		// old: newLightDir is position where sun reaches highest altitude
		const float sunLen = newLightDir.Length2D();
		const float sunAzimuth = (sunLen <= 0.001f) ? PI / 2.0f : atan(newLightDir.y / sunLen);
		const float sunHeight = tan(sunAzimuth - 0.001f);

		float3 v1(cos(initialSunAngle), sunHeight, sin(initialSunAngle));
		v1.ANormalize();

		if (v1.y <= orbitMinSunHeight) {
			newLightDir = UpVector;
			sunOrbitHeight = v1.y;
			sunOrbitRad = sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
		} else {
			float3 v2(cos(initialSunAngle + PI), orbitMinSunHeight, sin(initialSunAngle + PI));
			v2.ANormalize();
			float3 v3 = v2 - v1;
			sunOrbitRad = v3.Length() / 2.0f;
			v3.ANormalize();

			float3 v4 = (v3.cross(UpVector)).ANormalize();
			float3 v5 = (v3.cross(v4)).ANormalize();

			if (v5.y < 0.0f)
				v5 = -v5;

			newLightDir = v5;
			sunOrbitHeight = v5.dot(v1);
		}
	} else {
		// new: newLightDir is center position of orbit, and newLightDir.w is orbit height
		sunOrbitHeight = std::max(-1.0f, std::min(newLightDir.w, 1.0f));
		sunOrbitRad = sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
	}

	sunRotation.LoadIdentity();
	sunRotation.SetUpVector(newLightDir);

	const float4& peakDir  = CalculateSunPos(0.0f);
	const float peakElev   = std::max(0.01f, peakDir.y);

	shadowDensityFactor = 1.0f / peakElev;

	UpdateSunDir();
}
