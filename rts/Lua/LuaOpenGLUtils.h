/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OPENGLUTILS_H
#define LUA_OPENGLUTILS_H

#include <string>

//#include "LuaInclude.h"

class CMatrix44f;


class LuaOpenGLUtils {
	public:
		static const CMatrix44f* GetNamedMatrix(const std::string& name);

};

#endif // LUA_OPENGLUTILS_H
