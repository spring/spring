/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing The Base Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 */

#include "StdAfx.h"
#include "archdef.h"
#include <winsock2.h>
#include <windows.h>		// Header File For Windows
#include "myGL.h"
#ifdef USE_GLUT
#include <glut.h>
#else
#include <GL/glu.h>			// Header File For The GLu32 Library
#endif
#include <time.h>
#include <string>
#include <math.h>
#include "PreGame.h"
#include "Game.h"
#include "winreg.h"
#include "float.h"
#include "glFont.h"
#include "MouseHandler.h"
#include "RegHandler.h"
#include "InfoConsole.h"
#include "GameSetup.h"
#include "CameraController.h"
#include "Net.h"
//#include "mmgr.h"

#include "NewGuiDefine.h"

#ifdef NEW_GUI
#include "GUIcontroller.h"
#endif

// Use the crashrpt library 
#ifndef unix
#include "../crashrpt/include/crashrpt.h"
#endif

#ifndef USE_GLUT
#pragma comment(lib, "../crashrpt/lib/crashrpt")
HDC	hDC=NULL;			// Private GDI Device Context
HGLRC	hRC=NULL;			// Permanent Rendering Context
HWND	hWnd=NULL;			// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application
#else
int windows;
#endif 

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	globalQuit=false;
//time_t   fpstimer,starttime;
CGameController* activeController=0;

#ifndef USE_GLUT
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc
#endif

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
	return TRUE;										// Initialization Went OK
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

#ifdef USE_GLUT
inline void Draw()	
{
  DrawGLScene();
  glutSwapBuffers();
}
#endif

void KillGLWindow(GLvoid)								// Properly Kill The Window
{

#ifndef USE_GLUT
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
	}
	ShowCursor(TRUE);									// Show Mouse Pointer

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}
#else //USE_GLUT
	glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
#endif //USE_GLUT
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/

GLuint		PixelFormat;			// Holds The Results After Searching For A Match

#ifndef USE_GLUT
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag,int frequency)
#else
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag,int frequency, int* pargc, char** pargv)
#endif
{
#ifndef USE_GLUT
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

//	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "TASpring";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmDisplayFrequency = frequency;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		if(frequency)
			dmScreenSettings.dmFields|=DM_DISPLAYFREQUENCY;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;						// Return FALSE
			}
		}
	}

	if (fullscreen)									// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;					// Window Extended Style
		dwStyle=WS_POPUP;							// Windows Style
//		HideMouse();								// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;				// Windows Style
	}

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,			// Extended Style For The Window
								"TASpring",			// Class Name
								title,				// Window Title
								dwStyle |			// Defined Window Style
								WS_CLIPSIBLINGS |	// Required Window Style
								WS_CLIPCHILDREN,	// Required Window Style
								0, 0,				// Window Position
								width, height,		// Selected Width And Height
								NULL,				// No Parent Window
								NULL,				// No Menu
								hInstance,			// Instance
								NULL)))				// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		24,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		8,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		24,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		char t[500];
		sprintf(t,"Can't Create A GL Device Context. %d",GetLastError());
		MessageBox(NULL,t,"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		char t[500];
		sprintf(t,"Can't Create A GL Rendering Context. %d",GetLastError());
		MessageBox(NULL,t,"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window

#else
	unsigned int options=0;
	if(bits==32)
	  options |= GLUT_RGBA;
	else
	  options |= GLUT_INDEX;
	  
	//GLUT code
	glutInit(pargc,pargv);
	glutInitWindowSize(width, height);
	glutInitDisplayMode(options | GLUT_DOUBLE | GLUT_DEPTH);

	windows = glutCreateWindow(title);

	InitGL();
	glutReshapeFunc(ReSizeGLScene);
#endif //USE_GLUT
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen
#ifdef USE_GLUT
	//disable fullscreen while porting
#if 0
	if(fullscreen)
		glutFullScreen();
#endif
        glutDisplayFunc(Draw);
#endif


	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

#ifndef USE_GLUT
LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			if(wParam<256 && !keys[VK_RMENU])
				keys[wParam]=true;
			if(activeController){
				activeController->KeyPressed(wParam,!!(lParam & (1<<30)));
			}
			return 0;
		}				
		case WM_CHAR:							// Is A Key Being Held Down?
		{
			#ifdef NEW_GUI
				GUIcontroller::Character(char(wParam));
				return 0;
			#endif
			if(activeController){
				if(activeController->userWriting && (wParam>31))
					if(activeController->ignoreNextChar || activeController->ignoreChar==char(wParam))
						activeController->ignoreNextChar=false;
					else
						activeController->userInput+=char(wParam);
			}
			return 0;
		}				
		case WM_KEYUP:								// Has A Key Been Released?
		{
			if(activeController){
				activeController->KeyReleased(wParam);
			}
			if(wParam<256)
				keys[wParam]=false;
			return 0;				
		}
		case WM_SYSKEYDOWN:							// Is A Key Being Held Down?
		{
			if(wParam<256)
				keys[wParam]=true;
			return 0;
		}
		case WM_SYSKEYUP:								// Has A Key Been Released?
		{
			if(wParam<256)
				keys[wParam]=false;
			return 0;				
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
		case WM_MOUSEMOVE:
		{
			if ( fullscreen )
			{
				RECT WindowBounds;
				GetClientRect(hWnd, &WindowBounds);
				ClipCursor(&WindowBounds);
			}
			if(mouse)
				mouse->MouseMove(LOWORD(lParam),HIWORD(lParam));
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			if(mouse)
				mouse->MousePress(LOWORD(lParam),HIWORD(lParam),1);
			return 0;
		}
		case WM_RBUTTONUP:
		{
			if(mouse)
				mouse->MouseRelease(LOWORD(lParam),HIWORD(lParam),1);
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			if(mouse)
				mouse->MousePress(LOWORD(lParam),HIWORD(lParam),0);
			return 0;
		}
		case WM_LBUTTONUP:
		{
			if(mouse)
				mouse->MouseRelease(LOWORD(lParam),HIWORD(lParam),0);
			return 0;
		}
		case 522: //WM_MOUSEWHEEL:
		{
			if(mouse){
				float move;
				move=((short) HIWORD(wParam));    // wheel rotation
				mouse->currentCamController->MouseWheelMove(move);
			}
			return 0;
		}
		case WM_MBUTTONDOWN:
		{
			if(mouse)
				mouse->MousePress(LOWORD(lParam),HIWORD(lParam),2);
			return 0;
		}
		case WM_MBUTTONUP:
		{
			if(mouse)
				mouse->MouseRelease(LOWORD(lParam),HIWORD(lParam),2);
			return 0;
		}
		case 523: //Mouse key4?:
		{
			if(mouse)
				mouse->MousePress(mouse->lastx,mouse->lasty,4);
			return 0;
		}
	}
//	if(info)
//		info->AddLine("msg %i",uMsg);
	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);

}

BOOL CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{

	switch (message)
	{          // Place message cases here.  
	case WM_INITDIALOG:
		return true;
	default:             
		return FALSE; 
    } 
} 
#else

#warning complete glut input handling
#warning add mouse wheel handling

void processNormalKeys(unsigned char key, int x, int y) {
  keys[key]=true;
  if(activeController){
    activeController->KeyPressed(key,1);
  }
#ifdef NEW_GUI
  GUIcontroller::Character(char(key));
  return;
#endif
  if(activeController){
    if(activeController->userWriting && (key>31))
      if(activeController->ignoreNextChar || activeController->ignoreChar==char(key))
	activeController->ignoreNextChar=false;
      else
	activeController->userInput+=char(key);
  }
  glutPostRedisplay();
}

void processNormalKeysUP(unsigned char key, int x, int y) {
  if(key==27)
    exit(0);
  if(activeController){
    activeController->KeyReleased(key);
  }
  keys[key]=false;
  glutPostRedisplay();
}


void processSpecialKeys(int key, int x, int y) {
  /* catches :
    * GLUT_KEY_F1
    * GLUT_KEY_F2
    * GLUT_KEY_F3
    * GLUT_KEY_F4
    * GLUT_KEY_F5
    * GLUT_KEY_F6
    * GLUT_KEY_F7
    * GLUT_KEY_F8
    * GLUT_KEY_F9
    * GLUT_KEY_F10
    * GLUT_KEY_F11
    * GLUT_KEY_F12
    * GLUT_KEY_LEFT
    * GLUT_KEY_UP
    * GLUT_KEY_RIGHT
    * GLUT_KEY_DOWN
    * GLUT_KEY_PAGE_UP
    * GLUT_KEY_PAGE_DOWN
    * GLUT_KEY_HOME
    * GLUT_KEY_END
    * GLUT_KEY_INSERT 
    */
  keys[key]=true;
  if(activeController){
    activeController->KeyPressed(key,1);
  }
  glutPostRedisplay();
}

void processMousePassiveMotion(int x, int y) {
  if(mouse)
    mouse->MouseMove(x,y);
}

void processMouseActiveMotion(int x, int y) {
  if(mouse)
    mouse->MouseMove(x,y);
}

void processMouse(int button, int state, int x, int y) {
  if(mouse)
    {
      if(state==GLUT_DOWN)
	mouse->MousePress(x, y, (button==GLUT_RIGHT_BUTTON)?1:((button==GLUT_MIDDLE_BUTTON)?2:0));
      else
	mouse->MouseRelease(x, y, (button==GLUT_RIGHT_BUTTON)?1:((button==GLUT_MIDDLE_BUTTON)?2:0));
    }
  glutPostRedisplay();
}
#endif //USE_GLUT

// Called when spring crashes
BOOL CALLBACK crashCallback(LPVOID crState)
{
	info->AddLine("Spring has crashed");

	// Since we are going down, it's ok to delete the info console (it can't be mailed otherwise)
	delete info;
	if (net->recordDemo)
		delete net->recordDemo;

#ifndef NO_IO
	AddFile("infolog.txt", "Spring information log");
	AddFile("test.sdf", "Spring game demo");
#endif
	return true;
}

#ifdef _WIN32
int WINAPI WinMain(	HINSTANCE	hInstanceIn,			// Instance
									 HINSTANCE	hPrevInstance,		// Previous Instance
									 LPSTR		lpCmdLine,			// Command Line Parameters
									 int			nCmdShow)			// Window Show State
#else
int main( int argc, char *argv[ ], char *envp[ ] )
#endif
{
	INIT_SYNCIFY;
#ifndef USE_GLUT
	MSG		msg;									// Windows Message Structure
	hInstance=hInstanceIn;
#endif
	BOOL	done=FALSE;								// Bool Variable To Exit Loop
	for(int b=0;b<256;b++)
		keys[b]=false;
#ifdef _WIN32
	// Initialize crash reporting
	Install(crashCallback, "taspringcrash@clan-sy.com", "TA Spring Crashreport");
#endif
	ENTER_SYNCED;
	gs=new CGlobalSyncedStuff();
	ENTER_UNSYNCED;
	gu=new CGlobalUnsyncedStuff();

	ENTER_SYNCED;
	gameSetup=new CGameSetup();
#ifdef _WIN32
	if(!gameSetup->Init(lpCmdLine)){
		delete gameSetup;
		gameSetup=0;
	}
#else	
	gameSetup=0;
#endif

	ENTER_MIXED;

	bool server;
#ifndef _WIN32
	server=true;
	fullscreen=true;
#else
	if(gameSetup)
		server=gameSetup->myPlayer==0;
	else
		server=MessageBox(NULL,"Do you want to be server?", "Be server?",MB_YESNO|MB_ICONQUESTION)==IDYES;


	/*	// Ask The User Which Screen Mode They Prefer
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
	fullscreen=FALSE;							// Windowed Mode
	}*/
#endif
	fullscreen=regHandler.GetInt("Fullscreen",1)!=0;
	
#ifdef _DEBUG
	fullscreen=false;
#endif

	int xres=1024;
	int yres=768;
	xres=regHandler.GetInt("XResolution",xres);
	yres=regHandler.GetInt("YResolution",yres);

	int frequency=regHandler.GetInt("DisplayFrequency",0);
	// Create Our OpenGL Window
#ifndef USE_GLUT
	if (!CreateGLWindow("RtsSpring",xres,yres,32,fullscreen,frequency))
#else
	if (!CreateGLWindow("RtsSpring",xres,yres,32,fullscreen,frequency,&argc,argv))
#endif
	{
		return 0;									// Quit If Window Was Not Created
	}



	font=new CglFont(32,223);
	LoadExtensions();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

#ifndef USE_GLUT
	SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
#else
	glutSwapBuffers();
#endif
	pregame=new CPreGame(server);

#ifdef USE_GLUT

	glutKeyboardUpFunc(processNormalKeysUP);
	glutKeyboardFunc(processNormalKeys);
	glutSpecialFunc(processSpecialKeys);
	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);
	glutPassiveMotionFunc(processMousePassiveMotion);

	ENTER_UNSYNCED;
        glutMainLoop();
#else

	while(!done)									// Loop That Runs While done=FALSE
	{
		ENTER_UNSYNCED;
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || globalQuit)	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
				SleepEx(0,true);
			}
		}

	}
#endif //USE_GLUT
	ENTER_MIXED;

	// Shutdown
	delete gameSetup;
	delete game;
	delete font;
	UnloadExtensions();
	KillGLWindow();									// Kill The Window
	END_SYNCIFY;
	//m_dumpMemoryReport();
#ifndef USE_GLUT
	return (msg.wParam);							// Exit The Program
#else
	return 0;
#endif
}
