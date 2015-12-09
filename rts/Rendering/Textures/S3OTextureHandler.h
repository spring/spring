/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_TEXTURE_HANDLER_H
#define S3O_TEXTURE_HANDLER_H

#include <boost/unordered_map.hpp>
#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"

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
	int LoadS3OTextureNow(const S3DModel* model);
	void SetS3oTexture(int num);

public:
	const S3OTexMat* GetS3oTex(unsigned int num) {
		if (num < textures.size())
			return textures[num];

		return nullptr;
	}

	void UpdateDraw();

private:
	S3OTexMat* InsertTextureMat(const S3DModel* model);

private:
	typedef boost::unordered_map<std::string, CachedS3OTex> TextureCache;
	typedef boost::unordered_map<std::string, CachedS3OTex>::const_iterator TextureCacheIt;
	typedef boost::unordered_map<boost::uint64_t, S3OTexMat*> TextureTable;
	typedef boost::unordered_map<boost::uint64_t, S3OTexMat*>::const_iterator TextureTableIt;

	TextureCache textureCache; // stores individual primary- and secondary-textures by name
	TextureTable textureTable; // stores (primary, secondary) texture-pairs by unique ident

	std::vector<S3OTexMat*> textures;
};

extern CS3OTextureHandler* texturehandlerS3O;

#endif /* S3O_TEXTURE_HANDLER_H */
