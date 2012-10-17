/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LEGACY_ATLAS_ALLOC_H
#define LEGACY_ATLAS_ALLOC_H

#include "IAtlasAllocator.h"


class CLegacyAtlasAlloc : public IAtlasAllocator
{
public:
	virtual bool Allocate();
	virtual int GetMaxMipMaps() { return 0; }

private:
	bool IncreaseSize();
	static int CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2);
};

#endif // LEGACY_ATLAS_ALLOC_H
