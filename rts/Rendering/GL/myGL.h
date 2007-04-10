#ifndef MYGL_H
#define MYGL_H

#define GLEW_STATIC

#include <string>
#include <GL/glew.h>
#include "float3.h"

inline void glVertexf3(const float3 &v)
{
	glVertex3f(v.x,v.y,v.z);
}

inline void glColorf3(const float3& v)
{
	glColor3f(v.x,v.y,v.z);
}

inline void glNormalf3(const float3 &v)
{
	glNormal3f(v.x,v.y,v.z);
}

inline void glTranslatef3(const float3 &v)
{
	glTranslatef(v.x,v.y,v.z);
}

void LoadStartPicture(const std::string& sidePref);
void PrintLoadMsg(const char* text, bool swapbuffers = true);
void UnloadStartPicture();
bool ProgramStringIsNative(GLenum target, const char* filename);
unsigned int LoadVertexProgram(const char* filename);
unsigned int LoadFragmentProgram(const char* filename);

void glClearErrors();
void glSafeDeleteProgram(GLuint program);

void LoadExtensions();
void UnloadExtensions();

class CVertexArray;
CVertexArray* GetVertexArray();

#endif
