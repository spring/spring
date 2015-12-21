/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_SKY_H
#define I_SKY_H

#include "SkyLight.h"
#include "Map/ReadMap.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

class ISky
{
public:
	static ISky* GetSky();

	virtual ~ISky();
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void DrawSun() = 0;

	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

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
	static void SetupFog();

	virtual void SetLuaTexture(const MapTextureData& td) { }
public:
	bool wireframe;
	bool dynamicSky;

	float3 sundir1, sundir2; // (xvec, yvec) TODO: move these to SkyLight
	float3 modSunDir;
	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;
	float fogStart;

protected:
	ISky();

	ISkyLight* skyLight;

	float cloudDensity;
};

extern ISky* sky;

#endif // I_SKY_H
