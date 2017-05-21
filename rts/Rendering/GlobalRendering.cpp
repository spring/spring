/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include <SDL.h>


#include "GlobalRendering.h"
#include "GlobalRenderingInfo.h"

#include "Rendering/VerticalSync.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/type2.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"
#include "System/creg/creg_cond.h"


CONFIG(bool, DebugGL).defaultValue(false).description("Enables _driver_ debug feedback. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);
CONFIG(int, FSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("If >0 enables FullScreen AntiAliasing.");

CONFIG(int, ForceDisableShaders).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceCoreContext).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceSwapBuffers).defaultValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with ATI video cards.\n -1:=runtime detect, 0:=off, 1:=on");

// enabled in safemode, far more likely the gpu runs out of memory than this extension causes crashes!
CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true).description("Runtime compress most textures to save VideoRAM.");
CONFIG(bool, DualScreenMode).defaultValue(false).description("Sets whether to split the screen in half, with one half for minimap and one for main screen. Right side is for minimap unless DualScreenMiniMapOnLeft is set.");
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false).description("When set, will make the left half of the screen the minimap when DualScreenMode is set.");
CONFIG(bool, TeamNanoSpray).defaultValue(true).headlessValue(false);
CONFIG(float, TextureLODBias).defaultValue(0.0f).minimumValue(-4.0f).maximumValue(4.0f);
CONFIG(int, MinimizeOnFocusLoss).defaultValue(0).minimumValue(0).maximumValue(1).description("When set to 1 minimize Window if it loses key focus when in fullscreen mode.");

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
CGlobalRendering* globalRendering;
GlobalRenderingInfo globalRenderingInfo;

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
	CR_MEMBER(gldebug),
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

	CR_IGNORED(forceDisableShaders),
	CR_IGNORED(forceCoreContext),
	CR_IGNORED(forceSwapBuffers),

	CR_IGNORED(fsaaLevel),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(gpuMemorySize),
	CR_IGNORED(active),
	CR_IGNORED(isVideoCapturing),
	CR_IGNORED(compressTextures),

	CR_IGNORED(haveATI),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(atiHacks),
	CR_IGNORED(supportNPOTs),
	CR_IGNORED(support24bitDepthBuffers),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(supportClipControl),
	CR_IGNORED(haveARB),
	CR_IGNORED(haveGLSL),
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
	CR_IGNORED(borderless),

	CR_IGNORED(window),
	CR_IGNORED(sdlGlCtx)
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

	, forceDisableShaders(configHandler->GetInt("ForceDisableShaders"))
	, forceCoreContext(configHandler->GetInt("ForceCoreContext"))
	, forceSwapBuffers(configHandler->GetInt("ForceSwapBuffers"))

	, fsaaLevel(configHandler->GetInt("FSAALevel"))
	, maxTextureSize(2048)
	, gpuMemorySize(0)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)
	, drawdebug(false)
	, drawdebugtraceray(false)
	, gldebug(false)

	, teamNanospray(configHandler->GetBool("TeamNanoSpray"))
	, active(true)
	, isVideoCapturing(false)
	, compressTextures(false)
	, haveATI(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, atiHacks(false)
	, supportNPOTs(false)
	, support24bitDepthBuffers(false)
	, supportRestartPrimitive(false)
	, supportClipControl(false)
	, haveARB(false)
	, haveGLSL(false)

	, glslMaxVaryings(0)
	, glslMaxAttributes(0)
	, glslMaxDrawBuffers(0)
	, glslMaxRecommendedIndices(0)
	, glslMaxRecommendedVertices(0)
	, glslMaxUniformBufferBindings(0)
	, glslMaxUniformBufferSize(0)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(configHandler->GetBool("Fullscreen"))
	, borderless(configHandler->GetBool("WindowBorderless"))

	, window(nullptr)
	, sdlGlCtx(nullptr)
{
	configHandler->NotifyOnChange(this, {"TextureLODBias", "Fullscreen", "WindowBorderless"});
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
}


bool CGlobalRendering::CreateSDLWindow(const char* title)
{
	if (!CheckAvailableVideoModes())
		throw unsupported_error("desktop color-depth should be at least 24 bits per pixel, aborting");

	// Get wanted resolution and context-version
	const int2 winRes = GetWantedViewSize(fullScreen);
	const int2 oglCtx = {configHandler->GetInt("GLContextMajorVersion"), configHandler->GetInt("GLContextMinorVersion")};
	const int2 glCtxs[] = {{2, 0}, {2, 1},  {3, 0}, {3, 1}, {3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}};

	if (std::find(&glCtxs[0], &glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)), oglCtx) == (&glCtxs[0] + (sizeof(glCtxs) / sizeof(int2))))
		throw unsupported_error("illegal OpenGL context-version specified, aborting");

	uint32_t sdlFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	sdlFlags |= (borderless? SDL_WINDOW_FULLSCREEN_DESKTOP: SDL_WINDOW_FULLSCREEN) * fullScreen;
	sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless);

	// use standard: 24bit color + 24bit depth + 8bit stencil & doublebuffered
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// create GL debug-context if wanted (more verbose GL messages, but runs slower)
	// for Mesa requesting a core profile explicitly is needed to get versions later
	// than 3.0/1.30, although this still suffices for most of Spring
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, forceCoreContext? SDL_GL_CONTEXT_PROFILE_CORE: SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG * configHandler->GetBool("DebugGL"));

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, oglCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, oglCtx.y);

	// FullScreen AntiAliasing
	if (fsaaLevel > 0) {
		if (getenv("LIBGL_ALWAYS_SOFTWARE") != nullptr)
			LOG_L(L_WARNING, "FSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		make_even_number(fsaaLevel);
	}

	// Window Pos & State
	winPosX  = configHandler->GetInt("WindowPosX");
	winPosY  = configHandler->GetInt("WindowPosY");

	switch ((winState = configHandler->GetInt("WindowState"))) {
		case CGlobalRendering::WINSTATE_MAXIMIZED: sdlFlags |= SDL_WINDOW_MAXIMIZED; break;
		case CGlobalRendering::WINSTATE_MINIMIZED: sdlFlags |= SDL_WINDOW_MINIMIZED; break;
	}


	const int aaLvls[] = {fsaaLevel, fsaaLevel / 2, fsaaLevel / 4, fsaaLevel / 8, fsaaLevel / 16, fsaaLevel / 32, 0};
	const int zbBits[] = {24, 32, 16};

	// Create Window
	for (size_t i = 0; i < (sizeof(aaLvls) / sizeof(aaLvls[0])) && (window == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]);

		for (size_t j = 0; j < (sizeof(zbBits) / sizeof(zbBits[0])) && (window == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((window = SDL_CreateWindow(title, winPosX, winPosY, winRes.x, winRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, "[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer", __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG("[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer", __func__, aaLvls[i], zbBits[j]);
		}
	}

	if (window == nullptr) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "[GR::%s] could not create SDL-window\n", __func__);
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}


	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

#if defined(WIN32)
	if (borderless && !fullScreen) {
		WindowManagerHelper::SetWindowResizable(window, !borderless);

		SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);
		SDL_SetWindowPosition(window, winPosX, winPosY);
		SDL_SetWindowSize(window, winRes.x, winRes.y);
	}
#endif

	SDL_SetWindowMinimumSize(window, minWinSizeX, minWinSizeY);


	// Create GL Context
	assert(sdlGlCtx == nullptr);

	if ((sdlGlCtx = SDL_GL_CreateContext(window)) == nullptr) {
		const char* frmts[] = {"[GR::%s] error \"%s\" creating GL%d.%d %s-context", "[GR::%s] created GL%d.%d %s-context"};
		const char* profs[] = {"compat", "core"};
		char buf[1024];

		for (const int2 tmpCtx: glCtxs) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

			for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

				if ((sdlGlCtx = SDL_GL_CreateContext(window)) == nullptr) {
					LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
				} else {
					LOG_L(L_WARNING, frmts[true], __func__, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
				}

				// accepts nullptr's
				SDL_GL_DeleteContext(sdlGlCtx);
			}
		}

		SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), oglCtx.x, oglCtx.y, profs[forceCoreContext]);
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// redundant, but harmless
	SDL_GL_MakeCurrent(window, sdlGlCtx);


#if !defined(HEADLESS)
	// disable desktop compositing to fix tearing
	// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
	// On Windows Aero often uses vsync, and so when Spring runs windowed it will run with
	// vsync too, resulting in bad performance.
	if (configHandler->GetBool("BlockCompositing"))
		WindowManagerHelper::BlockCompositing(globalRendering->window);
#endif

	return true;
}


void CGlobalRendering::DestroySDLWindow() {
	SDL_SetWindowGrab(window, SDL_FALSE);
	SDL_DestroyWindow(window);

	window = nullptr;

#if !defined(HEADLESS)
	SDL_GL_DeleteContext(sdlGlCtx);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
	SDL_Quit();
}


void CGlobalRendering::PostInit() {
	#ifndef HEADLESS
	glewExperimental = true;
	#endif
	glewInit();
	// glewInit sets GL_INVALID_ENUM, get rid of it
	glGetError();

	char sdlVersionStr[64] = "";
	char glVidMemStr[64] = "unknown";

	QueryVersionInfo(sdlVersionStr, glVidMemStr);
	CheckGLExtensions();
	SetGLSupportFlags();
	QueryGLMaxVals();

	LogVersionInfo(sdlVersionStr, glVidMemStr);
	ToggleGLDebugOutput();
}

void CGlobalRendering::SwapBuffers(bool allowSwapBuffers)
{
	SCOPED_TIMER("Misc::SwapBuffers");
	assert(window != nullptr);

	if (!allowSwapBuffers && !forceSwapBuffers)
		return;

	const spring_time pre = spring_now();

	VSync.Delay();
	SDL_GL_SwapWindow(window);

	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
}


void CGlobalRendering::CheckGLExtensions() const
{
	char extMsg[2048] = {0};
	char errMsg[2048] = {0};
	char* ptr = &extMsg[0];

	if (!GLEW_ARB_multitexture       ) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " GL_ARB_multitexture");
	if (!GLEW_ARB_texture_env_combine) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " GL_ARB_texture_env_combine");
	if (!GLEW_ARB_texture_compression) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " GL_ARB_texture_compression");

	if (extMsg[0] != 0) {
		SNPRINTF(errMsg, sizeof(errMsg),
			"Needed OpenGL extension(s) not found:\n"
			"  %s\n\n"
			"Update your graphics-card drivers!\n"
			"  Graphics card:  %s\n"
			"  OpenGL version: %s\n",
			extMsg,
			globalRenderingInfo.glRenderer,
			globalRenderingInfo.glVersion);
		throw unsupported_error(errMsg);
	}
}

void CGlobalRendering::SetGLSupportFlags()
{
	const std::string& glVendor = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);

	haveARB   = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != nullptr);
	haveGLSL &= GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader;
	haveGLSL &= !!GLEW_VERSION_2_0; // we want OpenGL 2.0 core functions

	if (!haveARB || !haveGLSL)
		throw unsupported_error("OpenGL shaders not supported, aborting");

	// useful if a GPU claims to support GL3 and shaders but crashes (Intels...)
	haveARB  &= !forceDisableShaders;
	haveGLSL &= !forceDisableShaders;

	haveATI    = (  glVendor.find(   "ati ") != std::string::npos) || (glVendor.find("amd ") != std::string::npos);
	haveIntel  = (  glVendor.find(  "intel") != std::string::npos);
	haveNvidia = (  glVendor.find("nvidia ") != std::string::npos);
	haveMesa   = (glRenderer.find(  "mesa ") != std::string::npos) || (glRenderer.find("gallium ") != std::string::npos);

	if (haveATI) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "ATI";
	} else if (haveIntel) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Intel";
	} else if (haveNvidia) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Nvidia";
	} else if (haveMesa) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = globalRenderingInfo.glVendor;
	} else {
		globalRenderingInfo.gpuName   = "Unknown";
		globalRenderingInfo.gpuVendor = "Unknown";
	}

	// ATI's x-series doesn't support NPOTs, hd-series does
	supportNPOTs = GLEW_ARB_texture_non_power_of_two && (!haveATI || (glRenderer.find(" x") == std::string::npos && glRenderer.find(" 9") == std::string::npos));


	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glVersionShort) && globalRenderingInfo.glVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glVersionShort[n] = globalRenderingInfo.glVersion[n]) == ' ') {
			globalRenderingInfo.glVersionShort[n] = 0;
			break;
		}
	}

	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glslVersionShort) && globalRenderingInfo.glslVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glslVersionShort[n] = globalRenderingInfo.glslVersion[n]) == ' ') {
			globalRenderingInfo.glslVersionShort[n] = 0;
			break;
		}
	}


	{
		// use some ATI bugfixes?
		const int atiHacksCfg = configHandler->GetInt("AtiHacks");
		atiHacks = haveATI;
		atiHacks &= (atiHacksCfg < 0); // runtime detect
		atiHacks |= (atiHacksCfg > 0); // user override
	}

	// runtime-compress textures? (also already required for SMF ground textures)
	// default to off because it reduces quality (smallest mipmap level is bigger)
	if (GLEW_ARB_texture_compression)
		compressTextures = configHandler->GetBool("CompressTextures");


	#ifdef GLEW_NV_primitive_restart
	supportRestartPrimitive = !!(GLEW_NV_primitive_restart);
	#endif
	#ifdef GL_ARB_clip_control
	supportClipControl = !!(GL_ARB_clip_control);
	#endif

	// detect if GL_DEPTH_COMPONENT24 is supported (many ATIs don't;
	// they seem to support GL_DEPTH_COMPONENT24 for static textures
	// but those can't be rendered to)
	{
		#if 0
		GLint state = 0;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 16, 16, 0, GL_LUMINANCE, GL_FLOAT, nullptr);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &state);
		support24bitDepthBuffers = (state > 0);
		#else
		if (FBO::IsSupported() && !atiHacks) {
			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 16, 16);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT,  GL_DEPTH_COMPONENT24, 16, 16);
			support24bitDepthBuffers = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT);
			fbo.Unbind();
		}
		#endif
	}
}

void CGlobalRendering::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	// some GLSL relevant information
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glslMaxUniformBufferBindings);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,      &glslMaxUniformBufferSize);
	glGetIntegerv(GL_MAX_VARYING_FLOATS,          &glslMaxVaryings);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,          &glslMaxAttributes);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS,            &glslMaxDrawBuffers);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,        &glslMaxRecommendedIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,       &glslMaxRecommendedVertices);

	// GL_MAX_VARYING_FLOATS is the maximum number of floats, we count float4's
	glslMaxVaryings /= 4;
}

void CGlobalRendering::QueryVersionInfo(char (&sdlVersionStr)[64], char (&glVidMemStr)[64])
{
	SDL_VERSION(&globalRenderingInfo.sdlVersionCompiled);
	SDL_GetVersion(&globalRenderingInfo.sdlVersionLinked);

	if ((globalRenderingInfo.glVersion   = (const char*) glGetString(GL_VERSION                 )) == nullptr) globalRenderingInfo.glVersion   = "unknown";
	if ((globalRenderingInfo.glVendor    = (const char*) glGetString(GL_VENDOR                  )) == nullptr) globalRenderingInfo.glVendor    = "unknown";
	if ((globalRenderingInfo.glRenderer  = (const char*) glGetString(GL_RENDERER                )) == nullptr) globalRenderingInfo.glRenderer  = "unknown";
	if ((globalRenderingInfo.glslVersion = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION)) == nullptr) globalRenderingInfo.glslVersion = "unknown";
	if ((globalRenderingInfo.glewVersion = (const char*) glewGetString(GLEW_VERSION             )) == nullptr) globalRenderingInfo.glewVersion = "unknown";

	if (!ShowDriverWarning(globalRenderingInfo.glVendor, globalRenderingInfo.glRenderer))
		throw unsupported_error("OpenGL drivers not installed, aborting");

	memset(globalRenderingInfo.glVersionShort, 0, sizeof(globalRenderingInfo.glVersionShort));
	memset(globalRenderingInfo.glslVersionShort, 0, sizeof(globalRenderingInfo.glslVersionShort));

	constexpr const char* sdlFmtStr = "%d.%d.%d (linked) / %d.%d.%d (compiled)";
	constexpr const char* memFmtStr = "%iMB (total) / %iMB (available)";

	SNPRINTF(sdlVersionStr, sizeof(sdlVersionStr), sdlFmtStr,
		globalRenderingInfo.sdlVersionLinked.major, globalRenderingInfo.sdlVersionLinked.minor, globalRenderingInfo.sdlVersionLinked.patch,
		globalRenderingInfo.sdlVersionCompiled.major, globalRenderingInfo.sdlVersionCompiled.minor, globalRenderingInfo.sdlVersionCompiled.patch
	);

	GLint vidMemBuffer[2] = {0, 0};

	if (GetAvailableVideoRAM(vidMemBuffer)) {
		const GLint totalMemMB = vidMemBuffer[0] / 1024;
		const GLint availMemMB = vidMemBuffer[1] / 1024;

		if (totalMemMB > 0 && availMemMB > 0)
			SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, totalMemMB, availMemMB);

		gpuMemorySize = totalMemMB;
	}
}

void CGlobalRendering::LogVersionInfo(const char* sdlVersionStr, const char* glVidMemStr) const
{
	LOG("[GR::%s]", __func__);
	LOG("\tSDL version : %s", sdlVersionStr);
	LOG("\tGL version  : %s", globalRenderingInfo.glVersion);
	LOG("\tGL vendor   : %s", globalRenderingInfo.glVendor);
	LOG("\tGL renderer : %s", globalRenderingInfo.glRenderer);
	LOG("\tGLSL version: %s", globalRenderingInfo.glslVersion);
	LOG("\tGLEW version: %s", globalRenderingInfo.glewVersion);
	LOG("\tGPU memory  : %s", glVidMemStr);
	LOG("\tSwapInterval: %d", SDL_GL_GetSwapInterval());
	LOG("\t");
	LOG("\tARB shader support       : %i", haveARB);
	LOG("\tGLSL shader support      : %i", haveGLSL);
	LOG("\tFBO extension support    : %i", FBO::IsSupported());
	LOG("\tNPOT-texture support     : %i", supportNPOTs);
	LOG("\t24-bit Z-buffer support  : %i", support24bitDepthBuffers);
	LOG("\tprimitive-restart support: %i", supportRestartPrimitive);
	LOG("\tclip-control support     : %i", supportClipControl);
	LOG("\t");
	LOG("\tmax. FBO samples             : %i", FBO::GetMaxSamples());
	LOG("\tmax. texture size            : %i", maxTextureSize);
	LOG("\tmax. vec4 varyings/attributes: %i/%i", glslMaxVaryings, glslMaxAttributes);
	LOG("\tmax. draw-buffers            : %i", glslMaxDrawBuffers);
	LOG("\tmax. rec. indices/vertices   : %i/%i", glslMaxRecommendedIndices, glslMaxRecommendedVertices);
	LOG("\tmax. uniform buffer-bindings : %i", glslMaxUniformBufferBindings);
	LOG("\tmax. uniform block-size      : %iKB", glslMaxUniformBufferSize / 1024);
	LOG("\t");
	LOG("\tenable ATI-hacks : %i", atiHacks);
	LOG("\tcompress MIP-maps: %i", compressTextures);

}

void CGlobalRendering::LogDisplayMode() const
{
	// print final mode (call after SetupViewportGeometry, which updates viewSizeX/Y)
	SDL_DisplayMode dmode;
	SDL_GetWindowDisplayMode(window, &dmode);

	constexpr const char* names[] = {
		"windowed::decorated",
		"windowed::borderless",
		"fullscreen::decorated",
		"fullscreen::borderless",
	};

	const int fs = fullScreen;
	const int bl = borderless;

	LOG("[%s] display-mode set to %ix%ix%ibpp@%iHz (%s)", __func__, viewSizeX, viewSizeY, SDL_BITSPERPIXEL(dmode.format), dmode.refresh_rate, names[fs * 2 + bl]);
}


void CGlobalRendering::SetFullScreen(bool configFullScreen, bool cmdLineWindowed, bool cmdLineFullScreen)
{
	fullScreen = (configFullScreen && !cmdLineWindowed  );
	fullScreen = (configFullScreen ||  cmdLineFullScreen);

	configHandler->Set("Fullscreen", fullScreen);
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	if (window == nullptr)
		return;

	if (key == "TextureLODBias") {
		UpdateGLConfigs();
		return;
	}

	borderless = configHandler->GetBool("WindowBorderless");
	fullScreen = configHandler->GetBool("Fullscreen");

	const int2 res = GetWantedViewSize(fullScreen);

	SDL_SetWindowSize(window, res.x, res.y);
	SDL_SetWindowFullscreen(window, (borderless? SDL_WINDOW_FULLSCREEN_DESKTOP: SDL_WINDOW_FULLSCREEN) * fullScreen);
	SDL_SetWindowPosition(window, configHandler->GetInt("WindowPosX"), configHandler->GetInt("WindowPosY"));
	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);
	WindowManagerHelper::SetWindowResizable(window, !borderless && !fullScreen);
	// set size again in an attempt to fix some bugs
	SDL_SetWindowSize(window, res.x, res.y);
}


int2 CGlobalRendering::GetWantedViewSize(const bool fullscreen)
{
	static const char* xsKeys[2] = {"XResolutionWindowed", "XResolution"};
	static const char* ysKeys[2] = {"YResolutionWindowed", "YResolution"};

	int2 res = {configHandler->GetInt(xsKeys[fullscreen]), configHandler->GetInt(ysKeys[fullscreen])};

	{
		// use Native Desktop Resolution
		// SDL2 can do this itself when size{X,Y} are set to
		// zero, but fails with Display Cloneing and similar
		//  -> DVI monitor will run at 640x400 and HDMI at full-HD
		SDL_DisplayMode dmode;
		SDL_GetDesktopDisplayMode(0, &dmode); //TODO make screen configurable?

		if (res.x <= 0) res.x = dmode.w;
		if (res.y <= 0) res.y = dmode.h;
	}

	// limit minimum window size in windowed mode
	if (!fullscreen) {
		res.x = std::max(res.x, minWinSizeX);
		res.y = std::max(res.y, minWinSizeY);
	}

	return res;
}


void CGlobalRendering::SetDualScreenParams()
{
	if ((dualScreenMode = configHandler->GetBool("DualScreenMode"))) {
		dualScreenMiniMapOnLeft = configHandler->GetBool("DualScreenMiniMapOnLeft");
	} else {
		dualScreenMiniMapOnLeft = false;
	}
}

void CGlobalRendering::UpdateViewPortGeometry()
{
	// NOTE: viewPosY is not currently used (always 0)
	if (!dualScreenMode) {
		viewSizeX = winSizeX;
		viewSizeY = winSizeY;

		viewPosX = 0;
		viewPosY = 0;
		return;
	}

	viewSizeX = winSizeX >> 1;
	viewSizeY = winSizeY;

	viewPosX = (winSizeX >> 1) * dualScreenMiniMapOnLeft;
	viewPosY = 0;
}

void CGlobalRendering::UpdatePixelGeometry()
{
	pixelX = 1.0f / viewSizeX;
	pixelY = 1.0f / viewSizeY;

	aspectRatio = viewSizeX / float(viewSizeY);
}

void CGlobalRendering::UpdateGLConfigs()
{
	// re-read configuration value
	VSync.SetInterval();

	const float lodBias = configHandler->GetFloat("TextureLODBias");

	if (math::fabs(lodBias) > 0.01f) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, lodBias);
	} else {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0.0f);
	}
}

/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
bool CGlobalRendering::EnableFSAA() const
{
	if (fsaaLevel == 0)
		return false;
	if (!GLEW_ARB_multisample)
		return false;
	GLint buffers = 0;
	GLint samples = 0;
	glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &buffers);
	glGetIntegerv(GL_SAMPLES_ARB, &samples);
	return (buffers != 0 && samples != 0);
}

#if defined(WIN32) && !defined(HEADLESS)
	#if defined(_MSC_VER) && _MSC_VER >= 1600
		#define _GL_APIENTRY __stdcall
	#else
		#include <windef.h>
		#define _GL_APIENTRY APIENTRY
	#endif
#else
	#define _GL_APIENTRY
#endif

void _GL_APIENTRY glDebugMessageCallbackFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	switch (id) {
		case 131185: { return; } break; // "Buffer detailed info: Buffer object 260 (bound to GL_PIXEL_UNPACK_BUFFER_ARB, usage hint is GL_STREAM_DRAW) has been mapped in DMA CACHED memory."
		default: {} break;
	}

	const char*   sourceStr = "";
	const char*     typeStr = "";
	const char* severityStr = "";
	const char*  messageStr = message;

	switch (source) {
		case GL_DEBUG_SOURCE_API_ARB            : sourceStr =          "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB  : sourceStr = "WindowSystem"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: sourceStr =       "Shader"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB    : sourceStr =    "3rd Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB    : sourceStr =  "Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB          : sourceStr =        "other"; break;
		default                                 : sourceStr =      "unknown";
	}

	switch (type) {
		case GL_DEBUG_TYPE_ERROR_ARB              : typeStr =       "error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typeStr =  "deprecated"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB : typeStr =   "undefined"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB        : typeStr = "portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB        : typeStr = "performance"; break;
		case GL_DEBUG_TYPE_OTHER_ARB              : typeStr =       "other"; break;
		default                                   : typeStr =     "unknown";
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH_ARB  : severityStr =    "high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: severityStr =  "medium"; break;
		case GL_DEBUG_SEVERITY_LOW_ARB   : severityStr =     "low"; break;
		default                          : severityStr = "unknown";
	}

	LOG_L(L_INFO, "OpenGL: source<%s> type<%s> id<%u> severity<%s>:\n%s", sourceStr, typeStr, id, severityStr, messageStr);

	if ((userParam == nullptr) || !(*reinterpret_cast<const bool*>(userParam)))
		return;

	CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
	#endif
}


bool CGlobalRendering::ToggleGLDebugOutput()
{
	const static bool dbgOutput = configHandler->GetBool("DebugGL");
	const static bool dbgTraces = configHandler->GetBool("DebugGLStacktraces");

	if (!dbgOutput) {
		LOG("[GR::%s] OpenGL debug context required (dbgTraces=%d)", __func__, dbgTraces);
		return false;
	}

	#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	if ((gldebug = !gldebug)) {
		// install OpenGL debug message callback; typecast is a workaround
		// for #4510 (change in callback function signature with GLEW 1.11)
		// use SYNCHRONOUS output, we want our callback to run in the same
		// thread as the bugged GL call (for proper stacktraces)
		// CB userParam is const, but has to be specified sans qualifiers
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallbackARB((GLDEBUGPROCARB) &glDebugMessageCallbackFunc, (void*) &dbgTraces);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

		LOG("[GR::%s] OpenGL debug-message callback enabled", __func__);
	} else {
		glDebugMessageCallbackARB(nullptr, nullptr);
		glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

		LOG("[GR::%s] OpenGL debug-message callback disabled", __func__);
	}
	#endif

	return true;
}
