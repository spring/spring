/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include "GlobalRendering.h"

#include "Rendering/GL/myGL.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/mmgr.h"
#include "System/Util.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"
#include "System/creg/creg_cond.h"

#include <string>


/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
CGlobalRendering* globalRendering;

const float CGlobalRendering::MAX_VIEW_RANGE = 8000.0f;
const float CGlobalRendering::NEAR_PLANE = 2.8f;

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

	FSAA = 0;

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

	winState = 0;

	// window geometry
	winPosX = 0;
	winPosY = 0;
	winSizeX = 1;
	winSizeY = 1;
	screenSizeX = 1;
	screenSizeY = 1;

	// viewport geometry
	viewPosX = 0;
	viewPosY = 0;
	viewSizeX = 1;
	viewSizeY = 1;

	// pixel geometry
	pixelX = 0.01f;
	pixelY = 0.01f;
	aspectRatio = 1.0f;

	weightedSpeedFactor = 0.0f;
	lastFrameStart = 0;
}

void CGlobalRendering::PostInit() {
	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	haveARB = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	// not enough: we want OpenGL 2.0 core functions
	// haveGLSL = GL_ARB_vertex_shader && GL_ARB_fragment_shader;
	haveGLSL = !!GLEW_VERSION_2_0;

	{
		const char* glVendor = (const char*) glGetString(GL_VENDOR);
		const char* glRenderer = (const char*) glGetString(GL_RENDERER);
		const std::string vendor = (glVendor != NULL)? StringToLower(std::string(glVendor)): "";
		const std::string renderer = (glRenderer != NULL)? StringToLower(std::string(glRenderer)): "";

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
	atiHacks |= (atiHacksCfg > 0); //! user override
	if (atiHacks) {
		logOutput.Print("ATI hacks enabled\n");
	}
}



void CGlobalRendering::SetDualScreenParams() {
	dualScreenMode = !!configHandler->Get("DualScreenMode", 0);

	if (dualScreenMode) {
		dualScreenMiniMapOnLeft = !!configHandler->Get("DualScreenMiniMapOnLeft", 0);
	} else {
		dualScreenMiniMapOnLeft = false;
	}
}

void CGlobalRendering::UpdateWindowGeometry() {
	// NOTE:
	//   in headless builds this is not called,
	//   therefore winSize{X,Y} both remain 1
	screenSizeX = viewSizeX;
	screenSizeY = viewSizeY;
	winSizeX = viewSizeX;
	winSizeY = viewSizeY;
	winPosX = 0;
	winPosY = 0;
}

void CGlobalRendering::UpdateViewPortGeometry() {
	// NOTE: viewPosY is not currently used (always 0)
	if (!dualScreenMode) {
		viewSizeX = winSizeX;
		viewSizeY = winSizeY;
		viewPosX = 0;
		viewPosY = 0;
	} else {
		viewSizeX = winSizeX / 2;
		viewSizeY = winSizeY;

		if (dualScreenMiniMapOnLeft) {
			viewPosX = winSizeX / 2;
			viewPosY = 0;
		} else {
			viewPosX = 0;
			viewPosY = 0;
		}
	}
}

void CGlobalRendering::UpdatePixelGeometry() {
	pixelX = 1.0f / viewSizeX;
	pixelY = 1.0f / viewSizeY;

	aspectRatio = viewSizeX / float(viewSizeY);
}
