/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <string>
#include <vector>

#include "System/creg/creg_cond.h"
#include "System/float4.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"


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

	// add a texture from a memory pointer
	size_t AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void* data);
	// add a texture from a file
	size_t AddTexFromFile(std::string name, std::string file);
	// add a blank texture
	size_t AddTex(std::string name, int xsize, int ysize, TextureType texType = RGBA32);

	void* AddGetTex(std::string name, int xsize, int ysize, TextureType texType = RGBA32) {
		const size_t idx = AddTex(std::move(name), xsize, ysize, texType);

		MemTex& tex = memTextures[idx];
		auto& mem = tex.mem;
		return (mem.data());
	}


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

	// NOTE: safe with unordered_map after atlas has been finalized
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

	static void SetDebug(bool b) { debug = b; }
	static bool GetDebug() { return debug; }

protected:
	int GetBPP(TextureType texType) const {
		switch (texType) {
			case RGBA32: return 32;
			default: return 32;
		}
	}
	bool CreateTexture();

protected:
	IAtlasAllocator* atlasAllocator;

	struct MemTex {
	public:
		MemTex(): xsize(0), ysize(0), texType(RGBA32) {}
		MemTex(const MemTex&) = delete;
		MemTex(MemTex&& t) { *this = std::move(t); }

		MemTex& operator = (const MemTex&) = delete;
		MemTex& operator = (MemTex&& t) {
			xsize = t.xsize;
			ysize = t.ysize;
			texType = t.texType;

			names = std::move(t.names);
			mem = std::move(t.mem);
			return *this;
		}

	public:
		int xsize;
		int ysize;

		TextureType texType;

		std::vector<std::string> names;
		std::vector<char> mem;
	};

	std::string name;

	// temporary storage of all textures
	std::vector<MemTex> memTextures;

	spring::unordered_map<std::string, size_t> files;
	spring::unordered_map<std::string, AtlasedTexture> textures;

	unsigned int atlasTexID;

	//! set to true to write finalized texture atlas to disk
	static bool debug;

	bool initialized;
	bool freeTexture; //! free texture on atlas destruction?
};

#endif // TEXTURE_ATLAS_H
