/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Input/InputHandler.h"

#include <SDL.h>

#include <iostream>

#ifdef WIN32
//windows workarrounds
#undef KeyPress
#undef KeyRelease
#endif

#include "Rendering/GL/myGL.h"
#include "System/SpringApp.h"

#include "aGui/Gui.h"
#include "ExternalAI/IAILibraryManager.h"
#include "Game/Benchmark.h"
#include "Game/ClientSetup.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/GameController.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PreGame.h"
#include "Game/LoadScreen.h"
#include "Net/GameServer.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaOpenGL.h"
#include "Menu/SelectMenu.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GLContext.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/bitops.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/Log/ConsoleSink.h"
#include "System/Log/ILog.h"
#include "System/Log/DefaultFilter.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/MsgStrings.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/StartScriptGen.h"
#include "System/TimeProfiler.h"
#include "System/ThreadPool.h"
#include "System/Util.h"
#include "System/creg/creg_runtime_tests.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"
#include "System/Input/Joystick.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/Platform/CmdLineParams.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Sound/ISound.h"
#include "System/Sync/FPUCheck.h"
#include "System/UriParser.h"

#ifdef WIN32
	#include <winuser.h> //GetWindowPlacement
	#include <SDL_syswm.h>
	#include "System/Platform/Win/WinVersion.h"
#elif defined(__APPLE__)
#elif defined(HEADLESS)
#else
	#include <X11/Xlib.h>
	#include "System/Platform/Linux/myX11.h"
#endif

#include "lib/luasocket/src/restrictions.h"

CONFIG(unsigned, SetCoreAffinity).defaultValue(0).safemodeValue(1).description("Defines a bitmask indicating which CPU cores the main-thread should use.");
CONFIG(unsigned, SetCoreAffinitySim).defaultValue(0).safemodeValue(1).description("Defines a bitmask indicating which CPU cores the sim-thread should use.");
CONFIG(int, FSAALevel).defaultValue(0).minimumValue(0).maximumValue(8).description("If >0 enables FullScreen AntiAliasing.");
CONFIG(int, SmoothLines).defaultValue(2).safemodeValue(0).minimumValue(0).maximumValue(3).description("Smooth lines.\n 0 := off\n 1 := fastest\n 2 := don't care\n 3 := nicest");
CONFIG(int, SmoothPoints).defaultValue(2).safemodeValue(0).minimumValue(0).maximumValue(3).description("Smooth points.\n 0 := off\n 1 := fastest\n 2 := don't care\n 3 := nicest");
CONFIG(float, TextureLODBias).defaultValue(0.0f).minimumValue(-4.0f).maximumValue(4.0f);
CONFIG(bool, FixAltTab).defaultValue(false);
CONFIG(std::string, FontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine text.");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine small text.");
CONFIG(int, FontSize).defaultValue(23).description("Sets the font size (in pixels) of the MainMenu and more.");
CONFIG(int, SmallFontSize).defaultValue(14).description("Sets the font size (in pixels) of the engine GUIs and more.");
CONFIG(int, FontOutlineWidth).defaultValue(3).description("Sets the width of the black outline around Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(float, FontOutlineWeight).defaultValue(25.0f).description("Sets the opacity of Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(int, SmallFontOutlineWidth).defaultValue(2).description("see FontOutlineWidth");
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f).description("see FontOutlineWeight");
CONFIG(bool, Fullscreen).defaultValue(true).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(bool, UseHighResTimer).defaultValue(false).description("On Windows, sets whether Spring will use low- or high-resolution timer functions for tasks like graphical interpolation between game frames.");
CONFIG(int, XResolution).defaultValue(0).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, WindowPosX).defaultValue(32).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowState).defaultValue(CGlobalRendering::WINSTATE_MAXIMIZED);
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(int, PathingThreadCount).defaultValue(0).safemodeValue(1).minimumValue(0);
CONFIG(std::string, name).defaultValue(UnnamedPlayerName).description("Sets your name in the game. Since this is overridden by lobbies with your lobby username when playing, it usually only comes up when viewing replays or starting the engine directly for testing purposes.");
#ifdef __linux
CONFIG(bool, BlockCompositing).defaultValue(true).description("Disables kwin compositing to fix tearing.");
#endif

static SDL_GLContext sdlGlCtx;
static SDL_Window* window;


/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
static bool MultisampleVerify()
{
	if (!GLEW_ARB_multisample)
		return false;
	GLint buffers = 0;
	GLint samples = 0;
	glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &buffers);
	glGetIntegerv(GL_SAMPLES_ARB, &samples);
	if (buffers && samples) {
		return true;
	}
	return false;
}





/**
 * Initializes SpringApp variables
 *
 * @param argc argument count
 * @param argv array of argument strings
 */
SpringApp::SpringApp(int argc, char** argv): cmdline(new CmdLineParams(argc, argv))
{
	// initializes configHandler which we need
	ParseCmdLine(argv[0]);

	spring_clock::PushTickRate(configHandler->GetBool("UseHighResTimer") || cmdline->IsSet("useHighResTimer"));
	// set the Spring "epoch" to be whatever value the first
	// call to gettime() returns, should not be 0 (can safely
	// be done before SDL_Init, we are not using SDL_GetTicks
	// as our clock anymore)
	spring_time::setstarttime(spring_time::gettime(true));
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	spring_clock::PopTickRate();
	creg::System::FreeClasses();
	delete cmdline;
}

/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize()
{
	assert(cmdline != NULL);
	assert(configHandler != NULL);

	FileSystemInitializer::InitializeLogOutput();
	CLogOutput::LogSystemInfo();
	LOG("         CPU Clock: %s", spring_clock::GetName());
	LOG("Physical CPU Cores: %d", Threading::GetPhysicalCpuCores());
	LOG(" Logical CPU Cores: %d", Threading::GetLogicalCpuCores());
	CMyMath::Init();

	globalRendering = new CGlobalRendering();
	globalRendering->SetFullScreen(configHandler->GetBool("Fullscreen"), cmdline->IsSet("window"), cmdline->IsSet("fullscreen"));
	globalRendering->SetViewSize(configHandler->GetInt("XResolution"), configHandler->GetInt("YResolution"));

#if !(defined(WIN32) || defined(__APPLE__) || defined(HEADLESS))
	// this MUST run before any other X11 call (esp. those by SDL!)
	// we need it to make calls to X11 threadsafe
	if (!XInitThreads()) {
		LOG_L(L_FATAL, "Xlib is not thread safe");
		return false;
	}
#endif

#if defined(WIN32) && defined(__GNUC__)
	// load QTCreator's gdb helper dll; a variant of this should also work on other OSes
	{
		// don't display a dialog box if gdb helpers aren't found
		UINT olderrors = SetErrorMode(SEM_FAILCRITICALERRORS);
		if (LoadLibrary("gdbmacros.dll")) {
			LOG_L(L_DEBUG, "QT Creator's gdbmacros.dll loaded");
		}
		SetErrorMode(olderrors);
	}
#endif
	// Initialize crash reporting
	CrashHandler::Install();
	good_fpu_control_registers(__FUNCTION__);

	// CREG & GlobalConfig
	creg::System::InitializeClasses();
	GlobalConfig::Instantiate();

	// Create Window
	if (!InitWindow(("Spring " + SpringVersion::GetSync()).c_str())) {
		SDL_Quit();
		return false;
	}

	// Init OpenGL
	LoadExtensions(); // Initialize GLEW
	globalRendering->PostInit();
	InitOpenGL();

	// Install Watchdog (must happen after time epoch is set)
	Watchdog::Install();
	Watchdog::RegisterThread(WDT_MAIN, true);

	// ArchiveScanner uses for_mt --> needs thread-count set
	// (use all threads available, later switch to less)
	ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());
	FileSystemInitializer::Initialize();

	mouseInput = IMouseInput::GetInstance();
	input.AddHandler(boost::bind(&SpringApp::MainEventHandler, this, _1));

	// Global structures
	gs = new CGlobalSynced();
	gu = new CGlobalUnsynced();

	// GUIs
	agui::InitGui();
	LoadFonts();
	CNamedTextures::Init();
	LuaOpenGL::Init();
	ISound::Initialize();
	InitJoystick();

	// Lua socket restrictions
	luaSocketRestrictions = new CLuaSocketRestrictions();

	// Multithreading & Affinity
	Threading::SetThreadName("unknown"); // set default threadname
	Threading::InitThreadPool();

	// Create CGameSetup and CPreGame objects
	Startup();

	return true;
}


/**
 * @return whether window initialization succeeded
 * @param title char* string with window title
 *
 * Initializes the game window
 */
bool SpringApp::InitWindow(const char* title)
{
	// SDL will cause a creation of gpu-driver thread that will clone its name from the starting threads (= this one = mainthread)
	Threading::SetThreadName("gpu-driver");

	// the crash reporter should be catching errors, not SDL
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1)) {
		LOG_L(L_FATAL, "Could not initialize SDL: %s", SDL_GetError());
		return false;
	}

	PrintAvailableResolutions();
	SDL_DisableScreenSaver();

	if (!CreateSDLWindow(title)) {
		LOG_L(L_FATAL, "Failed to set SDL video mode: %s", SDL_GetError());
		return false;
	}

	if (cmdline->IsSet("minimise")) {
		SDL_HideWindow(window);
	}

	// anyone other thread spawned from the main-process should be `unknown`
	Threading::SetThreadName("unknown");
	return true;
}

/**
 * @return whether setting the video mode was successful
 *
 * Sets SDL video mode options/settings
 */
bool SpringApp::CreateSDLWindow(const char* title)
{
	int sdlflags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	// use standard: 24bit color + 24bit depth + 8bit stencil & doublebuffered
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	//SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Create GL debug context when wanted (allows further GL verbose informations, but runs slower)
	if (configHandler->GetBool("DebugGL")) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	}

	// FullScreen AntiAliasing
	globalRendering->FSAA = configHandler->GetInt("FSAALevel");
	if (globalRendering->FSAA > 0) {
		make_even_number(globalRendering->FSAA);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, globalRendering->FSAA);
	}

	// Use Native Desktop Resolution
	// and yes SDL2 can do this itself when sizeX & sizeY are set to zero, but
	// oh wonder SDL2 failes then when you use Display Cloneing and similar
	//  -> i.e. DVI monitor runs then at 640x400 and HDMI at full-HD (yes with display _cloning_!)
	SDL_DisplayMode dmode;
	SDL_GetDesktopDisplayMode(0, &dmode);
	if (globalRendering->viewSizeX<=0) globalRendering->viewSizeX = dmode.w;
	if (globalRendering->viewSizeY<=0) globalRendering->viewSizeY = dmode.h;

	// In Windowed Mode Limit Minimum Window Size
	static const int minViewSizeX = 400;
	static const int minViewSizeY = 300;
	if (!globalRendering->fullScreen) {
		globalRendering->viewSizeX = std::max(globalRendering->viewSizeX, minViewSizeX);
		globalRendering->viewSizeY = std::max(globalRendering->viewSizeY, minViewSizeY);
	}

	// Borderless
	const bool borderless = configHandler->GetBool("WindowBorderless");
	if (globalRendering->fullScreen) {
		sdlflags |= borderless ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
	}
	sdlflags |= borderless ? SDL_WINDOW_BORDERLESS : 0;

#if defined(WIN32)
	if (borderless && !globalRendering->fullScreen) {
		sdlflags &= ~SDL_WINDOW_RESIZABLE;
	}
#endif

	// Window Pos & State
	globalRendering->winPosX  = configHandler->GetInt("WindowPosX");
	globalRendering->winPosY  = configHandler->GetInt("WindowPosY");
	globalRendering->winState = configHandler->GetInt("WindowState");
	switch (globalRendering->winState) {
		case CGlobalRendering::WINSTATE_MAXIMIZED: sdlflags |= SDL_WINDOW_MAXIMIZED; break;
		case CGlobalRendering::WINSTATE_MINIMIZED: sdlflags |= SDL_WINDOW_MINIMIZED; break;
	}

	// Create Window
	window = SDL_CreateWindow(title,
		globalRendering->winPosX, globalRendering->winPosY,
		globalRendering->viewSizeX, globalRendering->viewSizeY,
		sdlflags);
	if (!window) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Could not set video mode:\n%s", SDL_GetError());
		handleerror(NULL, buf, "ERROR", MBF_OK|MBF_EXCL);
		return false;
	}

	// Create GL Context
	SDL_SetWindowMinimumSize(window, minViewSizeX, minViewSizeY);
	sdlGlCtx = SDL_GL_CreateContext(window);
	globalRendering->window = window;

#ifdef STREFLOP_H
	// Something in SDL_SetVideoMode (OpenGL drivers?) messes with the FPU control word.
	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
#endif

#if defined(__linux) && !defined(HEADLESS)
	// disable kwin compositing to fix tearing
	// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
	if (configHandler->GetBool("BlockCompositing")) {
		MyX11BlockCompositing(window);
	}
#endif

	return true;
}


// origin for our coordinates is the bottom left corner
void SpringApp::GetDisplayGeometry()
{
#ifdef HEADLESS
	globalRendering->screenSizeX = 8;
	globalRendering->screenSizeY = 8;
	globalRendering->winSizeX = 8;
	globalRendering->winSizeY = 8;
	globalRendering->winPosX = 0;
	globalRendering->winPosY = 0;
	globalRendering->UpdateViewPortGeometry();
	return;

#else
	SDL_Rect screenSize;
	SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &screenSize);
	globalRendering->screenSizeX = screenSize.w;
	globalRendering->screenSizeY = screenSize.h;

	SDL_GetWindowSize(window, &globalRendering->winSizeX, &globalRendering->winSizeY);
	SDL_GetWindowPosition(window, &globalRendering->winPosX, &globalRendering->winPosY);

	globalRendering->UpdateViewPortGeometry();

	//XXX SDL2 is crap ...
	// Reading window state fails if it is changed via the window manager, like clicking on the titlebar (2013)
	// https://bugzilla.libsdl.org/show_bug.cgi?id=1508 & https://bugzilla.libsdl.org/show_bug.cgi?id=2282
	// happens on linux too!
  #ifdef __APPLE__
	auto state = SDL_GetWindowFlags(window);
  #elif defined(WIN32)
	int state = 0;
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	struct SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(globalRendering->window, &info);

	if (GetWindowPlacement(info.info.win.window, &wp)) {
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			state = SDL_WINDOW_MAXIMIZED;
		if (wp.showCmd == SW_SHOWMINIMIZED)
			state = SDL_WINDOW_MINIMIZED;
	}
  #else
	auto state = MyX11GetWindowState(window);
  #endif

	globalRendering->winState = CGlobalRendering::WINSTATE_DEFAULT;
	if (state & SDL_WINDOW_MAXIMIZED) {
		globalRendering->winState = CGlobalRendering::WINSTATE_MAXIMIZED;
	} else
	if (state & SDL_WINDOW_MINIMIZED) {
		globalRendering->winState = CGlobalRendering::WINSTATE_MINIMIZED;
	}
#endif
}


/**
 * Saves position of the window, if we are not in full-screen mode
 */
void SpringApp::SaveWindowPosition()
{
#ifndef HEADLESS
	configHandler->Set("Fullscreen", globalRendering->fullScreen);
	if (!globalRendering->fullScreen) {
		GetDisplayGeometry();
		if (globalRendering->winState == CGlobalRendering::WINSTATE_DEFAULT) {
			configHandler->Set("WindowPosX",  globalRendering->winPosX);
			configHandler->Set("WindowPosY",  globalRendering->winPosY);
			configHandler->Set("WindowState", globalRendering->winState);
		} else
		if (globalRendering->winState == CGlobalRendering::WINSTATE_MINIMIZED) {
			// don't automatically save minimized states
		} else {
			configHandler->Set("WindowState", globalRendering->winState);
		}
	}
#endif
}


void SpringApp::SetupViewportGeometry()
{
	GetDisplayGeometry();

	globalRendering->SetDualScreenParams();
	globalRendering->UpdateViewPortGeometry();
	globalRendering->UpdatePixelGeometry();

	const int vpx = globalRendering->viewPosX;
	const int vpy = globalRendering->winSizeY - globalRendering->viewSizeY - globalRendering->viewPosY;

	agui::gui->UpdateScreenGeometry(globalRendering->viewSizeX, globalRendering->viewSizeY, vpx, vpy);
}


/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL()
{
	// reinit vsync
	VSync.Init();

	// check if FSAA init worked fine
	if (globalRendering->FSAA && !MultisampleVerify())
		globalRendering->FSAA = 0;

	// setup GL smoothing
	const int lineSmoothing = configHandler->GetInt("SmoothLines");
	if (lineSmoothing > 0) {
		GLenum hint = GL_FASTEST;
		if (lineSmoothing >= 3) {
			hint = GL_NICEST;
		} else if (lineSmoothing >= 2) {
			hint = GL_DONT_CARE;
		}
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, hint);
	}
	const int pointSmoothing = configHandler->GetInt("SmoothPoints");
	if (pointSmoothing > 0) {
		GLenum hint = GL_FASTEST;
		if (pointSmoothing >= 3) {
			hint = GL_NICEST;
		} else if (pointSmoothing >= 2) {
			hint = GL_DONT_CARE;
		}
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, hint);
	}

	// setup LOD bias factor
	const float lodBias = configHandler->GetFloat("TextureLODBias");
	if (math::fabs(lodBias) > 0.01f) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, lodBias );
	}

	//FIXME not needed anymore with SDL2?
	if (configHandler->GetBool("FixAltTab")) {
		// free GL resources
		GLContext::Free();

		// initialize any GL resources that were lost
		GLContext::Init();
	}

	// setup viewport
	SetupViewportGeometry();
	glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	gluPerspective(45.0f, globalRendering->aspectRatio, 2.8f, CGlobalRendering::MAX_VIEW_RANGE);

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Clear Window
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(window);

	// Print Final Mode (call after SetupViewportGeometry, which updates viewSizeX/Y)
	SDL_DisplayMode dmode;
	SDL_GetWindowDisplayMode(window, &dmode);
	LOG("[%s] video mode set to %ix%i:%ibit @%iHz %s", __FUNCTION__, globalRendering->viewSizeX, globalRendering->viewSizeY, SDL_BITSPERPIXEL(dmode.format), dmode.refresh_rate, globalRendering->fullScreen ? "" : "(windowed)");
}


void SpringApp::LoadFonts()
{
	// Initialize font
	const std::string fontFile = configHandler->GetString("FontFile");
	const std::string smallFontFile = configHandler->GetString("SmallFontFile");
	const int fontSize = configHandler->GetInt("FontSize");
	const int smallFontSize = configHandler->GetInt("SmallFontSize");
	const int outlineWidth = configHandler->GetInt("FontOutlineWidth");
	const float outlineWeight = configHandler->GetFloat("FontOutlineWeight");
	const int smallOutlineWidth = configHandler->GetInt("SmallFontOutlineWidth");
	const float smallOutlineWeight = configHandler->GetFloat("SmallFontOutlineWeight");

	SafeDelete(font);
	SafeDelete(smallFont);
	font = CglFont::LoadFont(fontFile, fontSize, outlineWidth, outlineWeight);
	smallFont = CglFont::LoadFont(smallFontFile, smallFontSize, smallOutlineWidth, smallOutlineWeight);

	const static std::string installBroken = ", installation of spring broken? did you run make install?";
	if (!font) {
		throw content_error(std::string("Failed to load FontFile: ") + fontFile + installBroken);
	}
	if (!smallFont) {
		throw content_error(std::string("Failed to load SmallFontFile: ") + smallFontFile + installBroken);
	}
}



/**
 * @return whether commandline parsing was successful
 *
 * Parse command line arguments
 */
void SpringApp::ParseCmdLine(const std::string& binaryName)
{
	cmdline->SetUsageDescription("Usage: " + binaryName + " [options] [path_to_script.txt or demo.sdf]");
	cmdline->AddSwitch(0,   "sync-version",       "Display program sync version (for online gaming)");
	cmdline->AddSwitch('f', "fullscreen",         "Run in fullscreen mode");
	cmdline->AddSwitch('w', "window",             "Run in windowed mode");
	cmdline->AddInt(   'x', "xresolution",        "Set X resolution");
	cmdline->AddInt(   'y', "yresolution",        "Set Y resolution");
	cmdline->AddSwitch('b', "minimise",           "Start in background (minimised)");
	cmdline->AddSwitch(0,   "nocolor",            "Disables colorized stdout");
	cmdline->AddSwitch('q', "quiet",              "Ignore unrecognized arguments");
	cmdline->AddString('s', "server",             "Run as a server");

	cmdline->AddSwitch('t', "textureatlas",       "Dump each finalized textureatlas in textureatlasN.tga");
	cmdline->AddInt(   0,   "benchmark",          "Enable benchmark mode (writes a benchmark.data file). The given number specifies the timespan to test.");
	cmdline->AddInt(   0,   "benchmarkstart",     "Benchmark start time in minutes.");

	cmdline->AddSwitch(0,   "list-ai-interfaces", "Dump a list of available AI Interfaces to stdout");
	cmdline->AddSwitch(0,   "list-skirmish-ais",  "Dump a list of available Skirmish AIs to stdout");
	cmdline->AddSwitch(0,   "list-config-vars",   "Dump a list of config vars and meta data to stdout");
	cmdline->AddSwitch(0,   "list-def-tags",      "Dump a list of all unitdef-, weapondef-, ... tags and meta data to stdout");
	cmdline->AddSwitch(0,   "list-ceg-classes",   "Dump a list of available projectile classes to stdout");
	cmdline->AddSwitch(0,   "test-creg",          "Test if all CREG classes are completed");

	cmdline->AddSwitch(0,   "safemode",           "Turns off many things that are known to cause problems (i.e. on PC/Mac's with lower-end graphic cards)");

	cmdline->AddString('C', "config",             "Exclusive configuration file");
	cmdline->AddSwitch('i', "isolation",          "Limit the data-dir (games & maps) scanner to one directory");
	cmdline->AddString(0,   "isolation-dir",      "Specify the isolation-mode data-dir (see --isolation)");
	cmdline->AddString('d', "write-dir",          "Specify where Spring writes to.");
	cmdline->AddString('g', "game",               "Specify the game that will be instantly loaded");
	cmdline->AddString('m', "map",                "Specify the map that will be instantly loaded");
	cmdline->AddString('n', "name",               "Set your player name");

	try {
		cmdline->Parse();
	} catch (const CmdLineParams::unrecognized_option& err) {
		LOG_L(L_ERROR, "%s\n", err.what());
		if (!cmdline->IsSet("quiet")) {
			cmdline->PrintUsage();
			exit(EXIT_FAILURE);
		}
	}

#ifndef WIN32
	if (!cmdline->IsSet("nocolor") && (getenv("SPRING_NOCOLOR") == NULL)) {
		// don't colorize, if our output is piped to a diff tool or file
		if (isatty(fileno(stdout)))
			log_console_colorizedOutput(true);
	}
#endif

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->IsSet("help")) {
		cmdline->PrintUsage();
		exit(EXIT_SUCCESS);
	}
	if (cmdline->IsSet("version")) {
		std::cout << "Spring " << SpringVersion::GetFull() << std::endl;
		exit(EXIT_SUCCESS);
	}
	if (cmdline->IsSet("sync-version")) {
		// Note, the missing "Spring " is intentionally to make it compatible with `spring-dedicated --sync-version`
		std::cout << SpringVersion::GetSync() << std::endl;
		exit(EXIT_SUCCESS);
	}

	if (cmdline->IsSet("isolation")) {
		dataDirLocater.SetIsolationMode(true);
	}

	if (cmdline->IsSet("isolation-dir")) {
		dataDirLocater.SetIsolationMode(true);
		dataDirLocater.SetIsolationModeDir(cmdline->GetString("isolation-dir"));
	}

	if (cmdline->IsSet("write-dir")) {
		dataDirLocater.SetWriteDir(cmdline->GetString("write-dir"));
	}

	// Interface Documentations in JSON-Format
	if (cmdline->IsSet("list-config-vars")) {
		ConfigVariable::OutputMetaDataMap();
		exit(EXIT_SUCCESS);
	}
	else if (cmdline->IsSet("list-def-tags")) {
		DefType::OutputTagMap();
		exit(0);
	}
	else if (cmdline->IsSet("list-ceg-classes")) {
		const int res = CCustomExplosionGenerator::OutputProjectileClassInfo() ? EXIT_SUCCESS : EXIT_FAILURE;
		exit(res);
	}

	// Runtime Tests
	if (cmdline->IsSet("test-creg")) {
		const int res = creg::RuntimeTest() ? EXIT_SUCCESS : EXIT_FAILURE;
		exit(res);
	}

	const string configSource = (cmdline->IsSet("config") ? cmdline->GetString("config") : "");
	const bool safemode = cmdline->IsSet("safemode");

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->IsSet("list-ai-interfaces")) {
		LOG_DISABLE();
		FileSystemInitializer::PreInitializeConfigHandler(configSource, safemode);
		FileSystemInitializer::InitializeLogOutput();
		LOG_ENABLE();
		IAILibraryManager::OutputAIInterfacesInfo();
		exit(0);
	}
	else if (cmdline->IsSet("list-skirmish-ais")) {
		LOG_DISABLE();
		FileSystemInitializer::PreInitializeConfigHandler(configSource, safemode);
		FileSystemInitializer::InitializeLogOutput();
		LOG_ENABLE();
		IAILibraryManager::OutputSkirmishAIInfo();
		exit(0);
	}

	LOG("[%s] command-line args: \"%s\"", __FUNCTION__, cmdline->GetCmdLine().c_str());
	FileSystemInitializer::PreInitializeConfigHandler(configSource, safemode);

	if (cmdline->IsSet("textureatlas")) {
		CTextureAtlas::SetDebug(true);
	}

	if (cmdline->IsSet("name")) {
		const string name = cmdline->GetString("name");
		if (!name.empty()) {
			configHandler->SetString("name", StringReplace(name, " ", "_"));
		}
	}

	if (cmdline->IsSet("benchmark")) {
		CBenchmark::enabled = true;
		if (cmdline->IsSet("benchmarkstart")) {
			CBenchmark::startFrame = cmdline->GetInt("benchmarkstart") * 60 * GAME_SPEED;
		}
		CBenchmark::endFrame = CBenchmark::startFrame + cmdline->GetInt("benchmark") * 60 * GAME_SPEED;
	}
}


void SpringApp::RunScript(boost::shared_ptr<ClientSetup> clientSetup, const std::string& buf)
{
	clientSetup->LoadFromStartScript(buf);
	// LoadFromStartScript overrides all values so reset cmdline defined ones
	if (cmdline->IsSet("server")) { clientSetup->hostIP = cmdline->GetString("server"); clientSetup->isHost = true; }
	clientSetup->SanityCheck();

	pregame = new CPreGame(clientSetup);
	if (clientSetup->isHost)
		pregame->LoadSetupscript(buf);
}


/**
 * Initializes instance of GameSetup
 */
void SpringApp::Startup()
{
	// bash input
	const std::string inputFile = cmdline->GetInputFile();
	const std::string extension = FileSystem::GetExtension(inputFile);

	// create base clientsetup
	boost::shared_ptr<ClientSetup> startsetup(new ClientSetup());
	if (cmdline->IsSet("server")) { startsetup->hostIP = cmdline->GetString("server"); startsetup->isHost = true; }
	startsetup->myPlayerName = configHandler->GetString("name");
	startsetup->SanityCheck();

	// no argument (either game is given or show selectmenu)
	if (inputFile.empty()) {
		startsetup->isHost = true;
		if (cmdline->IsSet("game") && cmdline->IsSet("map")) {
			// --game and --map directly specified, try to run them
			std::string buf = StartScriptGen::CreateMinimalSetup(cmdline->GetString("game"), cmdline->GetString("map"));
			RunScript(startsetup, buf);
		} else {
			// menu
		#ifdef HEADLESS
			handleerror(NULL,
				"The headless version of the engine can not be run in interactive mode.\n"
				"Please supply a start-script, save- or demo-file.", "ERROR", MBF_OK|MBF_EXCL);
		#endif
			activeController = new SelectMenu(startsetup);
		}
		return;
	}

	// process given argument
	if (inputFile.find("spring://") == 0) {
		// url (syntax: spring://username:password@host:port)
		if (!ParseSpringUri(inputFile, startsetup->myPlayerName, startsetup->myPasswd, startsetup->hostIP, startsetup->hostPort))
			throw content_error("invalid url specified: " + inputFile);
		startsetup->isHost = false;
		pregame = new CPreGame(startsetup);
	} else if (extension == "sdf") {
		// demo
		startsetup->isHost        = true;
		startsetup->myPlayerName += " (spec)";
		pregame = new CPreGame(startsetup);
		pregame->LoadDemo(inputFile);
	} else if (extension == "ssf") {
		// savegame
		startsetup->isHost = true;
		pregame = new CPreGame(startsetup);
		pregame->LoadSavefile(inputFile);
	} else {
		// startscript
		LOG("[%s] Loading StartScript from: %s", __FUNCTION__, inputFile.c_str());
		CFileHandler fh(inputFile, SPRING_VFS_PWD_ALL);
		if (!fh.FileExists())
			throw content_error("Setup-script does not exist in given location: " + inputFile);

		std::string buf;
		if (!fh.LoadStringData(buf))
			throw content_error("Setup-script cannot be read: " + inputFile);

		RunScript(startsetup, buf);
	}
}


/**
 * @return return code of activecontroller draw function
 *
 * Draw function repeatedly called, it calls all the
 * other draw functions
 */
int SpringApp::Update()
{
	if (globalRendering->FSAA)
		glEnable(GL_MULTISAMPLE_ARB);

	int ret = 1;
	if (activeController) {
		ret = activeController->Update();
		if (ret) {
			ScopedTimer cputimer("GameController::Draw");
			ret = activeController->Draw();
		}
	}

	ScopedTimer cputimer("SwapBuffers");
	spring_time pre = spring_now();
	VSync.Delay();
	SDL_GL_SwapWindow(window);
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
	return ret;
}


/**
 * Executes the application
 * (contains main game loop)
 */
int SpringApp::Run()
{
	try {
		if (!Initialize())
			return -1;
	}
	CATCH_SPRING_ERRORS

	while (!gu->globalQuit) {
		Watchdog::ClearTimer(WDT_MAIN);
		input.PushEvents();
		if (!Update())
			break;
	}
	SaveWindowPosition();
	ShutDown();

	LOG("[SpringApp::%s] exitCode=%d", __FUNCTION__, GetExitCode());
	return (GetExitCode());
}



/**
 * Deallocates and shuts down game
 */
void SpringApp::ShutDown()
{
	// if a thread crashes *during* first SA::Run-->SA::ShutDown
	// then main::Run will call us again via ErrorMessageBox (not
	// a good idea)
	static unsigned int numCalls = 0;

	if ((numCalls++) != 0)
		return;
	if (gu != NULL)
		gu->globalQuit = true;

	LOG("[SpringApp::%s][1]", __FUNCTION__);
	ThreadPool::SetThreadCount(0);
	LOG("[SpringApp::%s][2]", __FUNCTION__);

	SafeDelete(pregame);
	// don't use SafeDelete: many components in ~CGame
	// expect game-pointer to remain valid during call
	delete game; game = NULL;

	agui::FreeGui();

	LOG("[SpringApp::%s][3]", __FUNCTION__);
	SafeDelete(net);
	SafeDelete(gameServer);
	SafeDelete(gameSetup);

	LOG("[SpringApp::%s][4]", __FUNCTION__);
	CLoadScreen::DeleteInstance();
	ISound::Shutdown();
	FreeJoystick();

	LOG("[SpringApp::%s][5]", __FUNCTION__);
	SafeDelete(font);
	SafeDelete(smallFont);
	CNamedTextures::Kill();
	GLContext::Free();
	GlobalConfig::Deallocate();
	UnloadExtensions();

	IMouseInput::FreeInstance(mouseInput);

	LOG("[SpringApp::%s][6]", __FUNCTION__);
	SDL_SetWindowGrab(window, SDL_FALSE);
	WindowManagerHelper::FreeIcon();
#if !defined(HEADLESS)
	SDL_GL_DeleteContext(sdlGlCtx);
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
#endif
	SDL_Quit();

	LOG("[SpringApp::%s][7]", __FUNCTION__);
	SafeDelete(gs);
	SafeDelete(gu);
	SafeDelete(globalRendering);
	SafeDelete(luaSocketRestrictions);

	FileSystemInitializer::Cleanup();

	LOG("[SpringApp::%s][8]", __FUNCTION__);
	Watchdog::Uninstall();
	LOG("[SpringApp::%s][9]", __FUNCTION__);
}


bool SpringApp::MainEventHandler(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_WINDOWEVENT: {
			switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED: {
					Watchdog::ClearTimer(WDT_MAIN, true);
					InitOpenGL();
					activeController->ResizeEvent();
					mouseInput->InstallWndCallback();
				} break;
				case SDL_WINDOWEVENT_SHOWN: {
					// reactivate sounds and other
					globalRendering->active = true;
					if (ISound::IsInitialized()) {
						sound->Iconified(false);
					}
					if (globalRendering->fullScreen) {
						FBO::GLContextReinit();
					}
				} break;
				case SDL_WINDOWEVENT_HIDDEN: {
					// deactivate sounds and other
					globalRendering->active = false;
					if (ISound::IsInitialized()) {
						sound->Iconified(true);
					}
					if (globalRendering->fullScreen) {
						FBO::GLContextLost();
					}
				} break;
				case SDL_WINDOWEVENT_FOCUS_GAINED: {
					// update keydown table
					KeyInput::Update(0, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));
				} break;
				case SDL_WINDOWEVENT_FOCUS_LOST: {
					Watchdog::ClearTimer(WDT_MAIN, true);

					// SDL has some bug and does not update modstate on alt+tab/minimize etc.
					//FIXME check if still happens with SDL2 (2013)
					SDL_SetModState((SDL_Keymod)(SDL_GetModState() & (KMOD_NUM | KMOD_CAPS | KMOD_MODE)));

					// release all keyboard keys
					KeyInput::ReleaseAllKeys();

					// simulate mouse release to prevent hung buttons
					for (int i = 1; i <= NUM_BUTTONS; ++i) {
						if (!mouse)
							continue;

						if (!mouse->buttons[i].pressed)
							continue;

						SDL_Event event;
						event.type = event.button.type = SDL_MOUSEBUTTONUP;
						event.button.state = SDL_RELEASED;
						event.button.which = 0;
						event.button.button = i;
						event.button.x = -1;
						event.button.y = -1;
						SDL_PushEvent(&event);
					}

					// unlock mouse
					if (mouse && mouse->locked) {
						mouse->ToggleMiddleClickScroll();
					}

					// and make sure to un-capture mouse
					if (SDL_GetWindowGrab(window))
						SDL_SetWindowGrab(window, SDL_FALSE);

					break;
				}
				case SDL_WINDOWEVENT_CLOSE: {
					gu->globalQuit = true;
					break;
				}
			};
		} break;
		case SDL_QUIT: {
			gu->globalQuit = true;
		} break;
		case SDL_TEXTEDITING: {
			//FIXME don't known when this is called
		} break;
		case SDL_TEXTINPUT: {
			if (!activeController) {
				break;
			}

			std::string utf8Text = event.text.text;

			const bool catched = eventHandler.TextInput(utf8Text);

			if (activeController->userWriting && !catched){
				auto ac = activeController;
				if (ac->ignoreNextChar) {
					utf8Text = utf8Text.substr(Utf8NextChar(utf8Text, 0));
				}
				ac->writingPos = Clamp<int>(ac->writingPos, 0, ac->userInput.length());
				ac->userInput.insert(ac->writingPos, utf8Text);
				ac->writingPos += utf8Text.length();
			}
		} break;
		case SDL_KEYDOWN: {
			KeyInput::Update(event.key.keysym.sym, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				activeController->KeyPressed(KeyInput::GetNormalizedKeySymbol(event.key.keysym.sym), event.key.repeat);
			}
		} break;
		case SDL_KEYUP: {
			KeyInput::Update(event.key.keysym.sym, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				if (activeController->ignoreNextChar) {
					activeController->ignoreNextChar = false;
				}
				activeController->KeyReleased(KeyInput::GetNormalizedKeySymbol(event.key.keysym.sym));
			}
		} break;
	};

	return false;
}
