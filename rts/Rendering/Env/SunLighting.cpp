/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "SunLighting.h"

#include "Map/MapInfo.h"
#include "System/EventHandler.h"
#include "System/Sync/HsiehHash.h"

/**
 * @brief sunLightingInst
 *
 * Global instance of CSunLighting
 */
CSunLighting sunLightingInst;

CSunLighting::CSunLighting() {
	colors[0] = &groundAmbientColor;
	colors[1] = &groundDiffuseColor;
	colors[2] = &groundSpecularColor;

	colors[3] = &unitAmbientColor;
	colors[4] = &unitDiffuseColor;
	colors[5] = &unitSpecularColor;
}

// need an explicit copy-ctor because of colors[]
CSunLighting::CSunLighting(const CSunLighting& sl) {
	colors[0] = &groundAmbientColor;
	colors[1] = &groundDiffuseColor;
	colors[2] = &groundSpecularColor;

	colors[3] = &unitAmbientColor;
	colors[4] = &unitDiffuseColor;
	colors[5] = &unitSpecularColor;

	Copy(sl);
}

CSunLighting& CSunLighting::operator = (const CSunLighting& sl) {
	Copy(sl);
	return (*this);
}


void CSunLighting::Init() {
	assert(mapInfo != nullptr);
	assert(IsGlobalInstance());

	const CMapInfo::light_t& light = mapInfo->light;

	groundAmbientColor   = light.groundAmbientColor;
	groundDiffuseColor   = light.groundDiffuseColor;
	groundSpecularColor  = light.groundSpecularColor;

	unitAmbientColor     = light.unitAmbientColor;
	unitDiffuseColor     = light.unitDiffuseColor;
	unitSpecularColor    = light.unitSpecularColor;

	specularExponent     = light.specularExponent;
}

void CSunLighting::Copy(const CSunLighting& sl) {
	assert(   colors[0] == &   groundAmbientColor);
	assert(sl.colors[0] == &sl.groundAmbientColor);

	if (sl == (*this))
		return;

	for (unsigned int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++)
		*colors[n] = *sl.colors[n];

	specularExponent = sl.specularExponent;

	if (!IsGlobalInstance())
		return;

	// send event only if at least one value was changed for the global instance
	eventHandler.SunChanged();
}


bool CSunLighting::SetValue(unsigned int keyHash, const float4 value) {
	static const unsigned int keyHashes[] = {
		HsiehHash("groundAmbientColor",  sizeof("groundAmbientColor" ) - 1, 0),
		HsiehHash("groundDiffuseColor",  sizeof("groundDiffuseColor" ) - 1, 0),
		HsiehHash("groundSpecularColor", sizeof("groundSpecularColor") - 1, 0),

		HsiehHash("unitAmbientColor",  sizeof("unitAmbientColor" ) - 1, 0),
		HsiehHash("unitDiffuseColor",  sizeof("unitDiffuseColor" ) - 1, 0),
		HsiehHash("unitSpecularColor", sizeof("unitSpecularColor") - 1, 0),

		HsiehHash("specularExponent", sizeof("specularExponent") - 1, 0),
	};

	// special case
	if (keyHash == keyHashes[6]) {
		specularExponent = value.x;
		return true;
	}

	for (unsigned int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++) {
		if (keyHash == keyHashes[n]) {
			*(colors[n]) = value;
			return true;
		}
	}

	return false;
}


bool CSunLighting::operator == (const CSunLighting& sl) const {
	for (unsigned int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++) {
		if (colors[n] != sl.colors[n]) {
			return false;
		}
	}

	return (specularExponent == sl.specularExponent);
}

bool CSunLighting::IsGlobalInstance() const {
	return (this == &sunLightingInst);
}

