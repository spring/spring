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
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#ifdef DRAW_DEBUG_IN_MINIMAP
	#include "Game/UI/MiniMap.h"
#endif

#include <cmath>


// ---------------------------------------------------------------------
// Log Section
//
#define LOG_SECTION_ROAM "RoamMeshDrawer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ROAM)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_ROAM


// ---------------------------------------------------------------------
// Definition of the static member variables
//
bool CRoamMeshDrawer::forceRetessellate = false;


// ---------------------------------------------------------------------
// Ctor
//
CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfReadMap(rm)
	, smfGroundDrawer(gd)
	, lastGroundDetail(0)
	, visibilitygrid(NULL)
{
	eventHandler.AddClient(this);

	// Set ROAM upload mode (VA,DL,VBO)
	const int mode = configHandler->GetInt("ROAM");
	Patch::SwitchRenderMode(mode);

	numPatchesX = gs->mapx / PATCH_SIZE;
	numPatchesY = gs->mapy / PATCH_SIZE;
	//assert((numPatchesX == smfReadMap->numBigTexX) && (numPatchesY == smfReadMap->numBigTexY));

	//m_Patches.reserve(numPatchesX * numPatchesY);
	m_Patches.resize(numPatchesX * numPatchesY);

	// Initialize all terrain patches
	for (int Y = 0; Y < numPatchesY; ++Y) {
		for (int X = 0; X < numPatchesX; ++X) {
			/*Patch* patch = new Patch(
				smfGroundDrawer,
				X * PATCH_SIZE,
				Y * PATCH_SIZE);
			patch->ComputeVariance();
			m_Patches.push_back(patch);*/

			Patch& patch = m_Patches[Y * numPatchesX + X];
			patch.Init(
					smfGroundDrawer,
					X * PATCH_SIZE,
					Y * PATCH_SIZE);
			patch.ComputeVariance();
		}
	}

	visibilitygrid = new bool[numPatchesX * numPatchesY];
	for (int i = 0; i < (numPatchesX * numPatchesY); ++i){
		visibilitygrid[i] = false;
	}

	CTriNodePool::InitPools();
}

CRoamMeshDrawer::~CRoamMeshDrawer()
{
	configHandler->Set("ROAM", (int)Patch::renderMode);

	delete[] visibilitygrid;

	CTriNodePool::FreePools();
}


/**
 * Retessellates the current terrain
 */
void CRoamMeshDrawer::Update()
{
	//FIXME this retessellates with the current camera frustum, shadow pass and others don't have to see the same patches!

	//const CCamera* cam = (inShadowPass)? camera: cam2;
	CCamera* cam = cam2;

	// Update Patch visibility
	Patch::UpdateVisibility(cam, m_Patches, numPatchesX);

	// Check if a retessellation is needed
#define RETESSELLATE_MODE 1
	bool retessellate = false;
	{ SCOPED_TIMER("ROAM::ComputeVariance");
		for (int i = 0; i < (numPatchesX * numPatchesY); ++i) { //FIXME multithread?
			Patch& p = m_Patches[i];
		#if (RETESSELLATE_MODE == 2)
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
		#else
			if (p.IsVisible() != visibilitygrid[i]) {
				visibilitygrid[i] = p.IsVisible();
				retessellate = true;
			}
			if (p.IsVisible() && p.IsDirty()) {
				//FIXME don't retessellate on small heightmap changes?
				p.ComputeVariance();
				retessellate = true;
			}
		#endif
		}
	}

	// Further conditions that can cause a retessellation
#if (RETESSELLATE_MODE == 2)
	static const float maxCamDeltaDistSq = 500.0f * 500.0f;
	retessellate |= ((cam->GetPos() - lastCamPos).SqLength() > maxCamDeltaDistSq);
#endif
	retessellate |= forceRetessellate;
	retessellate |= (lastGroundDetail != smfGroundDrawer->GetGroundDetail());

	bool retessellateAgain = false;

	// Retessellate
	if (retessellate) {
		{ SCOPED_TIMER("ROAM::Tessellate");
			//FIXME this tessellates with current camera + viewRadius
			//  so it doesn't retessellate patches that are e.g. only vis. in the shadow frustum
			Reset();
			retessellateAgain = Tessellate(cam->GetPos(), smfGroundDrawer->GetGroundDetail());
		}

		{ SCOPED_TIMER("ROAM::GenerateIndexArray");
			for_mt(0, m_Patches.size(), [&](const int i){
				Patch* it = &m_Patches[i];
				if (it->IsVisible()) {
					it->GenerateIndices();
				}
			});
		}

		{ SCOPED_TIMER("ROAM::Upload");
			for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); ++it) {
				if (it->IsVisible()) {
					it->Upload();
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

			LOG_L(L_DEBUG, "ROAM dbg: Framechange, fram=%i tris=%i, viewrad=%i, cd=%f, camera=(%5.0f, %5.0f, %5.0f) camera2=  (%5.0f, %5.0f, %5.0f)",
				globalRendering->drawFrame,
				tricount,
				smfGroundDrawer->viewRadius,
				(cam->pos - lastCamPos).SqLength();,
				camera->GetPos().x,
				camera->GetPos().y,
				camera->GetPos().z,
				cam2->pos.x,
				cam2->pos.y,
				cam2->pos.z
				);
		}*/

		lastGroundDetail = smfGroundDrawer->GetGroundDetail();
		lastCamPos = cam->GetPos();
		forceRetessellate = retessellateAgain;
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
	CCamera* cam = (inShadowPass)? camera: cam2;
	Patch::UpdateVisibility(cam, m_Patches, numPatchesX);

	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); ++it) {
		if (it->IsVisible()) {
			if (!inShadowPass)
				it->SetSquareTexture();

			it->Draw();
		}
	}
}

void CRoamMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass)
{
	const bool inShadowPass = (drawPass == DrawPass::Shadow);

	for (int Y = 0; Y < numPatchesY; ++Y) {
		for (int X = 0; X < numPatchesX; ++X) {
			if (X % (numPatchesX - 1) == 0 || Y % (numPatchesY - 1) == 0) {
				Patch& p = m_Patches[Y * numPatchesX + X];
				if (p.IsVisible()) {
					if (!inShadowPass)
						p.SetSquareTexture();

					p.DrawBorder();
				}
			}
		}
	}
}

#ifdef DRAW_DEBUG_IN_MINIMAP
void CRoamMeshDrawer::DrawInMiniMap()
{
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0, -1.0);
		glTranslatef((float)minimap->GetPosX() * globalRendering->pixelX, (float)minimap->GetPosY() * globalRendering->pixelY, 0.0f);
		glScalef((float)minimap->GetSizeX() * globalRendering->pixelX, (float)minimap->GetSizeY() * globalRendering->pixelY, 1.0f);
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.0f, 1.0f, 0.0f);
		glScalef(1.0f / gs->mapx, -1.0f / gs->mapy, 1.0f);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); ++it) {
		if (!it->IsVisible()) {
			glRectf(it->m_WorldX, it->m_WorldY, it->m_WorldX + PATCH_SIZE, it->m_WorldY + PATCH_SIZE);
		}
	}

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
}
#endif


// ---------------------------------------------------------------------
// Reset all patches, recompute variance if needed
//
void CRoamMeshDrawer::Reset()
{
	// Set the next free triangle pointer back to the beginning
	CTriNodePool::ResetAll();

	// Go through the patches performing resets, compute variances, and linking.
	for (int Y = 0; Y < numPatchesY; ++Y) {
		for (int X = 0; X < numPatchesX; ++X) {
			Patch& patch = m_Patches[Y * numPatchesX + X];

			// Reset the patch
			patch.Reset();

			// Link all the patches together. (leave borders NULL)
			if (X > 0)
				patch.GetBaseLeft()->LeftNeighbor = m_Patches[Y * numPatchesX + X - 1].GetBaseRight();

			if (X < (numPatchesX - 1))
				patch.GetBaseRight()->LeftNeighbor = m_Patches[Y * numPatchesX + X + 1].GetBaseLeft();

			if (Y > 0)
				patch.GetBaseLeft()->RightNeighbor = m_Patches[(Y - 1) * numPatchesX + X].GetBaseRight();

			if (Y < (numPatchesY - 1))
				patch.GetBaseRight()->RightNeighbor = m_Patches[(Y + 1) * numPatchesX + X].GetBaseLeft();
		}
	}
}

// ---------------------------------------------------------------------
// Create an approximate mesh of the landscape.
//
bool CRoamMeshDrawer::Tessellate(const float3& campos, int viewradius)
{
	// Perform Tessellation
	// hint: threading just helps a little with huge cpu usage in retessellation, still better than nothing

	//  _____
	// |0|_|_|..
	// |_|_|_|..
	// |_|_|8|..
	//  .....
	// split the patches in 3x3 sized blocks. The tessellation itself can
	// extend into the neighbor patches (check Patch::Split). So we could
	// not multi-thread the whole loop w/o mutexes (in ::Split).
	// But instead we take a safety distance between the thread's working
	// area (which is 2 patches), so they don't conflict with each other.
	bool forceRetessellate = false;
	for (int idx = 0; idx < 9; ++idx) {
		for_mt(0, m_Patches.size(), [&](const int i){
			Patch* it = &m_Patches[i];

			const int X = it->m_WorldX;
			const int Z = it->m_WorldY;
			const int subindex = (X % 3) + (Z % 3) * 3;

			if ((subindex == idx) && it->IsVisible()) {
				if (!it->Tessellate(campos, viewradius))
					forceRetessellate = true;
			}
		});
		if (forceRetessellate)
			return true;
	}

	return false;
}


// ---------------------------------------------------------------------
// UnsyncedHeightMapUpdate event
//
void CRoamMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	const int margin = 2;
	const float INV_PATCH_SIZE = 1.0f / PATCH_SIZE;

	// hint: the -+1 are cause Patches share 1 pixel border (no vertex holes!)
	const int xstart = std::max(0,           (int)math::floor((rect.x1 - margin) * INV_PATCH_SIZE));
	const int xend   = std::min(numPatchesX, (int)math::ceil( (rect.x2 + margin) * INV_PATCH_SIZE));
	const int zstart = std::max(0,           (int)math::floor((rect.z1 - margin) * INV_PATCH_SIZE));
	const int zend   = std::min(numPatchesY, (int)math::ceil( (rect.z2 + margin) * INV_PATCH_SIZE));

	for (int z = zstart; z < zend; ++z) {
		for (int x = xstart; x < xend; ++x) {
			Patch& p = m_Patches[z * numPatchesX + x];

			// clamp the update rect to the patch constraints
			SRectangle prect(
				std::max(rect.x1 - margin - p.m_WorldX, 0),
				std::max(rect.z1 - margin - p.m_WorldY, 0),
				std::min(rect.x2 + margin - p.m_WorldX, PATCH_SIZE),
				std::min(rect.z2 + margin - p.m_WorldY, PATCH_SIZE)
			);

			p.UpdateHeightMap(prect);
		}
	}

	LOG_L(L_DEBUG, "UnsyncedHeightMapUpdate, fram=%i, numpatches=%i, xi1=%i xi2=%i zi1=%i zi2=%i, x1=%i x2=%i z1=%i z2=%i",
		globalRendering->drawFrame,
		(xend - xstart) * (zend - zstart),
		xstart, xend, zstart, zend,
		rect.x1, rect.x2, rect.z1, rect.z2
   	);
}
