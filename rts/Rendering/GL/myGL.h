/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MY_GL_H
#define _MY_GL_H

#define GLEW_STATIC
#ifndef NOMINMAX
#define NOMINMAX
#endif



#if defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
	#if defined(__APPLE__)
		#include <OpenGL/glew.h>
	#else
		#include <GL/glew.h>
	#endif
#endif // defined(HEADLESS)


#include "AttribState.hpp"
#include "MatrixState.hpp"
#include "glStateDebug.h"


#ifndef GL_INVALID_INDEX
	#define GL_INVALID_INDEX -1
#endif


// replaced by Matrix::{Ortho,Persp}Proj
#undef glOrtho
#undef gluOrtho2D
#undef glFrustum


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
