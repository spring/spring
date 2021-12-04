/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_DRAWER_H_
#define _ADV_TREE_DRAWER_H_

#include <array>

#include "ITreeDrawer.h"
#include "AdvTreeGenerator.h"
#include "System/type2.h"

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
	void DrawTrees(const CCamera* cam, Shader::IProgramObject* ipo);
	void DrawFallingTrees(const CCamera* cam, Shader::IProgramObject* ipo) const;

	void Update() override;
	void DrawPass() override;
	void DrawShadowPass() override;

	void DrawTree(const TreeStruct& ts, int treeMatIdx);
	void DrawTree(const float3& pos, int treeType, int treeMatIdx);
	void BindTreeGeometry(int treeType) const;
	void DrawTreeGeometry(int treeType) const;

	void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) override;

	struct FallingTree {
		int id;
		int type;

		CMatrix44f fallMat;
		float3 fallDir;

		float fallSpeed;
		float fallAngle;
	};

	static constexpr int TREE_MAT_IDX = 14;
	static constexpr int VIEW_MAT_IDX = 15;
	static constexpr int PROJ_MAT_IDX = 16;

private:
	enum TreeShaderProgram {
		TREE_PROGRAM_BASIC  = 0, // shader (V) without self-shadowing
		TREE_PROGRAM_SHADOW = 1, // shader (V+F) with self-shadowing
		TREE_PROGRAM_ACTIVE = 2,
		TREE_PROGRAM_LAST   = 3
	};

	std::array<Shader::IProgramObject*, TREE_PROGRAM_LAST> treeShaders;
	std::vector<FallingTree> fallingTrees[2];

	CAdvTreeGenerator treeGen;

	float3 prvUpdateCamPos;
	float3 prvUpdateCamDir;

	bool updateVisibility = false;
};

#endif // _ADV_TREE_DRAWER_H_

