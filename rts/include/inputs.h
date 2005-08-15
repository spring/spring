#ifndef _INPUTS_H
#define _INPUTS_H

#ifdef USE_GLUT
#include <GL/glew.h>
#include <GL/glut.h>
#define VK_RETURN  13
#define VK_ESCAPE  27
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
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7a
#define VK_F12 0x7b

#define VK_END              0x23
#define VK_HOME             0x24
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_INSERT           0x2D
#define VK_PRIOR            0x21
#define VK_NEXT             0x22
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
