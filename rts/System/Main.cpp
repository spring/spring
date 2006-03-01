/**
 * @file Main.cpp
 * @brief Main class
 *
 * Main application class that launches
 * everything else
 */
#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include <time.h>
#include <string>
#include <algorithm>
#include <math.h>
#include "Game/PreGame.h"
#include "Game/Game.h"
#include <float.h>
#include "Rendering/glFont.h"
#include "Rendering/Textures/TAPalette.h"
#include "Game/UI/MouseHandler.h"
#include "Platform/ConfigHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/GameSetup.h"
#include "Game/CameraController.h"
#include "Net.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Platform/BaseCmd.h"
#include "Game/GameVersion.h"
#include "Platform/errorhandler.h"
#include "creg/creg.h"
#include "bitops.h"
#ifndef NO_LUA
#include "Script/LuaBinder.h"
#endif
#include <SDL.h>
#include <SDL_main.h>
#include "mmgr.h"
#include "Game/UI/NewGuiDefine.h"
#ifdef NEW_GUI
#include "Game/UI/GUI/GUIcontroller.h"
#endif

#ifdef _WIN32
#include "CrashRpt.h"
#include "Platform/Win/win32.h"
#include <winreg.h>
#include <direct.h>
#include <SDL_syswm.h>
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
	bool Initialize (); 				//!< Initialize app
	void CheckCmdLineFile (int argc,char *argv[]); 	//!< Check command line for files
	bool ParseCmdLine(); 				//!< Parse command line
	void InitVFS (); 				//!< Initialize VFS
	void CreateGameSetup (); 			//!< Creates GameSetup
	bool InitWindow (const char* title); 		//!< Initializes window
	void InitOpenGL (); 				//!< Initializes OpenGL
	bool SetSDLVideoMode(); 			//!< Sets SDL video mode
	void Shutdown (); 				//!< Shuts down application
	int Draw (); 					//!< Repeated draw function
	void UpdateSDLKeys (); 				//!< Update SDL key array

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
	 * @brief screen freq
	 * 
	 * Game screen frequency
	 */
	int screenFreq;

	/**
	 * @brief active
	 * 
	 * Whether game is active
	 */
	bool active;

	/**
	 * @brief FSAA
	 * 
	 * Level of fullscreen anti-aliasing
	 */
	bool FSAA;
};

/**
 * Initializes SpringApp variables
 */
SpringApp::SpringApp ()
{
	cmdline = 0;
	screenWidth = screenHeight = 0;
	screenFreq = 0;
	keys = 0;

	active = true;
	fullscreen = true;
	FSAA = false;
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	if (cmdline) delete cmdline;
	if (keys) delete[] keys;

	creg::ClassBinder::FreeClasses ();
}

#ifdef _WIN32
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
	info->AddLine("Spring has crashed");

	// Since we are going down, it's ok to delete the info console (it can't be mailed otherwise)
	delete info;
	bool wasRecording = false;
	if (net->recordDemo) {
		delete net->recordDemo;
		wasRecording = true;
	}

	AddFile("infolog.txt", "Spring information log");
	AddFile("test.sdf", "Spring game demo");

	if (wasRecording)
		AddFile(net->demoName.c_str(), "Spring game demo");

	return true;
}
#endif

/**
 * @return whether initialization was successful
 * 
 * Initializes the SpringApp instance
 */
bool SpringApp::Initialize ()
{
	ParseCmdLine ();

#ifdef WIN32
	// Initialize crash reporting
	Install( (LPGETLOGFILE) crashCallback, "taspringcrash@clan-sy.com", "TA Spring Crashreport");
#endif

	// Initialize class system
	creg::ClassBinder::InitializeClasses ();

#ifndef NO_LUA
	// Initialize lua bindings
	CLuaBinder lua;
	if (!lua.LoadScript("testscript.lua")) 
		handleerror(NULL, lua.lastError.c_str(), "lua",MBF_OK|MBF_EXCL);
#endif

	InitVFS ();

	if (!InitWindow ("RtsSpring"))
	{
		SDL_Quit ();
		return false;
	}
	// Global structures
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();

	InitOpenGL();

	palette.Init();

	// Initialize keyboard
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_SetModState (KMOD_NONE);
	
	keys = new Uint8[SDLK_LAST];
	memset (keys,0,sizeof(Uint8)*SDLK_LAST);

	// Initialize font
	font = new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

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
	GLuint fsaalevel = min(configHandler.GetInt("FSAALevel",2),(unsigned int)8);

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
#ifdef DEBUG
		char t[22];
		SNPRINTF(t,22, "FSAA level %d enabled",samples);
		handleerror(0,t,"SDL_GL",MBF_OK|MBF_INFO);
#endif
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
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) {
		handleerror(NULL,"Could not initialize SDL.","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}

	// Sets window manager properties
	SDL_WM_SetIcon(SDL_LoadBMP("spring.bmp"),NULL);
	SDL_WM_SetCaption(title, title);

	SetSDLVideoMode ();
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

	conditionally_set_flag(sdlflags, SDL_FULLSCREEN, fullscreen);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
#ifdef __APPLE__
	const int defaultDepthSize = 32;
#else
	const int defaultDepthSize = 16;
#endif
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, configHandler.GetInt("DepthBufferBits", defaultDepthSize));
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	FSAA = MultisampleTest();

	SDL_Surface *screen = SDL_SetVideoMode(screenWidth,screenHeight,0,sdlflags);
	if (!screen) {
		handleerror(NULL,"Could not set video mode","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}
	if (FSAA)
		FSAA = MultisampleVerify();
	
	return true;
}

/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL ()
{
	// Setup viewport
	glViewport (0, 0, screenWidth, screenHeight);
	gluPerspective(45.0f,(GLfloat)screenWidth/(GLfloat)screenHeight,2.8f,MAX_VIEW_RANGE);
	
	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	gu->screenx = screenWidth;
	gu->screeny = screenHeight;
}

/**
 * Initializes the virtual filesystem
 */
void SpringApp::InitVFS()
{
	// Create the archive scanner and vfs handler
	archiveScanner = new CArchiveScanner();
	archiveScanner->ReadCacheData();
	archiveScanner->Scan("./maps");
	archiveScanner->Scan("./base");
	archiveScanner->Scan("./mods");
	archiveScanner->WriteCacheData();
	hpiHandler = new CVFSHandler();
}

/**
 * @return whether commandline parsing was successful
 * 
 * Parse command line arguments
 */
bool SpringApp::ParseCmdLine()
{
	cmdline->addoption('f',"fullscreen",OPTPARM_NONE,"","Run in fullscreen mode");
	cmdline->addoption('w',"window",OPTPARM_NONE,"","Run in windowed mode");
	cmdline->addoption('s',"server",OPTPARM_NONE,"","Run as a server");
	cmdline->addoption('c',"client",OPTPARM_NONE,"","Run as a client");
//	cmdline->addoption('g',"runscript",OPTPARM_STRING,"script.txt", "Run with a game setup script");
	cmdline->parse();

#ifdef _DEBUG
	fullscreen = false;
#else
	fullscreen = configHandler.GetInt("Fullscreen",1)!=0;
#endif
	Uint8 scrollWheelSpeed = configHandler.GetInt("ScrollWheelSpeed",25);

	if (cmdline->result("help")) {
		cmdline->usage("TA:Spring",VERSION_STRING);
		return false;
	} else if (cmdline->result("version")) {
		std::cout << "TA:Spring " << VERSION_STRING << std::endl;
		return false;
	} else if (cmdline->result("window"))
		fullscreen = false;
	else if (cmdline->result("fullscreen"))
		fullscreen = true;

	screenWidth = configHandler.GetInt("XResolution",XRES_DEFAULT);
	screenHeight = configHandler.GetInt("YResolution",YRES_DEFAULT);
	screenFreq = configHandler.GetInt("DisplayFrequency",0);

	return true;
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
	const char *arg0=argv[0];
	int a,s = strlen(arg0);
	for (a=s-1;a>0;a--) 
		if (arg0[a] == '\\') break;
	if (a > 0) {
		string path(arg0, arg0+a);
		if (path.at(0) == '"')
			path.append(1,'"');
		_chdir(path.c_str());
	}
#endif

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			string command(argv[i]);
			int idx = command.rfind("sdf");
			if (idx == command.size()-3) {
				demofile = command;
			} else {
				startscript = command;
			}
		}
	}
}

/**
 * Initializes instance of GameSetup
 */
void SpringApp::CreateGameSetup ()
{
	ENTER_SYNCED;
	if (!startscript.empty()) {
		gameSetup=new CGameSetup();
		if(!gameSetup->Init(startscript)){
			delete gameSetup;
			gameSetup=0;
		}
	}

	ENTER_MIXED;

	bool server = true;
	if (!demofile.empty())
		server = false;
	else if(gameSetup)
		server=gameSetup->myPlayer-gameSetup->numDemoPlayers == 0;
	else
		server=!cmdline->result("client") || cmdline->result("server");

	if (!demofile.empty())
		pregame = new CPreGame(false, demofile);
	else
		pregame = new CPreGame(server, "");
}

/**
 * @return return code of activecontroller draw function
 * 
 * Draw function repeatedly called, it calls all the
 * other draw functions
 */
int SpringApp::Draw ()
{
	if (FSAA)
		glEnable(GL_MULTISAMPLE_ARB);

	int ret = 1;
	if(activeController) {
		if(activeController->Update()==0) ret = 0;
		else ret = activeController->Draw();
	}

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

	SDLMod mods = SDL_GetModState();
	keys[SDLK_LSHIFT] = mods&KMOD_SHIFT?1:0;
	keys[SDLK_LCTRL] = mods&KMOD_CTRL?1:0;
	keys[SDLK_LALT] = mods&KMOD_ALT?1:0;
	keys[SDLK_LMETA] = mods&KMOD_META?1:0;
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

	if (!Initialize ())
		return -1;

#ifdef WIN32
	SDL_EventState (SDL_SYSWMEVENT, SDL_ENABLE);
#endif

	SDL_Event event;
	bool done=false;
	std::map<int, int> toUnicode; // maps keysym.sym to keysym.unicode
	Uint8 scrollWheelSpeed = configHandler.GetInt("ScrollWheelSpeed",25);

	while (!done) {
		ENTER_UNSYNCED;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE:
					screenWidth = event.resize.w;
					screenHeight = event.resize.h;
#ifndef WIN32
					// HACK   We don't want to break resizing on windows (again?),
					//        so someone should test this very well before enabling it.
					SetSDLVideoMode();
#endif
					InitOpenGL();
					break;
				case SDL_QUIT:
					done = true;
					break;
				case SDL_MOUSEMOTION:
					if(mouse)
						mouse->MouseMove(event.motion.x,event.motion.y);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (mouse) {
						if (event.button.button == SDL_BUTTON_WHEELUP)
							mouse->currentCamController->MouseWheelMove(scrollWheelSpeed);
						else if (event.button.button == SDL_BUTTON_WHEELDOWN)
							mouse->currentCamController->MouseWheelMove(-scrollWheelSpeed);
						else
							mouse->MousePress(event.button.x,event.button.y,event.button.button);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (mouse)
						mouse->MouseRelease(event.button.x,event.button.y,event.button.button);
					break;
				case SDL_KEYDOWN:
				{
					int i = event.key.keysym.sym;

					UpdateSDLKeys ();

					if(activeController) {
						int j = std::tolower(event.key.keysym.unicode);
						// Don't translate 0-9 because with ctrl that maps to weird unicode characters (eg same as esc)
						if (j > SDLK_FIRST && j <= SDLK_DELETE && (i < SDLK_0 || i > SDLK_9)) {
							// With control+letter, the unicode field is 1-26, so convert it back to ascii.
							if (keys[SDLK_LCTRL] && j >= 1 && j <= 26) j += SDLK_a - 1;
							// Store the unicode value of this sym because SDL is too stupid
							// to do Unicode translation for SDL_KEYUP events.
							toUnicode[i] = j;
							i = j;
						}
						else if (i == SDLK_RSHIFT) i = SDLK_LSHIFT;
						else if (i == SDLK_RCTRL)  i = SDLK_LCTRL;
						else if (i == SDLK_RMETA)  i = SDLK_LMETA;
						else if (i == SDLK_RALT)   i = SDLK_LALT;
						activeController->KeyPressed(i,1);
#ifndef NEW_GUI
						if(activeController->userWriting){ 
							i = event.key.keysym.unicode;
							if (i >= SDLK_SPACE && i <= SDLK_DELETE)
								if(activeController->ignoreNextChar || activeController->ignoreChar==char(i))
									activeController->ignoreNextChar=false;
								else
									activeController->userInput+=char(i);
						}
#endif
					}
#ifdef NEW_GUI
					i = event.key.keysym.unicode;
					if (i > SDLK_FIRST && i <= SDLK_DELETE) /* HACK */
						GUIcontroller::Character(char(i));
#endif
					break;
				}
				case SDL_KEYUP:
				{
					int i = event.key.keysym.sym;

					UpdateSDLKeys();

					if (activeController) {
						// Retrieve the Unicode value stored in the KEYDOWN event (if any).
						std::map<int, int>::const_iterator j = toUnicode.find(i);
						if (j != toUnicode.end())  i = j->second;
						else if (i == SDLK_RSHIFT) i = SDLK_LSHIFT;
						else if (i == SDLK_RCTRL)  i = SDLK_LCTRL;
						else if (i == SDLK_RMETA)  i = SDLK_LMETA;
						else if (i == SDLK_RALT)   i = SDLK_LALT;
						activeController->KeyReleased(i);
					}
        			break;
				}
#ifdef WIN32
				case SDL_SYSWMEVENT:
				{
					SDL_SysWMmsg *msg = event.syswm.msg;
					if (msg->msg == 0x020B) { // WM_XBUTTONDOWN, beats me why it isn't defined by default
						if (msg->wParam & 0x20) // MK_XBUTTON1
							mouse->MousePress (LOWORD(msg->lParam),HIWORD(msg->lParam), 4);
						if (msg->wParam & 0x40) // MK_XBUTTON2
							mouse->MousePress (LOWORD(msg->lParam),HIWORD(msg->lParam), 5);
					}
					break;
				}
#endif
			}
		}
		int ret = Draw();
		if (globalQuit || (active && !ret))
			done=true;
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
	ConfigHandler::Deallocate();
	UnloadExtensions();
#ifndef DEBUG
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
	SDL_Quit();
	delete gs;
	delete gu;
	END_SYNCIFY;
#ifdef USE_MMGR
	m_dumpMemoryReport();
#endif
}

#if !defined(_WIN32) && !defined(__APPLE__)
/**
 * @brief locate spring data directory
 *
 * On *nix platforms, attempts to locate
 * and change to the spring data directory
 */
static inline int LocateDataDir(void)
{
	int ret;
	char *env = getenv("SPRING_DATADIR");
	if (!env)
		env = SPRING_DATADIR;
	ret = chdir(configHandler.GetString("SpringData",std::string(env)).c_str());
	if (ret == -1)
		perror("Spring datadir warning");
	return ret;
}
#endif

/**
 * @brief main
 * @return exit code
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Main entry point function
 */
#if defined(WIN32) || defined(__APPLE__) 
int main( int argc, char *argv[] )
#else
int main( int argc, char *argv[ ], char *envp[ ] ) /* envp only on linux/bsd */
#endif
{
#if !defined(_WIN32) && !defined(__APPLE__)
	int ret = LocateDataDir();
// Just stay in current working directory when chdir fails.
// 	if (ret != 0)
// 		exit(ret);
#endif

// It's nice to be able to disable catching when you're debugging
#ifndef NO_CATCH_EXCEPTIONS
	try {
		SpringApp app;
		return app.Run (argc,argv);
	} catch (const std::exception& e) {
		handleerror(NULL, e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
		return 1;
	}
#else
	SpringApp app;
	return app.Run (argc, argv);
#endif
}
