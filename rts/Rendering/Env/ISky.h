/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_SKY_H
#define I_SKY_H

#include "SkyLight.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

struct MapTextureData;
class ISky
{
protected:
	ISky();

public:
	virtual ~ISky();

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

	virtual void Draw() = 0;
	virtual void DrawSun() = 0;

	virtual void SetLuaTexture(const MapTextureData& td) {}

	ISkyLight* GetLight() const { return skyLight; }

	bool SunVisible(const float3 pos) const;

	bool& WireFrameModeRef() { return wireFrameMode; }

	/**
	 * Sets up OpenGL to draw fog or not, according to the value of
	 * globalRendering->drawFog.
	 */
	void SetupFog();

public:
	static ISky* GetSky();

public:
	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;
	float4 fogColor;

	float fogStart;
	float fogEnd;

protected:
	ISkyLight* skyLight;

	bool wireFrameMode;
};

extern ISky* sky;

#endif // I_SKY_H
