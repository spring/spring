/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/GuiHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"

// FIXME
#define private public
#include "Sim/Path/Default/IPath.h"
#include "Sim/Path/Default/PathFinder.h"
#include "Sim/Path/Default/PathFinderDef.h"
#include "Sim/Path/Default/PathEstimator.h"
#include "Sim/Path/Default/PathManager.h"
#undef private

#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/DefaultPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "System/myMath.h"
#include "System/Color.h"
#include "System/Util.h"

#define PE_EXTRA_DEBUG_OVERLAYS 1

static CPathManager* pm = NULL;

static const MoveData* GetMoveData() {
	const MoveData* md = NULL;
	const CUnitSet& unitSet = selectedUnits.selectedUnits;

	if (moveinfo->moveData.empty()) {
		return md;
	}

	md = moveinfo->moveData[0];

	if (!unitSet.empty()) {
		const CUnit* unit = *(unitSet.begin());
		const UnitDef* unitDef = unit->unitDef;

		if (unitDef->movedata != NULL) {
			md = unitDef->movedata;
		}
	}

	return md;
}

static inline float GetSpeedMod(const MoveData* md, const CMoveMath* mm, int sqx, int sqy) {
	float m = 0.0f;

	#if 0
	const int hmIdx = sqy * gs->mapxp1 + sqx;
	const int cnIdx = sqy * gs->mapx   + sqx;

	const float height = hm[hmIdx];
	const float slope = 1.0f - cn[cnIdx].y;

	if (md->moveFamily == MoveData::Ship) {
		// only check water depth
		m = (height >= (-md->depth))? 0.0f: m;
	} else {
		// check depth and slope (if hover, only over land)
		m = std::max(0.0f, 1.0f - (slope / (md->maxSlope + 0.1f)));
		m = (height < (-md->depth))? 0.0f: m;
		m = (height <= 0.0f && md->moveFamily == MoveData::Hover)? 1.0f: m;
	}
	#else
	// NOTE: these values are not necessarily in [0, 1]
	m = mm->GetPosSpeedMod(md, sqx, sqy);
	#endif

	return m;
}

static inline SColor GetSpeedModColor(const float& m) {
	SColor col(120, 0, 80);

	if (m > 0.0f) {
		col.r = 255 - ((m <= 1.0f) ? (m * 255) : 255);
		col.g = 255 - col.r;
		col.b =   0;
	}

	return col;
}



static const SColor buildColors[] = {
	SColor(138, 138, 138), // nolos
	SColor(  0, 210,   0), // free
	SColor(190, 180,   0), // objblocked
	SColor(210,   0,   0), // terrainblocked
};

static inline const SColor& GetBuildColor(const DefaultPathDrawer::BuildSquareStatus& status) {
	return buildColors[status];
}




DefaultPathDrawer::DefaultPathDrawer() {
	pm = dynamic_cast<CPathManager*>(pathManager);
}

void DefaultPathDrawer::DrawAll() const {
	// CPathManager is not thread-safe
	if (!GML::SimEnabled() && globalRendering->drawdebug && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT);

		Draw();
		Draw(pm->maxResPF);
		Draw(pm->medResPE);
		Draw(pm->lowResPE);

		glPopAttrib();
	}
}



void DefaultPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CBaseGroundDrawer::drawPathTraversability: {
			bool useCurrentBuildOrder = true;

			if (guihandler->inCommand <= 0) {
				useCurrentBuildOrder = false;
			}
			if (guihandler->inCommand >= guihandler->commands.size()) {
				useCurrentBuildOrder = false;
			}
			if (useCurrentBuildOrder && guihandler->commands[guihandler->inCommand].type != CMDTYPE_ICON_BUILDING) {
				useCurrentBuildOrder = false;
			}

			if (useCurrentBuildOrder) {
				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < gs->hmapx; ++tx) {
						const float3 pos(tx * (SQUARE_SIZE << 1) + SQUARE_SIZE, 0.0f, ty * (SQUARE_SIZE << 1) + SQUARE_SIZE);
						const int idx = ((ty * (gs->pwr2mapx >> 1)) + tx) * 4 - offset;

						BuildSquareStatus status = FREE;

						if (!loshandler->InLos(pos, gu->myAllyTeam)) {
							status = NOLOS;
						} else {
							const UnitDef* ud = unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
							const BuildInfo bi(ud, pos, guihandler->buildFacing);

							CFeature* f = NULL;

							GML_RECMUTEX_LOCK(quad); // UpdateExtraTexture - testunitbuildsquare accesses features in the quadfield

							if (uh->TestUnitBuildSquare(bi, f, gu->myAllyTeam, false)) {
								if (f != NULL) {
									status = OBJECTBLOCKED;
								}
							} else {
								status = TERRAINBLOCKED;
							}
						}

						const SColor& col = GetBuildColor(status);
						texMem[idx + CBaseGroundDrawer::COLOR_R] = col.r;
						texMem[idx + CBaseGroundDrawer::COLOR_G] = col.g;
						texMem[idx + CBaseGroundDrawer::COLOR_B] = col.b;
						texMem[idx + CBaseGroundDrawer::COLOR_A] = col.a;
					}
				}
			} else {
				const MoveData* md = NULL;
				const CMoveMath* mm = NULL;
				const bool los = (gs->cheatEnabled || gu->spectating);

				{
					GML_RECMUTEX_LOCK(sel); // UpdateExtraTexture

					const CUnitSet& selUnits = selectedUnits.selectedUnits;
					const CUnit* selUnit = NULL;

					// use the first selected unit, if it has the ability to move
					if (!selUnits.empty()) {
						selUnit = *selUnits.begin();
						md = selUnit->unitDef->movedata;
						mm = (md != NULL)? md->moveMath: NULL;
					}
				}

				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < gs->hmapx; ++tx) {
						const int sqx = (tx << 1);
						const int sqy = (ty << 1);
						const int texIdx = ((ty * (gs->pwr2mapx >> 1)) + tx) * 4 - offset;

						if (md != NULL) {
							float s = 1.0f;

							if (los || loshandler->InLos(sqx, sqy, gu->myAllyTeam)) {
								if (mm->IsBlocked(*md, sqx,     sqy    ) & CMoveMath::BLOCK_STRUCTURE) { s -= 0.25f; }
								if (mm->IsBlocked(*md, sqx + 1, sqy    ) & CMoveMath::BLOCK_STRUCTURE) { s -= 0.25f; }
								if (mm->IsBlocked(*md, sqx,     sqy + 1) & CMoveMath::BLOCK_STRUCTURE) { s -= 0.25f; }
								if (mm->IsBlocked(*md, sqx + 1, sqy + 1) & CMoveMath::BLOCK_STRUCTURE) { s -= 0.25f; }
							}

							const float& m  = GetSpeedMod(md, mm, sqx, sqy);
							const SColor& c = GetSpeedModColor(m * s);

							texMem[texIdx + CBaseGroundDrawer::COLOR_R] = c.r;
							texMem[texIdx + CBaseGroundDrawer::COLOR_G] = c.g;
							texMem[texIdx + CBaseGroundDrawer::COLOR_B] = c.b;
							texMem[texIdx + CBaseGroundDrawer::COLOR_A] = c.a;
						} else {
							// we have nothing to show
							// -> draw a dark red overlay
							texMem[texIdx + CBaseGroundDrawer::COLOR_R] = 100;
							texMem[texIdx + CBaseGroundDrawer::COLOR_G] = 0;
							texMem[texIdx + CBaseGroundDrawer::COLOR_B] = 0;
							texMem[texIdx + CBaseGroundDrawer::COLOR_A] = 255;
						}
					}
				}
			}
		} break;

		case CBaseGroundDrawer::drawPathHeat: {
			for (int ty = starty; ty < endy; ++ty) {
				for (int tx = 0; tx < gs->hmapx; ++tx) {
					const int texIdx = ((ty * (gs->pwr2mapx >> 1)) + tx) * 4 - offset;

					texMem[texIdx + CBaseGroundDrawer::COLOR_R] = Clamp(8 * pm->GetHeatOnSquare(tx << 1, ty << 1), 32, 255);
					texMem[texIdx + CBaseGroundDrawer::COLOR_G] = 32;
					texMem[texIdx + CBaseGroundDrawer::COLOR_B] = 32;
					texMem[texIdx + CBaseGroundDrawer::COLOR_A] = 255;
				}
			}
		} break;

		case CBaseGroundDrawer::drawPathCost: {
			const PathNodeStateBuffer& maxResStates = pm->maxResPF->squareStates;
			const PathNodeStateBuffer& medResStates = pm->medResPE->blockStates;
			const PathNodeStateBuffer& lowResStates = pm->lowResPE->blockStates;

			const unsigned int medResBlockSize = pm->medResPE->BLOCK_SIZE, medResBlocksX = pm->medResPE->nbrOfBlocksX;
			const unsigned int lowResBlockSize = pm->lowResPE->BLOCK_SIZE, lowResBlocksX = pm->lowResPE->nbrOfBlocksX;

			const float gCostMax[3] = {
				std::max(1.0f, maxResStates.GetMaxGCost()),
				std::max(1.0f, medResStates.GetMaxGCost()),
				std::max(1.0f, lowResStates.GetMaxGCost()),
			};

			for (int ty = starty; ty < endy; ++ty) {
				for (int tx = 0; tx < gs->hmapx; ++tx) {
					const unsigned int texIdx = ((ty * (gs->pwr2mapx >> 1)) + tx) * 4 - offset;
					// NOTE:
					//    tx is in [0, gs->hmapx>
					//    ty is in [0, gs->hmapy> (highResInfoTexWanted == false)
					const unsigned int hx = tx << 1;
					const unsigned int hy = ty << 1;

					float gCost[3] = {
						maxResStates.gCost[hy * gs->mapx + hx],
						medResStates.gCost[(hy / medResBlockSize) * medResBlocksX + (hx / medResBlockSize)],
						lowResStates.gCost[(hy / lowResBlockSize) * lowResBlocksX + (hx / lowResBlockSize)],
					};

					if (math::isinf(gCost[0])) { gCost[0] = gCostMax[0]; }
					if (math::isinf(gCost[1])) { gCost[1] = gCostMax[1]; }
					if (math::isinf(gCost[2])) { gCost[2] = gCostMax[2]; }

					// NOTE:
					//     the normalisation means each extraTextureUpdate block
					//     of rows gets assigned different colors when units are
					//     moving (so view it while paused)
					texMem[texIdx + CBaseGroundDrawer::COLOR_R] = (gCost[0] / gCostMax[0]) * 255;
					texMem[texIdx + CBaseGroundDrawer::COLOR_G] = (gCost[1] / gCostMax[1]) * 255;
					texMem[texIdx + CBaseGroundDrawer::COLOR_B] = (gCost[2] / gCostMax[2]) * 255;
					texMem[texIdx + CBaseGroundDrawer::COLOR_A] = 255;
				}
			}
		} break;

		default: {
		} break;
	}
}




void DefaultPathDrawer::Draw() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(3);

	const std::map<unsigned int, CPathManager::MultiPath*>& pathMap = pm->pathMap;
	std::map<unsigned int, CPathManager::MultiPath*>::const_iterator pi;

	for (pi = pathMap.begin(); pi != pathMap.end(); ++pi) {
		const CPathManager::MultiPath* path = pi->second;

		glBegin(GL_LINE_STRIP);

			typedef IPath::path_list_type::const_iterator PathIt;

			// draw low-res segments of <path> (green)
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			for (PathIt pvi = path->lowResPath.path.begin(); pvi != path->lowResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw med-res segments of <path> (blue)
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->medResPath.path.begin(); pvi != path->medResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw max-res segments of <path> (red)
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->maxResPath.path.begin(); pvi != path->maxResPath.path.end(); ++pvi) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

		glEnd();

		// visualize the path definition (goal, radius)
		Draw(path->peDef);
	}

	glLineWidth(1);
}



void DefaultPathDrawer::Draw(const CPathFinderDef* pfd) const {
	glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	glSurfaceCircle(pfd->goal, sqrt(pfd->sqGoalRadius), 20);
}

void DefaultPathDrawer::Draw(const CPathFinder* pf) const {
	glColor3f(0.7f, 0.2f, 0.2f);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);

	for (unsigned int idx = 0; idx < pf->openSquareBuffer.GetSize(); idx++) {
		const PathNode* os = pf->openSquareBuffer.GetNode(idx);
		const int2 sqr = os->nodePos;
		const int square = os->nodeNum;

		if (pf->squareStates.nodeMask[square] & PATHOPT_START)
			continue;

		float3 p1;
			p1.x = sqr.x * SQUARE_SIZE;
			p1.z = sqr.y * SQUARE_SIZE;
			p1.y = ground->GetHeightAboveWater(p1.x, p1.z, false) + 15;
		float3 p2;

		const int dir = pf->squareStates.nodeMask[square] & PATHOPT_DIRECTION;
		const int obx = sqr.x - pf->directionVector[dir].x;
		const int obz = sqr.y - pf->directionVector[dir].y;
		const int obsquare =  obz * gs->mapx + obx;

		if (obsquare >= 0) {
			p2.x = obx * SQUARE_SIZE;
			p2.z = obz * SQUARE_SIZE;
			p2.y = ground->GetHeightAboveWater(p2.x, p2.z, false) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();
}



void DefaultPathDrawer::Draw(const CPathEstimator* pe) const {
	GML_RECMUTEX_LOCK(sel); // Draw

	const MoveData* md = GetMoveData();
	const PathNodeStateBuffer& blockStates = pe->blockStates;

	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 0.0f);

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	const int overlayPeriod = GAME_SPEED * 5;
	const int overlayNumber = (gs->frameNum % (overlayPeriod * 3)) / overlayPeriod;
	const bool extraOverlay =
		(overlayNumber == 0 && pe == pm->medResPE) ||
		(overlayNumber == 1 && pe == pm->lowResPE);
	const int peNumBlocks = pe->nbrOfBlocksX * pe->nbrOfBlocksZ;
	const float peBlueValue = (pe == pm->lowResPE)? 1.0f: 0.0f;

	// alternate between the extra debug-overlays
	// (normally TMI, but useful to keep the code
	// compiling)
	if (extraOverlay && false) {
		glBegin(GL_LINES);

		for (int z = 0; z < pe->nbrOfBlocksZ; z++) {
			for (int x = 0; x < pe->nbrOfBlocksX; x++) {
				const int blockNr = z * pe->nbrOfBlocksX + x;

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[blockNr][md->pathType].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[blockNr][md->pathType].y) * SQUARE_SIZE;
					p1.y = ground->GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				glColor3f(1.0f, 1.0f, peBlueValue);
				glVertexf3(p1);
				glVertexf3(p1 - UpVector * 10.0f);

				for (int dir = 0; dir < CPathEstimator::PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + pe->directionVector[dir].x;
					const int obz = z + pe->directionVector[dir].y;

					if (obx <                 0) continue;
					if (obz <                 0) continue;
					if (obx >= pe->nbrOfBlocksX) continue;
					if (obz >= pe->nbrOfBlocksZ) continue;

					const int obBlockNr = obz * pe->nbrOfBlocksX + obx;
					const int vertexNr =
						md->pathType * (peNumBlocks) * CPathEstimator::PATH_DIRECTION_VERTICES +
						blockNr * CPathEstimator::PATH_DIRECTION_VERTICES + pe->directionVertex[dir];
					const float cost = pe->vertices[vertexNr] / pe->BLOCK_SIZE;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[obBlockNr][md->pathType].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[obBlockNr][md->pathType].y) * SQUARE_SIZE;
						p2.y = ground->GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					glColor3f(1.0f / math::sqrtf(cost), 1.0f / cost, peBlueValue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

		glEnd();

		for (int z = 0; z < pe->nbrOfBlocksZ; z++) {
			for (int x = 0; x < pe->nbrOfBlocksX; x++) {
				const int blockNr = z * pe->nbrOfBlocksX + x;

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[blockNr][md->pathType].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[blockNr][md->pathType].y) * SQUARE_SIZE;
					p1.y = ground->GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				for (int dir = 0; dir < CPathEstimator::PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + pe->directionVector[dir].x;
					const int obz = z + pe->directionVector[dir].y;

					if (obx <                 0) continue;
					if (obz <                 0) continue;
					if (obx >= pe->nbrOfBlocksX) continue;
					if (obz >= pe->nbrOfBlocksZ) continue;

					const int obBlockNr = obz * pe->nbrOfBlocksX + obx;
					const int vertexNr =
						md->pathType * (peNumBlocks) * CPathEstimator::PATH_DIRECTION_VERTICES +
						blockNr * CPathEstimator::PATH_DIRECTION_VERTICES + pe->directionVertex[dir];
					const float cost = pe->vertices[vertexNr] / pe->BLOCK_SIZE;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[obBlockNr][md->pathType].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[obBlockNr][md->pathType].y) * SQUARE_SIZE;
						p2.y = ground->GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					p2 = (p1 + p2) / 2.0f;

					if (camera->pos.SqDistance(p2) >= (4000.0f * 4000.0f))
						continue;

					font->SetTextColor(1.0f, 1.0f / cost, peBlueValue, 1.0f);
					font->glWorldPrint(p2, 5.0f, FloatToString(cost, "f(%.2f)"));
				}
			}
		}
	}
	#endif


	if (pe == pm->medResPE) {
		glColor3f(0.2f, 0.7f, 0.2f);
	} else {
		glColor3f(0.2f, 0.2f, 0.7f);
	}

	{
		glBegin(GL_LINES);
		for (unsigned int idx = 0; idx < pe->openBlockBuffer.GetSize(); idx++) {
			const PathNode* ob = pe->openBlockBuffer.GetNode(idx);
			const int blockNr = ob->nodeNum;

			const int obx = blockStates.peParentNodePos[ob->nodeNum].x;
			const int obz = blockStates.peParentNodePos[ob->nodeNum].y;
			const int obBlockNr = obz * pe->nbrOfBlocksX + obx;

			if (obBlockNr < 0)
				continue;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[blockNr][md->pathType].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[blockNr][md->pathType].y) * SQUARE_SIZE;
				p1.y = ground->GetHeightAboveWater(p1.x, p1.z, false) + 15.0f;
			float3 p2;
				p2.x = (blockStates.peNodeOffsets[obBlockNr][md->pathType].x) * SQUARE_SIZE;
				p2.z = (blockStates.peNodeOffsets[obBlockNr][md->pathType].y) * SQUARE_SIZE;
				p2.y = ground->GetHeightAboveWater(p2.x, p2.z, false) + 15.0f;

			glVertexf3(p1);
			glVertexf3(p2);
		}
		glEnd();
	}

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	if (extraOverlay && false) {
		const PathNodeBuffer& openBlockBuffer = pe->openBlockBuffer;
		char blockCostsStr[32];

		for (unsigned int blockIdx = 0; blockIdx < openBlockBuffer.GetSize(); blockIdx++) {
			const PathNode* ob = openBlockBuffer.GetNode(blockIdx);
			const int blockNr = ob->nodeNum;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[blockNr][md->pathType].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[blockNr][md->pathType].y) * SQUARE_SIZE;
				p1.y = ground->GetHeightAboveWater(p1.x, p1.z, false) + 35.0f;

			if (camera->pos.SqDistance(p1) >= (4000.0f * 4000.0f))
				continue;

			SNPRINTF(blockCostsStr, sizeof(blockCostsStr), "f(%.2f) g(%.2f)", ob->fCost, ob->gCost);
			font->SetTextColor(1.0f, 0.7f, peBlueValue, 1.0f);
			font->glWorldPrint(p1, 5.0f, blockCostsStr);
		}
	}
	#endif
}

