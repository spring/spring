/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cctype>
#include <set>
#include <sstream>

#include "3DOTextureHandler.h"
#include "LegacyAtlasAlloc.h"
#include "QuadtreeAtlasAlloc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "TAPalette.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Vec2.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/SimpleParser.h"


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
	std::vector<TexFile*> texfiles = LoadTexFiles();

	CLegacyAtlasAlloc atlas;
	//CQuadtreeAtlasAlloc atlas;
	atlas.SetNonPowerOfTwo(globalRendering->supportNPOTs);
	atlas.SetMaxSize(2048, 2048);

	for (std::vector<TexFile*>::iterator it = texfiles.begin(); it != texfiles.end(); ++it) {
		atlas.AddEntry((*it)->name, int2((*it)->tex.xsize, (*it)->tex.ysize));
	}

	bool success = atlas.Allocate();
	if (!success) {
		throw content_error("Too many/large texture in 3do texture-atlas to fit in 2048*2048");
	}

	const size_t bigTexX = atlas.GetAtlasSize().x;
	const size_t bigTexY = atlas.GetAtlasSize().y;

	unsigned char* bigtex1 = new unsigned char[bigTexX * bigTexY * 4];
	unsigned char* bigtex2 = new unsigned char[bigTexX * bigTexY * 4];
	for (int a = 0; a < (bigTexX * bigTexY); ++a) {
		bigtex1[a*4 + 0] = 128;
		bigtex1[a*4 + 1] = 128;
		bigtex1[a*4 + 2] = 128;
		bigtex1[a*4 + 3] = 0;

		bigtex2[a*4 + 0] = 0;
		bigtex2[a*4 + 1] = 128;
		bigtex2[a*4 + 2] = 0;
		bigtex2[a*4 + 3] = 255;
	}

	for (std::vector<TexFile*>::iterator it = texfiles.begin(); it != texfiles.end(); ++it) {
		CBitmap& curtex1 = (*it)->tex;
		CBitmap& curtex2 = (*it)->tex2;

		const size_t foundx = atlas.GetEntry((*it)->name).x;
		const size_t foundy = atlas.GetEntry((*it)->name).y;
		const float4 texCoords = atlas.GetTexCoords((*it)->name);

		for (int y = 0; y < curtex1.ysize; ++y) {
			for (int x = 0; x < curtex1.xsize; ++x) {
				for (int col = 0; col < 4; ++col) {
					bigtex1[(((foundy + y) * bigTexX + (foundx + x)) * 4) + col] = curtex1.mem[(((y * curtex1.xsize) + x) * 4) + col];
					bigtex2[(((foundy + y) * bigTexX + (foundx + x)) * 4) + col] = curtex2.mem[(((y * curtex1.xsize) + x) * 4) + col];
				}
			}
		}

		textures[(*it)->name] = UnitTexture(texCoords);

		delete (*it);
	}

	const int maxMipMaps = atlas.GetMaxMipMaps();

	glGenTextures(1, &atlas3do1);
		glBindTexture(GL_TEXTURE_2D, atlas3do1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);
		if (maxMipMaps > 0) {
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bigTexX, bigTexY, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1);
		}

	glGenTextures(1, &atlas3do2);
		glBindTexture(GL_TEXTURE_2D, atlas3do2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (maxMipMaps > 0) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  maxMipMaps);
		if (maxMipMaps > 0) {
			glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2); //FIXME disable texcompression
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bigTexX, bigTexY, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2);
		}

#if 0
	CBitmap save(bigtex1, bigTexX, bigTexY);
	save.Save("unittex1.jpg");
	CBitmap save2(bigtex2, bigTexX, bigTexY);
	save2.Save("unittex2.jpg");
#endif

	textures["___dummy___"] = UnitTexture(0.0f, 0.0f, 1.0f, 1.0f);

	delete[] bigtex1;
	delete[] bigtex2;
}

C3DOTextureHandler::~C3DOTextureHandler()
{
	glDeleteTextures(1, &atlas3do1);
	glDeleteTextures(1, &atlas3do2);
}


std::vector<TexFile*> C3DOTextureHandler::LoadTexFiles()
{
	CFileHandler teamTexFile("unittextures/tatex/teamtex.txt");
	CFileHandler paletteFile("unittextures/tatex/palette.pal");

	CSimpleParser parser(teamTexFile);

	std::set<std::string> teamTexes;
	while (!parser.Eof()) {
		teamTexes.insert(StringToLower(parser.GetCleanLine()));
	}

	std::vector<TexFile*> texfiles;

	const std::vector<std::string>& filesBMP = CFileHandler::FindFiles("unittextures/tatex/", "*.bmp");
	std::vector<std::string> files = CFileHandler::FindFiles("unittextures/tatex/", "*.tga");
	files.insert(files.end(), filesBMP.begin(), filesBMP.end());

	std::set<string> usedNames;
	for (std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
		const std::string& s = *fi;
		const std::string s2 = StringToLower(FileSystem::GetBasename(s));

		// avoid duplicate names and give tga images priority
		if (usedNames.find(s2) != usedNames.end()) {
			continue;
		}
		usedNames.insert(s2);

		if(teamTexes.find(s2) == teamTexes.end()){
			TexFile* tex = CreateTex(s, s2, false);
			texfiles.push_back(tex);
		} else {
			TexFile* tex = CreateTex(s, s2, true);
			texfiles.push_back(tex);
		}
	}

	if (paletteFile.FileExists()) {
		palette.Init(paletteFile);
	}

	for (unsigned a = 0; a < CTAPalette::NUM_PALETTE_ENTRIES; ++a) {
		const std::string name = "ta_color" + IntToString(a, "%i");

		TexFile* tex = new TexFile;
		tex->name = name;
		tex->tex.Alloc(1, 1);
		tex->tex.mem[0] = palette[a][0];
		tex->tex.mem[1] = palette[a][1];
		tex->tex.mem[2] = palette[a][2];
		tex->tex.mem[3] = 0; // teamcolor

		tex->tex2.Alloc(1, 1);
		tex->tex2.mem[0] = 0;  // self illum
		tex->tex2.mem[1] = 30; // reflectivity
		tex->tex2.mem[2] =  0;
		tex->tex2.mem[3] = 255;

		texfiles.push_back(tex);
	}

	return texfiles;
}


C3DOTextureHandler::UnitTexture* C3DOTextureHandler::Get3DOTexture(const std::string& name)
{
	std::map<std::string, UnitTexture>::iterator tti;
	if ((tti = textures.find(name)) != textures.end()) {
		return &tti->second;
	}

	// unknown texture
	return NULL;
}

void C3DOTextureHandler::Set3doAtlases() const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, atlas3do2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas3do1);
}

TexFile* C3DOTextureHandler::CreateTex(const std::string& name, const std::string& name2, bool teamcolor)
{
	TexFile* tex = new TexFile;
	tex->tex.Load(name, 30);
	tex->name = name2;

	tex->tex2.Alloc(tex->tex.xsize, tex->tex.ysize);

	CBitmap* tex1 = &tex->tex;
	CBitmap* tex2 = &tex->tex2;

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

	return tex;
}

