/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MY_GL_H
#define _MY_GL_H

#ifndef NOMINMAX
#define NOMINMAX
#endif



#if defined(HEADLESS)
	#include "lib/headlessStubs/glewstub.h"
#else
		#include <GL/glew.h>
#endif // defined(HEADLESS)


#include "AttribState.hpp"
#include "MatrixState.hpp"
#include "glStateDebug.h"


#ifndef GL_INVALID_INDEX
	#define GL_INVALID_INDEX -1
#endif


void glSaveTexture(const GLuint textureID, const char* filename);
void glSpringBindTextures(GLuint first, GLsizei count, const GLuint* textures);
void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height);
void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data);

void ClearScreen();

void glClearErrors(const char* cls, const char* fnc, bool verbose = false);

bool CheckAvailableVideoModes();

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor);
bool ShowDriverWarning(const char* glVendor, const char* glRenderer);


namespace GL {
	struct RenderDataBuffer;
	template<typename VertexArrayType> struct TRenderDataBuffer;

	enum {
		SHADER_TYPE_VS  = 0, // vertex
		SHADER_TYPE_FS  = 1, // fragment
		SHADER_TYPE_GS  = 2, // geometry
		SHADER_TYPE_CS  = 3, // compute
		SHADER_TYPE_TCS = 4, // tess-ctrl
		SHADER_TYPE_TES = 5, // tess-eval
		SHADER_TYPE_CNT = 6,
	};

	inline const/*[C++14]expr*/ unsigned int ShaderTypeToEnum(GLuint glType) {
		switch (glType) {
			case GL_VERTEX_SHADER         : { return SHADER_TYPE_VS ; } break;
			case GL_FRAGMENT_SHADER       : { return SHADER_TYPE_FS ; } break;
			case GL_GEOMETRY_SHADER       : { return SHADER_TYPE_GS ; } break;
			case GL_COMPUTE_SHADER        : { return SHADER_TYPE_CS ; } break;
			case GL_TESS_CONTROL_SHADER   : { return SHADER_TYPE_TCS; } break;
			case GL_TESS_EVALUATION_SHADER: { return SHADER_TYPE_TES; } break;
		}

		return -1u;
	}

	inline const/*[C++14]expr*/ GLuint ShaderEnumToType(unsigned int enumType) {
		switch (enumType) {
			case SHADER_TYPE_VS : { return GL_VERTEX_SHADER         ; } break;
			case SHADER_TYPE_FS : { return GL_FRAGMENT_SHADER       ; } break;
			case SHADER_TYPE_GS : { return GL_GEOMETRY_SHADER       ; } break;
			case SHADER_TYPE_CS : { return GL_COMPUTE_SHADER        ; } break;
			case SHADER_TYPE_TCS: { return GL_TESS_CONTROL_SHADER   ; } break;
			case SHADER_TYPE_TES: { return GL_TESS_EVALUATION_SHADER; } break;
		}

		return -1u;
	}

	inline const/*[C++14]expr const*/ char* ShaderEnumToStr(unsigned int enumType) {
		switch (enumType) {
			case SHADER_TYPE_VS : { return  "vs"; } break;
			case SHADER_TYPE_FS : { return  "fs"; } break;
			case SHADER_TYPE_GS : { return  "gs"; } break;
			case SHADER_TYPE_CS : { return  "cs"; } break;
			case SHADER_TYPE_TCS: { return "tcs"; } break;
			case SHADER_TYPE_TES: { return "tes"; } break;
		}

		return "";
	}
};


#endif // _MY_GL_H
