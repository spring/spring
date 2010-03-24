/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ADV_TREE_DRAWER_H__
#define __ADV_TREE_DRAWER_H__

#include <map>
#include <list>
#include "BaseTreeDrawer.h"

class CVertexArray;
class CGrassDrawer;

namespace Shader {
	struct IProgramObject;
}

class CAdvTreeDrawer: public CBaseTreeDrawer
{
public:
	CAdvTreeDrawer();
	virtual ~CAdvTreeDrawer();

	void LoadTreeShaders();
	void Draw(float treeDistance,bool drawReflection);
	void Update();
	void ResetPos(const float3& pos);
	void AddTree(int type, float3 pos, float size);
	void DeleteTree(float3 pos);
	int AddFallingTree(float3 pos, float3 dir, int type);
	void DrawGrass(void);
	void AddGrass(float3 pos);
	void RemoveGrass(int x, int z);
	void DrawShadowPass(void);

	int lastListClean;
	float oldTreeDistance;

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
		unsigned int displist;
		unsigned int farDisplist;
		int lastSeen;
		int lastSeenFar;
		float3 viewVector;
		std::map<int, TreeStruct> trees;
	};

	int treesX;
	int treesY;
	int nTrees;

	TreeSquareStruct* trees;

private:
	Shader::IProgramObject* treeNearDefShader;  // near-tree shader (V) without self-shadowing
	Shader::IProgramObject* treeNearAdvShader;  // near-tree shader (V+F) with self-shadowing
	Shader::IProgramObject* treeDistAdvShader;  // far-tree shader (V+F) with self-shadowing

	CGrassDrawer* grassDrawer;

	std::list<FallingTree> fallingTrees;
};

#endif // __ADV_TREE_DRAWER_H__
