/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <string>
#include <vector>
#include <map>

#include "System/creg/creg_cond.h"

class IAtlasAllocator;


/** @brief Class for combining multiple bitmaps into one large single bitmap. */
struct AtlasedTexture
{
public:
	CR_DECLARE_STRUCT(AtlasedTexture);

	AtlasedTexture() { xstart = xend = ystart = yend = 0.0f; }

	AtlasedTexture(const float* v) {
		xstart = v[0];
		xend   = v[1];
		ystart = v[2];
		yend   = v[3];
	}

	float xstart;
	float xend;
	float ystart;
	float yend;
};

/**
 * @brief Same as AtlasedTexture, but with a different name,
 * so the explosiongenerator can differentiate between different atlases.
 */
struct GroundFXTexture : public AtlasedTexture
{
public:
	CR_DECLARE_STRUCT(AtlasedTexture);
};


class CTextureAtlas
{
public:
	//! set to true to write finalized texture atlas to disk
	static bool debug;

	unsigned int gltex;
	bool freeTexture; //! free texture on atlas destruction?

	int xsize;
	int ysize;

public:
	CTextureAtlas(int maxxSize, int maxySize);
	~CTextureAtlas();

public:
	enum TextureType {
		RGBA32
	};

	//! Add a texture from a memory pointer returns -1 if failed.
	int AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void* data);

	/**
	 * Returns a memory pointer to the texture pixel data array.
	 * (reduces redundant memcpy in contrast to AddTexFromMem())
	 */
	void* AddTex(std::string name, int xsize, int ysize, TextureType texType = RGBA32);

	//! Add a texture from a file, returns -1 if failed.
	int AddTexFromFile(std::string name, std::string file);

public:
	/**
	 * Creates the atlas containing all the specified textures.
	 * @return true if suceeded, false if not all textures did fit
	 *         into the specified maxsize.
	 */
	bool Finalize();
	void BindTexture();

public:
	/**
	 * @return a boolean true if the texture exists within
	 *         the "textures" map and false if it does not.
	 */
	bool TextureExists(const std::string& name);

	//! @return reference to the Texture struct of the specified texture
	AtlasedTexture& GetTexture(const std::string& name);
	
	/**
	 * @return a Texture struct of the specified texture if it exists,
	 *         otherwise return a backup texture.
	 */
	AtlasedTexture& GetTextureWithBackup(const std::string& name, const std::string& backupName);

protected:
	IAtlasAllocator* atlasAllocator;

	struct MemTex
	{
		std::vector<std::string> names;
		int xsize, ysize;
		TextureType texType;
		void* data;
	};

	// temporary storage of all textures
	std::vector<MemTex*> memtextures;
	std::map<std::string, MemTex*> files;

	std::map<std::string, AtlasedTexture> textures;
	int maxxsize;
	int maxysize;
	int usedPixels;
	bool initialized;

	int GetBPP(TextureType tetxType);
	void CreateTexture();
};

#endif // TEXTURE_ATLAS_H
