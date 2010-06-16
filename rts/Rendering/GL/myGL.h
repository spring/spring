/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MYGL_H
#define MYGL_H

#define GLEW_STATIC
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#ifndef HEADLESS
	#include <GL/glew.h>
#else
	#include "lib/headlessStubs/glewstub.h"
#endif
#include "lib/gml/gml.h"

// includes boost now!
#include "float3.h"


inline void glVertexf3(const float3 &v)
{
	glVertex3f(v.x, v.y, v.z);
}

inline void glColorf3(const float3& v)
{
	glColor3f(v.x, v.y, v.z);
}

inline void glNormalf3(const float3 &v)
{
	glNormal3f(v.x, v.y, v.z);
}

inline void glTranslatef3(const float3 &v)
{
	glTranslatef(v.x, v.y, v.z);
}

inline void glSecondaryColorf3(const float3& v)
{
	glSecondaryColor3f(v.x,v.y,v.z);
}

inline void glColorf4(const float3& v, const float& alpha)
{
	glColor4f(v.x,v.y,v.z,alpha);
}

inline void glUniformf3(const GLint& location, const float3 &v)
{
	glUniform3f(location, v.x,v.y,v.z);
}


void glBuildMipmaps(const GLenum target,GLint internalFormat,const GLsizei width,const GLsizei height,const GLenum format,const GLenum type,const void *data);

void SetTexGen(const float& scaleX, const float& scaleZ, const float& offsetX, const float& offsetZ);

void RandomStartPicture(const std::string& sidePref);
void LoadStartPicture(const std::string& picture);
void ClearScreen();
void PrintLoadMsg(const char* text, bool swapbuffers = true);
void UnloadStartPicture();

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

#endif
