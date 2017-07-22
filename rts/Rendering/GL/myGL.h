/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MY_GL_H
#define _MY_GL_H

#define GLEW_STATIC
#ifndef NOMINMAX
#define NOMINMAX
#endif



#if       defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
	#include <GL/glew.h>
#endif // defined(HEADLESS)


#include "System/float3.h"
#include "System/float4.h"

#include "glStateDebug.h"
#include "glMarkers.h"

#if       defined(HEADLESS)
	// All OpenGL functions should always exists on HEADLESS.
	// If one does not, we should crash, and it has to be added to glstub.c.
	// No runtime check should be performed, or it can be optimized away
	// by the compiler.
	// This also prevents a compile warning.
	#define IS_GL_FUNCTION_AVAILABLE(functionName) true
#else
	// Check if the functions address is non-NULL.
	#define IS_GL_FUNCTION_AVAILABLE(functionName) (functionName != NULL)
#endif // defined(HEADLESS)

#ifndef GL_INVALID_INDEX
	#define GL_INVALID_INDEX -1
#endif


static inline void glVertexf3(const float3& v)    { glVertex3f(v.r, v.g, v.b); }
static inline void glColorf3(const float3& v)     { glColor3f(v.r, v.g, v.b); }
static inline void glColorf4(const float4& v)     { glColor4f(v.r, v.g, v.b, v.a); }
static inline void glNormalf3(const float3& v)    { glNormal3f(v.r, v.g, v.b); }
static inline void glTranslatef3(const float3& v) { glTranslatef(v.r, v.g, v.b); }
static inline void glSecondaryColorf3(const float3& v) { glSecondaryColor3f(v.r, v.g, v.b); }
static inline void glColorf4(const float3& v, const float alpha) { glColor4f(v.r, v.g, v.b, alpha); }
static inline void glUniformf3(const GLint location, const float3& v) { glUniform3f(location, v.r, v.g, v.b); }


typedef   void   (*   glOrthoFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
typedef   void   (*gluOrtho2DFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
typedef   void   (* glFrustumFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);

static inline void __spring_glOrtho_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) { glOrtho(l, r, b, t, n, f); }
static inline void __spring_glOrtho     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) {
	#ifndef UNIT_TEST
	glTranslatef(0.0f, 0.0f, 0.5f);
	glScalef(1.0f, 1.0f, 0.5f);
	glOrtho(l, r,  b, t,  n, f);
	#else
	#error myGL.h included in unit-test?
	#endif
}

static inline void __spring_gluOrtho2D_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t) { __spring_glOrtho_noCC(l, r, b, t, -1.0, 1.0); }
static inline void __spring_gluOrtho2D     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t) { __spring_glOrtho     (l, r, b, t, -1.0, 1.0); }

static inline void __spring_glFrustum_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) { glFrustum(l, r, b, t, n, f); }
static inline void __spring_glFrustum     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) {
	#ifndef UNIT_TEST
	glTranslatef(0.0f, 0.0f, 0.5f);
	glScalef(1.0f, 1.0f, 0.5f);
	glFrustum(l, r,  b, t,  n, f);
	#endif
}

static constexpr    glOrthoFuncPtr    glOrthoFuncs[2] = {__spring_glOrtho_noCC, __spring_glOrtho};
static constexpr gluOrtho2DFuncPtr gluOrtho2DFuncs[2] = {__spring_gluOrtho2D_noCC, __spring_gluOrtho2D};
static constexpr  glFrustumFuncPtr  glFrustumFuncs[2] = {__spring_glFrustum_noCC, __spring_glFrustum};

#undef glOrtho
#undef gluOrtho2D
#undef glFrustum

#define glOrtho       glOrthoFuncs[globalRendering->supportClipSpaceControl]
#define gluOrtho2D gluOrtho2DFuncs[globalRendering->supportClipSpaceControl]
#define glFrustum   glFrustumFuncs[globalRendering->supportClipSpaceControl]



void WorkaroundATIPointSizeBug();
void SetTexGen(const float scaleX, const float scaleZ, const float offsetX, const float offsetZ);

void glSaveTexture(const GLuint textureID, const char* filename);
void glSpringBindTextures(GLuint first, GLsizei count, const GLuint* textures);
void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height);
void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data);

void glSpringMatrix2dProj(const int sizex, const int sizey);

void ClearScreen();

bool ProgramStringIsNative(GLenum target, const char* filename);
unsigned int LoadVertexProgram(const char* filename);
unsigned int LoadFragmentProgram(const char* filename);

void glClearErrors(const char* cls, const char* fnc, bool verbose = false);
void glSafeDeleteProgram(GLuint program);

bool CheckAvailableVideoModes();

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor);
bool ShowDriverWarning(const char* glVendor, const char* glRenderer);


class CVertexArray;
CVertexArray* GetVertexArray();

#endif // _MY_GL_H
