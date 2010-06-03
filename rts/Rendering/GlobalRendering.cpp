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
}

void CGlobalRendering::PostInit() {

	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	haveARB = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL = !!GLEW_VERSION_2_0;

	{
		std::string vendor = StringToLower(std::string((char*) glGetString(GL_VENDOR)));
		haveATI = (vendor.find("ati ") != std::string::npos);

		if (haveATI) {
			std::string renderer = StringToLower(std::string((char*) glGetString(GL_RENDERER)));
			//! x-series doesn't support NPOTs (but hd-series does)
			supportNPOTs = (renderer.find(" x") == std::string::npos && renderer.find(" 9") == std::string::npos);
		}
	}

	// Runtime compress textures?
	if (GLEW_ARB_texture_compression) {
		// we don't even need to check it, 'cos groundtextures must have that extension
		// default to off because it reduces quality (smallest mipmap level is bigger)
		compressTextures = !!configHandler->Get("CompressTextures", 0);
	}

	// maximum 2D texture size
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	}

	// use some ATI bugfixes?
	if ((atiHacks = !!configHandler->Get("AtiHacks", haveATI? 1: 0))) {
		logOutput.Print("ATI hacks enabled\n");
	}
}

