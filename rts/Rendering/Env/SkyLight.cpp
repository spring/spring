/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "SkyLight.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"

StaticSkyLight::StaticSkyLight() {
	const CMapInfo::light_t& light = mapInfo->light;

	lightIntensity = 1.0f;
	groundShadowDensity = light.groundShadowDensity;
	unitShadowDensity = light.unitShadowDensity;

	lightDir = light.sunDir;
	lightDir.w = 0.0f;
}



DynamicSkyLight::DynamicSkyLight(): luaControl(false) {
	const CMapInfo::light_t& light = mapInfo->light;

	lightIntensity = 1.0f;
	groundShadowDensity = light.groundShadowDensity;
	unitShadowDensity = light.unitShadowDensity;

	// light.sunDir has already been normalized (in 3D) in MapInfo
	SetLightParams(light.sunDir, light.sunStartAngle, light.sunOrbitTime);
}

void DynamicSkyLight::Update() {
	if (luaControl || !SetLightDir(CalculateDir(sunStartAngle))) {
		// sun position cannot change between simframes
		return;
	}

	lightIntensity = sqrt(std::max(0.0f, std::min(lightDir.y, 1.0f)));

	const float shadowDensity = std::min(1.0f, lightIntensity * shadowDensityFactor);
	const CMapInfo::light_t& light = mapInfo->light;

	groundShadowDensity = shadowDensity * light.groundShadowDensity;
	unitShadowDensity = shadowDensity * light.unitShadowDensity;

	// FIXME: ugly dependencies + misnomers
	sky->UpdateSunDir();
	unitDrawer->UpdateSunDir();
	readmap->GetGroundDrawer()->UpdateSunDir();
	groundDecals->UpdateSunDir();
}



bool DynamicSkyLight::SetLightDir(const float4& newLightDir) {
	float4 newLightDirNorm = newLightDir;
	newLightDirNorm.ANormalize();

	if (newLightDirNorm != lightDir) {
		lightDir = newLightDirNorm;
		lightDir.w = 0.0f;
		return true;
	}

	return false;
}

float4 DynamicSkyLight::CalculateDir(float startAngle) const {
	const float sunAng = PI - startAngle - initialSunAngle - gs->frameNum * 2.0f * PI / (GAME_SPEED * sunOrbitTime);
	const float4 sunPos = sunRotation.Mul(float3(sunOrbitRad * cos(sunAng), sunOrbitHeight, sunOrbitRad * sin(sunAng)));

	return sunPos;
}

void DynamicSkyLight::SetLightParams(float4 newLightDir, float startAngle, float orbitTime) {
	newLightDir.ANormalize();

	sunStartAngle = startAngle;
	sunOrbitTime = orbitTime;
	initialSunAngle = fastmath::coords2angle(newLightDir.x, newLightDir.z);

	if (newLightDir.w == FLT_MAX) {
		// old: newLightDir is position where sun reaches highest altitude
		const float sunLen = newLightDir.Length2D();
		const float sunAzimuth = (sunLen <= 0.001f) ? PI / 2.0f : atan(newLightDir.y / sunLen);
		const float sunHeight = tan(sunAzimuth - 0.001f);

		// the lowest sun altitude for an auto-generated orbit
		const float orbitMinSunHeight = 0.1f;

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

	const float4& startDir = CalculateDir(sunStartAngle);
	const float4& peakDir = CalculateDir(0.0f);
	const float peakElev = std::max(0.01f, peakDir.y);

	shadowDensityFactor = 1.0f / peakElev;

	SetLightDir(startDir);
}
