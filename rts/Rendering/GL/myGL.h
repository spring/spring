/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MY_GL_H
#define _MY_GL_H

#define GLEW_STATIC
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#if       defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
	#include <GL/glew.h>
#endif // defined(HEADLESS)

// includes boost now!
#include "System/float3.h"


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


inline void glVertexf3(const float3& v)
{
	glVertex3f(v.x, v.y, v.z);
}

inline void glColorf3(const float3& v)
{
	glColor3f(v.x, v.y, v.z);
}

inline void glNormalf3(const float3& v)
{
	glNormal3f(v.x, v.y, v.z);
}

inline void glTranslatef3(const float3& v)
{
	glTranslatef(v.x, v.y, v.z);
}

inline void glSecondaryColorf3(const float3& v)
{
	glSecondaryColor3f(v.x, v.y, v.z);
}

inline void glColorf4(const float3& v, const float alpha)
{
	glColor4f(v.x, v.y, v.z, alpha);
}

inline void glUniformf3(const GLint location, const float3& v)
{
	glUniform3f(location, v.x, v.y, v.z);
}

void WorkaroundATIPointSizeBug();

void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height);

void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data);

void SetTexGen(const float& scaleX, const float& scaleZ, const float& offsetX, const float& offsetZ);

void ClearScreen();

bool ProgramStringIsNative(GLenum target, const char* filename);
unsigned int LoadVertexProgram(const char* filename);
unsigned int LoadFragmentProgram(const char* filename);

void glClearErrors();
void glSafeDeleteProgram(GLuint program);

void PrintAvailableResolutions();

void LoadExtensions();
void UnloadExtensions();

class CVertexArray;
CVertexArray* GetVertexArray();

#endif // _MY_GL_H
