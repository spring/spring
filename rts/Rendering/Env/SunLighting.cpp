/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "SunLighting.h"

#include "Map/MapInfo.h"
#include "System/EventHandler.h"

/**
 * @brief sunLightingInst
 *
 * Global instance of CSunLighting
 */
CSunLighting sunLightingInst;

void CSunLighting::Init() {
	assert(mapInfo != nullptr);
	assert(this == &sunLightingInst);

	const CMapInfo::light_t& light = mapInfo->light;

	groundAmbientColor   = light.groundAmbientColor;
	groundSunColor       = light.groundSunColor;
	groundSpecularColor  = light.groundSpecularColor;
	unitAmbientColor     = light.unitAmbientColor;
	unitSunColor         = light.unitSunColor;
	unitSpecularColor    = light.unitSpecularColor;
	specularExponent     = light.specularExponent;
}

void CSunLighting::Set(const CSunLighting& sl) {
	groundAmbientColor   = sl.groundAmbientColor;
	groundSunColor       = sl.groundSunColor;
	groundSpecularColor  = sl.groundSpecularColor;
	unitAmbientColor     = sl.unitAmbientColor;
	unitSunColor         = sl.unitSunColor;
	unitSpecularColor    = sl.unitSpecularColor;
	specularExponent     = sl.specularExponent;

	eventHandler.SunLightingChanged();
}