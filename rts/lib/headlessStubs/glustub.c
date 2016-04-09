/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#ifdef _WIN32
# define _GDI32_
# ifdef _DLL
#  undef _DLL
# endif
# include <windows.h>
#endif
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GLAPI
# undef GLAPI
#endif
#define GLAPI


GLAPI GLUquadric* APIENTRY gluNewQuadric() {
   return 0;
}

GLAPI void APIENTRY gluQuadricDrawStyle(GLUquadric* quad, GLenum draw) {}
GLAPI void APIENTRY gluSphere(GLUquadric* quad, GLdouble radius, GLint slices, GLint stacks) {}
GLAPI void APIENTRY gluDeleteQuadric(GLUquadric* quad) {}
GLAPI void APIENTRY gluCylinder(GLUquadric* quad, GLdouble base, GLdouble top, GLdouble height, GLint slices, GLint stacks) {}

GLAPI GLint APIENTRY gluBuild2DMipmaps (GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data) {
   //printf("gluBuild2DMipmaps\n");
   return 0;
}

GLAPI void APIENTRY gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {}
GLAPI void APIENTRY gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top) {}


#ifdef __cplusplus
} // extern "C"
#endif

