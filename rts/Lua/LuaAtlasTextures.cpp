/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <sstream>

#include "LuaTextures.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/Log/ILog.h"
#include "LuaAtlasTextures.h"

void LuaAtlasTextures::Clear()
{
	for (auto& kv : textureAtlasMap)
		spring::SafeDelete(kv.second);

	textureAtlasMap.clear();
}

std::string LuaAtlasTextures::Create(const std::string& name, const int xsize, const int ysize, const int allocatorType)
{
	CTextureAtlas* atlas = new CTextureAtlas(allocatorType, xsize, ysize);
	atlas->SetName(name);

	textureAtlasMap[lastIndex] = atlas;

	std::ostringstream ss;
	ss << prefix << lastIndex;

	++lastIndex;

	return ss.str();
}

bool LuaAtlasTextures::Delete(const std::string& idStr)
{
	spring::unsynced_map<size_t, CTextureAtlas*>::const_iterator iter;
	if (GetIterator(idStr, iter))
		return textureAtlasMap.erase(iter->first);

	return false;
}

CTextureAtlas* LuaAtlasTextures::GetAtlasById(const std::string& idStr) const
{
	spring::unsynced_map<size_t, CTextureAtlas*>::const_iterator iter;
	if (GetIterator(idStr, iter))
		return iter->second;

	return nullptr;
}

CTextureAtlas* LuaAtlasTextures::GetAtlasByIndex(const size_t index) const
{
	const auto iter = textureAtlasMap.find(index);
	if (iter == textureAtlasMap.end())
		return nullptr;

	return iter->second;
}

size_t LuaAtlasTextures::GetAtlasIndexById(const std::string& idStr) const
{
	spring::unsynced_map<size_t, CTextureAtlas*>::const_iterator iter;
	if (GetIterator(idStr, iter))
		return iter->first;

	return size_t(-1);
}

bool LuaAtlasTextures::GetIterator(const std::string& idStr, spring::unsynced_map<size_t, CTextureAtlas*>::iterator& iter)
{
	if (idStr[0] != prefix)
		return false;

	try {
		int numID = std::stoi(idStr.substr(1));
		iter = textureAtlasMap.find(numID);
		if (iter == textureAtlasMap.end())
			return false;

		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

bool LuaAtlasTextures::GetIterator(const std::string& idStr, spring::unsynced_map<size_t, CTextureAtlas*>::const_iterator& iter) const
{
	if (idStr[0] != prefix)
		return false;

	try {
		int numID = std::stoi(idStr.substr(1));
		iter = textureAtlasMap.find(numID);
		if (iter == textureAtlasMap.cend())
			return false;

		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}