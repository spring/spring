/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IATLAS_ALLOC_H
#define IATLAS_ALLOC_H

#include "System/float4.h"
#include "System/Vec2.h"
#include <string>
#include <map>


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
	void AddEntry(const std::string& name, int2 size)
	{
		entries[name] = SAtlasEntry(size);
	}

	float4 GetEntry(const std::string& name)
	{
		return entries[name].texCoords;
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

	int2 GetMaxSize() const { return maxsize; }
	int2 GetAtlasSize() const { return atlasSize; }

protected:
	struct SAtlasEntry
	{
		SAtlasEntry() {}
		SAtlasEntry(int2 _size) : size(_size) {}
		
		int2 size;
		float4 texCoords;
	};

	std::map<std::string, SAtlasEntry> entries;

	int2 atlasSize;
	int2 maxsize;

	bool npot;
};

#endif // IATLAS_ALLOC_H
