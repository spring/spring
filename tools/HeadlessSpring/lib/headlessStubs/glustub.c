/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GL/glu.h"

// #include <stdio.h>

GLAPI void APIENTRY gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {}

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

GLAPI void APIENTRY gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top) {}

GLAPI GLint APIENTRY gluProject (GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble* winX, GLdouble* winY, GLdouble* winZ) {
   //printf("gluProject\n");
   return 0;
}

