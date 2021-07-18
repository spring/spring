/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <algorithm>

#include "LuaTextures.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "LuaAtlasTextures.h"

void LuaAtlasTextures::Clear()
{
	textureAtlasVec.clear();
	textureAtlasMap.clear();
}

std::string LuaAtlasTextures::Create(const int xsize, const int ysize, const int allocatorType)
{
	std::string idStr = prefix + std::to_string(textureAtlasVec.size());

	textureAtlasVec.emplace_back(CTextureAtlas(allocatorType, xsize, ysize, idStr));
	textureAtlasMap[idStr] = textureAtlasVec.size() - 1;

	return idStr;
}

bool LuaAtlasTextures::Delete(const std::string& idStr)
{
	std::size_t index = GetAtlasIndexById(idStr);
	if (index == invalidIndex)
		return false;

	if (index != textureAtlasVec.size() - 1) {
		textureAtlasMap[textureAtlasVec.back().GetName()] = index;
		std::swap(textureAtlasVec[index], textureAtlasVec.back());
	}

	textureAtlasVec.pop_back();
	textureAtlasMap.erase(idStr);

	return true;
}

CTextureAtlas* LuaAtlasTextures::GetAtlasById(const std::string& idStr) const
{
	return GetAtlasByIndex(GetAtlasIndexById(idStr));
}

CTextureAtlas* LuaAtlasTextures::GetAtlasByIndex(const size_t index) const
{
	if (index == invalidIndex)
		return nullptr;

	return const_cast<CTextureAtlas*>(&textureAtlasVec[index]);
}

size_t LuaAtlasTextures::GetAtlasIndexById(const std::string& idStr) const
{
	auto it = textureAtlasMap.find(idStr);
	if (it == textureAtlasMap.end())
		return invalidIndex;

	return it->second;
}