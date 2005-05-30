/**
This file is here only for fast porting. 
It has to be removed so that no #include <windows.h>
appears in the common code

@author gnibu

*/

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#define  WINAPI

#ifdef NO_DLL
#define FreeLibrary(a) (0)
#define LoadLibrary(a) (0)
#define GetProcAddress(a,b) (0)
#endif


#ifdef EMULE_WINTYPES
//FIXME check following lines
typedef void *HANDLE;
#define CALLBACK
typedef void* HINSTANCE ;
typedef void *HDC; 
//following might stay like as is in a first time
typedef unsigned char BYTE;
typedef int WORD;
typedef bool BOOL;
typedef void *LPVOID;
typedef long int DWORD;
typedef unsigned int UINT;
typedef void VOID;
typedef long int DWORD;
typedef DWORD *DWORD_PTR;
typedef int LRESULT;
#include <string>
using namespace std;
typedef string LPSTR;
typedef int* LPINT;
/* TODO check if replacing
 	gu->lastFrameTime = (double)(start.QuadPart - lastMoveUpdate.QuadPart)/timeSpeed.QuadPart;

by 
	gu->lastFrameTime = (double)(start - lastMoveUpdate)/timeSpeed;
is OK for LARGE_INTEGER
Game.cpp:768: error: request for member `QuadPart' in `this->CGame::timeSpeed', 
   which is of non-class type `long int'
*/
#define LARGE_INTEGER long int
#define __int64 gint64
#else
#error please check the above types and maybe remove them from the code cf. http://www.jniwrapper.com/wintypes.jsp
#endif //EMULE_WINTYPES

#ifdef NO_WINDOWS
typedef void* HWND;
typedef void* WPARAM;
typedef void* LPARAM;
#endif

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) |\\
			((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) |\\
			((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))


#ifdef NO_INPUT
#define VK_SHIFT 1
#define VK_RETURN 1 
#define VK_UP 1
#define VK_DOWN 1
#define VK_LEFT 1
#define VK_RIGHT 1
#define VK_CONTROL 1
#define VK_MENU 1
#define VK_NUMPAD0 1
#define VK_NUMPAD1 1
#define VK_NUMPAD2 1
#define VK_NUMPAD3 1
#define VK_NUMPAD4 1
#define VK_NUMPAD5 1
#define VK_NUMPAD6 1
#define VK_NUMPAD7 1
#define VK_NUMPAD8 1
#define VK_NUMPAD9 1
#define VK_SPACE 1
#define VK_PAUSE 1
#define VK_RMENU 1
#define VK_LMENU 1
#define VK_ESCAPE 1
#define VK_RWIN 1
#define VK_LWIN 1
#define VK_BACK 1
#else
#error fake VK_XXXX : port input to GLUT
#endif //NO_INPUT

#ifdef ENABLE_SMALLFIXES
#define MessageBox(hWnd, lpText, lpCaption, uType) {fprintf(stderr,lpText);fprintf(stderr,"\n");}
#define ShowCursor(a) while(0){}
#else
#error unix : small fixes to replace by cleaner code or dev
#endif

#ifdef NO_MUTEXTHREADS
#define CreateMutex(a,b,c) (0)
#define ReleaseMutex(a) (0)
#define	CloseHandle(netMutex) while(0){}
#define WaitForSingleObject(a,b)  while(0){}
#endif

#define Sleep(a) sleep(a)


#ifdef NO_WINSTUFF
#define QueryPerformanceCounter(a) while(0){}
#define _hypot(a,b) 0
#endif

//FIXME read cpuinfo only one time for many calls (store value in static var)
#define NOMFICH_CPUINFO "/proc/cpuinfo" 
inline bool QueryPerformanceFrequency(LARGE_INTEGER* frequence)
{
  const char* prefixe_cpu_mhz = "cpu MHz";
  FILE* F;
  char ligne[300+1];
  char *pos;
  int ok=0;

  F = fopen(NOMFICH_CPUINFO, "r");
  if (!F) return false;

  while (!feof(F))
  {
    fgets (ligne, sizeof(ligne), F);

    if (!strncmp(ligne, prefixe_cpu_mhz, strlen(prefixe_cpu_mhz)))
    {
      pos = strrchr (ligne, ':') +2;
      if (!pos) break;
      if (pos[strlen(pos)-1] == '\n') pos[strlen(pos)-1] = '\0';
      strcpy (ligne, pos);
      strcat (ligne,"e6");
      *frequence = (long int)(atof (ligne)*1000000);
      ok = 1;
      break;
    }
  }
  fclose (F);
  return true;
}

#endif //__WINDOWS_H__
