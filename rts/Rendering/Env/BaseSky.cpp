/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BaseSky.h"
#include "BasicSky.h"
#include "AdvSky.h"
#include "Rendering/GL/myGL.h"
#include "ConfigHandler.h"
#include "SkyBox.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Exceptions.h"
#include "LogOutput.h"

CBaseSky* sky = NULL;

CBaseSky::CBaseSky()
	: skyLight(NULL)
	, dynamicSky(false)
	, cloudDensity(0.0f)
	, wireframe(false)
	, fogStart(0.0f)
{
	SetLight(configHandler->Get("DynamicSun", 1) != 0);
}

CBaseSky::~CBaseSky()
{
	delete skyLight;
}



CBaseSky* CBaseSky::GetSky()
{
	CBaseSky* sky = NULL;

	try {
		if (!mapInfo->atmosphere.skyBox.empty()) {
			sky = new CSkyBox("maps/" + mapInfo->atmosphere.skyBox);
		} else if (configHandler->Get("AdvSky", 1) != 0) {
			sky = new CAdvSky();
		}
	} catch (content_error& e) {
		logOutput.Print("[%s] error: %s (falling back to BasicSky)", __FUNCTION__, e.what());
		// sky can not be != NULL here
		// delete sky;
	}

	if (sky == NULL) {
		sky = new CBasicSky();
	}

	return sky;
}

void CBaseSky::SetLight(bool dynamic) {
	delete skyLight;

	if (dynamic)
		skyLight = new DynamicSkyLight();
	else
		skyLight = new StaticSkyLight();
}



void CBaseSky::UpdateSunDir() {
	const float3& L = skyLight->GetLightDir();

	sundir2 = L;
	sundir2.y = 0.0f;

	if (sundir2.SqLength() == 0.0f)
		sundir2.x = 1.0f;

	sundir2.ANormalize();
	sundir1 = sundir2.cross(UpVector);

	modSunDir.y = L.y;
	modSunDir.x = 0.0f;
	modSunDir.z = math::sqrt(L.x * L.x + L.z * L.z);

	sunTexCoordX = 0.5f;
	sunTexCoordY = GetTexCoordFromDir(modSunDir);

	UpdateSunFlare();
}

void CBaseSky::UpdateSkyDir() {
	skydir2 = mapInfo->atmosphere.skyDir;
	skydir2.y = 0.0f;

	if (skydir2.SqLength() == 0.0f)
		skydir2.x = 1.0f;

	skydir2.ANormalize();
	skydir1 = skydir2.cross(UpVector);
	skyAngle = fastmath::coords2angle(skydir2.x, skydir2.z) + PI / 2.0f;
}

void CBaseSky::UpdateSkyTexture() {
	const int mod = skyTexUpdateIter % 3;

	if (mod <= 1) {
		const int y = (skyTexUpdateIter / 3) * 2 + mod;
		for (int x = 0; x < 512; x++) {
			UpdateTexPart(x, y, skytexpart);
		}
		glBindTexture(GL_TEXTURE_2D, skyTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, skytexpart[0]);
	} else {
		const int y = (skyTexUpdateIter / 3);
		for (int x = 0; x < 256; x++) {
			UpdateTexPartDot3(x, y, skytexpart);
		}
		glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, skytexpart[0]);
	}

	skyTexUpdateIter = (skyTexUpdateIter + 1) % (512 + 256);
}



float3 CBaseSky::GetDirFromTexCoord(float x, float y)
{
	float3 dir;

	dir.x = (x - 0.5f) * domeWidth;
	dir.z = (y - 0.5f) * domeWidth;

	const float hdist = math::sqrt(dir.x * dir.x + dir.z * dir.z);
	const float ang = fastmath::coords2angle(dir.x, dir.z) + skyAngle;
	const float fy = asin(hdist / 400);

	dir.x = hdist * cos(ang);
	dir.z = hdist * sin(ang);
	dir.y = (fastmath::cos(fy) - domeheight) * 400;

	dir.ANormalize();
	return dir;
}

// should be improved
// only take stuff in yz plane
float CBaseSky::GetTexCoordFromDir(const float3& dir)
{
	float tp = 0.5f;
	float step = 0.25f;

	for (int a = 0; a < 10; ++a) {
		float tx = 0.5f + tp;
		const float3& d = GetDirFromTexCoord(tx, 0.5f);

		if (d.y < dir.y)
			tp -= step;
		else
			tp += step;

		step *= 0.5f;
	}

	return (0.5f + tp);
}

void CBaseSky::UpdateTexPartDot3(int x, int y, unsigned char (*texp)[4]) {
	const float3& dir = GetDirFromTexCoord(x / 256.0f, (255.0f - y) / 256.0f);

	const float sunInt = skyLight->GetLightIntensity();
	const float sunDist = acos(dir.dot(skyLight->GetLightDir())) * 50;
	const float sunMod = sunInt * (0.3f / math::sqrt(sunDist) + 3.0f / (1 + sunDist));

	const float green = std::min(1.0f, (0.55f + sunMod));
	const float blue  = 203 - sunInt * (40.0f / (3 + sunDist));

	texp[x][0] = (unsigned char) (sunInt * (255 - std::min(255.0f, sunDist))); // sun on borders
	texp[x][1] = (unsigned char) (green * 255); // sun light through
	texp[x][2] = (unsigned char)  blue; // ambient
	texp[x][3] = 255;
}

void CBaseSky::UpdateTexPart(int x, int y, unsigned char (*texp)[4]) {
	const float3& dir = GetDirFromTexCoord(x / 512.0f, (511.0f - y) / 512.0f);

	const float sunDist = acos(dir.dot(skyLight->GetLightDir())) * 70;
	const float sunMod = skyLight->GetLightIntensity() * 12.0f / (12 + sunDist);

	const float red   = std::min(skyColor.x + sunMod * sunColor.x, 1.0f);
	const float green = std::min(skyColor.y + sunMod * sunColor.y, 1.0f);
	const float blue  = std::min(skyColor.z + sunMod * sunColor.z, 1.0f);

	texp[x][0] = (unsigned char)(red   * 255);
	texp[x][1] = (unsigned char)(green * 255);
	texp[x][2] = (unsigned char)(blue  * 255);
	texp[x][3] = 255;
}
