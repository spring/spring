/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLSL_COPY_STATE_H
#define _GLSL_COPY_STATE_H

#include "System/UnorderedMap.hpp"
#include "Shader.h"
#include "ShaderStates.h"

typedef unsigned int GLuint;

namespace Shader {
	/**
	 * @brief
	 * Copies all hidden states related to a GLSL program,
	 * in particular uniforms, sampler bindings, UBO bindings,
	 * feedback state, geoshader state, ...
	 *
	 * @example
	 * Very useful when you want to recompile an uber-shader
	 * with a different #define-flagset.
	 */
	void GLSLCopyState(GLuint newProgID, GLuint oldProgID, spring::unordered_map<std::size_t, UniformState, fast_hash>* uniformStates);
}

#endif //_GLSL_COPY_STATE_H
