/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

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
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "System/bitops.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FPUCheck.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/OffscreenGLContext.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/FileSystem/FileSystemHandler.h"
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
	#include "System/Platform/Win/win32.h"
	#include "System/Platform/Win/WinVersion.h"
#elif defined(__APPLE__)
#elif defined(HEADLESS)
#else
	#include <X11/Xlib.h>
	#include <sched.h>
	#include "System/Platform/Linux/myX11.h"
#endif

#undef KeyPress
#undef KeyRelease

#ifdef USE_GML
	#include "lib/gml/gmlsrv.h"
	extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif


ClientSetup* startsetup = NULL;
COffscreenGLContext* SpringApp::ogc = NULL;



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
{
	cmdline = NULL;
	lastRequiredDraw = 0;
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

	Watchdog::Install();
	//! register (this) mainthread
	Watchdog::RegisterThread(WDT_MAIN, true);

	// log OS version
	LOG("OS: %s", Platform::GetOS().c_str());
	if (Platform::Is64Bit())
		LOG("OS: 64bit native mode");
	else if (Platform::Is32BitEmulation())
		LOG("OS: emulated 32bit mode");
	else
		LOG("OS: 32bit native mode");

	FileSystemHandler::Initialize(true);

	UpdateOldConfigs();

	if (!InitWindow(("Spring " + SpringVersion::Get()).c_str())) {
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

	InitOpenGL();
	agui::InitGui();
	LoadFonts();

	globalRendering->PostInit();

	// Initialize named texture handler
	CNamedTextures::Init();

	// Initialize Lua GL
	LuaOpenGL::Init();

	// Sound
	ISound::Initialize();
	InitJoystick();

	SetProcessAffinity(configHandler->Get("SetCoreAffinity", 0));

	// Create CGameSetup and CPreGame objects
	Startup();

	return true;
}

void SpringApp::SetProcessAffinity(int affinity)
{
#ifdef WIN32
	if (affinity > 0) {
		//! Get the available cores
		DWORD curMask;
		DWORD cores = 0;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &cores);

		DWORD_PTR wantedCore = 0xff;

		//! Find an useable core
		while ((wantedCore & cores) == 0 ) {
			wantedCore >>= 1;
		}

		//! Set the affinity
		HANDLE thread = GetCurrentThread();
		DWORD_PTR result = 0;
		if (affinity == 1) {
			result = SetThreadIdealProcessor(thread, (DWORD)wantedCore);
		} else if (affinity >= 2) {
			result = SetThreadAffinityMask(thread, wantedCore);
		}

		if (result > 0) {
			LOG("CPU: affinity set (%d)", affinity);
		} else {
			LOG("CPU: affinity failed");
		}
	}
#elif defined(__APPLE__)
	// no-op
#elif defined(HEADLESS)
	// no-op
#else
	if (affinity > 0) {
		cpu_set_t cpusSystem; CPU_ZERO(&cpusSystem);
		cpu_set_t cpusWanted; CPU_ZERO(&cpusWanted);

		// use pid of calling process (0)
		sched_getaffinity(0, sizeof(cpu_set_t), &cpusSystem);

		// interpret <affinity> as a bit-mask indicating
		// on which of the available system CPU's (which
		// are numbered logically from 0 to N-1) we want
		// to run
		// note that this approach will fail when N > 32
		LOG("[SetProcessAffinity(%d)] available system CPU's: %d", affinity, CPU_COUNT(&cpusSystem));

		// add the available system CPU's to the wanted set
		for (int n = CPU_COUNT(&cpusSystem) - 1; n >= 0; n--) {
			if ((affinity & (1 << n)) != 0 && CPU_ISSET(n, &cpusSystem)) {
				CPU_SET(n, &cpusWanted);
			}
		}

		sched_setaffinity(0, sizeof(cpu_set_t), &cpusWanted);
	}
#endif
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

	bool winBorderless = configHandler->Get("WindowBorderless", false);
	sdlflags |= winBorderless ? SDL_NOFRAME : 0;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); //! enable alpha channel ???

	globalRendering->depthBufferBits = configHandler->Get("DepthBufferBits", 24);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, globalRendering->depthBufferBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, configHandler->Get("StencilBufferBits", 8));
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	//! FullScreen AntiAliasing
	globalRendering->FSAA = Clamp(configHandler->Get("FSAALevel", 0), 0, 8);
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
	streflop_init<streflop::Simple>();
#endif

	//! setup GL smoothing
	const int defaultSmooth = 0; //! FSAA ? 0 : 3;  // until a few things get fixed
	const int lineSmoothing = configHandler->Get("SmoothLines", defaultSmooth);
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
	const int pointSmoothing = configHandler->Get("SmoothPoints", defaultSmooth);
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
	const float lodBias = Clamp(configHandler->Get("TextureLODBias", 0.0f), -4.f, 4.f);
	if (fabs(lodBias)>0.01f) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
	}

	//! there must be a way to see if this is necessary, compare old/new context pointers?
	if (!!configHandler->Get("FixAltTab", 0)) {
		//! free GL resources
		GLContext::Free();

		//! initialize any GL resources that were lost
		GLContext::Init();
	}

	VSync.Init();

	int bits;
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bits);
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
	if (globalRendering->fullScreen) {
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
	SetupViewportGeometry();
	glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	gluPerspective(45.0f,  globalRendering->aspectRatio, 2.8f, CGlobalRendering::MAX_VIEW_RANGE);

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}


void SpringApp::UpdateOldConfigs()
{
	const int cfgVersion = configHandler->Get("Version",0);
	if (cfgVersion < 2) {
		// force an update to new defaults
		configHandler->Delete("FontFile");
		configHandler->Delete("FontSize");
		configHandler->Delete("SmallFontSize");
		configHandler->Delete("StencilBufferBits"); //! update to 8 bits
		configHandler->Delete("DepthBufferBits"); //! update to 24bits
		configHandler->Set("Version",2);
	}
	if (cfgVersion < 3) {
		configHandler->Delete("AtiHacks"); //! new runtime detection with -1
		configHandler->Set("Version",3);
	}
	if (cfgVersion < 4) {
		const bool fsaaEnabled = configHandler->Get("FSAA", false);
		if (!fsaaEnabled)
			configHandler->Set("FSAALevel", 0);
		configHandler->Delete("FSAA");
		configHandler->Set("Version",4);
	}
	if (cfgVersion < 5) {
		const int xres = configHandler->Get("XResolution", 0);
		const int yres = configHandler->Get("YResolution", 0);
		if ((xres == 1024) && (yres == 768)) { //! old default res (use desktop res now by default)
			configHandler->Delete("XResolution");
			configHandler->Delete("YResolution");
		}
		configHandler->Set("Version",5);
	}
}


void SpringApp::LoadFonts()
{
	// Initialize font
	const std::string fontFile = configHandler->GetString("FontFile", "fonts/FreeSansBold.otf");
	const std::string smallFontFile = configHandler->GetString("SmallFontFile", "fonts/FreeSansBold.otf");
	const int fontSize = configHandler->Get("FontSize", 23);
	const int smallFontSize = configHandler->Get("SmallFontSize", 14);
	const int outlineWidth = configHandler->Get("FontOutlineWidth", 3);
	const float outlineWeight = configHandler->Get("FontOutlineWeight", 25.0f);
	const int smallOutlineWidth = configHandler->Get("SmallFontOutlineWidth", 2);
	const float smallOutlineWeight = configHandler->Get("SmallFontOutlineWeight", 10.0f);

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
	cmdline->AddSwitch(0,   "list-ai-interfaces", "Dump a list of available AI Interfaces to stdout");
	cmdline->AddSwitch(0,   "list-skirmish-ais",  "Dump a list of available Skirmish AIs to stdout");

	try {
		cmdline->Parse();
	} catch (const std::exception& err) {
		std::cerr << err.what() << std::endl << std::endl;
		cmdline->PrintUsage("Spring", SpringVersion::GetFull());
		exit(1);
	}

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->IsSet("help")) {
		cmdline->PrintUsage("Spring",SpringVersion::GetFull());
		exit(0);
	} else if (cmdline->IsSet("version")) {
		std::cout << "Spring " << SpringVersion::GetFull() << std::endl;
		exit(0);
	} else if (cmdline->IsSet("projectiledump")) {
		CCustomExplosionGenerator::OutputProjectileClassInfo();
		exit(0);
	}

	if (cmdline->IsSet("config")) {
		string configSource = cmdline->GetString("config");
		LOG("using configuration source \"%s\"",
				ConfigHandler::Instantiate(configSource).c_str());
	} else {
		LOG("using default configuration source \"%s\"",
				ConfigHandler::Instantiate().c_str());
	}
	GlobalConfig::Instantiate();

	// mutually exclusive options that cause spring to quit immediately
	// and require the configHandler
	if (cmdline->IsSet("list-ai-interfaces")) {
		IAILibraryManager::OutputAIInterfacesInfo();
		exit(0);
	} else if (cmdline->IsSet("list-skirmish-ais")) {
		IAILibraryManager::OutputSkirmishAIInfo();
		exit(0);
	}

#ifdef _DEBUG
	globalRendering->fullScreen = false;
#else
	globalRendering->fullScreen = !!configHandler->Get("Fullscreen", 1);
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

	globalRendering->viewSizeX = configHandler->Get("XResolution", 0);
	if (cmdline->IsSet("xresolution"))
		globalRendering->viewSizeX = std::max(cmdline->GetInt("xresolution"), 640);

	globalRendering->viewSizeY = configHandler->Get("YResolution", 0);
	if (cmdline->IsSet("yresolution"))
		globalRendering->viewSizeY = std::max(cmdline->GetInt("yresolution"), 480);

	globalRendering->winPosX  = configHandler->Get("WindowPosX", 32);
	globalRendering->winPosY  = configHandler->Get("WindowPosY", 32);
	globalRendering->winState = configHandler->Get("WindowState", 0);

#ifdef USE_GML
	gmlThreadCountOverride = configHandler->Get("HardwareThreadCount", 0);
	gmlThreadCount=GML_CPU_COUNT;
#if GML_ENABLE_SIM
	extern volatile int gmlMultiThreadSim;
	gmlMultiThreadSim=configHandler->Get("MultiThreadSim", 1);
#endif
#endif
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
		std::string demoPlayerName = configHandler->GetString("name", "UnnamedPlayer");

		if (demoPlayerName.empty()) {
			demoPlayerName = "UnnamedPlayer";
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
		startsetup->myPlayerName = configHandler->GetString("name", "unnamed");
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

bool SpringApp::UpdateSim(CGameController *ac)
{
	GML_MSTMUTEX_LOCK(sim); // UpdateSim

	Threading::SetSimThread(true);
	bool ret = ac->Update();
	Threading::SetSimThread(false);
	return ret;
}

#if defined(USE_GML) && GML_ENABLE_SIM
volatile int gmlMultiThreadSim;
volatile int gmlStartSim;
volatile int SpringApp::gmlKeepRunning = 0;

int SpringApp::Sim()
{
	try {
		if(GML_SHARE_LISTS)
			ogc->WorkerThreadPost();

		Watchdog::ClearTimer(WDT_SIM, true);

		while(gmlKeepRunning && !gmlStartSim)
			SDL_Delay(100);

		Watchdog::ClearTimer(WDT_SIM);

		while(gmlKeepRunning) {
			if(!gmlMultiThreadSim) {
				Watchdog::ClearTimer(WDT_SIM, true);
				while(!gmlMultiThreadSim && gmlKeepRunning)
					SDL_Delay(200);
			}
			else if (activeController) {
				Watchdog::ClearTimer(WDT_SIM);
				gmlProcessor->ExpandAuxQueue();

				if(!UpdateSim(activeController))
					return 0;

				gmlProcessor->GetQueue();
			}
			boost::thread::yield();
		}

		if(GML_SHARE_LISTS)
			ogc->WorkerThreadFree();
	} CATCH_SPRING_ERRORS

	return 1;
}
#endif




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
#if defined(USE_GML) && GML_ENABLE_SIM
		if (gmlMultiThreadSim) {
			if (!gs->frameNum) {
				ret = UpdateSim(activeController);
				if (gs->frameNum) {
					gmlStartSim = 1;
				}
			}
		} else {
			ret = UpdateSim(activeController);
		}
#else
		ret = UpdateSim(activeController);
#endif
		if (ret) {
			globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);

			if (
#if defined(USE_GML) && GML_ENABLE_SIM
				!gmlMultiThreadSim &&
#endif
				(gs->frameNum - lastRequiredDraw) >= GAME_SPEED/float(gu->minFPS) * gs->userSpeedFactor)
			{
				ScopedTimer cputimer("GameController::Draw"); // Update

				ret = activeController->Draw();
				lastRequiredDraw = gs->frameNum;
			} else {
				ret = activeController->Draw();
			}
#if defined(USE_GML) && GML_ENABLE_SIM
			gmlProcessor->PumpAux();
#endif
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

	if (!Initialize())
		return -1;

#ifdef USE_GML
	gmlProcessor = new gmlClientServer<void, int, CUnit*>;
#	if GML_ENABLE_SIM
	gmlKeepRunning = 1;
	gmlStartSim = 0;

	if (GML_SHARE_LISTS)
		ogc = new COffscreenGLContext();
	gmlProcessor->AuxWork(&SpringApp::Simcb, this); // start sim thread
#	endif
#endif

	while (!gu->globalQuit) {
		ResetScreenSaverTimeout();

		{
			SCOPED_TIMER("InputHandler::PushEvents");
			SDL_Event event;

			while (SDL_PollEvent(&event)) {
				input.PushEvent(event);
			}
		}

		if (!Update())
			break;
	}

	SaveWindowPosition();

	// Shutdown
	Shutdown();
	return 0;
}



/**
 * Deallocates and shuts down game
 */
void SpringApp::Shutdown()
{
	if (gu) gu->globalQuit = true;

#define DeleteAndNull(x) delete x; x = NULL;

#ifdef USE_GML
	if(gmlProcessor) {
#if GML_ENABLE_SIM
		gmlKeepRunning=0; // wait for sim to finish
		while(!gmlProcessor->PumpAux())
			boost::thread::yield();
		if(GML_SHARE_LISTS)
			DeleteAndNull(ogc);
#endif
		DeleteAndNull(gmlProcessor);
	}
#endif

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

	FileSystemHandler::Cleanup();

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
				mouse->ToggleState();
			}

			//! release all keyboard keys
			if ((event.active.state & (SDL_APPACTIVE | SDL_APPINPUTFOCUS)) && !event.active.gain) {
				for (boost::uint16_t i = 1; i < SDLK_LAST; ++i) {
					if (keyInput->IsKeyPressed(i)) {
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
			}

			//! simulate mouse release to prevent hung buttons
			if ((event.active.state & (SDL_APPACTIVE | SDL_APPMOUSEFOCUS)) && !event.active.gain) {
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
				if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
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
