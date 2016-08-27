/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GlobalRendering.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Util.h"
#include "System/type2.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/creg_cond.h"

#include <string>
#include <SDL_video.h>

CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true).description("Runtime compress most textures to save VideoRAM."); // in safemode enabled, cause it ways more likely the gpu runs out of memory than this extension cause crashes!
CONFIG(int, ForceShaders).defaultValue(-1).minimumValue(-1).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with ATI video cards.\n -1:=runtime detect, 0:=off, 1:=on");
CONFIG(bool, DualScreenMode).defaultValue(false).description("Sets whether to split the screen in half, with one half for minimap and one for main screen. Right side is for minimap unless DualScreenMiniMapOnLeft is set.");
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false).description("When set, will make the left half of the screen the minimap when DualScreenMode is set.");
CONFIG(bool, TeamNanoSpray).defaultValue(true).headlessValue(false);

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
CGlobalRendering* globalRendering;

const float CGlobalRendering::MAX_VIEW_RANGE     = 8000.0f;
const float CGlobalRendering::NEAR_PLANE         =    2.8f;
const float CGlobalRendering::SMF_INTENSITY_MULT = 210.0f / 255.0f;
const int CGlobalRendering::minWinSizeX = 400;
const int CGlobalRendering::minWinSizeY = 300;

CR_BIND(CGlobalRendering, )

CR_REG_METADATA(CGlobalRendering, (
	CR_MEMBER(teamNanospray),
	CR_MEMBER(drawSky),
	CR_MEMBER(drawWater),
	CR_MEMBER(drawGround),
	CR_MEMBER(drawMapMarks),
	CR_MEMBER(drawFog),
	CR_MEMBER(drawdebug),
	CR_MEMBER(drawdebugtraceray),
	CR_MEMBER(timeOffset),
	CR_MEMBER(lastFrameTime),
	CR_MEMBER(lastFrameStart),
	CR_MEMBER(weightedSpeedFactor),
	CR_MEMBER(drawFrame),
	CR_MEMBER(FPS),

	CR_IGNORED(winState),
	CR_IGNORED(screenSizeX),
	CR_IGNORED(screenSizeY),
	CR_IGNORED(winPosX),
	CR_IGNORED(winPosY),
	CR_IGNORED(winSizeX),
	CR_IGNORED(winSizeY),
	CR_IGNORED(viewPosX),
	CR_IGNORED(viewPosY),
	CR_IGNORED(viewSizeX),
	CR_IGNORED(viewSizeY),
	CR_IGNORED(pixelX),
	CR_IGNORED(pixelY),
	CR_IGNORED(aspectRatio),
	CR_IGNORED(zNear),
	CR_IGNORED(viewRange),
	CR_IGNORED(FSAA),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(active),
	CR_IGNORED(compressTextures),
	CR_IGNORED(haveATI),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),
	CR_IGNORED(atiHacks),
	CR_IGNORED(supportNPOTs),
	CR_IGNORED(support24bitDepthBuffers),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(haveARB),
	CR_IGNORED(haveGLSL),
	CR_IGNORED(maxSmoothPointSize),
	CR_IGNORED(glslMaxVaryings),
	CR_IGNORED(glslMaxAttributes),
	CR_IGNORED(glslMaxDrawBuffers),
	CR_IGNORED(glslMaxRecommendedIndices),
	CR_IGNORED(glslMaxRecommendedVertices),
	CR_IGNORED(glslMaxUniformBufferBindings),
	CR_IGNORED(glslMaxUniformBufferSize),
	CR_IGNORED(dualScreenMode),
	CR_IGNORED(dualScreenMiniMapOnLeft),
	CR_IGNORED(fullScreen),
	CR_IGNORED(window)
))

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_notime)
	, weightedSpeedFactor(0.0f)
	, drawFrame(1)
	, FPS(GAME_SPEED)

	, winState(WINSTATE_DEFAULT)
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

	, maxTextureSize(2048)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)
	, drawdebug(false)
	, drawdebugtraceray(false)

	, teamNanospray(true)
	, active(true)
	, compressTextures(false)
	, haveATI(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, atiHacks(false)
	, supportNPOTs(false)
	, support24bitDepthBuffers(false)
	, supportRestartPrimitive(false)
	, haveARB(false)
	, haveGLSL(false)
	, maxSmoothPointSize(1.0f)

	, glslMaxVaryings(0)
	, glslMaxAttributes(0)
	, glslMaxDrawBuffers(0)
	, glslMaxRecommendedIndices(0)
	, glslMaxRecommendedVertices(0)
	, glslMaxUniformBufferBindings(0)
	, glslMaxUniformBufferSize(0)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(true)

	, window(nullptr)
{
	configHandler->NotifyOnChange(this);
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
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
		haveMesa   = (renderer.find("mesa ") != std::string::npos) || (renderer.find("gallium ") != std::string::npos);
		haveIntel  = (vendor.find("intel") != std::string::npos);
		haveNvidia = (vendor.find("nvidia ") != std::string::npos);

		const int useGlslShaders = configHandler->GetInt("ForceShaders");
		if (useGlslShaders < 0) {
			// disable Shaders for Intel drivers
			haveARB  &= !haveIntel;
			haveGLSL &= !haveIntel;
		} else if (useGlslShaders == 0) {
			haveARB  = false;
			haveGLSL = false;
		} else if (useGlslShaders > 0) {
			// rely on extension detection (don't force enable shaders, when the extensions aren't exposed!)
		}

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

#ifdef GLEW_NV_primitive_restart
	supportRestartPrimitive = !!(GLEW_NV_primitive_restart);
#endif

	// maximum 2D texture size
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	}

	// retrieve maximu smoothed PointSize
	float2 aliasedPointSizeRange, smoothPointSizeRange;
	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, (GLfloat*)&aliasedPointSizeRange);
	glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE,  (GLfloat*)&smoothPointSizeRange);
	maxSmoothPointSize = std::min(aliasedPointSizeRange.y, smoothPointSizeRange.y);

	// some GLSL relevant information
	{
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glslMaxUniformBufferBindings);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,      &glslMaxUniformBufferSize);
		glGetIntegerv(GL_MAX_VARYING_FLOATS,          &glslMaxVaryings);
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,          &glslMaxAttributes);
		glGetIntegerv(GL_MAX_DRAW_BUFFERS,            &glslMaxDrawBuffers);
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES,        &glslMaxRecommendedIndices);
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,       &glslMaxRecommendedVertices);
		glslMaxVaryings /= 4; // GL_MAX_VARYING_FLOATS returns max individual floats, we want float4
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
		"\tmaximum texture size: %i, compress MIP-map textures: %i\n"
		"\tmaximum SmoothPointSize: %0.0f, maximum vec4 varying/attributes: %i/%i\n"
		"\tmaximum drawbuffers: %i, maximum recommended indices/vertices: %i/%i\n"
		"\tnumber of UniformBufferBindings: %i (%ikB)",
		haveARB, haveGLSL, atiHacks,
		FBO::IsSupported(), supportNPOTs, support24bitDepthBuffers,
		maxTextureSize, compressTextures, maxSmoothPointSize,
		glslMaxVaryings, glslMaxAttributes, glslMaxDrawBuffers,
		glslMaxRecommendedIndices, glslMaxRecommendedVertices,
		glslMaxUniformBufferBindings, glslMaxUniformBufferSize / 1024
	);

	teamNanospray = configHandler->GetBool("TeamNanoSpray");
}

void CGlobalRendering::SetFullScreen(bool configFullScreen, bool cmdLineWindowed, bool cmdLineFullScreen)
{
	fullScreen = configFullScreen;

	// flags
	if (cmdLineWindowed) {
		fullScreen = false;
	} else if (cmdLineFullScreen) {
		fullScreen = true;
	}

	configHandler->Set("Fullscreen", fullScreen);
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	if (key != "Fullscreen" && key != "WindowBorderless")
		return;

	fullScreen = configHandler->GetBool("Fullscreen");

	const int2 res = GetWantedViewSize(fullScreen);
	const bool borderless = configHandler->GetBool("WindowBorderless");
	if (fullScreen) {
		SDL_SetWindowSize(window, res.x, res.y);
		if (borderless) {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		} else {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		}
	} else {
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);
		SDL_SetWindowSize(window, res.x, res.y);
		SDL_SetWindowPosition(window, configHandler->GetInt("WindowPosX"), configHandler->GetInt("WindowPosY"));
	}
}


int2 CGlobalRendering::GetWantedViewSize(const bool fullscreen)
{
	int2 res = int2(configHandler->GetInt("XResolution"), configHandler->GetInt("YResolution"));

	if (!fullscreen) {
		res = int2(configHandler->GetInt("XResolutionWindowed"), configHandler->GetInt("YResolutionWindowed"));
	}

	// Use Native Desktop Resolution
	// and yes SDL2 can do this itself when sizeX & sizeY are set to zero, but
	// oh wonder SDL2 fails then when you use Display Cloneing and similar
	//  -> i.e. DVI monitor runs then at 640x400 and HDMI at full-HD (yes with display _cloning_!)
	{
		SDL_DisplayMode dmode;
		SDL_GetDesktopDisplayMode(0, &dmode); //TODO make screen configurable?
		if (res.x<=0) res.x = dmode.w;
		if (res.y<=0) res.y = dmode.h;
	}

	// In Windowed Mode Limit Minimum Window Size
	if (!fullscreen) {
		res.x = std::max(res.x, minWinSizeX);
		res.y = std::max(res.y, minWinSizeY);
	}

	return res;
}


void CGlobalRendering::SetDualScreenParams() {
	dualScreenMode = configHandler->GetBool("DualScreenMode");

	if (dualScreenMode) {
		dualScreenMiniMapOnLeft = configHandler->GetBool("DualScreenMiniMapOnLeft");
	} else {
		dualScreenMiniMapOnLeft = false;
	}
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
