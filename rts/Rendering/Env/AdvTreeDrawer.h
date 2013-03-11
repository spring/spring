/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_DRAWER_H_
#define _ADV_TREE_DRAWER_H_

#include <map>
#include <list>
#include "ITreeDrawer.h"

class CVertexArray;
class CGrassDrawer;

namespace Shader {
	struct IProgramObject;
}

class CAdvTreeDrawer: public ITreeDrawer
{
public:
	CAdvTreeDrawer();
	virtual ~CAdvTreeDrawer();

	void LoadTreeShaders();
	void Draw(float treeDistance, bool drawReflection);
	void Update();
	void ResetPos(const float3& pos);
	void AddTree(int type, const float3& pos, float size);
	void DeleteTree(const float3& pos);
	void AddFallingTree(const float3& pos, const float3& dir, int type);
	void DrawGrass();
	void DrawShadowGrass();
	void AddGrass(const float3& pos);
	void RemoveGrass(int x, int z);
	void DrawShadowPass();

	static void DrawTreeVertexA(CVertexArray* va, float3& ftpos, float dx, float dy);
	static void DrawTreeVertex(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexMid(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexFar(CVertexArray* va, const float3& pos, const float3& swd, float dx, float dy, bool enlarge = true);

	struct TreeStruct {
		float3 pos;
		int type;
	};
	struct FadeTree {
		float3 pos;
		float relDist;
		float deltaY;
		int type;
	};
	struct FallingTree {
		int type;
		float3 pos;
		float3 dir;
		float speed;
		float fallPos;
	};

	struct TreeSquareStruct {
		unsigned int dispList;
		unsigned int farDispList;
		int lastSeen;
		int lastSeenFar;
		float3 viewVector;
		std::map<int, TreeStruct> trees;
	};

	int lastListClean;
	float oldTreeDistance;

	int treesX;
	int treesY;
	int nTrees;

	TreeSquareStruct* trees;

private:
	enum TreeShaderProgram {
		TREE_PROGRAM_NEAR_BASIC  = 0, // near-tree shader (V) without self-shadowing
		TREE_PROGRAM_NEAR_SHADOW = 1, // near-tree shader (V+F) with self-shadowing
		TREE_PROGRAM_DIST_SHADOW = 2, // far-tree shader (V+F) with self-shadowing
		TREE_PROGRAM_LAST        = 3
	};

	std::vector<Shader::IProgramObject*> treeShaders;
	std::list<FallingTree> fallingTrees;

	CGrassDrawer* grassDrawer;
};

#endif // _ADV_TREE_DRAWER_H_
