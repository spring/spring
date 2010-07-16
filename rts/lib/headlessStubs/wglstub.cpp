/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "wglstub.h"

#ifdef WIN32
WINGDIAPI PROC WINAPI wglGetProcAddress(LPCSTR) {}
#endif
