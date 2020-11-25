#include "LuaVAOImpl.h"

#include <iterator>
#include <limits>
#include <algorithm>

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
#include "Rendering/GlobalRendering.h"

#include "LuaUtils.h"

LuaVAOImpl::LuaVAOImpl(const sol::optional<bool> freqUpdatedOpt, sol::this_state L) :
	L(L),

	vao(nullptr),
	vboVert(nullptr),
	vboInst(nullptr),
	ebo(nullptr),

	freqUpdated(freqUpdatedOpt.value_or(false)),

	numAttributes(0),

	maxVertCount(0),
	maxInstCount(0),
	maxIndxCount(0),

	vertAttribsSizeInBytes(0),
	instAttribsSizeInBytes(0),
	indxElemSizeInBytes(0)
{
	//LOG("LuaVAOImpl()");
}

void LuaVAOImpl::Delete()
{
	spring::SafeDestruct(vao);

	spring::SafeDestruct(vboVert);
	spring::SafeDestruct(vboInst);
	spring::SafeDestruct(ebo);

	vaoAttribs.clear();
}

LuaVAOImpl::~LuaVAOImpl()
{
	//LOG("~LuaVAOImpl()");
	Delete();
}



template<typename TIn, typename TOut>
TOut LuaVAOImpl::TransformFunc(const TIn input) {
	if constexpr(std::is_same<TIn, TOut>::value)
		return input;

	const auto m = static_cast<TIn>(std::numeric_limits<TOut>::min());
	const auto M = static_cast<TIn>(std::min(
		static_cast<TOut>(1 << 24),  //spring has this limitation
		std::numeric_limits<TOut>::max()));

	return static_cast<TOut>(std::clamp(input, m, M));
}


template<typename T>
T LuaVAOImpl::MaybeFunc(const sol::table& tbl, const std::string& key, T defValue) {
	const sol::optional<T> maybeValue = tbl[key];
	return maybeValue.value_or(defValue);
}


/**
* local vertexAtrribs = {
*		[0] = {type = GL.FLOAT, ...}
* }
*/
bool LuaVAOImpl::FillAttribsTableImpl(const sol::table& attrDefTable, const int divisor)
{
	bool atLeastOne = false;
	for (const auto& kv : attrDefTable) {
		const sol::object& key = kv.first;
		const sol::object& value = kv.second;

		if (numAttributes >= LuaVAOImpl::MAX_NUMBER_OF_ATTRIBUTES)
			return false;

		if (!key.is<int>() || value.get_type() != sol::type::table) //key should be int, value should be table i.e. [1] = {}
			continue;

		sol::table vaDefTable = value.as<sol::table>();

		const int attrID = MaybeFunc(vaDefTable, "id", numAttributes);

		if ((attrID < 0) || (attrID > LuaVAOImpl::MAX_NUMBER_OF_ATTRIBUTES) || vaoAttribs.find(attrID) != vaoAttribs.end())
			continue;

		const GLenum type = MaybeFunc(vaDefTable, "type", LuaVAOImpl::DEFAULT_VERT_ATTR_TYPE);
		const GLboolean normalized = MaybeFunc(vaDefTable, "normalized", false) ? GL_TRUE : GL_FALSE;
		const GLint size = std::clamp(MaybeFunc(vaDefTable, "size", 4), 1, 4);
		const std::string name = MaybeFunc(vaDefTable, "name", fmt::format("attr{}", attrID));

		int typeSizeInBytes = 0;

		switch (type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			typeSizeInBytes = sizeof(uint8_t);
			break;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			typeSizeInBytes = sizeof(uint16_t);
			break;
		case GL_INT:
		case GL_UNSIGNED_INT:
			typeSizeInBytes = sizeof(uint32_t);
			break;
		case GL_FLOAT:
			typeSizeInBytes = sizeof(GLfloat);
			break;
		}

		if (typeSizeInBytes == 0)
			continue;

		vaoAttribs[attrID] = {
			divisor,
			size,
			type,
			normalized,
			0, // stride , will be filled up later
			0, // pointer, will be filled up later
			//AUX
			typeSizeInBytes,
			name
		};
		atLeastOne = true;
		++numAttributes;
	}

	return atLeastOne;
}

bool LuaVAOImpl::FillAttribsNumberImpl(const int numFloatAttribs, const int divisor)
{
	if (numFloatAttribs <= 0)
		return false;

	if (numFloatAttribs % 4 != 0) {
		LuaError("Simplified LuaVAO Set{Vertex,Instance}Attributes should come in strides of 4");
		return false;
	}

	bool atLeastOne = false;

	constexpr GLenum type = GL_FLOAT;
	constexpr GLboolean normalized = GL_FALSE;
	constexpr GLint size = 4;
	constexpr int typeSizeInBytes = sizeof(GLfloat);

	const int attrID0 = numAttributes;
	for (int attrID = 0; attrID < std::ceil(numFloatAttribs / 4.0f); ++attrID) {

		const std::string name = fmt::format("attr{}", attrID);

		vaoAttribs[attrID0 + attrID] = {
			divisor,
			size,
			type,
			normalized,
			0, // stride , will be filled up later
			0, // pointer, will be filled up later
			//AUX
			typeSizeInBytes,
			name
		};
		atLeastOne = true;
		++numAttributes;
	}

	return atLeastOne;
}

void LuaVAOImpl::FillAttribsImpl(const sol::object& attrDefObject, const int divisor, int& attribsSizeInBytes)
{
	switch (attrDefObject.get_type()) {
	case sol::type::table:
		if (!FillAttribsTableImpl(attrDefObject.as<const sol::table&>(), divisor))
			return;
		break;
	case sol::type::number:
		if (!attrDefObject.is<int>() || (!FillAttribsNumberImpl(attrDefObject.as<const int>(), divisor)))
			return;
		break;
	default:
		return;
	}

	attribsSizeInBytes = 0;
	GLsizei pointer = 0;

	//second pass is necessary because attrDefTable can be iterated in any order, but vaoAttribs order is guaranteed
	for (auto& kv : vaoAttribs) {
		if (kv.second.divisor != divisor)
			continue;

		const auto vaIndex = kv.first;
		const auto& attr = kv.second;

		const GLsizei attribSizeInBytes = attr.size * attr.typeSizeInBytes;
		attribsSizeInBytes += attribSizeInBytes;

		vaoAttribs[vaIndex].pointer = pointer;

		pointer += attribSizeInBytes; //the pointer points to the next attrib
	}

	//third pass is need to fill in stride value, which is same as sizeof imaginary attribs structure
	for (auto& kv : vaoAttribs) {
		if (kv.second.divisor != divisor)
			continue;

		kv.second.stride = attribsSizeInBytes;

#if 0
		LOG("Attribute name = %s, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d, typeSizeInBytes = %d", kv.second.name.c_str(), kv.second.divisor, kv.second.size, kv.second.type, kv.second.normalized, kv.second.stride, kv.second.pointer, kv.second.typeSizeInBytes);
#endif

	}
}

int LuaVAOImpl::SetVertexAttributes(const int maxVertCount, const sol::object& attrDefObject)
{
	if (vboVert)
		return 0;

	if (maxVertCount <= 0)
		return 0;

	this->maxVertCount = maxVertCount;

	FillAttribsImpl(attrDefObject, 0/*per vertex divisor*/, this->vertAttribsSizeInBytes);

	if (this->vertAttribsSizeInBytes <= 0)
		return 0;

	vboVert = new VBO(GL_ARRAY_BUFFER, MapPersistently());
	vboVert->Bind();
	vboVert->New(this->maxVertCount * this->vertAttribsSizeInBytes, freqUpdated ? GL_STREAM_DRAW : GL_STATIC_DRAW);
	vboVert->Unbind();

	return numAttributes;
}

int LuaVAOImpl::SetInstanceAttributes(const int maxInstCount, const sol::object& attrDefObject)
{
	if (vboInst)
		return 0;

	if (maxInstCount <= 0)
		return 0;

	this->maxInstCount = maxInstCount;

	FillAttribsImpl(attrDefObject, 1/*per instance divisor*/, this->instAttribsSizeInBytes);

	if (this->instAttribsSizeInBytes <= 0)
		return 0;

	vboInst = new VBO(GL_ARRAY_BUFFER, MapPersistently());
	vboInst->Bind();
	vboInst->New(this->maxInstCount * this->instAttribsSizeInBytes, freqUpdated ? GL_STREAM_DRAW : GL_STATIC_DRAW);
	vboInst->Unbind();

	return numAttributes;
}

bool LuaVAOImpl::SetIndexAttributes(const int maxIndxCount, const sol::optional<GLenum> indTypeOpt)
{
	const auto indexType = indTypeOpt.value_or(LuaVAOImpl::DEFAULT_INDX_ATTR_TYPE);
	return SetIndexAttributesImpl(maxIndxCount, indexType);
}

bool LuaVAOImpl::MapPersistently()
{
	return false;
	//return globalRendering->supportPersistentMapping && freqUpdated;
}

bool LuaVAOImpl::CheckPrimType(GLenum mode)
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
		LuaError("%s(): Using deprecated primType (%d)", __func__, mode);
		return false;
	}

	return true;
}

bool LuaVAOImpl::CondInitVAO()
{
	if (vao)
		return true; //already init

	if (this->maxVertCount <= 0)
		return false;

	vao = new VAO();

	////
	vao->Bind();
	////

	//LOG("VAO = %d", vao->GetId());

	if (vboVert) {
		vboVert->Bind();
		//LOG("vboVert = %d", vboVert->GetId());
	}

	if (ebo) {
		ebo->Bind();
		//LOG("ebo = %d", ebo->GetId());
	}

	#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))

	for (const auto& va : vaoAttribs) {
		const auto& attr = va.second;
		if (attr.divisor == 0) {
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, attr.stride, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 0);
			//LOG("attrID = %d, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d", va.first, attr.divisor, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
		}
	}

	if (vboInst) {
		if (vboVert && vboVert->bound) {
			vboVert->Unbind();
		}

		vboInst->Bind();
		//LOG("vboInst = %d", vboInst->GetId());

		for (const auto& va : vaoAttribs) {
			const auto& attr = va.second;
			if (attr.divisor >= 1) {
				glEnableVertexAttribArray(va.first);
				glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, attr.stride, INT2PTR(attr.pointer));
				glVertexAttribDivisor(va.first, attr.divisor);
				//LOG("attrID = %d, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d", va.first, attr.divisor, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
			}
		}
	}

	#undef INT2PTR

	////
	vao->Unbind();
	////

	if (vboVert && vboVert->bound)
		vboVert->Unbind();

	if (vboInst && vboInst->bound)
		vboInst->Unbind();

	if (ebo && ebo->bound)
		ebo->Unbind();

	//restore default state
	for (const auto& va : vaoAttribs) {
		glVertexAttribDivisor(va.first, 0);
		glDisableVertexAttribArray(va.first);
	}

	return true;
}

bool LuaVAOImpl::SetIndexAttributesImpl(const int maxIndxCount, const GLenum indType)
{
	if (ebo)
		return false;

	if (maxIndxCount <= 0)
		return false;

	this->maxIndxCount = maxIndxCount;
	this->indexType = indType;

	int typeSizeInBytes = 0;
	switch (this->indexType) {
	case GL_UNSIGNED_BYTE:
		typeSizeInBytes = sizeof(uint8_t);
		break;
	case GL_UNSIGNED_SHORT:
		typeSizeInBytes = sizeof(uint16_t);
		break;
	case GL_UNSIGNED_INT:
		typeSizeInBytes = sizeof(uint32_t);
		break;
	}

	if (typeSizeInBytes == 0)
		return false;

	this->indxElemSizeInBytes = typeSizeInBytes;
	ebo = new VBO(GL_ELEMENT_ARRAY_BUFFER, MapPersistently());
	ebo->Bind();
	ebo->New(this->maxIndxCount * this->indxElemSizeInBytes, freqUpdated ? GL_STREAM_DRAW : GL_STATIC_DRAW);
	ebo->Unbind();

	return true;
}

int LuaVAOImpl::UploadImpl(const sol::stack_table& luaTblData, const sol::optional<int> offsetOpt, const int divisor, const int* attrNum, const int aSizeInBytes, VBO* vbo)
{
	if (!vbo) {
		LuaError("[%s] Invalid VBO, did you call Set*Attributes?", __func__);
		return 0;
	}

	if (attrNum) {
		if (*attrNum < 0 || *attrNum >= numAttributes) {
			LuaError("[%s] Attibute number (%d) is too large or too small", __func__, *attrNum);
			return 0; //invalid attrNum
		}

		const auto vaoAttribIter = vaoAttribs.find(*attrNum);
		if (vaoAttribIter == vaoAttribs.cend()) {
			LuaError("[%s] Definition of attibute number (%d) not found", __func__, *attrNum);
			return 0; //non-existing attrNum
		}

		if (vaoAttribIter->second.divisor != divisor) {
			LuaError("[%s] Mismatch between attribute upload and attribute definition types", __func__, *attrNum);
			return 0; //wrong divisor
		}
	}


	const int aOffset = std::max(offsetOpt.value_or(0), 0);
	const int byteOffset = aOffset * aSizeInBytes;

	const int byteSize = vbo->GetSize() - byteOffset; //non-precise, but save way

	if (byteSize <= 0)
		return 0;

	int bytesWritten = 0;

	const int luaTblDataSize = luaTblData.size();

	std::vector<lua_Number> dataVec;
	dataVec.resize(luaTblDataSize);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (auto k = 0; k < luaTblDataSize; ++k) {
		dataVec[k] = luaTblData.raw_get_or<lua_Number>(k + 1, defaultValue);
	}

	vbo->Bind();
	auto mappedBuf = vbo->MapBuffer(byteOffset, byteSize, GL_MAP_WRITE_BIT);

	#define TRANSFORM_COPY_ATTRIB(outT, sz, iter, mcpy) \
	{ \
		constexpr int outValSize = sizeof(outT); \
		const int outValSizeStride = sz * outValSize; \
		if (bytesWritten + outValSizeStride > byteSize) { \
			vbo->UnmapBuffer(); \
			vbo->Unbind(); \
			return bytesWritten; \
		} \
		if (mcpy) { \
			for (int n = 0; n < sz; ++n) { \
				const auto outVal = TransformFunc<lua_Number, outT>(*iter); \
				memcpy(mappedBuf, &outVal, outValSize); \
				mappedBuf += outValSize; \
				++iter; \
			} \
		} else { \
			mappedBuf += outValSizeStride; \
		} \
		bytesWritten += outValSizeStride; \
	}

	for (auto bdvIter = dataVec.cbegin(); bdvIter != dataVec.cend(); ) {
		for (const auto& va : vaoAttribs) {
			const auto& attr = va.second;

			// the attribute num we are interested in
			// the divisor we are interested in
			bool mcpy = true;

			if (attr.divisor != divisor)
				continue;

			if (attrNum && *attrNum != va.first)
				mcpy = false; //to skip copying

			switch (attr.type) {
			case GL_BYTE:
				TRANSFORM_COPY_ATTRIB(int8_t, attr.size, bdvIter, mcpy);
				break;
			case GL_UNSIGNED_BYTE:
				TRANSFORM_COPY_ATTRIB(uint8_t, attr.size, bdvIter, mcpy);
				break;
			case GL_SHORT:
				TRANSFORM_COPY_ATTRIB(int16_t, attr.size, bdvIter, mcpy);
				break;
			case GL_UNSIGNED_SHORT:
				TRANSFORM_COPY_ATTRIB(uint16_t, attr.size, bdvIter, mcpy);
				break;
			case GL_INT:
				TRANSFORM_COPY_ATTRIB(int32_t, attr.size, bdvIter, mcpy);
				break;
			case GL_UNSIGNED_INT:
				TRANSFORM_COPY_ATTRIB(uint32_t, attr.size, bdvIter, mcpy);
				break;
			case GL_FLOAT:
				TRANSFORM_COPY_ATTRIB(GLfloat, attr.size, bdvIter, mcpy);
				break;
			}
		}
	}

	#undef TRANSFORM_COPY_ATTRIB

	if (vbo->mapped)
		vbo->UnmapBuffer();
	if (vbo->bound)
		vbo->Unbind();

	return bytesWritten;
}

int LuaVAOImpl::UploadVertexBulk(const sol::stack_table& bulkData, const sol::optional<int> vertexOffsetOpt)
{
	return UploadImpl(bulkData, vertexOffsetOpt, 0, nullptr, this->vertAttribsSizeInBytes, vboVert);
}

int LuaVAOImpl::UploadInstanceBulk(const sol::stack_table& bulkData, const sol::optional<int> instanceOffsetOpt)
{
	return UploadImpl(bulkData, instanceOffsetOpt, 1, nullptr, this->instAttribsSizeInBytes, vboInst);
}

int LuaVAOImpl::UploadVertexAttribute(const int attrIndex, const sol::stack_table& attrData, const sol::optional<int> vertexOffsetOpt)
{
	return UploadImpl(attrData, vertexOffsetOpt, 0, &attrIndex, this->vertAttribsSizeInBytes, vboVert);
}

int LuaVAOImpl::UploadInstanceAttribute(const int attrIndex, const sol::stack_table& attrData, const sol::optional<int> instanceOffsetOpt)
{
	return UploadImpl(attrData, instanceOffsetOpt, 1, &attrIndex, this->instAttribsSizeInBytes, vboInst);
}

int LuaVAOImpl::UploadIndices(const sol::stack_table& indData, const sol::optional<int> indOffsetOpt)
{
	const int indDataSize = indData.size();
	if (indDataSize <= 0)
		return 0;

	if (!ebo) { //allow skipping SetIndexAttributes() call from Lua and set attributes based on defaults and the content of first invocation of UploadInidces()
		if (!SetIndexAttributesImpl(indDataSize, LuaVAOImpl::DEFAULT_INDX_ATTR_TYPE))
			return 0;
	}

	const int aOffset = std::max(indOffsetOpt.value_or(0), 0);
	const int byteOffset = aOffset * indxElemSizeInBytes;

	const int byteSize = ebo->GetSize() - byteOffset; //non-precise, but save way

	if (byteSize <= 0)
		return 0;

	int bytesWritten = 0;

	std::vector<lua_Number> dataVec;
	dataVec.resize(indDataSize);

	for (auto k = 0; k < indDataSize; ++k) {
		const auto& maybeNum = indData.get<sol::optional<lua_Number>>(k + 1);
		dataVec.at(k) = maybeNum.value_or(static_cast<lua_Number>(0));
	}

	ebo->Bind();
	auto mappedBuf = ebo->MapBuffer(byteOffset, byteSize, GL_MAP_WRITE_BIT);

	#define TRANSFORM_COPY_INDEX(outT, iter) \
	{ \
		constexpr int outValSize = sizeof(outT); \
		const auto outVal = TransformFunc<lua_Number, outT>(*iter); \
		if (bytesWritten + outValSize > byteSize) {	\
			ebo->UnmapBuffer(); \
			ebo->Unbind(); \
			return bytesWritten; \
		} \
		memcpy(mappedBuf, &outVal, outValSize); \
		mappedBuf += outValSize; \
		bytesWritten += outValSize; \
	}

	for (auto bdvIter = dataVec.cbegin(); bdvIter != dataVec.cend(); ++bdvIter) {
		switch (indexType) {
		case GL_UNSIGNED_BYTE:
			TRANSFORM_COPY_INDEX(uint8_t, bdvIter);
			break;
		case GL_UNSIGNED_SHORT:
			TRANSFORM_COPY_INDEX(uint16_t, bdvIter);
			break;
		case GL_UNSIGNED_INT:
			TRANSFORM_COPY_INDEX(uint32_t, bdvIter);
			break;
		}
	}

#undef TRANSFORM_COPY_INDEX

	ebo->UnmapBuffer();
	ebo->Unbind();

	return bytesWritten;
}

bool LuaVAOImpl::DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt)
{
	const auto vertCount = std::max(vertCountOpt.value_or(maxVertCount), 0);

	if (this->maxVertCount <= 0)
		return false;

	if (vertCount <= 0)
		return false;

	if (!CheckPrimType(mode))
		return false;

	if (!CondInitVAO())
		return false;

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()
	const auto first = std::max(firstOpt.value_or(0), 0);

	//LOG("mode = %d, first = %d, vertCount = %d, instanceCount = %d", mode, first, vertCount, instanceCount);

	vao->Bind();

	if (instanceCount == 0)
		glDrawArrays(mode, first, vertCount);
	else
		glDrawArraysInstanced(mode, first, vertCount, instanceCount);

	vao->Unbind();

	return true;
}

bool LuaVAOImpl::DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt, const sol::optional<int> baseVertexOpt)
{
	const auto indCount = std::max(indCountOpt.value_or(maxIndxCount), 0);

	if ((this->maxVertCount <= 0) || (this->maxIndxCount <= 0))
		return false;

	if (indCount <= 0)
		return false;

	if (!CheckPrimType(mode))
		return false;

	if (!CondInitVAO())
		return false;

	const auto indElemOffset = std::max(indElemOffsetOpt.value_or(0), 0);
	const auto indElemOffsetInBytes = indElemOffset * indxElemSizeInBytes;
	const auto baseVertex = std::max(baseVertexOpt.value_or(0), 0);
	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()

	vao->Bind();

#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))
	if (instanceCount == 0) {
		if (baseVertex == 0)
			glDrawElements(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes));
		else
			glDrawElementsBaseVertex(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes), baseVertex);
	} else {
		if (baseVertex == 0)
			glDrawElementsInstanced(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes), instanceCount);
		else
			glDrawElementsInstancedBaseVertex(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes), instanceCount, baseVertex);
	}
#undef INT2PTR

	vao->Unbind();

	return true;
}