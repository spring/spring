/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLX_EXT_STUB_H_
#define _GLX_EXT_STUB_H_

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

int glXGetVideoSyncSGI(unsigned int *count);

int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count);

typedef void (*__GLXextFuncPtr)(void);
__GLXextFuncPtr glXGetProcAddressARB (const GLubyte *procName);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _GLX_EXT_STUB_H_

