/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_DRAWER_H_
#define _ADV_TREE_DRAWER_H_

#include "ITreeDrawer.h"

class CVertexArray;

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
	void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir);
	void DrawShadowPass();

	static void DrawTreeVertexA(CVertexArray* va, float3& ftpos, float dx, float dy);
	static void DrawTreeVertex(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexMid(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexFar(CVertexArray* va, const float3& pos, const float3& swd, float dx, float dy, bool enlarge = true);

	struct FadeTree {
		int id;
		int type;
		float3 pos;
		float relDist;
		float deltaY;
	};
	struct FallingTree {
		int id;
		int type;
		float3 pos;
		float3 dir;
		float speed;
		float fallPos;
	};

	int lastListClean;
	float oldTreeDistance;

private:
	enum TreeShaderProgram {
		TREE_PROGRAM_NEAR_BASIC  = 0, // near-tree shader (V) without self-shadowing
		TREE_PROGRAM_NEAR_SHADOW = 1, // near-tree shader (V+F) with self-shadowing
		TREE_PROGRAM_DIST_SHADOW = 2, // far-tree shader (V+F) with self-shadowing
		TREE_PROGRAM_LAST        = 3
	};

	std::vector<Shader::IProgramObject*> treeShaders;
	std::vector<FallingTree> fallingTrees;
};

#endif // _ADV_TREE_DRAWER_H_

