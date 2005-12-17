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
#ifndef NO_LUA
#include "Script/LuaBinder.h"
#endif
#include <SDL.h>
#include <SDL_main.h>
//#include "mmgr.h"

#include "Game/UI/NewGuiDefine.h"

#ifdef NEW_GUI
#include "Game/UI/GUI/GUIcontroller.h"
#endif

#ifdef _WIN32
#include "CrashRpt.h"
#include "Platform/Win/win32.h"
#include <winreg.h>
#include <direct.h>
#endif

#define XRES_DEFAULT 1024
#define YRES_DEFAULT 768

Uint8 *keys;			// Array Used For The Keyboard Routine
Uint8 scrollWheelSpeed;
SDL_Surface *screen;
int sdlflags;
bool	active=true;		// Window Active Flag Set To true By Default
bool	fullscreen=true;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	globalQuit=false;
bool	FSAA = false;
//time_t   fpstimer,starttime;
CGameController* activeController=0;

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	gu->screenx=width;
	gu->screeny=height;

	glViewport(0,0,gu->screenx,gu->screeny);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)gu->screenx/(GLfloat)gu->screeny,2.8f,MAX_VIEW_RANGE);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.1f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	return true;										// Initialization Went OK
}

int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing
{
	if(activeController){
		if(activeController->Update()==0)
			return 0;
		return activeController->Draw();						   // Keep Going
	}
	return true;
}

void KillGLWindow(GLvoid)								// Properly Kill The Window
{
#ifndef DEBUG
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
}

bool MultisampleTest(void)
{
	if (!GL_ARB_multisample)
		return false;
	GLuint fsaa = configHandler.GetInt("FSAA",0);
	if (!fsaa)
		return false;
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
	GLuint fsaalevel = min(configHandler.GetInt("FSAALevel",2),(unsigned int)8);
	if (fsaalevel % 2)
		fsaalevel--;
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,fsaalevel);
	return true;
}

bool MultisampleVerify(void)
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
	} else
		return false;
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (true) Or Windowed Mode (false)	*/

GLuint		PixelFormat;			// Holds The Results After Searching For A Match

bool CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag,int frequency)
{
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)) {
		handleerror(NULL,"Could not initialize SDL.","ERROR",MBF_OK|MBF_EXCL);
		return false;
	}

	// Sets window manager properties
	SDL_WM_SetIcon(SDL_LoadBMP("spring.bmp"),NULL);
	SDL_WM_SetCaption(title, title);

	int sdlflags = SDL_OPENGL | SDL_RESIZABLE;
	if (fullscreenflag) {
		sdlflags |= SDL_FULLSCREEN;
	}
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
	
	InitGL();
	ReSizeGLScene(screen->w,screen->h);

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		handleerror(NULL,"Initialization Failed.","ERROR",MBF_OK|MBF_EXCL);
		return false;								// Return false
	}

	return true;									// Success
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

#ifdef WIN32 /* SDL_main can't use envp in the main function */
int main( int argc, char *argv[] )
#else
int main( int argc, char *argv[ ], char *envp[ ] )
#endif
{
#ifndef _WIN32
	chdir(DATADIR);
#endif
	INIT_SYNCIFY;
	bool	done=false;								// Bool Variable To Exit Loop
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
	fullscreen=configHandler.GetInt("Fullscreen",1)!=0;
#endif
	scrollWheelSpeed=configHandler.GetInt("ScrollWheelSpeed",25);

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
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();

#ifndef NO_LUA
	// Initialize lua bindings
	CLuaBinder lua;
	if (!lua.LoadScript("testscript.lua")) 
		MessageBox(NULL, lua.lastError.c_str(), "lua",MB_YESNO|MB_ICONQUESTION);
#endif

	// Check if the commandline parameter is specifying a demo file
	bool playDemo = false;
	string demofile, startscript;
	for (int i = 0; i < argc; i++) {
		if (i == 0) {
#ifdef _WIN32
			string command(argv[i]);
			int idx = command.rfind("spring");
			string path = command.substr(0,idx);
			if (path.at(0) == '"')
				path.append(1,'"');
			if (path != "")
				_chdir(path.c_str());
#endif
		} else if (argv[i][0] != '-') {
			string command(argv[i]);
			int idx = command.rfind("sdf");
			if (idx == command.size()-3) {
				playDemo = true;
				demofile = command;
			}
			else {
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
	
	int xres=configHandler.GetInt("XResolution",XRES_DEFAULT);
	int yres=configHandler.GetInt("YResolution",YRES_DEFAULT);

	int frequency=configHandler.GetInt("DisplayFrequency",0);
	// Create Our OpenGL Window
	if (!CreateGLWindow("RtsSpring",xres,yres,0,fullscreen,frequency))
	{
		SDL_Quit();
		return 0;									// Quit If Window Was Not Created
	}

	font=new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	SDL_GL_SwapBuffers();
	if (playDemo)
		pregame = new CPreGame(false, demofile);
	else
		pregame=new CPreGame(server, "");

	#ifdef NEW_GUI
	guicontroller = new GUIcontroller();
	#endif

#ifndef NO_LUA
	lua.CreateLateBindings();
#endif

	keys = new Uint8[SDLK_LAST];

	SDL_Event event;
	while(!done)									// Loop That Runs While done=false
	{
		ENTER_UNSYNCED;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode(event.resize.w,event.resize.h,0,SDL_OPENGL|SDL_RESIZABLE|SDL_HWSURFACE|SDL_DOUBLEBUF);
					if (screen)
						ReSizeGLScene(screen->w,screen->h);
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
					
					int numkeys;
					Uint8 * state;
					state = SDL_GetKeyState(&numkeys);
					memcpy(keys, state, sizeof(Uint8) * numkeys);
					SDLMod mods = SDL_GetModState();
					keys[SDLK_LSHIFT] = mods&KMOD_SHIFT?1:0;
					keys[SDLK_LCTRL] = mods&KMOD_CTRL?1:0;
					keys[SDLK_LALT] = mods&KMOD_ALT?1:0;
					keys[SDLK_LMETA] = mods&KMOD_META?1:0;
					if (i == SDLK_RETURN)
						keys[i] = 1;
					
					if(activeController)
						activeController->KeyPressed(i,1);
					
#ifdef NEW_GUI
					i = event.key.keysym.unicode;
					if (i > 0 && i < 128) /* HACK */
						GUIcontroller::Character(char(i));
						
					if (0)
#endif
					{
						if(activeController){
							if(activeController->userWriting){ 
								i = event.key.keysym.unicode;
								if (i>31 && i < 128)
									if(activeController->ignoreNextChar || activeController->ignoreChar==char(i))
										activeController->ignoreNextChar=false;
									else
										if(i>31 && i<250)
											activeController->userInput+=char(i);
							}
						}
					}
					break;
				}
				case SDL_KEYUP:
				{
					int i = event.key.keysym.sym;

					int numkeys;
					Uint8 * state;
					state = SDL_GetKeyState(&numkeys);
					memcpy(keys, state, sizeof(Uint8) * numkeys);
					SDLMod mods = SDL_GetModState();
					keys[SDLK_LSHIFT] = mods&KMOD_SHIFT?1:0;
					keys[SDLK_LCTRL] = mods&KMOD_CTRL?1:0;
					keys[SDLK_LALT] = mods&KMOD_ALT?1:0;
					keys[SDLK_LMETA] = mods&KMOD_META?1:0;
					if (i == SDLK_RETURN)
						keys[i] = 0;
					
					if (activeController)
						activeController->KeyReleased(i);
					
					break;
				}
			}
		}
		// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
		if ((active && !DrawGLScene()) || globalQuit)	// Active?  Was There A Quit Received?
		{
			done=true;							// ESC or DrawGLScene Signalled A Quit
		}
		else									// Not Time To Quit, Update Screen
		{
			if (FSAA)
				glEnable(GL_MULTISAMPLE_ARB);
			DrawGLScene();
			SDL_GL_SwapBuffers();
			if (FSAA)
				glDisable(GL_MULTISAMPLE_ARB);
		}

	}
	ENTER_MIXED;

	delete[] keys;

	// Shutdown
	if (gameSetup)
		delete gameSetup;
	if (pregame)
		delete pregame;								//in case we exit during init
	if (game)
		delete game;
	delete font;
	ConfigHandler::Deallocate();
	UnloadExtensions();
	KillGLWindow();									// Kill The Window
	SDL_Quit();
	delete gs;
	delete gu;
	END_SYNCIFY;
	//m_dumpMemoryReport();
	delete cmdline;
	return 0;
}
