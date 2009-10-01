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

#include "GL/gl.h"

//#include <stdio.h>

void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
}

void glEnable(GLenum i) {
}

void glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
}

void glColor3fv( const GLfloat *v ) {
}

void glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
}

void glColor3ubv( const GLubyte *v ) {
}

void glColor4ubv( const GLubyte *v ) {
}

void glCopyTexImage2D( GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLsizei height,
                                        GLint border ) {
}

void glCopyTexSubImage2D( GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height ) {
}

void glDrawBuffer( GLenum mode ) {
}

void glDrawElements( GLenum mode, GLsizei count,
                                      GLenum type, const GLvoid *indices ) {
}

void glEdgeFlag( GLboolean flag ) {
}

void glEvalCoord1f( GLfloat u ) {
}

void  glEvalCoord2f( GLfloat u, GLfloat v ) {
}

void glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
}

void glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
}

void glEvalPoint1( GLint i ) {
}

void glEvalPoint2( GLint i, GLint j ) {
}

void glFinish( void ) {
}

void glFlush( void ) {
}

void glFrontFace( GLenum mode ) {
}

void glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val ) {
}

void glGetFloatv( GLenum pname, GLfloat *params ) {
   *params = 0;
}

void glGetTexImage( GLenum target, GLint level,
                                     GLenum format, GLenum type,
                                     GLvoid *pixels ) {
}

void glGetTexLevelParameteriv( GLenum target, GLint level,
                                                GLenum pname, GLint *params ) {
}

void glInitNames( void ) {
}

GLboolean glIsTexture( GLuint texture ) {
   return 0;
}

void glLightf( GLenum light, GLenum pname, GLfloat param ) {
}

void glLightModelfv( GLenum pname, const GLfloat *params ) {
}

void glLoadMatrixf( const GLfloat *m ) {
}

void glLoadName( GLuint name ) {
}

void glMap1f( GLenum target, GLfloat u1, GLfloat u2,
                               GLint stride,
                               GLint order, const GLfloat *points ) {
}

void glMap2f( GLenum target,
		     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		     const GLfloat *points ) {
}

void glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
}

void glMapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                                   GLint vn, GLfloat v1, GLfloat v2 ) {
}

void glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
}

void glNormal3fv( const GLfloat *v ) {
}

void glNormalPointer( GLenum type, GLsizei stride,
                                       const GLvoid *ptr ) {
}

void glPixelStorei( GLenum pname, GLint param ) {
}

void glPushName( GLuint name ) {
}

void glPopName( void ) {
}

void glReadBuffer( GLenum mode ) {
}

void glReadPixels( GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    GLvoid *pixels ) {
}

void glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
}

GLint glRenderMode( GLenum mode ) {
   return 0;
}

void glRotatef( GLfloat angle,
                                   GLfloat x, GLfloat y, GLfloat z ) {
}

void glScalef( GLfloat x, GLfloat y, GLfloat z ) {
}

void glScissor( GLint x, GLint y, GLsizei width, GLsizei height) {
}

void glSelectBuffer( GLsizei size, GLuint *buffer ) {
}

void glTexCoord1f( GLfloat s ) {
}

void glTexCoord2fv( const GLfloat *v ) {
}

void glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
}

void glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
}

void glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {
}

void glTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
}

void glTexImage1D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLint border,
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels ) {
}

void glVertex3fv( const GLfloat *v ) {
}

void glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
}

void glClipPlane( GLenum plane, const GLdouble *equation ) {
}

void glMatrixMode(GLenum mode) {
}

void  glGetBooleanv( GLenum pname, GLboolean *params ) {
   *params = 0;
}

void glLoadMatrixd( const GLdouble *m ) {
}

void glViewport(GLint x, GLint y,
                                    GLsizei width, GLsizei height) {
}

void  glNewList( GLuint list, GLenum mode ){
}

void glStencilFunc( GLenum func, GLint ref, GLuint mask ){}

void glStencilMask( GLuint mask ){}

void glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ){}

void glBlendEquation( GLenum mode ) {
}

void glEnableClientState( GLenum cap ){
}

void glDisableClientState( GLenum cap ) {
}

void glVertexPointer( GLint size, GLenum type,
                                       GLsizei stride, const GLvoid *ptr ){
}

void  glTexCoordPointer( GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr ){
}

void  glDrawArrays( GLenum mode, GLint first, GLsizei count ) {
}

void glTexEnvi(  GLenum target, GLenum pname, GLint param ){
 //  printf("glTexEnvi\n");
}

void glMultMatrixd( const GLdouble *m ) {
}

void glMultMatrixf( const GLfloat *m ) {
}

void glTexGeni( GLenum coord, GLenum pname, GLint param ) {
}

void glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) {
}

void glLightModeli( GLenum pname, GLint param ) {
}

void  glMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) {
}

void glMaterialf( GLenum face, GLenum pname, GLfloat param ) {
}

void glPointSize( GLfloat size ) {
}

 void glCullFace( GLenum mode ) {
}

void glLogicOp( GLenum opcode ) {
}

void  glEndList( void ){
}

// used by lua
GLuint glGenLists( GLsizei range ) {
   return 0;
}

void glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
}

void glLoadIdentity() {
}

void glOrtho(GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val) {
}

void glGenTextures(GLsizei n, GLuint *textures) {
   //*textures = 0;
}

void glBindTexture(GLenum target, GLuint texture) {
}

void glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data ) {
     //printf("glCompressedTexImage2D\n"); 
}

void glColorPointer( GLint size, GLenum type,
                                      GLsizei stride, const GLvoid *ptr ) {
   //*ptr = 0;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
   //printf( "glTexParameteri\n" );
}

void  glTexParameterf( GLenum target, GLenum pname, GLfloat param ){
   //printf ( "glTexParameterf\n" );
}

void glTexParameterfv(GLenum target, GLenum pname,
                                          const GLfloat *params) {
  // printf(  "glTexParameterfv\n" );
}

void glTexImage2D(GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels) {
  // printf( "glTexImage2D\n" );
}

void glClear(GLbitfield mask) {
}

void glBegin(GLenum mode) {
}

void glTexCoord2i(GLint s, GLint t ){
}

void glVertex2f(GLfloat x, GLfloat y ) {
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z){
}

void glEnd() {
}

void glDeleteTextures(GLsizei n, const GLuint *textures) {
   //int i = 0;
   //for( i = 0; i < n; i++ ) {
    //  textures[i] = 0;
   //}
}

void glGetIntegerv(GLenum pname, GLint *params) {
}

void glDepthFunc(GLenum func) {
}

void glShadeModel(GLenum mode) {
}

//void gluPerspective() {
//}

void glHint(GLenum target, GLenum mode) {
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param ) {
}

const GLubyte * glGetString( GLenum name ) {
   return "";
}

void glClearStencil(GLint s ) {
}

void glLightfv( GLenum light, GLenum pname,
                                 const GLfloat *params ) {
}

void glDeleteLists( GLuint list, GLsizei range ) {
}

void glDisable(GLenum i) {
}

void glClearDepth(GLclampd depth) {
}

void glColor4f( GLfloat red, GLfloat green,
                                   GLfloat blue, GLfloat alpha ) {
}

void glMultiTexCoord1f( GLenum target, GLfloat s ) {
  // printf("glMultiTexCoord1f\n");
}

void glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  // printf("glMultiTexCoord2f\n");
}

GLenum glGetError( void ) {
   return 0;
}

void glPolygonMode(GLenum face, GLenum mode) {
}

void glBlendFunc(GLenum sfactor, GLenum dfactor ) {
}

void  glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
}

void glColor4fv( const GLfloat *v) {
}

void glLineStipple(GLint factor, GLushort pattern) {
}

void glPopAttrib() {
}

void glPushAttrib(GLbitfield mask) {
}

void glDepthMask(GLboolean flag) {
}

void glAlphaFunc(GLenum func, GLclampf ref) {
}

void glFogfv(GLenum pname, const GLfloat *params) {
}

void  glFogf( GLenum pname, GLfloat param ) {
}

void glFogi( GLenum pname, GLint param ) {
}

void glPushMatrix() {
}

void glPopMatrix() {
}

void glCallList( GLuint list) {
}

void glTexSubImage2D(GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels ) {
}

void glTexCoord2f(GLfloat s, GLfloat t ) {
}

void glLineWidth(GLfloat width) {
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue ) {
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
}



