/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cctype>
#include <sstream>

#include "3DOTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/IAtlasAllocator.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "TAPalette.h"
#include "System/Exceptions.h"
#include "System/UnorderedSet.hpp"
#include "System/StringUtil.h"
#include "System/type2.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

C3DOTextureHandler textureHandler3DO;

struct TexFile {
	CBitmap tex;  ///< same format as s3o's
	CBitmap tex2; ///< same format as s3o's
	std::string name;
};


void C3DOTextureHandler::Init()
{
	std::vector<TexFile> texFiles = std::move(LoadTexFiles());

	// TODO: make this use TextureAtlas directly
	CTextureAtlas atlas;
	atlas.SetName("3DOModelTextureAtlas");

	IAtlasAllocator* atlasAlloc = atlas.GetAllocator();

	// NOTE: most Intels report maxTextureSize=2048, some even 1024 (!)
	atlasAlloc->SetNonPowerOfTwo(true);
	atlasAlloc->SetMaxSize(std::min(globalRendering->maxTextureSize, 2048), std::min(globalRendering->maxTextureSize, 2048));

	// default for 3DO primitives that point to non-existing textures
	textures["___dummy___"] = UnitTexture(0.0f, 0.0f, 1.0f, 1.0f);

	for (const TexFile& texFile: texFiles) {
		atlasAlloc->AddEntry(texFile.name, int2(texFile.tex.xsize, texFile.tex.ysize));
	}

	const bool allocated = atlasAlloc->Allocate();
	const int2 curAtlasSize = atlasAlloc->GetAtlasSize();
	const int2 maxAtlasSize = atlasAlloc->GetMaxSize();

	if (!allocated) {
		// either the algorithm simply failed to fit all
		// textures or the textures would really not fit
		LOG_L(L_WARNING,
			"[%s] failed to allocate 3DO texture-atlas memory (size=%dx%d max=%dx%d)",
			__func__,
			curAtlasSize.x, curAtlasSize.y,
			maxAtlasSize.x, maxAtlasSize.y
		);

		return;
	}

	bigTexX = curAtlasSize.x;
	bigTexY = curAtlasSize.y;

	assert(curAtlasSize.x <= maxAtlasSize.x);
	assert(curAtlasSize.y <= maxAtlasSize.y);

	std::vector<uint8_t> bigtex1(curAtlasSize.x * curAtlasSize.y * 4, 0);
	std::vector<uint8_t> bigtex2(curAtlasSize.x * curAtlasSize.y * 4, 0);

	for (int a = 0; a < (curAtlasSize.x * curAtlasSize.y); ++a) {
		bigtex1[a*4 + 0] = 128;
		bigtex1[a*4 + 1] = 128;
		bigtex1[a*4 + 2] = 128;
		bigtex1[a*4 + 3] =   0;

		bigtex2[a*4 + 0] =   0;
		bigtex2[a*4 + 1] = 128;
		bigtex2[a*4 + 2] =   0;
		bigtex2[a*4 + 3] = 255;
	}

	for (const TexFile& texFile: texFiles) {
		const CBitmap& curtex1 = texFile.tex;
		const CBitmap& curtex2 = texFile.tex2;

		const size_t foundx = atlasAlloc->GetEntry(texFile.name).x;
		const size_t foundy = atlasAlloc->GetEntry(texFile.name).y;
		const float4 texCoords = atlasAlloc->GetTexCoords(texFile.name);

		const uint8_t* rmem1 = curtex1.GetRawMem();
		const uint8_t* rmem2 = curtex2.GetRawMem();

		for (int y = 0; y < curtex1.ysize; ++y) {
			for (int x = 0; x < curtex1.xsize; ++x) {
				for (int col = 0; col < 4; ++col) {
					bigtex1[(((foundy + y) * curAtlasSize.x + (foundx + x)) * 4) + col] = rmem1[(((y * curtex1.xsize) + x) * 4) + col];
					bigtex2[(((foundy + y) * curAtlasSize.x + (foundx + x)) * 4) + col] = rmem2[(((y * curtex1.xsize) + x) * 4) + col];
				}
			}
		}

		textures[texFile.name] = UnitTexture(texCoords);
	}

	const int maxMipMaps = atlasAlloc->GetMaxMipMaps();

	{
		glGenTextures(1, &atlas3do1);
		glBindTexture(GL_TEXTURE_2D, atlas3do1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);

		if (maxMipMaps > 0) {
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1.data()); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1.data());
		}
	}
	{
		glGenTextures(1, &atlas3do2);
		glBindTexture(GL_TEXTURE_2D, atlas3do2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);

		if (maxMipMaps > 0) {
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2.data()); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2.data());
		}
	}

	if (CTextureAtlas::GetDebug()) {
		CBitmap tex1(bigtex1.data(), curAtlasSize.x, curAtlasSize.y);
		CBitmap tex2(bigtex2.data(), curAtlasSize.x, curAtlasSize.y);

		tex1.Save(atlas.GetName() + "-1-" + IntToString(curAtlasSize.x) + "x" + IntToString(curAtlasSize.y) + ".png");
		tex2.Save(atlas.GetName() + "-2-" + IntToString(curAtlasSize.x) + "x" + IntToString(curAtlasSize.y) + ".png");
	}
}

void C3DOTextureHandler::Kill()
{
	glDeleteTextures(1, &atlas3do1);
	glDeleteTextures(1, &atlas3do2);

	atlas3do1 = 0;
	atlas3do2 = 0;

	textures.clear();
}


std::vector<TexFile> C3DOTextureHandler::LoadTexFiles()
{
	CFileHandler teamTexFile("unittextures/tatex/teamtex.txt");
	CFileHandler paletteFile("unittextures/tatex/palette.pal");

	CSimpleParser parser(teamTexFile);

	spring::unordered_set<std::string> teamTexes;
	spring::unordered_set<string> usedNames;

	while (!parser.Eof()) {
		teamTexes.insert(StringToLower(parser.GetCleanLine()));
	}

	std::vector<TexFile> texFiles;

	const std::vector<std::string>& bmpFiles = CFileHandler::FindFiles("unittextures/tatex/", "*.bmp");
	      std::vector<std::string>  tgaFiles = CFileHandler::FindFiles("unittextures/tatex/", "*.tga");

	tgaFiles.insert(tgaFiles.end(), bmpFiles.begin(), bmpFiles.end());
	texFiles.reserve(tgaFiles.size() + CTAPalette::NUM_PALETTE_ENTRIES);

	for (const std::string& s: tgaFiles) {
		const std::string s2 = StringToLower(FileSystem::GetBasename(s));

		// avoid duplicate names and give tga images priority
		if (usedNames.find(s2) != usedNames.end())
			continue;

		usedNames.insert(s2);

		if (teamTexes.find(s2) == teamTexes.end()) {
			texFiles.push_back(CreateTex(s, s2, false));
		} else {
			texFiles.push_back(CreateTex(s, s2, true));
		}
	}

	palette.Init(paletteFile);

	for (unsigned a = 0; a < CTAPalette::NUM_PALETTE_ENTRIES; ++a) {
		TexFile texFile;
		texFile.name = "ta_color" + IntToString(a, "%i");

		texFile.tex.AllocDummy(SColor{palette[a][0], palette[a][1], palette[a][2], uint8_t(0)}); // A=team-color
		texFile.tex2.AllocDummy(SColor{0, 30, 0, 255}); // R=self-illum, G=reflectivity

		texFiles.push_back(texFile);
	}

	return texFiles;
}


C3DOTextureHandler::UnitTexture* C3DOTextureHandler::Get3DOTexture(const std::string& name)
{
	const auto tti = textures.find(name);

	if (tti != textures.end())
		return &tti->second;

	// unknown texture
	return nullptr;
}

TexFile C3DOTextureHandler::CreateTex(const std::string& name, const std::string& name2, bool teamcolor)
{
	TexFile texFile;
	texFile.tex.Load(name, 30);
	texFile.name = name2;

	texFile.tex2.Alloc(texFile.tex.xsize, texFile.tex.ysize);

	CBitmap* tex1 = &texFile.tex;
	CBitmap* tex2 = &texFile.tex2;

	unsigned char* wmem1 = tex1->GetRawMem();
	unsigned char* wmem2 = tex2->GetRawMem();

	for (int a = 0; a < (tex1->ysize * tex1->xsize); ++a) {
		wmem2[a*4 + 0] = 0;
		wmem2[a*4 + 1] = wmem1[a*4 + 3]; // move reflectivity to texture2
		wmem2[a*4 + 2] = 0;
		wmem2[a*4 + 3] = 255;

		wmem1[a*4 + 3] = 0;

		if (teamcolor) {
			//purple = teamcolor
			if ((wmem1[a*4] == wmem1[a*4 + 2]) && (wmem1[a*4+1] == 0)) {
				const unsigned char lum = wmem1[a*4];
				wmem1[a*4 + 0] = 0;
				wmem1[a*4 + 1] = 0;
				wmem1[a*4 + 2] = 0;
				wmem1[a*4 + 3] = (unsigned char)(std::min(255.0f, lum * 1.5f));
			}
		}
	}

	return texFile;
}

