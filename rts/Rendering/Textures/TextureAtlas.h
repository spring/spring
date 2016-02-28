/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <string>
#include <vector>
#include <map>

#include "System/creg/creg_cond.h"
#include "System/float4.h"
#include "System/type2.h"


class IAtlasAllocator;

/** @brief texture coordinates of an atlas subimage. */
//typedef float4 AtlasedTexture;

struct AtlasedTexture : public float4
{
	AtlasedTexture() : float4() {}
	AtlasedTexture(const float4& f) : float4(f) {}

	CR_DECLARE_STRUCT(AtlasedTexture)
};



/** @brief Class for combining multiple bitmaps into one large single bitmap. */
class CTextureAtlas
{
public:
	enum TextureType {
		RGBA32
	};
	enum {
		ATLAS_ALLOC_LEGACY   = 0,
		ATLAS_ALLOC_QUADTREE = 1,
	};

public:
	CTextureAtlas(unsigned int allocType = ATLAS_ALLOC_LEGACY);
	~CTextureAtlas();

	//! Add a texture from a memory pointer returns -1 if failed.
	int AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void* data);

	/**
	 * Returns a memory pointer to the texture pixel data array.
	 * (reduces redundant memcpy in contrast to AddTexFromMem())
	 */
	void* AddTex(std::string name, int xsize, int ysize, TextureType texType = RGBA32);

	//! Add a texture from a file, returns -1 if failed.
	int AddTexFromFile(std::string name, std::string file);


	/**
	 * Creates the atlas containing all the specified textures.
	 * @return true if suceeded, false if not all textures did fit
	 *         into the specified maxsize.
	 */
	bool Finalize();

	/**
	 * @return a boolean true if the texture exists within
	 *         the "textures" map and false if it does not.
	 */
	bool TextureExists(const std::string& name);


	//! @return reference to the Texture struct of the specified texture
	AtlasedTexture& GetTexture(const std::string& name);

	AtlasedTexture* GetTexturePtr(const std::string& name) { return &GetTexture(name); }

	/**
	 * @return a Texture struct of the specified texture if it exists,
	 *         otherwise return a backup texture.
	 */
	AtlasedTexture& GetTextureWithBackup(const std::string& name, const std::string& backupName);


	IAtlasAllocator* GetAllocator() { return atlasAllocator; }

	int2 GetSize() const;
	std::string GetName() const { return name; }

	unsigned int GetTexID() const { return atlasTexID; }

	void BindTexture();
	void SetFreeTexture(bool b) { freeTexture = b; }
	void SetName(const std::string& s) { name = s; }

	static void SetDebug(bool b) { debug = true; }
	static bool GetDebug() { return debug; }

protected:
	int GetBPP(TextureType tetxType);
	void CreateTexture();

protected:
	IAtlasAllocator* atlasAllocator;

	struct MemTex
	{
		std::vector<std::string> names;
		int xsize, ysize;
		TextureType texType;
		void* data;
	};

	std::string name;

	// temporary storage of all textures
	std::vector<MemTex*> memtextures;
	std::map<std::string, MemTex*> files;

	std::map<std::string, AtlasedTexture> textures;

	unsigned int atlasTexID;

	//! set to true to write finalized texture atlas to disk
	static bool debug;

	bool initialized;
	bool freeTexture; //! free texture on atlas destruction?
};

#endif // TEXTURE_ATLAS_H
