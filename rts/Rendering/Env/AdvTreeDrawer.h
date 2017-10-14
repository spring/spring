/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_DRAWER_H_
#define _ADV_TREE_DRAWER_H_

#include "ITreeDrawer.h"
#include "AdvTreeGenerator.h"
#include <array>

class CCamera;

namespace Shader {
	struct IProgramObject;
}

class CAdvTreeDrawer: public ITreeDrawer
{
public:
	CAdvTreeDrawer();
	~CAdvTreeDrawer();

	void LoadTreeShaders();
	void SetupDrawState();
	void SetupDrawState(const CCamera* cam, Shader::IProgramObject* ipo);
	void ResetDrawState();
	void SetupShadowDrawState();
	void SetupShadowDrawState(const CCamera* cam, Shader::IProgramObject* ipo);
	void ResetShadowDrawState();
	void DrawPass() override;
	void DrawTree(const TreeStruct& ts, int posOffsetIdx);
	void DrawTree(const float3& pos, int treeType, int posOffsetIdx);
	void Update() override;
	void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) override;
	void DrawShadowPass() override;

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
		TREE_PROGRAM_BASIC  = 0, // shader (V) without self-shadowing
		TREE_PROGRAM_SHADOW = 1, // shader (V+F) with self-shadowing
		TREE_PROGRAM_ACTIVE = 2,
		TREE_PROGRAM_LAST   = 3
	};

	std::array<Shader::IProgramObject*, TREE_PROGRAM_LAST> treeShaders;
	std::vector<FallingTree> fallingTrees;

	CAdvTreeGenerator treeGen;
};

#endif // _ADV_TREE_DRAWER_H_

