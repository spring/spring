#include "StdAfx.h"
#include "BaseTreeDrawer.h"
#include "BasicTreeDrawer.h"
#include "AdvTreeDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Platform/ConfigHandler.h"
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
	if(GLEW_ARB_vertex_program && configHandler.GetInt("3DTrees",1)){
		int maxTexel;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&maxTexel);
		if(maxTexel>=4){
			td=new CAdvTreeDrawer;
			return td;
		}
	}
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
