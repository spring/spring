/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OPENGLUTILS_H
#define LUA_OPENGLUTILS_H

#include <string>

//#include "LuaInclude.h"
#include "System/type2.h"

class CMatrix44f;
class LuaMatTexture;
struct lua_State;

typedef unsigned int GLuint;

enum LuaMatrixType {
	LUAMATRICES_SHADOW,
	LUAMATRICES_VIEW,
	LUAMATRICES_VIEWINVERSE,
	LUAMATRICES_PROJECTION,
	LUAMATRICES_PROJECTIONINVERSE,
	LUAMATRICES_VIEWPROJECTION,
	LUAMATRICES_VIEWPROJECTIONINVERSE,
	LUAMATRICES_BILLBOARD,
	LUAMATRICES_NONE
};


class LuaOpenGLUtils {
	public:
		static const CMatrix44f* GetNamedMatrix(const std::string& name);
		static bool ParseTextureImage(lua_State* L, LuaMatTexture& texUnit, const std::string& image);
};


class LuaMatTexture {
	public:
		enum Type {
			LUATEX_NONE = 0,

			LUATEX_NAMED,
			LUATEX_LUATEXTURE,
			LUATEX_UNITTEXTURE1,
			LUATEX_UNITTEXTURE2,
			LUATEX_3DOTEXTURE,
			LUATEX_UNITBUILDPIC,
			LUATEX_UNITRADARICON,

			LUATEX_MAP_REFLECTION,
			LUATEX_SKY_REFLECTION,
			LUATEX_SPECULAR,


			LUATEX_SHADOWMAP,
			LUATEX_HEIGHTMAP,

			// NOTE: must be in same order as MAP_BASE_*!
			LUATEX_SMF_GRAT,
			LUATEX_SMF_DETT,
			LUATEX_SMF_MINT,
			LUATEX_SMF_SHAT,
			LUATEX_SMF_NORT,
			// NOTE: must be in same order as MAP_SSMF_*!
			LUATEX_SSMF_NORT,
			LUATEX_SSMF_SPET,
			LUATEX_SSMF_SDIT,
			LUATEX_SSMF_SDET,
			LUATEX_SSMF_SNOT,
			LUATEX_SSMF_SKRT,
			LUATEX_SSMF_LEMT,
			LUATEX_SSMF_PAHT,


			LUATEX_INFOTEX,
			LUATEX_INFOTEX_ACTIVE,
			LUATEX_INFOTEX_LOSMAP,
			LUATEX_INFOTEX_MTLMAP,
			LUATEX_INFOTEX_HGTMAP,
			LUATEX_INFOTEX_BLKMAP,

			LUATEX_MAP_GBUFFER_NORMTEX,
			LUATEX_MAP_GBUFFER_DIFFTEX,
			LUATEX_MAP_GBUFFER_SPECTEX,
			LUATEX_MAP_GBUFFER_EMITTEX,
			LUATEX_MAP_GBUFFER_MISCTEX,
			LUATEX_MAP_GBUFFER_ZVALTEX,

			LUATEX_MODEL_GBUFFER_NORMTEX,
			LUATEX_MODEL_GBUFFER_DIFFTEX,
			LUATEX_MODEL_GBUFFER_SPECTEX,
			LUATEX_MODEL_GBUFFER_EMITTEX,
			LUATEX_MODEL_GBUFFER_MISCTEX,
			LUATEX_MODEL_GBUFFER_ZVALTEX,

			LUATEX_FONT,
			LUATEX_FONTSMALL,
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
