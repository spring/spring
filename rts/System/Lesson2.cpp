/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing The Base Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
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

#define XRES_DEFAULT 1024
#define YRES_DEFAULT 768

Uint8 *keys;			// Array Used For The Keyboard Routine
SDL_Surface *screen;
int sdlflags;
bool active = true;		// Window Active Flag Set To true By Default
bool fullscreen = true;
bool globalQuit = false;
bool FSAA = false;
//time_t   fpstimer,starttime;
CGameController* activeController=0;

/*
 * resizescene(width, height)
 * Called at resize
 */
static void resizescene(GLsizei width, GLsizei height)
{
	if (!height)
		height++;

	/*
	 * Reset current viewport
	 */
	glViewport(0,0,width,height);

	/*
	 * Reset projection matrix
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/*
	 * Aspect ratio
	 */
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,2.8f,MAX_VIEW_RANGE);

	/*
	 * Reset modelview matrix
	 */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gu->screenx=width;
	gu->screeny=height;
}

/*
 * setupgl()
 * Set initial OpenGL settings
 */
static inline void setupgl()
{
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.1f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); 	/* Nice perspective calculations */
}

/*
 * drawscene()
 * Triggers draw functions
 */
static inline int drawscene()
{
	if(activeController){
		if(activeController->Update()==0)
			return 0;
		return activeController->Draw();
	}
	return true;
}


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

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fsflag	- Use Fullscreen Mode (true) Or Windowed Mode (false)	*/

static bool glwindow(char* title, int width, int height, int bits, bool fsflag,int frequency)
{
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) {
		handleerror(NULL,"Could not initialize SDL.","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}

	// Sets window manager properties
	SDL_WM_SetIcon(SDL_LoadBMP("spring.bmp"),NULL);
	SDL_WM_SetCaption(title, title);

	int sdlflags = SDL_OPENGL | SDL_RESIZABLE;

	conditionally_set_flag(sdlflags, SDL_FULLSCREEN, fsflag);
	
	// FIXME: Might want to set color and depth sizes, too  -- johannes
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	FSAA = MultisampleTest();

	screen = SDL_SetVideoMode(width,height,bits,sdlflags);
	if (!screen) {
		handleerror(NULL,"Could not set video mode","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}
	if (FSAA)
		FSAA = MultisampleVerify();
	
	setupgl();
	resizescene(screen->w,screen->h);

	return true;
}

#ifdef _WIN32
// Called when spring crashes
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

#define update_sdlkeys(k) 				\
do { 							\
	int __numkeys; 					\
	Uint8 *__state; 				\
	__state = SDL_GetKeyState(&__numkeys); 		\
	memcpy((k), __state, sizeof(Uint8) * __numkeys);\
} while (0)

#define update_sdlmods(k) 				\
do { 							\
	SDLMod __mods = SDL_GetModState(); 		\
	(k)[SDLK_LSHIFT] = __mods&KMOD_SHIFT?1:0; 	\
	(k)[SDLK_LCTRL] = __mods&KMOD_CTRL?1:0; 	\
	(k)[SDLK_LALT] = __mods&KMOD_ALT?1:0; 		\
	(k)[SDLK_LMETA] = __mods&KMOD_META?1:0; 	\
} while (0)


#ifdef WIN32 /* SDL_main can't use envp in the main function */
int main( int argc, char *argv[] )
#else
int main( int argc, char *argv[ ], char *envp[ ] )
#endif
{
#ifndef _WIN32
	chdir(SPRING_DATADIR);
#endif
	INIT_SYNCIFY;
	bool	done=false;
	BaseCmd *cmdline = BaseCmd::initialize(argc,argv);
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
		delete cmdline;
		return 0;
	} else if (cmdline->result("version")) {
		std::cout << "TA:Spring " << VERSION_STRING << std::endl;
		delete cmdline;
		return 0;
	} else if (cmdline->result("window"))
		fullscreen = false;
	else if (cmdline->result("fullscreen"))
		fullscreen = true;
#ifdef _WIN32
	// Initialize crash reporting
	Install( (LPGETLOGFILE) crashCallback, "taspringcrash@clan-sy.com", "TA Spring Crashreport");
#endif
	// Initialize class system
	creg::ClassBinder::InitializeClasses ();

	// Global structures
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();


#ifndef NO_LUA
	// Initialize lua bindings
	CLuaBinder lua;
	if (!lua.LoadScript("testscript.lua")) 
		handleerror(NULL, lua.lastError.c_str(), "lua",MBF_OK|MBF_EXCL);
#endif

	// Check if the commandline parameter is specifying a demo file
	bool playDemo = false;
	string demofile, startscript;

#ifdef _WIN32
	string command(argv[0]);
	int idx = command.rfind("spring");
	string path = command.substr(0,idx);
	if (path.at(0) == '"')
		path.append(1,'"');
	if (path != "")
		_chdir(path.c_str());
#endif

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			string command(argv[i]);
			int idx = command.rfind("sdf");
			if (idx == command.size()-3) {
				playDemo = true;
				demofile = command;
			} else {
				playDemo = false;
				startscript = command;
			}
		}
	}

	// Create the archive scanner and vfs handler
	archiveScanner = new CArchiveScanner();
	archiveScanner->ReadCacheData();
	archiveScanner->Scan("./maps");
	archiveScanner->Scan("./base");
	archiveScanner->Scan("./mods");
	archiveScanner->WriteCacheData();
	hpiHandler = new CVFSHandler();

	palette.Init();

	ENTER_SYNCED;
	if (!playDemo) {
		gameSetup=new CGameSetup();
		if(!gameSetup->Init(startscript)){
			delete gameSetup;
			gameSetup=0;
		}
	}

	ENTER_MIXED;

	bool server = true;
	if (playDemo)
		server = false;
	else if(gameSetup)
		server=gameSetup->myPlayer-gameSetup->numDemoPlayers == 0;
	else
		server=!cmdline->result("client") || cmdline->result("server");
	
	if (
		!glwindow (
			"RtsSpring",
			configHandler.GetInt("XResolution",XRES_DEFAULT),
			configHandler.GetInt("YResolution",YRES_DEFAULT),
			0,
			fullscreen,
			configHandler.GetInt("DisplayFrequency",0)
		)
	) {
		SDL_Quit();
		return 0;
	}

	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_SetModState (KMOD_NONE);

	font = new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SDL_GL_SwapBuffers();
	
	if (playDemo)
		pregame = new CPreGame(false, demofile);
	else
		pregame = new CPreGame(server, "");

#ifdef NEW_GUI
	guicontroller = new GUIcontroller();
#endif

#ifndef NO_LUA
	lua.CreateLateBindings();
#endif

	keys = new Uint8[SDLK_LAST];
	memset (keys,0,sizeof(Uint8)*SDLK_LAST);

#ifdef WIN32
	SDL_EventState (SDL_SYSWMEVENT, SDL_ENABLE);
#endif

	SDL_Event event;
	while (!done) {
		ENTER_UNSYNCED;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode(event.resize.w,event.resize.h,0,SDL_OPENGL|SDL_RESIZABLE|SDL_HWSURFACE|SDL_DOUBLEBUF);
					if (screen)
						resizescene(screen->w,screen->h);
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
				
					update_sdlkeys(keys);
					update_sdlmods(keys);
					if (i == SDLK_RETURN)
						keys[i] = 1;
					
					if(activeController) {
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

					update_sdlkeys(keys);
					update_sdlmods(keys);
					if (i == SDLK_RETURN)
						keys[i] = 0;
					
					if (activeController)
						activeController->KeyReleased(i);
					
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
		if (FSAA)
			glEnable(GL_MULTISAMPLE_ARB);
		int ret = drawscene();
		SDL_GL_SwapBuffers();
		if (FSAA)
			glDisable(GL_MULTISAMPLE_ARB);
		if (globalQuit || (active && !ret))
			done=true;
	}
	ENTER_MIXED;

	delete[] keys;

	// Shutdown
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
	delete cmdline;
	return 0;
}
