/*
Copyright (c) 2012 Advanced Micro Devices, Inc.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
//Originally written by Erwin Coumans


#ifndef __OPENGL_INCLUDE_H
#define __OPENGL_INCLUDE_H


//think different
#if defined(__APPLE__) && !defined (VMDMESA)
#include <OpenGL/OpenGL.h>
//#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
//#import <Cocoa/Cocoa.h>
#if defined (USE_OPENGL2) || defined (NO_OPENGL3)
#include <OpenGL/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
#else

#ifdef GLEW_STATIC
#include "CustomGL/glew.h"
#else
#include <GL/glew.h>
#endif //GLEW_STATIC

#ifdef _WINDOWS
#include <windows.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
#else
//#include <GL/gl.h>
//#include <GL/glu.h>
#endif //_WINDOWS
#endif //APPLE

//disable glGetError
//#undef glGetError
//#define glGetError MyGetError
//
//GLenum inline MyGetError()
//{
//	return 0;
//}

///on Linux only glDrawElementsInstancedARB is defined?!?
//#ifdef __linux
//#define glDrawElementsInstanced glDrawElementsInstancedARB
//
//#endif //__linux

#endif //__OPENGL_INCLUDE_H

