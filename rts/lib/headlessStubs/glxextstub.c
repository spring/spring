/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glxextstub.h"

#include <stddef.h> // for NULL

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

int glXGetVideoSyncSGI(unsigned int *count) {
	return 0;
}

int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count) {
	return 0;
}

__GLXextFuncPtr glXGetProcAddressARB (const GLubyte *procName) {
	return NULL;
}

#ifdef __cplusplus
} // extern "C"
#endif

