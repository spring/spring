/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "SkyLight.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Config/ConfigHandler.h"

CONFIG(float, DynamicSunMinElevation).defaultValue(0.1f).description("Sets the minimum elevation of the dynamic sun. If less than 0.0, the sun can disappear under the map completely as it moves.");


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
	orbitMinSunHeight = configHandler->GetFloat("DynamicSunMinElevation"); //FIXME mapinfo option???
	orbitMinSunHeight = Clamp(orbitMinSunHeight, -1.0f, 1.0f);

	// light.sunDir has already been normalized (in 3D) in MapInfo
	SetLightParams(light.sunDir, light.sunStartAngle, light.sunOrbitTime);
}

void DynamicSkyLight::Update() {
	if (!luaControl) {
		SetLightDir(CalculateSunPos(sunStartAngle).ANormalize());

		lightIntensity = std::sqrt(Clamp(lightDir.y, 0.0f, 1.0f));

		const float shadowDensity = std::min(1.0f, lightIntensity * shadowDensityFactor);
		const CMapInfo::light_t& light = mapInfo->light;

		groundShadowDensity = shadowDensity * light.groundShadowDensity;
		unitShadowDensity   = shadowDensity * light.unitShadowDensity;
	}

	if (updateNeeded) {
		updateNeeded = false;

		sky->UpdateSunDir();
		eventHandler.SunChanged();
	}
}



bool DynamicSkyLight::SetLightDir(const float4& newLightDir) {
	if (newLightDir != lightDir) {
		static float4 lastUpdate = ZeroVector;
		static const float minCosAngle = std::cos(1.5f * (PI/180.f));

		if (lastUpdate.dot(newLightDir) < minCosAngle) {
			lastUpdate   = newLightDir;
			updateNeeded = true;
		}

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
	const float4 sunPos = sunRotation.Mul(float3(sunOrbitRad * std::cos(sunAng), sunOrbitHeight, sunOrbitRad * std::sin(sunAng)));

	return sunPos;
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
		const float sunAzimuth = (sunLen <= 0.001f) ? PI / 2.0f : std::atan(newLightDir.y / sunLen);
		const float sunHeight = std::tan(sunAzimuth - 0.001f);

		float3 v1(std::cos(initialSunAngle), sunHeight, std::sin(initialSunAngle));
		v1.ANormalize();

		if (v1.y <= orbitMinSunHeight) {
			newLightDir = UpVector;
			sunOrbitHeight = v1.y;
			sunOrbitRad = std::sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
		} else {
			float3 v2(std::cos(initialSunAngle + PI), orbitMinSunHeight, std::sin(initialSunAngle + PI));
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
		sunOrbitHeight = Clamp(newLightDir.w, -1.0f, 1.0f);
		sunOrbitRad = std::sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
	}

	sunRotation.LoadIdentity();
	sunRotation.SetUpVector(newLightDir);

	const float4& peakDir  = CalculateSunPos(0.0f);
	const float peakElev   = std::max(0.01f, peakDir.y);

	shadowDensityFactor = 1.0f / peakElev;

	SetLightDir(CalculateSunPos(sunStartAngle).ANormalize());
}
