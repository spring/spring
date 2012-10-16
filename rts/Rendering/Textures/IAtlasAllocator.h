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
	virtual void Allocate() = 0;

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
		//FIXME adjust texture coordinates by half a pixel (in opengl pixel centers are centeriods)???
		float4 uv(entries[name].texCoords);
		uv.x /= atlasSize.x;
		uv.y /= atlasSize.y;
		uv.z /= atlasSize.x;
		uv.w /= atlasSize.y;
		return uv;
	}

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
};

#endif // IATLAS_ALLOC_H
