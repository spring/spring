/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "GlobalRendering.h"

#include "Rendering/GL/myGL.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/mmgr.h"
#include "System/Util.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"
#include "System/creg/creg_cond.h"
#include "Rendering/UnitDrawer.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/Env/BaseSky.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundDecalHandler.h"

#include <string>


/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
CGlobalRendering* globalRendering;


CR_BIND(CGlobalRendering, );

CR_REG_METADATA(CGlobalRendering, (
				CR_MEMBER(teamNanospray), // ??
				CR_MEMBER(lastFrameTime),
				CR_MEMBER(lastFrameStart),
				CR_MEMBER(weightedSpeedFactor), // ??
				CR_MEMBER(drawFrame), // ??
				CR_MEMBER(drawdebug), // ??
				CR_MEMBER(active),
				CR_MEMBER(viewRange),
				CR_MEMBER(timeOffset),
//				CR_MEMBER(compressTextures),
				CR_MEMBER(drawFog),
				CR_RESERVED(64)
				));

CGlobalRendering::CGlobalRendering() {

	teamNanospray = false;

	lastFrameTime = 0.0f;
	drawFrame = 1;

	viewSizeX = 100;
	viewSizeY = 100;
	pixelX = 0.01f;
	pixelY = 0.01f;
	aspectRatio = 1.0f;

	drawSky      = true;
	drawWater    = true;
	drawGround   = true;
	drawMapMarks = true;
	drawFog      = true;
	drawdebug    = false;

	active = true;
	viewRange = MAX_VIEW_RANGE;
	timeOffset = 0;

	compressTextures = false;
	atiHacks = false;
	maxTextureSize = 1024;

	fullScreen = true;
	dualScreenMiniMapOnLeft = false;
	dualScreenMode = false;
	haveGLSL = false;
	haveARB = false;
	supportNPOTs = false;
	haveATI = false;
	depthBufferBits = false;
	
	viewPosX = 0;
	viewPosY = 0;
	
	winSizeX = 0;
	winSizeY = 0;
	
	winPosX = 0;
	winPosY = 0;
	
	screenSizeX = 0;
	screenSizeY = 0;
	
	weightedSpeedFactor = 0.0f;
	lastFrameStart = 0;
	needsUpdate = true;
}

void CGlobalRendering::PostInit() {

	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	haveARB = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL = !!GLEW_VERSION_2_0;

	{
		const std::string vendor = StringToLower(std::string((char*) glGetString(GL_VENDOR)));
		const std::string renderer = StringToLower(std::string((char*) glGetString(GL_RENDERER)));

		haveATI = (vendor.find("ati ") != std::string::npos);

		if (haveATI) {
			//! x-series doesn't support NPOTs (but hd-series does)
			supportNPOTs = (renderer.find(" x") == std::string::npos && renderer.find(" 9") == std::string::npos);
		}
	}

	//! Runtime compress textures?
	if (GLEW_ARB_texture_compression) {
		//! we don't even need to check it, 'cos groundtextures must have that extension
		//! default to off because it reduces quality (smallest mipmap level is bigger)
		compressTextures = !!configHandler->Get("CompressTextures", 0);
	}

	//! maximum 2D texture size
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	}

	//! use some ATI bugfixes?
	const int atiHacksCfg = configHandler->Get("AtiHacks", -1);
	atiHacks = haveATI && (atiHacksCfg < 0); //! runtime detect
	atiHacks |= atiHacksCfg > 0; //! user override
	if (atiHacks) {
		logOutput.Print("ATI hacks enabled\n");
	}

	dynamicSun = configHandler->Get("DynamicSun", 1);
}

void CGlobalRendering::Update() {
	if(!needsUpdate)
		return;
	needsUpdate = false;

	sky->UpdateSunDir();
	unitDrawer->UpdateSunDir();
	readmap->GetGroundDrawer()->UpdateSunDir();
	groundDecals->UpdateSunDir();

	sunIntensity = sqrt(std::max(0.0f, std::min(sunDir.y, 1.0f)));
	float shadowDensity = std::min(1.0f, sunIntensity * shadowDensityFactor);
	unitShadowDensity = shadowDensity * mapInfo->light.unitShadowDensity;
	groundShadowDensity = shadowDensity *  mapInfo->light.groundShadowDensity;
}

float4 CGlobalRendering::CalculateSunDir(float startAngle) {
	float sang = PI - startAngle - initialSunAngle - gs->frameNum * 2.0f * PI / (GAME_SPEED * sunOrbitTime);

	float4 sdir(sunOrbitRad * cos(sang), sunOrbitHeight, sunOrbitRad * sin(sang));
	sdir = sunRotation.Mul(sdir);

	return sdir;
}

void CGlobalRendering::UpdateSunParams(float4 newSunDir, float startAngle, float orbitTime, bool iscompat) {
	newSunDir.ANormalize();
	sunStartAngle = startAngle;
	sunOrbitTime = orbitTime;

	initialSunAngle = fastmath::coords2angle(newSunDir.x, newSunDir.z);

	if(iscompat) { // backwards compatible: sunDir is position where sun reaches highest altitude
		float sunLen = newSunDir.Length2D();
		float sunAzimuth = (sunLen <= 0.001f) ? PI / 2.0f : atan(newSunDir.y / sunLen);
		float sunHeight = tan(sunAzimuth - 0.001f);

		float orbitMinSunHeight = 0.1f; // the lowest sun altitude for an auto generated orbit
		float3 v1(cos(initialSunAngle), sunHeight, sin(initialSunAngle));
		v1.ANormalize();

		if(v1.y <= orbitMinSunHeight) {
			newSunDir = float3(0.0f, 1.0f, 0.0f);
			sunOrbitHeight = v1.y;
			sunOrbitRad = sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
		}
		else {
			float3 v2(cos(initialSunAngle + PI), orbitMinSunHeight, sin(initialSunAngle + PI));
			v2.ANormalize();
			float3 v3 = v2 - v1;
			sunOrbitRad = v3.Length() / 2.0f;
			v3.ANormalize();
			float3 v4 = v3.cross(float3(0.0f, 1.0f, 0.0f));
			v4.ANormalize();
			float3 v5 = v3.cross(v4);
			v5.ANormalize();
			if(v5.y < 0)
				v5 = -v5;
			newSunDir = v5;
			sunOrbitHeight = v5.dot(v1);
		}
	}
	else { // new: sunDir is center position of orbit, and sunDir.w is orbit height
		sunOrbitHeight = std::max(-1.0f, std::min(newSunDir.w, 1.0f));
		sunOrbitRad = sqrt(1.0f - sunOrbitHeight * sunOrbitHeight);
	}

	sunRotation.LoadIdentity();
	sunRotation.SetUpVector(newSunDir);

	float4 peakSunDir = CalculateSunDir(0.0f);
	shadowDensityFactor = 1.0f / std::max(0.01f, peakSunDir.y);
	UpdateSun(true);
}

void CGlobalRendering::UpdateSunDir(const float4 &newSunDir) {
	float4 newSunDirNorm = newSunDir;
	newSunDirNorm.ANormalize();

	if(newSunDirNorm != sunDir) {
		sunDir = newSunDirNorm;
		needsUpdate = true;
	}
}

void CGlobalRendering::UpdateSun(bool forced) {
	if(!forced && globalRendering->dynamicSun != 1)
		return;

	UpdateSunDir(CalculateSunDir(sunStartAngle));
}

