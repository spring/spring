/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>
#include <memory>

#include "lib/sol2/forward.hpp"

#include "Rendering/GL/myGL.h"


struct VAO;
struct VBO;
struct LuaVBOImpl;

class LuaVAOImpl {
public:
	LuaVAOImpl();

	LuaVAOImpl(const LuaVAOImpl& lva) = delete; //no copy cons
	LuaVAOImpl(LuaVAOImpl&& lva) = default; //move cons

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported();
public:
	void AttachVertexBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO);
	void AttachInstanceBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO);
	void AttachIndexBuffer(const std::shared_ptr<LuaVBOImpl>& luaVBO);

	void DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt);
	void DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt);
private:
	std::pair<GLsizei, GLsizei> DrawCheck(const GLenum mode, const sol::optional<GLsizei> drawCountOpt, const sol::optional<int> instanceCountOpt, const bool indexed);
	void CondInitVAO();
	void CheckDrawPrimitiveType(GLenum mode);
	void AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget);
private:
	template <typename... Args>
	void LuaError(std::string format, Args... args);
private:
	VAO* vao = nullptr;

	std::shared_ptr<LuaVBOImpl> vertLuaVBO;
	std::shared_ptr<LuaVBOImpl> instLuaVBO;
	std::shared_ptr<LuaVBOImpl> indxLuaVBO;
};

#endif //LUA_VAO_IMPL_H