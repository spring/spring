#include "LuaVAOImpl.h"

#include <algorithm>

#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "lib/sol2/sol.hpp"

#if 0
#include "System/Log/ILog.h"
//			LOG("%s, %f, %p, %d, %d", attr.name.c_str(), *iter, mappedBuf, outValSize, bytesWritten);
#endif
#if 0
#include "System/TimeProfiler.h"
//			SCOPED_TIMER("LuaVAOImpl::UploadImpl::Resize");
#endif
#include "System/SafeUtil.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

LuaVAOImpl::LuaVAOImpl()
	: vertLuaVBO{nullptr}
	, instLuaVBO{nullptr}
	, indxLuaVBO{nullptr}
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
	static bool supported = VBO::IsSupported(GL_ARRAY_BUFFER) && VAO::IsSupported() && GLEW_ARB_instanced_arrays && GLEW_ARB_draw_elements_base_vertex;
	return supported;
}


void LuaVAOImpl::AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget)
{
	if (thisLuaVBO) {
		LuaError("[LuaVAOImpl::%s] LuaVBO already attached", __func__);
	}

	if (luaVBO->defTarget != reqTarget) {
		LuaError("[LuaVAOImpl::%s] LuaVBO should have been created with [%u] target, got [%u] target instead", __func__, reqTarget, luaVBO->defTarget);
	}

	if (!luaVBO->vbo) {
		LuaError("[LuaVAOImpl::%s] LuaVBO is invalid. Did you sucessfully call vbo:Define()?", __func__);
	}

	thisLuaVBO = luaVBO;

	if (vertLuaVBO && instLuaVBO) {
		for (const auto& v : vertLuaVBO->bufferAttribDefs) {
			for (const auto& i : instLuaVBO->bufferAttribDefs) {
				if (v.first == i.first) {
					LuaError("[LuaVAOImpl::%s] Vertex and Instance LuaVBO have defined a duplicate attribute [%d]", __func__, v.first);
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

template<typename ...Args>
void LuaVAOImpl::LuaError(std::string format, Args ...args)
{
	std::string what = fmt::sprintf(format, args...);
	throw std::runtime_error(what.c_str());
}

void LuaVAOImpl::CheckDrawPrimitiveType(GLenum mode)
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
		LuaError("[LuaVAOImpl::%s]: Using deprecated or incorrect primitive type (%d)", __func__, mode);
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

std::pair<GLsizei, GLsizei> LuaVAOImpl::DrawCheck(const GLenum mode, const sol::optional<GLsizei> drawCountOpt, const sol::optional<int> instanceCountOpt, const bool indexed)
{
	GLsizei drawCount;

	if (indexed) {
		if (!indxLuaVBO)
			LuaError("[LuaVAOImpl::%s]: No index buffer is attached. Did you succesfully call vao:AttachIndexBuffer()?", __func__);

		drawCount = drawCountOpt.value_or(indxLuaVBO->elementsCount);
		if (drawCount <= 0)
			drawCount = indxLuaVBO->elementsCount;
	} else {
		const GLsizei drawCountDef = vertLuaVBO ? vertLuaVBO->elementsCount : 1;

		if (!vertLuaVBO && !drawCountOpt.has_value())
			LuaError("[LuaVAOImpl::%s]: In case vertex buffer is not attached, the drawCount param should be set explicitly", __func__);

		drawCount = drawCountOpt.value_or(drawCountDef);
		if (drawCount <= 0)
			drawCount = drawCountDef;
	}

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()
	if (instanceCount > 0 && !instLuaVBO)
		LuaError("[LuaVAOImpl::%s]: Requested rendering of [%d] instances, but no instance buffer is attached. Did you succesfully call vao:AttachInstanceBuffer()?", __func__, instanceCount);

	CheckDrawPrimitiveType(mode);
	CondInitVAO();

	return std::make_pair(drawCount, instanceCount);
}

void LuaVAOImpl::DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> vertexFirstOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> instanceFirstOpt)
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

void LuaVAOImpl::DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt)
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