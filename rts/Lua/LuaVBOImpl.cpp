#include "LuaVBOImpl.h"

#include <unordered_map>
#include <algorithm>
#include <sstream>

#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "Rendering/GL/VBO.h"

#include "LuaUtils.h"

LuaVBOImpl::LuaVBOImpl(const sol::optional<GLenum> defTargetOpt, const sol::optional<bool> freqUpdatedOpt, sol::this_state L_):
	defTarget{defTargetOpt.value_or(GL_ARRAY_BUFFER)}, freqUpdated{freqUpdatedOpt.value_or(false)},
	elemSizeInBytes{0u}, elementsCount{0u}, bufferSizeInBytes{0u}, attributesCount{0u}, vaoAttachCount{0u}
{
	memcpy(&L[0], &L_, std::min(sizeof(sol::this_state_container), sizeof(sol::this_state)));
}

LuaVBOImpl::~LuaVBOImpl()
{
	DeleteImpl();
}

void LuaVBOImpl::DeleteImpl()
{
	//safe to call multiple times
	spring::SafeDestruct(vbo);
	bufferAttribDefs.clear();
}

void LuaVBOImpl::Delete()
{
	if (vaoAttachCount != 0)
		LuaError("[LuaVBOImpl::%s] Attempt to Delete a VBO attached to one or more VAOs. Delete() VAOs first", __func__);

	DeleteImpl();
}

/////////////////////////////////

bool LuaVBOImpl::IsTypeValid(GLenum type)
{
	const auto arrayBufferValidType = [type]() {
		switch (type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
			return true;
		default:
			return false;
		};
	};

	const auto ubossboValidType = [type]() {
		switch (type) {
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_FLOAT_MAT4:
			return true;
		default:
			return false;
		};
	};

	switch (defTarget) {
	case GL_ARRAY_BUFFER:
		return arrayBufferValidType();
	case GL_UNIFORM_BUFFER:
	case GL_SHADER_STORAGE_BUFFER: //assume std140 for now for SSBO
		return ubossboValidType();
	default:
		return false;
	};
}

void LuaVBOImpl::GetTypePtr(GLenum type, GLint size, uint32_t& thisPointer, uint32_t& nextPointer, GLsizei& alignment, GLsizei& sizeInBytes)
{
	const auto tightParams = [type, size](GLsizei& sz, GLsizei& al) -> bool {
		switch (type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE: {
			sz = 1; al = 1;
		} break;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT: {
			sz = 2; al = 2;
		} break;
		case GL_INT:
		case GL_UNSIGNED_INT: {
			sz = 4; al = 4;
		} break;
		case GL_FLOAT: {
			sz = 4; al = 4;
		} break;
		default:
			return false;
		}

		sz *= size;

		return true;
	};

	// commented section below is probably terribly bugged. Avoid using something not multiple of i/u/b/vec4 as plague
	const auto std140Params = [type, size](GLsizei& sz, GLsizei& al) -> bool {
		const auto std140ArrayRule = [size, &sz, &al]() {
			if (size > 1) {
				//al = (al > 16) ? al : 16;
				al = 16;
				sz += (size - 1) * al;
			}
		};

		switch (type) {
		/*
		case GL_BYTE:
		case GL_UNSIGNED_BYTE: {
			sz = 1; al = 1;
			std140ArrayRule();
		} break;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT: {
			sz = 2; al = 2;
			std140ArrayRule();
		} break;
		case GL_INT:
		case GL_UNSIGNED_INT: {
			sz = 4; al = 4;
			std140ArrayRule();
		} break;
		case GL_FLOAT: {
			sz = 4; al = 4;
			std140ArrayRule();
		} break;
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2: {
			sz = 8; al = 8;
			std140ArrayRule();
		} break;
		case GL_FLOAT_VEC3:
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3: {
			sz = 12; al = 16;
			std140ArrayRule();
		} break;
		*/
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4: {
			sz = 16; al = 16;
			std140ArrayRule();
		} break;
		case GL_FLOAT_MAT4: {
			sz = 64; al = 16;
			std140ArrayRule();
		} break;
		default:
			return false;
		}

		return true;
	};

	switch (defTarget) {
	case GL_ARRAY_BUFFER: {
		if (!tightParams(sizeInBytes, alignment))
			return;
	} break;
	case GL_UNIFORM_BUFFER:
	case GL_SHADER_STORAGE_BUFFER: { //assume std140 for now for SSBO
		if (!std140Params(sizeInBytes, alignment))
			return;
	} break;
	default:
		return;
	}

	thisPointer = AlignUp(nextPointer, alignment);
	nextPointer = thisPointer + sizeInBytes;
}

bool LuaVBOImpl::FillAttribsTableImpl(const sol::table& attrDefTable)
{
	uint32_t attributesCountMax;
	GLenum typeDefault;
	GLint sizeDefault;
	GLint sizeMax;

	if (defTarget == GL_ARRAY_BUFFER) {
		attributesCountMax = LuaVBOImpl::VA_NUMBER_OF_ATTRIBUTES;
		typeDefault = LuaVBOImpl::DEFAULT_VERT_ATTR_TYPE;
		sizeDefault = 4;
		sizeMax = 4;
	} else {
		attributesCountMax = ~0u;
		typeDefault = LuaVBOImpl::DEFAULT_BUFF_ATTR_TYPE;
		sizeDefault = 1;
		sizeMax = 1 << 12;
	};

	for (const auto& kv : attrDefTable) {
		const sol::object& key = kv.first;
		const sol::object& value = kv.second;

		if (attributesCount >= attributesCountMax)
			return false;

		if (!key.is<int>() || value.get_type() != sol::type::table) //key should be int, value should be table i.e. [1] = {}
			continue;

		sol::table vaDefTable = value.as<sol::table>();

		const int attrID = MaybeFunc(vaDefTable, "id", attributesCount);

		if ((attrID < 0) || (attrID > attributesCountMax))
			continue;

		if (bufferAttribDefs.find(attrID) != bufferAttribDefs.cend())
			continue;

		const GLenum type = MaybeFunc(vaDefTable, "type", typeDefault);

		if (!IsTypeValid(type)) {
			LOG_L(L_ERROR, "[LuaVBOImpl::%s] Invalid attribute type [%u] for selected buffer type [%u]", __func__, type, defTarget);
			continue;
		}

		const GLboolean normalized = MaybeFunc(vaDefTable, "normalized", false) ? GL_TRUE : GL_FALSE;
		const GLint size = std::clamp(MaybeFunc(vaDefTable, "size", sizeDefault), 1, sizeMax);
		const std::string name = MaybeFunc(vaDefTable, "name", fmt::format("attr{}", attrID));

		bufferAttribDefs[attrID] = {
			type,
			size, // in number of elements of type
			normalized, //VAO only
			name,
			//AUX
			0, //to be filled later
			0, //to be filled later
			0  //to be filled later
		};

		++attributesCount;
	};

	if (bufferAttribDefs.empty())
		return false;

	uint32_t nextPointer = 0u;
	uint32_t thisPointer;
	GLsizei fieldAlignment, fieldSizeInBytes;

	for (auto& kv : bufferAttribDefs) { //guaranteed increasing order of key
		auto& baDef = kv.second;

		GetTypePtr(baDef.type, baDef.size, thisPointer, nextPointer, fieldAlignment, fieldSizeInBytes);
		baDef.pointer = static_cast<GLsizei>(thisPointer);
		baDef.strideSizeInBytes = fieldSizeInBytes; //nextPointer - thisPointer;
		baDef.typeSizeInBytes = fieldSizeInBytes / baDef.size;
	};

	elemSizeInBytes = nextPointer; //TODO check if correct in case alignment != size
	return true;
}

bool LuaVBOImpl::FillAttribsNumberImpl(const int numVec4Attribs)
{
	uint32_t attributesCountMax;
	GLenum typeDefault;
	GLint sizeDefault;

	if (defTarget == GL_ARRAY_BUFFER) {
		attributesCountMax = LuaVBOImpl::VA_NUMBER_OF_ATTRIBUTES;
		typeDefault = LuaVBOImpl::DEFAULT_VERT_ATTR_TYPE;
		sizeDefault = 4;
	}
	else {
		attributesCountMax = ~0u;
		typeDefault = LuaVBOImpl::DEFAULT_BUFF_ATTR_TYPE;
		sizeDefault = 1;
	};

	if (numVec4Attribs > attributesCountMax) {
		LuaError("[LuaVBOImpl::%s] Invalid number of vec4 arguments [%d], exceeded maximum of [%u]", __func__, numVec4Attribs, attributesCountMax);
	}

	uint32_t nextPointer = 0u;
	uint32_t thisPointer;
	GLsizei fieldAlignment, fieldSizeInBytes;

	for (int attrID = 0; attrID < numVec4Attribs; ++attrID) {
		const GLenum type = typeDefault;

		const GLboolean normalized = GL_FALSE;
		const GLint size = sizeDefault;
		const std::string name = fmt::format("attr{}", attrID);

		GetTypePtr(type, size, thisPointer, nextPointer, fieldAlignment, fieldSizeInBytes);

		bufferAttribDefs[attrID] = {
			type,
			size, // in number of elements of type
			normalized, //VAO only
			name,
			//AUX
			static_cast<GLsizei>(thisPointer), //pointer
			fieldSizeInBytes / size, // typeSizeInBytes
			fieldSizeInBytes // strideSizeInBytes
		};
	}

	elemSizeInBytes = nextPointer; //TODO check if correct in case alignment != size
	return true;
}

bool LuaVBOImpl::DefineElementArray(const sol::optional<sol::object> attribDefArgOpt)
{
	GLenum indexType = LuaVBOImpl::DEFAULT_INDX_ATTR_TYPE;

	if (attribDefArgOpt.has_value()) {
		if (attribDefArgOpt.value().is<int>())
			indexType = attribDefArgOpt.value().as<int>();
		else
			LuaError("[LuaVBOImpl::%s] Invalid argument object type [%d]. Must be a valid GL type constant", __func__, attribDefArgOpt.value().get_type());
	}

	switch (indexType) {
	case GL_UNSIGNED_BYTE:
		elemSizeInBytes = sizeof(uint8_t);
		break;
	case GL_UNSIGNED_SHORT:
		elemSizeInBytes = sizeof(uint16_t);
		break;
	case GL_UNSIGNED_INT:
		elemSizeInBytes = sizeof(uint32_t);
		break;
	}

	if (elemSizeInBytes == 0u) {
		LuaError("[LuaVBOImpl::%s] Invalid GL type constant [%d]", __func__, indexType);
	}

	bufferAttribDefs[0] = {
		indexType,
		1, // in number of elements of type
		GL_FALSE, //VAO only
		"index",
		//AUX
		0, //pointer
		static_cast<GLsizei>(elemSizeInBytes), // typeSizeInBytes
		static_cast<GLsizei>(elemSizeInBytes) // strideSizeInBytes
	};

	return true;
}

void LuaVBOImpl::Define(const int elementsCount, const sol::optional<sol::object> attribDefArgOpt)
{
	if (vbo) {
		LuaError("[LuaVBOImpl::%s] Attempt to call %s multiple times. VBO definition is immutable.", __func__);
	}

	if (elementsCount <= 0) {
		LuaError("[LuaVBOImpl::%s] Elements count cannot be <= 0", __func__);
	}

	this->elementsCount = elementsCount;

	const auto defineBufferFunc = [this](const sol::object& attribDefArg) {
		if (attribDefArg.get_type() == sol::type::table)
			return FillAttribsTableImpl(attribDefArg.as<sol::table>());

		if (attribDefArg.is<int>())
			return FillAttribsNumberImpl(attribDefArg.as<const int>());

		LuaError("[LuaVBOImpl::%s] Invalid argument object type [%d]. Must be a number or table", __func__);
		return false;
	};

	bool result;
	switch (defTarget) {
	case GL_ELEMENT_ARRAY_BUFFER:
		result = DefineElementArray(attribDefArgOpt);
		break;
	case GL_ARRAY_BUFFER:
	case GL_UNIFORM_BUFFER:
	case GL_SHADER_STORAGE_BUFFER: {
		if (!attribDefArgOpt.has_value())
			LuaError("[LuaVBOImpl::%s] Function has to contain non-empty second argument", __func__);
		result = defineBufferFunc(attribDefArgOpt.value());
	} break;
	default:
		LuaError("[LuaVBOImpl::%s] Invalid buffer target [%u]", __func__, defTarget);
	}

	if (!result) {
		LuaError("[LuaVBOImpl::%s] Error in definition. See infolog for possible reasons", __func__);
	}

	CopyAttrMapToVec();
	AllocGLBuffer(elemSizeInBytes * elementsCount);
}

size_t LuaVBOImpl::Upload(const sol::stack_table& luaTblData, const sol::optional<int> elemOffsetOpt, const sol::optional<int> attribIdxOpt)
{
	if (!vbo) {
		LuaError("[LuaVBOImpl::%s] Invalid VBO. Did you call :Define()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	if (elemOffset >= elementsCount) {
		LuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] >= elementsCount [%u]", __func__, elemOffset, elementsCount);
	}

	const int attribIdx = std::max(attribIdxOpt.value_or(-1), -1);
	if (attribIdx != -1 && bufferAttribDefs.find(attribIdx) == bufferAttribDefs.cend()) {
		LuaError("[LuaVBOImpl::%s] attribIdx is not found in bufferAttribDefs", __func__);
	}

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;
	const int luaTblDataSize = luaTblData.size();

	std::vector<lua_Number> dataVec;
	dataVec.resize(luaTblDataSize);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (auto k = 0; k < luaTblDataSize; ++k) {
		dataVec[k] = luaTblData.raw_get_or<lua_Number>(k + 1, defaultValue);
	}

	vbo->Bind();
	const int mappedBufferSizeInBytes = bufferSizeInBytes - bufferOffsetInBytes;
	auto mappedBuf = vbo->MapBuffer(bufferOffsetInBytes, mappedBufferSizeInBytes, GL_MAP_WRITE_BIT);

	/*
	#define TRANSFORM_COPY_ATTRIB(outT, sz, iter, mcpy) \
	{ \
		constexpr int outValSize = sizeof(outT); \
		const int outValSizeStride = sz * outValSize; \
		if (bytesWritten + outValSizeStride > mappedBufferSizeInBytes) { \
			vbo->UnmapBuffer(); \
			vbo->Unbind(); \
			LuaError("[LuaVBOImpl::%s] Upload array contains too much data", __func__); \
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
	*/

	int bytesWritten = 0;
	for (auto bdvIter = dataVec.cbegin(); bdvIter < dataVec.cend(); ) {
		for (const auto& va : bufferAttribDefsVec) {
			const int   attrID  = va.first;
			const auto& attrDef = va.second;

			int basicTypeSize = attrDef.size;

			//vec4, uvec4, ivec4, mat4, etc...
			// for the purpose of type cast we need basic types
			if (attrDef.typeSizeInBytes > 4) {
				assert(attrDef.typeSizeInBytes % 4 == 0);
				basicTypeSize *= attrDef.typeSizeInBytes >> 2; // / 4;
			}

			bool copyData = attribIdx == -1 || attribIdx == attrID; // copy data if specific attribIdx is not requested or requested and matches attrID

			#define TRANSFORM_AND_WRITE(T) { \
				if (!TransformAndWrite<T>(bytesWritten, mappedBuf, mappedBufferSizeInBytes, basicTypeSize, bdvIter, copyData)) { \
					vbo->UnmapBuffer(); \
					vbo->Unbind(); \
					return bytesWritten; \
				} \
			}

			switch (attrDef.type) {
			case GL_BYTE:
				TRANSFORM_AND_WRITE(int8_t)
				break;
			case GL_UNSIGNED_BYTE:
				TRANSFORM_AND_WRITE(uint8_t);
				break;
			case GL_SHORT:
				TRANSFORM_AND_WRITE(int16_t);
				break;
			case GL_UNSIGNED_SHORT:
				TRANSFORM_AND_WRITE(uint16_t);
				break;
			case GL_INT:
			case GL_INT_VEC4:
				TRANSFORM_AND_WRITE(int32_t);
				break;
			case GL_UNSIGNED_INT:
			case GL_UNSIGNED_INT_VEC4:
				TRANSFORM_AND_WRITE(uint32_t);
				break;
			case GL_FLOAT:
			case GL_FLOAT_VEC4:
			case GL_FLOAT_MAT4:
				TRANSFORM_AND_WRITE(GLfloat);
				break;
			}

			#undef TRANSFORM_AND_WRITE
		}
	}

	vbo->UnmapBuffer();
	vbo->Unbind();
	return bytesWritten;
}

sol::as_table_t<std::vector<lua_Number>> LuaVBOImpl::Download(const sol::optional<int> elemOffsetOpt, const sol::optional<int> elemCountOpt, const sol::optional<int> attribIdxOpt)
{
	std::vector<lua_Number> dataVec;

	if (!vbo) {
		LuaError("[LuaVBOImpl::%s] Invalid VBO. Did you call :Define()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	const uint32_t elemCount = static_cast<uint32_t>(std::clamp(elemCountOpt.value_or(elementsCount), 1, static_cast<int>(elementsCount)));

	if (elemOffset + elemCount > elementsCount) {
		LuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] + elemCount [%u] >= elementsCount [%u]", __func__, elemOffset, elemCount, elementsCount);
	}

	const int attribIdx = std::max(attribIdxOpt.value_or(-1), -1);
	if (attribIdx != -1 && bufferAttribDefs.find(attribIdx) == bufferAttribDefs.cend()) {
		LuaError("[LuaVBOImpl::%s] attribIdx is not found in bufferAttribDefs", __func__);
	}

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;

	vbo->Bind();
	const int mappedBufferSizeInBytes = bufferSizeInBytes - bufferOffsetInBytes;
	auto mappedBuf = vbo->MapBuffer(bufferOffsetInBytes, mappedBufferSizeInBytes, GL_MAP_READ_BIT);

	int bytesRead = 0;

	for (int e = 0; e < elemCount; ++e) {
		for (const auto& va : bufferAttribDefsVec) {
			const int   attrID = va.first;
			const auto& attrDef = va.second;

			int basicTypeSize = attrDef.size;

			//vec4, uvec4, ivec4, mat4, etc...
			// for the purpose of type cast we need basic types
			if (attrDef.typeSizeInBytes > 4) {
				assert(attrDef.typeSizeInBytes % 4 == 0);
				basicTypeSize *= attrDef.typeSizeInBytes >> 2; // / 4;
			}

			bool copyData = attribIdx == -1 || attribIdx == attrID; // copy data if specific attribIdx is not requested or requested and matches attrID

			#define TRANSFORM_AND_READ(T) { \
				if (!TransformAndRead<T>(bytesRead, mappedBuf, mappedBufferSizeInBytes, basicTypeSize, dataVec, copyData)) { \
					vbo->UnmapBuffer(); \
					vbo->Unbind(); \
					return sol::as_table(dataVec); \
				} \
			}

			switch (attrDef.type) {
			case GL_BYTE:
				TRANSFORM_AND_READ(int8_t);
				break;
			case GL_UNSIGNED_BYTE:
				TRANSFORM_AND_READ(uint8_t);
				break;
			case GL_SHORT:
				TRANSFORM_AND_READ(int16_t);
				break;
			case GL_UNSIGNED_SHORT:
				TRANSFORM_AND_READ(uint16_t);
				break;
			case GL_INT:
			case GL_INT_VEC4:
				TRANSFORM_AND_READ(int32_t);
				break;
			case GL_UNSIGNED_INT:
			case GL_UNSIGNED_INT_VEC4:
				TRANSFORM_AND_READ(uint32_t);
				break;
			case GL_FLOAT:
			case GL_FLOAT_VEC4:
			case GL_FLOAT_MAT4:
				TRANSFORM_AND_READ(GLfloat);
				break;
			}

			#undef TRANSFORM_AND_READ
		}
	}

	vbo->UnmapBuffer();
	vbo->Unbind();
	return sol::as_table(dataVec);
}

int LuaVBOImpl::BindBufferRangeImpl(const GLuint index,  const sol::optional<int> elemOffsetOpt, const sol::optional<int> elemCountOpt, const sol::optional<GLenum> targetOpt, const bool bind)
{
	if (!vbo) {
		LuaError("[LuaVBOImpl::%s] Buffer definition is invalid. Did you succesfully call :Define()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	const uint32_t elemCount = static_cast<uint32_t>(std::clamp(elemCountOpt.value_or(elementsCount), 1, static_cast<int>(elementsCount)));

	if (elemOffset + elemCount > elementsCount) {
		LuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] + elemCount [%u] > elementsCount [%u]", __func__, elemOffset, elemCount, elementsCount);
	}

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;

	// can't use bufferSizeInBytes here, cause vbo->BindBufferRange expects binding with UBO/SSBO alignment
	// need to use real GPU buffer size, because it's sized with alignment in mind
	const int boundBufferSizeInBytes = /*bufferSizeInBytes*/ vbo->GetSize() - bufferOffsetInBytes;

	GLenum target = targetOpt.value_or(defTarget);
	if (target != GL_UNIFORM_BUFFER && target != GL_SHADER_STORAGE_BUFFER) {
		LuaError("[LuaVBOImpl::%s] (Un)binding target can only be equal to [%u] or [%u]", __func__, GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER);
	}
	defTarget = target;

	GLuint bindingIndex = index;
	switch (defTarget) {
	case GL_UNIFORM_BUFFER: {
		bindingIndex += uboMinIndex;
	} break;
	case GL_SHADER_STORAGE_BUFFER: {
		bindingIndex += ssboMinIndex;
	} break;
	default:
		LuaError("[LuaVBOImpl::%s] (Un)binding target can only be equal to [%u] or [%u]", __func__, GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER);
	}

	bool result = false;
	if (bind) {
		result = vbo->BindBufferRange(defTarget, bindingIndex, bufferOffsetInBytes, boundBufferSizeInBytes);
	} else {
		result = vbo->UnbindBufferRange(defTarget, bindingIndex, bufferOffsetInBytes, boundBufferSizeInBytes);
	}
	if (!result) {
		LuaError("[LuaVBOImpl::%s] Error (un)binding. See infolog for possible reasons", __func__);
	}

	return result ? bindingIndex : -1;
}

int LuaVBOImpl::BindBufferRange(const GLuint index, const sol::optional<int> elemOffsetOpt, const sol::optional<int> elemCountOpt, const sol::optional<GLenum> targetOpt)
{
	return BindBufferRangeImpl(index, elemOffsetOpt, elemCountOpt, targetOpt, true);
}

int LuaVBOImpl::UnbindBufferRange(const GLuint index, const sol::optional<int> elemOffsetOpt, const sol::optional<int> elemCountOpt, const sol::optional<GLenum> targetOpt)
{
	return BindBufferRangeImpl(index, elemOffsetOpt, elemCountOpt, targetOpt, false);
}

void LuaVBOImpl::DumpDefinition()
{
	if (!vbo) {
		LuaError("[LuaVBOImpl::%s] Buffer definition is invalid. Did you succesfully call :Define()?", __func__);
	}

	std::ostringstream ss;
	ss << fmt::format("Definition information on LuaVBO. OpenGL Buffer ID={}:\n", vbo->GetId());
	for (const auto& kv : bufferAttribDefs) { //guaranteed increasing order of key
		const int attrID = kv.first;
		const auto& baDef = kv.second;
		ss << fmt::format("\tid={} name={} type={} size={} normalized={} pointer={} typeSizeInBytes={} strideSizeInBytes={}\n", attrID, baDef.name, baDef.type, baDef.size, baDef.normalized, baDef.pointer, baDef.typeSizeInBytes, baDef.strideSizeInBytes);
	};
	ss << fmt::format("Count of elements={}\nSize of one element={}\nTotal buffer size={}", elementsCount, elemSizeInBytes, vbo->GetSize());

	LOG("%s", ss.str().c_str());
}

void LuaVBOImpl::SetAttachedToVAO(bool attach)
{
	if (attach)
		++vaoAttachCount;
	else {
		--vaoAttachCount;
		vaoAttachCount = std::max(vaoAttachCount, 0);
	}
}

void LuaVBOImpl::AllocGLBuffer(size_t byteSize)
{
	if (defTarget == GL_UNIFORM_BUFFER && bufferSizeInBytes > BUFFER_SANE_LIMIT_BYTES) {
		LuaError("[LuaVBOImpl::%s] Exceeded [%u] safe UBO buffer size limit of [%u] bytes", __func__, bufferSizeInBytes, LuaVBOImpl::UBO_SAFE_SIZE_BYTES);
	}

	if (bufferSizeInBytes > BUFFER_SANE_LIMIT_BYTES) {
		LuaError("[LuaVBOImpl::%s] Exceeded [%u] sane buffer size limit of [%u] bytes", __func__, bufferSizeInBytes, LuaVBOImpl::BUFFER_SANE_LIMIT_BYTES);
	}

	bufferSizeInBytes = byteSize; //be strict here and don't account for possible increase of size on GPU due to alignment requirements

	vbo = new VBO(defTarget, MapPersistently());
	vbo->Bind();
	vbo->New(byteSize, freqUpdated ? GL_STREAM_DRAW : GL_STATIC_DRAW);
	vbo->Unbind();
}

// Allow for a ~magnitude faster loops than other the map
void LuaVBOImpl::CopyAttrMapToVec()
{
	bufferAttribDefsVec.reserve(bufferAttribDefs.size());
	for (const auto& va : bufferAttribDefs)
		bufferAttribDefsVec.push_back(va);
}

bool LuaVBOImpl::Supported(GLenum target)
{
	return VBO::IsSupported(target);
}

template<typename T>
T LuaVBOImpl::MaybeFunc(const sol::table& tbl, const std::string& key, T defValue) {
	const sol::optional<T> maybeValue = tbl[key];
	return maybeValue.value_or(defValue);
}

template<typename TOut, typename TIter>
bool LuaVBOImpl::TransformAndWrite(int& bytesWritten, GLubyte*& mappedBuf, const int mappedBufferSizeInBytes, const int count, TIter& bdvIter, const bool copyData)
{
	constexpr int outValSize = sizeof(TOut);
	const int outValSizeStride = count * outValSize;

	if (bytesWritten + outValSizeStride > mappedBufferSizeInBytes) {
		LOG_L(L_ERROR, "[LuaVBOImpl::%s] Upload array contains too much data", __func__);
		return false;
	}

	if (copyData) {
		for (int n = 0; n < count; ++n) {
			const auto outVal = TransformFunc<lua_Number, TOut>(*bdvIter);
			memcpy(mappedBuf, &outVal, outValSize);
			mappedBuf += outValSize;
			++bdvIter;
		}
	}
	else {
		mappedBuf += outValSizeStride;
	}

	bytesWritten += outValSizeStride;
	return true;
}

template<typename TIn>
bool LuaVBOImpl::TransformAndRead(int& bytesRead, GLubyte*& mappedBuf, const int mappedBufferSizeInBytes, const int count, std::vector<lua_Number>& vec, const bool copyData)
{
	constexpr int inValSize = sizeof(TIn);
	const int inValSizeStride = count * inValSize;

	if (bytesRead + inValSizeStride > mappedBufferSizeInBytes) {
		LOG_L(L_ERROR, "[LuaVBOImpl::%s] Trying to read beyond the mapped buffer boundaries", __func__);
		return false;
	}

	if (copyData) {
		for (int n = 0; n < count; ++n) {
			TIn inVal; memcpy(&inVal, mappedBuf, inValSize);
			vec.push_back(TransformFunc<TIn, lua_Number>(inVal));

			mappedBuf += inValSize;
		}
	} else {
		mappedBuf += inValSizeStride;
	}

	bytesRead += inValSizeStride;
	return true;
}

template<typename ...Args>
void LuaVBOImpl::LuaError(std::string format, Args ...args)
{
	luaL_error(*reinterpret_cast<sol::this_state*>(L), fmt::sprintf(format, args...).c_str());
}
