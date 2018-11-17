/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // memset
#include <algorithm>
#include <vector>

#include "QuadtreeAtlasAlloc.h"
#include "System/Exceptions.h"
#include "System/bitops.h"
#include "System/Log/ILog.h"

static int NODE_MIN_SIZE = 8;


struct QuadTreeNode {
	QuadTreeNode() {
		used = false;
		posx = posy = size = 0;
		memset(children, 0, 4 * sizeof(QuadTreeNode*));
	}
	~QuadTreeNode() {
		for (auto& child: children) {
			if (child != nullptr) {
				delete child;
				child = nullptr;
			}
		}
	}

	QuadTreeNode(QuadTreeNode* node, int i) {
		// i ... 0:=topleft, 1:=topright, 2:=btmleft, 3:=btmright
		used = false;
		size = node->size >> 1;
		posx = node->posx + ((i & 1)     ) * size;
		posy = node->posy + ((i & 2) >> 1) * size;
		memset(children, 0, 4 * sizeof(QuadTreeNode*));
	}

	QuadTreeNode* FindPosInQuadTree(int xsize, int ysize);

	int GetMinSize() {
		int minsize = size;
		for (const auto& child: children) {
			if (child != nullptr) {
				minsize = std::min(minsize, child->GetMinSize());
			}
		}
		return minsize;
	}

	short int posx;
	short int posy;
	int size;
	bool used;
	QuadTreeNode* children[4];
};


QuadTreeNode* QuadTreeNode::FindPosInQuadTree(int xsize, int ysize)
{
	if (used)
		return nullptr;

	if ((size < xsize) || (size < ysize))
		return nullptr;

	const bool xfit = ((size >> 1) < xsize);
	const bool yfit = ((size >> 1) < ysize);
	const bool minSizeReached = (size <= NODE_MIN_SIZE);

	if (xfit || yfit || minSizeReached) {
		if (!children[0]) {
			if (!minSizeReached) {
				if (!yfit) {
					children[0] = new QuadTreeNode(this, 0);
					children[1] = new QuadTreeNode(this, 1);
					children[0]->used = true;
					children[1]->used = true;
					return this;
				}
				if (!xfit) {
					children[0] = new QuadTreeNode(this, 0);
					children[2] = new QuadTreeNode(this, 2);
					children[0]->used = true;
					children[2]->used = true;
					return this;
				}
			}

			used = true;
			return this;
		} else {
			return nullptr; //FIXME dynamic x/y quadnode size?
		}
	}

	for (int i = 0; i < 4; ++i) {
		if (!children[i]) children[i] = new QuadTreeNode(this, i);
		QuadTreeNode* subnode = children[i]->FindPosInQuadTree(xsize, ysize);
		if (subnode) return subnode;
	}

	return nullptr;
}


CQuadtreeAtlasAlloc::CQuadtreeAtlasAlloc()
{
	root = nullptr;
}


CQuadtreeAtlasAlloc::~CQuadtreeAtlasAlloc()
{
	delete root;
}


QuadTreeNode* CQuadtreeAtlasAlloc::FindPosInQuadTree(int xsize, int ysize)
{
	QuadTreeNode* node = root->FindPosInQuadTree(xsize, ysize);
	while (!node) {
		if (root->size >= maxsize.x) {
			break;
		}

		if (!root->used && !root->children[0]) {
			root->size = root->size << 1;
			node = root->FindPosInQuadTree(xsize, ysize);
			continue;
		}
		QuadTreeNode* oldroot = root;
		root = new QuadTreeNode();
		root->posx = 0;
		root->posy = 0;
		root->size = oldroot->size << 1;
		root->children[0] = oldroot;
		node = root->FindPosInQuadTree(xsize, ysize);
	}
	return node;
}


int CQuadtreeAtlasAlloc::CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2)
{
	const size_t area1 = std::max(tex1->size.x, tex1->size.y);
	const size_t area2 = std::max(tex2->size.x, tex2->size.y);
	return (area1 > area2);
}


bool CQuadtreeAtlasAlloc::Allocate()
{
	if (root == nullptr) {
		root = new QuadTreeNode();
		root->posx = 0;
		root->posy = 0;
		root->size = 32;
	}

	bool failure = false;

	std::vector<SAtlasEntry*> sortedEntries;
	sortedEntries.reserve(entries.size());

	for (auto& entry: entries) {
		sortedEntries.push_back(&entry.second);
	}

	std::sort(sortedEntries.begin(), sortedEntries.end(), CQuadtreeAtlasAlloc::CompareTex);

	for (const auto& item: sortedEntries) {
		SAtlasEntry& entry = *item;
		QuadTreeNode* node = FindPosInQuadTree(entry.size.x, entry.size.y);

		if (node == nullptr) {
			for (auto jt = entries.begin(); jt != entries.end(); ++jt) {
				if (&entry == &(jt->second)) {
					LOG_L(L_ERROR, "CQuadtreeAtlasAlloc full: failed to add %s", jt->first.c_str());
					break;
				}
			}
			failure = true;
			continue;
		}

		entry.texCoords.x1 = node->posx;
		entry.texCoords.y1 = node->posy;
		entry.texCoords.x2 = node->posx + entry.size.x - 1; //FIXME stretch if image is smaller than node!
		entry.texCoords.y2 = node->posy + entry.size.y - 1;
	}

	atlasSize.x = root->size;
	atlasSize.y = root->size;
	if (!root->children[2] && !root->children[3]) {
		atlasSize.y = std::max(atlasSize.y >> 1, NODE_MIN_SIZE);
	}

	return !failure;
}


int CQuadtreeAtlasAlloc::GetMaxMipMaps()
{
	if (!root) return 0;
	return bits_ffs(root->GetMinSize());
}
