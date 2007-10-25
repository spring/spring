/**
 * @file Main.cpp
 * @brief Main class
 *
 * Main application class that launches
 * everything else
 */
#include "StdAfx.h"
#include <algorithm>
#include <string>
#include <time.h>
#include <float.h>
#include "bitops.h"
#include "FPUCheck.h"
#include "LogOutput.h"
#include "NetProtocol.h"
#include "MouseInput.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/PreGame.h"
#include "Game/StartScripts/ScriptHandler.h"
#include "Game/Team.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaOpenGL.h"
#include "Platform/BaseCmd.h"
#include "Platform/ConfigHandler.h"
#include "Platform/errorhandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/GLContext.h"
#include "Rendering/glFont.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TAPalette.h"
#include "creg/creg.h"
#include "mmgr.h"

#include <GL/glu.h>  // Header File For The GLu32 Library
                     // Must come after Rendering/GL/myGL.h for GLEW

#include <SDL.h>     // Must come after Game/UI/MouseHandler.h for ButtonPress
#include <SDL_main.h>
#include <SDL_syswm.h>

#include <assert.h>
#include <signal.h>

#ifdef WIN32
#ifdef __GNUC__
	#include "Platform/Win/CrashHandler.h"
#else
	// FIXME either remove totally or reintegrate properly
	// so it can be used on MinGW too?
	//#include "CrashRpt.h"
	//#pragma comment(lib,"../../../CrashRpt/lib/CrashRpt.lib")
#endif
	#include "Platform/Win/win32.h"
	#include <winreg.h>
	#include <direct.h>
	#include "Platform/Win/seh.h"
#endif // WIN32
#include "Sim/Projectiles/ExplosionGenerator.h"


#ifdef WIN32
/**
 * Win32 only: command line passed to WinMain() (not including exe filename)
 */
static const char *win_lpCmdLine=0;
#endif

/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
CGameController* activeController=0;

/**
 * @brief global quit
 *
 * Global boolean indicating whether the user
 * wants to quit
 */
bool globalQuit = false;

/**
 * @brief keys
 *
 * Array of possible keys, and which are being pressed
 */
Uint8 *keys;

/**
 * @brief fullscreen
 *
 * Whether or not the game is running in fullscreen
 */
bool fullscreen;

/**
 * @brief xres default
 *
 * Defines the default X resolution as 1024
 */
#define XRES_DEFAULT 1024

/**
 * @brief yres default
 *
 * Defines the default Y resolution as 768
 */
#define YRES_DEFAULT 768

/**
 * @brief Spring App
 *
 * Main Spring application class launched by main()
 */
class SpringApp
{
public:
	SpringApp (); 					//!< Constructor
	~SpringApp (); 					//!< Destructor

	int Run(int argc, char *argv[]); 		//!< Run game loop

protected:
	bool Initialize (); 	//!< Initialize app
	void CheckCmdLineFile (int argc,char *argv[]); 	//!< Check command line for files
	void ParseCmdLine(); 				//!< Parse command line
	void CreateGameSetup (); 			//!< Creates GameSetup
	bool InitWindow (const char* title); 		//!< Initializes window
	void InitOpenGL (); 				//!< Initializes OpenGL
	bool SetSDLVideoMode(); 			//!< Sets SDL video mode
	void Shutdown (); 				//!< Shuts down application
	int Update (); 					//!< Run simulation and draw
	void UpdateSDLKeys (); 				//!< Update SDL key array
	bool GetDisplayGeometry();
	void SetupViewportGeometry();

	/**
	 * @brief command line
	 *
	 * Pointer to instance of commandline parser
	 */
	BaseCmd *cmdline;

	/**
	 * @brief demofile
	 *
	 * Name of a demofile
	 */
	string demofile;

	/**
	 * @brief savefile
	 *
	 * Name of a savefile
	 */
	string savefile;

	/**
	 * @brief startscript
	 *
	 * Name of a start script
	 */
	string startscript;

	/**
	 * @brief screen width
	 *
	 * Game screen width
	 */
	int screenWidth;

	/**
	 * @brief screen height
	 *
	 * Game screen height
	 */
	int screenHeight;

	/**
	 * @brief FSAA
	 *
	 * Level of fullscreen anti-aliasing
	 */
	bool FSAA;

	int2 prevMousePos;
private:
	static void SigAbrtHandler(int unused);
};

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
	keys = 0;

	fullscreen = true;
	FSAA = false;

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

#ifdef _CRASHRPT_H_
/**
 * @brief crash callback
 * @return whether callback was successful
 * @param crState current state
 *
 * Callback function executed when
 * game crashes (win32 only)
 */
bool crashCallback(void* crState)
{
	logOutput.RemoveAllSubscribers();
	logOutput.Print("Spring has crashed");

	// Stop writing to infolog.txt
	logOutput.End();

	// FIXME needs to be updated for new netcode
	//bool wasRecording = false;
	//if (net->recordDemo) {
	//	delete net->recordDemo;
	//	wasRecording = true;
	//}

	AddFile("infolog.txt", "Spring information log");

	//if (wasRecording)
	//	AddFile(net->demoName.c_str(), "Spring game demo");

	SDL_Quit();

	return true;
}
#endif

/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize ()
{
	logOutput.SetMirrorToStdout(!!configHandler.GetInt("StdoutDebug",0));

	// Initialize class system
	creg::System::InitializeClasses ();

	// Initialize crash reporting
#ifdef _WIN32
#if defined(_CRASHRPT_H_)
	Install( (LPGETLOGFILE) crashCallback, "taspringcrash@clan-sy.com", "Spring Crashreport");
	if (!GetInstance())
	{
		ErrorMessageBox("Error installing crash reporter", "CrashReport error:", MBF_OK);
		return false;
	}
#elif defined(CRASHHANDLER_H)
	CrashHandler::Install();
#endif
	InitializeSEH();
#endif

	ParseCmdLine();
	FileSystemHandler::Initialize(true);

	if (!InitWindow ("RtsSpring"))
	{
		SDL_Quit ();
		return false;
	}

	mouseInput = IMouseInput::Get ();

	// Global structures
	ENTER_SYNCED;
	gs=SAFE_NEW CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=SAFE_NEW CGlobalUnsyncedStuff();

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

	keys = SAFE_NEW Uint8[SDLK_LAST];
	memset (keys,0,sizeof(Uint8)*SDLK_LAST);

	// Initialize font
	const int charFirst = configHandler.GetInt("FontCharFirst", 32);
	const int charLast  = configHandler.GetInt("FontCharLast", 255);
	std::string fontFile = configHandler.GetString("FontFile", "fonts/Luxi.ttf");

	try {
		font = SAFE_NEW CglFont(charFirst, charLast, fontFile.c_str());
	} catch(content_error&) {
		// If the standard location fails, retry in fonts directory or vice versa.
		if (fontFile.substr(0, 6) == "fonts/")
			fontFile = fontFile.substr(6);
		else
			fontFile = "fonts/" + fontFile;
		font = SAFE_NEW CglFont(charFirst, charLast, fontFile.c_str());
	}

	// Initialize GLEW
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	// Initialize named texture handler
	CNamedTextures::Init();

	// Initialize Lua GL
	LuaOpenGL::Init();

	// Initialize ScriptHandler / LUA
	CScriptHandler::Instance().StartLua();

	// Create CGameSetup and CPreGame objects
	CreateGameSetup ();

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
	GLuint fsaa = configHandler.GetInt("FSAA",0);
	if (!fsaa)
		return false;
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
	GLuint fsaalevel = max(min(configHandler.GetInt("FSAALevel", 2), 8), 0);

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

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);

#ifdef __APPLE__
	const int defaultDepthSize = 32;
#else
	const int defaultDepthSize = 16;
#endif
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, configHandler.GetInt("DepthBufferBits", defaultDepthSize));
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, configHandler.GetInt("StencilBufferBits", 1));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	FSAA = MultisampleTest();

	SDL_Surface *screen = SDL_SetVideoMode(screenWidth, screenHeight, 0, sdlflags);
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
	const int lineSmoothing = configHandler.GetInt("SmoothLines", defaultSmooth);
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
	const int pointSmoothing = configHandler.GetInt("SmoothPoints", defaultSmooth);
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

	// there must be a way to see if this is necessary, compare old/new context pointers?
	if (!!configHandler.GetInt("FixAltTab", 0)) {
		// free GL resources
		GLContext::Free();

		// initialize any GL resources that were lost
		GLContext::Init();
	}

	// initialize the stencil
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();

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

#ifndef _WIN32
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

	gu->dualScreenMode = !!configHandler.GetInt("DualScreenMode", 0);
	if (gu->dualScreenMode) {
		gu->dualScreenMiniMapOnLeft =
			!!configHandler.GetInt("DualScreenMiniMapOnLeft", 0);
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

/**
 * @return whether commandline parsing was successful
 *
 * Parse command line arguments
 */
void SpringApp::ParseCmdLine()
{
	cmdline->addoption('f', "fullscreen",     OPTPARM_NONE,   "",  "Run in fullscreen mode");
	cmdline->addoption('w', "window",         OPTPARM_NONE,   "",  "Run in windowed mode");
	cmdline->addoption('m', "minimise",       OPTPARM_NONE,   "",  "Start minimised");
	cmdline->addoption('s', "server",         OPTPARM_NONE,   "",  "Run as a server");
	cmdline->addoption('c', "client",         OPTPARM_NONE,   "",  "Run as a client");
	cmdline->addoption('p', "projectiledump", OPTPARM_NONE,   "",  "Dump projectile class info in projectiles.txt");
	cmdline->addoption('t', "textureatlas",   OPTPARM_NONE,   "",  "Dump each finalized textureatlas in textureatlasN.tga");
	cmdline->addoption('q', "quit",           OPTPARM_INT,    "T", "Quit immediately on game over or after T seconds");
	cmdline->addoption('n', "name",           OPTPARM_STRING, "",  "Set your player name");
	cmdline->addoption('a', "autohost",           OPTPARM_INT,    "A", "Connect to autohost on localhost:A (UDP)");
	cmdline->parse();

#ifdef _DEBUG
	fullscreen = false;
#else
	fullscreen = configHandler.GetInt("Fullscreen", 1) != 0;
#endif

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->result("help")) {
		cmdline->usage("Spring",VERSION_STRING);
		exit(0);
	} else if (cmdline->result("version")) {
		std::cout << "Spring " << VERSION_STRING << std::endl;
		exit(0);
	} else if (cmdline->result("projectiledump")) {
		CCustomExplosionGenerator::OutputProjectileClassInfo();
		exit(0);
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
		configHandler.SetString("name", name);
	}

	int autohost;
	if (cmdline->result("autohost", autohost)) {
		configHandler.SetInt("Autohost", autohost);
	}

	screenWidth = configHandler.GetInt("XResolution", XRES_DEFAULT);
	screenHeight = configHandler.GetInt("YResolution", YRES_DEFAULT);
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

	string cmdLineStr = win_lpCmdLine;
	string::size_type offset = 0;
	//Simply assumes that any argument coming after a argument starting with /q is a variable to /q.
	for(int i=1; i < argc && (argv[i][0] == '/' || (argv[i-1][0] == '/' && argv[i-1][1] == 'q')); i++){
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
			logOutput << "Using demofile " << demofile.c_str() << "\n";
		} else if (command.rfind("ssf") == command.size()-3) {
			savefile = command;
			logOutput << "Using savefile " << savefile.c_str() << "\n";
		} else {
			startscript = command;
			logOutput << "Using script " << startscript.c_str() << "\n";
		}
	}
#else
	for (int i = 1; i < argc; i++)
		if (argv[i][0] != '-')
		{
			string command(argv[i]);
			int idx = command.rfind("sdf");
			if (idx == command.size()-3) {
				demofile = command;
				logOutput << "Using demofile " << demofile.c_str() << "\n";
			} else if (command.rfind("ssf") == command.size()-3) {
				savefile = command;
				logOutput << "Using savefile " << savefile.c_str() << "\n";
			} else {
				startscript = command;
				logOutput << "Using script " << startscript.c_str() << "\n";
			}
		}
#endif
}

/**
 * Initializes instance of GameSetup
 */
void SpringApp::CreateGameSetup ()
{
	ENTER_SYNCED;

	if (!startscript.empty()) {
		gameSetup = SAFE_NEW CGameSetup();
		if (!gameSetup->Init(startscript)) {
			delete gameSetup;
			gameSetup = 0;
		}
	}

	if (!gameSetup && demofile.empty()) {
		gs->noHelperAIs = !!configHandler.GetInt("NoHelperAIs", 0);
		//FIXME: duplicated in Net.cpp
		const string luaGaiaStr  = configHandler.GetString("LuaGaia",  "1");
		const string luaRulesStr = configHandler.GetString("LuaRules", "1");
		gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
		gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);
		if (gs->useLuaGaia) {
			gs->gaiaTeamID = gs->activeTeams;
			gs->gaiaAllyTeamID = gs->activeAllyTeams;
			gs->activeTeams++;
			gs->activeAllyTeams++;
			CTeam* team = gs->Team(gs->gaiaTeamID);
			team->color[0] = 255;
			team->color[1] = 255;
			team->color[2] = 255;
			team->color[3] = 255;
			team->gaia = true;
			gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
		}
	}

	ENTER_MIXED;

	bool server = true;

	if (!demofile.empty()) {
		server = false;
	}
	else if (gameSetup) {
		// first real player is demo host
		server = (gameSetup->myPlayer == gameSetup->numDemoPlayers);
	}
	else {
		server = !cmdline->result("client") || cmdline->result("server");
	}

#ifdef SYNCDEBUG
	// initialize sync debugger as soon as we know whether we will be server
	CSyncDebugger::GetInstance()->Initialize(server);
#endif

	if (!demofile.empty()) {
		pregame = SAFE_NEW CPreGame(false, demofile, "");
	} else {
		pregame = SAFE_NEW CPreGame(server, "", savefile);
	}
}


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
		if (!activeController->Update()) {
			ret = 0;
		} else {
			ret = activeController->Draw();
		}
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
	Uint8 *state;
	state = SDL_GetKeyState(&numkeys);
	memcpy(keys, state, sizeof(Uint8) * numkeys);

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
	INIT_SYNCIFY;
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
	SDL_EventState (SDL_SYSWMEVENT, SDL_ENABLE);
#endif

	SDL_Event event;
	bool done = false;

	while (!done) {
		ENTER_UNSYNCED;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE: {
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
									if (i < SDLK_DELETE) {
										const int len = (int)ac->userInput.length();
										ac->writingPos = max(0, min(len, ac->writingPos));
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

		if (!Update())
			break;
	}
	ENTER_MIXED;

	// Shutdown
	Shutdown();
	return 0;
}

/**
 * Deallocates and shuts down game
 */
void SpringApp::Shutdown()
{
	if (pregame)
		delete pregame;			//in case we exit during init
	if (game)
		delete game;
	if (gameSetup)
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
	END_SYNCIFY;
#ifdef USE_MMGR
	m_dumpMemoryReport();
#endif
}

// On msvc main() is declared as a non-throwing function.
// Moving the catch clause to a seperate function makes it possible to re-throw the exception for the installed crash reporter
int Run(int argc, char *argv[])
{
#ifdef __MINGW32__
	// For the MinGW backtrace() implementation we need to know the stack end.
	{
		extern void* stack_end;
		char here;
		stack_end = (void*) &here;
	}
#endif

#ifdef STREFLOP_H
	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
#endif
	good_fpu_control_registers("::Run");

// It's nice to be able to disable catching when you're debugging
#ifndef NO_CATCH_EXCEPTIONS
	try {
		SpringApp app;
		return app.Run (argc,argv);
	}
	catch (const content_error& e) {
		SDL_Quit();
		logOutput.RemoveAllSubscribers();
		logOutput.Print ("Content error: %s\n",  e.what());
		handleerror(NULL, e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);
		return -1;
	}
	catch (const std::exception& e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print ("Fatal error: %s\n",  e.what());
		logOutput.RemoveAllSubscribers();
		throw; // let the error handler catch it
	#else
		logOutput.RemoveAllSubscribers();
		handleerror(NULL, e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
	catch (const char* e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print ("Fatal error: %s\n",  e);
		logOutput.RemoveAllSubscribers();
		throw; // let the error handler catch it
	#else
		logOutput.RemoveAllSubscribers();
		handleerror(NULL, e, "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
#else
	SpringApp app;
	return app.Run (argc, argv);
#endif
}

/**
 * @brief main
 * @return exit code
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Main entry point function
 */
#if defined(__APPLE__)
int main( int argc, char *argv[] )
#else
int main( int argc, char *argv[ ], char *envp[ ] ) /* envp only on linux/bsd */
#endif
{
	return Run (argc,argv);
}

#ifdef WIN32

int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE	hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
	setbuf(stdout, NULL); // unbuffered
	win_lpCmdLine = lpCmdLine;
	return Run (__argc, __argv);
}

#endif

