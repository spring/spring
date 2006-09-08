/**
 * @file Main.cpp
 * @brief Main class
 *
 * Main application class that launches
 * everything else
 */
#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GLContext.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include <time.h>
#include <string>
#include <algorithm>
#include "Game/PreGame.h"
#include "Game/Game.h"
#include <float.h>
#include "Rendering/glFont.h"
#include "Rendering/Textures/TAPalette.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/KeyBindings.h"
#include "MouseInput.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Game/UI/InfoConsole.h"
#include "Game/GameSetup.h"
#include "Game/CameraController.h"
#include "Net.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Platform/BaseCmd.h"
#include "Game/GameVersion.h"
#include "Platform/errorhandler.h"
#include "Game/StartScripts/ScriptHandler.h"
#include "creg/creg.h"
#include "bitops.h"
#include "FPUCheck.h"
#include <SDL.h>
#include <SDL_main.h>
#ifdef WIN32
#include <SDL_syswm.h>
#endif
#include "mmgr.h"
#include "Game/UI/NewGuiDefine.h"
#ifdef NEW_GUI
#include "Game/UI/GUI/GUIcontroller.h"
#endif

#ifdef _WIN32
#ifndef __MINGW32__
#include "CrashRpt.h"
#endif
#include "Platform/Win/win32.h"
#include <winreg.h>
#include <direct.h>
#include <SDL_syswm.h>
#endif
#include "Sim/Projectiles/ExplosionGenerator.h"

#ifndef WIN32
#include <GL/glxew.h> // for glXWaitVideoSyncSGI()
#endif

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
	bool ParseCmdLine(); 				//!< Parse command line
	void CreateGameSetup (); 			//!< Creates GameSetup
	bool InitWindow (const char* title); 		//!< Initializes window
	void InitOpenGL (); 				//!< Initializes OpenGL
	bool SetSDLVideoMode(); 			//!< Sets SDL video mode
	void Shutdown (); 				//!< Shuts down application
	int Update (); 					//!< Run simulation and draw
	void UpdateSDLKeys (); 				//!< Update SDL key array
	void SetVSync ();				//!< Enable or disable VSync based on the "VSync" config option

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

	/**
	 * @brief vertical sync
	 * 
	 * Synchronize buffer swaps with the screen refresh rate.
	 * A value < 0 leaves the vsync mechanism at the system
	 * default, and higher values specify the interval between
	 * synced frames.
	 */
	int vsyncFrames;

	int2 prevMousePos;
};

/**
 * Initializes SpringApp variables
 */
SpringApp::SpringApp ()
{
	cmdline = 0;
	screenWidth = screenHeight = 0;
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

#ifdef _MSC_VER
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
	// Initialize class system
	creg::ClassBinder::InitializeClasses ();

	if (!ParseCmdLine ())
		return false;

#ifdef _MSC_VER
	// Initialize crash reporting
	Install( (LPGETLOGFILE) crashCallback, "taspringcrash@clan-sy.com", "TA Spring Crashreport");
#endif

	FileSystemHandler::Initialize(true);

	if (!InitWindow ("RtsSpring"))
	{
		SDL_Quit ();
		return false;
	}
	
	mouseInput = IMouseInput::Get ();

	// Global structures
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();

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
	
	keys = new Uint8[SDLK_LAST];
	memset (keys,0,sizeof(Uint8)*SDLK_LAST);

	// Initialize font
	font = new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	CScriptHandler::Instance().StartLua();

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
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) {
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
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	FSAA = MultisampleTest();

	SDL_Surface *screen = SDL_SetVideoMode(screenWidth,screenHeight,0,sdlflags);
	if (!screen) {
		handleerror(NULL,"Could not set video mode","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}

	// Something in SDL_SetVideoMode (OpenGL drivers?) messes with the FPU control word.
	// Set single precision floating point math.
	streflop_init<streflop::Simple>();

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

	SetVSync();

	if (cmdline->result("minimise"))
		SDL_WM_IconifyWindow();

	// there must be a way to see if this is necessary, compare old/new context pointers?
	if (!!configHandler.GetInt("FixAltTab", 0)) {
		// free GL resources
		GLContext::Free();

		// initialize any GL resources that were lost
		GLContext::Init();
	}

	return true;
}

/**
 * Set VSync based on the "VSync" config option
 * Uses WGL_EXT_swap_control or GLX_SGI_video_sync
 */
void SpringApp::SetVSync ()
{
	vsyncFrames = configHandler.GetInt("VSync", -1); // default to noop

#ifdef WIN32
	// VSync enabled is the default for OpenGL drivers
	if (vsyncFrames < 0) {
		return;
	}

	// WGL_EXT_swap_control is the only WGL extension exposed in glGetString(GL_EXTENSIONS)
	if (strstr( (const char*)glGetString(GL_EXTENSIONS), "WGL_EXT_swap_control")) {
		typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
		PFNWGLSWAPINTERVALEXTPROC SwapIntervalProc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if (SwapIntervalProc) {
			SwapIntervalProc(vsyncFrames);
		}
	}
#endif
}

/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL ()
{
	gu->dualScreenMode = !!configHandler.GetInt("DualScreenMode", 0);
	gu->screenx = screenWidth;
	if (gu->dualScreenMode) gu->screenx /= 2;
	gu->screeny = screenHeight;

	// Setup viewport
	glViewport (0, 0, gu->screenx, gu->screeny);
	gluPerspective(45.0f,(GLfloat)gu->screenx/(GLfloat)gu->screeny,2.8f,MAX_VIEW_RANGE);
	
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
bool SpringApp::ParseCmdLine()
{
	cmdline->addoption('f',"fullscreen",OPTPARM_NONE,"","Run in fullscreen mode");
	cmdline->addoption('w',"window",OPTPARM_NONE,"","Run in windowed mode");
	cmdline->addoption('m',"minimise", OPTPARM_NONE, "", "Start minimised");
	cmdline->addoption('s',"server",OPTPARM_NONE,"","Run as a server");
	cmdline->addoption('c',"client",OPTPARM_NONE,"","Run as a client");
	cmdline->addoption('p',"projectiledump", OPTPARM_NONE, "", "Dump projectile class info in projectiles.txt");
	cmdline->addoption('q',"quit", OPTPARM_INT, "T", "Quit immediately on game over or after T seconds");
	cmdline->parse();

#ifdef _DEBUG
	fullscreen = false;
#else
	fullscreen = configHandler.GetInt("Fullscreen",1)!=0;
#endif

	// mutually exclusive options that cause spring to quit immediately
	if (cmdline->result("help")) {
		cmdline->usage("TA:Spring",VERSION_STRING);
		return false;
	} else if (cmdline->result("version")) {
		std::cout << "TA:Spring " << VERSION_STRING << std::endl;
		return false;
	} else if (cmdline->result("projectiledump")) {
		CCustomExplosionGenerator::OutputProjectileClassInfo();
		return false;
	}

	// flags
	if (cmdline->result("window"))
		fullscreen = false;
	else if (cmdline->result("fullscreen"))
		fullscreen = true;

	screenWidth = configHandler.GetInt("XResolution",XRES_DEFAULT);
	screenHeight = configHandler.GetInt("YResolution",YRES_DEFAULT);

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
		int idx = command.rfind("sdf");
		if (idx == command.size()-3) 
			demofile = command;
		else startscript = command;
	}
#else
	for (int i = 1; i < argc; i++)
		if (argv[i][0] != '-')	
		{
			string command(argv[i]);
			int idx = command.rfind("sdf");
			if (idx == command.size()-3) {
				demofile = command;
			} else {
				startscript = command;
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
	else if(gameSetup) {
		server=gameSetup->myPlayer-gameSetup->numDemoPlayers == 0;
	}
	else
		server=!cmdline->result("client") || cmdline->result("server");

#ifdef SYNCDEBUG
	// initialize sync debugger as soon as we know whether we will be server
	CSyncDebugger::GetInstance()->Initialize(server);
#endif

	if (!demofile.empty()) {
		pregame = new CPreGame(false, demofile);
	} else {
		pregame = new CPreGame(server, "");
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

	mouseInput->Update ();

	int ret = 1;
	if(activeController) {
		if(activeController->Update()==0) ret = 0;
		else ret = activeController->Draw();
	}

#ifndef WIN32
	if (vsyncFrames > 0) {
		if (!GLXEW_SGI_video_sync) {
			vsyncFrames = 0; // disable
	} else {
			GLuint frameCount;
			if (glXGetVideoSyncSGI(&frameCount) == 0) {
				glXWaitVideoSyncSGI(vsyncFrames, frameCount % vsyncFrames, &frameCount);
			}
		}
	}
#endif  

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
				case SDL_ACTIVEEVENT:
					if (event.active.state & SDL_APPACTIVE) {
						gu->active = !!event.active.gain;
					}
					break;
				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_SYSWMEVENT:
					mouseInput->HandleSDLMouseEvent (event);
					break;
				case SDL_KEYDOWN:
				{
					int i = event.key.keysym.sym;
					bool isRepeat=!!keys[i];

					UpdateSDLKeys ();

					if(activeController) {
						int j = tolower(event.key.keysym.unicode);
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
						
						if (keyBindings) {
							const int fakeMetaKey = keyBindings->GetFakeMetaKey();
							if (fakeMetaKey >= 0) {
								keys[SDLK_LMETA] |= keys[fakeMetaKey];
							}
						}
						activeController->KeyPressed(i,isRepeat);

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
			}
		}
		if (globalQuit) 
			break;
	
		if (!Update() && active)
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

	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
	assert(good_fpu_control_registers());

// It's nice to be able to disable catching when you're debugging
#ifndef NO_CATCH_EXCEPTIONS
	try {
		SpringApp app;
		return app.Run (argc,argv);
	}
	catch (const content_error& e) {
		handleerror(NULL, e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);
	#ifdef _MSC_VER
		info->AddLine ("Content error: %s\n",  e.what());
	#endif
		return -1;
	}
	catch (const std::exception& e) {
		handleerror(NULL, e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
	#ifdef _MSC_VER
		info->AddLine ("Fatal error: %s\n",  e.what());
		throw; // let the error handler catch it
	#endif
		return -1;
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
	win_lpCmdLine = lpCmdLine;
	return Run (__argc, __argv);
}

#endif

