#ifndef __BASE_TREE_DRAWER_H__
#define __BASE_TREE_DRAWER_H__

#include "Rendering/GL/myGL.h"
#include "float3.h"

#define TREE_SQUARE_SIZE 64

class CBaseTreeDrawer
{
public:
	CBaseTreeDrawer(void);
	virtual ~CBaseTreeDrawer(void);

	void Draw(bool drawReflection);
	virtual void Draw(float treeDistance,bool drawReflection)=0;
	virtual void DrawGrass(){};
	virtual void Update()=0;
	virtual void ResetPos(const float3& pos)=0;
	static CBaseTreeDrawer* GetTreeDrawer(void);
	virtual void AddTree(int type, float3 pos, float size)=0;
	virtual void DeleteTree(float3 pos)=0;

	std::vector<GLuint> delDispLists;
	virtual void UpdateDraw();

	float baseTreeDistance;
	bool drawTrees;

	virtual int AddFallingTree(float3 pos, float3 dir, int type);
	virtual void AddGrass(float3 pos){};
	virtual void RemoveGrass(int x, int z){};
	virtual void DrawShadowPass(void);
};

extern CBaseTreeDrawer* treeDrawer;

#endif // __BASE_TREE_DRAWER__
