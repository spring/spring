#include "LuaVBOImpl.h"

#include <unordered_map>
#include <algorithm>
#include <sstream>

#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

#include "System/Log/ILog.h"
#include "System/SpringMem.h"
#include "System/SafeUtil.h"
#include "Rendering/ModelsDataUploader.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/GlobalUnsynced.h"

#include "LuaUtils.h"

LuaVBOImpl::LuaVBOImpl(const sol::optional<GLenum> defTargetOpt, const sol::optional<bool> freqUpdatedOpt)
	: defTarget{defTargetOpt.value_or(GL_ARRAY_BUFFER)}
	, freqUpdated{freqUpdatedOpt.value_or(false)}

	, attributesCount{ 0u }

	, elementsCount{ 0u }
	, elemSizeInBytes{ 0u }
	, bufferSizeInBytes{ 0u }

	, vbo{ nullptr }
	, vboOwner{ true }
	, bufferData{ nullptr }

	, primitiveRestartIndex{ ~0u }
	, bufferAttribDefsVec{}
	, bufferAttribDefs{}
{ }

LuaVBOImpl::~LuaVBOImpl()
{
	Delete();
}

void LuaVBOImpl::Delete()
{
	//safe to call multiple times
	if (vboOwner)
		spring::SafeDelete(vbo);

	if (bufferData) {
		spring::FreeAlignedMemory(bufferData);
		bufferData = nullptr;
	}

	bufferAttribDefs.clear();
	bufferAttribDefsVec.clear();
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
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid number of vec4 arguments [%d], exceeded maximum of [%u]", __func__, numVec4Attribs, attributesCountMax);
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
			LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid argument object type [%d]. Must be a valid GL type constant", __func__, static_cast<int>(attribDefArgOpt.value().get_type()));
	}

	switch (indexType) {
	case GL_UNSIGNED_BYTE: {
		elemSizeInBytes = sizeof(uint8_t);
		primitiveRestartIndex = 0xff;
	} break;
	case GL_UNSIGNED_SHORT: {
		elemSizeInBytes = sizeof(uint16_t);
		primitiveRestartIndex = 0xffff;
	} break;
	case GL_UNSIGNED_INT: {
		elemSizeInBytes = sizeof(uint32_t);
		primitiveRestartIndex = 0xffffff; //NB: less than (2^32 - 1) due to Lua 2^24 limitation
	} break;

	}

	if (elemSizeInBytes == 0u) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid GL type constant [%d]", __func__, indexType);
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

	attributesCount = 1;

	return true;
}

void LuaVBOImpl::Define(const int elementsCount, const sol::optional<sol::object> attribDefArgOpt)
{
	if (vbo) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Attempt to call %s() multiple times. VBO definition is immutable.", __func__, __func__);
	}

	if (elementsCount <= 0) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Elements count cannot be <= 0", __func__);
	}

	this->elementsCount = elementsCount;

	const auto defineBufferFunc = [this](const sol::object& attribDefArg) {
		if (attribDefArg.get_type() == sol::type::table)
			return FillAttribsTableImpl(attribDefArg.as<sol::table>());

		if (attribDefArg.is<int>())
			return FillAttribsNumberImpl(attribDefArg.as<const int>());

		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid argument object type [%d]. Must be a number or table", __func__);
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
			LuaUtils::SolLuaError("[LuaVBOImpl::%s] Function has to contain non-empty second argument", __func__);
		result = defineBufferFunc(attribDefArgOpt.value());
	} break;
	default:
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid buffer target [%u]", __func__, defTarget);
	}

	if (!result) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Error in definition. See infolog for possible reasons", __func__);
	}

	CopyAttrMapToVec();
	AllocGLBuffer(elemSizeInBytes * elementsCount);
}

std::tuple<uint32_t, uint32_t, uint32_t> LuaVBOImpl::GetBufferSize()
{
	return std::make_tuple(
		elementsCount,
		bufferSizeInBytes,
		static_cast<uint32_t>(vbo != nullptr ? vbo->GetSize() : 0u)
	);
}

size_t LuaVBOImpl::Upload(const sol::stack_table& luaTblData, sol::optional<int> attribIdxOpt, sol::optional<int> elemOffsetOpt, sol::optional<int> luaStartIndexOpt, sol::optional<int> luaFinishIndexOpt)
{
	if (!vbo) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid VBO. Did you call :Define() or :ShapeFromUnitDefID/ShapeFromFeatureDefID()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	if (elemOffset >= elementsCount) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] >= elementsCount [%u]", __func__, elemOffset, elementsCount);
	}

	const int attribIdx = std::max(attribIdxOpt.value_or(-1), -1);
	if (attribIdx != -1 && bufferAttribDefs.find(attribIdx) == bufferAttribDefs.cend()) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] attribIdx is not found in bufferAttribDefs", __func__);
	}

	const uint32_t luaTblDataSize = luaTblData.size();

	const uint32_t luaStartIndex = static_cast<uint32_t>(std::max(luaStartIndexOpt.value_or(1), 1));
	if (luaStartIndex > luaTblDataSize) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid luaStartIndex [%u] exceeds table size [%u]", __func__, luaStartIndex, luaTblDataSize);
	}

	const uint32_t luaFinishIndex = static_cast<uint32_t>(std::max(luaFinishIndexOpt.value_or(luaTblDataSize), 1));
	if (luaFinishIndex > luaTblDataSize) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid luaFinishIndex [%u] exceeds table size [%u]", __func__, luaFinishIndex, luaTblDataSize);
	}

	if (luaStartIndex > luaFinishIndex) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid luaStartIndex [%u] is greater than luaFinishIndex [%u]", __func__, luaStartIndex, luaFinishIndex);
	}

	std::vector<lua_Number> dataVec;
	dataVec.resize(luaFinishIndex - luaStartIndex + 1);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (auto k = 0; k < dataVec.size(); ++k) {
		dataVec[k] = luaTblData.raw_get_or<lua_Number>(luaStartIndex + k, defaultValue);
	}

	return UploadImpl<lua_Number>(dataVec, elemOffset, attribIdx);
}

sol::as_table_t<std::vector<lua_Number>> LuaVBOImpl::Download(sol::optional<int> attribIdxOpt, sol::optional<int> elemOffsetOpt, sol::optional<int> elemCountOpt, sol::optional<bool> forceGPUReadOpt)
{
	std::vector<lua_Number> dataVec;

	if (!vbo) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid VBO. Did you call :Define()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	const uint32_t elemCount = static_cast<uint32_t>(std::clamp(elemCountOpt.value_or(elementsCount), 1, static_cast<int>(elementsCount)));

	if (elemOffset + elemCount > elementsCount) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] + elemCount [%u] >= elementsCount [%u]", __func__, elemOffset, elemCount, elementsCount);
	}

	const int attribIdx = std::max(attribIdxOpt.value_or(-1), -1);
	if (attribIdx != -1 && bufferAttribDefs.find(attribIdx) == bufferAttribDefs.cend()) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] attribIdx is not found in bufferAttribDefs", __func__);
	}

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;

	const bool forceGPURead = forceGPUReadOpt.value_or(false);
	GLubyte* mappedBuf = nullptr;

	const int mappedBufferSizeInBytes = bufferSizeInBytes - bufferOffsetInBytes;
	if (forceGPURead) {
		vbo->Bind();
		mappedBuf = vbo->MapBuffer(bufferOffsetInBytes, mappedBufferSizeInBytes, GL_MAP_READ_BIT);
	}
	else {
		mappedBuf = reinterpret_cast<GLubyte*>(bufferData) + bufferOffsetInBytes;
	}

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
					if (forceGPURead) { \
						vbo->UnmapBuffer(); \
						vbo->Unbind(); \
					} \
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

	if (forceGPURead) {
		vbo->UnmapBuffer();
		vbo->Unbind();
	}
	return sol::as_table(dataVec);
}

/*
	float3 pos;
	float3 normal = UpVector;
	float3 sTangent;
	float3 tTangent;

	// TODO:
	//   with pieceIndex this struct is no longer 64 bytes in size which ATI's prefer
	//   support an arbitrary number of channels, would be easy but overkill (for now)
	float2 texCoords[NUM_MODEL_UVCHANNS];

	uint32_t pieceIndex = 0;
*/
size_t LuaVBOImpl::ModelsVBOImpl()
{
	const auto engineVertAttribDefFunc = [this]() {
		// float3 pos
		this->bufferAttribDefs[0] = {
			GL_FLOAT, //type
			3, //size
			GL_FALSE, //normalized
			"pos", //name
			offsetof(SVertexData, pos), //pointer
			sizeof(float), //typeSizeInBytes
			3 * sizeof(float) //strideSizeInBytes
		};

		// float3 normal
		this->bufferAttribDefs[1] = {
			GL_FLOAT, //type
			3, //size
			GL_FALSE, //normalized
			"normal", //name
			offsetof(SVertexData, normal), //pointer
			sizeof(float), //typeSizeInBytes
			3 * sizeof(float) //strideSizeInBytes
		};

		// float3 sTangent
		this->bufferAttribDefs[2] = {
			GL_FLOAT, //type
			3, //size
			GL_FALSE, //normalized
			"sTangent", //name
			offsetof(SVertexData, sTangent), //pointer
			sizeof(float), //typeSizeInBytes
			3 * sizeof(float) //strideSizeInBytes
		};

		// float3 tTangent
		this->bufferAttribDefs[3] = {
			GL_FLOAT, //type
			3, //size
			GL_FALSE, //normalized
			"tTangent", //name
			offsetof(SVertexData, tTangent), //pointer
			sizeof(float), //typeSizeInBytes
			3 * sizeof(float) //strideSizeInBytes
		};

		// 2 x float2 texCoords, packed as vec4
		this->bufferAttribDefs[4] = {
			GL_FLOAT, //type
			4, //size
			GL_FALSE, //normalized
			"texCoords", //name
			offsetof(SVertexData, texCoords), //pointer
			sizeof(float), //typeSizeInBytes
			4 * sizeof(float) //strideSizeInBytes
		};

		// uint32_t pieceIndex
		this->bufferAttribDefs[5] = {
			GL_UNSIGNED_INT, //type
			1, //size
			GL_FALSE, //normalized
			"pieceIndex", //name
			offsetof(SVertexData, pieceIndex), //pointer
			sizeof(uint32_t), //typeSizeInBytes
			1 * sizeof(uint32_t) //strideSizeInBytes
		};

		this->attributesCount = 6;
		this->elemSizeInBytes = sizeof(SVertexData);
		this->bufferSizeInBytes = vbo->GetSize();
		this->elementsCount = this->bufferSizeInBytes / this->elemSizeInBytes;
	};

	const auto engineIndxAttribDefFunc = [this]() {
		// float3 pos
		this->bufferAttribDefs[0] = {
			GL_UNSIGNED_INT, //type
			1, //size
			GL_FALSE, //normalized
			"index", //name
			0, //pointer
			sizeof(uint32_t), //typeSizeInBytes
			1 * sizeof(uint32_t) //strideSizeInBytes
		};

		this->attributesCount = 1;
		this->elemSizeInBytes = sizeof(uint32_t);
		this->bufferSizeInBytes = vbo->GetSize();
		this->elementsCount = this->bufferSizeInBytes / this->elemSizeInBytes;
		this->primitiveRestartIndex = 0xffffff;
	};

	switch (defTarget) {
	case GL_ARRAY_BUFFER: {
		vbo = S3DModelVAO::GetInstance().GetVertVBO();
		engineVertAttribDefFunc();
	} break;
	case GL_ELEMENT_ARRAY_BUFFER: {
		vbo = S3DModelVAO::GetInstance().GetIndxVBO();
		engineIndxAttribDefFunc();
	} break;
	default:
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid buffer target [%u]", __func__, defTarget);
	}

	CopyAttrMapToVec();

	vboOwner = false;

	return bufferSizeInBytes;
}

void LuaVBOImpl::InstanceBufferCheckAndFormatCheck(int attrID, const char* func)
{
	InstanceBufferCheck(attrID, func);

	const BufferAttribDef& bad = bufferAttribDefs[attrID];
	if (bad.type != GL_UNSIGNED_INT) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Instance VBO attribute %d must have a type of GL_UNSIGNED_INT", func, attrID);
	}
	if (bad.size != 4) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Instance VBO attribute %d must have a size of 4", func, attrID);
	}
}

void LuaVBOImpl::InstanceBufferCheck(int attrID, const char* func)
{
	if (!vbo) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid instance VBO. Did you call :Define() succesfully?", func);
	}
	/*
	if (defTarget != GL_ARRAY_BUFFER) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid instance VBO. Target type (%u) is not GL_ARRAY_BUFFER(%u)", func, defTarget, GL_ARRAY_BUFFER);
	}
	*/
	if (bufferAttribDefs.find(attrID) == bufferAttribDefs.cend()) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] No instance attribute definition %d found", func, attrID);
	}
}

template<typename Iterable>
size_t LuaVBOImpl::MatrixDataFromProjectileIDsImpl(const Iterable& ids, int attrID, sol::optional<int> elemOffsetOpt, const char* func)
{
	const size_t idsSize = ids.size();
	if (idsSize == 0u) //empty Iterable
		return 0u;

	//do basic sanity check
	InstanceBufferCheck(attrID + 0, func);

	//matrix can be represented in several ways:
	// * 4 attributes of GL_FLOAT vec4 (VBO)
	// * 4 sized vector of GL_FLOAT_VEC4 (UBO/SSBO)
	// * 1 field of GL_FLOAT_MAT4 (UBO/SSBO)
	// There're are other ways, but we won't support them

	int strideSize = 0;

	const auto& attr0 = bufferAttribDefs[attrID + 0];

	switch (attr0.type)
	{
	case GL_FLOAT: {
		for (int i = 1; i <= 3; ++i) {
			InstanceBufferCheck(attrID + i, func);

			const auto& attrN = bufferAttribDefs[attrID + 1];

			if (attrN.type != GL_FLOAT)
				LuaUtils::SolLuaError("[LuaVBOImpl::%s] Buffer attribute %d is of GL_FLOAT type, but attribute %d is not, got %u type instead", func, attrID, attrID + i, attrN.type);

			strideSize += attrN.strideSizeInBytes;
		}
	} break;
	case GL_FLOAT_VEC4: [[fallthrough]];
	case GL_FLOAT_MAT4: {
		strideSize += attr0.strideSizeInBytes;
	} break;
	default:
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Buffer attribute %d must have floating type, got (%u) type instead", func, attrID, attr0.type);
		break;
	}

	if (strideSize != 64)
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Attributes starting from (%d), don't define matrix. Size mismatch (%d != 64).", func, attrID, strideSize);

	const uint32_t elemOffset = elemOffsetOpt.value_or(0u);

	if (idsSize > elementsCount - elemOffset)
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Too many elements in Lua table", func);

	static std::vector<float> matDataVec;
	matDataVec.resize(16 * idsSize); //16 floats (matrix) per projectile id

	constexpr auto defaultValue = static_cast<lua_Number>(0);

	size_t idx = 0;
	for (const auto id : ids) {
		const CProjectile* p = LuaUtils::SolIdToObject<CProjectile>(id, __func__);
		const CWeaponProjectile* wp = p->weapon ? static_cast<const CWeaponProjectile*>(p) : nullptr;
		const bool doOffset = wp && wp->GetProjectileType() == WEAPON_MISSILE_PROJECTILE;

		const CMatrix44f trMat = projectileDrawer->CanDrawProjectile(p, /*p->owner()*/ nullptr) ?
			p->GetTransformMatrix(doOffset) :
			CMatrix44f::Zero();

		memcpy(&matDataVec[16 * idx], &trMat, sizeof(CMatrix44f));

		++idx;
	}

	if (attr0.type == GL_FLOAT)
		return UploadImpl<float>(matDataVec, elemOffset, { attrID + 0, attrID + 1, attrID + 2, attrID + 3 });
	else
		return UploadImpl<float>(matDataVec, elemOffset, attrID);
}

template<typename TObj>
SInstanceData LuaVBOImpl::InstanceDataFromGetData(int id, int attrID, uint8_t defTeamID)
{
	uint32_t teamID = defTeamID;

	const TObj* obj = LuaUtils::SolIdToObject<TObj>(id, __func__);
	const uint32_t matOffset = static_cast<uint32_t>(MatrixUploader::GetInstance().GetElemOffset(obj));
	const uint32_t uniIndex  = static_cast<uint32_t>(modelsUniformsStorage.GetObjOffset(obj)); //doesn't need to exist for defs and model. Don't check for validity

	uint8_t drawFlags = 0u;
	if   constexpr (std::is_same_v<TObj, CUnit> || std::is_same_v<TObj, CFeature>) {
		teamID = obj->team;
		drawFlags = obj->drawFlag;
	}

	if (matOffset == ~0u) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid data supplied. See infolog for details", __func__);
	}
	return SInstanceData(matOffset, teamID, drawFlags, uniIndex);
}

template<typename TObj>
size_t LuaVBOImpl::InstanceDataFromImpl(int id, int attrID, uint8_t defTeamID, const sol::optional<int>& elemOffsetOpt)
{
	InstanceBufferCheckAndFormatCheck(attrID, __func__);

	const uint32_t elemOffset = elemOffsetOpt.value_or(0u);
	const SInstanceData instanceData = InstanceDataFromGetData<TObj>(id, attrID, defTeamID);

	if (elemOffset + 1 > elementsCount || elemOffset < 0)
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Element offset (%u) is too big or negative", __func__, elemOffset);

	static std::vector<uint32_t> instanceDataVec(4);

	memcpy(instanceDataVec.data(), &instanceData, sizeof(SInstanceData));

	return UploadImpl<uint32_t>(instanceDataVec, elemOffset, attrID);
}

template<typename TObj>
size_t LuaVBOImpl::InstanceDataFromImpl(const sol::stack_table& ids, int attrID, uint8_t defTeamID, const sol::optional<int>& elemOffsetOpt)
{
	InstanceBufferCheckAndFormatCheck(attrID, __func__);

	std::size_t idsSize = ids.size();

	if (idsSize == 0u) //empty array
		return 0u;

	const uint32_t elemOffset = elemOffsetOpt.value_or(0u);

	if (idsSize > elementsCount - elemOffset)
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Too many elements in Lua table", __func__);

	static std::vector<uint32_t> instanceDataVec;
	instanceDataVec.resize(4 * idsSize);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (std::size_t i = 0u; i < idsSize; ++i) {
		lua_Number idLua = ids.raw_get_or<lua_Number>(i + 1, defaultValue);
		int id = spring::SafeCast<int, lua_Number>(idLua);
		const SInstanceData instanceData = InstanceDataFromGetData<TObj>(id, attrID, defTeamID);
		memcpy(&instanceDataVec[4 * i], &instanceData, sizeof(SInstanceData));
	}

	return UploadImpl<uint32_t>(instanceDataVec, elemOffset, attrID);
}

template<typename TIn, typename AttribTestFunc>
size_t LuaVBOImpl::UploadImpl(const std::vector<TIn>& dataVec, uint32_t elemOffset, AttribTestFunc attribTestFunc)
{
	if (dataVec.empty())
		return 0u;

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;
	const int mappedBufferSizeInBytes = bufferSizeInBytes - bufferOffsetInBytes;

	auto buffDataWithOffset = static_cast<uint8_t*>(bufferData) + bufferOffsetInBytes;

	const auto uploadToGPU = [this, buffDataWithOffset, bufferOffsetInBytes, mappedBufferSizeInBytes](int bytesWritten) -> int {
		vbo->Bind();
#if 1
		vbo->SetBufferSubData(bufferOffsetInBytes, bytesWritten, buffDataWithOffset);
#else
		// very CPU heavy for some reason (NV & Windows)
		auto gpuMappedBuff = vbo->MapBuffer(bufferOffsetInBytes, mappedBufferSizeInBytes, GL_MAP_WRITE_BIT);
		memcpy(gpuMappedBuff, buffDataWithOffset, bytesWritten);
		vbo->UnmapBuffer();
#endif
		vbo->Unbind();

		//LOG("buffDataWithOffset = %p, bufferOffsetInBytes = %u, mappedBufferSizeInBytes = %d, bytesWritten = %d", (void*)buffDataWithOffset, bufferOffsetInBytes, mappedBufferSizeInBytes, bytesWritten);
		return bytesWritten;
	};

	int bytesWritten = 0;

	for (auto bdvIter = dataVec.cbegin(); bdvIter < dataVec.cend();) {
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

			bool copyData = attribTestFunc(attrID);

			#define TRANSFORM_AND_WRITE(T) { \
				if (!TransformAndWrite<TIn, T>(bytesWritten, buffDataWithOffset, mappedBufferSizeInBytes, basicTypeSize, bdvIter, dataVec.cend(), copyData)) { \
					return uploadToGPU(bytesWritten); \
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

	return uploadToGPU(bytesWritten);
}

size_t LuaVBOImpl::ModelsVBO()
{
	if (!S3DModelVAO::IsValid()) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] No ModelsVBO is available. Probably due to no GL4 support", __func__);
		return 0;
	}

	return ModelsVBOImpl();
}


size_t LuaVBOImpl::InstanceDataFromUnitDefIDs(int id, int attrID, sol::optional<int> teamIdOpt, sol::optional<int> elemOffsetOpt)
{
	uint8_t defTeamID = teamIdOpt.value_or(gu->myTeam);
	return InstanceDataFromImpl<UnitDef>(id, attrID, defTeamID, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromUnitDefIDs(const sol::stack_table& ids, int attrID, sol::optional<int> teamIdOpt, sol::optional<int> elemOffsetOpt)
{
	uint8_t defTeamID = teamIdOpt.value_or(gu->myTeam);
	return InstanceDataFromImpl<UnitDef>(ids, attrID, defTeamID, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromFeatureDefIDs(int id, int attrID, sol::optional<int> teamIdOpt, sol::optional<int> elemOffsetOpt)
{
	uint8_t defTeamID = teamIdOpt.value_or(gu->myTeam);
	return InstanceDataFromImpl<FeatureDef>(id, attrID, defTeamID, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromFeatureDefIDs(const sol::stack_table& ids, int attrID, sol::optional<int> teamIdOpt, sol::optional<int> elemOffsetOpt)
{
	uint8_t defTeamID = teamIdOpt.value_or(gu->myTeam);
	return InstanceDataFromImpl<FeatureDef>(ids, attrID, defTeamID, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromUnitIDs(int id, int attrID, sol::optional<int> elemOffsetOpt)
{
	return InstanceDataFromImpl<CUnit>(id, attrID, /*noop*/ 0u, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromUnitIDs(const sol::stack_table& ids, int attrID, sol::optional<int> elemOffsetOpt)
{
	return InstanceDataFromImpl<CUnit>(ids, attrID, /*noop*/ 0u, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromFeatureIDs(int id, int attrID, sol::optional<int> elemOffsetOpt)
{
	return InstanceDataFromImpl<CFeature>(id, attrID, /*noop*/ 0u, elemOffsetOpt);
}

size_t LuaVBOImpl::InstanceDataFromFeatureIDs(const sol::stack_table& ids, int attrID, sol::optional<int> elemOffsetOpt)
{
	return InstanceDataFromImpl<CFeature>(ids, attrID, /*noop*/ 0u, elemOffsetOpt);
}

size_t LuaVBOImpl::MatrixDataFromProjectileIDs(int id, int attrID, sol::optional<int> elemOffsetOpt)
{
	return MatrixDataFromProjectileIDsImpl(std::initializer_list<int>{id}, attrID, elemOffsetOpt, __func__);
}

size_t LuaVBOImpl::MatrixDataFromProjectileIDs(const sol::stack_table& ids, int attrID, sol::optional<int> elemOffsetOpt)
{
	std::size_t idsSize = ids.size();

	static std::vector<int> idsVec;
	idsVec.resize(idsSize);

	constexpr auto defaultValue = static_cast<lua_Number>(0);
	for (std::size_t i = 0u; i < idsSize; ++i) {
		lua_Number idLua = ids.raw_get_or<lua_Number>(i + 1, defaultValue);
		int id = spring::SafeCast<int, lua_Number>(idLua);
		idsVec[i] = id;
	}
	return MatrixDataFromProjectileIDsImpl(idsVec, attrID, elemOffsetOpt, __func__);
}

int LuaVBOImpl::BindBufferRangeImpl(GLuint bindingIndex,  const sol::optional<int> elemOffsetOpt, const sol::optional<int> elemCountOpt, const sol::optional<GLenum> targetOpt, bool bind)
{
	if (!vbo) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Buffer definition is invalid. Did you succesfully call :Define()?", __func__);
	}

	const uint32_t elemOffset = static_cast<uint32_t>(std::max(elemOffsetOpt.value_or(0), 0));
	const uint32_t elemCount = static_cast<uint32_t>(std::clamp(elemCountOpt.value_or(elementsCount), 1, static_cast<int>(elementsCount)));

	if (elemOffset + elemCount > elementsCount) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid elemOffset [%u] + elemCount [%u] > elementsCount [%u]", __func__, elemOffset, elemCount, elementsCount);
	}

	const uint32_t bufferOffsetInBytes = elemOffset * elemSizeInBytes;

	// can't use bufferSizeInBytes here, cause vbo->BindBufferRange expects binding with UBO/SSBO alignment
	// need to use real GPU buffer size, because it's sized with alignment in mind
	const int boundBufferSizeInBytes = /*bufferSizeInBytes*/ vbo->GetSize() - bufferOffsetInBytes;

	GLenum target = targetOpt.value_or(defTarget);
	if (target != GL_UNIFORM_BUFFER && target != GL_SHADER_STORAGE_BUFFER) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] (Un)binding target can only be equal to [%u] or [%u]", __func__, GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER);
	}
	defTarget = target;

	switch (defTarget) {
	case GL_UNIFORM_BUFFER: {
		if (bindingIndex < uboMinIndex || bindingIndex >= globalRendering->glslMaxUniformBufferBindings)
			LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid (Un)binding index [%u]. Index must be within [%u : %d)", __func__, uboMinIndex, globalRendering->glslMaxUniformBufferBindings);
	} break;
	case GL_SHADER_STORAGE_BUFFER: {
		if (bindingIndex < ssboMinIndex || bindingIndex >= globalRendering->glslMaxStorageBufferBindings)
			LuaUtils::SolLuaError("[LuaVBOImpl::%s] Invalid (Un)binding index [%u]. Index must be within [%u : %d)", __func__, ssboMinIndex, globalRendering->glslMaxStorageBufferBindings);
	} break;
	default:
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] (Un)binding target can only be equal to [%u] or [%u]", __func__, GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER);
	}

	bool result = false;
	if (bind) {
		result = vbo->BindBufferRange(defTarget, bindingIndex, bufferOffsetInBytes, boundBufferSizeInBytes);
	} else {
		result = vbo->UnbindBufferRange(defTarget, bindingIndex, bufferOffsetInBytes, boundBufferSizeInBytes);
	}
	if (!result) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Error (un)binding. See infolog for possible reasons", __func__);
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
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Buffer definition is invalid. Did you succesfully call :Define()?", __func__);
	}

	std::ostringstream ss;
	ss << fmt::format("Definition information on LuaVBOs. OpenGL Buffer ID={}:\n", vbo->GetId());
	for (const auto& kv : bufferAttribDefs) { //guaranteed increasing order of key
		const int attrID = kv.first;
		const auto& baDef = kv.second;
		ss << fmt::format("\tid={} name={} type={} size={} normalized={} pointer={} typeSizeInBytes={} strideSizeInBytes={}\n", attrID, baDef.name, baDef.type, baDef.size, baDef.normalized, baDef.pointer, baDef.typeSizeInBytes, baDef.strideSizeInBytes);
	};
	ss << fmt::format("Count of elements={}\nSize of one element={}\nTotal buffer size={}", elementsCount, elemSizeInBytes, vbo->GetSize());

	LOG("%s", ss.str().c_str());
}

void LuaVBOImpl::AllocGLBuffer(size_t byteSize)
{
	if (defTarget == GL_UNIFORM_BUFFER && bufferSizeInBytes > UBO_SAFE_SIZE_BYTES) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Exceeded [%u] safe UBO buffer size limit of [%u] bytes", __func__, bufferSizeInBytes, LuaVBOImpl::UBO_SAFE_SIZE_BYTES);
	}

	if (bufferSizeInBytes > BUFFER_SANE_LIMIT_BYTES) {
		LuaUtils::SolLuaError("[LuaVBOImpl::%s] Exceeded [%u] sane buffer size limit of [%u] bytes", __func__, bufferSizeInBytes, LuaVBOImpl::BUFFER_SANE_LIMIT_BYTES);
	}

	bufferSizeInBytes = static_cast<uint32_t>(byteSize); //be strict here and don't account for possible increase of size on GPU due to alignment requirements

	vbo = new VBO(defTarget, false);
	vbo->Bind();
	//LOG("freqUpdated = %d", freqUpdated);
	vbo->New(byteSize, freqUpdated ? GL_STREAM_DRAW : GL_STATIC_DRAW);
	vbo->Unbind();

	//allocate shadow buffer
	bufferData = spring::AllocateAlignedMemory(bufferSizeInBytes, 32);

	vboOwner = true;
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

template<typename TIn, typename TOut, typename TIter>
bool LuaVBOImpl::TransformAndWrite(int& bytesWritten, GLubyte*& mappedBuf, const int mappedBufferSizeInBytes, const int count, TIter& bdvIter, const TIter& bdvIterEnd, const bool copyData)
{
	constexpr int outValSize = sizeof(TOut);
	const int outValSizeStride = count * outValSize;

	if (bytesWritten + outValSizeStride > mappedBufferSizeInBytes) {
		LOG_L(L_ERROR, "[LuaVBOImpl::%s] Upload array contains too much data", __func__);
		return false;
	}

	if (copyData) {
		for (int n = 0; n < count; ++n) {
			if (bdvIter == bdvIterEnd) {
				LOG_L(L_ERROR, "[LuaVBOImpl::%s] Upload array contains too few data to fill the attribute", __func__);
				return false;
			}

			const auto outVal = spring::SafeCast<TOut, TIn>(*bdvIter);
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
			vec.push_back(spring::SafeCast<lua_Number, TIn>(inVal));

			mappedBuf += inValSize;
		}
	} else {
		mappedBuf += inValSizeStride;
	}

	bytesRead += inValSizeStride;
	return true;
}