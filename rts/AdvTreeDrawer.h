// TreeDrawer.h: interface for the CTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADVTREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_)
#define AFX_ADVTREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "basetreedrawer.h"
#include <map>

class CVertexArray;
class CGrassDrawer;

class CAdvTreeDrawer : public CBaseTreeDrawer
{
public:
	CAdvTreeDrawer();
	virtual ~CAdvTreeDrawer();

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
	};

	TreeSquareStruct* trees;
	int treesX;
	int treesY;

	CGrassDrawer* grassDrawer;

	struct FallingTree {
		int type;
		float3 pos;
		float3 dir;
		float speed;
		float fallPos;
	};

	std::list<FallingTree> fallingTrees;
};

#endif // !defined(AFX_TREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_)
