/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUADTREE_ATLAS_ALLOC_H
#define QUADTREE_ATLAS_ALLOC_H

#include "IAtlasAllocator.h"


class CQuadtreeAtlasAlloc : public IAtlasAllocator
{
public:
	virtual bool Allocate();
	virtual int GetMaxMipMaps();
};

#endif // QUADTREE_ATLAS_ALLOC_H
