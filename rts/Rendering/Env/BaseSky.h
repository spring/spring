/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASE_SKY_H__
#define __BASE_SKY_H__

#include "SkyLight.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

class IBaseSky
{
public:
	static IBaseSky* GetSky();

	virtual ~IBaseSky();
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void DrawSun() = 0;

	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

	void IncreaseCloudDensity() { cloudDensity *= 1.05f; }
	void DecreaseCloudDensity() { cloudDensity *= 0.95f; }
	float GetCloudDensity() const { return cloudDensity; }

	ISkyLight* GetLight() const { return skyLight; }
	void SetLight(bool);

	static void SetFog();

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
	IBaseSky();

	ISkyLight* skyLight;

	float cloudDensity;
};

extern IBaseSky* sky;

#endif // __BASE_SKY_H__
