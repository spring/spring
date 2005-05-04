// TreeDrawer.h: interface for the CTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_)
#define AFX_TREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "basetreedrawer.h"
#include <map>

#define MAX_TREE_HEIGHT 60

class CBasicTreeDrawer : public CBaseTreeDrawer
{
public:
	CBasicTreeDrawer();
	virtual ~CBasicTreeDrawer();

	void Draw(float treeDistance,bool drawReflection);
	void Update();
	void CreateTreeTex(unsigned int& texnum,unsigned char* data,int xsize,int ysize);
	void AddTree(int type, float3 pos, float size);
	void DeleteTree(float3 pos);

	unsigned int treetex;
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
	};

	TreeSquareStruct* trees;
	int treesX;
	int treesY;

	void ResetPos(const float3& pos);
};

#endif // !defined(AFX_TREEDRAWER_H__9E1F499C_815E_4F58_AD8B_0D3B04C73C4A__INCLUDED_)
