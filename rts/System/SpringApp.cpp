/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include <iostream>

#include <signal.h>
#include <SDL.h>
#if !defined(HEADLESS)
	#include <SDL_syswm.h>
#endif

#include "mmgr.h"

#include "SpringApp.h"

#undef KeyPress
#undef KeyRelease

#include "aGui/Gui.h"
#include "ExternalAI/IAILibraryManager.h"
#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/ClientSetup.h"
#include "Game/GameController.h"
#include "Game/PreGame.h"
#include "Game/Game.h"
#include "Game/LoadScreen.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Input/MouseInput.h"
#include "Input/InputHandler.h"
#include "Input/Joystick.h"
#include "Lua/LuaOpenGL.h"
#include "Menu/SelectMenu.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/glFont.h"
#include "Rendering/GLContext.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/WindowManagerHelper.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TAPalette.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "System/bitops.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FPUCheck.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/FileSystem/FileSystemHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/BaseCmd.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/Sound/ISound.h"

#include "mmgr.h"

#ifdef WIN32
	#include "Platform/Win/win32.h"
	#include "Platform/Win/WinVersion.h"
#elif defined(__APPLE__) || defined(HEADLESS)
#else
	#include <X11/Xlib.h>
	#include <sched.h>
	#include "Platform/Linux/myX11.h"
#endif

#ifdef USE_GML
	#include "lib/gml/gmlsrv.h"
	extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

using std::string;

volatile bool globalQuit = false;

CGameController* activeController = NULL;
ClientSetup* startsetup = NULL;

boost::uint8_t* keys = 0;
boost::uint16_t currentUnicode = 0;

COffscreenGLContext *SpringApp::ogc = NULL;

/**
 * @brief xres default
 *
 * Defines the default X resolution as 1024
 */
const int XRES_DEFAULT = 1024;

/**
 * @brief yres default
 *
 * Defines the default Y resolution as 768
 */
const int YRES_DEFAULT = 768;


/**
 * Initializes SpringApp variables
 */
SpringApp::SpringApp()
{
	cmdline = NULL;
	screenWidth = screenHeight = 0;
	FSAA = false;
	lastRequiredDraw = 0;
	depthBufferBits = 24;
	windowState = 0;
	windowPosX = windowPosY = 0;
	ogc = NULL;
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	delete cmdline;
	delete[] keys;

	creg::System::FreeClasses();
}

/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize()
{
#if defined(_WIN32) && defined(__GNUC__)
	// load QTCreator's gdb helper dll; a variant of this should also work on other OSes
	{
		// don't display a dialog box if gdb helpers aren't found
		UINT olderrors = SetErrorMode(SEM_FAILCRITICALERRORS);
		if(LoadLibrary("gdbmacros.dll")) {
			LogObject() << "QT Creator's gdbmacros.dll loaded";
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
	// TODO: improve version logging of non-Windows OSes
	logOutput.Print("OS: %s", Platform::GetOS().c_str());
	if (Platform::Is64Bit())
		logOutput.Print("OS: 64bit native mode");
	else if (Platform::Is32BitEmulation())
		logOutput.Print("OS: emulated 32bit mode");
	else
		logOutput.Print("OS: 32bit native mode");

	FileSystemHandler::Initialize(true);

	UpdateOldConfigs();
	
	if (!InitWindow(("Spring " + SpringVersion::Get()).c_str())) {
		SDL_Quit();
		return false;
	}

	mouseInput = IMouseInput::Get();

	// Global structures
	gs = new CGlobalSynced();
	gu = new CGlobalUnsynced();

	if (cmdline->IsSet("minimise")) {
		globalRendering->active = false;
		SDL_WM_IconifyWindow();
	}

	InitOpenGL();
	agui::InitGui();
	palette.Init();

	// Initialize keyboard
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_SetModState(KMOD_NONE);

	input.AddHandler(boost::bind(&SpringApp::MainEventHandler, this, _1));
	keys = new boost::uint8_t[SDLK_LAST];
	memset(keys,0,sizeof(boost::uint8_t)*SDLK_LAST);

	LoadFonts();

	// Initialize GLEW
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	gu->PostInit();
	globalRendering->PostInit();

	// Initialize named texture handler
	CNamedTextures::Init();

	// Initialize Lua GL
	LuaOpenGL::Init();

	// Sound
	ISound::Initialize();

	SetProcessAffinity(configHandler->Get("SetCoreAffinity", 0));

	InitJoystick();
	// Create CGameSetup and CPreGame objects
	Startup();

	return true;
}

void SpringApp::SetProcessAffinity(int affinity) const {
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
		if (affinity==1) {
			result = SetThreadIdealProcessor(thread,(DWORD)wantedCore);
		} else if (affinity>=2) {
			result = SetThreadAffinityMask(thread,wantedCore);
		}

		if (result > 0) {
			logOutput.Print("CPU: affinity set (%d)", affinity);
		} else {
			logOutput.Print("CPU: affinity failed");
		}
	}
#elif defined(__APPLE__)
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
		logOutput.Print("[SetProcessAffinity(%d)] available system CPU's: %d", affinity, CPU_COUNT(&cpusSystem));

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
 * @brief multisample test
 * @return whether the multisampling test was successful
 *
 * Tests if a user has requested FSAA, if it's a valid
 * FSAA mode, and if it's supported by the current GL context
 */
static bool MultisampleTest(void)
{
	if (!GL_ARB_multisample)
		return false;
	GLuint fsaa = configHandler->Get("FSAA",0);
	if (!fsaa)
		return false;
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
	GLuint fsaalevel = std::max(std::min(configHandler->Get("FSAALevel", 2), 8), 0);

	make_even_number(fsaalevel);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,fsaalevel);
	return true;
}

/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
static bool MultisampleVerify(void)
{
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
		logOutput.Print("Could not initialize SDL: %s", SDL_GetError());
		return false;
	}

	PrintAvailableResolutions();

	WindowManagerHelper::SetCaption(title);

	if (!SetSDLVideoMode()) {
		logOutput.Print("Failed to set SDL video mode: %s", SDL_GetError());
		return false;
	}

	RestoreWindowPosition();

	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
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

	//! w/o SDL_NOFRAME, kde's windowmanager still creates a border and force a `window`-resize causing a lot of trouble
	sdlflags |= globalRendering->fullScreen ? SDL_FULLSCREEN | SDL_NOFRAME : 0;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // enable alpha channel

	depthBufferBits = configHandler->Get("DepthBufferBits", 24);
	if (globalRendering) globalRendering->depthBufferBits = depthBufferBits;

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBufferBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, configHandler->Get("StencilBufferBits", 8));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	FSAA = MultisampleTest();

	// screen will be freed by SDL_Quit()
	// from: http://sdl.beuc.net/sdl.wiki/SDL_SetVideoMode
	// Note 3: This function should be called in the main thread of your application.
	// User note 1: Some have found that enabling OpenGL attributes like SDL_GL_STENCIL_SIZE (the stencil buffer size) before the video mode has been set causes the application to simply ignore those attributes, while enabling attributes after the video mode has been set works fine.
	// User note 2: Also note that, in Windows, setting the video mode resets the current OpenGL context. You must execute again the OpenGL initialization code (set the clear color or the shade model, or reload textures, for example) after calling SDL_SetVideoMode. In Linux, however, it works fine, and the initialization code only needs to be executed after the first call to SDL_SetVideoMode (although there is no harm in executing the initialization code after each call to SDL_SetVideoMode, for example for a multiplatform application). 
	SDL_Surface* screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, sdlflags);
	if (!screen) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Could not set video mode:\n%s", SDL_GetError());
		handleerror(NULL, buf, "ERROR", MBF_OK|MBF_EXCL);
		return false;
	}

#ifdef STREFLOP_H
	// Something in SDL_SetVideoMode (OpenGL drivers?) messes with the FPU control word.
	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
#endif

	if (FSAA) {
 		FSAA = MultisampleVerify();
	}

	// setup GL smoothing
	const int defaultSmooth = 0; // FSAA ? 0 : 3;  // until a few things get fixed
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

	// setup LOD bias factor
	const float lodBias = std::max(std::min( configHandler->Get("TextureLODBias", 0.0f) , 4.0f), -4.0f);
	if (fabs(lodBias)>0.01f) {
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
	}

	// there must be a way to see if this is necessary, compare old/new context pointers?
	if (!!configHandler->Get("FixAltTab", 0)) {
		// free GL resources
		GLContext::Free();

		// initialize any GL resources that were lost
		GLContext::Init();
	}

	VSync.Init();

	int bits;
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bits);
	if (globalRendering->fullScreen) {
		logOutput.Print("Video mode set to %ix%i/%ibit", screenWidth, screenHeight, bits);
	} else {
		logOutput.Print("Video mode set to %ix%i/%ibit (windowed)", screenWidth, screenHeight, bits);
	}

	return true;
}


// origin for our coordinates is the bottom left corner
bool SpringApp::GetDisplayGeometry()
{
#if       defined(HEADLESS)
	return false;

#else  // defined(HEADLESS)
	//! not really needed, but makes it safer against unknown windowmanager behaviours
	if (globalRendering->fullScreen) {
		windowPosX = 0;
		windowPosY = 0;
		globalRendering->screenSizeX = screenWidth;
		globalRendering->screenSizeY = screenHeight;
		globalRendering->winSizeX = globalRendering->screenSizeX;
		globalRendering->winSizeY = globalRendering->screenSizeY;
		globalRendering->winPosX = windowPosX;
		globalRendering->winPosY = globalRendering->screenSizeY - globalRendering->winSizeY - windowPosY; //! origin BOTTOMLEFT
		return true;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		return false;
	}


  #if       defined(__APPLE__)
	// TODO: implement this function & RestoreWindowPosition() on Mac
	windowPosX = 30;
	windowPosY = 30;
	windowState = 0;

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

	windowPosX = rect.left;
	windowPosY = rect.top;

	// Get WindowState
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(info.window, &wp)) {
		switch (wp.showCmd) {
			case SW_SHOWMAXIMIZED:
				windowState = 1;
				break;
			case SW_SHOWMINIMIZED: //minimized startup breaks init stuff, so don't store it
			default:
				windowState = 0;
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
		windowPosX = xp;
		windowPosY = yp;

		if (!globalRendering->fullScreen) {
			int frame_left, frame_top;
			MyX11GetFrameBorderOffset(display, window, &frame_left, &frame_top);
			windowPosX -= frame_left;
			windowPosY -= frame_top;
		}

		windowState = MyX11GetWindowState(display, window);
	}
	info.info.x11.unlock_func();

  #endif // defined(__APPLE__)

	globalRendering->winPosX = windowPosX;
	globalRendering->winPosY = globalRendering->screenSizeY - globalRendering->winSizeY - windowPosY; //! origin BOTTOMLEFT

	return true;
#endif // defined(HEADLESS)
}


/**
 * Restores position of the window, if we are not in full-screen mode
 */
void SpringApp::RestoreWindowPosition()
{
#if       defined(HEADLESS)
	return;

#else  // defined(HEADLESS)
	if (!globalRendering->fullScreen) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		if (SDL_GetWMInfo(&info)) {
  #if       defined(WIN32)
			bool stateChanged = false;

			if (windowState > 0) {
				WINDOWPLACEMENT wp;
				memset(&wp,0,sizeof(WINDOWPLACEMENT));
				wp.length = sizeof(WINDOWPLACEMENT);
				stateChanged = true;
				int wState;
				switch (windowState) {
					case 1: wState = SW_SHOWMAXIMIZED; break;
					//Setting the main-window minimized breaks initialization
					case 2: // wState = SW_SHOWMINIMIZED; break;
					default: stateChanged = false;
				}
				if (stateChanged) {
					ShowWindow(info.window, wState);
					GetDisplayGeometry();
				}
			}

			if (!stateChanged) {
				MoveWindow(info.window, windowPosX, windowPosY, screenWidth, screenHeight, true);
			}

  #elif     defined(__APPLE__)
			// TODO: implement this function

  #else
			info.info.x11.lock_func();
			{
				XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, windowPosX, windowPosY);
				MyX11SetWindowState(info.info.x11.display, info.info.x11.wmwindow, windowState);
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
#if       defined(HEADLESS)
	return;

#else
	if (!globalRendering->fullScreen) {
		GetDisplayGeometry();
		configHandler->Set("WindowPosX", windowPosX);
		configHandler->Set("WindowPosY", windowPosY);
		configHandler->Set("WindowState", windowState);

	}
#endif // defined(HEADLESS)
}


void SpringApp::SetupViewportGeometry()
{
	if (!GetDisplayGeometry()) {
		globalRendering->screenSizeX = screenWidth;
		globalRendering->screenSizeY = screenHeight;
		globalRendering->winSizeX = screenWidth;
		globalRendering->winSizeY = screenHeight;
		globalRendering->winPosX = 0;
		globalRendering->winPosY = 0;
	}

	globalRendering->dualScreenMode = !!configHandler->Get("DualScreenMode", 0);
	if (globalRendering->dualScreenMode) {
		globalRendering->dualScreenMiniMapOnLeft =
			!!configHandler->Get("DualScreenMiniMapOnLeft", 0);
	} else {
		globalRendering->dualScreenMiniMapOnLeft = false;
	}

	if (!globalRendering->dualScreenMode) {
		globalRendering->viewSizeX = globalRendering->winSizeX;
		globalRendering->viewSizeY = globalRendering->winSizeY;
		globalRendering->viewPosX = 0;
		globalRendering->viewPosY = 0;
	}
	else {
		globalRendering->viewSizeX = globalRendering->winSizeX / 2;
		globalRendering->viewSizeY = globalRendering->winSizeY;
		if (globalRendering->dualScreenMiniMapOnLeft) {
			globalRendering->viewPosX = globalRendering->winSizeX / 2;
			globalRendering->viewPosY = 0;
		} else {
			globalRendering->viewPosX = 0;
			globalRendering->viewPosY = 0;
		}
	}

	agui::gui->UpdateScreenGeometry(
			globalRendering->viewSizeX,
			globalRendering->viewSizeY,
			globalRendering->viewPosX,
			(globalRendering->winSizeY - globalRendering->viewSizeY - globalRendering->viewPosY) );
	globalRendering->pixelX = 1.0f / (float)globalRendering->viewSizeX;
	globalRendering->pixelY = 1.0f / (float)globalRendering->viewSizeY;

	// NOTE:  globalRendering->viewPosY is not currently used

	globalRendering->aspectRatio = (float)globalRendering->viewSizeX / (float)globalRendering->viewSizeY;
}


/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL()
{
	SetupViewportGeometry();

	glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);

	gluPerspective(45.0f, (GLfloat)globalRendering->viewSizeX / (GLfloat)globalRendering->viewSizeY, 2.8f, MAX_VIEW_RANGE);

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}


void SpringApp::UpdateOldConfigs()
{
	// not very neat, should be done in the installer someday
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
				fi++;
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

	try
	{
		cmdline->Parse();
	}
	catch (const std::exception& err)
	{
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
		logOutput.Print("using configuration source \"" + ConfigHandler::Instantiate(configSource) + "\"");
	} else {
		logOutput.Print("using default configuration source \"" + ConfigHandler::Instantiate() + "\"");
	}

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

	screenWidth = configHandler->Get("XResolution", XRES_DEFAULT);
	if (cmdline->IsSet("xresolution"))
		screenWidth = std::max(cmdline->GetInt("xresolution"), 640);

	screenHeight = configHandler->Get("YResolution", YRES_DEFAULT);
	if (cmdline->IsSet("yresolution"))
		screenHeight = std::max(cmdline->GetInt("yresolution"), 480);

	windowPosX  = configHandler->Get("WindowPosX", 32);
	windowPosY  = configHandler->Get("WindowPosY", 32);
	windowState = configHandler->Get("WindowState", 0);

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
		LogObject()
				<< "The headless version of the engine can not be run in interactive mode.\n"
				<< "Please supply a start-script, save- or demo-file.";
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
		LogObject() << "Loading startscript from: " << inputFile;
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

#if defined(USE_GML) && GML_ENABLE_SIM
volatile int gmlMultiThreadSim;
volatile int gmlStartSim;
volatile int SpringApp::gmlKeepRunning = 0;

int SpringApp::Sim()
{
	try {
		if(GML_SHARE_LISTS)
			ogc->WorkerThreadPost();

		while(gmlKeepRunning && !gmlStartSim)
			SDL_Delay(100);

		while(gmlKeepRunning) {
			if(!gmlMultiThreadSim) {
				CrashHandler::ClearSimWDT(true);
				while(!gmlMultiThreadSim && gmlKeepRunning)
					SDL_Delay(200);
			}
			else if (activeController) {
				CrashHandler::ClearSimWDT();
				gmlProcessor->ExpandAuxQueue();

				{
					GML_MSTMUTEX_LOCK(sim); // Sim

					if(!activeController->Update())
						return 0;
				}

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
	if (FSAA)
		glEnable(GL_MULTISAMPLE_ARB);

	int ret = 1;
	if (activeController) {
		CrashHandler::ClearDrawWDT();
#if defined(USE_GML) && GML_ENABLE_SIM
			if (gmlMultiThreadSim) {
				if (!gs->frameNum) {
					GML_MSTMUTEX_LOCK(sim); // Update

					activeController->Update();
					if (gs->frameNum) {
						gmlStartSim = 1;
					}
				}
			} else {
				GML_MSTMUTEX_LOCK(sim); // Update

				activeController->Update();
			}
#else
		if (!activeController->Update()) {
			ret = 0;
		} else {
#endif

			globalRendering->drawFrame++;
			if (globalRendering->drawFrame == 0) {
				globalRendering->drawFrame++;
			}

			if (
#if defined(USE_GML) && GML_ENABLE_SIM
				!gmlMultiThreadSim &&
#endif
				gs->frameNum-lastRequiredDraw >= (float)MAX_CONSECUTIVE_SIMFRAMES * gs->userSpeedFactor) {

				ScopedTimer cputimer("CPU load"); // Update

				ret = activeController->Draw();
				lastRequiredDraw=gs->frameNum;
			} else {
				ret = activeController->Draw();
			}
#if defined(USE_GML) && GML_ENABLE_SIM
			gmlProcessor->PumpAux();
#else
		}
#endif
	}

	VSync.Delay();
	SDL_GL_SwapBuffers();

	if (FSAA) {
		glDisable(GL_MULTISAMPLE_ARB);
	}

	return ret;
}


/**
 * Tests SDL keystates and sets values
 * in key array
 */
void SpringApp::UpdateSDLKeys()
{
	int numkeys;
	boost::uint8_t *state;
	state = SDL_GetKeyState(&numkeys);
	memcpy(keys, state, sizeof(boost::uint8_t) * numkeys);

	const SDLMod mods = SDL_GetModState();
	keys[SDLK_LALT]   = (mods & KMOD_ALT)   ? 1 : 0;
	keys[SDLK_LCTRL]  = (mods & KMOD_CTRL)  ? 1 : 0;
	keys[SDLK_LMETA]  = (mods & KMOD_META)  ? 1 : 0;
	keys[SDLK_LSHIFT] = (mods & KMOD_SHIFT) ? 1 : 0;
}


static void ResetScreenSaverTimeout()
{
#if   defined(HEADLESS)
	return;
#elif defined(WIN32)
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
	cmdline = new BaseCmd(argc, argv);

	if (!Initialize())
		return -1;

	CrashHandler::InstallHangHandler();

#ifdef USE_GML
	gmlProcessor=new gmlClientServer<void, int, CUnit*>;
#	if GML_ENABLE_SIM
	gmlKeepRunning=1;
	gmlStartSim=0;
	if(GML_SHARE_LISTS)
		ogc = new COffscreenGLContext();
	gmlProcessor->AuxWork(&SpringApp::Simcb,this); // start sim thread
#	endif
#endif
	while (true) // end is handled by globalQuit
	{
		ResetScreenSaverTimeout();

		{
			SCOPED_TIMER("Input");
			SDL_Event event;

			while (SDL_PollEvent(&event)) {
				input.PushEvent(event);
			}
		}

		if (globalQuit)
			break;

		try {
			if (!Update())
				break;
		} catch (content_error &e) {
			//FIXME should this really be in here and not the crashhandler???
			LogObject() << "Caught content exception: " << e.what() << "\n";
			handleerror(NULL, e.what(), "Content error", MBF_OK | MBF_EXCL);
		}
	}

	SaveWindowPosition();

	// Shutdown
	Shutdown();
	return 0;
}


static void normalizeKeySym(int& sym) {

	if (sym <= SDLK_DELETE) {
		sym = tolower(sym);
	}
	else if (sym == SDLK_RSHIFT) { sym = SDLK_LSHIFT; }
	else if (sym == SDLK_RCTRL)  { sym = SDLK_LCTRL;  }
	else if (sym == SDLK_RMETA)  { sym = SDLK_LMETA;  }
	else if (sym == SDLK_RALT)   { sym = SDLK_LALT;   }

	if (keyBindings) {
		const int fakeMetaKey = keyBindings->GetFakeMetaKey();
		if (fakeMetaKey >= 0) {
			keys[SDLK_LMETA] |= keys[fakeMetaKey];
		}
	}
}

/**
 * Deallocates and shuts down game
 */
void SpringApp::Shutdown()
{
#ifdef USE_GML
	if(gmlProcessor) {
#if GML_ENABLE_SIM
		gmlKeepRunning=0; // wait for sim to finish
		while(!gmlProcessor->PumpAux())
			boost::thread::yield();
		if(GML_SHARE_LISTS)
			delete ogc;
#endif
		delete gmlProcessor;
	}
#endif

	CrashHandler::UninstallHangHandler();

	delete pregame;
	delete game;
	delete gameServer;
	delete gameSetup;
	CLoadScreen::DeleteInstance();
	ISound::Shutdown();
	delete font;
	delete smallFont;
	CNamedTextures::Kill();
	GLContext::Free();
	ConfigHandler::Deallocate();
	UnloadExtensions();
	delete mouseInput;
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK);
#endif
	SDL_Quit();
	delete gs;
	delete gu;
	delete startsetup;
#ifdef USE_MMGR
	m_dumpMemoryReport();
#endif
}

bool SpringApp::MainEventHandler(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_VIDEORESIZE: {

			GML_MSTMUTEX_LOCK(sim); // MainEventHandler

			CrashHandler::ClearDrawWDT(true);
			screenWidth = event.resize.w;
			screenHeight = event.resize.h;
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

			CrashHandler::ClearDrawWDT(true);
			// re-initialize the stencil
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
			glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
			SetupViewportGeometry();

			break;
		}
		case SDL_QUIT: {
			globalQuit = true;
			break;
		}
		case SDL_ACTIVEEVENT: {
			CrashHandler::ClearDrawWDT(true);

			if (event.active.state & (SDL_APPACTIVE | (globalRendering->fullScreen ? SDL_APPINPUTFOCUS : 0))) {
				globalRendering->active = !!event.active.gain;
				if (ISound::IsInitialized()) {
					sound->Iconified(!event.active.gain);
				}
			}

			if (mouse && mouse->locked) {
				mouse->ToggleState();
			}

			if ((event.active.state & (SDL_APPACTIVE | SDL_APPINPUTFOCUS)) && !event.active.gain) {
				// simulate release all to prevent hung buttons
				for (int i = 1; i <= NUM_BUTTONS; ++i) {
					SDL_Event event;
					event.type = event.button.type = SDL_MOUSEBUTTONUP;
					event.button.which = 0;
					event.button.button = i;
					event.button.x = -1;
					event.button.y = -1;
					event.button.state = SDL_RELEASED;
					if(!mouse || mouse->buttons[i].pressed)
						input.PushEvent(event);
				}
				// and make sure to un-capture mouse
				if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
					SDL_WM_GrabInput(SDL_GRAB_OFF);
			}

			break;
		}
		case SDL_KEYDOWN: {
			int sym = event.key.keysym.sym;
			currentUnicode = event.key.keysym.unicode;

			const bool isRepeat = !!keys[sym];

			UpdateSDLKeys();

			if (activeController) {
				normalizeKeySym(sym);

				activeController->KeyPressed(sym, isRepeat);

				if (activeController->userWriting){
					// use unicode for printed characters
					sym = event.key.keysym.unicode;
					if ((sym >= SDLK_SPACE) && (sym <= 255)) {
						CGameController* ac = activeController;
						if (ac->ignoreNextChar || (ac->ignoreChar == (char)sym)) {
							ac->ignoreNextChar = false;
						} else {
							if (sym < 255 && (!isRepeat || ac->userInput.length()>0)) {
								const int len = (int)ac->userInput.length();
								ac->writingPos = std::max(0, std::min(len, ac->writingPos));
								char str[2] = { (char)sym, 0 };
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
			int sym = event.key.keysym.sym;
			currentUnicode = event.key.keysym.unicode;

			UpdateSDLKeys();

			if (activeController) {
				normalizeKeySym(sym);

				activeController->KeyReleased(sym);
			}
			break;
		}
		default:
			break;
	}
	return false;
}
