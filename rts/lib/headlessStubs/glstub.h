/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_STUB_H_
#define _GL_STUB_H_

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#ifdef _WIN32
# define _GDI32_
# ifdef _DLL
#  undef _DLL
# endif
# include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glext.h> //gl.h may not include all extensions


#endif // _GL_STUB_H_

