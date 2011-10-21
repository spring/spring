/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// ROAM Simplistic Implementation
// Added to Spring by Peter Sarkozy (mysterme AT gmail DOT com)
// Billion thanks to Bryan Turner (Jan, 2000)
//                    brturn@bellsouth.net
//
// Based on the Tread Marks engine by Longbow Digital Arts
//                               (www.LongbowDigitalArts.com)
// Much help and hints provided by Seumas McNally, LDA.


#include "RoamMeshDrawer.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"
//#include "System/OpenMP_cond.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h" //FIXME?
#include "System/Log/ILog.h"

#include <cmath>

// ---------------------------------------------------------------------
// Definition of the static member variables
//
int CRoamMeshDrawer::m_NextTriNode;
TriTreeNode CRoamMeshDrawer::m_TriPool[POOL_SIZE];
bool CRoamMeshDrawer::forceRetessellate = false;


// ---------------------------------------------------------------------
// Ctor
//
CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfReadMap(rm)
	, smfGroundDrawer(gd)
	, visibilitygrid(NULL)
{
	eventHandler.AddClient(this);

	//viewRadius = configHandler->GetInt("GroundDetail");
	//viewRadius += (viewRadius & 1); // we need a multiple of 2

	numBigTexX = smfReadMap->numBigTexX;
	numBigTexY = smfReadMap->numBigTexY;
	h = gs->mapy;
	w = gs->mapx;

	//m_Patches.reserve((w / PATCH_SIZE) * (h / PATCH_SIZE));
	m_Patches.resize((w / PATCH_SIZE) * (h / PATCH_SIZE));

	const float* hmap = rm->GetCornerHeightMapSynced(); //FIXME use GetCornerHeightMapUnsynced()

	// Initialize all terrain patches
	for (int Y = 0; Y < h / PATCH_SIZE; Y++) {
		for (int X = 0; X < w / PATCH_SIZE; X++) {
			/*Patch* patch = new Patch(
				smfGroundDrawer,
				X * PATCH_SIZE,
				Y * PATCH_SIZE,
				hMap,
				w);
			patch->ComputeVariance();
			m_Patches.push_back(patch);*/

			Patch& patch = m_Patches[Y * (w / PATCH_SIZE) + X];
			patch.Init(
					smfGroundDrawer,
					X * PATCH_SIZE,
					Y * PATCH_SIZE,
					hmap,
					w);
			patch.ComputeVariance();
		}
	}

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
	//FIXME this retessellates with the current camera frustum, shadow pass and others don't have to see the same patches!
	
	//const CCamera* cam = (inShadowPass)? camera: cam2;
	const CCamera* cam = cam2;
	bool retessellate = false;

	{ SCOPED_TIMER("ROAM::Visibility");
		for (int i = 0; i < (numBigTexX * numBigTexY); i++) {
			Patch& p = m_Patches[i];
			p.UpdateVisibility();
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
			//FIXME this tessellates with current camera + viewRadius
			//  so it doesn't retessellate patches that are e.g. only vis. in the shadow frustum
			Reset();
			Tessellate(cam->pos, smfGroundDrawer->viewRadius);
		}

		{ SCOPED_TIMER("ROAM::Render");
			for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
				if (it->IsVisible()) {
					it->Render();
				}
			}
		}

		/*{
			int tricount = 0;
			for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
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


// ---------------------------------------------------------------------
// Render Mesh
//
void CRoamMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	const bool inShadowPass = (drawPass == DrawPass::Shadow);

	//FIXME this just updates the visibilty.
	//  It doesn't update the tessellation, neither does it send the indices to the GPU.
	//  So it patches that weren't updated the last tessellation, just uses the last
	//  tessellation which may created with a totally different camera.
	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		it->UpdateVisibility();
	}

	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		if (it->IsVisible()) {
			if (!inShadowPass)
				it->SetSquareTexture();

			it->DrawTriArray();
		}
	}
}


// ---------------------------------------------------------------------
// Allocate a TriTreeNode from the pool.
//
TriTreeNode* CRoamMeshDrawer::AllocateTri()
{
	// IF we've run out of TriTreeNodes, just return NULL (this is handled gracefully)
	if (m_NextTriNode >= POOL_SIZE)
		return NULL;

	TriTreeNode* pTri = &(m_TriPool[m_NextTriNode++]);
	pTri->LeftChild = pTri->RightChild = NULL;
	return pTri;
}


// ---------------------------------------------------------------------
// Reset all patches, recompute variance if needed
//
void CRoamMeshDrawer::Reset()
{
	// Set the next free triangle pointer back to the beginning
	SetNextTriNode(0);

	// Go through the patches performing resets, compute variances, and linking.
	for (int Y = 0; Y < h / PATCH_SIZE; Y++)
		for (int X = 0; X < w / PATCH_SIZE; X++) {
			Patch& patch = m_Patches[Y * (w / PATCH_SIZE) + X];

			// Reset the patch
			patch.Reset();
			patch.UpdateVisibility();

			// Link all the patches together.
			if (X > 0)
				patch.GetBaseLeft()->LeftNeighbor = m_Patches[Y * (w
						/ PATCH_SIZE) + X - 1].GetBaseRight();
			else
				patch.GetBaseLeft()->LeftNeighbor = NULL; // Link to bordering Landscape here..

			if (X < ((w / PATCH_SIZE) - 1))
				patch.GetBaseRight()->LeftNeighbor = m_Patches[Y * (w
						/ PATCH_SIZE) + X + 1].GetBaseLeft();
			else
				patch.GetBaseRight()->LeftNeighbor = NULL; // Link to bordering Landscape here..

			if (Y > 0)
				patch.GetBaseLeft()->RightNeighbor = m_Patches[(Y - 1)
						* (w / PATCH_SIZE) + X].GetBaseRight();
			else
				patch.GetBaseLeft()->RightNeighbor = NULL; // Link to bordering Landscape here..

			if (Y < ((h / PATCH_SIZE) - 1))
				patch.GetBaseRight()->RightNeighbor = m_Patches[(Y + 1)
						* (w / PATCH_SIZE) + X].GetBaseLeft();
			else
				patch.GetBaseRight()->RightNeighbor = NULL; // Link to bordering Landscape here..
		}

}

// ---------------------------------------------------------------------
// Create an approximate mesh of the landscape.
//
void CRoamMeshDrawer::Tessellate(const float3& campos, int viewradius)
{
	// Perform Tessellation
	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		if (it->IsVisible())
			it->Tessellate(campos, viewradius);
	}
}


// ---------------------------------------------------------------------
// UnsyncedHeightMapUpdate event
//
void CRoamMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	const int xstart = std::floor(rect.x1 / PATCH_SIZE);
	const int xend   = std::ceil( rect.x2 / PATCH_SIZE);
	const int zstart = std::floor(rect.z1 / PATCH_SIZE);
	const int zend   = std::ceil( rect.z2 / PATCH_SIZE);

	const int pwidth = (gs->mapx / PATCH_SIZE);

	for (int z = zstart; z <= zend; z++) {
		for (int x = xstart; x <= xend; x++) {
			Patch& p = m_Patches[z * pwidth + x];
			p.UpdateHeightMap();
		}
	}

	//LOG("ROAM dbg: UnsyncedHeightMapUpdate, fram=%i", globalRendering->drawFrame);
}
