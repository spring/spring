/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IATLAS_ALLOC_H
#define IATLAS_ALLOC_H

#include <string>

#include "System/float4.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"



class IAtlasAllocator
{
public:
	IAtlasAllocator() : maxsize(2048, 2048), npot(false) {}
	virtual ~IAtlasAllocator() {}

	void SetMaxSize(int xsize, int ysize) { maxsize = int2(xsize, ysize); }
	void SetNonPowerOfTwo(bool nonPowerOfTwo) { npot = nonPowerOfTwo; }

public:
	virtual bool Allocate() = 0;
	virtual int GetMaxMipMaps() = 0;

public:
	void AddEntry(const std::string& name, int2 size, void* data = nullptr)
	{
		entries[name] = SAtlasEntry(size, data);
	}

	float4 GetEntry(const std::string& name)
	{
		return entries[name].texCoords;
	}

	void*& GetEntryData(const std::string& name)
	{
		return entries[name].data;
	}

	float4 GetTexCoords(const std::string& name)
	{
		float4 uv(entries[name].texCoords);
		uv.x1 /= atlasSize.x;
		uv.y1 /= atlasSize.y;
		uv.x2 /= atlasSize.x;
		uv.y2 /= atlasSize.y;

		// adjust texture coordinates by half a texel (opengl uses centeroids)
		uv.x1 += 0.5f / atlasSize.x;
		uv.y1 += 0.5f / atlasSize.y;
		uv.x2 += 0.5f / atlasSize.x;
		uv.y2 += 0.5f / atlasSize.y;

		return uv;
	}

	bool contains(const std::string& name) const
	{
		return (entries.find(name) != entries.end());
	}

	//! note: it doesn't clear the atlas! it only clears the entry db!
	void clear()
	{
		entries.clear();
	}
	//auto begin() { return entries.begin(); }
	//auto end() { return entries.end(); }

	int2 GetMaxSize() const { return maxsize; }
	int2 GetAtlasSize() const { return atlasSize; }

protected:
	struct SAtlasEntry
	{
		SAtlasEntry() : data(nullptr) {}
		SAtlasEntry(const int2 _size, void* _data = nullptr) : size(_size), data(_data) {}
		
		int2 size;
		float4 texCoords;
		void* data;
	};

	spring::unordered_map<std::string, SAtlasEntry> entries;

	int2 atlasSize;
	int2 maxsize;

	bool npot;
};

#endif // IATLAS_ALLOC_H
