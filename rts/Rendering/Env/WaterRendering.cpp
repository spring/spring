/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "WaterRendering.h"

#include "Map/MapInfo.h"
#include "System/EventHandler.h"

/**
 * @brief waterRenderingInst
 *
 * Global instance of CWaterRendering
 */
CWaterRendering waterRenderingInst;

void CWaterRendering::Init() {
	assert(mapInfo != nullptr);
	assert(IsGlobalInstance());

	const CMapInfo::water_t& water = mapInfo->water;

	repeatX = water.repeatX;
	repeatY = water.repeatY;
	absorb = water.absorb;
	baseColor = water.baseColor;
	minColor = water.minColor;
	surfaceColor = water.surfaceColor;
	surfaceAlpha = water.surfaceAlpha;
	planeColor = water.planeColor;
	diffuseColor = water.diffuseColor;
	specularColor = water.specularColor;
	ambientFactor = water.ambientFactor;
	diffuseFactor = water.diffuseFactor;
	specularFactor = water.specularFactor;
	specularPower = water.specularPower;
	fresnelMin = water.fresnelMin;
	fresnelMax = water.fresnelMax;
	fresnelPower = water.fresnelPower;
	reflDistortion = water.reflDistortion;
	blurBase = water.blurBase;
	blurExponent = water.blurExponent;
	perlinStartFreq = water.perlinStartFreq;
	perlinLacunarity = water.perlinLacunarity;
	perlinAmplitude = water.perlinAmplitude;
	windSpeed = water.windSpeed;
	shoreWaves = water.shoreWaves;
	forceRendering = water.forceRendering;
	hasWaterPlane = water.hasWaterPlane;
	numTiles = water.numTiles;
	texture = water.texture;
	foamTexture = water.foamTexture;
	normalTexture = water.normalTexture;
	causticTextures = water.causticTextures;
}

bool CWaterRendering::IsGlobalInstance() const {
	return (this == &waterRenderingInst);
}
