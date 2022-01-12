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


#define RETESSELLATE_TO_FREE_INVISIBLE_PATCHES 50
#define MAGIC_RETESSELATE_CAMERA_RATIO 1.5f
#define MAGIC_TOTAL_CAMDIST_RETESSELATE 2.5f
#define CAMERA_CHANGE_TRESHOLD 16.0f
#define TESSELATION_DEBUG 0

bool CRoamMeshDrawer::forceNextTesselation[MESH_COUNT] = {false, false};


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

	#ifdef DRAW_DEBUG_IN_MINIMAP
		debugColors.resize(numPatchesX*numPatchesY);
	#endif

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

	for (unsigned int i = MESH_NORMAL; i < MESH_COUNT; i++) {
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


// Notes for LOAM (Lazy Optimally Adapting Meshes).
// Dear reader, consider yourself a rubber ducky.

// 1. a dirty patch can be retesselated by just running tessellate
// --needs the ability to 'resume' tesselation
// --after retesselation, only update the vertex buffers of the stuff that changed

// 2. when a patch goes out of view, dont force a reset on the whole thing

// 3. visibility checking needs some serious rework, and the groundwork for that is already laid out above
//  -- 3x as many shadow patches rendered as regular!

// MT roam is not appreciably faster than ST roam, gut it
// MT roam easily runs out of pool space, and crashes due to:
// not guaranteed that two threads do not tesselate patches that dont share neighbours
// unbalanced use of trinodepools
// wasteful of RAM
// Easy to run out of, completely nondeterministic that a resized pool will not fuck up

// we need to get rid of the malloc errors that come from MT roam

// Why do we even keep a separate set of meshes for shadow and regular?
// we could easily half the load again by reusing the same meshes (especiallally the ram load)
// we need to use visible in shadowORmain

// fix pool.resize failing, either by retrying with a tiny bit smaller window, or by stopping tesselation straight up

// How can we retessellate patches entering view sanely?
// The same way we do it for dirty patches? does this mean that they might be forced to be at a lower resolution?
// can they be at a lower resolution even?

// Tricky cases:
// going from full view to zoomed in state:
// theoretically, full view should be tesselated at a low degree, so not all of the trinodepool should be exhausted
// when zooming in further, an increase in detail should be forced , but this 1. further exhausts the tripool, and should not be done too often.
// we can call retesselate of a patch,

// an exhaustion of the tritreenodepool should instantly trigger a reset and retesselate

// 4. Shadowpass seems to have on average 3x more patches than regular.
//  - also, its 'camera' is quite static, only really changes along one axis, and is unreliable:
// [f=0002142] [RoamMeshDrawer] Skip:37 oldcam: 4334.830078:-259.387939:-259.387939 nowcam 5011.015137:-259.387939:1593.139526 dist=924793.375000
// [f=0002142] [RoamMeshDrawer] Skip:50 oldcam: 4334.830078:-259.387939:-259.387939 nowcam 5011.015137:-259.387939:1593.139526 dist=924793.375000
// [f=0002142] [RoamMeshDrawer] Skip:63 oldcam: 4334.830078:-259.387939:-259.387939 nowcam 5011.015137:-259.387939:1593.139526 dist=924793.375000
// [f=0002142] [RoamMeshDrawer] Skip:76 oldcam: 4334.830078:-259.387939:-259.387939 nowcam 5011.015137:-259.387939:1593.139526 dist=924793.375000

// TODO:
// unintialized memory read in Patch::IsVisible!

// TODO tritreenodepool:
// allow growth
// handle bad allocs

// 5. Better zoom handling:
//  -since the tesselation depends only on the distance from camera
//  -position doesnt really matter
//  - if a patch was tesselated with a camera that is more than 2x closer than the last tesselation of that patch, then we should retesselate
//  - I do not know how this relates to the shadow camera, as that still needs quite a bit of work anyway
//  - maybe just use the shadowcamera for visibility, but actually use the main camera for distance?
//  - this would also add a 'magic'
//  TODO: lastCamPos[shadowPass] = cam->GetPos(); // is update a frame too late
//  CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER) // gets the actual camera! lets hope its updated before shadow pass.
void CRoamMeshDrawer::Update()
{
	CCamera* cam = CCameraHandler::GetActiveCamera();

	bool shadowPass = (cam->GetCamType() == CCamera::CAMTYPE_SHADOW);
#if TESSELATION_DEBUG
	bool tesselMesh = forceNextTesselation[shadowPass];
#endif

	auto& patches = patchMeshGrid[shadowPass];
	auto& pvflags = patchVisFlags[shadowPass];
	const int numPatches = patches.size();

	Patch::UpdateVisibility(cam, patches, numPatchesX);

	//Early bailout conditions:
	if ((cam->GetPos().distance(lastCamPos[shadowPass]) < CAMERA_CHANGE_TRESHOLD) &&
		(cam->GetDir().distance(lastCamDir[shadowPass]) < CAMERA_CHANGE_TRESHOLD * 0.001f) &&
		(heightMapChanged == false) &&
		(smfGroundDrawer->GetGroundDetail() == lastGroundDetail[shadowPass])) {
		return;
	}

	size_t numPatchesVisible = 0;

#if TESSELATION_DEBUG
	size_t numPatchesEnterVisibility = 0;
	size_t numPatchesExitVisibility = 0;
	size_t numPatchesZoomChanged = 0;
#endif

	CCamera* playerCamera = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	float3 playerCameraPosition = playerCamera->GetPos();
	float totalCameraDistanceRatioInv = 0.0f;
	std::vector<bool> patchesToTesselate(numPatches);
	std::fill(patchesToTesselate.begin(), patchesToTesselate.end(),0);

#if TESSELATION_DEBUG
	if (tesselMesh)
		LOG("Tesselation forced for pass %i in frame %i", shadowPass,globalRendering->drawFrame);
#endif //TESSELATION_DEBUG

	{
		// Check if a retessellation is needed
		//SCOPED_TIMER("ROAM::ComputeVariance");

		for (int i = 0; i < numPatches; ++i) {
			//FIXME multithread? don't retessellate on small heightmap changes?
			Patch& p = patches[i];

			const bool isVisibleNow = p.IsVisible(cam);
			const bool wasVisible = pvflags[i];
			pvflags[i] = uint8_t(p.IsVisible(cam));

			// first case: a patch left visibility;
			// do nothing, just count the number of patches in total that have done so, as they still use the trinodepools
			// when a sufficient number of patches have left visibility, we are going to have to force a reset.
			if (!isVisibleNow && wasVisible) {
			#if TESSELATION_DEBUG
				numPatchesExitVisibility++;
			#endif
				numPatchesLeftVisibility[shadowPass]++;
				if (numPatchesLeftVisibility[shadowPass] > numPatches * RETESSELLATE_TO_FREE_INVISIBLE_PATCHES) {

					// Be even lazier, and only force full retesselation once zoomed back in
					if ((numPatchesVisible * (5 - 3 * shadowPass) < numPatches) ||
						(numPatchesLeftVisibility[shadowPass] > 2 * numPatches * RETESSELLATE_TO_FREE_INVISIBLE_PATCHES)) {
						#if TESSELATION_DEBUG
							LOG("Too many patches (%i) left visibility, forcing retess on this df #%i! poolsize=%i used=%i",
								numPatchesLeftVisibility[shadowPass],
								globalRendering->drawFrame,
								p.curTriPool->getPoolSize(),
								p.curTriPool->getNextTriNodeIdx());
						#endif
						forceNextTesselation[shadowPass] = true;
						numPatchesLeftVisibility[shadowPass] = 0;
					}
				}
			}


			// second case, a patch entered visibility:
			if (isVisibleNow) {
				numPatchesVisible++;
				// if it was dirty(had heightmap change) then recompute variances.
				if (p.IsDirty()) {
					p.ComputeVariance();
					// here we can do incremental retesselation?
					patchesToTesselate[i] = true;
				}

			#if TESSELATION_DEBUG
				if (!wasVisible)
					numPatchesEnterVisibility++;
			#endif

				const float currentDistanceFromCamera = playerCameraPosition.distance(p.midPos);

				//high numbers means we zoomed in more:
				const float distanceFromCameraRatio = p.camDistanceLastTesselation / currentDistanceFromCamera;

				if ((currentDistanceFromCamera > 500.0f) && (distanceFromCameraRatio > MAGIC_RETESSELATE_CAMERA_RATIO)) {
					patchesToTesselate[i] = true;
					// Since we just retesselated,
					totalCameraDistanceRatioInv += 1.0;
					//LOG("Zoom%i currdist=%.1f lastdist=%.1f ratio=%.1f",i,currentDistanceFromCamera,p.camDistanceLastTesselation,distanceFromCameraRatio);
				#if TESSELATION_DEBUG
					numPatchesZoomChanged++;
				#endif
				} else {
					totalCameraDistanceRatioInv += 1.0 / distanceFromCameraRatio;
				}
			}

			#ifdef DRAW_DEBUG_IN_MINIMAP
				if (isVisibleNow) {
					if (shadowPass) {
						debugColors[i].z = 1.0;
					} else {
						debugColors[i].y = 1.0;
					}
				} else {
					if (shadowPass) {
						debugColors[i].z = 0.0;
					} else {
						debugColors[i].y = 0.0;
					}

				}
			#endif
		}
	}

	int actualTesselations = 0;
	int actualUploads = 0;
	totalCameraDistanceRatioInv = totalCameraDistanceRatioInv / numPatchesVisible;

	if (totalCameraDistanceRatioInv > MAGIC_TOTAL_CAMDIST_RETESSELATE) {
		forceNextTesselation[shadowPass] = true;

	#if TESSELATION_DEBUG
		LOG("Zoomed out too far, retesselating all to save tris: Visible=%i pass=%i ratio=%.2f",
			numPatchesVisible,
			shadowPass,
			totalCameraDistanceRatioInv);
	#endif
	}

	{
		//SCOPED_TIMER("ROAM::Tessellate");

		// tesselate all the patches patches that are marked for it, but if we run out of trinodes,
		// then reset all of them and try again
		bool tessSuccess = true;
		if (!forceNextTesselation[shadowPass]) {
			for (int pi = 0; pi < numPatches; ++pi) {
				Patch& p = patches[pi];

			#ifdef DRAW_DEBUG_IN_MINIMAP
				debugColors[pi].x = std::max (debugColors[pi].x -0.002f,0.0f);
			#endif

				if (!patchesToTesselate[pi])
					continue;

				actualTesselations++;
				tesselationsSinceLastReset[shadowPass]++;

			#ifdef DRAW_DEBUG_IN_MINIMAP
				debugColors[pi].x = 1.0;
			#endif

				if (p.Tessellate(playerCameraPosition, smfGroundDrawer->GetGroundDetail(), shadowPass))
					continue;

				tessSuccess = false;

			#if TESSELATION_DEBUG
				LOG("Lazy tesselation ran out of trinodes, trying again after a reset #Visible=%i #tesssincelastreset=%i, shadow=%i",
					numPatchesVisible,
					tesselationsSinceLastReset[shadowPass],
					shadowPass);
			#endif // TESSELATION_DEBUG

				break;
			}
		}

		if(forceNextTesselation[shadowPass] || !tessSuccess) {
			Reset(shadowPass);
			for (int pi = 0; pi < numPatches; ++pi) {
				Patch& p = patches[pi];
				if (!p.IsVisible(cam))
					continue;

				p.Tessellate(playerCameraPosition, smfGroundDrawer->GetGroundDetail(), shadowPass);
				actualTesselations++;
				tesselationsSinceLastReset[shadowPass]++;

			#ifdef DRAW_DEBUG_IN_MINIMAP
				debugColors[pi].x = 1.0;
			#endif
			}
		}
	}

	{
		//SCOPED_TIMER("ROAM::GenerateIndexArray");
		//only do for_mt if a full tesselation happened, since only a few patches come into visibility
		if (forceNextTesselation[shadowPass]) {
			for_mt(0, numPatches, [&patches, &cam](const int i) {
				Patch* p = &patches[i];
				if (!p->IsVisible(cam))
					return;

				p->GenerateIndices();
				p->GenerateBorderVertices();
			});
		} else {
			for (Patch& p: patches) {
				if (!p.IsVisible(cam) || !p.isChanged)
					continue;

				p.GenerateIndices();
				p.GenerateBorderVertices();
			}
		}
	}
	{
		//SCOPED_TIMER("ROAM::Upload");

		for (Patch& p: patches) {
			if (!p.IsVisible(cam) || !p.isChanged)
				continue;

			p.UploadIndices();
			p.UploadBorderVertices();
			actualUploads++;
		}
	}
#if TESSELATION_DEBUG
	if (actualTesselations > 0 || actualUploads > 0) {
		LOG("#V=%i d_in=%i d_out=%i in df:#%i shadow:%i Z+%i actual=%i up=%i mratio = %.2f",
			numPatchesVisible,
			numPatchesEnterVisibility,
			numPatchesExitVisibility,
			globalRendering->drawFrame,
			shadowPass,
			numPatchesZoomChanged,
			actualTesselations,
			actualUploads,
			totalCameraDistanceRatioInv);

		float3 nowcampos = cam->GetPos();
		//LOG("Camera pass=%i pos= %.1f, %.1f, %.1f", shadowPass, nowcampos.x, nowcampos.y, nowcampos.z);
	}
#endif // TESSELATION_DEBUG

	lastGroundDetail[shadowPass] = smfGroundDrawer->GetGroundDetail();
	lastCamPos[shadowPass] = cam->GetPos();
	lastCamDir[shadowPass] = cam->GetDir();
	forceNextTesselation[shadowPass] = false;
	heightMapChanged = false;
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
	{
		SCOPED_TIMER("Draw::World::Terrain::ROAM::Update");

		switch (drawPass) {
			case DrawPass::Normal: { Update(); } break;
			case DrawPass::Shadow: { Update(); } break;
			default: {
				Patch::UpdateVisibility(CCameraHandler::GetActiveCamera(), patchMeshGrid[MESH_NORMAL], numPatchesX);
			} break;
		}
	}

	{
		SCOPED_TIMER("Draw::World::Terrain::ROAM::Draw");
		for (Patch& p: patchMeshGrid[drawPass == DrawPass::Shadow]) {
			if (!p.IsVisible(CCameraHandler::GetActiveCamera()))
				continue;

			// do not need textures in the SP
			if (drawPass != DrawPass::Shadow)
				p.SetSquareTexture();

			p.Draw();
		}
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
	prog->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
	prog->SetUniformMatrix4x4<float>("u_proj_mat", false, projMat);

	int pi = 0;
	for (const Patch& p: patchMeshGrid[MESH_NORMAL]) {
		const float2 patchPos = {p.coors.x * 1.0f, p.coors.y * 1.0f};

		rdbc->SafeAppend({{patchPos.x                    , patchPos.y                    , 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // tl
		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y                    , 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // tr
		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // br

		rdbc->SafeAppend({{patchPos.x + PATCH_SIZE * 1.0f, patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // br
		rdbc->SafeAppend({{patchPos.x                    , patchPos.y + PATCH_SIZE * 1.0f, 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // bl
		rdbc->SafeAppend({{patchPos.x                    , patchPos.y                    , 0.0f}, {debugColors[pi].x, debugColors[pi].y, debugColors[pi].z ,0.5f}}); // tl
		pi++;
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
	tesselationsSinceLastReset[shadowPass] = 0;
}


void CRoamMeshDrawer::Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass) {
	bool forceTess = false;

	for (Patch& p: patches) {
		if (!p.IsVisible(cam))
			continue;

		forceTess = forceTess || (!p.Tessellate(cam->GetPos(), viewRadius, shadowPass));
	}

	forceNextTesselation[shadowPass] = forceTess;
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
	heightMapChanged = true;
}

