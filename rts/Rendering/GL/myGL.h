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

// replaced by Matrix::{Ortho,Persp}Proj
#undef glOrtho
#undef gluOrtho2D
#undef glFrustum


void WorkaroundATIPointSizeBug();

void glSaveTexture(const GLuint textureID, const char* filename);
void glSpringBindTextures(GLuint first, GLsizei count, const GLuint* textures);
void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height);
void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data);

// SetupVP loads/pushes view first, then proj
// SetupPV loads/pushes proj first, then view
// pv := pushView, pp := pushProj
void glSpringMatrix2dSetupVP(float l, float r, float b, float t, float n, float f,  bool pv = false, bool pp = false);
void glSpringMatrix2dSetupPV(float l, float r, float b, float t, float n, float f,  bool pv = false, bool pp = false);
// ResetVP pops view first, then proj
// ResetPV pops proj first, then view
// pv := popView, pp := popProj
void glSpringMatrix2dResetVP(bool pv = false, bool pp = false);
void glSpringMatrix2dResetPV(bool pv = false, bool pp = false);

void ClearScreen();

void glClearErrors(const char* cls, const char* fnc, bool verbose = false);

bool CheckAvailableVideoModes();

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor);
bool ShowDriverWarning(const char* glVendor, const char* glRenderer);


class CVertexArray* GetVertexArray();

namespace GL {
	struct RenderDataBuffer;
	template<typename VertexArrayType> struct TRenderDataBuffer;
};

#endif // _MY_GL_H
