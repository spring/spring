/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LUA_SHADER_CONTAINER_H
#define _LUA_SHADER_CONTAINER_H

#include <string>

namespace Shader {
	struct IProgramObject;

	bool LoadFromLua(Shader::IProgramObject* program, const std::string& filename);
};

#endif // _LUA_SHADER_CONTAINER_H
