/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include "GL/glew.h"
#ifndef WIN32
//   #include "GL/glxew.h"
#endif
#include "glewstub.h"

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

//#include <stdio.h>

const GLubyte* glewGetString(GLenum name) {
	//printf( "glewGetString()"  );
	if (name == GLEW_VERSION) {
		return (const GLubyte*) "spring headless stub GLEW version";
	} else {
		return (const GLubyte*) "GL_ARB_multitexture GL_ARB_texture_env_combine GL_ARB_texture_compression";
	}
}

GLboolean glewIsSupported (const char* name) {
   return GL_FALSE;
}

GLenum glewInit() {
   //printf( "glewInit()\n"  );

   return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

