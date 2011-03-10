/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASE_SKY_H__
#define __BASE_SKY_H__

#include "SkyLight.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

class CBaseSky
{
public:
	static CBaseSky* GetSky();

	virtual ~CBaseSky();
	virtual void Update() = 0;
	virtual void UpdateSunFlare() = 0;
	virtual void Draw() = 0;
	virtual void DrawSun() = 0;

	void UpdateSunDir();
	void UpdateSkyDir();
	void UpdateSkyTexture();

	void IncreaseCloudDensity() { cloudDensity *= 1.05f; }
	void DecreaseCloudDensity() { cloudDensity *= 0.95f; }
	float GetCloudDensity() const { return cloudDensity; }

	ISkyLight* GetLight() { return skyLight; }
	void SetLight(bool);

	bool dynamicSky;
	bool wireframe;

	float3 skydir1, skydir2; // (xvec, yvec)
	float3 sundir1, sundir2; // (xvec, yvec) TODO: move these to SkyLight
	float3 modSunDir;
	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;

protected:
	CBaseSky();

	void UpdateTexPartDot3(int x, int y, unsigned char (*texp)[4]);
	void UpdateTexPart(int x, int y, unsigned char (*texp)[4]);
	float3 GetDirFromTexCoord(float x, float y);
	float GetTexCoordFromDir(const float3& dir);

	ISkyLight* skyLight;

	unsigned int skyTex;
	unsigned int skyDot3Tex;
	unsigned int cloudDot3Tex;
	unsigned int sunTex;
	unsigned int sunFlareTex;

	unsigned char (* skytexpart)[4];
	unsigned int skyTexUpdateIter;

	unsigned int skyDomeList;
	unsigned int sunFlareList;

	float fogStart;
	float skyAngle;
	float cloudDensity;

	float domeheight;
	float domeWidth;

	float sunTexCoordX;
	float sunTexCoordY;

	int*** randMatrix;
	int** rawClouds;
	int*** blendMatrix;

	unsigned char* cloudThickness;

	float covers[4][32];
	int oldCoverBaseX;
	int oldCoverBaseY;

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];
	bool cloudDown[10];

	int ydif[CLOUD_SIZE];
	int updatecounter;
};

extern CBaseSky* sky;

#endif // __BASE_SKY_H__
