/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASIC_TREE_DRAWER_H__
#define __BASIC_TREE_DRAWER_H__

#include <map>
#include "BaseTreeDrawer.h"
#include "Rendering/GL/myGL.h"

#define MAX_TREE_HEIGHT 60

class CBasicTreeDrawer : public CBaseTreeDrawer
{
public:
	CBasicTreeDrawer();
	virtual ~CBasicTreeDrawer();

	void Draw(float treeDistance,bool drawReflection);
	void Update();
	void CreateTreeTex(GLuint& texnum,unsigned char* data,int xsize,int ysize);
	void AddTree(int type, float3 pos, float size);
	void DeleteTree(float3 pos);

	GLuint treetex;
	int lastListClean;

	struct TreeStruct{
		float3 pos;
		int type;
	};

	struct TreeSquareStruct {
		unsigned int displist;
		unsigned int farDisplist;
		int lastSeen;
		int lastSeenFar;
		float3 viewVector;
		std::map<int,TreeStruct> trees;
		TreeSquareStruct() : displist(0), farDisplist(0), lastSeen(0), lastSeenFar(0) {}
	};

	TreeSquareStruct* trees;
	int treesX;
	int treesY;
	int nTrees;

	void ResetPos(const float3& pos);
};

#endif // __BASIC_TREE_DRAWER_H__
