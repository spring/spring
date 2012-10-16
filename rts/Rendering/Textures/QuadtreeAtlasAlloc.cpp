/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "QuadtreeAtlasAlloc.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"


struct QuadTreeNode {
	QuadTreeNode() {
		used = false;
		memset(children, 0, 4 * sizeof(QuadTreeNode*));
	}
	QuadTreeNode(QuadTreeNode* node, int i) {
		used = false;
		size = node->size >> 1;
		posx = node->posx;
		posy = node->posy;

		switch (i) {
			case 0:
				break;
			case 1:
				posy += size;
				break;
			case 2:
				posx += size;
				break;
			case 3:
				posy += size;
				posx += size;
				break;
		}

		memset(children, 0, 4 * sizeof(QuadTreeNode*));
	}
	short int posx;
	short int posy;
	int size;
	bool used;
	QuadTreeNode* children[4];
	
	static int MIN_SIZE;
};

int QuadTreeNode::MIN_SIZE = 8;

static QuadTreeNode* FindPosInQuadTree(QuadTreeNode* node, int xsize, int ysize)
{
	if (node->used)
		return NULL;

	if ((node->size < xsize) || (node->size < ysize))
		return NULL;

	if (((node->size >> 1) < xsize) || ((node->size >> 1) < ysize)) {
		if (!node->children[0]) {
			node->used = true;
			return node;
		} else {
			return NULL;
		}
	}

	if (node->size <= QuadTreeNode::MIN_SIZE)
		return NULL;

	for (int i = 0; i < 4; ++i) {
		if (!node->children[i])
			node->children[i] = new QuadTreeNode(node, i);

		QuadTreeNode* subnode = FindPosInQuadTree(node->children[i], xsize, ysize);
		if (subnode)
			return subnode;
	}

	//FIXME resize whole quadmap then
	return NULL;
}


void CQuadtreeAtlasAlloc::Allocate()
{
	QuadTreeNode root;
	root.posx = 0;
	root.posy = 0;
	root.size = 2048;

	for (std::map<std::string, SAtlasEntry>::iterator it = entries.begin(); it != entries.end(); ++it) {
		QuadTreeNode* node = FindPosInQuadTree(&root, it->second.size.x, it->second.size.y);

		if (!node) {
			//LOG_L(L_ERROR, "DecalTextureAtlas full: failed to add %s", it->first.c_str());
			continue;
		}

		it->second.texCoords.x = node->posx;
		it->second.texCoords.y = node->posy;
		it->second.texCoords.z = node->posx + node->size; //FIXME
		it->second.texCoords.w = node->posy + node->size;
	}

	atlasSize.x = 2048;
	atlasSize.y = 2048;
}
