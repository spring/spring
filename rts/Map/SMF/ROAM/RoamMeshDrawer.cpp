/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RoamMeshDrawer.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h" //FIXME?
#include "System/Log/ILog.h"


CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: smfReadMap(rm)
	, smfGroundDrawer(gd)
	, landscape(gd, rm->GetCornerHeightMapSynced(), gs->mapx, gs->mapy) //FIXME use GetCornerHeightMapUnsynced()
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
	//FIXME this retessellate with the current camera frustum, shadow pass ad others don't have to see the same patches!
	
	//const CCamera* cam = (inShadowPass)? camera: cam2;
	const CCamera* cam = cam2;
	bool retessellate = false;

	{ SCOPED_TIMER("ROAM::Visibility");
		for (int i = 0; i < (numBigTexX * numBigTexY); i++) {
			Patch& p = landscape.m_Patches[i];
			p.UpdateVisibility();
			/*if (p.IsVisible() != visibilitygrid[i]) {
				visibilitygrid[i] = p.IsVisible();
				retessellate = true;
			}
			if (p.IsVisible() && p.IsDirty()) {
				p.ComputeVariance();
				retessellate = true;
			}*/
			if (p.IsVisible()) {
				if (!visibilitygrid[i]) {
					visibilitygrid[i] = true;
					retessellate = true;
				}
				if (p.IsDirty()) {
					//FIXME don't retessellate on small heightmap changes?
					p.ComputeVariance();
					retessellate = true;
				}
			} else {
				visibilitygrid[i] = false;
			}
		}
	}

	//static const float maxCamDeltaDistSq = 500.0f * 500.0f;
	//retessellate |= ((cam->pos - lastCamPos).SqLength() > maxCamDeltaDistSq);
	//const float cd = (cam->pos - lastCamPos).SqLength();

	retessellate |= (lastViewRadius != smfGroundDrawer->viewRadius);

	if (retessellate) {
		lastViewRadius = smfGroundDrawer->viewRadius;
		lastCamPos = cam->pos;
		
		{ SCOPED_TIMER("ROAM::Tessellate");
			//FIXME this tessellate with current camera + viewRadius
			//  so it doesn't retessellate patches that are e.g. only vis. in the shadow frustum
			landscape.Reset();
			landscape.Tessellate(cam->pos, smfGroundDrawer->viewRadius);
		}

		{ SCOPED_TIMER("ROAM::Render");
			for (std::vector<Patch>::iterator it = landscape.m_Patches.begin(); it != landscape.m_Patches.end(); it++) {
				if (it->IsVisible()) {
					it->Render();
				}
			}
		}

		/*{
			int tricount = 0;
			for (std::vector<Patch>::iterator it = landscape.m_Patches.begin(); it != landscape.m_Patches.end(); it++) {
				if (it->IsVisible()) {
					tricount += it->GetTriCount();
				}
			}
		
			LOG("ROAM dbg: Framechange, fram=%i tris=%i, viewrad=%i, cd=%f, camera=(%5.0f, %5.0f, %5.0f) camera2=  (%5.0f, %5.0f, %5.0f)",
				globalRendering->drawFrame,
				tricount,
				smfGroundDrawer->viewRadius,
				cd,
				camera->pos.x,
				camera->pos.y,
				camera->pos.z,
				cam2->pos.x,
				cam2->pos.y,
				cam2->pos.z
				);
		}*/
	}
}

void CRoamMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	const bool inShadowPass = (drawPass == DrawPass::Shadow);

	//FIXME this just updates the visibilty.
	//  It doesn't update the tessellation, neither does it send the indices to the GPU.
	//  So it patches that weren't updated the last tessellation, just uses the last
	//  tessellation which may created with a totally different camera.
	for (int i = 0; i < (numBigTexX * numBigTexY); i++){
		Patch& p = landscape.m_Patches[i];
		p.UpdateVisibility();
	}

	const int tricount = landscape.Render(inShadowPass);
}
