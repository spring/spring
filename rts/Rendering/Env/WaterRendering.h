/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WATER_RENDERING_H_
#define _WATER_RENDERING_H_

#include "System/float3.h"
#include "System/float4.h"

#include <string>
#include <vector>

struct CWaterRendering {
public:
	void Init();
	bool IsGlobalInstance() const;
public:
	float  repeatX;           ///< (calculated default is in IWater)
	float  repeatY;           ///< (calculated default is in IWater)
	float3 absorb;
	float3 baseColor;
	float3 minColor;
	float3 surfaceColor;
	float  surfaceAlpha;
	float4 planeColor;
	float3 diffuseColor;
	float3 specularColor;
	float  ambientFactor;
	float  diffuseFactor;
	float  specularFactor;
	float  specularPower;
	float  fresnelMin;
	float  fresnelMax;
	float  fresnelPower;
	float  reflDistortion;
	float  blurBase;
	float  blurExponent;
	float  perlinStartFreq;
	float  perlinLacunarity;
	float  perlinAmplitude;
	float  windSpeed;
	bool   shoreWaves;
	bool   forceRendering;    ///< if false the renderers will render it only if currentMinMapHeight<0
	bool   hasWaterPlane;     ///< true if "MAP\WATER\WaterPlaneColor" is set
	unsigned char numTiles;
	std::string texture;
	std::string foamTexture;
	std::string normalTexture;
	std::vector<std::string> causticTextures;
};

extern CWaterRendering waterRenderingInst;

#define waterRendering (&waterRenderingInst)
#endif
