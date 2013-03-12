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
#include "System/Vec2.h"

#include <list>
#include <cstring>

CR_BIND(AtlasedTexture, );
CR_REG_METADATA(AtlasedTexture,
		(CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)));

CR_BIND(GroundFXTexture, );
CR_REG_METADATA(GroundFXTexture,
		(CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)));

// texture spacing in the atlas (in pixels)
#define TEXMARGIN 2

bool CTextureAtlas::debug;

CTextureAtlas::CTextureAtlas(int maxxsize, int maxysize)
	: gltex(0)
	, freeTexture(true)
	, xsize(0)
	, ysize(0)
	, maxxsize(maxxsize)
	, maxysize(maxysize)
	, initialized(false)
{
	atlasAllocator = new CLegacyAtlasAlloc();
	//atlasAllocator = new CQuadtreeAtlasAlloc();
	atlasAllocator->SetNonPowerOfTwo(globalRendering->supportNPOTs);
	//atlasAllocator->SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);
}

CTextureAtlas::~CTextureAtlas()
{
	if (freeTexture) {
		glDeleteTextures(1, &gltex);
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

	if (bitmap.type != CBitmap::BitmapTypeStandardRGBA) {
		// only suport RGBA for now
		throw content_error("Unsupported bitmap format in file " + file);
	}

	const int ret = AddTexFromMem(name, bitmap.xsize, bitmap.ysize, RGBA32, bitmap.mem);
	if (ret == 1) {
		files[lcFile] = memtextures.back();
	}
	return ret;
}

bool CTextureAtlas::Finalize()
{
	bool success = atlasAllocator->Allocate();

	CreateTexture();

	for(std::vector<MemTex*>::iterator it = memtextures.begin(); it != memtextures.end(); ++it) {
		delete[] (char*)(*it)->data;
		delete (*it);
	}
	memtextures.clear();

	return success;
}

void CTextureAtlas::CreateTexture()
{
	const int2 atlasSize = atlasAllocator->GetAtlasSize();
	xsize = atlasSize.x;
	ysize = atlasSize.y;

	PBO pbo;
	pbo.Bind();
	pbo.Resize(atlasSize.x * atlasSize.y * 4);

	unsigned char* data = (unsigned char*)pbo.MapBuffer(GL_WRITE_ONLY);
		std::memset(data, 0, atlasSize.x * atlasSize.y * 4); // make spacing between textures black transparent to avoid ugly lines with linear filtering

		for(std::vector<MemTex*>::iterator it = memtextures.begin(); it != memtextures.end(); ++it) {
			float4 texCoords = atlasAllocator->GetTexCoords((*it)->names[0]);
			float4 absCoords = atlasAllocator->GetEntry((*it)->names[0]);
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
			// hack to make sure we don't overwrite our own atlases
			static int count = 0;
			char fname[256];
			SNPRINTF(fname, sizeof(fname), "textureatlas%d.png", ++count);

			CBitmap save(data, atlasSize.x, atlasSize.y);
			save.Save(fname);
			LOG("Saved finalized texture-atlas to '%s'.", fname);
		}
	pbo.UnmapBuffer();

	const int maxMipMaps = atlasAllocator->GetMaxMipMaps();
	glGenTextures(1, &gltex);
		glBindTexture(GL_TEXTURE_2D, gltex);
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
	glBindTexture(GL_TEXTURE_2D, gltex);
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
