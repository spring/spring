/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <sstream>
#include <iostream>

#include <SDL.h>
#if !defined(HEADLESS)
	#include <SDL_syswm.h>
#endif

#include "System/mmgr.h"

#include "Rendering/GL/myGL.h"
#include "System/SpringApp.h"

#include "aGui/Gui.h"
#include "ExternalAI/IAILibraryManager.h"
#include "Game/ClientSetup.h"
#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/GameController.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PreGame.h"
#include "Game/LoadScreen.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"
#include "System/Input/InputHandler.h"
#include "System/Input/Joystick.h"
#include "System/MsgStrings.h"
#include "Lua/LuaOpenGL.h"
#include "Menu/SelectMenu.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/glFont.h"
#include "Rendering/GLContext.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/bitops.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/OpenMP_cond.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/CmdLineParams.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Sound/ISound.h"

#include "System/mmgr.h"

#ifdef WIN32
	#include "System/Platform/Win/WinVersion.h"
#elif defined(__APPLE__)
#elif defined(HEADLESS)
#else
	#include <X11/Xlib.h>
	#include "System/Platform/Linux/myX11.h"
#endif

#undef KeyPress
#undef KeyRelease

#include "lib/gml/gml_base.h"
#include "lib/luasocket/src/restrictions.h"

CONFIG(unsigned, SetCoreAffinity).defaultValue(0).safemodeValue(1).description("Defines a bitmask indicating which CPU cores the main-thread should use.");
CONFIG(int, DepthBufferBits).defaultValue(24);
CONFIG(int, StencilBufferBits).defaultValue(8);
CONFIG(int, FSAALevel).defaultValue(0);
CONFIG(int, SmoothLines).defaultValue(2).safemodeValue(0).minimumValue(0).maximumValue(3).description("Smooth lines.\n 0 := off\n 1 := fastest\n 2 := don't care\n 3 := nicest");
CONFIG(int, SmoothPoints).defaultValue(2).safemodeValue(0).minimumValue(0).maximumValue(3).description("Smooth points.\n 0 := off\n 1 := fastest\n 2 := don't care\n 3 := nicest");
CONFIG(float, TextureLODBias).defaultValue(0.0f);
CONFIG(bool, FixAltTab).defaultValue(false);
CONFIG(std::string, FontFile).defaultValue("fonts/FreeSansBold.otf");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf");
CONFIG(int, FontSize).defaultValue(23);
CONFIG(int, SmallFontSize).defaultValue(14);
CONFIG(int, FontOutlineWidth).defaultValue(3);
CONFIG(float, FontOutlineWeight).defaultValue(25.0f);
CONFIG(int, SmallFontOutlineWidth).defaultValue(2);
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f);
CONFIG(bool, Fullscreen).defaultValue(true);
CONFIG(int, XResolution).defaultValue(0);
CONFIG(int, YResolution).defaultValue(0);
CONFIG(int, WindowPosX).defaultValue(32);
CONFIG(int, WindowPosY).defaultValue(32);
CONFIG(int, WindowState).defaultValue(0);
CONFIG(bool, WindowBorderless).defaultValue(false);
CONFIG(int, HardwareThreadCount).defaultValue(0).safemodeValue(1);
CONFIG(std::string, name).defaultValue(UnnamedPlayerName);


ClientSetup* startsetup = NULL;


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
 */
SpringApp::SpringApp()
	: cmdline(NULL)
{
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	delete cmdline;
	creg::System::FreeClasses();
}

/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize()
{
#if !(defined(WIN32) || defined(__APPLE__) || defined(HEADLESS))
	//! this MUST run before any other X11 call (esp. those by SDL!)
	//! we need it to make calls to X11 threadsafe
	if (!XInitThreads()) {
		LOG_L(L_FATAL, "Xlib is not thread safe");
		return false;
	}
#endif

#if defined(_WIN32) && defined(__GNUC__)
	// load QTCreator's gdb helper dll; a variant of this should also work on other OSes
	{
		// don't display a dialog box if gdb helpers aren't found
		UINT olderrors = SetErrorMode(SEM_FAILCRITICALERRORS);
		if (LoadLibrary("gdbmacros.dll")) {
			LOG("QT Creator's gdbmacros.dll loaded");
		}
		SetErrorMode(olderrors);
	}
#endif

	// Initialize class system
	creg::System::InitializeClasses();

	// Initialize crash reporting
	CrashHandler::Install();

	globalRendering = new CGlobalRendering();

	ParseCmdLine();
	CMyMath::Init();
	good_fpu_control_registers("::Run");

	// log OS version
	LOG("OS: %s", Platform::GetOS().c_str());
	if (Platform::Is64Bit())
		LOG("OS: 64bit native mode");
	else if (Platform::Is32BitEmulation())
		LOG("OS: emulated 32bit mode");
	else
		LOG("OS: 32bit native mode");

	// Rename Threads
	// We give the process itself the name `unknown`, htop & co. will still show the binary's name.
	// But all child threads copy by default the name of their parent, so all threads that don't set
	// their name themselves will show up as 'unknown'.
	Threading::SetThreadName("unknown");
#ifdef _OPENMP
	#pragma omp parallel
	{
		int i = omp_get_thread_num();
		if (i != 0) { // 0 is the source thread
			std::ostringstream buf;
			buf << "omp" << i;
			Threading::SetThreadName(buf.str().c_str());
		}
	}
#endif

	// Install Watchdog
	Watchdog::Install();
	Watchdog::RegisterThread(WDT_MAIN, true);

	FileSystemInitializer::Initialize();

	// Create Window
	if (!InitWindow(("Spring " + SpringVersion::GetSync()).c_str())) {
		SDL_Quit();
		return false;
	}

	mouseInput = IMouseInput::GetInstance();
	keyInput = KeyInput::GetInstance();
	input.AddHandler(boost::bind(&SpringApp::MainEventHandler, this, _1));

	// Global structures
	gs = new CGlobalSynced();
	gu = new CGlobalUnsynced();

	// Initialize GLEW
	LoadExtensions();

	//! check if FSAA init worked fine
	if (globalRendering->FSAA && !MultisampleVerify())
		globalRendering->FSAA = 0;

	globalRendering->PostInit();
	
	InitOpenGL();
	agui::InitGui();
	LoadFonts();

	// Initialize named texture handler
	CNamedTextures::Init();

	// Initialize Lua GL
	LuaOpenGL::Init();

	// Sound & Input
	ISound::Initialize();
	InitJoystick();

	// Lua socket restrictions
	luaSocketRestrictions = new CLuaSocketRestrictions();

	// Multithreading & Affinity
	LOG("CPU Cores: %d", Threading::GetAvailableCores());
	Threading::SetThreadScheduler();
	const uint32_t affinity = configHandler->GetUnsigned("SetCoreAffinity");
	const uint32_t cpuMask  = Threading::SetAffinity(affinity);
	if (cpuMask == 0xFFFFFF) {
		LOG("CPU affinity not set");
	}
	else if (cpuMask != affinity) {
		LOG("CPU affinity mask set: %d (config is %d)", cpuMask, affinity);
	}
	else if (cpuMask == 0) {
		LOG_L(L_ERROR, "Failed to CPU affinity mask <%d>", affinity);
	}
	else {
		LOG("CPU affinity mask set: %d", cpuMask);
	}

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
	unsigned int sdlInitFlags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
#ifdef WIN32
	// the crash reporter should be catching the errors
	sdlInitFlags |= SDL_INIT_NOPARACHUTE;
#endif
	if ((SDL_Init(sdlInitFlags) == -1)) {
		LOG_L(L_FATAL, "Could not initialize SDL: %s", SDL_GetError());
		return false;
	}

	PrintAvailableResolutions();

	WindowManagerHelper::SetCaption(title);

	if (!SetSDLVideoMode()) {
		LOG_L(L_FATAL, "Failed to set SDL video mode: %s", SDL_GetError());
		return false;
	}

	RestoreWindowPosition();
	if (cmdline->IsSet("minimise")) {
		globalRendering->active = false;
		SDL_WM_IconifyWindow();
	}

	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	return true;
}

/**
 * @return whether setting the video mode was successful
 *
 * Sets SDL video mode options/settings
 */
bool SpringApp::SetSDLVideoMode()
{
	int sdlflags = SDL_OPENGL | SDL_RESIZABLE;

	//! w/o SDL_NOFRAME, kde's windowmanager still creates a border (in fullscreen!) and forces a `window`-resize causing a lot of trouble (in the ::SaveWindowPosition)
	sdlflags |= globalRendering->fullScreen ? SDL_FULLSCREEN | SDL_NOFRAME : 0;

	const bool winBorderless = configHandler->GetBool("WindowBorderless");
	sdlflags |= winBorderless ? SDL_NOFRAME : 0;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); //! enable alpha channel ???

	globalRendering->depthBufferBits = configHandler->GetInt("DepthBufferBits");
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, globalRendering->depthBufferBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, configHandler->GetInt("StencilBufferBits"));
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	//! FullScreen AntiAliasing
	globalRendering->FSAA = Clamp(configHandler->GetInt("FSAALevel"), 0, 8);
	if (globalRendering->FSAA > 0) {
		make_even_number(globalRendering->FSAA);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, globalRendering->FSAA);
	}

	//! use desktop resolution?
	if ((globalRendering->viewSizeX<=0) || (globalRendering->viewSizeY<=0)) {
		const SDL_VideoInfo* screenInfo = SDL_GetVideoInfo(); //! it's a read-only struct (we don't need to free it!)
		globalRendering->viewSizeX = screenInfo->current_w;
		globalRendering->viewSizeY = screenInfo->current_h;
	}
	//! fallback if resolution couldn't be detected
	if ((globalRendering->viewSizeX<=0) || (globalRendering->viewSizeY<=0)) {
		globalRendering->viewSizeX = 1024;
		globalRendering->viewSizeY = 768;
	}

	//! screen will be freed by SDL_Quit()
	//! from: http://sdl.beuc.net/sdl.wiki/SDL_SetVideoMode
	//! Note 3: This function should be called in the main thread of your application.
	//! User note 1: Some have found that enabling OpenGL attributes like SDL_GL_STENCIL_SIZE (the stencil buffer size) before the video mode has been set causes the application to simply ignore those attributes, while enabling attributes after the video mode has been set works fine.
	//! User note 2: Also note that, in Windows, setting the video mode resets the current OpenGL context. You must execute again the OpenGL initialization code (set the clear color or the shade model, or reload textures, for example) after calling SDL_SetVideoMode. In Linux, however, it works fine, and the initialization code only needs to be executed after the first call to SDL_SetVideoMode (although there is no harm in executing the initialization code after each call to SDL_SetVideoMode, for example for a multiplatform application).
	SDL_Surface* screen = SDL_SetVideoMode(globalRendering->viewSizeX, globalRendering->viewSizeY, 32, sdlflags);
	if (!screen) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Could not set video mode:\n%s", SDL_GetError());
		handleerror(NULL, buf, "ERROR", MBF_OK|MBF_EXCL);
		return false;
	}

#ifdef STREFLOP_H
	//! Something in SDL_SetVideoMode (OpenGL drivers?) messes with the FPU control word.
	//! Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
#endif

	//! setup GL smoothing
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

	//! setup LOD bias factor
	const float lodBias = Clamp(configHandler->GetFloat("TextureLODBias"), -4.f, 4.f);
	if (math::fabs(lodBias)>0.01f) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
	}

	//! there must be a way to see if this is necessary, compare old/new context pointers?
	if (configHandler->GetBool("FixAltTab")) {
		//! free GL resources
		GLContext::Free();

		//! initialize any GL resources that were lost
		GLContext::Init();
	}

	int bits;
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bits);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE,  &globalRendering->depthBufferBits);
	if (globalRendering->fullScreen) {
		LOG("Video mode set to %ix%i/%ibit", globalRendering->viewSizeX, globalRendering->viewSizeY, bits);
	} else {
		LOG("Video mode set to %ix%i/%ibit (windowed)", globalRendering->viewSizeX, globalRendering->viewSizeY, bits);
	}

	return true;
}


// origin for our coordinates is the bottom left corner
bool SpringApp::GetDisplayGeometry()
{
#ifndef HEADLESS
	//! not really needed, but makes it safer against unknown windowmanager behaviours
	if (globalRendering->fullScreen) {
		globalRendering->screenSizeX = globalRendering->viewSizeX;
		globalRendering->screenSizeY = globalRendering->viewSizeY;
		globalRendering->winSizeX = globalRendering->screenSizeX;
		globalRendering->winSizeY = globalRendering->screenSizeY;
		globalRendering->winPosX = 0;
		globalRendering->winPosY = 0;
		return true;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		return false;
	}


  #if       defined(__APPLE__)
	// TODO: implement this function & RestoreWindowPosition() on Mac
	globalRendering->screenSizeX = 0;
	globalRendering->screenSizeY = 0;
	globalRendering->winSizeX = globalRendering->viewSizeX;
	globalRendering->winSizeY = globalRendering->viewSizeY;
	globalRendering->winPosX = 30;
	globalRendering->winPosY = 30;
	globalRendering->winState = 0;

  #elif     defined(WIN32)
	globalRendering->screenSizeX = GetSystemMetrics(SM_CXSCREEN);
	globalRendering->screenSizeY = GetSystemMetrics(SM_CYSCREEN);

	RECT rect;
	if (!GetClientRect(info.window, &rect)) {
		return false;
	}

	if ((rect.right - rect.left) == 0 || (rect.bottom - rect.top) == 0)
		return false;

	globalRendering->winSizeX = rect.right - rect.left;
	globalRendering->winSizeY = rect.bottom - rect.top;

	// translate from client coords to screen coords
	MapWindowPoints(info.window, HWND_DESKTOP, (LPPOINT)&rect, 2);

	// GetClientRect doesn't do the right thing for restoring window position
	if (!globalRendering->fullScreen) {
		GetWindowRect(info.window, &rect);
	}

	globalRendering->winPosX = rect.left;
	globalRendering->winPosY = rect.top;

	// Get WindowState
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(info.window, &wp)) {
		switch (wp.showCmd) {
			case SW_SHOWMAXIMIZED:
				globalRendering->winState = 1;
				break;
			case SW_SHOWMINIMIZED:
				//! minimized startup breaks SDL_init stuff, so don't store it
				//globalRendering->winState = 2;
				//break;
			default:
				globalRendering->winState = 0;
		}
	}

  #else
	info.info.x11.lock_func();
	{
		Display* display = info.info.x11.display;
		Window   window  = info.info.x11.wmwindow;

		XWindowAttributes attrs;
		XGetWindowAttributes(display, window, &attrs);
		const Screen* screen = attrs.screen;

		globalRendering->screenSizeX = WidthOfScreen(screen);
		globalRendering->screenSizeY = HeightOfScreen(screen);
		globalRendering->winSizeX = attrs.width;
		globalRendering->winSizeY = attrs.height;

		Window tmp;
		int xp, yp;
		XTranslateCoordinates(display, window, attrs.root, 0, 0, &xp, &yp, &tmp);
		globalRendering->winPosX = xp;
		globalRendering->winPosY = yp;

		if (!globalRendering->fullScreen) {
			int frame_left, frame_top;
			MyX11GetFrameBorderOffset(display, window, &frame_left, &frame_top);
			globalRendering->winPosX -= frame_left;
			globalRendering->winPosY -= frame_top;
		}

		globalRendering->winState = MyX11GetWindowState(display, window);
	}
	info.info.x11.unlock_func();

  #endif // defined(__APPLE__)
#endif // defined(HEADLESS)

	return true;
}


/**
 * Restores position of the window, if we are not in full-screen mode
 */
void SpringApp::RestoreWindowPosition()
{
	globalRendering->winPosX  = configHandler->GetInt("WindowPosX");
	globalRendering->winPosY  = configHandler->GetInt("WindowPosY");
	globalRendering->winState = configHandler->GetInt("WindowState");

#ifndef HEADLESS
	if (!globalRendering->fullScreen) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		if (SDL_GetWMInfo(&info)) {
  #if       defined(WIN32)
			bool stateChanged = false;

			if (globalRendering->winState > 0) {
				WINDOWPLACEMENT wp;
				memset(&wp,0,sizeof(WINDOWPLACEMENT));
				wp.length = sizeof(WINDOWPLACEMENT);
				stateChanged = true;
				int wState;
				switch (globalRendering->winState) {
					case 1: wState = SW_SHOWMAXIMIZED; break;
					//! Setting the main-window minimized breaks initialization
					case 2: // wState = SW_SHOWMINIMIZED; break;
					default: stateChanged = false;
				}
				if (stateChanged) {
					ShowWindow(info.window, wState);
					GetDisplayGeometry();
				}
			}

			if (!stateChanged) {
				MoveWindow(info.window, globalRendering->winPosX, globalRendering->winPosY, globalRendering->viewSizeX, globalRendering->viewSizeY, true);
				streflop::streflop_init<streflop::Simple>(); // MoveWindow may modify FPU flags
			}

  #elif     defined(__APPLE__)
			// TODO: implement this function

  #else
			info.info.x11.lock_func();
			{
				XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, globalRendering->winPosX, globalRendering->winPosY);
				MyX11SetWindowState(info.info.x11.display, info.info.x11.wmwindow, globalRendering->winState);
			}
			info.info.x11.unlock_func();

  #endif // defined(WIN32)
		}
	}
#endif // defined(HEADLESS)
}


/**
 * Saves position of the window, if we are not in full-screen mode
 */
void SpringApp::SaveWindowPosition()
{
#ifndef HEADLESS
	if (!globalRendering->fullScreen) {
		GetDisplayGeometry();
		configHandler->Set("WindowPosX",  globalRendering->winPosX);
		configHandler->Set("WindowPosY",  globalRendering->winPosY);
		configHandler->Set("WindowState", globalRendering->winState);

	}
#endif
}


void SpringApp::SetupViewportGeometry()
{
	if (!GetDisplayGeometry()) {
		globalRendering->UpdateWindowGeometry();
	}

	globalRendering->SetDualScreenParams();
	globalRendering->UpdateViewPortGeometry();
	globalRendering->UpdatePixelGeometry();

	agui::gui->UpdateScreenGeometry(
			globalRendering->viewSizeX,
			globalRendering->viewSizeY,
			globalRendering->viewPosX,
			(globalRendering->winSizeY - globalRendering->viewSizeY - globalRendering->viewPosY) );
}


/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL()
{
	VSync.Init();

	SetupViewportGeometry();
	glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	gluPerspective(45.0f,  globalRendering->aspectRatio, 2.8f, CGlobalRendering::MAX_VIEW_RANGE);

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
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

	if (!font || !smallFont) {
		const std::vector<std::string> &fonts = CFileHandler::DirList("fonts/", "*.*tf", SPRING_VFS_RAW_FIRST);
		std::vector<std::string>::const_iterator fi = fonts.begin();
		while (fi != fonts.end()) {
			SafeDelete(font);
			SafeDelete(smallFont);
			font = CglFont::LoadFont(*fi, fontSize, outlineWidth, outlineWeight);
			smallFont = CglFont::LoadFont(*fi, smallFontSize, smallOutlineWidth, smallOutlineWeight);
			if (font && smallFont) {
				break;
			} else {
				++fi;
			}
		}
		if (!font) {
			throw content_error(std::string("Failed to load font: ") + fontFile);
		} else if (!smallFont) {
			throw content_error(std::string("Failed to load font: ") + smallFontFile);
		}
		configHandler->SetString("FontFile", *fi);
		configHandler->SetString("SmallFontFile", *fi);
	}
}


/**
 * @return whether commandline parsing was successful
 *
 * Parse command line arguments
 */
void SpringApp::ParseCmdLine()
{
	cmdline->SetUsageDescription("Usage: " + binaryName + " [options] [path_to_script.txt or demo.sdf]");
	cmdline->AddSwitch(0,   "sync-version",       "Display program sync version (for online gaming)");
	cmdline->AddSwitch('f', "fullscreen",         "Run in fullscreen mode");
	cmdline->AddSwitch('w', "window",             "Run in windowed mode");
	cmdline->AddInt(   'x', "xresolution",        "Set X resolution");
	cmdline->AddInt(   'y', "yresolution",        "Set Y resolution");
	cmdline->AddSwitch('m', "minimise",           "Start minimised");
	cmdline->AddSwitch('s', "server",             "Run as a server");
	cmdline->AddSwitch('c', "client",             "Run as a client");
	cmdline->AddSwitch('p', "projectiledump",     "Dump projectile class info in projectiles.txt");
	cmdline->AddSwitch('t', "textureatlas",       "Dump each finalized textureatlas in textureatlasN.tga");
	cmdline->AddString('n', "name",               "Set your player name");
	cmdline->AddString('C', "config",             "Configuration file");
	cmdline->AddSwitch(0,   "safemode",           "Turns off many things that are known to cause problems (i.e. on PC/Mac's with lower-end graphic cards)");
	cmdline->AddSwitch(0,   "list-ai-interfaces", "Dump a list of available AI Interfaces to stdout");
	cmdline->AddSwitch(0,   "list-skirmish-ais",  "Dump a list of available Skirmish AIs to stdout");
	cmdline->AddSwitch(0,   "list-config-vars",   "Dump a list of config vars and meta data to stdout");
	cmdline->AddSwitch('i', "isolation",          "Limit the data-dir (games & maps) scanner to one directory");
	cmdline->AddString(0,   "isolation-dir",      "Specify the isolation-mode data-dir (see --isolation)");

	try {
		cmdline->Parse();
	} catch (const std::exception& err) {
		std::cerr << err.what() << std::endl << std::endl;
		cmdline->PrintUsage();
		exit(1);
	}

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->IsSet("help")) {
		cmdline->PrintUsage();
		exit(0);
	}
	if (cmdline->IsSet("version")) {
		std::cout << "Spring " << SpringVersion::GetFull() << std::endl;
		exit(0);
	}
	if (cmdline->IsSet("sync-version")) {
		// Note, the missing "Spring " is intentionally to make it compatible with `spring-dedicated --sync-version`
		std::cout << SpringVersion::GetSync() << std::endl;
		exit(0);
	}
	if (cmdline->IsSet("projectiledump")) {
		CCustomExplosionGenerator::OutputProjectileClassInfo();
		exit(0);
	}

	if (cmdline->IsSet("isolation")) {
		dataDirLocater.SetIsolationMode(true);
	}

	if (cmdline->IsSet("isolation-dir")) {
		dataDirLocater.SetIsolationMode(true);
		dataDirLocater.SetIsolationModeDir(cmdline->GetString("isolation-dir"));
	}

	const string configSource = (cmdline->IsSet("config") ? cmdline->GetString("config") : "");
	const bool safemode = cmdline->IsSet("safemode");

	ConfigHandler::Instantiate(configSource, safemode);
	GlobalConfig::Instantiate();

	// mutually exclusive options that cause spring to quit immediately
	// and require the configHandler
	if (cmdline->IsSet("list-ai-interfaces")) {
		dataDirLocater.LocateDataDirs();
		IAILibraryManager::OutputAIInterfacesInfo();
		exit(0);
	}
	else if (cmdline->IsSet("list-skirmish-ais")) {
		dataDirLocater.LocateDataDirs();
		IAILibraryManager::OutputSkirmishAIInfo();
		exit(0);
	}
	else if (cmdline->IsSet("list-config-vars")) {
		ConfigVariable::OutputMetaDataMap();
		exit(0);
	}

#ifdef _DEBUG
	globalRendering->fullScreen = false;
#else
	globalRendering->fullScreen = configHandler->GetBool("Fullscreen");
#endif
	// flags
	if (cmdline->IsSet("window")) {
		globalRendering->fullScreen = false;
	} else if (cmdline->IsSet("fullscreen")) {
		globalRendering->fullScreen = true;
	}

	if (cmdline->IsSet("textureatlas")) {
		CTextureAtlas::debug = true;
	}

	if (cmdline->IsSet("name")) {
		const string name = cmdline->GetString("name");
		if (!name.empty()) {
			configHandler->SetString("name", StringReplace(name, " ", "_"));
		}
	}

	globalRendering->viewSizeX = configHandler->GetInt("XResolution");
	if (cmdline->IsSet("xresolution"))
		globalRendering->viewSizeX = std::max(cmdline->GetInt("xresolution"), 640);

	globalRendering->viewSizeY = configHandler->GetInt("YResolution");
	if (cmdline->IsSet("yresolution"))
		globalRendering->viewSizeY = std::max(cmdline->GetInt("yresolution"), 480);
}


/**
 * Initializes instance of GameSetup
 */
void SpringApp::Startup()
{
	std::string inputFile = cmdline->GetInputFile();
	if (inputFile.empty())
	{
#ifdef HEADLESS
		LOG_L(L_FATAL,
				"The headless version of the engine can not be run in interactive mode.\n"
				"Please supply a start-script, save- or demo-file.");
		exit(1);
#endif
		bool server = !cmdline->IsSet("client") || cmdline->IsSet("server");
#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(server, 64);
#endif
		activeController = new SelectMenu(server);
	}
	else if (inputFile.rfind("sdf") == inputFile.size() - 3)
	{
		std::string demoFileName = inputFile;
		std::string demoPlayerName = configHandler->GetString("name");

		if (demoPlayerName.empty()) {
			demoPlayerName = UnnamedPlayerName;
		} else {
			demoPlayerName = StringReplaceInPlace(demoPlayerName, ' ', '_');
		}

		demoPlayerName += " (spec)";

		startsetup = new ClientSetup();
			startsetup->isHost       = true; // local demo play
			startsetup->myPlayerName = demoPlayerName;

#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(true, 64); //FIXME: add actual number of player
#endif

		pregame = new CPreGame(startsetup);
		pregame->LoadDemo(demoFileName);
	}
	else if (inputFile.rfind("ssf") == inputFile.size() - 3)
	{
		std::string savefile = inputFile;
		startsetup = new ClientSetup();
		startsetup->isHost = true;
		startsetup->myPlayerName = configHandler->GetString("name");
#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(true, 64); //FIXME: add actual number of player
#endif
		pregame = new CPreGame(startsetup);
		pregame->LoadSavefile(savefile);
	}
	else
	{
		LOG("Loading startscript from: %s", inputFile.c_str());
		std::string startscript = inputFile;
		CFileHandler fh(startscript);
		if (!fh.FileExists())
			throw content_error("Setup-script does not exist in given location: "+startscript);

		std::string buf;
		if (!fh.LoadStringData(buf))
			throw content_error("Setup-script cannot be read: " + startscript);
		startsetup = new ClientSetup();
		startsetup->Init(buf);

		// commandline parameters overwrite setup
		if (cmdline->IsSet("client"))
			startsetup->isHost = false;
		else if (cmdline->IsSet("server"))
			startsetup->isHost = true;

#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(startsetup->isHost, 64); //FIXME: add actual number of player
#endif
		pregame = new CPreGame(startsetup);
		if (startsetup->isHost)
			pregame->LoadSetupscript(buf);
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
		Watchdog::ClearTimer(WDT_MAIN);

		if (!GML::SimThreadRunning()) {
			ret = GML::UpdateSim(activeController);
		}

		if (ret) {
			ScopedTimer cputimer("GameController::Draw");
			ret = activeController->Draw();
			GML::PumpAux();
		}
	}

	VSync.Delay();
	SDL_GL_SwapBuffers();

	if (globalRendering->FSAA)
		glDisable(GL_MULTISAMPLE_ARB);

	return ret;
}



static void ResetScreenSaverTimeout()
{
#ifndef HEADLESS
  #if defined(WIN32)
	static unsigned lastreset = 0;
	unsigned curreset = SDL_GetTicks();
	if(globalRendering->active && (curreset - lastreset > 1000)) {
		lastreset = curreset;
		int timeout; // reset screen saver timer
		if(SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0))
			SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, timeout, NULL, 0);
		streflop::streflop_init<streflop::Simple>(); // SystemParametersInfo may modify FPU flags
	}
  #elif defined(__APPLE__)
	// TODO: implement
	return;
  #else
	static unsigned lastreset = 0;
	unsigned curreset = SDL_GetTicks();
	if(globalRendering->active && (curreset - lastreset > 1000)) {
		lastreset = curreset;
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWMInfo(&info)) {
			XForceScreenSaver(info.info.x11.display, ScreenSaverReset);
		}
	}
  #endif
#endif
}


/**
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Executes the application
 * (contains main game loop)
 */
int SpringApp::Run(int argc, char *argv[])
{
	cmdline = new CmdLineParams(argc, argv);
	binaryName = argv[0];

	if (!Initialize())
		return -1;

	GML::Init();

	while (!gu->globalQuit) {
		ResetScreenSaverTimeout();

		{
			SCOPED_TIMER("InputHandler::PushEvents");
			SDL_Event event;

			while (SDL_PollEvent(&event)) {
				streflop::streflop_init<streflop::Simple>(); // SDL_PollEvent may modify FPU flags
				input.PushEvent(event);
			}
		}

		if (!Update())
			break;
	}

	SaveWindowPosition();

	// Shutdown
	Shutdown();
	return GetExitCode();
}



/**
 * Deallocates and shuts down game
 */
void SpringApp::Shutdown()
{
	if (gu) gu->globalQuit = true;

#define DeleteAndNull(x) delete x; x = NULL;

	GML::Exit();
	DeleteAndNull(pregame);
	DeleteAndNull(game);
	DeleteAndNull(gameServer);
	DeleteAndNull(gameSetup);
	CLoadScreen::DeleteInstance();
	ISound::Shutdown();
	DeleteAndNull(font);
	DeleteAndNull(smallFont);
	CNamedTextures::Kill();
	GLContext::Free();
	GlobalConfig::Deallocate();
	ConfigHandler::Deallocate();
	UnloadExtensions();

	IMouseInput::FreeInstance(mouseInput);
	KeyInput::FreeInstance(keyInput);

	SDL_WM_GrabInput(SDL_GRAB_OFF);
#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK);
#endif
	SDL_Quit();

	DeleteAndNull(gs);
	DeleteAndNull(gu);
	DeleteAndNull(globalRendering);
	DeleteAndNull(startsetup);
	DeleteAndNull(luaSocketRestrictions);

	FileSystemInitializer::Cleanup();

	Watchdog::Uninstall();

#ifdef USE_MMGR
	m_dumpMemoryReport();
#endif
}

bool SpringApp::MainEventHandler(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_VIDEORESIZE: {
			GML_MSTMUTEX_LOCK(sim); // MainEventHandler

			Watchdog::ClearTimer(WDT_MAIN, true);
			globalRendering->viewSizeX = event.resize.w;
			globalRendering->viewSizeY = event.resize.h;
#ifndef WIN32
			// HACK   We don't want to break resizing on windows (again?),
			//        so someone should test this very well before enabling it.
			SetSDLVideoMode();
#endif
			InitOpenGL();
			activeController->ResizeEvent();

			break;
		}
		case SDL_VIDEOEXPOSE: {
			GML_MSTMUTEX_LOCK(sim); // MainEventHandler

			Watchdog::ClearTimer(WDT_MAIN, true);
			// re-initialize the stencil
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
			glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
			SetupViewportGeometry();

			break;
		}
		case SDL_QUIT: {
			gu->globalQuit = true;
			break;
		}
		case SDL_ACTIVEEVENT: {
			Watchdog::ClearTimer(WDT_MAIN, true);

			//! deactivate sounds and other
			if (event.active.state & (SDL_APPACTIVE | (globalRendering->fullScreen ? SDL_APPINPUTFOCUS : 0))) {
				globalRendering->active = !!event.active.gain;
				if (ISound::IsInitialized()) {
					sound->Iconified(!event.active.gain);
				}
			}

			//! update keydown table
			if (event.active.gain) {
				keyInput->Update(event.key.keysym.unicode, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));
			}

			//! unlock mouse
			if (mouse && mouse->locked) {
				mouse->ToggleMiddleClickScroll();
			}

			//! release all keyboard keys
			if ((event.active.state & (SDL_APPACTIVE | SDL_APPINPUTFOCUS))) {
				for (boost::uint16_t i = 1; i < SDLK_LAST; ++i) {
					if (i != SDLK_NUMLOCK && i != SDLK_CAPSLOCK && i != SDLK_SCROLLOCK && keyInput->IsKeyPressed(i)) {
						SDL_Event event;
						event.type = event.key.type = SDL_KEYUP;
						event.key.state = SDL_RELEASED;
						event.key.keysym.sym = (SDLKey)i;
						event.key.keysym.unicode = i;
						//event.keysym.mod =;
						//event.keysym.scancode =;
						SDL_PushEvent(&event);
					}
				}
				// SDL has some bug and does not update modstate on alt+tab/minimize etc.
				SDL_SetModState((SDLMod)(SDL_GetModState() & (KMOD_NUM | KMOD_CAPS | KMOD_MODE)));
			}

			//! simulate mouse release to prevent hung buttons
			if ((event.active.state & (SDL_APPACTIVE | SDL_APPMOUSEFOCUS))) {
				for (int i = 1; i <= NUM_BUTTONS; ++i) {
					if (mouse && mouse->buttons[i].pressed) {
						SDL_Event event;
						event.type = event.button.type = SDL_MOUSEBUTTONUP;
						event.button.state = SDL_RELEASED;
						event.button.which = 0;
						event.button.button = i;
						event.button.x = -1;
						event.button.y = -1;
						SDL_PushEvent(&event);
					}
				}

				//! and make sure to un-capture mouse
				if(!event.active.gain && SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
					SDL_WM_GrabInput(SDL_GRAB_OFF);
			}

			break;
		}
		case SDL_KEYDOWN: {
			const boost::uint16_t sym = keyInput->GetNormalizedKeySymbol(event.key.keysym.sym);
			const bool isRepeat = !!keyInput->GetKeyState(sym);

			keyInput->Update(event.key.keysym.unicode, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				activeController->KeyPressed(sym, isRepeat);

				if (activeController->userWriting){
					// use unicode for printed characters
					const boost::uint16_t usym = keyInput->GetCurrentKeyUnicodeChar();

					if ((usym >= SDLK_SPACE) && (usym <= 255)) {
						CGameController* ac = activeController;

						if (ac->ignoreNextChar || (ac->ignoreChar == (char)usym)) {
							ac->ignoreNextChar = false;
						} else {
							if (usym < 255 && (!isRepeat || ac->userInput.length() > 0)) {
								const int len = (int)ac->userInput.length();
								const char str[2] = { (char)usym, 0 };

								ac->writingPos = std::max(0, std::min(len, ac->writingPos));
								ac->userInput.insert(ac->writingPos, str);
								ac->writingPos++;
							}
						}
					}
				}
				activeController->ignoreNextChar = false;
			}
			break;
		}
		case SDL_KEYUP: {
			keyInput->Update(event.key.keysym.unicode, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				activeController->KeyReleased(keyInput->GetNormalizedKeySymbol(event.key.keysym.sym));
			}
			break;
		}
		default:
			break;
	}

	return false;
}
