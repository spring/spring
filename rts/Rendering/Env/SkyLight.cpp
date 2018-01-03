/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkyLight.h"
#include "Map/MapInfo.h"

ISkyLight::ISkyLight() {
	SetLightDir(mapInfo->light.sunDir);
}

