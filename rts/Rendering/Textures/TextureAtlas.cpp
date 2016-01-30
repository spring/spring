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
#include "System/Util.h"
#include "System/Exceptions.h"

#include <list>
#include <cstring>

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture,
		(CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)))

// texture spacing in the atlas (in pixels)
#define TEXMARGIN 2

bool CTextureAtlas::debug;

CTextureAtlas::CTextureAtlas(unsigned int allocType)
	: atlasAllocator(NULL)
	, atlasTexID(0)
	, initialized(false)
	, freeTexture(true)
{
	switch (allocType) {
		case ATLAS_ALLOC_LEGACY: { atlasAllocator = new CLegacyAtlasAlloc(); } break;
		case ATLAS_ALLOC_QUADTREE: { atlasAllocator = new CQuadtreeAtlasAlloc(); } break;
		default: { assert(false); } break;
	}

	atlasAllocator->SetNonPowerOfTwo(globalRendering->supportNPOTs);
	// atlasAllocator->SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);
}

CTextureAtlas::~CTextureAtlas()
{
	if (freeTexture) {
		glDeleteTextures(1, &atlasTexID);
	}
	delete atlasAllocator;
}

void* CTextureAtlas::AddTex(std::string name, int xsize, int ysize, TextureType texType)
{
	const int gpp = GetBPP(texType);
	const size_t data_size = xsize * ysize * gpp / 8;
	MemTex* memtex = new MemTex;
	memtex->xsize = xsize;
	memtex->ysize = ysize;
	memtex->texType = texType;
	memtex->data = new char[data_size];
	StringToLowerInPlace(name);
	memtex->names.push_back(name);
	memtextures.push_back(memtex);

	atlasAllocator->AddEntry(name, int2(xsize, ysize));

	return memtex->data;
}

int CTextureAtlas::AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void* data)
{
	void* memdata = AddTex(name, xsize, ysize, texType);
	const int gpp = GetBPP(texType);
	const size_t data_size = xsize * ysize * gpp / 8;
	std::memcpy(memdata, data, data_size);
	return 1;
}

int CTextureAtlas::AddTexFromFile(std::string name, std::string file)
{
	StringToLowerInPlace(name);

	// if the file is already loaded, use that instead
	std::string lcFile = StringToLower(file);
	std::map<std::string, MemTex*>::iterator it = files.find(lcFile);
	if (it != files.end()) {
		MemTex* memtex = it->second;
		memtex->names.push_back(name);
		return 1;
	}


	CBitmap bitmap;
	if (!bitmap.Load(file)) {
		throw content_error("Could not load texture from file " + file);
	}

	if (bitmap.channels != 4 || bitmap.compressed) {
		// only suport RGBA for now
		throw content_error("Unsupported bitmap format in file " + file);
	}

	const int ret = AddTexFromMem(name, bitmap.xsize, bitmap.ysize, RGBA32, &bitmap.mem[0]);
	if (ret == 1) {
		files[lcFile] = memtextures.back();
	}
	return ret;
}

bool CTextureAtlas::Finalize()
{
	const bool success = atlasAllocator->Allocate();

	if (success)
		CreateTexture();

	for (std::vector<MemTex*>::iterator it = memtextures.begin(); it != memtextures.end(); ++it) {
		delete[] (char*)(*it)->data;
		delete (*it);
	}
	memtextures.clear();

	return success;
}

void CTextureAtlas::CreateTexture()
{
	const int2 atlasSize = atlasAllocator->GetAtlasSize();

	PBO pbo;
	pbo.Bind();
	pbo.New(atlasSize.x * atlasSize.y * 4);

	unsigned char* data = (unsigned char*)pbo.MapBuffer(GL_WRITE_ONLY);

	{
		// make spacing between textures black transparent to avoid ugly lines with linear filtering
		std::memset(data, 0, atlasSize.x * atlasSize.y * 4);

		for (std::vector<MemTex*>::iterator it = memtextures.begin(); it != memtextures.end(); ++it) {
			const float4 texCoords = atlasAllocator->GetTexCoords((*it)->names[0]);
			const float4 absCoords = atlasAllocator->GetEntry((*it)->names[0]);
			const int xpos = absCoords.x;
			const int ypos = absCoords.y;

			AtlasedTexture tex(texCoords);
			for (size_t n = 0; n < (*it)->names.size(); ++n) {
				textures[(*it)->names[n]] = tex;
			}

			for (int y = 0; y < (*it)->ysize; ++y) {
				int* dst = ((int*)data) + xpos + (ypos + y) * atlasSize.x;
				int* src = ((int*)(*it)->data) + y * (*it)->xsize;
				memcpy(dst, src, (*it)->xsize * 4);
			}
		}

		if (debug) {
			CBitmap tex(data, atlasSize.x, atlasSize.y);
			tex.Save(name + "-" + IntToString(atlasSize.x) + "x" + IntToString(atlasSize.y) + ".png");
		}
	}

	pbo.UnmapBuffer();

	const int maxMipMaps = atlasAllocator->GetMaxMipMaps();
	glGenTextures(1, &atlasTexID);
		glBindTexture(GL_TEXTURE_2D, atlasTexID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);
		if (maxMipMaps > 0) {
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, atlasSize.x, atlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr()); //FIXME disable texcompression //FIXME 2 PBO!!!!
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasSize.x, atlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr());
		}

	pbo.Invalidate();
	pbo.Unbind();

	initialized = true;
}

int CTextureAtlas::GetBPP(TextureType texType)
{
	switch(texType) {
		case RGBA32:
			return 32;
		default:
			return 32;
	}
}

void CTextureAtlas::BindTexture()
{
	glBindTexture(GL_TEXTURE_2D, atlasTexID);
}

bool CTextureAtlas::TextureExists(const std::string& name)
{
	return textures.find(StringToLower(name)) != textures.end();
}


AtlasedTexture& CTextureAtlas::GetTexture(const std::string& name)
{
	return textures[StringToLower(name)];
}


AtlasedTexture& CTextureAtlas::GetTextureWithBackup(const std::string& name, const std::string& backupName)
{
	if (textures.find(StringToLower(name)) != textures.end()) {
		return textures[StringToLower(name)];
	} else {
		return textures[StringToLower(backupName)];
	}
}

int2 CTextureAtlas::GetSize() const {
	return (atlasAllocator->GetAtlasSize());
}

