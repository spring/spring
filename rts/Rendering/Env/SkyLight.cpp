/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkyLight.h"
#include "Map/MapInfo.h"
#include "System/FastMath.h"

ISkyLight::ISkyLight() {
	SetLightDir(mapInfo->light.sunDir);
}

float3& ISkyLight::CalcPolarLightDir() {
	lightDirZ = lightDir;
	lightDirZ.y = 0.0f;

	if (lightDirZ.SqLength() == 0.0f)
		lightDirZ.x = 1.0f;

	lightDirZ.ANormalize();
	lightDirX = lightDirZ.cross(UpVector);

	// polar-coordinate direction in yz-plane (sin(zenith-angle), radius)
	modLightDir.x = 0.0f;
	modLightDir.y = lightDir.y;
	modLightDir.z = fastmath::sqrt_sse(lightDir.dot2D(lightDir));
	return modLightDir;
}

