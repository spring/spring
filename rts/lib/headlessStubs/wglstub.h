/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WGL_STUB_H_
#define _WGL_STUB_H_

#ifdef WIN32
#include <windows.h>
WINGDIAPI PROC WINAPI wglGetProcAddress(LPCSTR);
#endif

#endif // _WGL_STUB_H_
