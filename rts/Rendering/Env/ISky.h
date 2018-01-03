/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_SKY_H
#define I_SKY_H

#include "SkyLight.h"
#include "Game/GameDrawMode.h"

struct MapTextureData;
class ISky
{
protected:
	ISky();

public:
	virtual ~ISky() {}

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

	virtual void Draw(Game::DrawMode mode) = 0;
	virtual void DrawSun(Game::DrawMode mode) = 0;

	virtual void SetLuaTexture(const MapTextureData& td) {}

	const ISkyLight* GetLight() const { return &skyLight; }
	      ISkyLight* GetLight()       { return &skyLight; }

	bool SunVisible(const float3 pos) const;

	bool& WireFrameModeRef() { return wireFrameMode; }

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
	ISkyLight skyLight;

	bool wireFrameMode;
};

extern ISky* sky;

#endif // I_SKY_H
