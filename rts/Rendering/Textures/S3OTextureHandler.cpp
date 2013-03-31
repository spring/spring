/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "S3OTextureHandler.h"

#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

#define LOG_SECTION_TEXTURE "Texture"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_TEXTURE)
#ifdef LOG_SECTION_CURRENT
        #undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_TEXTURE

// The S3O texture handler uses two textures.
// The first contains diffuse color (RGB) and teamcolor (A)
// The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CS3OTextureHandler* texturehandlerS3O = NULL;

CS3OTextureHandler::CS3OTextureHandler()
{
	s3oTextures.push_back(new S3oTex());
	s3oTextures.push_back(new S3oTex());
	if (GML::SimEnabled() && GML::ShareLists())
		DoUpdateDraw();
}

CS3OTextureHandler::~CS3OTextureHandler()
{
	for (int i = 0; i < s3oTextures.size(); ++i){
		glDeleteTextures(1, &s3oTextures[i]->tex1);
		glDeleteTextures(1, &s3oTextures[i]->tex2);
		delete s3oTextures[i];
	}
}

void CS3OTextureHandler::LoadS3OTexture(S3DModel* model) {
	model->textureType = GML::SimEnabled() && !GML::ShareLists() && GML::IsSimThread() ? -1 : LoadS3OTextureNow(model);
}

int CS3OTextureHandler::LoadS3OTextureNow(const S3DModel* model)
{
	GML_RECMUTEX_LOCK(model); // LoadS3OTextureNow
	LOG("Load S3O texture now (Flip Y Axis: %s, Invert Team Alpha: %s)",
			model->flipTexY ? "yes" : "no",
			model->invertTexAlpha ? "yes" : "no");

	const string totalName = model->tex1 + model->tex2;

	if (s3oTextureNames.find(totalName) != s3oTextureNames.end()) {
		if (GML::SimEnabled() && GML::ShareLists() && !GML::IsSimThread())
			DoUpdateDraw();

		return s3oTextureNames[totalName];
	}

	CBitmap tex1bm;
	CBitmap tex2bm;
	S3oTex* tex = new S3oTex();

	if (!tex1bm.Load(model->tex1)) {
		if (!tex1bm.Load("unittextures/" + model->tex1)) {
			LOG_L(L_WARNING, "[%s] could not load texture \"%s\" from model \"%s\"",
					__FUNCTION__, model->tex1.c_str(), model->name.c_str());

			// file not found (or headless build), set single pixel to red so unit is visible
			tex1bm.channels = 4;
			tex1bm.Alloc(1, 1);
			tex1bm.mem[0] = 255;
			tex1bm.mem[1] =   0;
			tex1bm.mem[2] =   0;
			tex1bm.mem[3] = 255; // team-color
		}
	}

	// No error checking here... other code relies on an empty texture
	// being generated if it couldn't be loaded.
	// Also many map features specify a tex2 but don't ship it with the map,
	// so throwing here would cause maps to break.
	if (!tex2bm.Load(model->tex2)) {
		if (!tex2bm.Load("unittextures/" + model->tex2)) {
			tex2bm.channels = 4;
			tex2bm.Alloc(1, 1);
			tex2bm.mem[0] =   0; // self-illum
			tex2bm.mem[1] =   0; // spec+refl
			tex2bm.mem[2] =   0; // unused
			tex2bm.mem[3] = 255; // transparency
		}
	}

	if (model->invertTexAlpha)
		tex1bm.InvertAlpha();
	if (model->flipTexY) {
		tex1bm.ReverseYAxis();
		tex2bm.ReverseYAxis();
	}

	tex->num       = s3oTextures.size();
	tex->tex1      = tex1bm.CreateTexture(true);
	tex->tex1SizeX = tex1bm.xsize;
	tex->tex1SizeY = tex1bm.ysize;
	tex->tex2      = tex2bm.CreateTexture(true);
	tex->tex2SizeX = tex2bm.xsize;
	tex->tex2SizeY = tex2bm.ysize;

	s3oTextures.push_back(tex);
	s3oTextureNames[totalName] = tex->num;

	if (GML::SimEnabled() && GML::ShareLists() && !GML::IsSimThread())
		DoUpdateDraw();

	return tex->num;
}


inline void DoSetS3oTexture(int num, std::vector<CS3OTextureHandler::S3oTex*>& s3oTex) {
	if (shadowHandler->inShadowPass) {
		glBindTexture(GL_TEXTURE_2D, s3oTex[num]->tex2);
	} else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, s3oTex[num]->tex1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, s3oTex[num]->tex2);
	}
}


void CS3OTextureHandler::SetS3oTexture(int num)
{
	if (GML::SimEnabled() && GML::ShareLists()) {
		if (!GML::IsSimThread()) {
			DoSetS3oTexture(num, s3oTexturesDraw);
		} else {
			// it seems this is only accessed by draw thread, but just in case..
			GML_RECMUTEX_LOCK(model); // SetS3oTexture
			DoSetS3oTexture(num, s3oTextures);
		}
	} else {
		DoSetS3oTexture(num, s3oTextures);
	}
}

void CS3OTextureHandler::UpdateDraw() {
	if (GML::SimEnabled() && GML::ShareLists()) {
		GML_RECMUTEX_LOCK(model); // UpdateDraw

		DoUpdateDraw();
	}
}
