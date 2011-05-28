/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_TREE_DRAWER_H_
#define _BASIC_TREE_DRAWER_H_

#include <map>
#include "ITreeDrawer.h"
#include "Rendering/GL/myGL.h"

class CVertexArray;


// XXX This has a duplicate in AdvTreeGenerator.h
#define MAX_TREE_HEIGHT 60

class CBasicTreeDrawer : public ITreeDrawer
{
public:
	CBasicTreeDrawer();
	virtual ~CBasicTreeDrawer();

	void Draw(float treeDistance, bool drawReflection);
	void Update();
	void ResetPos(const float3& pos);
	void AddTree(int type, const float3& pos, float size);
	void DeleteTree(const float3& pos);

	struct TreeStruct {
		float3 pos;
		int type;
	};

	struct TreeSquareStruct {
		TreeSquareStruct()
			: dispList(0)
			, farDispList(0)
			, lastSeen(0)
			, lastSeenFar(0)
		{}

		unsigned int dispList;
		unsigned int farDispList;
		int lastSeen;
		int lastSeenFar;
		float3 viewVector;
		std::map<int, TreeStruct> trees;
	};

	TreeSquareStruct* trees;
	int treesX;
	int treesY;
	int nTrees;

private:
	GLuint treetex;
	int lastListClean;
};

#endif // _BASIC_TREE_DRAWER_H_
