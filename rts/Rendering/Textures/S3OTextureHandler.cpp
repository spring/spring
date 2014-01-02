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

#define TEX_MAT_UID(pTxID, sTxID) ((boost::uint64_t(pTxID) << 32u) | sTxID)
#define TEX_TBL_UID(pIter, sIter) TEX_MAT_UID((pIter->second).texID, (sIter->second).texID)


// The S3O texture handler uses two textures.
// The first contains diffuse color (RGB) and teamcolor (A)
// The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CS3OTextureHandler* texturehandlerS3O = NULL;

CS3OTextureHandler::CS3OTextureHandler()
{
	// dummies
	textures.push_back(new S3OTexMat());
	textures.push_back(new S3OTexMat());

	if (GML::SimEnabled() && GML::ShareLists())
		DoUpdateDraw();
}

CS3OTextureHandler::~CS3OTextureHandler()
{
	for (int i = 0; i < textures.size(); ++i){
		glDeleteTextures(1, &textures[i]->tex1);
		glDeleteTextures(1, &textures[i]->tex2);
		delete textures[i];
	}
}

void CS3OTextureHandler::LoadS3OTexture(S3DModel* model) {
	model->textureType = GML::SimEnabled() && !GML::ShareLists() && GML::IsSimThread() ? -1 : LoadS3OTextureNow(model);
}

int CS3OTextureHandler::LoadS3OTextureNow(const S3DModel* model)
{
	GML_RECMUTEX_LOCK(model); // LoadS3OTextureNow
	LOG("Load S3O texture now (Flip Y Axis: %s, Invert Team Alpha: %s)",
			model->invertTexYAxis ? "yes" : "no",
			model->invertTexAlpha ? "yes" : "no");

	CBitmap texBitMaps[2];
	TextureCacheIt texCacheIters[2] = {
		textureCache.find(model->tex1),
		textureCache.find(model->tex2),
	};
	TextureTableIt texTableIter;

	if (texCacheIters[0] == textureCache.end()) {
		if (!texBitMaps[0].Load(model->tex1)) {
			if (!texBitMaps[0].Load("unittextures/" + model->tex1)) {
				LOG_L(L_WARNING, "[%s] could not load texture \"%s\" from model \"%s\"",
					__FUNCTION__, model->tex1.c_str(), model->name.c_str());

				// file not found (or headless build), set single pixel to red so unit is visible
				texBitMaps[0].Alloc(1, 1, 4);
				texBitMaps[0].mem[0] = 255;
				texBitMaps[0].mem[1] =   0;
				texBitMaps[0].mem[2] =   0;
				texBitMaps[0].mem[3] = 255; // team-color
			}
		}

		if (model->invertTexAlpha)
			texBitMaps[0].InvertAlpha();
		if (model->invertTexYAxis)
			texBitMaps[0].ReverseYAxis();

		textureCache[model->tex1] = {
			texBitMaps[0].CreateTexture(true),
			static_cast<unsigned int>(texBitMaps[0].xsize),
			static_cast<unsigned int>(texBitMaps[0].ysize)
		};
	}

	if (texCacheIters[1] == textureCache.end()) {
		if (!texBitMaps[1].Load(model->tex2)) {
			if (!texBitMaps[1].Load("unittextures/" + model->tex2)) {
				texBitMaps[1].Alloc(1, 1, 4);
				texBitMaps[1].mem[0] =   0; // self-illum
				texBitMaps[1].mem[1] =   0; // spec+refl
				texBitMaps[1].mem[2] =   0; // unused
				texBitMaps[1].mem[3] = 255; // transparency
			}
		}

		if (model->invertTexYAxis)
			texBitMaps[1].ReverseYAxis();

		textureCache[model->tex2] = {
			texBitMaps[1].CreateTexture(true),
			static_cast<unsigned int>(texBitMaps[1].xsize),
			static_cast<unsigned int>(texBitMaps[1].ysize)
		};
	}

	if (texCacheIters[0] == textureCache.end() || texCacheIters[1] == textureCache.end()) {
		// at least one texture was newly loaded, create new material
		// (note: at this point the cache has grown to contain both)
		return (InsertTextureMat(model)->num);
	}
	if ((texTableIter = textureTable.find(TEX_TBL_UID(texCacheIters[0], texCacheIters[1]))) == textureTable.end()) {
		// both textures were already loaded as parts of other models
		// one possible example where this can happen would be e.g.:
		//   model 1 uses textures A and B and gets loaded 1st
		//   model 2 uses textures B and C and gets loaded 2nd
		//   model 3 uses textures A and C and gets loaded 3rd
		//   --> (A,C) is a new pair consisting of old textures
		return (InsertTextureMat(model)->num);
	}

	return ((texTableIter->second)->num);
}

CS3OTextureHandler::S3OTexMat* CS3OTextureHandler::InsertTextureMat(const S3DModel* model)
{
	const CachedS3OTex& tex1 = textureCache[model->tex1];
	const CachedS3OTex& tex2 = textureCache[model->tex2];

	S3OTexMat* texMat = new S3OTexMat();
	texMat->num       = textures.size();
	texMat->tex1      = tex1.texID;
	texMat->tex2      = tex2.texID;
	texMat->tex1SizeX = tex1.xsize;
	texMat->tex1SizeY = tex1.ysize;
	texMat->tex2SizeX = tex2.xsize;
	texMat->tex2SizeY = tex2.ysize;

	textures.push_back(texMat);
	textureTable[TEX_MAT_UID(texMat->tex1, texMat->tex2)] = texMat;

	if (GML::SimEnabled() && GML::ShareLists() && !GML::IsSimThread())
		DoUpdateDraw();

	return texMat;
}

inline void DoSetTexture(const CS3OTextureHandler::S3OTexMat* texMat) {
	if (shadowHandler->inShadowPass) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texMat->tex2);
	} else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texMat->tex2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texMat->tex1);
	}
}


void CS3OTextureHandler::SetS3oTexture(int num)
{
	if (GML::SimEnabled() && GML::ShareLists()) {
		if (!GML::IsSimThread()) {
			DoSetTexture(texturesDraw[num]);
		} else {
			// it seems this is only accessed by draw thread, but just in case..
			GML_RECMUTEX_LOCK(model); // SetS3oTexture
			DoSetTexture(textures[num]);
		}
	} else {
		DoSetTexture(textures[num]);
	}
}

void CS3OTextureHandler::UpdateDraw() {
	if (GML::SimEnabled() && GML::ShareLists()) {
		GML_RECMUTEX_LOCK(model); // UpdateDraw

		DoUpdateDraw();
	}
}

