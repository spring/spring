#pragma once

#include <cstdint>
#include <array>
#include <unordered_map>
#include <memory>
#include <vector>

//#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/TexturesSet.h"

enum RenderPassType : uint8_t {
	RPT_OPAQUE_DEF           = 0, //deferred
	RPT_OPAQUE_DEF_UNDRCON   = 1, //deferred
	RPT_OPAQUE_FWD           = 2, //forward
	RPT_OPAQUE_FWD_UNDRCON   = 3, //under construction effect
	RPT_OPAQU_REFLECT        = 4,
	RPT_OPAQU_REFRACT        = 5,
	RPT_ALPHA                = 6,
	RPT_ALPHA_REFLECT        = 7,
	RPT_ALPHA_REFRACT        = 8,
	RPT_SHADOW               = 9,
	RPT_TYPE_COUNT           = 10
};

class RenderPassData {
public:
	RenderPassData() = default;
	bool operator==(const RenderPassData& rhs) const { return (textures.GetHash() == rhs.textures.GetHash()) && shaderID == rhs.shaderID; }
	bool operator!=(const RenderPassData& rhs) const { return !(*this == rhs); }

	std::size_t GetHash() const {
		if (hash == 0)
			hash = std::size_t((uint64_t)textures.GetHash() << 32 | shaderID);

		return hash;
	}

	uint32_t shaderID = 0u;
	TexturesSet textures;
private:
	mutable size_t hash = 0u;
};

class RenderPassDataHash {
public:
	size_t operator()(const RenderPassData& arg) const { return arg.GetHash(); }
};

class CustomMaterial {
public:
	bool HasRenderPass(RenderPassType rpt) const { return renderPassesData[rpt].shaderID > 0; }
	void AddRenderPass(RenderPassType rpt, const RenderPassData& rpd) {	renderPassesData[rpt] = rpd; }
	void DelRenderPass(RenderPassType rpt) { renderPassesData[rpt] = {}; }

	bool operator==(const CustomMaterial& rhs) const;
	bool operator!=(const CustomMaterial& rhs) const { return !(*this == rhs); }

	std::size_t GetHash() const;
private:
	mutable size_t hash = 0u;
	std::array<RenderPassData, RenderPassType::RPT_TYPE_COUNT> renderPassesData;
};

class CustomMaterialHash {
public:
	size_t operator()(const CustomMaterial& arg) const { return arg.GetHash(); }
};

using SharedCustomMaterial = std::shared_ptr<CustomMaterial>;

class CSolidObject;
struct SolidObjectDef;

using CSolidObjectPieces = std::pair<CSolidObject*, std::vector<bool>>;
using CSolidObjectDefPieces = std::pair<SolidObjectDef*, std::vector<bool>>;
// add hashes, transform to class?

class CustomMaterialObjects {
public:
	void AddCustomMaterial(const SolidObjectDef* def, const CustomMaterial& mat);
	bool HasCustomMaterial(const CSolidObject* obj, RenderPassType rpt) const;
private:
	std::unordered_map<const CSolidObject*, SharedCustomMaterial> objects;
	//std::unordered_map<CSolidObjectPieces, SharedCustomMaterial> objectPieces;

	std::unordered_map<const SolidObjectDef*, SharedCustomMaterial> objectDefs;
	//std::unordered_map<CSolidObjectDefPieces, SharedCustomMaterial> objectDefs;

	std::vector<SharedCustomMaterial> customMaterials;
};