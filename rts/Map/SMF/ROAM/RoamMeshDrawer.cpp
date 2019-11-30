/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RoamMeshDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"
#include "System/Threading/ThreadPool.h"
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



bool CRoamMeshDrawer::forceNextTesselation[2] = {false, false};
bool CRoamMeshDrawer::useThreadTesselation[2] = {false, false};



CRoamMeshDrawer::CRoamMeshDrawer(CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		CTriNodePool::InitPools(i == MESH_SHADOW);
	}

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	// assert((numPatchesX == smfReadMap->numBigTexX) && (numPatchesY == smfReadMap->numBigTexY));

	ForceNextTesselation(true, true);
	UseThreadTesselation(numPatchesX >= 4 && numPatchesY >= 4, numPatchesX >= 4 && numPatchesY >= 4);


	tesselateFuncs[true] = [this](std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass) {
		// create an approximate tessellated mesh of the landscape
		//
		//   px 0 1 2   3 4 5 . .
		//   z  _________
		//   0 |0|1|2 | 0 . .
		//   1 |3|4|5 | 3 . .
		//   2 |6|7|8 | 6 . .
		//     |
		//   3 |0|1|2 | 0 . .
		//   4  . . .   . . .
		//   5  . . .   . . .
		//   .
		//   .
		// each patch is connected to 2, 3, or 4 neighbors via its two base-triangles
		// tessellation can extend into these neighbors so patches sharing a neighbor
		// can not be touched concurrently without expensive locking, must split the
		// update into 3x3 sub-blocks instead s.t. only patches with equal sub-block
		// indices ([0,8]) are tessellated in parallel
		// note that both numPatchesX and numPatchesY must be larger than or equal to
		// 4 for this to be even barely worth it; threading with 9 (!) for_mt's has a
		// high setup-cost
		std::atomic<bool> forceTess{false};

		for (int blkIdx = 0; blkIdx < (3 * 3); ++blkIdx) {
			for_mt(0, patches.size(), [&](const int pi) {
				Patch& p = patches[pi];

				// convert (RM) grid-coors to subblock-index
				const int  px = pi % numPatchesX;
				const int  pz = pi / numPatchesX;
				const int sbx = px % 3;
				const int sbz = pz % 3;
				const int sbi = sbx + (sbz * 3);

				if (sbi != blkIdx)
					return;

				if (!p.IsVisible(cam))
					return;

				// stop early in case of pool exhaustion
				forceTess = forceTess || (!p.Tessellate(cam->GetPos(), viewRadius, shadowPass));
			});

			if (forceTess)
				return true;
		}

		return false;
	};

	tesselateFuncs[false] = [this](std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass) {
		bool forceTess{false};

		for (Patch& p: patches) {
			if (!p.IsVisible(cam))
				continue;

			forceTess = forceTess || (!p.Tessellate(cam->GetPos(), viewRadius, shadowPass));
		}

		return forceTess;
	};


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
		if (numPatchesX > 1                   ) borderPatches[i][patchIdx++] = &patches[                                  (numPatchesX - 1)];
		if (numPatchesY > 1                   ) borderPatches[i][patchIdx++] = &patches[(numPatchesY - 1) * numPatchesX + (              0)];
		if (numPatchesX > 1 && numPatchesY > 1) borderPatches[i][patchIdx++] = &patches[(numPatchesY - 1) * numPatchesX + (numPatchesX - 1)];

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
	eventHandler.RemoveClient(this);
}


/**
 * Retessellates the current terrain
 */
void CRoamMeshDrawer::Update()
{
	CCamera* cam = CCameraHandler::GetActiveCamera();

	bool shadowPass = (cam->GetCamType() == CCamera::CAMTYPE_SHADOW);
	bool tesselMesh = forceNextTesselation[shadowPass];

	auto& patches = patchMeshGrid[shadowPass];
	auto& pvflags = patchVisFlags[shadowPass];

	Patch::UpdateVisibility(cam, patches, numPatchesX);

#define RETESSELLATE_MODE 1

	{
		// Check if a retessellation is needed
		//SCOPED_TIMER("ROAM::ComputeVariance");

		for (int i = 0; i < (numPatchesX * numPatchesY); ++i) {
			//FIXME multithread? don't retessellate on small heightmap changes?
			Patch& p = patches[i];

		#if (RETESSELLATE_MODE == 2)
			if (p.IsVisible(cam)) {
				if (tesselMesh |= (pvflags[i] == 0))
					pvflags[i] = 1;
				if (tesselMesh |= p.IsDirty())
					p.ComputeVariance();
			} else {
				pvflags[i] = 0;
			}

		#else

			if (tesselMesh |= (uint8_t(p.IsVisible(cam)) != pvflags[i]))
				pvflags[i] = uint8_t(p.IsVisible(cam));

			if (tesselMesh |= (p.IsVisible(cam) && p.IsDirty()))
				p.ComputeVariance();
		#endif
		}
	}

	// Further conditions that can cause a retessellation
#if (RETESSELLATE_MODE == 2)
	tesselMesh |= ((cam->GetPos() - lastCamPos[shadowPass]).SqLength() > (500.0f * 500.0f));
#endif
	tesselMesh |= (lastGroundDetail[shadowPass] != smfGroundDrawer->GetGroundDetail());

	if (!tesselMesh)
		return;

	{
		//SCOPED_TIMER("ROAM::Tessellate");

		Reset(shadowPass);
		Tessellate(patches, cam, smfGroundDrawer->GetGroundDetail(), useThreadTesselation[shadowPass], shadowPass);
	}

	{
		//SCOPED_TIMER("ROAM::GenerateIndexArray");

		for_mt(0, patches.size(), [&patches, &cam](const int i) {
			Patch* p = &patches[i];

			if (!p->IsVisible(cam))
				return;

			p->GenerateIndices();
			p->GenerateBorderVertices();
		});
	}

	{
		//SCOPED_TIMER("ROAM::Upload");

		for (Patch& p: patches) {
			if (!p.IsVisible(cam))
				continue;

			p.UploadIndices();
			p.UploadBorderVertices();
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

	// SCOPED_TIMER can't have dynamic values in a single call
	//SCOPED_TIMER(drawPass == DrawPass::Normal ? "Draw::World::Terrain::ROAM" : "Misc::ROAM");
	SCOPED_TIMER("Draw::World::Terrain::ROAM");

	switch (drawPass) {
		case DrawPass::Normal: { Update(); } break;
		case DrawPass::Shadow: { Update(); } break;
		default: {
			Patch::UpdateVisibility(CCameraHandler::GetActiveCamera(), patchMeshGrid[MESH_NORMAL], numPatchesX);
		} break;
	}

	for (Patch& p: patchMeshGrid[drawPass == DrawPass::Shadow]) {
		if (!p.IsVisible(CCameraHandler::GetActiveCamera()))
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
		if (!p->IsVisible(CCameraHandler::GetActiveCamera()))
			continue;

		if (drawPass != DrawPass::Shadow)
			p->SetSquareTexture();

		p->DrawBorder();
	}
}

void CRoamMeshDrawer::DrawInMiniMap()
{
	#ifdef DRAW_DEBUG_IN_MINIMAP
	// HACK:
	//   MiniMap::{Update->UpdateTextureCache->DrawForReal->DrawInMM} runs before DrawWorld, so
	//   lastDrawFrames[cam->GetCamType()] will always be one behind globalRendering->drawFrame
	//   (which is incremented at the end of SwapBuffers) for visible patches
	globalRendering->drawFrame -= 1;

	GL::RenderDataBufferC* rdbc = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = rdbc->GetShader();

	const CMatrix44f& viewMat = minimap->GetViewMat(1);
	const CMatrix44f& projMat = minimap->GetProjMat(1);

	prog->Enable();
	prog->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, viewMat);
	prog->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, projMat);

	for (const Patch& p: patchMeshGrid[MESH_NORMAL]) {
		const float2 patchPos = {p.coors.x * 1.0f, p.coors.y * 1.0f};

		if (p.IsVisible(CCameraHandler::GetActiveCamera()))
			continue;

		rdbc->SafeAppend({{patchPos.x                    , patchPos.y                    , 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // tl
		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y                    , 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // tr
		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // br

		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // br
		rdbc->SafeAppend({{patchPos.x                    , patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // bl
		rdbc->SafeAppend({{patchPos.x                    , patchPos.y                    , 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // tl
	}

	rdbc->Submit(GL_TRIANGLES);
	prog->Disable();

	globalRendering->drawFrame += 1;
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
			if (x > (              0)) pbl->LeftNeighbor = patches[y * numPatchesX + x - 1].GetBaseRight();
			if (x < (numPatchesX - 1)) pbr->LeftNeighbor = patches[y * numPatchesX + x + 1].GetBaseLeft();

			if (y > (              0)) pbl->RightNeighbor = patches[(y - 1) * numPatchesX + x].GetBaseRight();
			if (y < (numPatchesY - 1)) pbr->RightNeighbor = patches[(y + 1) * numPatchesX + x].GetBaseLeft();
		}
	}
}



void CRoamMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	constexpr int BORDER_MARGIN = 2;
	constexpr float INV_PATCH_SIZE = 1.0f / PATCH_SIZE;

	// add margin since Patches share borders
	const int xstart = std::max(          0, (int)math::floor((rect.x1 - BORDER_MARGIN) * INV_PATCH_SIZE));
	const int xend   = std::min(numPatchesX, (int)math::ceil ((rect.x2 + BORDER_MARGIN) * INV_PATCH_SIZE));
	const int zstart = std::max(          0, (int)math::floor((rect.z1 - BORDER_MARGIN) * INV_PATCH_SIZE));
	const int zend   = std::min(numPatchesY, (int)math::ceil ((rect.z2 + BORDER_MARGIN) * INV_PATCH_SIZE));

	// update patches in both tessellations
	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		auto& patches = patchMeshGrid[i];

		for (int z = zstart; z < zend; ++z) {
			for (int x = xstart; x < xend; ++x) {
				Patch& p = patches[z * numPatchesX + x];

				// clamp the update-rectangle within the patch
				SRectangle prect(
					std::max(rect.x1 - BORDER_MARGIN - p.coors.x,          0),
					std::max(rect.z1 - BORDER_MARGIN - p.coors.y,          0),
					std::min(rect.x2 + BORDER_MARGIN - p.coors.x, PATCH_SIZE),
					std::min(rect.z2 + BORDER_MARGIN - p.coors.y, PATCH_SIZE)
				);

				p.UpdateHeightMap(prect);
			}
		}
	}
}

