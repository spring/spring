/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_STUB_H_
#define _GL_STUB_H_

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#ifdef    WIN32
#include <stddef.h> // for ptrdiff_t
// these are defined in glew.h under windows
typedef char GLchar;
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
typedef ptrdiff_t GLsizeiptrARB;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif // WIN32

#endif // _GL_STUB_H_

