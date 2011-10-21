/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RoamMeshDrawer.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "System/Config/ConfigHandler.h" //FIXME?
#include "System/Log/ILog.h"


CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: smfReadMap(rm)
	, smfGroundDrawer(gd)
	, landscape(smfGroundDrawer, readmap->GetCornerHeightMapSynced(), gs->mapx, gs->mapy) //FIXME use GetCornerHeightMapUnsynced()
	, visibilitygrid(NULL)
{
	//viewRadius = configHandler->GetInt("GroundDetail");
	//viewRadius += (viewRadius & 1); //! we need a multiple of 2

	numBigTexX = smfReadMap->numBigTexX;
	numBigTexY = smfReadMap->numBigTexY;

	visibilitygrid = new bool[smfReadMap->numBigTexX * smfReadMap->numBigTexY];
	
	for (int i = 0; i < (numBigTexX * numBigTexY); i++){
		visibilitygrid[i] = false;
	}
}


CRoamMeshDrawer::~CRoamMeshDrawer()
{
	delete[] visibilitygrid;
}


void CRoamMeshDrawer::Update()
{
}

void CRoamMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	const bool inShadowPass        = (drawPass == DrawPass::Shadow);
	const bool drawWaterReflection = (drawPass == DrawPass::WaterReflection);
	const bool drawUnitReflection  = (drawPass == DrawPass::UnitReflection);

	const CCamera* cam = (inShadowPass)? camera: cam2;

	bool camMoved = false;

	for (int i = 0; i < (numBigTexX * numBigTexY); i++){
		Patch& p = landscape.m_Patches[i];
		p.UpdateVisibility();
		if ((bool)p.isVisibile() != visibilitygrid[i] && !drawWaterReflection && !drawUnitReflection) {
			visibilitygrid[i] = (bool)p.isVisibile();
			camMoved = true;
		}
	}

	camMoved |= (landscape.updateCount > 0);
	//camMoved = ((cam - lastCamPos).SqLength() > maxCamDeltaDistSq);

	// This check is needed, because the shadows are rendered first, and if we
	// update terrain in same frame, we will get flashes of shadows on updates
	// So if shadows are on, we must put the terrain update part in the shadows part.
	const bool shadowsEnabled = (shadowHandler->shadowsLoaded && !smfGroundDrawer->DrawExtraTex());
	if (inShadowPass || !shadowsEnabled) { 
		if (camMoved) {
			landscape.Reset();
			landscape.Tessellate(cam->pos, smfGroundDrawer->viewRadius);
		}
	}

	const int tricount = landscape.Render(camMoved, inShadowPass, drawWaterReflection);

	/*LOG("ROAM dbg: Framechange, tris=%i, viewrad=%i, inshadowpass=%i, camera=(%5.0f, %5.0f, %5.0f) camera2=  (%5.0f, %5.0f, %5.0f)",
		tricount,
		viewRadius,
		inShadowPass,
		camera->pos.x,
		camera->pos.y,
		camera->pos.z,
		cam2->pos.x,
		cam2->pos.y,
		cam2->pos.z
		);*/
}
