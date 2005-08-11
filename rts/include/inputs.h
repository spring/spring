#ifndef _INPUTS_H
#define _INPUTS_H

#ifdef USE_GLUT
#include <GL/glew.h>
#include <GL/glut.h>
#define VK_RETURN  13
#define VK_UP      GLUT_KEY_UP
#define VK_DOWN    GLUT_KEY_DOWN	
#define VK_LEFT    GLUT_KEY_LEFT
#define VK_RIGHT   GLUT_KEY_RIGHT
#define VK_ESCAPE  27
#define VK_END     GLUT_KEY_END
//FIXME if you don't like SIGSEGV
#define VK_PAUSE   666
#define VK_SHIFT                    0x10
#define VK_CONTROL 		0x11
#define VK_MENU 		0x12
#define VK_NUMPAD0 GLUT_KEY_INSERT
#define VK_NUMPAD1 GLUT_KEY_END
#define VK_NUMPAD2 GLUT_KEY_DOWN
#define VK_NUMPAD3 GLUT_KEY_PAGE_DOWN
#define VK_NUMPAD4 GLUT_KEY_LEFT
#define VK_NUMPAD5 0
#define VK_NUMPAD6 GLUT_KEY_RIGHT
#define VK_NUMPAD7 GLUT_KEY_HOME
#define VK_NUMPAD8 GLUT_KEY_UP
#define VK_NUMPAD9 GLUT_KEY_PAGE_UP
#define VK_SPACE 32
//#define VK_PAUSE 1
//#define VK_RMENU 1
//#define VK_LMENU 
//#define VK_RWIN 1
//#define VK_LWIN 1
#define VK_BACK 92
#endif

extern bool keys[256];

inline bool keyShift()
{
	return keys[VK_SHIFT];
}

inline bool keyCtrl()
{
	return keys[VK_CONTROL];
}

inline bool  keyMenu()
{
	return keys[VK_MENU];
}

#endif //_INPUTS_H
