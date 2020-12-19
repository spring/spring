/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>

#include "Rendering/GL/myGL.h"
#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"

struct VAO;
struct VBO;

struct VAOAttrib {
	int divisor;
	GLint size; // in number of elements
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	GLsizei pointer;
	//AUX
	int typeSizeInBytes;
	std::string name;
};

class LuaVAOImpl {
public:
	LuaVAOImpl() = delete;
	LuaVAOImpl(const sol::optional<bool> freqUpdatedOpt, sol::this_state L);

	LuaVAOImpl(const LuaVAOImpl& lva) = delete; //no copy cons
	LuaVAOImpl(LuaVAOImpl&& lva) = default; //move cons

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported() {
		static bool supported = GLEW_ARB_vertex_buffer_object && GLEW_ARB_vertex_array_object && GLEW_ARB_instanced_arrays && GLEW_ARB_draw_elements_base_vertex;
		return supported;
	};
public:
	int SetVertexAttributes(const int maxVertCount, const sol::object& attrDefObject);
	int SetInstanceAttributes(const int maxInstCount, const sol::object& attrDefObject);
	bool SetIndexAttributes(const int maxIndxCount, const sol::optional<GLenum> indTypeOpt);

	int UploadVertexBulk(const sol::stack_table& bulkData, const sol::optional<int> vertexOffsetOpt);
	int UploadInstanceBulk(const sol::stack_table& bulkData, const sol::optional<int> instanceOffsetOpt);

	int UploadVertexAttribute(const int attrIndex, const sol::stack_table& attrData, const sol::optional<int> vertexOffsetOpt);
	int UploadInstanceAttribute(const int attrIndex, const sol::stack_table& attrData, const sol::optional<int> instanceOffsetOpt);

	int UploadIndices(const sol::stack_table& indData, const sol::optional<int> indOffsetOpt);

	bool DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt);
	bool DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt);
private:
	bool MapPersistently();
	bool CheckPrimType(GLenum mode);
	bool CondInitVAO();
	bool SetIndexAttributesImpl(const int maxIndxCount, const GLenum indType);
	int UploadImpl(const sol::stack_table& luaTblData, const sol::optional<int> offsetOpt, const int divisor, const int* attrNum, const int aSizeInBytes, VBO* vbo);

	bool FillAttribsTableImpl(const sol::table& attrDefTable, const int divisor);
	bool FillAttribsNumberImpl(const int numFloatAttribs, const int divisor);
	void FillAttribsImpl(const sol::object& attrDefTable, const int divisor, int& attribsSizeInBytes);
private:
	template <typename... Args>
	void LuaError(std::string format, Args... args) { luaL_error(L, format.c_str(), args...); };
private:
	template <typename  TIn, typename  TOut>
	static TOut TransformFunc(const TIn input);

	template<typename T>
	static T MaybeFunc(const sol::table& tbl, const std::string& key, T defValue);

	static GLint RoundBuffSizeUp(const int buffSizeSingle) {
		const auto getAllignment = []() {
			GLint buffAlignment = 0;
			glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &buffAlignment);
			return buffAlignment;
		};
		static GLint vboAlignment = getAllignment(); //executed once
		return ((buffSizeSingle / vboAlignment) + 1) * vboAlignment;
	}
private:
	static constexpr int BUFFERING = 3; //unused
	static constexpr int MAX_NUMBER_OF_ATTRIBUTES = 16;
	static constexpr GLenum DEFAULT_VERT_ATTR_TYPE = GL_FLOAT;
	static constexpr GLenum DEFAULT_INDX_ATTR_TYPE = GL_UNSIGNED_SHORT;
private:
	sol::this_state L;
private:
	int numAttributes;

	int maxVertCount;
	int maxInstCount;
	int maxIndxCount;

	int vertAttribsSizeInBytes;
	int instAttribsSizeInBytes;
	int indxElemSizeInBytes;

	GLenum indexType;
	bool freqUpdated;

	VAO* vao;
	VBO* vboVert;
	VBO* vboInst;
	VBO* ebo;

	std::map<int, VAOAttrib> vaoAttribs;
};

#endif //LUA_VAO_IMPL_H