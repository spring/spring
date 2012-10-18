/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUADTREE_ATLAS_ALLOC_H
#define QUADTREE_ATLAS_ALLOC_H

#include "IAtlasAllocator.h"


struct QuadTreeNode;


class CQuadtreeAtlasAlloc : public IAtlasAllocator
{
public:
	CQuadtreeAtlasAlloc();
	virtual ~CQuadtreeAtlasAlloc();

	virtual bool Allocate();
	virtual int GetMaxMipMaps();

private:
	static int CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2);
	QuadTreeNode* FindPosInQuadTree(int xsize, int ysize);

private:
	QuadTreeNode* root;
};

#endif // QUADTREE_ATLAS_ALLOC_H
