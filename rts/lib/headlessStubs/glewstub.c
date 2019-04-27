/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include "GL/glew.h"
#ifndef WIN32
//   #include "GL/glxew.h"
#endif
#include "glewstub.h"

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

const GLubyte* glewGetString(GLenum name) {
	assert(name == GLEW_VERSION);
	return (const GLubyte*) "spring headless stub GLEW version";
}

GLboolean glewIsSupported(const char* name) { return GL_FALSE; }
GLboolean glewIsExtensionSupported(const char* name) { return GL_FALSE; }

GLenum glewInit() {
   return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

