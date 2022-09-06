/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TextureAtlas.h"

#include "Bitmap.h"
#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "RowAtlasAlloc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"

#include <cstring>

CONFIG(int, MaxTextureAtlasSizeX).defaultValue(2048).minimumValue(512).maximumValue(32768);
CONFIG(int, MaxTextureAtlasSizeY).defaultValue(2048).minimumValue(512).maximumValue(32768);

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture, (CR_IGNORED(x), CR_IGNORED(y), CR_IGNORED(z), CR_IGNORED(w)))

CTextureAtlas::CTextureAtlas(uint32_t allocType_, int32_t atlasSizeX_, int32_t atlasSizeY_, const std::string& name_, bool reloadable_)
	: name{ name_ }
	, allocType{ allocType_ }
	, atlasSizeX{ atlasSizeX_ }
	, atlasSizeY{ atlasSizeY_ }
	, reloadable{ reloadable_ }
{

	textures.reserve(256);
	memTextures.reserve(128);
	ReinitAllocator();
}

CTextureAtlas::~CTextureAtlas()
{
	if (freeTexture) {
		glDeleteTextures(1, &atlasTexID);
		atlasTexID = 0u;
	}

	memTextures.clear();
	files.clear();

	spring::SafeDelete(atlasAllocator);
}

void CTextureAtlas::ReinitAllocator()
{
	spring::SafeDelete(atlasAllocator);

	switch (allocType) {
		case ATLAS_ALLOC_LEGACY:   { atlasAllocator = new   CLegacyAtlasAlloc(); } break;
		case ATLAS_ALLOC_QUADTREE: { atlasAllocator = new CQuadtreeAtlasAlloc(); } break;
		case ATLAS_ALLOC_ROW:      { atlasAllocator = new      CRowAtlasAlloc(); } break;
		default:                   {                              assert(false); } break;
	}

	// NB: maxTextureSize can be as large as 32768, resulting in a 4GB atlas
	atlasSizeX = std::min(globalRendering->maxTextureSize, (atlasSizeX > 0) ? atlasSizeX : configHandler->GetInt("MaxTextureAtlasSizeX"));
	atlasSizeY = std::min(globalRendering->maxTextureSize, (atlasSizeY > 0) ? atlasSizeY : configHandler->GetInt("MaxTextureAtlasSizeY"));

	atlasAllocator->SetNonPowerOfTwo(globalRendering->supportNonPowerOfTwoTex);
	atlasAllocator->SetMaxSize(atlasSizeX, atlasSizeY);
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
	if (initialized && !reloadable)
		return true;

	const bool success = atlasAllocator->Allocate() && (initialized = CreateTexture());

	if (!reloadable) {
		memTextures.clear();
		files.clear();
	}

	return success;
}

const uint32_t CTextureAtlas::GetTexTarget() const
{
	return GL_TEXTURE_2D; // just constant for now
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

			AtlasedTexture tex(texCoords);

			for (const auto& name: memTex.names) {
				textures[name] = std::move(tex); //make sure textures[name] gets only its guts replaced, so all pointers remain valid
			}

			for (int y = 0; y < memTex.ysize; ++y) {
				int* dst = ((int*)           data  ) + xpos + (ypos + y) * atlasSize.x;
				int* src = ((int*)memTex.mem.data()) +        (       y) * memTex.xsize;

				memcpy(dst, src, memTex.xsize * 4);
			}
		}

		if (debug) {
			CBitmap tex(data, atlasSize.x, atlasSize.y);
			tex.Save(name + "-" + IntToString(atlasSize.x) + "x" + IntToString(atlasSize.y) + ".png", true);
		}
	} else {
		LOG_L(L_ERROR, "[TextureAtlas::%s] failed to map PBO for atlas \"%s\" (size=<%d,%d>)", __func__, name.c_str(), atlasSize.x, atlasSize.y);
	}

	pbo.UnmapBuffer();

	if (atlasTexID == 0u) //make function re=entrant
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

const spring::unordered_map<std::string, IAtlasAllocator::SAtlasEntry>& CTextureAtlas::GetTextures() const
{
	return atlasAllocator->GetEntries();
}

void CTextureAtlas::ReloadTextures()
{
	if (!reloadable) {
		LOG_L(L_ERROR, "[CTextureAtlas::%s] Attempring to reload non-reloadable texture atlas name=\"%s\"", __func__, name.c_str());
		return;
	}

	ReinitAllocator();

	for (const auto& [filename, idx] : files) {
		CBitmap bitmap;

		if (!bitmap.Load(filename))
			continue;

		assert(idx < memTextures.size());
		auto& memTex = memTextures[idx];

		memTex.xsize = bitmap.xsize;
		memTex.ysize = bitmap.ysize;
		memTex.texType = RGBA32;
		memTex.mem.resize((memTex.xsize * memTex.ysize * GetBPP(memTex.texType)) / 8, 0);
		std::memcpy(memTex.mem.data(), bitmap.GetRawMem(), memTex.mem.size());

		for (const auto& name : memTex.names) {
			atlasAllocator->AddEntry(name, int2(memTex.xsize, memTex.ysize));
		}
	}

	Finalize();
}

void CTextureAtlas::DumpTexture(const char* newFileName) const
{
	std::string filename = newFileName ? newFileName : name.c_str();
	filename += ".png";

	glSaveTexture(atlasTexID, filename.c_str());
}


AtlasedTexture& CTextureAtlas::GetTexture(const std::string& name)
{
	if (TextureExists(name))
		return textures[StringToLower(name)];

	return CTextureAtlas::dummy;
}


AtlasedTexture& CTextureAtlas::GetTextureWithBackup(const std::string& name, const std::string& backupName)
{
	if (TextureExists(name))
		return textures[StringToLower(name)];

	if (TextureExists(backupName))
		return textures[StringToLower(backupName)];

	return CTextureAtlas::dummy;
}

std::string CTextureAtlas::GetTextureName(AtlasedTexture* tex)
{
	if (texToName.empty()) {
		for (auto& kv : textures)
			texToName[&kv.second] = kv.first;
	}
	const auto it = texToName.find(tex);
	return (it != texToName.end()) ? it->second : "";
}

int2 CTextureAtlas::GetSize() const {
	return (atlasAllocator->GetAtlasSize());
}

