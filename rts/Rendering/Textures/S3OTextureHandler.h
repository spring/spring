/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_TEXTURE_HANDLER_H
#define S3O_TEXTURE_HANDLER_H

#include <boost/unordered_map.hpp>
#include <string>
#include <vector>

#include "System/Threading/SpringMutex.h"

struct S3DModel;
class CBitmap;

class CS3OTextureHandler
{
public:
	struct S3OTexMat {
		int num;

		unsigned int tex1;
		unsigned int tex2;

		unsigned int tex1SizeX;
		unsigned int tex1SizeY;

		unsigned int tex2SizeX;
		unsigned int tex2SizeY;
	};

	struct CachedS3OTex {
		unsigned int texID;
		unsigned int xsize;
		unsigned int ysize;
	};

	typedef S3OTexMat S3oTex;


	CS3OTextureHandler();
	~CS3OTextureHandler();

	void LoadS3OTexture(S3DModel* model);
	void PreloadS3OTexture(S3DModel* model);
	void SetS3oTexture(int num);

public:
	const S3OTexMat* GetS3oTex(unsigned int num) {
		if (num < textures.size())
			return &textures[num];

		return nullptr;
	}

private:
	unsigned int LoadTexture(const S3DModel* model, const std::string& textureName, bool isTex1, bool preload);
	int LoadS3OTextureNow(const S3DModel* model);
	unsigned int InsertTextureMat(const S3DModel* model);

private:
	typedef boost::unordered_map<std::string, CachedS3OTex> TextureCache;
	typedef boost::unordered_map<std::string, CBitmap*> BitmapCache;
	typedef boost::unordered_map<boost::uint64_t, unsigned int> TextureTable;

	TextureCache textureCache; // stores individual primary- and secondary-textures by name
	TextureTable textureTable; // stores (primary, secondary) texture-pairs by unique ident
	BitmapCache bitmapCache;
	spring::mutex cacheMutex;

	std::vector<S3OTexMat> textures;
};

extern CS3OTextureHandler* texturehandlerS3O;

#endif /* S3O_TEXTURE_HANDLER_H */
