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

CONFIG(int, ForceDisableClipCtrl).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceCoreContext).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceSwapBuffers).defaultValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with ATI video cards.\n -1:=runtime detect, 0:=off, 1:=on");

// enabled in safemode, far more likely the gpu runs out of memory than this extension causes crashes!
// (otherwise defaults to off because it reduces mipmap quality, smallest compressed level is bigger)
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

	CR_IGNORED(forceCoreContext),
	CR_IGNORED(forceSwapBuffers),

	CR_IGNORED(msaaLevel),
	CR_IGNORED(maxTextureSize),
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

	CR_IGNORED(sdlWindows),
	CR_IGNORED(glContexts)
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

	, forceCoreContext(configHandler->GetInt("ForceCoreContext"))
	, forceSwapBuffers(configHandler->GetInt("ForceSwapBuffers"))

	// fallback
	, msaaLevel(std::max(configHandler->GetInt("MSAALevel"), configHandler->GetInt("FSAALevel")))
	, maxTextureSize(2048)
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
	, sdlWindows{nullptr, nullptr}
	, glContexts{nullptr, nullptr}
{
	verticalSync->WrapNotifyOnChange();
	configHandler->NotifyOnChange(this, {"TextureLODBias", "Fullscreen", "WindowBorderless"});
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
	verticalSync->WrapRemoveObserver();
	DestroyWindowAndContext(sdlWindows[0], glContexts[0]);
	DestroyWindowAndContext(sdlWindows[1], glContexts[1]);
	KillSDL();

	sdlWindows[0] = nullptr;
	sdlWindows[1] = nullptr;
	glContexts[0] = nullptr;
	glContexts[1] = nullptr;
}


SDL_Window* CGlobalRendering::CreateSDLWindow(const int2& winRes, const int2& minRes, const char* title, bool hidden) const
{
	SDL_Window* newWindow = nullptr;

	const int aaLvls[] = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const int zbBits[] = {24, 32, 16};

	uint32_t sdlFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	const char* winName = hidden? "hidden": "main";
	const char* wpfName = "";

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for %s window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for %s window",
	};

	// note:
	//   passing the minimized-flag is useless (state is not saved if minimized)
	//   and has no effect anyway, setting a minimum size for a window overrides
	//   it while disabling the SetWindowMinimumSize call still results in a 1x1
	//   window on the desktop
	sdlFlags |= (SDL_WINDOW_FULLSCREEN_DESKTOP * borderless + SDL_WINDOW_FULLSCREEN * (1 - borderless)) * fullScreen;
	sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless);
	sdlFlags |= (SDL_WINDOW_HIDDEN * hidden);

	for (size_t i = 0; i < (sizeof(aaLvls) / sizeof(aaLvls[0])) && (newWindow == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0; j < (sizeof(zbBits) / sizeof(zbBits[0])) && (newWindow == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((newWindow = SDL_CreateWindow(title, winPosX, winPosY, winRes.x, winRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, frmts[0], __func__, SDL_GetError(), aaLvls[i], zbBits[j], winName);
				continue;
			}

			LOG(frmts[1], __func__, aaLvls[i], zbBits[j], wpfName = SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(newWindow)), winName);
		}
	}

	if (newWindow == nullptr) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "[GR::%s] could not create (hidden=%d) SDL-window\n", __func__, hidden);
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

#if defined(WIN32)
	if (borderless && !fullScreen) {
		WindowManagerHelper::SetWindowResizable(newWindow, !borderless);

		SDL_SetWindowBordered(newWindow, borderless? SDL_FALSE: SDL_TRUE);
		SDL_SetWindowPosition(newWindow, winPosX, winPosY);
		SDL_SetWindowSize(newWindow, winRes.x, winRes.y);
	}
#endif

	SDL_SetWindowMinimumSize(newWindow, minRes.x, minRes.y);
	return newWindow;
}

SDL_GLContext CGlobalRendering::CreateGLContext(const int2& minCtx, SDL_Window* targetWindow) const
{
	SDL_GLContext newContext = nullptr;

	constexpr int2 glCtxs[] = {{2, 0}, {2, 1},  {3, 0}, {3, 1}, {3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}};
	          int2 cmpCtx;

	if (std::find(&glCtxs[0], &glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)), minCtx) == (&glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)))) {
		handleerror(nullptr, "illegal OpenGL context-version specified, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	if ((newContext = SDL_GL_CreateContext(targetWindow)) != nullptr)
		return newContext;

	const char* winName = (targetWindow == sdlWindows[1])? "hidden": "main";

	const char* frmts[] = {"[GR::%s] error (\"%s\") creating %s GL%d.%d %s-context", "[GR::%s] created %s GL%d.%d %s-context"};
	const char* profs[] = {"compatibility", "core"};

	char buf[1024] = {0};
	SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), winName, minCtx.x, minCtx.y, profs[forceCoreContext]);

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

			if ((newContext = SDL_GL_CreateContext(targetWindow)) == nullptr) {
				LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), winName, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			} else {
				// save the lowest successfully created fallback compatibility-context
				if (mask == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY && cmpCtx.x == 0 && tmpCtx.x >= minCtx.x)
					cmpCtx = tmpCtx;

				LOG_L(L_WARNING, frmts[true], __func__, winName, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			}

			// accepts nullptr's
			SDL_GL_DeleteContext(newContext);
		}
	}

	if (cmpCtx.x == 0) {
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cmpCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cmpCtx.y);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	// should never fail at this point
	return (newContext = SDL_GL_CreateContext(targetWindow));
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

	if ((sdlWindows[0] = CreateSDLWindow(winRes, minRes, title, false)) == nullptr)
		return false;

	if ((sdlWindows[1] = CreateSDLWindow(winRes, minRes, title, true)) == nullptr)
		return false;

	#if 0
	// do not use SDL_HideWindow since that also removes the taskbar entry
	if (minimized)
		SDL_MinimizeWindow(sdlWindows[0]);
	#else
	if (hidden)
		SDL_HideWindow(sdlWindows[0]);
	#endif
	// make extra sure the maximized-flag is set
	else if (winRes == maxRes)
		SDL_MaximizeWindow(sdlWindows[0]);

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

#if !defined(HEADLESS)
	// disable desktop compositing to fix tearing
	// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
	// On Windows Aero often uses vsync, and so when Spring runs windowed it will run with
	// vsync too, resulting in bad performance.
	if (configHandler->GetBool("BlockCompositing"))
		WindowManagerHelper::BlockCompositing(sdlWindows[0]);
#endif

	if ((glContexts[0] = CreateGLContext(minCtx, sdlWindows[0])) == nullptr)
		return false;
	if ((glContexts[1] = CreateGLContext(minCtx, sdlWindows[1])) == nullptr)
		return false;

	if (!CheckGLContextVersion(minCtx)) {
		handleerror(nullptr, "minimum required OpenGL version not supported, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// redundant, but harmless
	SDL_GL_MakeCurrent(sdlWindows[0], glContexts[0]);
	SDL_DisableScreenSaver();
	return true;
}


void CGlobalRendering::MakeCurrentContext(bool hidden, bool secondary, bool clear) {
	if (clear) {
		SDL_GL_MakeCurrent(sdlWindows[hidden], nullptr);
	} else {
		SDL_GL_MakeCurrent(sdlWindows[hidden], glContexts[secondary]);
	}
}


void CGlobalRendering::DestroyWindowAndContext(SDL_Window* window, SDL_GLContext context) {
	if (window == sdlWindows[0]) {
		WindowManagerHelper::SetIconSurface(window, nullptr);
		SDL_SetWindowGrab(window, SDL_FALSE);
	}

	SDL_GL_MakeCurrent(window, nullptr);
	SDL_DestroyWindow(window);

	#if !defined(HEADLESS)
	SDL_GL_DeleteContext(context);
	#endif
}

void CGlobalRendering::KillSDL() const {
	#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	#endif

	SDL_EnableScreenSaver();
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

	if (!CheckGLEWContextVersion(globalRenderingInfo.glContextVersion))
		throw (unsupported_error("GLEW version outdated, aborting"));

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
	assert(sdlWindows[0] != nullptr);

	// silently or verbosely clear queue at the end of every frame
	if (clearErrors || glDebugErrors)
		glClearErrors("GR", __func__, glDebugErrors);

	if (!allowSwapBuffers && !forceSwapBuffers)
		return;

	const spring_time pre = spring_now();

	SDL_GL_SwapWindow(sdlWindows[0]);
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
}


void CGlobalRendering::CheckGLExtensions() const
{
	const auto CheckExt = [](const char* extName, const bool extFlag) {
		if (extFlag)
			return;

		char buf[512];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "OpenGL extension \"%s\" not supported, aborting", extName);
		throw (unsupported_error(buf));
	};

	#ifndef HEADLESS
	#define CHECK_EXT(ext) CheckExt(#ext, ext)

	CHECK_EXT(GLEW_ARB_vertex_buffer_object); // 1.5 (VBO)
	CHECK_EXT(GLEW_ARB_pixel_buffer_object); // 2.1 (PBO)
	CHECK_EXT(GLEW_ARB_framebuffer_object); // 3.0 (FBO)
	CHECK_EXT(GLEW_ARB_vertex_array_object); // 3.0 (VAO; core in 4.x)
	CHECK_EXT(GLEW_ARB_uniform_buffer_object); // 3.1 (UBO)

	#ifdef GLEW_ARB_buffer_storage
	CHECK_EXT(GLEW_ARB_buffer_storage); // 4.4 (immutable storage)
	#else
	CheckExt("GLEW_ARB_buffer_storage", false);
	#endif

	CHECK_EXT(GLEW_ARB_copy_buffer); // 3.1 (glCopyBufferSubData)
	CHECK_EXT(GLEW_ARB_map_buffer_range); // 3.0 (glMapBufferRange[ARB])
	CHECK_EXT(GLEW_EXT_framebuffer_multisample); // 3.0 (multi-sampled FB's)
	CHECK_EXT(GLEW_EXT_framebuffer_blit); // 3.0 (glBlitFramebuffer[EXT])

	// not yet mandatory
	// CHECK_EXT(GLEW_ARB_multi_bind); // 4.4
	// CHECK_EXT(GLEW_ARB_texture_storage); // 4.2
	// CHECK_EXT(GLEW_ARB_program_interface_query); // 4.3
	// CHECK_EXT(GLEW_EXT_direct_state_access); // 3.3 (core in 4.5)
	// CHECK_EXT(GLEW_ARB_invalidate_subdata); // 4.3 (glInvalidateBufferData)
	// CHECK_EXT(GLEW_ARB_shader_storage_buffer_object); // 4.3 (glShaderStorageBlockBinding)
	CHECK_EXT(GLEW_ARB_get_program_binary); // 4.1

	CHECK_EXT(GLEW_ARB_texture_compression);
	CHECK_EXT(GLEW_EXT_texture_compression_s3tc);
	CHECK_EXT(GLEW_EXT_texture_compression_dxt1);
	CHECK_EXT(GLEW_ARB_texture_float); // 3.0 (FP{16,32} textures)
	CHECK_EXT(GLEW_ARB_texture_non_power_of_two); // 2.0 (NPOT textures)
	CHECK_EXT(GLEW_ARB_texture_rectangle); // 3.0 (rectangular textures)
	CHECK_EXT(GLEW_EXT_texture_filter_anisotropic); // 3.3 (AF; core in 4.6!)
	CHECK_EXT(GLEW_ARB_imaging); // 1.2 (imaging subset; texture_*_clamp [GL_CLAMP_TO_EDGE] etc)
	CHECK_EXT(GLEW_EXT_texture_edge_clamp); // 1.2
	CHECK_EXT(GLEW_ARB_texture_border_clamp); // 1.3

	CHECK_EXT(GLEW_EXT_blend_func_separate); // 1.4
	CHECK_EXT(GLEW_EXT_blend_equation_separate); // 2.0
	CHECK_EXT(GLEW_EXT_stencil_two_side); // 2.0

	CHECK_EXT(GLEW_ARB_occlusion_query); // 1.5
	CHECK_EXT(GLEW_ARB_occlusion_query2); // 3.3 (glBeginConditionalRender)

	CHECK_EXT(GLEW_ARB_depth_clamp); // 3.2

	CHECK_EXT(GLEW_NV_primitive_restart); // 3.1
	CHECK_EXT(GLEW_ARB_transform_feedback3); // 4.0 (VTF v3)
	CHECK_EXT(GLEW_ARB_explicit_attrib_location); // 3.3

	CHECK_EXT(GLEW_ARB_vertex_program); // VS-ARB
	CHECK_EXT(GLEW_ARB_fragment_program); // FS-ARB
	CHECK_EXT(GLEW_ARB_shading_language_100); // 2.0
	CHECK_EXT(GLEW_ARB_vertex_shader); // 1.5 (VS-GLSL; core in 2.0)
	CHECK_EXT(GLEW_ARB_fragment_shader); // 1.5 (FS-GLSL; core in 2.0)
	CHECK_EXT(GLEW_ARB_geometry_shader4); // GS v4 (GL3.2)

	#undef CHECK_EXT
	#else
	// for HL builds, all used GL functions are stubs
	#endif
}

void CGlobalRendering::SetGLSupportFlags()
{
	const std::string& glVendor   = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);

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

	// apply runtime texture compression for glBuildMipmaps?
	// glCompressedTex*Image* must additionally be supported
	// for DDS (SMF DXT1, etc)
	compressTextures = configHandler->GetBool("CompressTextures");


	#ifdef GLEW_NV_primitive_restart
	// not defined for headless builds
	supportRestartPrimitive = GLEW_NV_primitive_restart;
	#endif
	#ifdef GLEW_ARB_clip_control
	supportClipSpaceControl = GLEW_ARB_clip_control;
	#endif
	// CC did not exist as an extension before GL4.5, too recent to enforce
	supportClipSpaceControl &= (globalRenderingInfo.glContextVersion.x >= 4 && globalRenderingInfo.glContextVersion.y >= 5);
	supportClipSpaceControl &= (configHandler->GetInt("ForceDisableClipCtrl") == 0);

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
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA8, 16, 16);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT,  GL_DEPTH_COMPONENT24, 16, 16);
			support24bitDepthBuffer = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE);
			fbo.Unbind();
		}
		#endif
	}
}

void CGlobalRendering::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
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
	auto& grInfo = globalRenderingInfo;

	auto& sdlVC = grInfo.sdlVersionCompiled;
	auto& sdlVL = grInfo.sdlVersionLinked;

	SDL_VERSION(&sdlVC);
	SDL_GetVersion(&sdlVL);

	if ((grInfo.glVersion   = (const char*) glGetString(GL_VERSION                 )) == nullptr) grInfo.glVersion   = "unknown";
	if ((grInfo.glVendor    = (const char*) glGetString(GL_VENDOR                  )) == nullptr) grInfo.glVendor    = "unknown";
	if ((grInfo.glRenderer  = (const char*) glGetString(GL_RENDERER                )) == nullptr) grInfo.glRenderer  = "unknown";
	if ((grInfo.glslVersion = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION)) == nullptr) grInfo.glslVersion = "unknown";
	if ((grInfo.glewVersion = (const char*) glewGetString(GLEW_VERSION             )) == nullptr) grInfo.glewVersion = "unknown";

	// should never be null with any driver, no harm in an extra check
	// (absence of GLSL version string would indicate bigger problems)
	if (std::strcmp(globalRenderingInfo.glslVersion, "unknown") == 0)
		throw unsupported_error("OpenGL shaders not supported, aborting");

	if (!ShowDriverWarning(grInfo.glVendor, grInfo.glRenderer))
		throw unsupported_error("OpenGL drivers not installed, aborting");

	memset(grInfo.glVersionShort, 0, sizeof(grInfo.glVersionShort));
	memset(grInfo.glslVersionShort, 0, sizeof(grInfo.glslVersionShort));

	constexpr const char* sdlFmtStr = "%d.%d.%d (linked) / %d.%d.%d (compiled)";
	constexpr const char* memFmtStr = "%iMB (total) / %iMB (available)";

	SNPRINTF(sdlVersionStr, sizeof(sdlVersionStr), sdlFmtStr,
		sdlVL.major, sdlVL.minor, sdlVL.patch,
		sdlVC.major, sdlVC.minor, sdlVC.patch
	);

	if (!GetAvailableVideoRAM(&grInfo.gpuMemorySize.x, grInfo.glVendor))
		return;

	const GLint totalMemMB = grInfo.gpuMemorySize.x / 1024;
	const GLint availMemMB = grInfo.gpuMemorySize.y / 1024;

	SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, totalMemMB, availMemMB);
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
	LOG("\tSDL swap-int: %d", SDL_GL_GetSwapInterval());
	LOG("\t");
	LOG("\tFBO extension support     : %i", FBO::IsSupported());
	LOG("\tNVX GPU mem-info support  : %i", glewIsExtensionSupported("GL_NVX_gpu_memory_info"));
	LOG("\tATI GPU mem-info support  : %i", glewIsExtensionSupported("GL_ATI_meminfo"));
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

void CGlobalRendering::LogDisplayMode(SDL_Window* window) const
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


void CGlobalRendering::SetWindowTitle(const std::string& title)
{
	SDL_SetWindowTitle(sdlWindows[0], title.c_str());
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	if (sdlWindows[0] == nullptr)
		return;

	if (key == "TextureLODBias") {
		UpdateGLConfigs();
		return;
	}

	borderless = configHandler->GetBool("WindowBorderless");
	fullScreen = configHandler->GetBool("Fullscreen");

	const int2 res = GetCfgWinRes(fullScreen);

	SDL_SetWindowSize(sdlWindows[0], res.x, res.y);
	SDL_SetWindowPosition(sdlWindows[0], configHandler->GetInt("WindowPosX"), configHandler->GetInt("WindowPosY"));
	SDL_SetWindowFullscreen(sdlWindows[0], (borderless? SDL_WINDOW_FULLSCREEN_DESKTOP: SDL_WINDOW_FULLSCREEN) * fullScreen);
	SDL_SetWindowBordered(sdlWindows[0], borderless ? SDL_FALSE : SDL_TRUE);
	WindowManagerHelper::SetWindowResizable(sdlWindows[0], !borderless && !fullScreen);
	// set size again in an attempt to fix some bugs
	SDL_SetWindowSize(sdlWindows[0], res.x, res.y);
}


bool CGlobalRendering::SetWindowInputGrabbing(bool enable)
{
	if (enable) {
		SDL_SetWindowGrab(sdlWindows[0], SDL_TRUE);
		return true;
	}

	SDL_SetWindowGrab(sdlWindows[0], SDL_FALSE);
	return false;
}

bool CGlobalRendering::ToggleWindowInputGrabbing()
{
	if (SDL_GetWindowGrab(sdlWindows[0]))
		return (SetWindowInputGrabbing(false));

	return (SetWindowInputGrabbing(true));
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


void CGlobalRendering::SetFullScreen(bool cliWindowed, bool cliFullScreen)
{
	const bool cfgFullScreen = configHandler->GetBool("Fullscreen");

	fullScreen = (cfgFullScreen && !cliWindowed  );
	fullScreen = (cfgFullScreen ||  cliFullScreen);

	configHandler->Set("Fullscreen", fullScreen);
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
	SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(sdlWindows[0]), &screenSize);

	// no other good place to set these
	screenSizeX = screenSize.w;
	screenSizeY = screenSize.h;

	SDL_GetWindowSize(sdlWindows[0], &winSizeX, &winSizeY);
	SDL_GetWindowPosition(sdlWindows[0], &winPosX, &winPosY);
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
	if ((SDL_GetWindowFlags(sdlWindows[0]) & SDL_WINDOW_MINIMIZED) != 0)
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
	// temporarily left for ShadowHandler
	glShadeModel(GL_FLAT);

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

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(viewPosX, viewPosY, viewSizeX, viewSizeY);
	gluPerspective(45.0f, aspectRatio, 2.8f, MAX_VIEW_RANGE);

	SwapBuffers(true, true);
	LogDisplayMode(sdlWindows[0]);
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

bool CGlobalRendering::CheckGLEWContextVersion(const int2& curCtx) const
{
	#ifdef HEADLESS
	return true;
	#else
	int2 tmpCtx = {0, 0};

	// GLEW should know this GL version exists
	switch (curCtx.x * 10 + curCtx.y) {
		#ifdef GLEW_VERSION_3_0
		case 30: { tmpCtx = curCtx * GLEW_VERSION_3_0; } break;
		#endif
		#ifdef GLEW_VERSION_3_1
		case 31: { tmpCtx = curCtx * GLEW_VERSION_3_1; } break;
		#endif
		#ifdef GLEW_VERSION_3_2
		case 32: { tmpCtx = curCtx * GLEW_VERSION_3_2; } break;
		#endif
		#ifdef GLEW_VERSION_3_3
		case 33: { tmpCtx = curCtx * GLEW_VERSION_3_3; } break;
		#endif
		#ifdef GLEW_VERSION_4_0
		case 40: { tmpCtx = curCtx * GLEW_VERSION_4_0; } break;
		#endif
		#ifdef GLEW_VERSION_4_1
		case 41: { tmpCtx = curCtx * GLEW_VERSION_4_1; } break;
		#endif
		#ifdef GLEW_VERSION_4_2
		case 42: { tmpCtx = curCtx * GLEW_VERSION_4_2; } break;
		#endif
		#ifdef GLEW_VERSION_4_3
		case 43: { tmpCtx = curCtx * GLEW_VERSION_4_3; } break;
		#endif
		#ifdef GLEW_VERSION_4_4
		case 44: { tmpCtx = curCtx * GLEW_VERSION_4_4; } break;
		#endif
		#ifdef GLEW_VERSION_4_5
		case 45: { tmpCtx = curCtx * GLEW_VERSION_4_5; } break;
		#endif
		#ifdef GLEW_VERSION_4_6
		case 46: { tmpCtx = curCtx * GLEW_VERSION_4_6; } break;
		#endif
		default: {} break;
	}

	return ((tmpCtx.x * 10 + tmpCtx.y) >= (curCtx.x * 10 + curCtx.y));
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
		case 131169: { return; } break; // "Framebuffer detailed info: The driver allocated storage for renderbuffer N."
		case 131185: { return; } break; // "Buffer detailed info: Buffer object 260 (bound to GL_PIXEL_UNPACK_BUFFER_ARB, usage hint is GL_STREAM_DRAW) has been mapped in DMA CACHED memory."
		default: {} break;
	}

	const char* msgSrceStr = glDebugMessageSourceName(msgSrce);
	const char* msgTypeStr = glDebugMessageTypeName(msgType);
	const char* msgSevrStr = glDebugMessageSeverityName(msgSevr);

	LOG_L(L_WARNING, "[OPENGL_DEBUG] id=%u source=%s type=%s severity=%s msg=\"%s\"", msgID, msgSrceStr, msgTypeStr, msgSevrStr, dbgMessage);

	if ((userParam == nullptr) || !(*reinterpret_cast<const bool*>(userParam)))
		return;

	CrashHandler::PrepareStacktrace();
	CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
	CrashHandler::CleanupStacktrace();
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
