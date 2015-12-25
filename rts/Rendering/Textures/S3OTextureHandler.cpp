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
	textures.push_back(S3OTexMat());
	textures.push_back(S3OTexMat());
}

CS3OTextureHandler::~CS3OTextureHandler()
{
	for (S3OTexMat& texture: textures){
		glDeleteTextures(1, &(texture.tex1));
		glDeleteTextures(1, &(texture.tex2));
	}
	for (auto& it: bitmapCache) {
		delete it.second;
	}
}

void CS3OTextureHandler::LoadS3OTexture(S3DModel* model)
{
	boost::mutex::scoped_lock(cacheMutex);
	model->textureType = LoadS3OTextureNow(model);
}

void CS3OTextureHandler::PreloadS3OTexture(S3DModel* model)
{
	boost::mutex::scoped_lock(cacheMutex);
	LoadTexture(model, model->tex1, true, true);
	LoadTexture(model, model->tex2, false, true);
}


unsigned int CS3OTextureHandler::LoadTexture(const S3DModel* model, const std::string& textureName, bool isTex1, bool preload)
{
	CBitmap _bitmap;
	CBitmap* bitmap = &_bitmap;

	bool delBitmap = false;

	auto textureIt = textureCache.find(textureName);
	if (textureIt != textureCache.end())
		return textureIt->second.texID;

	auto bitmapIt = bitmapCache.find(textureName);
	if (bitmapIt != bitmapCache.end()) {
		bitmap = bitmapIt->second;
		delBitmap = true;
	} else {
		if (preload) {
			bitmap = new CBitmap();
			bitmapCache[textureName] = bitmap;
		}
		if (!bitmap->Load(textureName)) {
			if (!bitmap->Load("unittextures/" + textureName)) {
				LOG_L(L_WARNING, "[%s] could not load texture \"%s\" from model \"%s\"",
					__FUNCTION__, textureName.c_str(), model->name.c_str());

				// file not found (or headless build), set a single pixel so unit is visible
				bitmap->AllocDummy(isTex1 ? SColor(255, 0, 0, 255) : SColor(0, 0, 0, 255));
			}
		}

		if (isTex1 && model->invertTexAlpha)
			bitmap->InvertAlpha();
		if (model->invertTexYAxis)
			bitmap->ReverseYAxis();

	}
	//Don't generate a texture, just save the bitmap for later
	if (preload)
		return 0;

	const unsigned int texID = bitmap->CreateTexture(true);

	textureCache[textureName] = {
		texID,
		static_cast<unsigned int>(bitmap->xsize),
		static_cast<unsigned int>(bitmap->ysize)
	};

	if (delBitmap) {
		bitmapCache.erase(textureName);
		delete bitmap;
	}

	return texID;
}


int CS3OTextureHandler::LoadS3OTextureNow(const S3DModel* model)
{
	LOG_L(L_INFO, "Load S3O texture now (Flip Y Axis: %s, Invert Team Alpha: %s)",
			model->invertTexYAxis ? "yes" : "no",
			model->invertTexAlpha ? "yes" : "no");

	const unsigned int tex1ID = LoadTexture(model, model->tex1, true, false);
	const unsigned int tex2ID = LoadTexture(model, model->tex2, false, false);

	auto texTableIter = textureTable.find(TEX_MAT_UID(tex1ID, tex2ID));

	// even if both textures were already loaded as parts of
	// other models, the pair may still not be used in a single material
	if (texTableIter == textureTable.end())
		return InsertTextureMat(model);

	return texTableIter->second;
}

unsigned int CS3OTextureHandler::InsertTextureMat(const S3DModel* model)
{
	const CachedS3OTex& tex1 = textureCache[model->tex1];
	const CachedS3OTex& tex2 = textureCache[model->tex2];

	S3OTexMat texMat;
	texMat.num       = textures.size();
	texMat.tex1      = tex1.texID;
	texMat.tex2      = tex2.texID;
	texMat.tex1SizeX = tex1.xsize;
	texMat.tex1SizeY = tex1.ysize;
	texMat.tex2SizeX = tex2.xsize;
	texMat.tex2SizeY = tex2.ysize;

	textures.push_back(texMat);
	textureTable[TEX_MAT_UID(texMat.tex1, texMat.tex2)] = texMat.num;

	return texMat.num;
}

void CS3OTextureHandler::SetS3oTexture(int num)
{
	S3OTexMat& texMat = textures[num];

	if (shadowHandler->inShadowPass) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texMat.tex2);
	} else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texMat.tex2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texMat.tex1);
	}
}
