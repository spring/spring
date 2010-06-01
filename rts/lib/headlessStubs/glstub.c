/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GL/gl.h"

// We need this because newer versions of GL/gl.h
// undefine GLAPI in the end
#ifndef GLAPI
# ifdef _WIN32
#  define GLAPI __stdcall
# else
#  define GLAPI
# endif
# define __DEFINED_GLAPI
#endif

//#include <stdio.h>

GLAPI void APIENTRY glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {}
GLAPI void APIENTRY glEnable(GLenum i) {}
GLAPI void APIENTRY glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {}
GLAPI void APIENTRY glColor3fv( const GLfloat *v ) {}
GLAPI void APIENTRY glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {}
GLAPI void APIENTRY glColor3ubv( const GLubyte *v ) {}
GLAPI void APIENTRY glColor4ubv( const GLubyte *v ) {}

GLAPI void APIENTRY glCopyTexImage2D( GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLsizei height,
                                        GLint border ) {}
                                        
GLAPI void APIENTRY glCopyTexSubImage2D( GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height ) {}
                                           
GLAPI void APIENTRY glDrawBuffer( GLenum mode ) {}
GLAPI void APIENTRY glDrawElements( GLenum mode, GLsizei count,
                                      GLenum type, const GLvoid *indices ) {}
GLAPI void APIENTRY glEdgeFlag( GLboolean flag ) {}
GLAPI void APIENTRY glEvalCoord1f( GLfloat u ) {}
GLAPI void APIENTRY glEvalCoord2f( GLfloat u, GLfloat v ) {}
GLAPI void APIENTRY glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {}
GLAPI void APIENTRY glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {}
GLAPI void APIENTRY glEvalPoint1( GLint i ) {}
GLAPI void APIENTRY glEvalPoint2( GLint i, GLint j ) {}
GLAPI void APIENTRY glFinish( void ) {}
GLAPI void APIENTRY glFlush( void ) {}
GLAPI void APIENTRY glFrontFace( GLenum mode ) {}

GLAPI void APIENTRY glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val ) {}

GLAPI void APIENTRY glGetFloatv( GLenum pname, GLfloat *params ) {
   *params = 0;
}

GLAPI void APIENTRY glGetTexImage( GLenum target, GLint level,
                                     GLenum format, GLenum type,
                                     GLvoid *pixels ) {}

GLAPI void APIENTRY glGetTexLevelParameteriv( GLenum target, GLint level,
                                                GLenum pname, GLint *params ) {}

GLAPI void APIENTRY glInitNames( void ) {}

GLAPI GLboolean APIENTRY glIsTexture( GLuint texture ) {
   return 0;
}

GLAPI void APIENTRY glLightf( GLenum light, GLenum pname, GLfloat param ) {}
GLAPI void APIENTRY glLightModelfv( GLenum pname, const GLfloat *params ) {}
GLAPI void APIENTRY glLoadMatrixf( const GLfloat *m ) {}
GLAPI void APIENTRY glLoadName( GLuint name ) {}

GLAPI void APIENTRY glMap1f( GLenum target, GLfloat u1, GLfloat u2,
                               GLint stride,
                               GLint order, const GLfloat *points ) {}

GLAPI void APIENTRY glMap2f( GLenum target,
		     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		     const GLfloat *points ) {}

GLAPI void APIENTRY glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {}
GLAPI void APIENTRY glMapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                                   GLint vn, GLfloat v1, GLfloat v2 ) {}

GLAPI void APIENTRY glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {}
GLAPI void APIENTRY glNormal3fv( const GLfloat *v ) {}
GLAPI void APIENTRY glNormalPointer( GLenum type, GLsizei stride,
                                       const GLvoid *ptr ) {}

GLAPI void APIENTRY glPixelStorei( GLenum pname, GLint param ) {}
GLAPI void APIENTRY glPushName( GLuint name ) {}
GLAPI void APIENTRY glPopName( void ) {}
GLAPI void APIENTRY glReadBuffer( GLenum mode ) {}

GLAPI void APIENTRY glReadPixels( GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    GLvoid *pixels ) {}

GLAPI void APIENTRY glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {}

GLAPI GLint APIENTRY glRenderMode( GLenum mode ) {
   return 0;
}

GLAPI void APIENTRY glRotatef( GLfloat angle,
                                   GLfloat x, GLfloat y, GLfloat z ) {}

GLAPI void APIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z ) {}
GLAPI void APIENTRY glScissor( GLint x, GLint y, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glSelectBuffer( GLsizei size, GLuint *buffer ) {}
GLAPI void APIENTRY glTexCoord1f( GLfloat s ) {}
GLAPI void APIENTRY glTexCoord2fv( const GLfloat *v ) {}
GLAPI void APIENTRY glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {}
GLAPI void APIENTRY glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {}
GLAPI void APIENTRY glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {}
GLAPI void APIENTRY glTexGenf( GLenum coord, GLenum pname, GLfloat param ) {}

GLAPI void APIENTRY glTexImage1D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLint border,
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels ) {
}

GLAPI void APIENTRY glVertex3fv( const GLfloat *v ) {}
GLAPI void APIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {}
GLAPI void APIENTRY glClipPlane( GLenum plane, const GLdouble *equation ) {}
GLAPI void APIENTRY glMatrixMode(GLenum mode) {}

GLAPI void APIENTRY  glGetBooleanv( GLenum pname, GLboolean *params ) {
   *params = 0;
}

GLAPI void APIENTRY glLoadMatrixd( const GLdouble *m ) {}
GLAPI void APIENTRY glViewport(GLint x, GLint y,
                                    GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glNewList( GLuint list, GLenum mode ) {}
GLAPI void APIENTRY glStencilFunc( GLenum func, GLint ref, GLuint mask ) {}
GLAPI void APIENTRY glStencilMask( GLuint mask ) {}
GLAPI void APIENTRY glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {}
GLAPI void APIENTRY glBlendEquation( GLenum mode ) {}
GLAPI void APIENTRY glEnableClientState( GLenum cap ) {}
GLAPI void APIENTRY glDisableClientState( GLenum cap ) {}
GLAPI void APIENTRY glVertexPointer( GLint size, GLenum type,
                                       GLsizei stride, const GLvoid *ptr ){}
GLAPI void APIENTRY glTexCoordPointer( GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr ){}
GLAPI void APIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count ) {}

GLAPI void APIENTRY glTexEnvi(  GLenum target, GLenum pname, GLint param ){
 //  printf("glTexEnvi\n");
}

GLAPI void APIENTRY glMultMatrixd( const GLdouble *m ) {}
GLAPI void APIENTRY glMultMatrixf( const GLfloat *m ) {}
GLAPI void APIENTRY glTexGeni( GLenum coord, GLenum pname, GLint param ) {}
GLAPI void APIENTRY glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) {}
GLAPI void APIENTRY glLightModeli( GLenum pname, GLint param ) {}
GLAPI void APIENTRY glMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) {}
GLAPI void APIENTRY glMaterialf( GLenum face, GLenum pname, GLfloat param ) {}
GLAPI void APIENTRY glPointSize( GLfloat size ) {}
GLAPI void APIENTRY glCullFace( GLenum mode ) {}
GLAPI void APIENTRY glLogicOp( GLenum opcode ) {}
GLAPI void APIENTRY glEndList( void ){}

// used by lua
GLAPI GLuint APIENTRY glGenLists( GLsizei range ) {
   return 0;
}

GLAPI void APIENTRY glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {}
GLAPI void APIENTRY glLoadIdentity() {}
GLAPI void APIENTRY glOrtho(GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val) {}

GLAPI void APIENTRY glGenTextures(GLsizei n, GLuint *textures) {
   //*textures = 0;
}

GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture) {}
GLAPI void APIENTRY glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data ) {
     //printf("glCompressedTexImage2D\n"); 
}

GLAPI void APIENTRY glColorPointer( GLint size, GLenum type,
                                      GLsizei stride, const GLvoid *ptr ) {
   //*ptr = 0;
}

GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {
   //printf( "glTexParameteri\n" );
}

GLAPI void APIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param ){
   //printf ( "glTexParameterf\n" );
}

GLAPI void APIENTRY glTexParameterfv(GLenum target, GLenum pname,
                                          const GLfloat *params) {
  // printf(  "glTexParameterfv\n" );
}

GLAPI void APIENTRY glTexImage2D(GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels) {
  // printf( "glTexImage2D\n" );
}

GLAPI void APIENTRY glClear(GLbitfield mask) {}
GLAPI void APIENTRY glBegin(GLenum mode) {}
GLAPI void APIENTRY glTexCoord2i(GLint s, GLint t ){}
GLAPI void APIENTRY glVertex2f(GLfloat x, GLfloat y ) {}
GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z){}
GLAPI void APIENTRY glEnd() {}

GLAPI void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures) {
   //int i = 0;
   //for( i = 0; i < n; i++ ) {
    //  textures[i] = 0;
   //}
}

GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {}
GLAPI void APIENTRY glDepthFunc(GLenum func) {}
GLAPI void APIENTRY glShadeModel(GLenum mode) {}

//void gluPerspective() {
//}

GLAPI void APIENTRY glHint(GLenum target, GLenum mode) {}
GLAPI void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param ) {}

GLAPI const APIENTRY GLubyte * glGetString( GLenum name ) {
   return "";
}

GLAPI void APIENTRY glClearStencil(GLint s ) {}
GLAPI void APIENTRY glLightfv( GLenum light, GLenum pname,
                                 const GLfloat *params ) {}

GLAPI void APIENTRY glDeleteLists( GLuint list, GLsizei range ) {}
GLAPI void APIENTRY glDisable(GLenum i) {}
GLAPI void APIENTRY glClearDepth(GLclampd depth) {}
GLAPI void APIENTRY glColor4f( GLfloat red, GLfloat green,
                                   GLfloat blue, GLfloat alpha ) {}

GLAPI void APIENTRY glMultiTexCoord1f( GLenum target, GLfloat s ) {
  // printf("glMultiTexCoord1f\n");
}

GLAPI void APIENTRY glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  // printf("glMultiTexCoord2f\n");
}

GLAPI void APIENTRY glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  // printf("glMultiTexCoord4f\n");
}

GLAPI GLenum APIENTRY glGetError( void ) {
   return 0;
}

GLAPI void APIENTRY glPolygonMode(GLenum face, GLenum mode) {}
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor ) {}
GLAPI void APIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {}
GLAPI void APIENTRY glColor4fv( const GLfloat *v) {}
GLAPI void APIENTRY glLineStipple(GLint factor, GLushort pattern) {}
GLAPI void APIENTRY glPopAttrib() {}
GLAPI void APIENTRY glPushAttrib(GLbitfield mask) {}
GLAPI void APIENTRY glDepthMask(GLboolean flag) {}
GLAPI void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {}

GLAPI void APIENTRY glFogfv(GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glFogf( GLenum pname, GLfloat param ) {}
GLAPI void APIENTRY glFogi( GLenum pname, GLint param ) {}
GLAPI void APIENTRY glPushMatrix() {}
GLAPI void APIENTRY glPopMatrix() {}
GLAPI void APIENTRY glCallList( GLuint list) {}

GLAPI void APIENTRY glTexSubImage2D(GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels ) {}

GLAPI void APIENTRY glTexCoord2f(GLfloat s, GLfloat t ) {}
GLAPI void APIENTRY glLineWidth(GLfloat width) {}
GLAPI void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue ) {}
GLAPI void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {}

