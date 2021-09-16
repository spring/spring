/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>
#include <memory>

#include "lib/sol2/forward.hpp"

#include "Rendering/GL/myGL.h"

class VAO;
class VBO;
class LuaVBOImpl;

class LuaVAOImpl {
public:
	LuaVAOImpl();

	LuaVAOImpl(const LuaVAOImpl& lva) = delete;
	LuaVAOImpl(LuaVAOImpl&& lva) = default;

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported();
public:
	using LuaVBOImplSP = std::shared_ptr<LuaVBOImpl>; //my workaround to https://github.com/ThePhD/sol2/issues/1206
public:
	void AttachVertexBuffer(const LuaVBOImplSP& luaVBO);
	void AttachInstanceBuffer(const LuaVBOImplSP& luaVBO);
	void AttachIndexBuffer(const LuaVBOImplSP& luaVBO);

	void DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> vertexFirstOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> instanceFirstOpt);
	void DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt);
private:
	std::pair<GLsizei, GLsizei> DrawCheck(const GLenum mode, const sol::optional<GLsizei> drawCountOpt, const sol::optional<int> instanceCountOpt, const bool indexed);
	void CondInitVAO();
	void CheckDrawPrimitiveType(GLenum mode);
	void AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget);
private:
	VAO* vao = nullptr;

	std::shared_ptr<LuaVBOImpl> vertLuaVBO;
	std::shared_ptr<LuaVBOImpl> instLuaVBO;
	std::shared_ptr<LuaVBOImpl> indxLuaVBO;
};

#endif //LUA_VAO_IMPL_H
