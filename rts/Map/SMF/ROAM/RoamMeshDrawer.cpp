/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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



#define LOG_SECTION_ROAM "RoamMeshDrawer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ROAM)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_ROAM



bool CRoamMeshDrawer::forceTessellate[2] = {false, false};



CRoamMeshDrawer::CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfReadMap(rm)
	, smfGroundDrawer(gd)
	, lastGroundDetail{0, 0}
{
	eventHandler.AddClient(this);

	// set ROAM upload mode (VA,DL,VBO)
	Patch::SwitchRenderMode(configHandler->GetInt("ROAM"));
	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		CTriNodePool::InitPools(i);
	}

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	// assert((numPatchesX == smfReadMap->numBigTexX) && (numPatchesY == smfReadMap->numBigTexY));

	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		patchMeshGrid[i].resize(numPatchesX * numPatchesY);
		const int borderSizeX = 1 + (numPatchesX > 1);
		const int borderSizeY = 1 + (numPatchesY > 1);
		borderPatches[i].resize(borderSizeX * borderSizeY + (numPatchesY - borderSizeY) * borderSizeX + (numPatchesX - borderSizeX) * borderSizeY, nullptr);
		patchVisFlags[i].resize(numPatchesX * numPatchesY, 0);
	}

	// initialize all terrain patches
	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		auto& patches = patchMeshGrid[i];

		for (int y = 0; y < numPatchesY; ++y) {
			for (int x = 0; x < numPatchesX; ++x) {
				Patch& patch = patches[y * numPatchesX + x];

				patch.Init(smfGroundDrawer, x * PATCH_SIZE, y * PATCH_SIZE);
				patch.ComputeVariance();
			}
		}
	}

	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		auto& patches = patchMeshGrid[i];

		unsigned int patchIdx = 0;

		// gather corner patches
		borderPatches[i][patchIdx++] = &patches[                                  (              0)];

		if (numPatchesX > 1)
			borderPatches[i][patchIdx++] = &patches[                                  (numPatchesX - 1)];

		if (numPatchesY > 1)
			borderPatches[i][patchIdx++] = &patches[(numPatchesY - 1) * numPatchesX + (              0)];

		if (numPatchesX > 1 && numPatchesY > 1)
			borderPatches[i][patchIdx++] = &patches[(numPatchesY - 1) * numPatchesX + (numPatchesX - 1)];

		// gather x-border patches
		for (int py = 1; py < (numPatchesY - 1); ++py) {
			borderPatches[i][patchIdx++] = &patches[py * numPatchesX + (              0)];
			borderPatches[i][patchIdx++] = &patches[py * numPatchesX + (numPatchesX - 1)];
		}

		// gather z-border patches
		for (int px = 1; px < (numPatchesX - 1); ++px) {
			borderPatches[i][patchIdx++] = &patches[(              0) * numPatchesX + px];
			borderPatches[i][patchIdx++] = &patches[(numPatchesY - 1) * numPatchesX + px];
		}
	}
}

CRoamMeshDrawer::~CRoamMeshDrawer()
{
	configHandler->Set("ROAM", (int)Patch::renderMode);

	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		CTriNodePool::FreePools(i);
	}
}


/**
 * Retessellates the current terrain
 */
void CRoamMeshDrawer::Update()
{
	CCamera* cam = CCamera::GetActiveCamera();

	bool shadowPass = (cam->GetCamType() == CCamera::CAMTYPE_SHADOW);
	bool retessellate = forceTessellate[shadowPass];

	auto& patches = patchMeshGrid[shadowPass];
	auto& pvflags = patchVisFlags[shadowPass];

	Patch::UpdateVisibility(cam, patches, numPatchesX);

#define RETESSELLATE_MODE 1

	{
		// Check if a retessellation is needed
		SCOPED_TIMER("ROAM::ComputeVariance");

		for (int i = 0; i < (numPatchesX * numPatchesY); ++i) {
			//FIXME multithread? don't retessellate on small heightmap changes?
			Patch& p = patches[i];

		#if (RETESSELLATE_MODE == 2)
			if (p.IsVisible(cam)) {
				if (pvflags[i] == 0) {
					pvflags[i] = 1;
					retessellate = true;
				}
				if (p.IsDirty()) {
					p.ComputeVariance();
					retessellate = true;
				}
			} else {
				pvflags[i] = 0;
			}

		#else

			if (uint8_t(p.IsVisible(cam)) != pvflags[i]) {
				pvflags[i] = uint8_t(p.IsVisible(cam));
				retessellate = true;
			}
			if (p.IsVisible(cam) && p.IsDirty()) {
				p.ComputeVariance();
				retessellate = true;
			}
		#endif
		}
	}

	// Further conditions that can cause a retessellation
#if (RETESSELLATE_MODE == 2)
	retessellate |= ((cam->GetPos() - lastCamPos[shadowPass]).SqLength() > (500.0f * 500.0f));
#endif
	retessellate |= (lastGroundDetail[shadowPass] != smfGroundDrawer->GetGroundDetail());

	if (!retessellate)
		return;

	{
		SCOPED_TIMER("ROAM::Tessellate");

		Reset(shadowPass);
		forceTessellate[shadowPass] = Tessellate(patchMeshGrid[shadowPass], cam, smfGroundDrawer->GetGroundDetail(), shadowPass);
	}

	{
		SCOPED_TIMER("ROAM::GenerateIndexArray");

		for_mt(0, patches.size(), [&patches, &cam](const int i) {
			Patch* p = &patches[i];

			if (p->IsVisible(cam)) {
				p->GenerateIndices();
			}
		});
	}

	{
		SCOPED_TIMER("ROAM::Upload");

		for (Patch& p: patches) {
			if (p.IsVisible(cam)) {
				p.Upload();
			}
		}
	}

	lastGroundDetail[shadowPass] = smfGroundDrawer->GetGroundDetail();
	lastCamPos[shadowPass] = cam->GetPos();
}



void CRoamMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	// NOTE:
	//   this updates the *tessellation* as well as the *visibility* of
	//   patches at the same time, because both depend on the *current*
	//   draw-pass (before Update would retessellate+upload indices and
	//   Draw would update patch visibility)
	//
	//   Updating for all passes produces the optimal tessellation per
	//   camera but consumes far too many cycles; force any non-shadow
	//   pass to reuse MESH_NORMAL
	switch (drawPass) {
		case DrawPass::Normal: { Update(); } break;
		case DrawPass::Shadow: { Update(); } break;
		default: {
			Patch::UpdateVisibility(CCamera::GetActiveCamera(), patchMeshGrid[MESH_NORMAL], numPatchesX);
		} break;
	}

	for (Patch& p: patchMeshGrid[drawPass == DrawPass::Shadow]) {
		if (!p.IsVisible(CCamera::GetActiveCamera()))
			continue;

		// do not need textures in the SP
		if (drawPass != DrawPass::Shadow)
			p.SetSquareTexture();

		p.Draw();
	}
}

void CRoamMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass)
{
	for (Patch* p: borderPatches[drawPass == DrawPass::Shadow]) {
		if (!p->IsVisible(CCamera::GetActiveCamera()))
			continue;

		if (drawPass != DrawPass::Shadow)
			p->SetSquareTexture();

		p->DrawBorder();
	}
}

void CRoamMeshDrawer::DrawInMiniMap()
{
	#ifdef DRAW_DEBUG_IN_MINIMAP
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

	for (const Patch& p: patchMeshGrid[MESH_NORMAL]) {
		if (!p.IsVisible(CCamera::GetActiveCamera())) {
			glRectf(p.coors.x, p.coors.y, p.coors.x + PATCH_SIZE, p.coors.y + PATCH_SIZE);
		}
	}

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	#endif
}



void CRoamMeshDrawer::Reset(bool shadowPass)
{
	std::vector<Patch>& patches = patchMeshGrid[shadowPass];

	// set the next free triangle pointer back to the beginning
	CTriNodePool::ResetAll(shadowPass);

	// perform patch resets, compute variances, and link
	for (int y = 0; y < numPatchesY; ++y) {
		for (int x = 0; x < numPatchesX; ++x) {
			Patch& patch = patches[y * numPatchesX + x];
			// recompute variance if needed
			patch.Reset();

			TriTreeNode* pbl = patch.GetBaseLeft();
			TriTreeNode* pbr = patch.GetBaseRight();

			// link all patches together, leave borders NULL
			if (x > (              0)) { pbl->LeftNeighbor = patches[y * numPatchesX + x - 1].GetBaseRight(); }
			if (x < (numPatchesX - 1)) { pbr->LeftNeighbor = patches[y * numPatchesX + x + 1].GetBaseLeft(); }

			if (y > (              0)) { pbl->RightNeighbor = patches[(y - 1) * numPatchesX + x].GetBaseRight(); }
			if (y < (numPatchesY - 1)) { pbr->RightNeighbor = patches[(y + 1) * numPatchesX + x].GetBaseLeft(); }
		}
	}
}



bool CRoamMeshDrawer::Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass)
{
	// create an approximate tessellated mesh of the landscape
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
		for_mt(0, patches.size(), [&](const int i) {
			Patch* p = &patches[i];

			const int X = p->coors.x;
			const int Z = p->coors.y;
			const int subindex = (X % 3) + (Z % 3) * 3;

			if (subindex != idx)
				return;

			if (!p->IsVisible(cam))
				return;

			forceTess |= (!p->Tessellate(cam->GetPos(), viewRadius, shadowPass));
		});

		if (forceTess)
			return true;
	}

	return false;
}



void CRoamMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	const int margin = 2;
	const float INV_PATCH_SIZE = 1.0f / PATCH_SIZE;

	// hint: the -+1 are cause Patches share 1 pixel border (no vertex holes!)
	const int xstart = std::max(          0, (int)math::floor((rect.x1 - margin) * INV_PATCH_SIZE));
	const int xend   = std::min(numPatchesX, (int)math::ceil ((rect.x2 + margin) * INV_PATCH_SIZE));
	const int zstart = std::max(          0, (int)math::floor((rect.z1 - margin) * INV_PATCH_SIZE));
	const int zend   = std::min(numPatchesY, (int)math::ceil ((rect.z2 + margin) * INV_PATCH_SIZE));

	// update patches in both tessellations
	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		auto& patches = patchMeshGrid[i];

		for (int z = zstart; z < zend; ++z) {
			for (int x = xstart; x < xend; ++x) {
				Patch& p = patches[z * numPatchesX + x];

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
	}
}

