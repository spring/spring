#include "stdafx.h"
#include "basetreedrawer.h"
#include "basictreedrawer.h"
#include "advtreedrawer.h"
#include "mygl.h"
#include "reghandler.h"
#include ".\basetreedrawer.h"
//#include "mmgr.h"

CBaseTreeDrawer* treeDrawer=0;

CBaseTreeDrawer::CBaseTreeDrawer(void)
{
		drawTrees=true;
}

CBaseTreeDrawer::~CBaseTreeDrawer(void)
{
}

CBaseTreeDrawer* CBaseTreeDrawer::GetTreeDrawer(void)
{
	CBaseTreeDrawer* td;
	if(GLEW_ARB_vertex_program && regHandler.GetInt("3DTrees",1))
		td=new CAdvTreeDrawer;
	else
		td=new CBasicTreeDrawer;
	return td;
}

int CBaseTreeDrawer::AddFallingTree(float3 pos, float3 dir, int type)
{
	return 0;
}

void CBaseTreeDrawer::DrawShadowPass(void)
{
}
