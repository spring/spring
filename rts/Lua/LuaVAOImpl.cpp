#include "LuaVAOImpl.h"

#include <algorithm>
#include <type_traits>

#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "lib/sol2/sol.hpp"

#include "System/SafeUtil.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

LuaVAOImpl::LuaVAOImpl()
	: vao{nullptr}

	, vertLuaVBO{nullptr}
	, instLuaVBO{nullptr}
	, indxLuaVBO{nullptr}

	, baseInstance{0u}
{

}

void LuaVAOImpl::Delete()
{
	//LOG_L(L_WARNING, "[LuaVAOImpl::%s]", __func__);
	if (vertLuaVBO) {
		vertLuaVBO = nullptr;
	}

	if (instLuaVBO) {
		instLuaVBO = nullptr;
	}

	if (indxLuaVBO) {
		indxLuaVBO = nullptr;
	}

	spring::SafeDestruct(vao);
}

LuaVAOImpl::~LuaVAOImpl()
{
	Delete();
}

bool LuaVAOImpl::Supported()
{
	static bool supported = VBO::IsSupported(GL_ARRAY_BUFFER) && VAO::IsSupported() && GLEW_ARB_instanced_arrays && GLEW_ARB_draw_elements_base_vertex && GLEW_ARB_multi_draw_indirect;
	return supported;
}


void LuaVAOImpl::AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget)
{
	if (thisLuaVBO) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO already attached", __func__);
	}

	if (luaVBO->defTarget != reqTarget) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO should have been created with [%u] target, got [%u] target instead", __func__, reqTarget, luaVBO->defTarget);
	}

	if (!luaVBO->vbo) {
		LuaUtils::SolLuaError("[LuaVAOImpl::%s] LuaVBO is invalid. Did you sucessfully call vbo:Define()?", __func__);
	}

	thisLuaVBO = luaVBO;

	if (vertLuaVBO && instLuaVBO) {
		for (const auto& v : vertLuaVBO->bufferAttribDefs) {
			for (const auto& i : instLuaVBO->bufferAttribDefs) {
				if (v.first == i.first) {
					LuaUtils::SolLuaError("[LuaVAOImpl::%s] Vertex and Instance LuaVBO have defined a duplicate attribute [%d]", __func__, v.first);
				}
			}
		}
	}
}

void LuaVAOImpl::AttachVertexBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, vertLuaVBO, GL_ARRAY_BUFFER);
}

void LuaVAOImpl::AttachInstanceBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, instLuaVBO, GL_ARRAY_BUFFER);
}

void LuaVAOImpl::AttachIndexBuffer(const LuaVBOImplSP& luaVBO)
{
	AttachBufferImpl(luaVBO, indxLuaVBO, GL_ELEMENT_ARRAY_BUFFER);
}

template<typename TObj>
const SIndexAndCount LuaVAOImpl::GetDrawIndicesImpl(int id)
{
	const TObj* obj = LuaUtils::SolIdToObject<TObj>(id, __func__);
	return GetDrawIndicesImpl<TObj>(obj);
}

template<typename TObj>
const SIndexAndCount LuaVAOImpl::GetDrawIndicesImpl(const TObj* obj)
{
	static_assert(std::is_base_of_v<CSolidObject, TObj> || std::is_base_of_v<SolidObjectDef, TObj>);

	S3DModel * model = obj->model;
	assert(model);
	return SIndexAndCount(model->indxStart, model->indxCount);
}

template<typename TObj>
void LuaVAOImpl::AddObjectsToSubmissionImpl(int id)
{
	DrawCheck(GL_TRIANGLES, 0, submitCmds.size() + 1, true); //pair<indxCount,instCount>
	submitCmds.emplace_back(DrawObjectGetCmdImpl<TObj>(id));
}

template<typename TObj>
void LuaVAOImpl::AddObjectsToSubmissionImpl(const sol::stack_table& ids)
{
	const std::size_t idsSize = ids.size(); //size() is very costly to do in the loop
	DrawCheck(GL_TRIANGLES, 0, submitCmds.size() + idsSize, true); //pair<indxCount,instCount>

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (std::size_t i = 0u; i < idsSize; ++i) {
		lua_Number idLua = ids.raw_get_or<lua_Number>(i + 1, defaultValue);
		int id = spring::SafeCast<lua_Number, int>(idLua);

		submitCmds.emplace_back(DrawObjectGetCmdImpl<TObj>(id));
	}
}

template<typename TObj>
SDrawElementsIndirectCommand LuaVAOImpl::DrawObjectGetCmdImpl(int id)
{
	const auto& indexAndCount = LuaVAOImpl::GetDrawIndicesImpl<TObj>(id);

	return std::move(SDrawElementsIndirectCommand{
		indexAndCount.count,
		1u,
		indexAndCount.index,
		0u,
		baseInstance++
	});
}

void LuaVAOImpl::CheckDrawPrimitiveType(GLenum mode) const
{
	switch (mode) {
	case GL_POINTS:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_LINES:
	case GL_LINE_STRIP_ADJACENCY:
	case GL_LINES_ADJACENCY:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP_ADJACENCY:
	case GL_TRIANGLES_ADJACENCY:
	case GL_PATCHES:
		break;
	default:
		LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Using deprecated or incorrect primitive type (%d)", __func__, mode);
	}
}

void LuaVAOImpl::CondInitVAO()
{
	if (vao)
		return; //already init

	vao = new VAO();
	vao->Bind();

	if (vertLuaVBO)
		vertLuaVBO->vbo->Bind(GL_ARRAY_BUFFER); //type is needed cause same buffer could have been rebounded as something else using LuaVBO functions

	if (indxLuaVBO)
		indxLuaVBO->vbo->Bind(GL_ELEMENT_ARRAY_BUFFER);

	#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))

	GLenum indMin = ~0u;
	GLenum indMax =  0u;

	const auto glVertexAttribPointerFunc = [](GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
		if (type == GL_FLOAT)
			glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		else //assume int types
			glVertexAttribIPointer(index, size, type, stride, pointer);
	};

	if (vertLuaVBO)
		for (const auto& va : vertLuaVBO->bufferAttribDefsVec) {
			const auto& attr = va.second;
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointerFunc(va.first, attr.size, attr.type, attr.normalized, vertLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 0);
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
			indMax = std::max(indMax, static_cast<GLenum>(va.first));
		}

	if (instLuaVBO) {
		if (vertLuaVBO)
			vertLuaVBO->vbo->Unbind();

		instLuaVBO->vbo->Bind(GL_ARRAY_BUFFER);

		for (const auto& va : instLuaVBO->bufferAttribDefsVec) {
			const auto& attr = va.second;
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointerFunc(va.first, attr.size, attr.type, attr.normalized, instLuaVBO->elemSizeInBytes, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 1);
			indMin = std::min(indMin, static_cast<GLenum>(va.first));
			indMax = std::max(indMax, static_cast<GLenum>(va.first));
		}
	}

	#undef INT2PTR

	vao->Unbind();

	if (vertLuaVBO && vertLuaVBO->vbo->bound)
		vertLuaVBO->vbo->Unbind();

	if (instLuaVBO && instLuaVBO->vbo->bound)
		instLuaVBO->vbo->Unbind();

	if (indxLuaVBO && indxLuaVBO->vbo->bound)
		indxLuaVBO->vbo->Unbind();

	//restore default state
	for (GLuint index = indMin; index <= indMax; ++index) {
		glVertexAttribDivisor(index, 0);
		glDisableVertexAttribArray(index);
	}
}

std::pair<GLsizei, GLsizei> LuaVAOImpl::DrawCheck(GLenum mode, sol::optional<GLsizei> drawCountOpt, sol::optional<int> instanceCountOpt, bool indexed)
{
	GLsizei drawCount;

	if (indexed) {
		if (!indxLuaVBO)
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: No index buffer is attached. Did you succesfully call vao:AttachIndexBuffer()?", __func__);

		drawCount = drawCountOpt.value_or(indxLuaVBO->elementsCount);
		if (drawCount <= 0)
			drawCount = indxLuaVBO->elementsCount;
	} else {
		const GLsizei drawCountDef = vertLuaVBO ? vertLuaVBO->elementsCount : 1;

		if (!vertLuaVBO && !drawCountOpt.has_value())
			LuaUtils::SolLuaError("[LuaVAOImpl::%s]: In case vertex buffer is not attached, the drawCount param should be set explicitly", __func__);

		drawCount = drawCountOpt.value_or(drawCountDef);
		if (drawCount <= 0)
			drawCount = drawCountDef;
	}

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()
	if (instanceCount > 0 && !instLuaVBO)
		LuaUtils::SolLuaError("[LuaVAOImpl::%s]: Requested rendering of [%d] instances, but no instance buffer is attached. Did you succesfully call vao:AttachInstanceBuffer()?", __func__, instanceCount);

	CheckDrawPrimitiveType(mode);
	CondInitVAO();

	return std::make_pair(drawCount, instanceCount);
}

void LuaVAOImpl::DrawArrays(GLenum mode, sol::optional<GLsizei> vertCountOpt, sol::optional<GLint> vertexFirstOpt, sol::optional<int> instanceCountOpt, sol::optional<int> instanceFirstOpt)
{
	const auto [vertCount, instCount] = DrawCheck(mode, vertCountOpt, instanceCountOpt, false); //pair<vertCount,instCount>

	const auto vertexFirst = std::max(vertexFirstOpt.value_or(0), 0);
	const auto baseInstance = std::max(instanceFirstOpt.value_or(0), 0);

	vao->Bind();

	if (instCount == 0)
		glDrawArrays(mode, vertexFirst, vertCount);
	else {
		if (baseInstance > 0)
			glDrawArraysInstancedBaseInstance(mode, vertexFirst, vertCount, instCount, baseInstance);
		else
			glDrawArraysInstanced(mode, vertexFirst, vertCount, instCount);
	}

	vao->Unbind();
}

void LuaVAOImpl::DrawElements(GLenum mode, sol::optional<GLsizei> indCountOpt, sol::optional<int> indElemOffsetOpt, sol::optional<int> instanceCountOpt, sol::optional<int> baseVertexOpt)
{
	const auto [indxCount, instCount] = DrawCheck(mode, indCountOpt, instanceCountOpt, true); //pair<indxCount,instCount>

	const auto indElemOffset = std::max(indElemOffsetOpt.value_or(0), 0);
	const auto indElemOffsetInBytes = indElemOffset * indxLuaVBO->elemSizeInBytes;
	const auto baseVertex = std::max(baseVertexOpt.value_or(0), 0);

	const auto indexType = indxLuaVBO->bufferAttribDefsVec[0].second.type;

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(indxLuaVBO->primitiveRestartIndex);

	vao->Bind();

#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))
	if (instCount == 0) {
		if (baseVertex == 0)
			glDrawElements(mode, indxCount, indexType, INT2PTR(indElemOffsetInBytes));
		else
			glDrawElementsBaseVertex(mode, indxCount, indexType, INT2PTR(indElemOffsetInBytes), baseVertex);
	} else {
		if (baseVertex == 0)
			glDrawElementsInstanced(mode, indxCount, indexType, INT2PTR(indElemOffsetInBytes), instCount);
		else
			glDrawElementsInstancedBaseVertex(mode, indxCount, indexType, INT2PTR(indElemOffsetInBytes), instCount, baseVertex);
	}
#undef INT2PTR

	vao->Unbind();

	glDisable(GL_PRIMITIVE_RESTART);
}

void LuaVAOImpl::ClearSubmission()
{
	baseInstance = 0u;
	submitCmds.clear();
}

void LuaVAOImpl::AddUnitsToSubmission(int id)
{
	AddObjectsToSubmissionImpl<CUnit>(id);
}

void LuaVAOImpl::AddUnitsToSubmission(const sol::stack_table& ids)
{
	AddObjectsToSubmissionImpl<CUnit>(ids);
}

void LuaVAOImpl::AddFeaturesToSubmission(int id)
{
	AddObjectsToSubmissionImpl<CFeature>(id);
}

void LuaVAOImpl::AddFeaturesToSubmission(const sol::stack_table& ids)
{
	AddObjectsToSubmissionImpl<CFeature>(ids);
}

void LuaVAOImpl::AddUnitDefsToSubmission(int id)
{
	AddObjectsToSubmissionImpl<UnitDef>(id);
}

void LuaVAOImpl::AddUnitDefsToSubmission(const sol::stack_table& ids)
{
	AddObjectsToSubmissionImpl<UnitDef>(ids);
}

void LuaVAOImpl::AddFeatureDefsToSubmission(int id)
{
	AddObjectsToSubmissionImpl<FeatureDef>(id);
}

void LuaVAOImpl::AddFeatureDefsToSubmission(const sol::stack_table& ids)
{
	AddObjectsToSubmissionImpl<FeatureDef>(ids);
}

void LuaVAOImpl::Submit()
{
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(indxLuaVBO->primitiveRestartIndex);

	vao->Bind();
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, submitCmds.data(), submitCmds.size(), sizeof(SDrawElementsIndirectCommand));
	vao->Unbind();

	glDisable(GL_PRIMITIVE_RESTART);
}
