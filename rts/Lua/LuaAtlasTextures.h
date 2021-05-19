/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_ATLAS_TEXTURES_H
#define LUA_ATLAS_TEXTURES_H

#include <string>
//#include <unordered_map>

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/UnorderedMap.hpp"
#include "System/SafeUtil.h"

struct AtlasedTexture;

class LuaAtlasTextures {
public:
	static constexpr char prefix = '*';
	static constexpr size_t invalidIndex = size_t(-1);

	~LuaAtlasTextures() { Clear(); }
	LuaAtlasTextures() {
		textureAtlasMap.reserve(32);
	}

	void Clear();

	std::string Create(const int xsize, const int ysize, const int allocatorType = 0);
	bool Delete(const std::string& idStr);
	CTextureAtlas* GetAtlasById(const std::string& idStr) const;
	CTextureAtlas* GetAtlasByIndex(const size_t index) const;
	size_t GetAtlasIndexById(const std::string& idStr) const;
private:
	using TextureAtlasMap = spring::unsynced_map<std::string, std::size_t>;
	using TextureAtlasVec = std::vector<CTextureAtlas>;
private:
	TextureAtlasMap textureAtlasMap;
	TextureAtlasVec textureAtlasVec;
};

#endif /* LUA_ATLAS_TEXTURES_H */
