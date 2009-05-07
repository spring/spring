#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include <iostream>

#include <signal.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include "mmgr.h"

#include "SpringApp.h"

#undef KeyPress
#undef KeyRelease

#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameController.h"
#include "Game/SelectMenu.h"
#include "Game/PreGame.h"
#include "Game/Game.h"
#include "Sim/Misc/Team.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaOpenGL.h"
#include "Platform/BaseCmd.h"
#include "ConfigHandler.h"
#include "Platform/errorhandler.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "Rendering/glFont.h"
#include "Rendering/GLContext.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Textures/TAPalette.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Misc/GlobalConstants.h"
#include "LogOutput.h"
#include "MouseInput.h"
#include "bitops.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Exceptions.h"
#include "System/TimeProfiler.h"

#include "mmgr.h"

#ifdef WIN32
#if defined(__GNUC__) || defined(_MSC_VER)
	#include "Platform/Win/CrashHandler.h"
#endif
	#include "Platform/Win/win32.h"
	#include <winreg.h>
	#include <direct.h>
	#include "Platform/Win/seh.h"
	#include "Platform/Win/WinVersion.h"
#endif // WIN32

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

using std::string;

CGameController* activeController = 0;
bool globalQuit = false;
boost::uint8_t *keys = 0;
boost::uint16_t currentUnicode = 0;
bool fullscreen = true;
char *win_lpCmdLine = 0;

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

void SpringApp::SigAbrtHandler(int unused)
{
	// cause an exception if on windows
	// TODO FIXME do a proper stacktrace dump here
	#ifdef WIN32
	*((int*)(0)) = 0;
	#endif
}

/**
 * Initializes SpringApp variables
 */
SpringApp::SpringApp ()
{
	cmdline = 0;
	screenWidth = screenHeight = 0;
	FSAA = false;
	lastRequiredDraw=0;

	signal(SIGABRT, SigAbrtHandler);
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	if (cmdline) delete cmdline;
	if (keys) delete[] keys;

	creg::System::FreeClasses ();
}

#ifdef WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

/** @brief checks if the current process is running in 32bit emulation mode
    @return FALSE, TRUE, -1 on error (usually no permissions) */
static int GetWow64Status()
{
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
		{
			return -1;
		}
	}
	return bIsWow64;
}


#endif


/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize()
{
	// Initialize class system
	creg::System::InitializeClasses();

	// Initialize crash reporting
#ifdef _WIN32
	CrashHandler::Install();
	InitializeSEH();
#endif

	ParseCmdLine();

	logOutput.SetMirrorToStdout(!!configHandler->Get("StdoutDebug",0));

	// log OS version
	// TODO: improve version logging of non-Windows OSes
#if defined(WIN32)
	logOutput.Print("OS: %s\n", GetOSDisplayString().c_str());
	if (GetWow64Status() == TRUE) {
		logOutput.Print("OS: WOW64 detected\n");
	}
	logOutput.Print("Hardware: %s\n", GetHardwareInfoString().c_str());
#elif defined(__linux__)
	logOutput.Print("OS: Linux\n");
#elif defined(__FreeBSD__)
	logOutput.Print("OS: FreeBSD\n");
#elif defined(MACOS_X)
	logOutput.Print("OS: MacOS X\n");
#else
	logOutput.Print("OS: unknown\n");
#endif


	FileSystemHandler::Initialize(true);

	if (!InitWindow(("Spring " + SpringVersion::GetFull()).c_str())) {
		SDL_Quit();
		return false;
	}

	mouseInput = IMouseInput::Get ();

	// Global structures
	gs = new CGlobalSyncedStuff();
	gu = new CGlobalUnsyncedStuff();

	gu->depthBufferBits = depthBufferBits;

	if (cmdline->result("minimise")) {
		gu->active = false;
		SDL_WM_IconifyWindow();
	}

	// Enable auto quit?
	int quit_time;
	if (cmdline->result("quit", quit_time)) {
		gu->autoQuit = true;
		gu->quitTime = quit_time;
	}

	InitOpenGL();
	palette.Init();

	// Initialize keyboard
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_SetModState (KMOD_NONE);

	keys = new boost::uint8_t[SDLK_LAST];
	memset (keys,0,sizeof(boost::uint8_t)*SDLK_LAST);

	LoadFonts();

	// Initialize GLEW
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	// Runtime compress textures?
	if (GLEW_ARB_texture_compression) {
		// we don't even need to check it, 'cos groundtextures must have that extension
		// default to off because it reduces quality (smallest mipmap level is bigger)
		gu->compressTextures = !!configHandler->Get("CompressTextures", 0);
	}

	// use some ATI bugfixes?
	gu->atiHacks = !!configHandler->Get("AtiHacks", (GLEW_ATI_envmap_bumpmap)?1:0 );
	if (gu->atiHacks) {
		logOutput.Print("ATI hacks enabled\n");
	}

	// Initialize named texture handler
	CNamedTextures::Init();

	// Initialize Lua GL
	LuaOpenGL::Init();

	// Create CGameSetup and CPreGame objects
	Startup ();

	return true;
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
	GLint buffers, samples;
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
bool SpringApp::InitWindow (const char* title)
{
	unsigned int sdlInitFlags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
#ifdef WIN32
	// the crash reporter should be catching the errors
	sdlInitFlags |= SDL_INIT_NOPARACHUTE;
#endif
	if ((SDL_Init(sdlInitFlags) == -1)) {
		handleerror(NULL,"Could not initialize SDL.","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}

	// Sets window manager properties
	SDL_WM_SetIcon(SDL_LoadBMP("spring.bmp"),NULL);
	SDL_WM_SetCaption(title, title);

	if (!SetSDLVideoMode ())
		return false;

	return true;
}

/**
 * @return whether setting the video mode was successful
 *
 * Sets SDL video mode options/settings
 */
bool SpringApp::SetSDLVideoMode ()
{
	int sdlflags = SDL_OPENGL | SDL_RESIZABLE;

	//conditionally_set_flag(sdlflags, SDL_FULLSCREEN, fullscreen);
	sdlflags |= fullscreen ? SDL_FULLSCREEN : 0;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // enable alpha channel

	depthBufferBits = configHandler->Get("DepthBufferBits", 32);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBufferBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, configHandler->Get("StencilBufferBits", 1));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	FSAA = MultisampleTest();

	SDL_Surface *screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, sdlflags);
	if (!screen) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "Could not set video mode:\n%s", SDL_GetError());
		handleerror(NULL, buf, "ERROR", MBF_OK|MBF_EXCL);
		return false;
	}

	// set window position
	if (!fullscreen) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		if (SDL_GetWMInfo(&info)) {
#ifdef _WIN32
			MoveWindow(info.window, windowPosX, windowPosY, screenWidth, screenHeight, true);
#endif
		}
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

	// initialize the stencil
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();

	int bits;
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bits);
	logOutput.Print("Video mode set to  %i x %i / %i bit", screenWidth, screenHeight, bits );
	VSync.Init();

	return true;
}


// origin for our coordinates is the bottom left corner
bool SpringApp::GetDisplayGeometry()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		return false;
	}

#ifdef __APPLE__
	return false;
#elif !defined(_WIN32)
	info.info.x11.lock_func();
	{
		Display* display = info.info.x11.display;
		Window   window  = info.info.x11.window;

		XWindowAttributes attrs;
		XGetWindowAttributes(display, window, &attrs);
		const Screen* screen = attrs.screen;
		gu->screenSizeX = WidthOfScreen(screen);
		gu->screenSizeY = HeightOfScreen(screen);
		gu->winSizeX = attrs.width;
		gu->winSizeY = attrs.height;

		Window tmp;
		int xp, yp;
		XTranslateCoordinates(display, window, attrs.root, 0, 0, &xp, &yp, &tmp);
		gu->winPosX = xp;
		gu->winPosY = gu->screenSizeY - gu->winSizeY - yp;
	}
	info.info.x11.unlock_func();
#else
	gu->screenSizeX = GetSystemMetrics(SM_CXFULLSCREEN);
	gu->screenSizeY = GetSystemMetrics(SM_CYFULLSCREEN);

	RECT rect;
	if (!GetClientRect(info.window, &rect)) {
		return false;
	}

	if((rect.right - rect.left)==0 || (rect.bottom - rect.top)==0)
		return false;

	gu->winSizeX = rect.right - rect.left;
	gu->winSizeY = rect.bottom - rect.top;

	// translate from client coords to screen coords
	MapWindowPoints(info.window, HWND_DESKTOP, (LPPOINT)&rect, 2);
	gu->winPosX = rect.left;
	gu->winPosY = gu->screenSizeY - gu->winSizeY - rect.top;

	// get window position
	if (!fullscreen && GetWindowRect(info.window, &rect)) {
		windowPosX = rect.left;
		windowPosY = rect.top;
		configHandler->Set("WindowPosX", windowPosX);
		configHandler->Set("WindowPosY", windowPosY);
	}
#endif // _WIN32
	return true;
}


void SpringApp::SetupViewportGeometry()
{
	if (!GetDisplayGeometry()) {
		gu->screenSizeX = screenWidth;
		gu->screenSizeY = screenHeight;
		gu->winSizeX = screenWidth;
		gu->winSizeY = screenHeight;
		gu->winPosX = 0;
		gu->winPosY = 0;
	}

	gu->dualScreenMode = !!configHandler->Get("DualScreenMode", 0);
	if (gu->dualScreenMode) {
		gu->dualScreenMiniMapOnLeft =
			!!configHandler->Get("DualScreenMiniMapOnLeft", 0);
	} else {
		gu->dualScreenMiniMapOnLeft = false;
	}

	if (!gu->dualScreenMode) {
		gu->viewSizeX = gu->winSizeX;
		gu->viewSizeY = gu->winSizeY;
		gu->viewPosX = 0;
		gu->viewPosY = 0;
	}
	else {
		gu->viewSizeX = gu->winSizeX / 2;
		gu->viewSizeY = gu->winSizeY;
		if (gu->dualScreenMiniMapOnLeft) {
			gu->viewPosX = gu->winSizeX / 2;
			gu->viewPosY = 0;
		} else {
			gu->viewPosX = 0;
			gu->viewPosY = 0;
		}
	}

	gu->pixelX = 1.0f / (float)gu->viewSizeX;
	gu->pixelY = 1.0f / (float)gu->viewSizeY;

	// NOTE:  gu->viewPosY is not currently used

	gu->aspectRatio = (float)gu->viewSizeX / (float)gu->viewSizeY;
}


/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL ()
{
	SetupViewportGeometry();

	glViewport(gu->viewPosX, gu->viewPosY, gu->viewSizeX, gu->viewSizeY);

	gluPerspective(45.0f, (GLfloat)gu->viewSizeX / (GLfloat)gu->viewSizeY, 2.8f, MAX_VIEW_RANGE);

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}


void SpringApp::LoadFonts()
{
	// Initialize font
	const std::string fontFile = configHandler->GetString("FontFile", "fonts/Luxi.ttf");
	const std::string smallFontFile = configHandler->GetString("SmallFontFile", "fonts/Luxi.ttf");
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
}

/**
 * @return whether commandline parsing was successful
 *
 * Parse command line arguments
 */
void SpringApp::ParseCmdLine()
{
	cmdline->addoption('f', "fullscreen",         OPTPARM_NONE,   "",  "Run in fullscreen mode");
	cmdline->addoption('w', "window",             OPTPARM_NONE,   "",  "Run in windowed mode");
	cmdline->addoption('x', "xresolution",        OPTPARM_INT,    "",  "Set X resolution");
	cmdline->addoption('y', "yresolution",        OPTPARM_INT,    "",  "Set Y resolution");
	cmdline->addoption('m', "minimise",           OPTPARM_NONE,   "",  "Start minimised");
	cmdline->addoption('s', "server",             OPTPARM_NONE,   "",  "Run as a server");
	cmdline->addoption('c', "client",             OPTPARM_NONE,   "",  "Run as a client");
	cmdline->addoption('p', "projectiledump",     OPTPARM_NONE,   "",  "Dump projectile class info in projectiles.txt");
	cmdline->addoption('t', "textureatlas",       OPTPARM_NONE,   "",  "Dump each finalized textureatlas in textureatlasN.tga");
	cmdline->addoption('q', "quit",               OPTPARM_INT,    "T", "Quit immediately on game over or after T seconds");
	cmdline->addoption('n', "name",               OPTPARM_STRING, "",  "Set your player name");
	cmdline->addoption('C', "config",             OPTPARM_STRING, "",  "Configuration file");
	cmdline->addoption(0,   "list-ai-interfaces", OPTPARM_NONE,   "",  "Dump a list of available AI Interfaces to stdout");
	cmdline->addoption(0,   "list-skirmish-ais",  OPTPARM_NONE,   "",  "Dump a list of available Skirmish AIs to stdout");
//	cmdline->addoption(0,   "list-group-ais",     OPTPARM_NONE,   "",  "Dump a list of available Group AIs to stdout");
	cmdline->parse();

	string configSource;
	cmdline->result("config", configSource);
	// it is not allowed to use configHandler before this line runs.
	logOutput.Print("using configuration source \"" + ConfigHandler::Instantiate(configSource) + "\"");

#ifdef _DEBUG
	fullscreen = false;
#else
	fullscreen = configHandler->Get("Fullscreen", 1) != 0;
#endif

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->result("help")) {
		cmdline->usage("Spring",SpringVersion::GetFull());
		exit(0);
	} else if (cmdline->result("version")) {
		std::cout << "Spring " << SpringVersion::GetFull() << std::endl;
		exit(0);
	} else if (cmdline->result("projectiledump")) {
		CCustomExplosionGenerator::OutputProjectileClassInfo();
		exit(0);
	} else if (cmdline->result("list-ai-interfaces")) {
		IAILibraryManager::OutputAIInterfacesInfo();
		exit(0);
	} else if (cmdline->result("list-skirmish-ais")) {
		IAILibraryManager::OutputSkirmishAIInfo();
		exit(0);
//	} else if (cmdline->result("list-group-ais")) {
//		IAILibraryManager::OutputGroupAIInfo();
//		exit(0);
	}

	// flags
	if (cmdline->result("window")) {
		fullscreen = false;
	} else if (cmdline->result("fullscreen")) {
		fullscreen = true;
	}

	if (cmdline->result("textureatlas")) {
		CTextureAtlas::debug = true;
	}

	string name;
	if (cmdline->result("name", name)) {
		configHandler->SetString("name", name);
	}


	if (!cmdline->result("xresolution", screenWidth)) {
		screenWidth = configHandler->Get("XResolution", XRES_DEFAULT);
	} else {
		screenWidth = std::max(screenWidth, 1);
	}

	if (!cmdline->result("yresolution", screenHeight)) {
		screenHeight = configHandler->Get("YResolution", YRES_DEFAULT);
	} else {
		screenHeight = std::max(screenHeight, 1);
	}

	windowPosX = configHandler->Get("WindowPosX", 32);
	windowPosY = configHandler->Get("WindowPosY", 32);

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
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Checks if a demo SDF file or a startscript has
 * been specified on the command line
 */
void SpringApp::CheckCmdLineFile(int argc, char *argv[])
{
	// Check if the commandline parameter is specifying a demo file
#ifdef _WIN32
	// find the correct dir
	char exe[128];
	GetModuleFileName(0, exe, sizeof(exe));
	int a,s = strlen(exe);
	for (a=s-1;a>0;a--)
	    // Running in gdb causes the last dir separator to be a forward slash.
		if (exe[a] == '\\' || exe[a] == '/') break;
	if (a > 0) {
		string path(exe, exe+a);
		if (path.at(0) == '"')
			path.append(1,'"');
		_chdir(path.c_str());
	}

	// If there are any options, they will start before the demo file name.

	if (win_lpCmdLine == 0) {
		logOutput.Print("ERROR: invalid commandline ptr");
	}

	string cmdLineStr = win_lpCmdLine;
	string::size_type offset = 0;
	//Simply assumes that any argument coming after a argument starting with /q is a variable to /q.
	for(int i=1; i < argc && (argv[i][0] == '/' || (argv[i-1][0] == '/'
			&& (argv[i-1][1] == 'q' || argv[i-1][1] == 'a'
				|| argv[i-1][1] == 'C' || argv[i-1][1] == 'x'
				|| argv[i-1][1] == 'y' || argv[i-1][1] == 'n'))); i++){
		offset += strlen(argv[i]);
		offset = cmdLineStr.find_first_not_of(' ', offset);
		if(offset == string::npos)
			break;
	}

	string command;
	if(offset != string::npos)
		command = cmdLineStr.substr(offset);


	if (!command.empty()) {
		if (command[0] == '"' && *command.rbegin() == '"' && command.length() > 2)
			command = command.substr(1, command.length()-2);
		if (command.rfind("sdf") == command.size()-3) {
			demofile = command;
			LogObject() << "Using demofile " << demofile.c_str() << "\n";
		} else if (command.rfind("ssf") == command.size()-3) {
			savefile = command;
			LogObject() << "Using savefile " << savefile.c_str() << "\n";
		} else {
			startscript = command;
			LogObject() << "Using script " << startscript.c_str() << "\n";
		}
	}
#else
	// is there a reason for not using this in windows?
	for (int i = 1; i < argc; i++)
		if (argv[i][0] != '-')
		{
			string command(argv[i]);
			if (command.rfind("sdf") == command.size() - 3) {
				demofile = command;
				LogObject() << "Using demofile " << demofile.c_str() << "\n";
			} else if (command.rfind("ssf") == command.size() - 3) {
				savefile = command;
				LogObject() << "Using savefile " << savefile.c_str() << "\n";
			} else {
				startscript = command;
				LogObject() << "Using script " << startscript.c_str() << "\n";
			}
		}
#endif
}

/**
 * Initializes instance of GameSetup
 */
void SpringApp::Startup()
{
	ClientSetup* startsetup = 0;
	startsetup = new ClientSetup();
	if (!startscript.empty())
	{
		CFileHandler fh(startscript);
		if (!fh.FileExists())
			throw content_error("Setupscript doesn't exists in given location: "+startscript);

		std::string buf;
		if (!fh.LoadStringData(buf))
			throw content_error("Setupscript cannot be read: "+startscript);
		startsetup->Init(buf);

		// commandline parameters overwrite setup
		if (cmdline->result("client"))
			startsetup->isHost = false;
		else if (cmdline->result("server"))
			startsetup->isHost = true;

#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(startsetup->isHost, 64); //FIXME: add actual number of player
#endif
		pregame = new CPreGame(startsetup);
		if (startsetup->isHost)
			pregame->LoadSetupscript(buf);
	}
	else if (!demofile.empty())
	{
		startsetup->isHost = true; // local demo play
		startsetup->myPlayerName = configHandler->GetString("name", "unnamed")+ " (spec)";
#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(true, 64); //FIXME: add actual number of player
#endif
		pregame = new CPreGame(startsetup);
		pregame->LoadDemo(demofile);
	}
	else if (!savefile.empty())
	{
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
		bool server = !cmdline->result("client") || cmdline->result("server");
#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Initialize(server, 64);
#endif
		activeController = new SelectMenu(server);
	}
}




#if defined(USE_GML) && GML_ENABLE_SIM
volatile int gmlMultiThreadSim;
volatile int gmlStartSim;

int SpringApp::Sim() {
	while(gmlKeepRunning && !gmlStartSim)
		SDL_Delay(100);
	while(gmlKeepRunning) {
		if(!gmlMultiThreadSim) {
			while(!gmlMultiThreadSim && gmlKeepRunning)
				SDL_Delay(200);
		}
		else if (activeController) {
			gmlProcessor->ExpandAuxQueue();

			{
				GML_STDMUTEX_LOCK(sim); // Sim

				if (!activeController->Update()) {
					return 0;
				}
			}

			gmlProcessor->GetQueue();
		}
		boost::thread::yield();
	}
	return 1;
}
#endif




/**
 * @return return code of activecontroller draw function
 *
 * Draw function repeatedly called, it calls all the
 * other draw functions
 */
int SpringApp::Update ()
{
	if (FSAA)
		glEnable(GL_MULTISAMPLE_ARB);

	mouseInput->Update();

	int ret = 1;
	if (activeController) {
#if !defined(USE_GML) || !GML_ENABLE_SIM
		if (!activeController->Update()) {
			ret = 0;
		} else {
#else
			if(gmlMultiThreadSim) {
				if(!gs->frameNum) {

					GML_STDMUTEX_LOCK(sim); // Update

					activeController->Update();
					if(gs->frameNum)
						gmlStartSim=1;
				}
			}
			else {

				GML_STDMUTEX_LOCK(sim); // Update

				activeController->Update();
			}
#endif
			gu->drawFrame++;
			if (gu->drawFrame == 0) {
				gu->drawFrame++;
			}
			if(
#if defined(USE_GML) && GML_ENABLE_SIM
				!gmlMultiThreadSim &&
#endif
				gs->frameNum-lastRequiredDraw >= (float)MAX_CONSECUTIVE_SIMFRAMES * gs->userSpeedFactor) {

				ScopedTimer cputimer("CPU load"); // Update

				ret = activeController->Draw();
				lastRequiredDraw=gs->frameNum;
			}
			else {
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

	if (FSAA)
		glDisable(GL_MULTISAMPLE_ARB);

	return ret;
}


/**
 * Tests SDL keystates and sets values
 * in key array
 */
void SpringApp::UpdateSDLKeys ()
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

/**
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Executes the application
 * (contains main game loop)
 */
int SpringApp::Run (int argc, char *argv[])
{
	CheckCmdLineFile (argc, argv);
	cmdline = BaseCmd::initialize(argc,argv);
/*
	logOutput.Print ("Testing error catching");
	try {
		int *i_=0;
		*i_=1;
		logOutput.Print ("Error catching doesn't work!");
	} catch(...) {
		logOutput.Print ("Error catching works!");
	}
*/
	if (!Initialize ())
		return -1;

#ifdef WIN32
	//SDL_EventState (SDL_SYSWMEVENT, SDL_ENABLE);
#endif

#ifdef USE_GML
	gmlProcessor=new gmlClientServer<void, int,CUnit*>;
#	if GML_ENABLE_SIM
	gmlKeepRunning=1;
	gmlStartSim=0;
	gmlProcessor->AuxWork(&SpringApp::Simcb,this); // start sim thread
#	endif
#endif
	SDL_Event event;
	bool done = false;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE: {

					GML_STDMUTEX_LOCK(sim); // Run

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

					GML_STDMUTEX_LOCK(sim); // Run

					// re-initialize the stencil
					glClearStencil(0);
					glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
					glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
					SetupViewportGeometry();
					break;
				}
				case SDL_QUIT: {
					done = true;
					break;
				}
				case SDL_ACTIVEEVENT: {
					if (event.active.state & SDL_APPACTIVE) {
						gu->active = !!event.active.gain;
					}

					if (mouse && mouse->locked) {
						mouse->ToggleState();
					}
					break;
				}

				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_SYSWMEVENT: {
					mouseInput->HandleSDLMouseEvent (event);
					break;
				}
				case SDL_KEYDOWN: {
					int i = event.key.keysym.sym;
					currentUnicode = event.key.keysym.unicode;

					const bool isRepeat = !!keys[i];

					UpdateSDLKeys ();

					if (activeController) {
						if (i <= SDLK_DELETE) {
							i = tolower(i);
						}
						else if (i == SDLK_RSHIFT) { i = SDLK_LSHIFT; }
						else if (i == SDLK_RCTRL)  { i = SDLK_LCTRL;  }
						else if (i == SDLK_RMETA)  { i = SDLK_LMETA;  }
						else if (i == SDLK_RALT)   { i = SDLK_LALT;   }

						if (keyBindings) {
							const int fakeMetaKey = keyBindings->GetFakeMetaKey();
							if (fakeMetaKey >= 0) {
								keys[SDLK_LMETA] |= keys[fakeMetaKey];
							}
						}

						activeController->KeyPressed(i, isRepeat);

						if (activeController->userWriting){
							// use unicode for printed characters
							i = event.key.keysym.unicode;
							if ((i >= SDLK_SPACE) && (i <= SDLK_DELETE)) {
								CGameController* ac = activeController;
								if (ac->ignoreNextChar || ac->ignoreChar == char(i)) {
									ac->ignoreNextChar = false;
								} else {
									if (i < SDLK_DELETE && (!isRepeat || ac->userInput.length()>0)) {
										const int len = (int)ac->userInput.length();
										ac->writingPos = std::max(0, std::min(len, ac->writingPos));
										char str[2] = { char(i), 0 };
										ac->userInput.insert(ac->writingPos, str);
										ac->writingPos++;
									}
								}
							}
						}
					}
					activeController->ignoreNextChar = false;
					break;
				}
				case SDL_KEYUP: {
					int i = event.key.keysym.sym;
					currentUnicode = event.key.keysym.unicode;

					UpdateSDLKeys();

					if (activeController) {
						if (i <= SDLK_DELETE) {
							i = tolower(i);
						}
						else if (i == SDLK_RSHIFT) { i = SDLK_LSHIFT; }
						else if (i == SDLK_RCTRL)  { i = SDLK_LCTRL;  }
						else if (i == SDLK_RMETA)  { i = SDLK_LMETA;  }
						else if (i == SDLK_RALT)   { i = SDLK_LALT;   }

						if (keyBindings) {
							const int fakeMetaKey = keyBindings->GetFakeMetaKey();
							if (fakeMetaKey >= 0) {
								keys[SDLK_LMETA] |= keys[fakeMetaKey];
							}
						}

						activeController->KeyReleased(i);
					}
					break;
				}
				default:
					break;
			}
		}
		if (globalQuit)
			break;

		try {
			if (!Update())
				break;
		} catch (content_error &e) {
			LogObject() << "Caught content exception: " << e.what() << "\n";
			handleerror(NULL, e.what(), "Content error", MBF_OK | MBF_EXCL);
		}
	}

#ifdef USE_GML
	#if GML_ENABLE_SIM
	gmlKeepRunning=0; // wait for sim to finish
	while(!gmlProcessor->PumpAux())
		boost::thread::yield();
	#endif
	if(gmlProcessor)
		delete gmlProcessor;
#endif

	// Shutdown
	Shutdown();
	return 0;
}

/**
 * Saves position of the window, if we're not in fullscreen
 */
void SpringApp::SaveWindowGeometry()
{
	if (!fullscreen) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		if (SDL_GetWMInfo(&info)) {
#if defined(_WIN32)
			RECT rect;
			if (GetWindowRect(info.window, &rect)) {
				windowPosX = rect.left;
				windowPosY = rect.top;
				configHandler->Set("WindowPosX", windowPosX);
				configHandler->Set("WindowPosY", windowPosY);
			}
#else
			// TODO FIXME XXX
#endif
		}
	}
}


/**
 * Deallocates and shuts down game
 */
void SpringApp::Shutdown()
{
	SaveWindowGeometry();

	if (pregame)
		delete pregame;			//in case we exit during init
	if (game)
		delete game;
	delete gameSetup;
	delete font;
	CNamedTextures::Kill();
	GLContext::Free();
	ConfigHandler::Deallocate();
	UnloadExtensions();
	delete mouseInput;
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_Quit();
	delete gs;
	delete gu;
#ifdef USE_MMGR
	m_dumpMemoryReport();
#endif
}
