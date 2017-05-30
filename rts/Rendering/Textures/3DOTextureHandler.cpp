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

C3DOTextureHandler* texturehandler3DO = NULL;

struct TexFile {
	CBitmap tex;  ///< same format as s3o's
	CBitmap tex2; ///< same format as s3o's
	std::string name;
};


C3DOTextureHandler::C3DOTextureHandler()
{
	std::vector<TexFile> texfiles = std::move(LoadTexFiles());

	// TODO: make this use TextureAtlas directly
	CTextureAtlas atlas;
	atlas.SetName("3DOModelTextureAtlas");

	IAtlasAllocator* atlasAlloc = atlas.GetAllocator();

	// NOTE: most Intels report maxTextureSize=2048, some even 1024 (!)
	atlasAlloc->SetNonPowerOfTwo(globalRendering->supportNPOTs);
	atlasAlloc->SetMaxSize(std::min(globalRendering->maxTextureSize, 2048), std::min(globalRendering->maxTextureSize, 2048));

	// default for 3DO primitives that point to non-existing textures
	textures["___dummy___"] = UnitTexture(0.0f, 0.0f, 1.0f, 1.0f);

	for (auto it = texfiles.begin(); it != texfiles.end(); ++it) {
		atlasAlloc->AddEntry(it->name, int2(it->tex.xsize, it->tex.ysize));
	}

	const bool allocated = atlasAlloc->Allocate();
	const int2 curAtlasSize = atlasAlloc->GetAtlasSize();
	const int2 maxAtlasSize = atlasAlloc->GetMaxSize();

	if (!allocated) {
		// either the algorithm simply failed to fit all
		// textures or the textures would really not fit
		LOG_L(L_WARNING,
			"[%s] failed to allocate 3DO texture-atlas memory (size=%dx%d max=%dx%d)",
			__FUNCTION__,
			curAtlasSize.x, curAtlasSize.y,
			maxAtlasSize.x, maxAtlasSize.y
		);
		return;
	} else {
		assert(curAtlasSize.x <= maxAtlasSize.x);
		assert(curAtlasSize.y <= maxAtlasSize.y);
	}

	unsigned char* bigtex1 = new unsigned char[curAtlasSize.x * curAtlasSize.y * 4];
	unsigned char* bigtex2 = new unsigned char[curAtlasSize.x * curAtlasSize.y * 4];

	for (int a = 0; a < (curAtlasSize.x * curAtlasSize.y); ++a) {
		bigtex1[a*4 + 0] = 128;
		bigtex1[a*4 + 1] = 128;
		bigtex1[a*4 + 2] = 128;
		bigtex1[a*4 + 3] = 0;

		bigtex2[a*4 + 0] = 0;
		bigtex2[a*4 + 1] = 128;
		bigtex2[a*4 + 2] = 0;
		bigtex2[a*4 + 3] = 255;
	}

	for (auto it = texfiles.begin(); it != texfiles.end(); ++it) {
		CBitmap& curtex1 = it->tex;
		CBitmap& curtex2 = it->tex2;

		const size_t foundx = atlasAlloc->GetEntry(it->name).x;
		const size_t foundy = atlasAlloc->GetEntry(it->name).y;
		const float4 texCoords = atlasAlloc->GetTexCoords(it->name);

		for (int y = 0; y < curtex1.ysize; ++y) {
			for (int x = 0; x < curtex1.xsize; ++x) {
				for (int col = 0; col < 4; ++col) {
					bigtex1[(((foundy + y) * curAtlasSize.x + (foundx + x)) * 4) + col] = curtex1.mem[(((y * curtex1.xsize) + x) * 4) + col];
					bigtex2[(((foundy + y) * curAtlasSize.x + (foundx + x)) * 4) + col] = curtex2.mem[(((y * curtex1.xsize) + x) * 4) + col];
				}
			}
		}

		textures[it->name] = UnitTexture(texCoords);
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
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1);
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
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, curAtlasSize.x, curAtlasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2);
		}
	}

	if (CTextureAtlas::GetDebug()) {
		CBitmap tex1(bigtex1, curAtlasSize.x, curAtlasSize.y);
		CBitmap tex2(bigtex2, curAtlasSize.x, curAtlasSize.y);

		tex1.Save(atlas.GetName() + "-1-" + IntToString(curAtlasSize.x) + "x" + IntToString(curAtlasSize.y) + ".png");
		tex2.Save(atlas.GetName() + "-2-" + IntToString(curAtlasSize.x) + "x" + IntToString(curAtlasSize.y) + ".png");
	}

	delete[] bigtex1;
	delete[] bigtex2;
}

C3DOTextureHandler::~C3DOTextureHandler()
{
	glDeleteTextures(1, &atlas3do1);
	glDeleteTextures(1, &atlas3do2);
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

	const std::vector<std::string>& filesBMP = CFileHandler::FindFiles("unittextures/tatex/", "*.bmp");
	      std::vector<std::string>  files    = CFileHandler::FindFiles("unittextures/tatex/", "*.tga");

	files.insert(files.end(), filesBMP.begin(), filesBMP.end());
	texFiles.reserve(files.size() + CTAPalette::NUM_PALETTE_ENTRIES);

	for (auto fi = files.begin(); fi != files.end(); ++fi) {
		const std::string& s = *fi;
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

	if (paletteFile.FileExists())
		palette.Init(paletteFile);

	for (unsigned a = 0; a < CTAPalette::NUM_PALETTE_ENTRIES; ++a) {
		TexFile texFile;
		texFile.name = "ta_color" + IntToString(a, "%i");
		texFile.tex.Alloc(1, 1);
		texFile.tex.mem[0] = palette[a][0];
		texFile.tex.mem[1] = palette[a][1];
		texFile.tex.mem[2] = palette[a][2];
		texFile.tex.mem[3] = 0; // teamcolor

		texFile.tex2.Alloc(1, 1);
		texFile.tex2.mem[0] = 0;  // self illum
		texFile.tex2.mem[1] = 30; // reflectivity
		texFile.tex2.mem[2] =  0;
		texFile.tex2.mem[3] = 255;

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

	for (int a = 0; a < (tex1->ysize * tex1->xsize); ++a) {
		tex2->mem[a*4 + 0] = 0;
		tex2->mem[a*4 + 1] = tex1->mem[a*4 + 3]; // move reflectivity to texture2
		tex2->mem[a*4 + 2] = 0;
		tex2->mem[a*4 + 3] = 255;

		tex1->mem[a*4 + 3] = 0;

		if (teamcolor) {
			//purple = teamcolor
			if ((tex1->mem[a*4] == tex1->mem[a*4 + 2]) && (tex1->mem[a*4+1] == 0)) {
				unsigned char lum = tex1->mem[a*4];
				tex1->mem[a*4 + 0] = 0;
				tex1->mem[a*4 + 1] = 0;
				tex1->mem[a*4 + 2] = 0;
				tex1->mem[a*4 + 3] = (unsigned char)(std::min(255.0f, lum * 1.5f));
			}
		}
	}

	return texFile;
}

