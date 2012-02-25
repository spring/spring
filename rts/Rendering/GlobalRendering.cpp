/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GlobalRendering.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/mmgr.h"
#include "System/Util.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/creg_cond.h"

#include <string>

CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true); // in safemode enabled, cause it ways more likely the gpu runs out of memory than this extension cause crashes!
CONFIG(int, AtiHacks).defaultValue(-1);
CONFIG(bool, DualScreenMode).defaultValue(false);
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false);

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
CGlobalRendering* globalRendering;

const float CGlobalRendering::MAX_VIEW_RANGE = 8000.0f;
const float CGlobalRendering::NEAR_PLANE     =    2.8f;

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
	CR_MEMBER(drawFog),
	CR_RESERVED(64)
));

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_gettime())
	, weightedSpeedFactor(0.0f)
	, drawFrame(1)
	, FPS(30.0f)

	, winState(0)
	, screenSizeX(1)
	, screenSizeY(1)

	// window geometry
	, winPosX(0)
	, winPosY(0)
	, winSizeX(1)
	, winSizeY(1)

	// viewport geometry
	, viewPosX(0)
	, viewPosY(0)
	, viewSizeX(1)
	, viewSizeY(1)

	// pixel geometry
	, pixelX(0.01f)
	, pixelY(0.01f)

	, aspectRatio(1.0f)

	, zNear(NEAR_PLANE)
	, viewRange(MAX_VIEW_RANGE)
	, FSAA(0)
	, depthBufferBits(0)

	, maxTextureSize(1024)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)
	, drawdebug(false)

	, teamNanospray(false)
	, active(true)
	, compressTextures(false)
	, haveATI(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, atiHacks(false)
	, supportNPOTs(false)
	, support24bitDepthBuffers(false)
	, haveARB(false)
	, haveGLSL(false)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(true)
{
}

void CGlobalRendering::PostInit() {
	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	haveARB   = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != NULL);
	haveGLSL &= GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader;
	haveGLSL &= !!GLEW_VERSION_2_0; // we want OpenGL 2.0 core functions

	{
		const char* glVendor = (const char*) glGetString(GL_VENDOR);
		const char* glRenderer = (const char*) glGetString(GL_RENDERER);
		const std::string vendor = (glVendor != NULL)? StringToLower(std::string(glVendor)): "";
		const std::string renderer = (glRenderer != NULL)? StringToLower(std::string(glRenderer)): "";

		haveATI    = (vendor.find("ati ") != std::string::npos) || (vendor.find("amd ") != std::string::npos);
		haveMesa   = (renderer.find("mesa ") != std::string::npos);
		haveIntel  = (vendor.find("intel") != std::string::npos);
		haveNvidia = (vendor.find("nvidia ") != std::string::npos);

		//FIXME Neither Intel's nor Mesa's GLSL implementation seem to be in a workable state atm (date: Nov. 2011)
		haveGLSL &= !haveIntel;
		haveGLSL &= !haveMesa;
		//FIXME add an user config to force enable it!

		if (haveATI) {
			// x-series doesn't support NPOTs (but hd-series does)
			supportNPOTs = (renderer.find(" x") == std::string::npos && renderer.find(" 9") == std::string::npos);
		}
	}

	// use some ATI bugfixes?
	const int atiHacksCfg = configHandler->GetInt("AtiHacks");
	atiHacks = haveATI && (atiHacksCfg < 0); // runtime detect
	atiHacks |= (atiHacksCfg > 0); // user override

	// Runtime compress textures?
	if (GLEW_ARB_texture_compression) {
		// we don't even need to check it, 'cos groundtextures must have that extension
		// default to off because it reduces quality (smallest mipmap level is bigger)
		compressTextures = configHandler->GetBool("CompressTextures");
	}

	// maximum 2D texture size
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	}

	// detect if GL_DEPTH_COMPONENT24 is supported (many ATIs don't do so)
	{
		// ATI seems to support GL_DEPTH_COMPONENT24 for static textures, but you can't render to them
		/*
		GLint state = 0;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 16, 16, 0, GL_LUMINANCE, GL_FLOAT, NULL);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &state);
		support24bitDepthBuffers = (state > 0);
		*/

		support24bitDepthBuffers = false;
		if (FBO::IsSupported() && !atiHacks) {
			const int fboSizeX = 16, fboSizeY = 16;

			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, fboSizeX, fboSizeY);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT,  GL_DEPTH_COMPONENT24, fboSizeX, fboSizeY);
			const GLenum status = fbo.GetStatus();
			fbo.Unbind();

			support24bitDepthBuffers = (status == GL_FRAMEBUFFER_COMPLETE_EXT);
		}
	}

	// print info
	LOG(
		"GL info:\n"
		"\thaveARB: %i, haveGLSL: %i, ATI hacks: %i\n"
		"\tFBO support: %i, NPOT-texture support: %i, 24bit Z-buffer support: %i\n"
		"\tmaximum texture size: %i, compress MIP-map textures: %i",
		haveARB, haveGLSL, atiHacks,
		FBO::IsSupported(), supportNPOTs, support24bitDepthBuffers,
		maxTextureSize, compressTextures
	);
}



void CGlobalRendering::SetDualScreenParams() {
	dualScreenMode = configHandler->GetBool("DualScreenMode");

	if (dualScreenMode) {
		dualScreenMiniMapOnLeft = configHandler->GetBool("DualScreenMiniMapOnLeft");
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
