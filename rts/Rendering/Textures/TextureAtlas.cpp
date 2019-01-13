/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TextureAtlas.h"

#include "Bitmap.h"
#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"

#include <cstring>

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)))

// texture spacing in the atlas (in pixels)
#define TEXMARGIN 2


static AtlasedTexture dummy;

bool CTextureAtlas::debug;


CTextureAtlas::CTextureAtlas(unsigned int allocType)
	: atlasAllocator(nullptr)
	, atlasTexID(0)
	, initialized(false)
	, freeTexture(true)
{
	switch (allocType) {
		case ATLAS_ALLOC_LEGACY: { atlasAllocator = new CLegacyAtlasAlloc(); } break;
		case ATLAS_ALLOC_QUADTREE: { atlasAllocator = new CQuadtreeAtlasAlloc(); } break;
		default: { assert(false); } break;
	}

	atlasAllocator->SetNonPowerOfTwo(true);
	// atlasAllocator->SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);

	textures.reserve(256);
	memTextures.reserve(128);
}

CTextureAtlas::~CTextureAtlas()
{
	if (freeTexture)
		glDeleteTextures(1, &atlasTexID);

	delete atlasAllocator;
}

size_t CTextureAtlas::AddTex(std::string name, int xsize, int ysize, TextureType texType)
{
	memTextures.emplace_back();
	MemTex& tex = memTextures.back();

	tex.xsize = xsize;
	tex.ysize = ysize;
	tex.texType = texType;
	tex.mem.resize((xsize * ysize * GetBPP(texType)) / 8, 0);

	StringToLowerInPlace(name);
	tex.names.emplace_back(std::move(name));

	atlasAllocator->AddEntry(tex.names.back(), int2(xsize, ysize));

	return (memTextures.size() - 1);
}

size_t CTextureAtlas::AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void* data)
{
	const size_t texIdx = AddTex(std::move(name), xsize, ysize, texType);

	MemTex& tex = memTextures[texIdx];

	std::memcpy(tex.mem.data(), data, tex.mem.size());
	return texIdx;
}

size_t CTextureAtlas::AddTexFromFile(std::string name, std::string file)
{
	StringToLowerInPlace(name);

	// if the file is already loaded, use that instead
	const std::string& lcFile = StringToLower(file);
	const auto it = files.find(lcFile);

	if (it != files.end()) {
		memTextures[it->second].names.emplace_back(std::move(name));
		return (it->second);
	}


	CBitmap bitmap;
	if (!bitmap.Load(file)) {
		bitmap.Alloc(2, 2, 4);
		LOG_L(L_WARNING, "[TexAtlas::%s] could not load texture from file \"%s\"", __func__, file.c_str());
	}

	// only suport RGBA for now
	if (bitmap.channels != 4 || bitmap.compressed)
		throw content_error("Unsupported bitmap format in file " + file);

	return (files[lcFile] = AddTexFromMem(std::move(name), bitmap.xsize, bitmap.ysize, RGBA32, bitmap.GetRawMem()));
}


bool CTextureAtlas::Finalize()
{
	const bool success = atlasAllocator->Allocate() && (initialized = CreateTexture());

	memTextures.clear();
	files.clear();
	return success;
}

bool CTextureAtlas::CreateTexture()
{
	const int2 atlasSize = atlasAllocator->GetAtlasSize();
	const int maxMipMaps = atlasAllocator->GetMaxMipMaps();

	// ATI drivers like to *crash* in glTexImage if x=0 or y=0
	if (atlasSize.x <= 0 || atlasSize.y <= 0) {
		LOG_L(L_ERROR, "[TextureAtlas::%s] bad allocation for atlas \"%s\" (size=<%d,%d>)", __func__, name.c_str(), atlasSize.x, atlasSize.y);
		return false;
	}

	PBO pbo;
	pbo.Bind();
	pbo.New(atlasSize.x * atlasSize.y * 4);

	unsigned char* data = reinterpret_cast<unsigned char*>(pbo.MapBuffer(GL_WRITE_ONLY));

	if (data != nullptr) {
		// make spacing between textures black transparent to avoid ugly lines with linear filtering
		std::memset(data, 0, atlasSize.x * atlasSize.y * 4);

		for (const MemTex& memTex: memTextures) {
			const float4 texCoords = atlasAllocator->GetTexCoords(memTex.names[0]);
			const float4 absCoords = atlasAllocator->GetEntry(memTex.names[0]);

			const int xpos = absCoords.x;
			const int ypos = absCoords.y;

			const AtlasedTexture tex(texCoords);

			for (const auto& name: memTex.names) {
				textures[name] = tex;
			}

			for (int y = 0; y < memTex.ysize; ++y) {
				int* dst = ((int*)           data  ) + xpos + (ypos + y) * atlasSize.x;
				int* src = ((int*)memTex.mem.data()) +        (       y) * memTex.xsize;

				memcpy(dst, src, memTex.xsize * 4);
			}
		}

		if (debug) {
			CBitmap tex(data, atlasSize.x, atlasSize.y);
			tex.Save(name + "-" + IntToString(atlasSize.x) + "x" + IntToString(atlasSize.y) + ".png");
		}
	} else {
		LOG_L(L_ERROR, "[TextureAtlas::%s] failed to map PBO for atlas \"%s\" (size=<%d,%d>)", __func__, name.c_str(), atlasSize.x, atlasSize.y);
	}

	pbo.UnmapBuffer();


	glGenTextures(1, &atlasTexID);
	glBindTexture(GL_TEXTURE_2D, atlasTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);
	if (maxMipMaps > 0) {
		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, atlasSize.x, atlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr()); //FIXME disable texcompression, PBO
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasSize.x, atlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr());
	}

	pbo.Invalidate();
	pbo.Unbind();
	pbo.Release();

	return (data != nullptr);
}


void CTextureAtlas::BindTexture()
{
	glBindTexture(GL_TEXTURE_2D, atlasTexID);
}

bool CTextureAtlas::TextureExists(const std::string& name)
{
	return (textures.find(StringToLower(name)) != textures.end());
}


AtlasedTexture& CTextureAtlas::GetTexture(const std::string& name)
{
	if (TextureExists(name))
		return textures[StringToLower(name)];

	return dummy;
}


AtlasedTexture& CTextureAtlas::GetTextureWithBackup(const std::string& name, const std::string& backupName)
{
	if (TextureExists(name))
		return textures[StringToLower(name)];

	if (TextureExists(backupName))
		return textures[StringToLower(backupName)];

	return dummy;
}

int2 CTextureAtlas::GetSize() const {
	return (atlasAllocator->GetAtlasSize());
}

