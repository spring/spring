/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include "mmgr.h"

#include "S3OTextureHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/Bitmap.h"
#include "TAPalette.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"


CS3OTextureHandler* texturehandlerS3O = NULL;

CS3OTextureHandler::CS3OTextureHandler()
{
	s3oTextures.push_back(S3oTex());
	s3oTextures.push_back(S3oTex());
}

CS3OTextureHandler::~CS3OTextureHandler()
{
	while (s3oTextures.size() > 1){
		glDeleteTextures (1, &s3oTextures.back().tex1);
		glDeleteTextures (1, &s3oTextures.back().tex2);
		s3oTextures.pop_back();
	}
}

void CS3OTextureHandler::LoadS3OTexture(S3DModel* model) {
#if defined(USE_GML) && GML_ENABLE_SIM
	model->textureType = -1;
#else
	model->textureType = LoadS3OTextureNow(model);
#endif
}

void CS3OTextureHandler::Update() {
}

int CS3OTextureHandler::LoadS3OTextureNow(const S3DModel* model)
{
	GML_STDMUTEX_LOCK(model); // LoadS3OTextureNow

	const string totalName = model->tex1 + model->tex2;

	if (s3oTextureNames.find(totalName) != s3oTextureNames.end()) {
		return s3oTextureNames[totalName];
	}

	CBitmap tex1bm;
	CBitmap tex2bm;
	S3oTex tex;

	if (!tex1bm.Load(std::string("unittextures/" + model->tex1))) {
		logOutput.Print("[%s] could not load texture \"%s\" from model \"%s\"", __FUNCTION__, model->tex1.c_str(), model->name.c_str());

		// file not found (or headless build), set single pixel to red so unit is visible
		tex1bm.Alloc(1, 1);
		tex1bm.mem[0] = 255;
		tex2bm.mem[1] =   0;
		tex2bm.mem[2] =   0;
		tex1bm.mem[3] = 255;
	}

	tex.num       = s3oTextures.size();
	tex.tex1      = tex1bm.CreateTexture(true);
	tex.tex1SizeX = tex1bm.xsize;
	tex.tex1SizeY = tex1bm.ysize;

	// No error checking here... other code relies on an empty texture
	// being generated if it couldn't be loaded.
	// Also many map features specify a tex2 but don't ship it with the map,
	// so throwing here would cause maps to break.
	if (!tex2bm.Load(std::string("unittextures/" + model->tex2))) {
		tex2bm.Alloc(1, 1);
		tex2bm.mem[0] = 255;
		tex2bm.mem[1] = 255;
		tex2bm.mem[2] = 255;
		tex2bm.mem[3] = 255;
	}

	tex.tex2      = tex2bm.CreateTexture(true);
	tex.tex2SizeX = tex2bm.xsize;
	tex.tex2SizeY = tex2bm.ysize;

	s3oTextures.push_back(tex);
	s3oTextureNames[totalName] = tex.num;

	return tex.num;
}

void CS3OTextureHandler::SetS3oTexture(int num)
{
	if (shadowHandler->inShadowPass) {
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
	} else {
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex1);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
	}
}
