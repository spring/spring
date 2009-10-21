// Copyright Hugh Perkins 2009
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//

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
