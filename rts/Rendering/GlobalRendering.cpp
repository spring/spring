/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include <SDL.h>

#include "GlobalRendering.h"
#include "GlobalRenderingInfo.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/Fonts/glFont.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/type2.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/StringHash.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/Platform/Threading.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"
#include "System/ScopedResource.h"
#include "System/creg/creg_cond.h"

#include <SDL_syswm.h>


CONFIG(bool, DebugGL).defaultValue(false).description("Enables GL debug-context and output. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);
CONFIG(int, MSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("Enables multisample anti-aliasing; 'level' is the number of samples used.");

CONFIG(int, ForceDisablePersistentMapping).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableExplicitAttribLocs).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableClipCtrl).defaultValue(0).minimumValue(0).maximumValue(1);
//CONFIG(int, ForceDisableShaders).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableGL4).defaultValue(0).safemodeValue(1).minimumValue(0).maximumValue(1);

CONFIG(int, ForceCoreContext).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceSwapBuffers).defaultValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with AMD video cards.\n -1:=runtime detect, 0:=off, 1:=on");

// enabled in safemode, far more likely the gpu runs out of memory than this extension causes crashes!
CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true).description("Runtime compress most textures to save VideoRAM.");
CONFIG(bool, DualScreenMode).defaultValue(false).description("Sets whether to split the screen in half, with one half for minimap and one for main screen. Right side is for minimap unless DualScreenMiniMapOnLeft is set.");
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false).description("When set, will make the left half of the screen the minimap when DualScreenMode is set.");
CONFIG(bool, TeamNanoSpray).defaultValue(true).headlessValue(false);

CONFIG(int, MinimizeOnFocusLoss).defaultValue(0).minimumValue(0).maximumValue(1).description("When set to 1 minimize Window if it loses key focus when in fullscreen mode.");

CONFIG(bool, Fullscreen).defaultValue(true).headlessValue(false).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(bool, BlockCompositing).defaultValue(false).safemodeValue(true).description("Disables kwin compositing to fix tearing, possible fixes low FPS in windowed mode, too.");

CONFIG(int, XResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, XResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See XResolution, just for windowed.");
CONFIG(int, YResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See YResolution, just for windowed.");
CONFIG(int, WindowPosX).defaultValue(0 ).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");

//deprecated stuff
CONFIG(int, RendererHash).deprecated(true);
CONFIG(bool, FSAA).deprecated(true);
CONFIG(int, FSAALevel).deprecated(true);
CONFIG(bool, ForceDisableShaders).deprecated(true);


#define WINDOWS_NO_INVISIBLE_GRIPS 1

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */
static uint8_t globalRenderingMem[sizeof(CGlobalRendering)];

CGlobalRendering* globalRendering = nullptr;
GlobalRenderingInfo globalRenderingInfo;


CR_BIND(CGlobalRendering, )

CR_REG_METADATA(CGlobalRendering, (
	CR_MEMBER(teamNanospray),
	CR_MEMBER(drawSky),
	CR_MEMBER(drawWater),
	CR_MEMBER(drawGround),
	CR_MEMBER(drawMapMarks),
	CR_MEMBER(drawFog),

	CR_MEMBER(drawDebug),
	CR_MEMBER(drawDebugTraceRay),
	CR_MEMBER(drawDebugCubeMap),

	CR_MEMBER(glDebug),
	CR_MEMBER(glDebugErrors),

	CR_MEMBER(timeOffset),
	CR_MEMBER(lastTimeOffset),
	CR_MEMBER(lastFrameTime),
	CR_MEMBER(lastFrameStart),
	CR_MEMBER(lastSwapBuffersEnd),
	CR_MEMBER(weightedSpeedFactor),
	CR_MEMBER(drawFrame),
	CR_MEMBER(FPS),

	CR_IGNORED(screenSizeX),
	CR_IGNORED(screenSizeY),
	CR_IGNORED(screenPosX),
	CR_IGNORED(screenPosY),

	CR_IGNORED(winPosX),
	CR_IGNORED(winPosY),
	CR_IGNORED(winSizeX),
	CR_IGNORED(winSizeY),
	CR_IGNORED(viewPosX),
	CR_IGNORED(viewPosY),
	CR_IGNORED(viewSizeX),
	CR_IGNORED(viewSizeY),
	CR_IGNORED(winBorder),
	CR_IGNORED(winChgFrame),
	CR_IGNORED(screenViewMatrix),
	CR_IGNORED(screenProjMatrix),
	CR_IGNORED(pixelX),
	CR_IGNORED(pixelY),

	CR_IGNORED(minViewRange),
	CR_IGNORED(maxViewRange),
	CR_IGNORED(aspectRatio),

	CR_IGNORED(forceDisablePersistentMapping),
	CR_IGNORED(forceDisableShaders),
	CR_IGNORED(forceDisableGL4),
	CR_IGNORED(forceCoreContext),
	CR_IGNORED(forceSwapBuffers),

	CR_IGNORED(msaaLevel),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(maxTexAnisoLvl),

	CR_IGNORED(active),
	CR_IGNORED(compressTextures),

	CR_IGNORED(haveAMD),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(amdHacks),
	CR_IGNORED(supportPersistentMapping),
	CR_IGNORED(supportExplicitAttribLoc),
	CR_IGNORED(supportNonPowerOfTwoTex),
	CR_IGNORED(supportTextureQueryLOD),
	CR_IGNORED(supportMSAAFrameBuffer),
	CR_IGNORED(supportDepthBufferBitDepth),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(supportClipSpaceControl),
	CR_IGNORED(supportSeamlessCubeMaps),
	CR_IGNORED(supportFragDepthLayout),
	CR_IGNORED(haveARB),
	CR_IGNORED(haveGLSL),
	CR_IGNORED(haveGL4),
	CR_IGNORED(glslMaxVaryings),
	CR_IGNORED(glslMaxAttributes),
	CR_IGNORED(glslMaxDrawBuffers),
	CR_IGNORED(glslMaxRecommendedIndices),
	CR_IGNORED(glslMaxRecommendedVertices),
	CR_IGNORED(glslMaxUniformBufferBindings),
	CR_IGNORED(glslMaxUniformBufferSize),
	CR_IGNORED(glslMaxStorageBufferBindings),
	CR_IGNORED(glslMaxStorageBufferSize),
	CR_IGNORED(dualScreenMode),
	CR_IGNORED(dualScreenMiniMapOnLeft),

	CR_IGNORED(fullScreen),
	CR_IGNORED(borderless),

	CR_IGNORED(sdlWindows),
	CR_IGNORED(glContexts),

	CR_IGNORED(glTimerQueries)
))


void CGlobalRendering::InitStatic() { globalRendering = new (globalRenderingMem) CGlobalRendering(); }
void CGlobalRendering::KillStatic() { globalRendering->PreKill();  spring::SafeDestruct(globalRendering); }

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastTimeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_notime)
	, lastSwapBuffersEnd(spring_notime)
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

	, winBorder{ 0 }

	, winChgFrame(0)

	, screenViewMatrix()
	, screenProjMatrix()

	// pixel geometry
	, pixelX(0.01f)
	, pixelY(0.01f)

	// sane defaults
	, minViewRange(MIN_ZNEAR_DIST * 8.0f)
	, maxViewRange(MAX_VIEW_RANGE * 0.5f)
	, aspectRatio(1.0f)

	, forceDisableShaders(/*configHandler->GetInt("ForceDisableShaders")*/ false)
	, forceDisableGL4(configHandler->GetInt("ForceDisableGL4"))
	, forceCoreContext(configHandler->GetInt("ForceCoreContext"))
	, forceSwapBuffers(configHandler->GetInt("ForceSwapBuffers"))

	// fallback
	, msaaLevel(configHandler->GetInt("MSAALevel"))
	, maxTextureSize(2048)
	, maxTexAnisoLvl(0.0f)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)

	, drawDebug(false)
	, drawDebugTraceRay(false)
	, drawDebugCubeMap(false)

	, glDebug(configHandler->GetBool("DebugGL"))
	, glDebugErrors(false)

	, teamNanospray(configHandler->GetBool("TeamNanoSpray"))
	, active(true)
	, compressTextures(false)

	, haveAMD(false)
	, haveMesa(false)
	, haveIntel(false)
	, haveNvidia(false)
	, amdHacks(false)

	, supportPersistentMapping(false)
	, supportExplicitAttribLoc(false)
	, supportNonPowerOfTwoTex(false)
	, supportTextureQueryLOD(false)
	, supportMSAAFrameBuffer(false)
	, supportDepthBufferBitDepth(16)
	, supportRestartPrimitive(false)
	, supportClipSpaceControl(false)
	, supportSeamlessCubeMaps(false)
	, supportFragDepthLayout(false)
	, haveARB(false)
	, haveGLSL(false)
	, haveGL4(false)

	, glslMaxVaryings(0)
	, glslMaxAttributes(0)
	, glslMaxDrawBuffers(0)
	, glslMaxRecommendedIndices(0)
	, glslMaxRecommendedVertices(0)
	, glslMaxUniformBufferBindings(0)
	, glslMaxUniformBufferSize(0)
	, glslMaxStorageBufferBindings(0)
	, glslMaxStorageBufferSize(0)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(configHandler->GetBool("Fullscreen"))
	, borderless(configHandler->GetBool("WindowBorderless"))
	, sdlWindows{nullptr, nullptr}
	, glContexts{nullptr, nullptr}

	, glTimerQueries{0}
{
	verticalSync->WrapNotifyOnChange();
	configHandler->NotifyOnChange(this, {
		"Fullscreen",
		"WindowBorderless",
		"XResolution",
		"YResolution",
		"XResolutionWindowed",
		"YResolutionWindowed",
		"WindowPosX",
		"WindowPosY"
	});
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
	verticalSync->WrapRemoveObserver();

	// protect against aborted startup
	if (glContexts[0] != nullptr) {
		glDeleteQueries(glTimerQueries.size(), glTimerQueries.data());
	}

	DestroyWindowAndContext(sdlWindows[0], glContexts[0]);
	DestroyWindowAndContext(sdlWindows[1], glContexts[1]);
	KillSDL();

	sdlWindows[0] = nullptr;
	sdlWindows[1] = nullptr;
	glContexts[0] = nullptr;
	glContexts[1] = nullptr;
}

void CGlobalRendering::PreKill()
{
	UniformConstants::GetInstance().Kill(); //unsafe to kill in ~CGlobalRendering()
	RenderBuffer::KillStatic();
	CShaderHandler::FreeInstance();
}


SDL_Window* CGlobalRendering::CreateSDLWindow(const char* title, bool hidden) const
{
	SDL_Window* newWindow = nullptr;

	const int aaLvls[] = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const int zbBits[] = {24, 32, 16};

	const char* winName = hidden? "hidden": "main";
	const char* wpfName = "";

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for %s window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for %s window",
	};

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");
	int2 newRes = GetCfgWinRes();

	// note:
	//   passing the minimized-flag is useless (state is not saved if minimized)
	//   and has no effect anyway, setting a minimum size for a window overrides
	//   it while disabling the SetWindowMinimumSize call still results in a 1x1
	//   window on the desktop
	//
	//   SDL_WINDOW_FULLSCREEN, for "real" fullscreen with a videomode change;
	//   SDL_WINDOW_FULLSCREEN_DESKTOP for "fake" fullscreen that takes the size of the desktop;
	//   and 0 for windowed mode.

	uint32_t sdlFlags  = (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	         sdlFlags |= (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
	         sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);
	         sdlFlags |= (SDL_WINDOW_HIDDEN * hidden);

	for (size_t i = 0; i < (sizeof(aaLvls) / sizeof(aaLvls[0])) && (newWindow == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0; j < (sizeof(zbBits) / sizeof(zbBits[0])) && (newWindow == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr) {
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

	if (!hidden)
		UpdateWindowBorders(newWindow);

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);


	if (msaaLevel > 0) {
		if (softGL != nullptr)
			LOG_L(L_WARNING, "MSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		make_even_number(msaaLevel);
	}

	if ((sdlWindows[0] = CreateSDLWindow(title, false)) == nullptr)
		return false;

	if ((sdlWindows[1] = CreateSDLWindow(title, true)) == nullptr)
		return false;

	if (hidden)
		SDL_HideWindow(sdlWindows[0]);

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	SetWindowAttributes(sdlWindows[0]);
	//SetWindowAttributes(sdlWindows[1]);

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

	MakeCurrentContext(false, false, false);
	SDL_DisableScreenSaver();
	return true;
}


void CGlobalRendering::MakeCurrentContext(bool hidden, bool secondary, bool clear) const {
	if (clear) {
		SDL_GL_MakeCurrent(sdlWindows[hidden], nullptr);
	} else {
		SDL_GL_MakeCurrent(sdlWindows[hidden], glContexts[secondary]);
	}
}


void CGlobalRendering::DestroyWindowAndContext(SDL_Window* window, SDL_GLContext context) {
	if (!window)
		return;

	if (window == sdlWindows[0]) {
		WindowManagerHelper::SetIconSurface(window, nullptr);
		SetWindowInputGrabbing(false);
	}

	SDL_GL_MakeCurrent(window, nullptr);
	SDL_DestroyWindow(window);

	#if !defined(HEADLESS)
	if (context)
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

	QueryVersionInfo(sdlVersionStr, glVidMemStr);
	CheckGLExtensions();
	SetGLSupportFlags();
	QueryGLMaxVals();

	LogVersionInfo(sdlVersionStr, glVidMemStr);
	ToggleGLDebugOutput(0, 0, 0);

	UniformConstants::GetInstance().Init();
	glGenQueries(glTimerQueries.size(), glTimerQueries.data());
	RenderBuffer::InitStatic();
}

void CGlobalRendering::SwapBuffers(bool allowSwapBuffers, bool clearErrors)
{
	spring_time pre;
	{
		SCOPED_TIMER("Misc::SwapBuffers");
		assert(sdlWindows[0] != nullptr);

		// silently or verbosely clear queue at the end of every frame
		if (clearErrors || glDebugErrors)
			glClearErrors("GR", __func__, glDebugErrors);

		if (!allowSwapBuffers && !forceSwapBuffers)
			return;

		pre = spring_now();

		RenderBuffer::SwapStandardRenderBuffers();
		CglFont::SwapRenderBuffers();
		IStreamBufferConcept::PutBufferLocks();

		//https://stackoverflow.com/questions/68480028/supporting-opengl-screen-capture-by-third-party-applications
		glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);

		SDL_GL_SwapWindow(sdlWindows[0]);
	}
	// exclude debug from SCOPED_TIMER("Misc::SwapBuffers");
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
	globalRendering->lastSwapBuffersEnd = spring_now();
}

void CGlobalRendering::SetGLTimeStamp(uint32_t queryIdx) const
{
	if (!GLEW_ARB_timer_query)
		return;

	glQueryCounter(glTimerQueries[(NUM_OPENGL_TIMER_QUERIES * (drawFrame & 1)) + queryIdx], GL_TIMESTAMP);
}

uint64_t CGlobalRendering::CalcGLDeltaTime(uint32_t queryIdx0, uint32_t queryIdx1) const
{
	if (!GLEW_ARB_timer_query)
		return 0;

	const uint32_t queryBase = NUM_OPENGL_TIMER_QUERIES * (1 - (drawFrame & 1));

	assert(queryIdx0 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx1 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx0 < queryIdx1);

	GLuint64 t0 = 0;
	GLuint64 t1 = 0;

	GLint res = 0;

	// results from the previous frame should already (or soon) be available
	while (!res) {
		glGetQueryObjectiv(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT_AVAILABLE, &res);
	}

	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx0], GL_QUERY_RESULT, &t0);
	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT, &t1);

	// nanoseconds between timestamps
	return (t1 - t0);
}


void CGlobalRendering::CheckGLExtensions() const
{
	char extMsg[ 128] = {0};
	char errMsg[2048] = {0};
	char* ptr = &extMsg[0];

	if (!GLEW_ARB_multitexture       ) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " multitexture ");
	if (!GLEW_ARB_texture_env_combine) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_env_combine ");
	if (!GLEW_ARB_texture_compression) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_compression ");

	if (extMsg[0] == 0)
		return;

	SNPRINTF(errMsg, sizeof(errMsg),
		"OpenGL extension(s) GL_ARB_{%s} not found; update your GPU drivers!\n"
		"  GL renderer: %s\n"
		"  GL  version: %s\n",
		extMsg,
		globalRenderingInfo.glRenderer,
		globalRenderingInfo.glVersion);

	throw unsupported_error(errMsg);
}

void CGlobalRendering::SetGLSupportFlags()
{
	const std::string& glVendor = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);

	haveARB   = static_cast<bool>(GLEW_ARB_vertex_program && GLEW_ARB_fragment_program);
	haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != nullptr);
	haveGLSL &= static_cast<bool>(GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader);
	haveGLSL &= static_cast<bool>(GLEW_VERSION_2_0); // we want OpenGL 2.0 core functions

	#ifndef HEADLESS
	if (!haveARB || !haveGLSL)
		throw unsupported_error("OpenGL shaders not supported, aborting");
	#endif

	// useful if a GPU claims to support GL4 and shaders but crashes (Intels...)
	haveARB  &= !forceDisableShaders;
	haveGLSL &= !forceDisableShaders;

	haveAMD    = (  glVendor.find(   "ati ") != std::string::npos) || (  glVendor.find("amd ") != std::string::npos) ||
				 (glRenderer.find("radeon ") != std::string::npos) || (glRenderer.find("amd ") != std::string::npos); //it's amazing how inconsistent AMD detection can be
	haveIntel  = (  glVendor.find(  "intel") != std::string::npos);
	haveNvidia = (  glVendor.find("nvidia ") != std::string::npos);
	haveMesa   = (glRenderer.find(  "mesa ") != std::string::npos) || (glRenderer.find("gallium ") != std::string::npos);

	if (haveAMD) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "AMD";
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

	supportPersistentMapping = GLEW_ARB_buffer_storage;
	supportPersistentMapping &= (configHandler->GetInt("ForceDisablePersistentMapping") == 0);

	supportExplicitAttribLoc = GLEW_ARB_explicit_attrib_location;
	supportExplicitAttribLoc &= (configHandler->GetInt("ForceDisableExplicitAttribLocs") == 0);

	// ATI's x-series doesn't support NPOTs, hd-series does
	supportNonPowerOfTwoTex = GLEW_ARB_texture_non_power_of_two /* && (!haveAMD || (glRenderer.find(" x") == std::string::npos && glRenderer.find(" 9") == std::string::npos))*/;
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
	if (int2 glslVerNum = { 0, 0 }; sscanf(globalRenderingInfo.glslVersionShort.data(), "%d.%d", &glslVerNum.x, &glslVerNum.y) == 2) {
		globalRenderingInfo.glslVersionNum = glslVerNum.x * 100 + glslVerNum.y;
	}

	haveGL4 = static_cast<bool>(GLEW_ARB_multi_draw_indirect);
	haveGL4 &= static_cast<bool>(GLEW_ARB_uniform_buffer_object);
	haveGL4 &= static_cast<bool>(GLEW_ARB_shader_storage_buffer_object);
	haveGL4 &= (globalRenderingInfo.glslVersionNum >= 430); // #version 430 is what engine shaders use
	haveGL4 &= !forceDisableGL4;

	{
		// use some ATI bugfixes?
		const int amdHacksCfg = configHandler->GetInt("AtiHacks");
		amdHacks = haveAMD;
		amdHacks &= (amdHacksCfg < 0); // runtime detect
		amdHacks |= (amdHacksCfg > 0); // user override
	}

	// runtime-compress textures? (also already required for SMF ground textures)
	// default to off because it reduces quality, smallest mipmap level is bigger
	if (GLEW_ARB_texture_compression)
		compressTextures = configHandler->GetBool("CompressTextures");


	#ifdef GLEW_NV_primitive_restart
	// not defined for headless builds
	supportRestartPrimitive = GLEW_NV_primitive_restart;
	#endif
	#ifdef GLEW_ARB_clip_control
	supportClipSpaceControl = GLEW_ARB_clip_control;
	#endif
	#ifdef GLEW_ARB_seamless_cube_map
	supportSeamlessCubeMaps = GLEW_ARB_seamless_cube_map;
	#endif
	#ifdef GLEW_EXT_framebuffer_multisample
	supportMSAAFrameBuffer = GLEW_EXT_framebuffer_multisample;
	#endif
	// CC did not exist as an extension before GL4.5, too recent to enforce

	//stick to the theory that reported = exist
	//supportClipSpaceControl &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 45);
	supportClipSpaceControl &= (configHandler->GetInt("ForceDisableClipCtrl") == 0);

	//supportFragDepthLayout = ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 42);
	#ifdef GLEW_ARB_conservative_depth
	supportFragDepthLayout = GLEW_ARB_conservative_depth; //stick to the theory that reported = exist
	#endif

	//stick to the theory that reported = exist
	//supportMSAAFrameBuffer &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 32);

	for (const int bits : {16, 24, 32}) {
		bool supported = false;
		if (FBO::IsSupported()) {
			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 16, 16);
			const GLint format = DepthBitsToFormat(bits);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, format, 16, 16);
			supported = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT);
			fbo.Unbind();
		}

		if (supported)
			supportDepthBufferBitDepth = std::max(supportDepthBufferBitDepth, bits);
	}

	//TODO figure out if needed
	if (globalRendering->amdHacks) {
		supportDepthBufferBitDepth = 24;
	}
}

void CGlobalRendering::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTexAnisoLvl);

	// some GLSL relevant information
	if (GLEW_ARB_uniform_buffer_object) {
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glslMaxUniformBufferBindings);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,      &glslMaxUniformBufferSize);
	}

	if (GLEW_ARB_shader_storage_buffer_object) {
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &glslMaxStorageBufferBindings);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,      &glslMaxStorageBufferSize);
	}

	glGetIntegerv(GL_MAX_VARYING_FLOATS,                 &glslMaxVaryings);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,                 &glslMaxAttributes);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS,                   &glslMaxDrawBuffers);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,               &glslMaxRecommendedIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,              &glslMaxRecommendedVertices);

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
	if ((grInfo.sdlDriverName = (const char*) SDL_GetCurrentVideoDriver(           )) == nullptr) grInfo.sdlDriverName = "unknown";
	// should never be null with any driver, no harm in an extra check
	// (absence of GLSL version string would indicate bigger problems)
	if (std::strcmp(globalRenderingInfo.glslVersion, "unknown") == 0)
		throw unsupported_error("OpenGL shaders not supported, aborting");

	if (!ShowDriverWarning(grInfo.glVendor))
		throw unsupported_error("OpenGL drivers not installed, aborting");

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
	LOG("\tSDL driver  : %s", globalRenderingInfo.sdlDriverName);
	LOG("\t");
	LOG("\tInitialized OpenGL Context: %i.%i (%s)", globalRenderingInfo.glContextVersion.x, globalRenderingInfo.glContextVersion.y, globalRenderingInfo.glContextIsCore ? "Core" : "Compat");
	LOG("\tARB shader support        : %i", haveARB);
	LOG("\tGLSL shader support       : %i", haveGLSL);
	LOG("\tGL4 support               : %i", haveGL4);
	LOG("\tFBO extension support     : %i", FBO::IsSupported());
	LOG("\tNVX GPU mem-info support  : %i", glewIsExtensionSupported("GL_NVX_gpu_memory_info"));
	LOG("\tATI GPU mem-info support  : %i", glewIsExtensionSupported("GL_ATI_meminfo"));
	LOG("\tNPOT-texture support      : %i (%i)", supportNonPowerOfTwoTex, glewIsExtensionSupported("GL_ARB_texture_non_power_of_two"));
	LOG("\tS3TC/DXT1 texture support : %i/%i", glewIsExtensionSupported("GL_EXT_texture_compression_s3tc"), glewIsExtensionSupported("GL_EXT_texture_compression_dxt1"));
	LOG("\ttexture query-LOD support : %i (%i)", supportTextureQueryLOD, glewIsExtensionSupported("GL_ARB_texture_query_lod"));
	LOG("\tMSAA frame-buffer support : %i (%i)", supportMSAAFrameBuffer, glewIsExtensionSupported("GL_EXT_framebuffer_multisample"));
	LOG("\tZ-buffer depth            : %i (-)", supportDepthBufferBitDepth);
	LOG("\tprimitive-restart support : %i (%i)", supportRestartPrimitive, glewIsExtensionSupported("GL_NV_primitive_restart"));
	LOG("\tclip-space control support: %i (%i)", supportClipSpaceControl, glewIsExtensionSupported("GL_ARB_clip_control"));
	LOG("\tseamless cube-map support : %i (%i)", supportSeamlessCubeMaps, glewIsExtensionSupported("GL_ARB_seamless_cube_map"));
	LOG("\tfrag-depth layout support : %i (%i)", supportFragDepthLayout, glewIsExtensionSupported("GL_ARB_conservative_depth"));
	LOG("\tpersistent maps support   : %i (%i)", supportPersistentMapping, glewIsExtensionSupported("GL_ARB_buffer_storage"));
	LOG("\texplicit attribs location : %i (%i)", supportExplicitAttribLoc, glewIsExtensionSupported("GL_ARB_explicit_attrib_location"));

	LOG("\t");
	LOG("\tmax. FBO samples             : %i", FBO::GetMaxSamples());
	LOG("\tmax. texture size            : %i", maxTextureSize);
	LOG("\tmax. texture anisotropy level: %f", maxTexAnisoLvl);
	LOG("\tmax. vec4 varyings/attributes: %i/%i", glslMaxVaryings, glslMaxAttributes);
	LOG("\tmax. draw-buffers            : %i", glslMaxDrawBuffers);
	LOG("\tmax. rec. indices/vertices   : %i/%i", glslMaxRecommendedIndices, glslMaxRecommendedVertices);
	LOG("\tmax. uniform buffer-bindings : %i", glslMaxUniformBufferBindings);
	LOG("\tmax. uniform block-size      : %iKB", glslMaxUniformBufferSize / 1024);
	LOG("\tmax. storage buffer-bindings : %i", glslMaxStorageBufferBindings);
	LOG("\tmax. storage block-size      : %iMB", glslMaxStorageBufferSize / (1024 * 1024));
	LOG("\t");
	LOG("\tenable AMD-hacks : %i", amdHacks);
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

void CGlobalRendering::GetAllDisplayBounds(SDL_Rect& r) const
{
	int displayIdx = 0;
	GetScreenBounds(r, &displayIdx);

	std::array<int, 4> mb = { r.x, r.y, r.x + r.w, r.y + r.h }; //L, T, R, B

	for (displayIdx = 1; displayIdx < SDL_GetNumVideoDisplays(); ++displayIdx) {
		SDL_Rect db;
		GetScreenBounds(db, &displayIdx);
		std::array<int, 4> b = { db.x, db.y, db.x + db.w, db.y + db.h }; //L, T, R, B

		if (b[0] < mb[0]) mb[0] = b[0];
		if (b[1] < mb[1]) mb[1] = b[1];
		if (b[2] > mb[2]) mb[2] = b[2];
		if (b[3] > mb[3]) mb[3] = b[3];
	}

	r = { mb[0], mb[1], mb[2] - mb[0], mb[3] - mb[1] };
}

void CGlobalRendering::GetWindowPosSizeBounded(int& x, int& y, int& w, int& h) const
{
	SDL_Rect r;
	GetAllDisplayBounds(r);

	x = std::clamp(x, r.x, r.x + r.w);
	y = std::clamp(y, r.y, r.y + r.h);
	w = std::max(w, minRes.x * (1 - fullScreen)); w = std::min(w, r.w - x);
	h = std::max(h, minRes.y * (1 - fullScreen)); h = std::min(h, r.h - y);
}


void CGlobalRendering::SetWindowTitle(const std::string& title)
{
	SDL_SetWindowTitle(sdlWindows[0], title.c_str());
}

void CGlobalRendering::SetWindowAttributes(SDL_Window* window)
{
	// Get wanted state
	borderless = configHandler->GetBool("WindowBorderless");
	fullScreen = configHandler->GetBool("Fullscreen");
	winPosX = configHandler->GetInt("WindowPosX");
	winPosY = configHandler->GetInt("WindowPosY");

	// get desired resolution
	// note that the configured fullscreen resolution is just
	// ignored by SDL if not equal to the user's screen size
	const int2 maxRes = GetMaxWinRes();
	      int2 newRes = GetCfgWinRes();

	LOG("[GR::%s][1] cfgFullScreen=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, winPosX, winPosY, newRes.x, newRes.y);

	if (SDL_SetWindowFullscreen(window, 0) != 0)
		LOG("[GR::%s][2][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	GetWindowPosSizeBounded(winPosX, winPosY, newRes.x, newRes.y);

	LOG("[GR::%s][3] cfgFullScreen=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, winPosX, winPosY, newRes.x, newRes.y);

	SDL_RestoreWindow(window);
	SDL_SetWindowMinimumSize(window, minRes.x, minRes.y);
	SDL_SetWindowPosition(window, winPosX, winPosY);
	SDL_SetWindowSize(window, newRes.x, newRes.y);
	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);

	if (SDL_SetWindowFullscreen(window, (borderless ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP) * fullScreen) != 0)
		LOG("[GR::%s][4][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	if (newRes == maxRes)
		SDL_MaximizeWindow(window);

	WindowManagerHelper::SetWindowResizable(window, !borderless && !fullScreen);
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	LOG("[GR::%s][1] key=%s val=%s", __func__, key.c_str(), value.c_str());
	winChgFrame = drawFrame + 1; //will happen on next frame
}

void CGlobalRendering::UpdateWindow()
{
	if (winChgFrame != drawFrame)
		return;

	if (sdlWindows[0] == nullptr)
		return;

	SetWindowAttributes(sdlWindows[0]);
	MakeCurrentContext(false, false, false);
}


bool CGlobalRendering::GetWindowInputGrabbing()
{
	return static_cast<bool>(SDL_GetWindowGrab(sdlWindows[0]));
}

bool CGlobalRendering::SetWindowInputGrabbing(bool enable)
{
	SDL_SetWindowGrab(sdlWindows[0], enable? SDL_TRUE: SDL_FALSE);
	return enable;
}

bool CGlobalRendering::ToggleWindowInputGrabbing()
{
	if (GetWindowInputGrabbing())
		return (SetWindowInputGrabbing(false));

	return (SetWindowInputGrabbing(true));
}

bool CGlobalRendering::SetWindowPosHelper(int displayIdx, int winRPosX, int winRPosY, int winSizeX_, int winSizeY_, bool fs, bool bl) const
{
#ifndef HEADLESS
	const int numDisplays = SDL_GetNumVideoDisplays();
	if (displayIdx < 0 || displayIdx >= numDisplays) {
		LOG_L(L_ERROR, "[GR::%s] displayIdx(%d) is out of bounds (%d,%d)", __func__, displayIdx, 0, numDisplays - 1);
		return false;
	}

	SDL_Rect db;
	GetScreenBounds(db, &displayIdx);

	const int2 tlPos = { db.x + winRPosX            , db.y + winRPosY             };
	const int2 brPos = { db.x + winRPosX + winSizeX_, db.y + winRPosY + winSizeY_ };

	configHandler->Set("WindowPosX", tlPos.x);
	configHandler->Set("WindowPosY", tlPos.y);

	configHandler->Set(xsKeys[fs], winSizeX_);
	configHandler->Set(ysKeys[fs], winSizeY_);
	configHandler->Set("Fullscreen", fs);
	configHandler->Set("WindowBorderless", bl);
#endif

	return true;
}

int2 CGlobalRendering::GetMaxWinRes() const {
	SDL_DisplayMode dmode;
	SDL_GetDesktopDisplayMode(GetCurrentDisplayIndex(), &dmode);
	return {dmode.w, dmode.h};
}

int2 CGlobalRendering::GetCfgWinRes() const
{
	int2 res = {configHandler->GetInt(xsKeys[fullScreen]), configHandler->GetInt(ysKeys[fullScreen])};

	// copy Native Desktop Resolution if user did not specify a value
	// SDL2 can do this itself if size{X,Y} are set to zero but fails
	// with Display Cloning and similar, causing DVI monitors to only
	// run at (e.g.) 640x400 and HDMI devices at full-HD
	// TODO: make screen configurable?
	if (res.x <= 0 || res.y <= 0)
		res = GetMaxWinRes();

	return res;
}

int CGlobalRendering::GetCurrentDisplayIndex() const
{
	return sdlWindows[0] ? SDL_GetWindowDisplayIndex(sdlWindows[0]) : 0;
}

void CGlobalRendering::GetScreenBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayBounds(displayIndex, &r);
}

void CGlobalRendering::GetUsableScreenBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayUsableBounds(displayIndex, &r);
}


// only called on startup; change the config based on command-line args
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
	winBorder = { 0 };
#else

	SDL_Rect screenSize;
	GetScreenBounds(screenSize);

	// no other good place to set these
	screenSizeX = screenSize.w;
	screenSizeY = screenSize.h;
	screenPosX  = screenSize.x;
	screenPosY  = screenSize.y;

	//probably redundant
	if (!borderless)
		UpdateWindowBorders(sdlWindows[0]);

	SDL_GetWindowSize(sdlWindows[0], &winSizeX, &winSizeY);
	SDL_GetWindowPosition(sdlWindows[0], &winPosX, &winPosY);

	//enforce >=0 https://github.com/beyond-all-reason/spring/issues/23
	//winPosX = std::max(winPosX, 0);
	//winPosY = std::max(winPosY, 0);
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

	// do not notify about changes to block update loop
	configHandler->Set("WindowPosX", winPosX, false, false);
	configHandler->Set("WindowPosY", winPosY, false, false);
	configHandler->Set("XResolutionWindowed", winSizeX, false, false);
	configHandler->Set("YResolutionWindowed", winSizeY, false, false);
}


void CGlobalRendering::UpdateGLConfigs()
{
	LOG("[GR::%s]", __func__);

	// re-read configuration value
	verticalSync->SetInterval();
}

void CGlobalRendering::UpdateScreenMatrices()
{
	// .x := screen width (meters), .y := eye-to-screen (meters)
	static float2 screenParameters = { 0.36f, 0.60f };

	const int remScreenSize = screenSizeY - winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - winPosY; // *bottom*-left origin

	const float vpx = viewPosX + winPosX;
	const float vpy = viewPosY + bottomWinCoor;
	const float vsx = viewSizeX; // same as winSizeX except in dual-screen mode
	const float vsy = viewSizeY; // same as winSizeY
	const float ssx = screenSizeX;
	const float ssy = screenSizeY;
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenParameters.y * (ssx / screenParameters.x);
	const float znear = zplane * 0.5f;
	const float zfar = zplane * 2.0f;
	constexpr float zfact = 0.5f;

	const float left = (vpx - hssx) * zfact;
	const float bottom = (vpy - hssy) * zfact;
	const float right = ((vpx + vsx) - hssx) * zfact;
	const float top = ((vpy + vsy) - hssy) * zfact;

	LOG("[GR::%s] vpx=%f, vpy=%f, vsx=%f, vsy=%f, ssx=%f, ssy=%f, screenPosX=%d, screenPosY=%d", __func__, vpx, vpy, vsx, vsy, ssx, ssy, screenPosX, screenPosY);

	// translate s.t. (0,0,0) is on the zplane, on the window's bottom-left corner
	screenViewMatrix = CMatrix44f{ float3{left / zfact, bottom / zfact, -zplane} };
	screenProjMatrix = CMatrix44f::ClipPerspProj(left, right, bottom, top, znear, zfar, supportClipSpaceControl * 1.0f);
}

void CGlobalRendering::UpdateWindowBorders(SDL_Window* window) const
{
#ifndef HEADLESS
	assert(window);

	SDL_GetWindowBordersSize(window, &winBorder[0], &winBorder[1], &winBorder[2], &winBorder[3]);

	#if defined(_WIN32) && (WINDOWS_NO_INVISIBLE_GRIPS == 1)
	// W/A for 8 px Aero invisible borders https://github.com/libsdl-org/SDL/commit/7c60bec493404905f512c835f502f1ace4eff003
	{
		auto scopedLib = spring::ScopedResource(
			LoadLibrary("dwmapi.dll"),
			[](HMODULE lib) { if (lib) FreeLibrary(lib); });

		if (!scopedLib())
			return;

		using DwmGetWindowAttributeT = HRESULT WINAPI(
			HWND,
			DWORD,
			PVOID,
			DWORD
		);

		static auto* DwmGetWindowAttribute = reinterpret_cast<DwmGetWindowAttributeT*>(GetProcAddress(scopedLib(), "DwmGetWindowAttribute"));

		if (!DwmGetWindowAttribute)
			return;

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		HWND& hwnd = wmInfo.info.win.window;

		RECT rect, frame;

		static constexpr DWORD DWMWA_EXTENDED_FRAME_BOUNDS = 9; // https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
		DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
		GetWindowRect(hwnd, &rect);

		winBorder[0] -= std::max(0l, frame.top   - rect.top    );
		winBorder[1] -= std::max(0l, frame.left  - rect.left   );
		winBorder[2] -= std::max(0l, rect.bottom - frame.bottom);
		winBorder[3] -= std::max(0l, rect.right  - frame.right);

		LOG_L(L_DEBUG, "[GR::%s] Working around Windows 10+ thick borders SDL2 issue, borders are slimmed by TLBR(%d,%d,%d,%d)", __func__,
			static_cast<int>(std::max(0l, frame.top   - rect.top    )),
			static_cast<int>(std::max(0l, frame.left  - rect.left   )),
			static_cast<int>(std::max(0l, rect.bottom - frame.bottom)),
			static_cast<int>(std::max(0l, rect.right  - frame.right ))
		);
	}
	LOG_L(L_DEBUG, "[GR::%s] Storing window borders {%d, %d, %d, %d}", __func__, winBorder[0], winBorder[1], winBorder[2], winBorder[3]);
	#endif
#endif
}

void CGlobalRendering::UpdateGLGeometry()
{
	LOG("[GR::%s][1] winSize=<%d,%d>", __func__, winSizeX, winSizeY);

	ReadWindowPosAndSize();
	SetDualScreenParams();
	UpdateViewPortGeometry();
	UpdatePixelGeometry();
	UpdateScreenMatrices();

	LOG("[GR::%s][2] winSize=<%d,%d>", __func__, winSizeX, winSizeY);
}

void CGlobalRendering::InitGLState()
{
	LOG("[GR::%s]", __func__);

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

	#ifdef GLEW_ARB_seamless_cube_map
	if (supportSeamlessCubeMaps)
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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
	gluPerspective(45.0f, aspectRatio, minViewRange, maxViewRange);

	// this does not accomplish much
	// SwapBuffers(true, true);
	LogDisplayMode(sdlWindows[0]);
}

int CGlobalRendering::DepthBitsToFormat(int bits)
{
	switch (bits)
	{
	case 16:
		return GL_DEPTH_COMPONENT16;
	case 24:
		return GL_DEPTH_COMPONENT24;
	case 32:
		return GL_DEPTH_COMPONENT32;
	default:
		return GL_DEPTH_COMPONENT; //should never hit this
	}
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

	GLint profile = 0;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

	if (profile != 0)
		globalRenderingInfo.glContextIsCore = (profile == GL_CONTEXT_CORE_PROFILE_BIT);
	else
		globalRenderingInfo.glContextIsCore = !GLEW_ARB_compatibility;

	// keep this for convenience
	globalRenderingInfo.glContextVersion = tmpCtx;

	// compare major * 10 + minor s.t. 4.1 evaluates as larger than 3.2
	return ((tmpCtx.x * 10 + tmpCtx.y) >= (minCtx.x * 10 + minCtx.y));
	#endif
}



#if defined(_WIN32) && !defined(HEADLESS)
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
	const static bool dbgTraces = configHandler->GetBool("DebugGLStacktraces");

	#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	if (glDebug) {
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
	configHandler->Set("DebugGL", globalRendering->glDebug);
	#endif

	return true;
}
