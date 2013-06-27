/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OPENGLUTILS_H
#define LUA_OPENGLUTILS_H

#include <string>

//#include "LuaInclude.h"
#include "System/Vec2.h"

class CMatrix44f;
class LuaMatTexture;
struct lua_State;

typedef unsigned int GLuint;


class LuaOpenGLUtils {
	public:
		static const CMatrix44f* GetNamedMatrix(const std::string& name);
		static bool ParseTextureImage(lua_State* L, LuaMatTexture& texUnit, const std::string& image);
};


class LuaMatTexture {
	public:
		enum Type {
			LUATEX_NONE       = 0,
			LUATEX_NAMED,
			LUATEX_LUATEXTURE,
			LUATEX_UNITTEXTURE1,
			LUATEX_UNITTEXTURE2,
			LUATEX_3DOTEXTURE,
			LUATEX_UNITBUILDPIC,
			LUATEX_UNITRADARICON,
			LUATEX_SHADOWMAP,
			LUATEX_REFLECTION,
			LUATEX_SPECULAR,
			LUATEX_HEIGHTMAP,
			LUATEX_SHADING,
			LUATEX_GRASS,
			LUATEX_FONT,
			LUATEX_FONTSMALL,
			LUATEX_MINIMAP,
			LUATEX_INFOTEX
		};

	public:
		LuaMatTexture()
		: type(LUATEX_NONE), data(NULL), enable(false), enableTexParams(false) {}

		void Finalize();

		void Bind() const;
		void Unbind() const;

		void Print(const std::string& indent) const;

		static int Compare(const LuaMatTexture& a, const LuaMatTexture& b);
		bool operator <(const LuaMatTexture& mt) const {
			return Compare(*this, mt)  < 0;
		}
		bool operator==(const LuaMatTexture& mt) const {
			return Compare(*this, mt) == 0;
		}
		bool operator!=(const LuaMatTexture& mt) const {
			return Compare(*this, mt) != 0;
		}

	public:
		Type type;
		const void* data;
		bool enable;
		bool enableTexParams;

		int2 GetSize() const;
		GLuint GetTextureID() const;
		GLuint GetTextureTarget() const;

	public:
		static const int maxTexUnits = 16;
};

#endif // LUA_OPENGLUTILS_H
