/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing The Base Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 */

#include "StdAfx.h"
#include <windows.h>		// Header File For Windows
#include "myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include <time.h>
#include <string>
#include <algorithm>
#include <math.h>
#include "PreGame.h"
#include "Game.h"
#include "winreg.h"
#include "float.h"
#include "glFont.h"
#include "MouseHandler.h"
#include "ConfigHandler.h"
#include "InfoConsole.h"
#include "GameSetup.h"
#include "CameraController.h"
#include "Net.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#ifdef _WIN32
#include <direct.h>
#endif
//#include "mmgr.h"

#include "NewGuiDefine.h"

#ifdef NEW_GUI
#include "GUIcontroller.h"
#endif

// Use the crashrpt library 
#ifndef NO_CRASHRPT
#include "../crashrpt/include/crashrpt.h"
#endif

Uint8 *keys;			// Array Used For The Keyboard Routine
Uint8 *oldkeys;
Uint64 init_time = 0;
SDL_Surface *screen;
bool	active=true;		// Window Active Flag Set To true By Default
bool	fullscreen=true;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	globalQuit=false;
//time_t   fpstimer,starttime;
CGameController* activeController=0;

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	gu->screenx=width-(!fullscreen&&!game)*6;
	gu->screeny=height-(!fullscreen&&!game)*26;

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
	SDL_Quit();
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
	SDL_Init(SDL_INIT_VIDEO);

	screen = SDL_SetVideoMode(width,height,bits,SDL_OPENGL|SDL_RESIZABLE);
	if (!screen) {
		MessageBox(NULL,"Could not set video mode","ERROR",MB_OK|MB_ICONEXCLAMATION);
		SDL_Quit();
		return false;
	}
	SDL_WM_SetCaption(title,title);
	  
	InitGL();
	ReSizeGLScene(screen->w,screen->h);

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return false
	}

	return true;									// Success
}

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

#ifndef NO_CRASHRPT
	AddFile("infolog.txt", "Spring information log");
	AddFile("test.sdf", "Spring game demo");

	if (wasRecording)
		AddFile(net->demoName.c_str(), "Spring game demo");
#endif

	return true;
}

int main( int argc, char *argv[ ], char *envp[ ] )
{
	INIT_SYNCIFY;
	bool	done=false;								// Bool Variable To Exit Loop
#ifdef NO_CRASHRPT
	// Initialize crash reporting
	Install(crashCallback, "taspringcrash@clan-sy.com", "TA Spring Crashreport");
#endif
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();

	// Check if the commandline parameter is specifying a demo file
	bool playDemo = false;
#ifdef _WIN32
	string cmdline(lpCmdLine);
	for (string::size_type pos = cmdline.find("\""); pos != string::npos; pos = cmdline.find("\"")) 
		cmdline.erase(pos, 1);
	string cmdext = cmdline.substr(cmdline.find_last_of('.') + 1);
	transform(cmdext.begin(), cmdext.end(), cmdext.begin(), (int (*)(int))tolower);
	if (cmdext == "sdf") {
		playDemo = true;

		// Launching a demo through a file association will not start spring with a correct working directory
		// So we need to determine this from the commandline and change manually
		// It should look like "x:\path\path\spring.exe" demo.sdf

		string fullcmd(GetCommandLine());
		string executable;
		if (fullcmd[0] == '"')
			executable = fullcmd.substr(1, fullcmd.find('"', 1));
		else 
			executable = fullcmd.substr(0, fullcmd.find(' '));

		string path = executable.substr(0, executable.find_last_of('\\'));
		if (path != "")
			_chdir(path.c_str()); 

		//MessageBox(NULL,path.c_str(), "Path",MB_YESNO|MB_ICONQUESTION);
	}
#endif

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
#ifdef _WIN32
		if(!gameSetup->Init(lpCmdLine)){
			delete gameSetup;
			gameSetup=0;
		}
#else	
		gameSetup=0;
#endif
	}

	ENTER_MIXED;

	bool server;
#ifndef _WIN32
	server=true;
	fullscreen=true;
#else
	if (playDemo)
		server = false;
	else if(gameSetup)
		server=gameSetup->myPlayer==0;
	else
		server=MessageBox(NULL,"Do you want to be server?", "Be server?",MB_YESNO|MB_ICONQUESTION)==IDYES;


	/*	// Ask The User Which Screen Mode They Prefer
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
	fullscreen=false;							// Windowed Mode
	}*/
#endif
	fullscreen=configHandler.GetInt("Fullscreen",1)!=0;
	
#ifdef _DEBUG
	fullscreen=false;
#endif

	int xres=1024;
	int yres=768;
	xres=configHandler.GetInt("XResolution",xres);
	yres=configHandler.GetInt("YResolution",yres);

	int frequency=configHandler.GetInt("DisplayFrequency",0);
	// Create Our OpenGL Window
	if (!CreateGLWindow("RtsSpring",xres,yres,32,fullscreen,frequency))
	{
		return 0;									// Quit If Window Was Not Created
	}



	font=new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	SDL_GL_SwapBuffers();
#ifdef _WIN32
	if (playDemo)
		pregame = new CPreGame(false, cmdline);
	else
#endif
		pregame=new CPreGame(server, "");

	#ifdef NEW_GUI
	guicontroller = new GUIcontroller();
	#endif

	init_time = time(NULL);

	while(!done)									// Loop That Runs While done=false
	{
		ENTER_UNSYNCED;
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode(event.resize.w,event.resize.h,0,SDL_OPENGL|SDL_RESIZABLE);
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
							mouse->currentCamController->MouseWheelMove(8);
						else if (event.button.button == SDL_BUTTON_WHEELDOWN)
							mouse->currentCamController->MouseWheelMove(-8);
						mouse->MousePress(event.button.x,event.button.y,event.button.button);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (mouse)
						mouse->MouseRelease(event.button.x,event.button.y,event.button.button);
					break;
			}
		}
		keys = SDL_GetKeyState(NULL);
		int mods = SDL_GetModState();
		if (mods&KMOD_SHIFT)
			keys[SDLK_LSHIFT] = 1;
		else
			keys[SDLK_LSHIFT] = 0;
		if (mods&KMOD_CTRL)
			keys[SDLK_LCTRL] = 1;
		else
			keys[SDLK_LCTRL] = 0;
		if (mods&KMOD_ALT)
			keys[SDLK_LALT] = 1;
		else
			keys[SDLK_LALT] = 0;
		if (mods&KMOD_META)
			keys[SDLK_LMETA] = 1;
		else
			keys[SDLK_LMETA] = 0;
		if (!oldkeys) {
			oldkeys = new Uint8[SDLK_LAST];
			for (int j = 0; j < SDLK_LAST; j++) {
				oldkeys[j] = 0;
			}
		}
		for (int i = 0; i < SDLK_LAST; i++) {
			if (keys[i] && !oldkeys[i]) {
				if(activeController){
					activeController->KeyPressed(i,1);
				}
				oldkeys[i] = 1;
#ifdef NEW_GUI
				GUIcontroller::Character(char(i));
				if (0)
#endif
				{
					if(activeController){
						if(activeController->userWriting && (i>31))
							if(activeController->ignoreNextChar || activeController->ignoreChar==char(i))
								activeController->ignoreNextChar=false;
							else
								activeController->userInput+=char(i);
					}
				}
			} else if (oldkeys[i] && !keys[i]) {
				if (activeController) {
					activeController->KeyReleased(i);
				}
				oldkeys[i] = 0;
			}
		}
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
		if ((active && !DrawGLScene()) || globalQuit)	// Active?  Was There A Quit Received?
		{
			done=true;							// ESC or DrawGLScene Signalled A Quit
		}
		else									// Not Time To Quit, Update Screen
		{
			DrawGLScene();
			SDL_GL_SwapBuffers();
		}

	}
	ENTER_MIXED;

	delete[] oldkeys;

	// Shutdown
	delete gameSetup;
	delete pregame;								//in case we exit during init
	delete game;
	delete font;
	ConfigHandler::Deallocate();
	UnloadExtensions();
	KillGLWindow();									// Kill The Window
	delete gs;
	delete gu;
	END_SYNCIFY;
	//m_dumpMemoryReport();
	return 0;
}
