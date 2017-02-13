/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DO_TEXTURE_HANDLER_H
#define _3DO_TEXTURE_HANDLER_H

#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"
#include "System/float4.h"
#include "System/UnorderedMap.hpp"

struct TexFile;

class C3DOTextureHandler
{
public:
	typedef float4 UnitTexture;

	C3DOTextureHandler();
	~C3DOTextureHandler();

	// NOTE: safe with unordered_map after all textures have been loaded
	UnitTexture* Get3DOTexture(const std::string& name);

	unsigned int GetAtlasTex1ID() const { return atlas3do1; }
	unsigned int GetAtlasTex2ID() const { return atlas3do2; }
	unsigned int GetAtlasTexSizeX() const { return bigTexX; }
	unsigned int GetAtlasTexSizeY() const { return bigTexY; }

	const spring::unordered_map<std::string, UnitTexture>& GetAtlasTextures() const { return textures; }

private:
	std::vector<TexFile> LoadTexFiles();

	static TexFile CreateTex(const std::string& name, const std::string& name2, bool teamcolor = false);

private:
	spring::unordered_map<std::string, UnitTexture> textures;

	GLuint atlas3do1;
	GLuint atlas3do2;
	int bigTexX;
	int bigTexY;
};

extern C3DOTextureHandler* texturehandler3DO;

#endif /* _3DO_TEXTURE_HANDLER_H */
