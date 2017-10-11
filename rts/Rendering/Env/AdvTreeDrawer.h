/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_DRAWER_H_
#define _ADV_TREE_DRAWER_H_

#include "ITreeDrawer.h"
#include "AdvTreeGenerator.h"

class CVertexArray;

namespace Shader {
	struct IProgramObject;
}

class CAdvTreeDrawer: public ITreeDrawer
{
public:
	CAdvTreeDrawer();
	~CAdvTreeDrawer();

	void LoadTreeShaders();
	void DrawPass() override;
	void Update() override;
	void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) override;
	void DrawShadowPass() override;

	static void DrawTreeVertexA(CVertexArray* va, float3& ftpos, float dx, float dy);
	static void DrawTreeVertex(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexMid(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true);
	static void DrawTreeVertexFar(CVertexArray* va, const float3& pos, const float3& swd, float dx, float dy, bool enlarge = true);

	struct FallingTree {
		int id;
		int type;
		float3 pos;
		float3 dir;
		float speed;
		float fallPos;
	};

private:
	enum TreeShaderProgram {
		TREE_PROGRAM_BASIC  = 0, // near-tree shader (V) without self-shadowing
		TREE_PROGRAM_SHADOW = 1, // near-tree shader (V+F) with self-shadowing
		TREE_PROGRAM_LAST   = 2
	};

	std::array<Shader::IProgramObject*, TREE_PROGRAM_LAST> treeShaders;
	std::vector<FallingTree> fallingTrees;

	CAdvTreeGenerator treeGen;

	int lastListClean;
};

#endif // _ADV_TREE_DRAWER_H_

