/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include <SDL.h>


#include "GlobalRendering.h"
#include "GlobalRenderingInfo.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/type2.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/Platform/Threading.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"
#include "System/creg/creg_cond.h"


CONFIG(bool, DebugGL).defaultValue(false).description("Enables GL debug-context and output. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);
CONFIG(int, FSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("Deprecated, set MSAALevel instead.");
CONFIG(int, MSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("Enables multisample anti-aliasing; 'level' is the number of samples used.");

CONFIG(int, ForceDisableShaders).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableClipCtrl).defaultValue(0).minimumValue(0).maximumValue(1);
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

CONFIG(bool, Fullscreen).defaultValue(true).headlessValue(false).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(bool, BlockCompositing).defaultValue(false).safemodeValue(true).description("Disables kwin compositing to fix tearing, possible fixes low FPS in windowed mode, too.");

CONFIG(int, XResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, XResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See XResolution, just for windowed.");
CONFIG(int, YResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See YResolution, just for windowed.");
CONFIG(int, WindowPosX).defaultValue(32).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");


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
	CR_MEMBER(glDebug),
	CR_MEMBER(glDebugErrors),

	CR_MEMBER(timeOffset),
	CR_MEMBER(lastFrameTime),
	CR_MEMBER(lastFrameStart),
	CR_MEMBER(weightedSpeedFactor),
	CR_MEMBER(drawFrame),
	CR_MEMBER(FPS),

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

	CR_IGNORED(msaaLevel),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(gpuMemorySize),
	CR_IGNORED(maxTexAnisoLvl),

	CR_IGNORED(active),
	CR_IGNORED(isVideoCapturing),
	CR_IGNORED(compressTextures),

	CR_IGNORED(haveATI),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(atiHacks),
	CR_IGNORED(supportNonPowerOfTwoTex),
	CR_IGNORED(supportTextureQueryLOD),
	CR_IGNORED(support24bitDepthBuffer),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(supportClipSpaceControl),
	CR_IGNORED(supportFragDepthLayout),
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
	CR_IGNORED(glContext)
))

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_notime)
	, weightedSpeedFactor(0.0f)
	, drawFrame(1)
	, FPS(1.0f)

	, screenSizeX(1)
	, screenSizeY(1)

	// window geometry
	, winPosX(configHandler->GetInt("WindowPosX"))
	, winPosY(configHandler->GetInt("WindowPosY"))
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

	// fallback
	, msaaLevel(std::max(configHandler->GetInt("MSAALevel"), configHandler->GetInt("FSAALevel")))
	, maxTextureSize(2048)
	, gpuMemorySize(0)
	, maxTexAnisoLvl(0.0f)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)

	, drawdebug(false)
	, drawdebugtraceray(false)
	, glDebug(false)
	, glDebugErrors(false)

	, teamNanospray(configHandler->GetBool("TeamNanoSpray"))
	, active(true)
	, isVideoCapturing(false)
	, compressTextures(false)
	, haveATI(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, atiHacks(false)
	, supportNonPowerOfTwoTex(false)
	, supportTextureQueryLOD(false)
	, support24bitDepthBuffer(false)
	, supportRestartPrimitive(false)
	, supportClipSpaceControl(false)
	, supportFragDepthLayout(false)
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
	, glContext(nullptr)
{
	verticalSync->WrapNotifyOnChange();
	configHandler->NotifyOnChange(this, {"TextureLODBias", "Fullscreen", "WindowBorderless"});
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
	verticalSync->WrapRemoveObserver();
	DestroyWindowAndContext();
}


bool CGlobalRendering::CreateSDLWindow(const int2& winRes, const int2& minRes, const char* title)
{
	const int aaLvls[] = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const int zbBits[] = {24, 32, 16};

	uint32_t sdlFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	// note:
	//   passing the minimized-flag is useless (state is not saved if minimized)
	//   and has no effect anyway, setting a minimum size for a window overrides
	//   it while disabling the SetWindowMinimumSize call still results in a 1x1
	//   window on the desktop
	sdlFlags |= (SDL_WINDOW_FULLSCREEN_DESKTOP * borderless + SDL_WINDOW_FULLSCREEN * (1 - borderless)) * fullScreen;
	sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless);

	for (size_t i = 0; i < (sizeof(aaLvls) / sizeof(aaLvls[0])) && (window == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0; j < (sizeof(zbBits) / sizeof(zbBits[0])) && (window == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((window = SDL_CreateWindow(title, winPosX, winPosY, winRes.x, winRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, "[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer", __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG("[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\")", __func__, aaLvls[i], zbBits[j], SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(window)));
		}
	}

	if (window == nullptr) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "[GR::%s] could not create SDL-window\n", __func__);
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

#if defined(WIN32)
	if (borderless && !fullScreen) {
		WindowManagerHelper::SetWindowResizable(window, !borderless);

		SDL_SetWindowBordered(window, borderless? SDL_FALSE: SDL_TRUE);
		SDL_SetWindowPosition(window, winPosX, winPosY);
		SDL_SetWindowSize(window, winRes.x, winRes.y);
	}
#endif

	SDL_SetWindowMinimumSize(window, minRes.x, minRes.y);
	return true;
}

bool CGlobalRendering::CreateGLContext(const int2& minCtx)
{
	assert(glContext == nullptr);

	constexpr int2 glCtxs[] = {{2, 0}, {2, 1},  {3, 0}, {3, 1}, {3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}};
	          int2 cmpCtx;

	if (std::find(&glCtxs[0], &glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)), minCtx) == (&glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)))) {
		handleerror(nullptr, "illegal OpenGL context-version specified, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	if ((glContext = SDL_GL_CreateContext(window)) != nullptr)
		return true;

	const char* frmts[] = {"[GR::%s] error (\"%s\") creating GL%d.%d %s-context", "[GR::%s] created GL%d.%d %s-context"};
	const char* profs[] = {"compatibility", "core"};

	char buf[1024] = {0};
	SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), minCtx.x, minCtx.y, profs[forceCoreContext]);

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

			if ((glContext = SDL_GL_CreateContext(window)) == nullptr) {
				LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			} else {
				// save the lowest successfully created fallback compatibility-context
				if (mask == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY && cmpCtx.x == 0 && tmpCtx.x >= minCtx.x)
					cmpCtx = tmpCtx;

				LOG_L(L_WARNING, frmts[true], __func__, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			}

			// accepts nullptr's
			SDL_GL_DeleteContext(glContext);
		}
	}

	if (cmpCtx.x == 0) {
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cmpCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cmpCtx.y);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	// should never fail at this point
	return ((glContext = SDL_GL_CreateContext(window)) != nullptr);
}

bool CGlobalRendering::CreateWindowAndContext(const char* title, bool hidden)
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		LOG_L(L_FATAL, "[GR::%s] error \"%s\" initializing SDL", __func__, SDL_GetError());
		return false;
	}

	if (!CheckAvailableVideoModes()) {
		handleerror(nullptr, "desktop color-depth should be at least 24 bits per pixel, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// should be set to "3.0" (non-core Mesa is stuck there), see below
	const char* mesaGL = getenv("MESA_GL_VERSION_OVERRIDE");
	const char* softGL = getenv("LIBGL_ALWAYS_SOFTWARE");

	// get wanted resolution and context-version
	const int2 winRes = GetCfgWinRes(fullScreen);
	const int2 maxRes = GetMaxWinRes();
	const int2 minRes = {minWinSizeX, minWinSizeY};
	const int2 minCtx = (mesaGL != nullptr && std::strlen(mesaGL) >= 3)?
		int2{                  std::max(mesaGL[0] - '0', 3),                   std::max(mesaGL[2] - '0', 0)}:
		int2{configHandler->GetInt("GLContextMajorVersion"), configHandler->GetInt("GLContextMinorVersion")};

	// start with the standard (R8G8B8A8 + 24-bit depth + 8-bit stencil + DB) format
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// create GL debug-context if wanted (more verbose GL messages, but runs slower)
	// note:
	//   requesting a core profile explicitly is needed to get versions later than
	//   3.0/1.30 for Mesa, other drivers return their *maximum* supported context
	//   in compat and do not make 3.0 itself available in core (though this still
	//   suffices for most of Spring)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, forceCoreContext? SDL_GL_CONTEXT_PROFILE_CORE: SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG * configHandler->GetBool("DebugGL"));

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);


	if (msaaLevel > 0) {
		if (softGL != nullptr)
			LOG_L(L_WARNING, "MSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		make_even_number(msaaLevel);
	}

	if (!CreateSDLWindow(winRes, minRes, title))
		return false;

	#if 0
	// do not use SDL_HideWindow since that also removes the taskbar entry
	if (minimized)
		SDL_MinimizeWindow(window);
	#else
	if (hidden)
		SDL_HideWindow(window);
	#endif
	// make extra sure the maximized-flag is set
	else if (winRes == maxRes)
		SDL_MaximizeWindow(window);

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

#if !defined(HEADLESS)
	// disable desktop compositing to fix tearing
	// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
	// On Windows Aero often uses vsync, and so when Spring runs windowed it will run with
	// vsync too, resulting in bad performance.
	if (configHandler->GetBool("BlockCompositing"))
		WindowManagerHelper::BlockCompositing(window);
#endif

	if (!CreateGLContext(minCtx))
		return false;

	if (!CheckGLContextVersion(minCtx)) {
		handleerror(nullptr, "minimum required OpenGL version not supported, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// redundant, but harmless
	SDL_GL_MakeCurrent(window, glContext);
	SDL_DisableScreenSaver();
	return true;
}


void CGlobalRendering::DestroyWindowAndContext() {
	WindowManagerHelper::SetIconSurface(window, nullptr);

	SDL_SetWindowGrab(window, SDL_FALSE);
	SDL_GL_MakeCurrent(window, nullptr);
	SDL_DestroyWindow(window);

#if !defined(HEADLESS)
	SDL_GL_DeleteContext(glContext);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
	SDL_EnableScreenSaver();
	SDL_Quit();

	window = nullptr;
	glContext = nullptr;
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
	ToggleGLDebugOutput(0, 0, 0);
}

void CGlobalRendering::SwapBuffers(bool allowSwapBuffers, bool clearErrors)
{
	SCOPED_TIMER("Misc::SwapBuffers");
	assert(window != nullptr);

	// silently or verbosely clear queue at the end of every frame
	if (clearErrors || glDebugErrors)
		glClearErrors("GR", __func__, glDebugErrors);

	if (!allowSwapBuffers && !forceSwapBuffers)
		return;

	const spring_time pre = spring_now();

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

	if (extMsg[0] == 0)
		return;

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

void CGlobalRendering::SetGLSupportFlags()
{
	const std::string& glVendor = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);

	haveARB   = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != nullptr);
	haveGLSL &= (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader);
	haveGLSL &= GLEW_VERSION_2_0; // we want OpenGL 2.0 core functions

	#ifndef HEADLESS
	if (!haveARB || !haveGLSL)
		throw unsupported_error("OpenGL shaders not supported, aborting");
	#endif

	// useful if a GPU claims to support GL4 and shaders but crashes (Intels...)
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
	supportNonPowerOfTwoTex = GLEW_ARB_texture_non_power_of_two && (!haveATI || (glRenderer.find(" x") == std::string::npos && glRenderer.find(" 9") == std::string::npos));
	supportTextureQueryLOD = GLEW_ARB_texture_query_lod;


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
	// default to off because it reduces quality, smallest mipmap level is bigger
	if (GLEW_ARB_texture_compression)
		compressTextures = configHandler->GetBool("CompressTextures");


	#ifdef GLEW_NV_primitive_restart
	supportRestartPrimitive = GLEW_NV_primitive_restart;
	#endif
	#ifdef GLEW_ARB_clip_control
	// use this only if we have a head-context; it came into existence with GL4.5
	supportClipSpaceControl |= GLEW_ARB_clip_control;
	supportClipSpaceControl &= (globalRenderingInfo.glContextVersion.x >= 4 && globalRenderingInfo.glContextVersion.y >= 5);
	supportClipSpaceControl &= (configHandler->GetInt("ForceDisableClipCtrl") == 0);
	#endif
	supportFragDepthLayout = (globalRenderingInfo.glContextVersion.x >= 4 && globalRenderingInfo.glContextVersion.y >= 2);


	// detect if GL_DEPTH_COMPONENT24 is supported (many ATIs don't;
	// they seem to support GL_DEPTH_COMPONENT24 for static textures
	// but those can't be rendered to)
	{
		#if 0
		GLint state = 0;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 16, 16, 0, GL_LUMINANCE, GL_FLOAT, nullptr);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &state);
		support24bitDepthBuffer = (state > 0);
		#else
		if (FBO::IsSupported() && !atiHacks) {
			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 16, 16);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT,  GL_DEPTH_COMPONENT24, 16, 16);
			support24bitDepthBuffer = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT);
			fbo.Unbind();
		}
		#endif
	}
}

void CGlobalRendering::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTexAnisoLvl);

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

	if (GetAvailableVideoRAM(vidMemBuffer, globalRenderingInfo.glVendor)) {
		const GLint totalMemMB = vidMemBuffer[0] / 1024;
		const GLint availMemMB = vidMemBuffer[1] / 1024;

		SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, gpuMemorySize = totalMemMB, availMemMB);
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
	LOG("\tARB shader support        : %i", haveARB);
	LOG("\tGLSL shader support       : %i", haveGLSL);
	LOG("\tFBO extension support     : %i", FBO::IsSupported());
	LOG("\tNPOT-texture support      : %i (%i)", supportNonPowerOfTwoTex, glewIsExtensionSupported("GL_ARB_texture_non_power_of_two"));
	LOG("\ttexture query-LOD support : %i (%i)", supportTextureQueryLOD, glewIsExtensionSupported("GL_ARB_texture_query_lod"));
	LOG("\t24-bit Z-buffer support   : %i (-)", support24bitDepthBuffer);
	LOG("\tprimitive-restart support : %i (%i)", supportRestartPrimitive, glewIsExtensionSupported("GL_NV_primitive_restart"));
	LOG("\tclip-space control support: %i (%i)", supportClipSpaceControl, glewIsExtensionSupported("GL_ARB_clip_control"));
	LOG("\tfrag-depth layout support : %i (-)", supportFragDepthLayout);
	LOG("\t");
	LOG("\tmax. FBO samples             : %i", FBO::GetMaxSamples());
	LOG("\tmax. texture size            : %i", maxTextureSize);
	LOG("\tmax. texture anisotropy level: %f", maxTexAnisoLvl);
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

	LOG("[GR::%s] display-mode set to %ix%ix%ibpp@%iHz (%s)", __func__, viewSizeX, viewSizeY, SDL_BITSPERPIXEL(dmode.format), dmode.refresh_rate, names[fs * 2 + bl]);
}


void CGlobalRendering::SetFullScreen(bool cliWindowed, bool cliFullScreen)
{
	const bool cfgFullScreen = configHandler->GetBool("Fullscreen");

	fullScreen = (cfgFullScreen && !cliWindowed  );
	fullScreen = (cfgFullScreen ||  cliFullScreen);

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

	const int2 res = GetCfgWinRes(fullScreen);

	SDL_SetWindowSize(window, res.x, res.y);
	SDL_SetWindowPosition(window, configHandler->GetInt("WindowPosX"), configHandler->GetInt("WindowPosY"));
	SDL_SetWindowFullscreen(window, (borderless? SDL_WINDOW_FULLSCREEN_DESKTOP: SDL_WINDOW_FULLSCREEN) * fullScreen);
	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);
	WindowManagerHelper::SetWindowResizable(window, !borderless && !fullScreen);
	// set size again in an attempt to fix some bugs
	SDL_SetWindowSize(window, res.x, res.y);
}


int2 CGlobalRendering::GetMaxWinRes() const {
	SDL_DisplayMode dmode;
	SDL_GetDesktopDisplayMode(0, &dmode);
	return {dmode.w, dmode.h};
}

int2 CGlobalRendering::GetCfgWinRes(bool fullScrn) const
{
	static const char* xsKeys[2] = {"XResolutionWindowed", "XResolution"};
	static const char* ysKeys[2] = {"YResolutionWindowed", "YResolution"};

	int2 res = {configHandler->GetInt(xsKeys[fullScrn]), configHandler->GetInt(ysKeys[fullScrn])};

	// copy Native Desktop Resolution if user did not specify a value
	// SDL2 can do this itself if size{X,Y} are set to zero but fails
	// with Display Cloning and similar, causing DVI monitors to only
	// run at (e.g.) 640x400 and HDMI devices at full-HD
	// TODO: make screen configurable?
	if (res.x <= 0 || res.y <= 0)
		res = GetMaxWinRes();

	// limit minimum window size in windowed mode
	res.x = std::max(res.x, minWinSizeX * (1 - fullScrn));
	res.y = std::max(res.y, minWinSizeY * (1 - fullScrn));
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
	viewSizeX = winSizeX >> (1 * dualScreenMode);
	viewSizeY = winSizeY;

	viewPosX = (winSizeX >> 1) * dualScreenMode * dualScreenMiniMapOnLeft;
	viewPosY = 0;
}

void CGlobalRendering::UpdatePixelGeometry()
{
	pixelX = 1.0f / viewSizeX;
	pixelY = 1.0f / viewSizeY;

	aspectRatio = viewSizeX / float(viewSizeY);
}


void CGlobalRendering::ReadWindowPosAndSize()
{
#ifdef HEADLESS
	screenSizeX = 8;
	screenSizeY = 8;
	winSizeX = 8;
	winSizeY = 8;
	winPosX = 0;
	winPosY = 0;

#else

	SDL_Rect screenSize;
	SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &screenSize);

	// no other good place to set these
	screenSizeX = screenSize.w;
	screenSizeY = screenSize.h;

	SDL_GetWindowSize(window, &winSizeX, &winSizeY);
	SDL_GetWindowPosition(window, &winPosX, &winPosY);
#endif

	// should be done by caller
	// UpdateViewPortGeometry();
}

void CGlobalRendering::SaveWindowPosAndSize()
{
#ifdef HEADLESS
	return;
#endif

	if (fullScreen)
		return;

	// do not save if minimized
	// note that maximized windows are automagically restored; SDL2
	// apparently detects if the resolution is maximal and sets the
	// flag (but we also check if winRes equals maxRes to be safe)
	if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
		return;

	configHandler->Set("WindowPosX", winPosX);
	configHandler->Set("WindowPosY", winPosY);
	configHandler->Set("XResolutionWindowed", winSizeX);
	configHandler->Set("YResolutionWindowed", winSizeY);
}


void CGlobalRendering::UpdateGLConfigs()
{
	// re-read configuration value
	verticalSync->SetInterval();

	const float lodBias = configHandler->GetFloat("TextureLODBias");
	const float absBias = math::fabs(lodBias);

	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, lodBias * (absBias > 0.01f));
}

void CGlobalRendering::UpdateGLGeometry()
{
	ReadWindowPosAndSize();
	SetDualScreenParams();
	UpdateViewPortGeometry();
	UpdatePixelGeometry();
}

void CGlobalRendering::InitGLState()
{
	glShadeModel(GL_SMOOTH);

	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	#ifdef GLEW_ARB_clip_control
	// avoid precision loss with default DR transform
	if (supportClipSpaceControl)
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	#endif

	// MSAA rasterization
	if ((msaaLevel *= CheckGLMultiSampling()) != 0) {
		glEnable(GL_MULTISAMPLE);
	} else {
		glDisable(GL_MULTISAMPLE);
	}

	// FFP model lighting
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(viewPosX, viewPosY, viewSizeX, viewSizeY);
	gluPerspective(45.0f, aspectRatio, 2.8f, MAX_VIEW_RANGE);

	SwapBuffers(true, true);
	LogDisplayMode();
}


/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
bool CGlobalRendering::CheckGLMultiSampling() const
{
	if (msaaLevel == 0)
		return false;
	if (!GLEW_ARB_multisample)
		return false;

	GLint buffers = 0;
	GLint samples = 0;

	glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
	glGetIntegerv(GL_SAMPLES, &samples);

	return (buffers != 0 && samples != 0);
}

bool CGlobalRendering::CheckGLContextVersion(const int2& minCtx) const
{
	#ifdef HEADLESS
	return true;
	#else
	int2 tmpCtx = {0, 0};

	glGetIntegerv(GL_MAJOR_VERSION, &tmpCtx.x);
	glGetIntegerv(GL_MINOR_VERSION, &tmpCtx.y);

	// keep this for convenience
	globalRenderingInfo.glContextVersion = tmpCtx;

	// compare major * 10 + minor s.t. 4.1 evaluates as larger than 3.2
	return ((tmpCtx.x * 10 + tmpCtx.y) >= (minCtx.x * 10 + minCtx.y));
	#endif
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


#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))

#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API                GL_DEBUG_SOURCE_API_ARB
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM      GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB
#define GL_DEBUG_SOURCE_SHADER_COMPILER    GL_DEBUG_SOURCE_SHADER_COMPILER_ARB
#define GL_DEBUG_SOURCE_THIRD_PARTY        GL_DEBUG_SOURCE_THIRD_PARTY_ARB
#define GL_DEBUG_SOURCE_APPLICATION        GL_DEBUG_SOURCE_APPLICATION_ARB
#define GL_DEBUG_SOURCE_OTHER              GL_DEBUG_SOURCE_OTHER_ARB

#define GL_DEBUG_TYPE_ERROR                GL_DEBUG_TYPE_ERROR_ARB
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR  GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR   GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_PORTABILITY          GL_DEBUG_TYPE_PORTABILITY_ARB
#define GL_DEBUG_TYPE_PERFORMANCE          GL_DEBUG_TYPE_PERFORMANCE_ARB
#if (defined(GL_DEBUG_TYPE_MARKER_ARB) && defined(GL_DEBUG_TYPE_PUSH_GROUP_ARB) && defined(GL_DEBUG_TYPE_POP_GROUP_ARB))
#define GL_DEBUG_TYPE_MARKER               GL_DEBUG_TYPE_MARKER_ARB
#define GL_DEBUG_TYPE_PUSH_GROUP           GL_DEBUG_TYPE_PUSH_GROUP_ARB
#define GL_DEBUG_TYPE_POP_GROUP            GL_DEBUG_TYPE_POP_GROUP_ARB
#else
#define GL_DEBUG_TYPE_MARKER               -1u
#define GL_DEBUG_TYPE_PUSH_GROUP           -2u
#define GL_DEBUG_TYPE_POP_GROUP            -3u
#endif
#define GL_DEBUG_TYPE_OTHER                GL_DEBUG_TYPE_OTHER_ARB

#define GL_DEBUG_SEVERITY_HIGH             GL_DEBUG_SEVERITY_HIGH_ARB
#define GL_DEBUG_SEVERITY_MEDIUM           GL_DEBUG_SEVERITY_MEDIUM_ARB
#define GL_DEBUG_SEVERITY_LOW              GL_DEBUG_SEVERITY_LOW_ARB

#define GL_DEBUG_OUTPUT_SYNCHRONOUS        GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB
#define GLDEBUGPROC                        GLDEBUGPROCARB
#endif

#ifndef glDebugMessageCallback
#define glDebugMessageCallback  glDebugMessageCallbackARB
#define glDebugMessageControl   glDebugMessageControlARB
#endif

constexpr static std::array<GLenum,  7> msgSrceEnums = {GL_DONT_CARE, GL_DEBUG_SOURCE_API, GL_DEBUG_SOURCE_WINDOW_SYSTEM, GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_SOURCE_OTHER};
constexpr static std::array<GLenum, 10> msgTypeEnums = {GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DEBUG_TYPE_PORTABILITY, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_TYPE_MARKER, GL_DEBUG_TYPE_PUSH_GROUP, GL_DEBUG_TYPE_POP_GROUP, GL_DEBUG_TYPE_OTHER};
constexpr static std::array<GLenum,  4> msgSevrEnums = {GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, GL_DEBUG_SEVERITY_MEDIUM, GL_DEBUG_SEVERITY_HIGH};

static inline const char* glDebugMessageSourceName(GLenum msgSrce) {
	switch (msgSrce) {
		case GL_DEBUG_SOURCE_API            : return             "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM  : return   "WINDOW_SYSTEM"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER_COMPILER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY    : return     "THIRD_PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION    : return     "APPLICATION"; break;
		case GL_DEBUG_SOURCE_OTHER          : return           "OTHER"; break;
		case GL_DONT_CARE                   : return       "DONT_CARE"; break;
		default                             :                         ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageTypeName(GLenum msgType) {
	switch (msgType) {
		case GL_DEBUG_TYPE_ERROR              : return       "ERROR"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return  "DEPRECATED"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : return   "UNDEFINED"; break;
		case GL_DEBUG_TYPE_PORTABILITY        : return "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE        : return "PERFORMANCE"; break;
		case GL_DEBUG_TYPE_MARKER             : return      "MARKER"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP         : return  "PUSH_GROUP"; break;
		case GL_DEBUG_TYPE_POP_GROUP          : return   "POP_GROUP"; break;
		case GL_DEBUG_TYPE_OTHER              : return       "OTHER"; break;
		case GL_DONT_CARE                     : return   "DONT_CARE"; break;
		default                               :                     ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageSeverityName(GLenum msgSevr) {
	switch (msgSevr) {
		case GL_DEBUG_SEVERITY_HIGH  : return      "HIGH"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: return    "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_LOW   : return       "LOW"; break;
		case GL_DONT_CARE            : return "DONT_CARE"; break;
		default                      :                   ; break;
	}

	return "UNKNOWN";
}

static void _GL_APIENTRY glDebugMessageCallbackFunc(
	GLenum msgSrce,
	GLenum msgType,
	GLuint msgID,
	GLenum msgSevr,
	GLsizei length,
	const GLchar* dbgMessage,
	const GLvoid* userParam
) {
	switch (msgID) {
		case 131185: { return; } break; // "Buffer detailed info: Buffer object 260 (bound to GL_PIXEL_UNPACK_BUFFER_ARB, usage hint is GL_STREAM_DRAW) has been mapped in DMA CACHED memory."
		default: {} break;
	}

	const char* msgSrceStr = glDebugMessageSourceName(msgSrce);
	const char* msgTypeStr = glDebugMessageTypeName(msgType);
	const char* msgSevrStr = glDebugMessageSeverityName(msgSevr);

	LOG_L(L_WARNING, "[OPENGL_DEBUG] id=%u source=%s type=%s severity=%s msg=\"%s\"", msgID, msgSrceStr, msgTypeStr, msgSevrStr, dbgMessage);

	if ((userParam == nullptr) || !(*reinterpret_cast<const bool*>(userParam)))
		return;

	CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
}
#endif


bool CGlobalRendering::ToggleGLDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx)
{
	const static bool dbgOutput = configHandler->GetBool("DebugGL");
	const static bool dbgTraces = configHandler->GetBool("DebugGLStacktraces");

	if (!dbgOutput) {
		LOG("[GR::%s] OpenGL debug-context not installed (dbgErrors=%d dbgTraces=%d)", __func__, glDebugErrors, dbgTraces);
		return false;
	}

	#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	if ((glDebug = !glDebug)) {
		const char* msgSrceStr = glDebugMessageSourceName  (msgSrceEnums[msgSrceIdx %= msgSrceEnums.size()]);
		const char* msgTypeStr = glDebugMessageTypeName    (msgTypeEnums[msgTypeIdx %= msgTypeEnums.size()]);
		const char* msgSevrStr = glDebugMessageSeverityName(msgSevrEnums[msgSevrIdx %= msgSevrEnums.size()]);

		// install OpenGL debug message callback; typecast is a workaround
		// for #4510 (change in callback function signature with GLEW 1.11)
		// use SYNCHRONOUS output, we want our callback to run in the same
		// thread as the bugged GL call (for proper stacktraces)
		// CB userParam is const, but has to be specified sans qualifiers
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback((GLDEBUGPROC) &glDebugMessageCallbackFunc, (void*) &dbgTraces);
		glDebugMessageControl(msgSrceEnums[msgSrceIdx], msgTypeEnums[msgTypeIdx], msgSevrEnums[msgSevrIdx], 0, nullptr, GL_TRUE);

		LOG("[GR::%s] OpenGL debug-message callback enabled (source=%s type=%s severity=%s)", __func__, msgSrceStr, msgTypeStr, msgSevrStr);
	} else {
		glDebugMessageCallback(nullptr, nullptr);
		glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		LOG("[GR::%s] OpenGL debug-message callback disabled", __func__);
	}
	#endif

	return true;
}
