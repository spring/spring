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


bool CRoamMeshDrawer::forceRetessellate = false;


// ---------------------------------------------------------------------
// Ctor
//
CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfReadMap(rm)
	, smfGroundDrawer(gd)
	, lastGroundDetail(0)
{
	eventHandler.AddClient(this);

	// Set ROAM upload mode (VA,DL,VBO)
	Patch::SwitchRenderMode(configHandler->GetInt("ROAM"));

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	// assert((numPatchesX == smfReadMap->numBigTexX) && (numPatchesY == smfReadMap->numBigTexY));

	roamPatches.resize(numPatchesX * numPatchesY);
	patchVisGrid.resize(numPatchesX * numPatchesY, 0);

	// Initialize all terrain patches
	for (int Y = 0; Y < numPatchesY; ++Y) {
		for (int X = 0; X < numPatchesX; ++X) {
			Patch& patch = roamPatches[Y * numPatchesX + X];
			patch.Init(
					smfGroundDrawer,
					X * PATCH_SIZE,
					Y * PATCH_SIZE);
			patch.ComputeVariance();
		}
	}

	CTriNodePool::InitPools();
}

CRoamMeshDrawer::~CRoamMeshDrawer()
{
	configHandler->Set("ROAM", (int)Patch::renderMode);

	CTriNodePool::FreePools();
}


/**
 * Retessellates the current terrain
 */
void CRoamMeshDrawer::Update()
{
	CCamera* cam = nullptr;

	// TODO:
	//   remove the "false &&" when ShadowHandler uses its own camera
	//   otherwise this retessellates with the current camera frustum,
	//   shadow pass and others don't have to see the same patches!
	if (false && shadowHandler->inShadowPass) {
		cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
	} else {
		cam = CCamera::GetCamera(CCamera::CAMTYPE_VISCUL);
	}

	cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	// Update Patch visibility
	Patch::UpdateVisibility(cam, roamPatches, numPatchesX);

	// Check if a retessellation is needed
#define RETESSELLATE_MODE 1
	bool retessellate = false;

	{
		SCOPED_TIMER("ROAM::ComputeVariance");
		for (int i = 0; i < (numPatchesX * numPatchesY); ++i) { //FIXME multithread?
			Patch& p = roamPatches[i];
		#if (RETESSELLATE_MODE == 2)
			if (p.IsVisible()) {
				if (patchVisGrid[i] == 0) {
					patchVisGrid[i] = 1;
					retessellate = true;
				}
				if (p.IsDirty()) {
					//FIXME don't retessellate on small heightmap changes?
					p.ComputeVariance();
					retessellate = true;
				}
			} else {
				patchVisGrid[i] = 0;
			}
		#else
			if (char(p.IsVisible()) != patchVisGrid[i]) {
				patchVisGrid[i] = char(p.IsVisible());
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
			for_mt(0, roamPatches.size(), [&](const int i){
				Patch* it = &roamPatches[i];
				if (it->IsVisible()) {
					it->GenerateIndices();
				}
			});
		}

		{ SCOPED_TIMER("ROAM::Upload");
			for (std::vector<Patch>::iterator it = roamPatches.begin(); it != roamPatches.end(); ++it) {
				if (it->IsVisible()) {
					it->Upload();
				}
			}
		}

		/*{
			int tricount = 0;
			for (std::vector<Patch>::iterator it = roamPatches.begin(); it != roamPatches.end(); it++) {
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
				cam->pos.x,
				cam->pos.y,
				cam->pos.z
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

	CCamera* cam = nullptr;

	// FIXME:
	//  this only updates the *visibility* of patches
	//  It doesn't update the *tessellation*, neither are indices sent to the GPU.
	//  It just re-uses the last tessellation pattern which may have been created
	//  with a totally different camera (remove the "false &&" when ShadowHandler
	//  uses its own camera)
	if (false && inShadowPass) {
		cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
	} else {
		cam = CCamera::GetCamera(CCamera::CAMTYPE_VISCUL);
	}

	// NOTE: other places (e.g. DynWater) might want different constraints
	cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	Patch::UpdateVisibility(cam, roamPatches, numPatchesX);

	for (std::vector<Patch>::iterator it = roamPatches.begin(); it != roamPatches.end(); ++it) {
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

	for (int py = 0; py < numPatchesY; ++py) {
		for (int px = 0; px < numPatchesX; ++px) {
			if (IsInteriorPatch(px, py))
				continue;

			Patch& p = roamPatches[py * numPatchesX + px];

			if (!p.IsVisible())
				continue;

			if (!inShadowPass)
				p.SetSquareTexture();

			p.DrawBorder();
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
		minimap->ApplyConstraintsMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef3(UpVector);
		glScalef(1.0f / mapDims.mapx, -1.0f / mapDims.mapy, 1.0f);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

	for (const Patch& p: roamPatches) {
		if (!p.IsVisible()) {
			glRectf(p.coors.x, p.coors.y, p.coors.x + PATCH_SIZE, p.coors.y + PATCH_SIZE);
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
			Patch& patch = roamPatches[Y * numPatchesX + X];

			// Reset the patch
			patch.Reset();

			// Link all the patches together. (leave borders NULL)
			if (X > 0)
				patch.GetBaseLeft()->LeftNeighbor = roamPatches[Y * numPatchesX + X - 1].GetBaseRight();

			if (X < (numPatchesX - 1))
				patch.GetBaseRight()->LeftNeighbor = roamPatches[Y * numPatchesX + X + 1].GetBaseLeft();

			if (Y > 0)
				patch.GetBaseLeft()->RightNeighbor = roamPatches[(Y - 1) * numPatchesX + X].GetBaseRight();

			if (Y < (numPatchesY - 1))
				patch.GetBaseRight()->RightNeighbor = roamPatches[(Y + 1) * numPatchesX + X].GetBaseLeft();
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
	bool forceTess = false;

	for (int idx = 0; idx < 9; ++idx) {
		for_mt(0, roamPatches.size(), [&](const int i) {
			Patch* p = &roamPatches[i];

			const int X = p->coors.x;
			const int Z = p->coors.y;
			const int subindex = (X % 3) + (Z % 3) * 3;

			if ((subindex == idx) && p->IsVisible()) {
				if (!p->Tessellate(campos, viewradius))
					forceTess = true;
			}
		});

		if (forceTess)
			return true;
	}

	return false;
}

bool CRoamMeshDrawer::IsInteriorPatch(int px, int py) const {
	// using % will crash if numPatchesX == 1 or numPatchesY == 1
	if ((px == 0) || (px == numPatchesX - 1)) return false;
	if ((py == 0) || (py == numPatchesY - 1)) return false;
	return true;
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
			Patch& p = roamPatches[z * numPatchesX + x];

			// clamp the update rect to the patch constraints
			SRectangle prect(
				std::max(rect.x1 - margin - p.coors.x, 0),
				std::max(rect.z1 - margin - p.coors.y, 0),
				std::min(rect.x2 + margin - p.coors.x, PATCH_SIZE),
				std::min(rect.z2 + margin - p.coors.y, PATCH_SIZE)
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
