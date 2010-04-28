/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BaseTreeDrawer.h"
#include "BasicTreeDrawer.h"
#include "AdvTreeDrawer.h"
#include "Game/Camera.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/ConfigHandler.h"

CBaseTreeDrawer* treeDrawer = 0;

CBaseTreeDrawer::CBaseTreeDrawer(void)
{
	drawTrees = true;
	baseTreeDistance = configHandler->Get("TreeRadius", (unsigned int) (5.5f * 256)) / 256.0f;
}

CBaseTreeDrawer::~CBaseTreeDrawer(void) {
	configHandler->Set("TreeRadius", (unsigned int) (baseTreeDistance * 256));
}



static void AddTrees(CBaseTreeDrawer* td)
{
	for (int fID = 0; /* no test*/; fID++) {
		const CFeature* f = featureHandler->GetFeature(fID);

		if (f) {
			if (f->def->drawType >= DRAWTYPE_TREE) {
				td->AddTree(f->def->drawType - 1, f->pos, 1.0f);
			}
		} else {
			break;
		}
	}
}

CBaseTreeDrawer* CBaseTreeDrawer::GetTreeDrawer(void)
{
	CBaseTreeDrawer* td = NULL;

	if (GLEW_ARB_vertex_program && configHandler->Get("3DTrees", 1)) {
		GLint maxTexel;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &maxTexel);

		if (maxTexel >= 4) {
			td = new CAdvTreeDrawer();
		}
	}

	if (td == NULL) {
		td = new CBasicTreeDrawer();
	}

	AddTrees(td);
	return td;
}



int CBaseTreeDrawer::AddFallingTree(float3 pos, float3 dir, int type)
{
	return 0;
}

void CBaseTreeDrawer::DrawShadowPass(void)
{
}

void CBaseTreeDrawer::Draw (bool drawReflection)
{
	float zoom = 45.0f / camera->GetFov();
	float treeDistance=baseTreeDistance*fastmath::apxsqrt(zoom);
	treeDistance = std::max(1.0f, std::min(treeDistance, (float)MAX_VIEW_RANGE / (SQUARE_SIZE * TREE_SQUARE_SIZE)));

	Draw (treeDistance, drawReflection);
}

void CBaseTreeDrawer::Update() {

	GML_STDMUTEX_LOCK(tree); // Update

	for(std::vector<GLuint>::iterator i=delDispLists.begin(); i!=delDispLists.end(); ++i)
		glDeleteLists(*i, 1);
	delDispLists.clear();
}

