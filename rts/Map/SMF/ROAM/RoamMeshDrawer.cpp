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


//tessmode can be 1,2,(3 is LOAM)
#define RETESSELLATE_MODE 3
#define RETESSELATE_TO_FREE_INVISIBLE_PATCHES 2
#define MAGIC_RETESSELATE_RETURNING_PATCH_CAMDIST 1000.0f * 1000.0f
#define TESSMODE_3_DEBUG 0

bool CRoamMeshDrawer::forceNextTesselation[2] = {false, false};
bool CRoamMeshDrawer::useThreadTesselation[2] = {false, false};



CRoamMeshDrawer::CRoamMeshDrawer(CSMFGroundDrawer* gd)
	: CEventClient("[CRoamMeshDrawer]", 271989, false)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	// set patch upload-mode (VA,DL,VBO)
	Patch::SwitchRenderMode(configHandler->GetInt("ROAM"));

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

	//TODO: rip out MT completely, LOAM doesn't need/cant use/wont use it
    tesselateFuncs[true] = tesselateFuncs[false];

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
	bool fullRetesselate = false;

	auto& patches = patchMeshGrid[shadowPass];
	auto& pvflags = patchVisFlags[shadowPass];

	Patch::UpdateVisibility(cam, patches, numPatchesX);

    int numPatchesVisible = 0;
    int numPatchesEnterVisibility = 0;
    int numPatchesEnteredSkipped = 0;
    int numPatchesExitVisibility = 0;
    int numPatchesRetesselated = 0;    
    bool patchesToTesselate[numPatchesX * numPatchesY];
    memset( patchesToTesselate, 0, numPatchesX * numPatchesY*sizeof(bool) );
    // Notes for Tessmode 3, e.g. LOAM (Lazy Optimally Adapting Meshes).
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

    #if TESSMODE_3_DEBUG
        if(tesselMesh) LOG("Tesselation forced for pass %i in frame %i", shadowPass,globalRendering->drawFrame);

    #endif //TESSMODE_3_DEBUG

	{
		// Check if a retessellation is needed
		SCOPED_TIMER("ROAM::ComputeVariance");

		for (int i = 0; i < (numPatchesX * numPatchesY); ++i) {
			//FIXME multithread? don't retessellate on small heightmap changes?
			Patch& p = patches[i];

		#if (RETESSELLATE_MODE == 2)
			if (p.IsVisible(cam)) {
				if (tesselMesh |= (pvflags[i] == 0))
					pvflags[i] = 1;
				if (p.IsDirty()) {
					p.ComputeVariance();
					tesselMesh = true;
				}
			} else {
				pvflags[i] = 0;
			}

		#elif (RETESSELLATE_MODE == 3)
            bool isVisibleNow = p.IsVisible(cam);
            bool wasVisible = pvflags[i];
            pvflags[i] = uint8_t(p.IsVisible(cam));

            // first case: a patch left visibility;
            // do nothing, just count the number of patches in total that have done so, as they still use the trinodepools
            // when a sufficient number of patches have left visibility, we are going to have to force a reset.

            if (!isVisibleNow && wasVisible){
                numPatchesExitVisibility ++;
                numPatchesLeftVisibility[shadowPass]++;
                if (numPatchesLeftVisibility[shadowPass] > numPatchesX*numPatchesY*RETESSELATE_TO_FREE_INVISIBLE_PATCHES){
                    #if TESSMODE_3_DEBUG
                        LOG("Too many patches (%i) left visibility, forcing retess on this df #%i! poolsize=%i used=%i",numPatchesLeftVisibility[shadowPass],globalRendering->drawFrame,p.curTriPool->getPoolSize(),p.curTriPool->getNextTriNodeIdx());
                    #endif
                    forceNextTesselation[shadowPass] = true;
                    numPatchesLeftVisibility[shadowPass] = 0;
                }
            }


            // second case, a patch entered visibility:
            if (isVisibleNow){
                numPatchesVisible ++;
                // if it was dirty(had heightmap change) then recompute variances.
                if (p.IsDirty()){
                    p.ComputeVariance();
                    // here we can do incremental retesselation?
                    patchesToTesselate[i] = true;
                }

                if (!wasVisible){
                    numPatchesEnterVisibility++;
                    if ((p.lastCameraPosition - lastCamPos[shadowPass]).SqLength() > MAGIC_RETESSELATE_RETURNING_PATCH_CAMDIST){
                        patchesToTesselate[i] = true;

                    }else{
                        #if TESSMODE_3_DEBUG
                            LOG("Skip:%i oldcam: %.1f:%.1f:%.1f nowcam %.1f:%.1f:%.1f dist=%.1f",
                                i,
                                p.lastCameraPosition.x,
                                p.lastCameraPosition.y,
                                p.lastCameraPosition.y,
                                lastCamPos[shadowPass].x,
                                lastCamPos[shadowPass].y,
                                lastCamPos[shadowPass].z,
                                (p.lastCameraPosition - lastCamPos[shadowPass]).SqLength()

                                );
                        #endif // TESSMODE_3_DEBUG
                        numPatchesEnteredSkipped++;
                    }
                }
            }

        #elif (RETESSELLATE_MODE == 1)
            bool isVisibleNow = p.IsVisible(cam);
            bool wasVisible = pvflags[i];
            if (isVisibleNow && !wasVisible){
                numPatchesEnterVisibility++;
            }

            if (!isVisibleNow && wasVisible){
                numPatchesExitVisibility ++;
            }
            if (isVisibleNow)
                numPatchesVisible ++;
            if(p.IsVisible(cam) != pvflags[i]){
                if (pvflags[i]) {
                    numPatchesExitVisibility++;
                }else {
                    numPatchesEnterVisibility++;
                    p.Tessellate(cam->GetPos(), smfGroundDrawer->GetGroundDetail(), shadowPass);
                }
            }
			if (tesselMesh |= (uint8_t(p.IsVisible(cam)) != pvflags[i]))
				pvflags[i] = uint8_t(p.IsVisible(cam));
            numPatchesVisible+=pvflags[i];
			if (p.IsVisible(cam) && p.IsDirty()) {
				p.ComputeVariance();
				tesselMesh = true;
			}
		#endif
		}
	}

	// Further conditions that can cause a retessellation
#if (RETESSELLATE_MODE == 2)
	tesselMesh |= ((cam->GetPos() - lastCamPos[shadowPass]).SqLength() > (500.0f * 500.0f));
#endif

#if (RETESSELLATE_MODE == 3)
    int actualTesselations = 0;
    int actualUploads = 0;
     {
		SCOPED_TIMER("ROAM::Tessellate");
		// tesselate all the patches patches that are marked for it, but if we run out of trinodes,
		// then reset all of them and try again
		bool tessSuccess = true;
		if (!forceNextTesselation[shadowPass] ){
            int pi = 0;
            for (Patch& p: patches) {
                if (patchesToTesselate[pi]){
                    actualTesselations++;
                    tesselationsSinceLastReset[shadowPass]++;
                    if(!p.Tessellate(cam->GetPos(), smfGroundDrawer->GetGroundDetail(), shadowPass)){
                        tessSuccess = false;
                        #if TESSMODE_3_DEBUG
                            LOG("Lazy tesselation ran out of trinodes, trying again after a reset #Visible=%i #tesssincelastreset=%i, shadow=%i" , numPatchesVisible,tesselationsSinceLastReset[shadowPass],shadowPass);
                        #endif // TESSMODE_3_DEBUG
                        break;
                    }

                }
                pi++; //whoops
            }
		}

        if(forceNextTesselation[shadowPass] || !tessSuccess ){
            Reset(shadowPass);
            for (Patch& p: patches) {
                if (p.IsVisible(cam)){
                    p.Tessellate(cam->GetPos(), smfGroundDrawer->GetGroundDetail(), shadowPass);
                    actualTesselations ++;
                    tesselationsSinceLastReset[shadowPass]++;
                }
            }
            forceNextTesselation[shadowPass] = false;
        }
	}

	{
	    SCOPED_TIMER("ROAM::GenerateIndexArray");
        for_mt(0, patches.size(), [&patches, &cam](const int i) {
			Patch* p = &patches[i];
			if (p->IsVisible(cam)){
                if (p->isChanged )
                    p->GenerateIndices();
			}
		});
	}
    {
		SCOPED_TIMER("ROAM::Upload");

		for (Patch& p: patches) {
			if (p.IsVisible(cam)){
                if (p.isChanged ){
                    p.Upload();
                    actualUploads++;}
			}
		}
	}
	#if TESSMODE_3_DEBUG
	if (actualTesselations> 0 || numPatchesEnterVisibility> 0 || numPatchesExitVisibility > 0 ){
        LOG("#Visible=%i delta_in=%i out=%i in df:#%i shadow:%i mthread?=%i actual_retess=%i up=%i skip=%i poolsize=%iK pooluse=%iK" ,
            numPatchesVisible,
            numPatchesEnterVisibility,
            numPatchesExitVisibility,
            globalRendering->drawFrame,
            shadowPass,
            useThreadTesselation[shadowPass],
            actualTesselations,
            actualUploads,
            numPatchesEnteredSkipped,
            patches[0].curTriPool->getPoolSize()/1024,
            patches[0].curTriPool->getNextTriNodeIdx()/1024);
	}
	#endif // TESSMODE_3_DEBUG


#else

	tesselMesh |= (lastGroundDetail[shadowPass] != smfGroundDrawer->GetGroundDetail());

	if (!tesselMesh)
		return;
    LOG("Retesselating %i delta in:%i out: %iin df :%i pass:%i threads:%i" , numPatchesVisible,numPatchesEnterVisibility, numPatchesExitVisibility, globalRendering->drawFrame, shadowPass, useThreadTesselation[shadowPass]);


    {
		SCOPED_TIMER("ROAM::Tessellate");

		Reset(shadowPass);
		Tessellate(patches, cam, smfGroundDrawer->GetGroundDetail(), useThreadTesselation[shadowPass], shadowPass);
	}

	{
		SCOPED_TIMER("ROAM::GenerateIndexArray");

		for_mt(0, patches.size(), [&patches, &cam](const int i) {
			Patch* p = &patches[i];

			if (p->IsVisible(cam))
				p->GenerateIndices();
		});
	}

	{
		SCOPED_TIMER("ROAM::Upload");

		for (Patch& p: patches) {
			if (p.IsVisible(cam))
				p.Upload();
		}
	}
#endif

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
	// DrawInMiniMap runs before DrawWorld
	globalRendering->drawFrame -= 1;

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
		if (p.IsVisible(CCameraHandler::GetActiveCamera()))
			continue;

		glRectf(p.coors.x, p.coors.y, p.coors.x + PATCH_SIZE, p.coors.y + PATCH_SIZE);
	}

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

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

