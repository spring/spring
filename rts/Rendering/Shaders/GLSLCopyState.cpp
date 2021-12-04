/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GLSLCopyState.h"
#include "Shader.h"
#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"


#define LOG_SECTION_SHADER "Shader"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SHADER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SHADER


#if !defined(HEADLESS)

enum {
	FLOAT1 = 1,
	FLOAT2,
	FLOAT3,
	FLOAT4,
	INT1,
	INT2,
	INT3,
	INT4,
	UNSIGNED1,
	UNSIGNED2,
	UNSIGNED3,
	UNSIGNED4,
	FLOAT_MAT2,
	FLOAT_MAT3,
	FLOAT_MAT4,
	ATOMIC
};

static const spring::unordered_map<GLenum, int> bindingTypeMap = {
	{GL_FLOAT     , FLOAT1},
	{GL_FLOAT_VEC2, FLOAT2},
	{GL_FLOAT_VEC3, FLOAT3},
	{GL_FLOAT_VEC4, FLOAT4},

	{GL_BOOL     , INT1},
	{GL_BOOL_VEC2, INT2},
	{GL_BOOL_VEC3, INT3},
	{GL_BOOL_VEC4, INT4},

	{GL_INT     , INT1},
	{GL_INT_VEC2, INT2},
	{GL_INT_VEC3, INT3},
	{GL_INT_VEC4, INT4},

	{GL_UNSIGNED_INT     , UNSIGNED1},
	{GL_UNSIGNED_INT_VEC2, UNSIGNED2},
	{GL_UNSIGNED_INT_VEC3, UNSIGNED3},
	{GL_UNSIGNED_INT_VEC4, UNSIGNED4},

	{GL_FLOAT_MAT2, FLOAT_MAT2},
	{GL_FLOAT_MAT3, FLOAT_MAT3},
	{GL_FLOAT_MAT4, FLOAT_MAT4},
/*
	{GL_FLOAT_MAT2x3
	{GL_FLOAT_MAT2x4
	{GL_FLOAT_MAT3x2
	{GL_FLOAT_MAT3x4
	{GL_FLOAT_MAT4x2
	{GL_FLOAT_MAT4x3

	{GL_DOUBLE
	{GL_DOUBLE_VEC2
	{GL_DOUBLE_VEC3
	{GL_DOUBLE_VEC4
	{GL_DOUBLE_MAT2
	{GL_DOUBLE_MAT3
	{GL_DOUBLE_MAT4
	{GL_DOUBLE_MAT2x3
	{GL_DOUBLE_MAT2x4
	{GL_DOUBLE_MAT3x2
	{GL_DOUBLE_MAT3x4
	{GL_DOUBLE_MAT4x2
	{GL_DOUBLE_MAT4x3
*/

	{GL_SAMPLER_1D                  , INT1},
	{GL_SAMPLER_2D                  , INT1},
	{GL_SAMPLER_3D                  , INT1},
	{GL_SAMPLER_CUBE                , INT1},
	{GL_SAMPLER_1D_SHADOW           , INT1},
	{GL_SAMPLER_2D_SHADOW           , INT1},
	{GL_SAMPLER_1D_ARRAY            , INT1},
	{GL_SAMPLER_2D_ARRAY            , INT1},
	{GL_SAMPLER_1D_ARRAY_SHADOW     , INT1},
	{GL_SAMPLER_2D_ARRAY_SHADOW     , INT1},
	{GL_SAMPLER_2D_MULTISAMPLE      , INT1},
	{GL_SAMPLER_2D_MULTISAMPLE_ARRAY, INT1},
	{GL_SAMPLER_CUBE_SHADOW         , INT1},
	{GL_SAMPLER_BUFFER              , INT1},
	{GL_SAMPLER_2D_RECT             , INT1},
	{GL_SAMPLER_2D_RECT_SHADOW      , INT1},

	{GL_INT_SAMPLER_1D                  , INT1},
	{GL_INT_SAMPLER_2D                  , INT1},
	{GL_INT_SAMPLER_3D                  , INT1},
	{GL_INT_SAMPLER_CUBE                , INT1},
	{GL_INT_SAMPLER_1D_ARRAY            , INT1},
	{GL_INT_SAMPLER_2D_ARRAY            , INT1},
	{GL_INT_SAMPLER_2D_MULTISAMPLE      , INT1},
	{GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, INT1},
	{GL_INT_SAMPLER_BUFFER              , INT1},
	{GL_INT_SAMPLER_2D_RECT             , INT1},

	{GL_UNSIGNED_INT_SAMPLER_1D                  , INT1},
	{GL_UNSIGNED_INT_SAMPLER_2D                  , INT1},
	{GL_UNSIGNED_INT_SAMPLER_3D                  , INT1},
	{GL_UNSIGNED_INT_SAMPLER_CUBE                , INT1},
	{GL_UNSIGNED_INT_SAMPLER_1D_ARRAY            , INT1},
	{GL_UNSIGNED_INT_SAMPLER_2D_ARRAY            , INT1},
	{GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE      , INT1},
	{GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, INT1},
	{GL_UNSIGNED_INT_SAMPLER_BUFFER              , INT1},
	{GL_UNSIGNED_INT_SAMPLER_2D_RECT             , INT1},

/*
	{GL_IMAGE_1D                  , INT1},
	{GL_IMAGE_2D                  , INT1},
	{GL_IMAGE_3D                  , INT1},
	{GL_IMAGE_2D_RECT             , INT1},
	{GL_IMAGE_CUBE                , INT1},
	{GL_IMAGE_BUFFER              , INT1},
	{GL_IMAGE_1D_ARRAY            , INT1},
	{GL_IMAGE_2D_ARRAY            , INT1},
	{GL_IMAGE_2D_MULTISAMPLE      , INT1},
	{GL_IMAGE_2D_MULTISAMPLE_ARRAY, INT1},

	{GL_INT_IMAGE_1D                           , INT1},
	{GL_INT_IMAGE_2D                           , INT1},
	{GL_INT_IMAGE_3D                           , INT1},
	{GL_INT_IMAGE_2D_RECT                      , INT1},
	{GL_INT_IMAGE_CUBE                         , INT1},
	{GL_INT_IMAGE_BUFFER                       , INT1},
	{GL_INT_IMAGE_1D_ARRAY                     , INT1},
	{GL_INT_IMAGE_2D_ARRAY                     , INT1},
	{GL_INT_IMAGE_2D_MULTISAMPLE               , INT1},
	{GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY         , INT1},

	{GL_UNSIGNED_INT_IMAGE_1D                  , INT1},
	{GL_UNSIGNED_INT_IMAGE_2D                  , INT1},
	{GL_UNSIGNED_INT_IMAGE_3D                  , INT1},
	{GL_UNSIGNED_INT_IMAGE_2D_RECT             , INT1},
	{GL_UNSIGNED_INT_IMAGE_CUBE                , INT1},
	{GL_UNSIGNED_INT_IMAGE_BUFFER              , INT1},
	{GL_UNSIGNED_INT_IMAGE_1D_ARRAY            , INT1},
	{GL_UNSIGNED_INT_IMAGE_2D_ARRAY            , INT1},
	{GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE      , INT1},
	{GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, INT1},

	{GL_UNSIGNED_INT_ATOMIC_COUNTER, ATOMIC},
*/
};



static void CopyUniforms(GLuint newProgID, GLuint oldProgID, spring::unordered_map<std::size_t, Shader::UniformState, fast_hash>* uniformStates)
{
	GLsizei numUniforms = 0;
	GLsizei maxUniformNameLength = 0;

	glGetProgramiv(newProgID, GL_ACTIVE_UNIFORMS, &numUniforms);
	glGetProgramiv(newProgID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);

	char name[Shader::UniformState::NAME_BUF_LEN];

	if (maxUniformNameLength <= 0)
		return;
	if (maxUniformNameLength >= sizeof(name))
		return;

	glUseProgram(newProgID);

	for (int i = 0; i < numUniforms; ++i) {
		GLsizei nameLength = 0;
		GLint size = 0;
		GLenum type = 0;
		glGetActiveUniform(newProgID, i, maxUniformNameLength, &nameLength, &size, &type, &name[0]);
		name[nameLength] = 0;

		if (nameLength == 0)
			continue;

		// oldLoc is only used when we don't have data in our own state tracker
		const GLint newLoc = glGetUniformLocation(newProgID, name);
		      GLint oldLoc = -1;

		if (newLoc < 0)
			continue;

		// try to find old data for the uniform either in the old shader itself or in our own state tracker
		const size_t hash = hashString(name);
		const auto hashIt = uniformStates->find(hash);

		Shader::UniformState* oldUniformState = nullptr;

		if (hashIt != uniformStates->end()) {
			oldUniformState = &hashIt->second;
		} else {
			// uniform not found in state tracker, try to read it from old shader object
			const auto pair = uniformStates->emplace(hash, name);
			const auto iter = pair.first;

			oldUniformState = &iter->second;
		}

		oldUniformState->SetLocation(newLoc);

		// check if we have data we can use to initialize the uniform
		if (!oldUniformState->IsInitialized()) {
			if (oldProgID == 0) {
				oldLoc = -1;
				continue;
			}

			// no old data found, so we cannot initialize the uniform
			if ((oldLoc = glGetUniformLocation(oldProgID, name)) < 0)
				continue;

			//FIXME read data from old shader & save data in _new_ uniformState?
		}

		// initialize the uniform with previous data
		const auto typeIt = bindingTypeMap.find(type);

		if (typeIt == bindingTypeMap.end())
			continue;

		switch (typeIt->second) {
			#define HANDLE_TYPE(type, size, ftype, internalTypeName)                                      \
				case type##size: {                                                                        \
					internalTypeName _value[size];                                                        \
			                                                                                              \
					if (oldLoc >= 0) {                                                                    \
						glGetUniform##ftype(oldProgID, oldLoc, &_value[0]);                               \
					} else {                                                                              \
						memcpy(_value, oldUniformState->GetIntValues(), size * sizeof(internalTypeName)); \
					}                                                                                     \
			                                                                                              \
					glUniform##size##ftype(newLoc, 1, &_value[0]);                                        \
				} break;

			#define HANDLE_MATTYPE(type, size, ftype, internalTypeName)                                        \
				case type##size: {                                                                             \
					internalTypeName _value[size*size];                                                        \
			                                                                                                   \
					if (oldLoc >= 0) {                                                                         \
						glGetUniform##ftype(oldProgID, oldLoc, &_value[0]);                                    \
					} else {                                                                                   \
						memcpy(_value, oldUniformState->GetIntValues(), size*size * sizeof(internalTypeName)); \
					}                                                                                          \
			                                                                                                   \
					glUniformMatrix##size##ftype(newLoc, 1, false, &_value[0]);                                \
				} break;

			HANDLE_TYPE(INT, 1, iv, GLint)
			HANDLE_TYPE(INT, 2, iv, GLint)
			HANDLE_TYPE(INT, 3, iv, GLint)
			HANDLE_TYPE(INT, 4, iv, GLint)
			HANDLE_TYPE(FLOAT, 1, fv, GLfloat)
			HANDLE_TYPE(FLOAT, 2, fv, GLfloat)
			HANDLE_TYPE(FLOAT, 3, fv, GLfloat)
			HANDLE_TYPE(FLOAT, 4, fv, GLfloat)
			HANDLE_TYPE(UNSIGNED, 1, uiv, GLuint)
			HANDLE_TYPE(UNSIGNED, 2, uiv, GLuint)
			HANDLE_TYPE(UNSIGNED, 3, uiv, GLuint)
			HANDLE_TYPE(UNSIGNED, 4, uiv, GLuint)

			HANDLE_MATTYPE(FLOAT_MAT, 2, fv, GLfloat)
			HANDLE_MATTYPE(FLOAT_MAT, 3, fv, GLfloat)
			HANDLE_MATTYPE(FLOAT_MAT, 4, fv, GLfloat)

			case ATOMIC: {
				assert(false);
				/*GLint binding;
				glGetActiveAtomicCounterBufferiv(oldProgID, i, GL_ATOMIC_COUNTER_BUFFER_BINDING, &binding);
				glUniform1f(newLoc, 1, binding);*/
			} break;

			default:
				LOG_L(L_WARNING, "Unknown GLSL uniform \"%s\" has unknown vartype \"%X\"", name, type);
		}
	}

	glUseProgram(0);
}


static void CopyUniformBlocks(GLuint newProgID, GLuint oldProgID)
{
	GLint numUniformBlocks = 0;
	GLint maxNameLength = 0;

	glGetProgramiv(oldProgID, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
	glGetProgramiv(oldProgID, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxNameLength);

	char name[512];

	if (maxNameLength <= 0)
		return;
	if (maxNameLength >= sizeof(name))
		return;

	for (int i = 0; i < numUniformBlocks; ++i) {
		GLsizei nameLength = 0;
		glGetActiveUniformBlockName(oldProgID, i, maxNameLength, &nameLength, &name[0]);
		name[maxNameLength - 1] = 0;

		if (nameLength == 0)
			continue;

		GLuint oldLoc = glGetUniformBlockIndex(oldProgID, name);
		GLuint newLoc = glGetUniformBlockIndex(newProgID, name);

		if (oldLoc == GL_INVALID_INDEX || newLoc == GL_INVALID_INDEX)
			continue;

		GLint value;
		glGetActiveUniformBlockiv(oldProgID, oldLoc, GL_UNIFORM_BLOCK_BINDING, &value);
		glUniformBlockBinding(newProgID, newLoc, value);
	}
}


static void CopyShaderStorage(GLuint newProgID, GLuint oldProgID)
{
#ifdef GL_ARB_program_interface_query
	if (!GLEW_ARB_program_interface_query)
		return;

	GLint numUniformBlocks = 0;
	GLint maxNameLength = 0;

	glGetProgramInterfaceiv(oldProgID, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numUniformBlocks);
	glGetProgramInterfaceiv(oldProgID, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &maxNameLength);

	char name[512];

	if (maxNameLength <= 0)
		return;
	if (maxNameLength >= sizeof(name))
		return;

	for (int i = 0; i < numUniformBlocks; ++i) {
		GLsizei nameLength = 0;
		glGetProgramResourceName(oldProgID, GL_SHADER_STORAGE_BLOCK, i, maxNameLength, &nameLength, &name[0]);
		name[maxNameLength - 1] = 0;

		if (nameLength == 0)
			continue;

		GLuint oldLoc = glGetProgramResourceIndex(oldProgID, GL_SHADER_STORAGE_BLOCK, name);
		GLuint newLoc = glGetProgramResourceIndex(newProgID, GL_SHADER_STORAGE_BLOCK, name);

		if (oldLoc == GL_INVALID_INDEX || newLoc == GL_INVALID_INDEX)
			continue;

		GLint value;
		constexpr GLenum props[] = {GL_BUFFER_BINDING};
		glGetProgramResourceiv(oldProgID,
			GL_SHADER_STORAGE_BLOCK, oldLoc,
			1, props,
			1, nullptr, &value);
		glShaderStorageBlockBinding(newProgID, newLoc, value);
	}
#endif
}


static void CopyAttributes(GLuint newProgID, GLuint oldProgID)
{
	GLsizei numAttributes = 0;
	GLsizei maxNameLength = 0;

	glGetProgramiv(oldProgID, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	glGetProgramiv(oldProgID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);

	char name[512];

	if (maxNameLength <= 0)
		return;
	if (maxNameLength >= sizeof(name))
		return;

	for (int i = 0; i < numAttributes; ++i) {
		GLsizei nameLength = 0;
		GLint size = 0;
		GLenum type = 0;
		glGetActiveAttrib(oldProgID, i, maxNameLength, &nameLength, &size, &type, &name[0]);
		name[maxNameLength - 1] = 0;

		if (nameLength == 0)
			continue;

		GLint oldLoc = glGetAttribLocation(oldProgID, name);

		if (oldLoc < 0)
			continue;

		glBindAttribLocation(newProgID, oldLoc, name);
	}
}


static void CopyTransformFeedback(GLuint newProgID, GLuint oldProgID)
{
	GLint bufferMode = 0;
	GLint numVaryings = 0;
	GLint maxNameLength = 0;

	glGetProgramiv(oldProgID, GL_TRANSFORM_FEEDBACK_BUFFER_MODE, &bufferMode);
	glGetProgramiv(oldProgID, GL_TRANSFORM_FEEDBACK_VARYINGS, &numVaryings);
	glGetProgramiv(oldProgID, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, &maxNameLength);

	char name[512];

	if (numVaryings == 0)
		return;
	if (maxNameLength <= 0)
		return;
	if (maxNameLength >= sizeof(name))
		return;

	std::vector<std::string> varyings(numVaryings);
	std::vector<GLchar*> varyingsPtr(numVaryings);

	for (int i = 0; i < numVaryings; ++i) {
		GLsizei nameLength = 0;
		GLsizei size = 0;
		GLenum type = 0;
		glGetTransformFeedbackVarying(oldProgID, i, maxNameLength, &nameLength, &size, &type, &name[0]);
		name[maxNameLength - 1] = 0;

		if (nameLength == 0)
			continue;

		varyings[i].assign(name, nameLength + 1);
		varyingsPtr[i] = &varyings[i][0];
	}

	glTransformFeedbackVaryings(newProgID, numVaryings, (const GLchar**)&varyingsPtr[0], bufferMode);
}



static bool ContainsGeometryShader(GLuint oldProgID)
{
	bool ret = false;

	GLsizei numAttachedShaders = 0;
	GLuint attachedShaderIDs[3] = {0};
	GLint attachedShaderType = 0;

	// glGetProgramiv(oldProgID, GL_ATTACHED_SHADERS, &numAttachedShaders);
	glGetAttachedShaders(oldProgID, 3, &numAttachedShaders, &attachedShaderIDs[0]);

	for (GLsizei n = 0; n < numAttachedShaders; n++) {
		glGetShaderiv(attachedShaderIDs[n], GL_SHADER_TYPE, &attachedShaderType);

		if ((ret |= (attachedShaderType == GL_GEOMETRY_SHADER))) {
			break;
		}
	}

	return ret;
}

static void CopyGeometry(GLuint newProgID, GLuint oldProgID)
{
	// "GL_INVALID_OPERATION is generated if pname is GL_GEOMETRY_VERTICES_OUT,
	// GL_GEOMETRY_INPUT_TYPE, or GL_GEOMETRY_OUTPUT_TYPE, and program does not
	// contain a geometry shader."
	if (!ContainsGeometryShader(oldProgID))
		return;

	GLint verticesOut = 0;
	GLint inputType = 0;
	GLint outputType = 0;

	glGetProgramiv(oldProgID, GL_GEOMETRY_INPUT_TYPE, &inputType);
	glGetProgramiv(oldProgID, GL_GEOMETRY_OUTPUT_TYPE, &outputType);
	glGetProgramiv(oldProgID, GL_GEOMETRY_VERTICES_OUT, &verticesOut);

	if (inputType != 0)   glProgramParameteri(newProgID, GL_GEOMETRY_INPUT_TYPE, inputType);
	if (outputType != 0)  glProgramParameteri(newProgID, GL_GEOMETRY_OUTPUT_TYPE, outputType);
	if (verticesOut != 0) glProgramParameteri(newProgID, GL_GEOMETRY_VERTICES_OUT, verticesOut);
}
#endif



namespace Shader {
	void GLSLCopyState(GLuint newProgID, GLuint oldProgID, spring::unordered_map<std::size_t, UniformState, fast_hash>* uniformStates)
	{
	#if !defined(HEADLESS)
		CopyUniforms(newProgID, oldProgID, uniformStates);

		if (oldProgID == 0)
			return;

		CopyUniformBlocks(newProgID, oldProgID);
		CopyShaderStorage(newProgID, oldProgID);
		CopyAttributes(newProgID, oldProgID);
		CopyTransformFeedback(newProgID, oldProgID);
		CopyGeometry(newProgID, oldProgID);
	#endif
	}
}

