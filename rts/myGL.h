#ifndef MYGL_H
#define MYGL_H

#ifdef _WIN32
#ifndef _WINSOCKAPI_
	#define _WINSOCKAPI_
	#include <windows.h>
	#undef _WINSOCKAPI_
#else
	#include <windows.h>
#endif
#endif

#define GLEW_STATIC

#include "glew.h"
#ifdef _WIN32
#include "wglew.h"
#endif
#include "float3.h"

inline void glVertexf3(const float3 &v)
{
	glVertex3f(v.x,v.y,v.z);
}

inline void glNormalf3(const float3 &v)
{
	glNormal3f(v.x,v.y,v.z);
}

inline void glTranslatef3(const float3 &v)
{
	glTranslatef(v.x,v.y,v.z);
}

void LoadStartPicture();
void PrintLoadMsg(const char* text);
void UnloadStartPicture();
bool ProgramStringIsNative(GLenum target, const char* filename);
unsigned int LoadVertexProgram(const char* filename);
unsigned int LoadFragmentProgram(const char* filename);
void CheckParseErrors();

void LoadExtensions();
void UnloadExtensions();

class CVertexArray;
CVertexArray* GetVertexArray();

#endif
