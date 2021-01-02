/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>

#include "lib/sol2/forward.hpp"

#include "Rendering/GL/myGL.h"


struct VAO;
struct VBO;
struct LuaVBOImpl;

// Workaround to continue using lib/sol2/forward.hpp
namespace sol {
	using this_state_container = char[8]; //enough room to hold 64 bit pointer
}

class LuaVAOImpl {
public:
	LuaVAOImpl() = delete;
	LuaVAOImpl(sol::this_state L_);

	LuaVAOImpl(const LuaVAOImpl& lva) = delete; //no copy cons
	LuaVAOImpl(LuaVAOImpl&& lva) = default; //move cons

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported();
public:
	void AttachVertexBuffer(LuaVBOImpl& luaVBO);
	void AttachInstanceBuffer(LuaVBOImpl& luaVBO);
	void AttachIndexBuffer(LuaVBOImpl& luaVBO);

	void DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt);
	void DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt);
private:
	std::pair<GLsizei, GLsizei> DrawCheck(const GLenum mode, const sol::optional<GLsizei> drawCountOpt, const sol::optional<int> instanceCountOpt, const bool indexed);
	void CondInitVAO();
	void CheckDrawPrimitiveType(GLenum mode);
	void AttachBufferImpl(LuaVBOImpl& luaVBO, LuaVBOImpl*& thisLuaVBO, GLenum reqTarget);
private:
	template <typename... Args>
	void LuaError(std::string format, Args... args);
private:
	sol::this_state_container L;
private:
	VAO* vao = nullptr;

	LuaVBOImpl* vertLuaVBO = nullptr;
	LuaVBOImpl* instLuaVBO = nullptr;
	LuaVBOImpl* indxLuaVBO = nullptr;
};

#endif //LUA_VAO_IMPL_H