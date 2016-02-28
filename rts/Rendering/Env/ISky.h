/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_SKY_H
#define I_SKY_H

#include "SkyLight.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

struct MapTextureData;
class ISky
{
public:
	static ISky* GetSky();

	virtual ~ISky();

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

	virtual void Draw() = 0;
	virtual void DrawSun() = 0;

	virtual void SetLuaTexture(const MapTextureData& td) {}

	void IncreaseCloudDensity() { cloudDensity *= 1.05f; }
	void DecreaseCloudDensity() { cloudDensity *= 0.95f; }
	float GetCloudDensity() const { return cloudDensity; }

	ISkyLight* GetLight() const { return skyLight; }
	void SetLight(bool dynamic);

	bool SunVisible(const float3 pos) const;

	/**
	 * Sets up OpenGL to draw fog or not, according to the value of
	 * globalRendering->drawFog.
	 */
	void SetupFog();

public:
	bool wireframe;
	bool dynamicSky;

	float3 sundir1, sundir2; // (xvec, yvec) TODO: move these to SkyLight
	float3 modSunDir;

	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;
	float4 fogColor;

	float fogStart;
	float fogEnd;
	float cloudDensity;

protected:
	ISky();

	ISkyLight* skyLight;
};

extern ISky* sky;

#endif // I_SKY_H
