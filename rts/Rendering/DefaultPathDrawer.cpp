/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "StdAfx.h"

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

#include "Rendering/GlobalRendering.h"
#include "Rendering/DefaultPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "System/GlobalUnsynced.h"
#include "System/myMath.h"

DefaultPathDrawer::DefaultPathDrawer() {
	pm = dynamic_cast<CPathManager*>(pathManager);
}

void DefaultPathDrawer::Draw() const {
	// CPathManager is not thread-safe
	#if !defined(USE_GML) || !GML_ENABLE_SIM
	if (globalRendering->drawdebug && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT);

		Draw(pm);
		Draw(pm->maxResPF);
		Draw(pm->medResPE);
		Draw(pm->lowResPE);

		glPopAttrib();
	}
	#endif
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
			if (guihandler->commands[guihandler->inCommand].type != CMDTYPE_ICON_BUILDING) {
				useCurrentBuildOrder = false;
			}

			if (useCurrentBuildOrder) {
				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x < gs->hmapx; ++x) {
						const float3 pos(x * (SQUARE_SIZE << 1) + SQUARE_SIZE, 0.0f, y * (SQUARE_SIZE << 1) + SQUARE_SIZE);
						const int idx = ((y * (gs->pwr2mapx >> 1)) + x) * 4 - offset;
						float m = 0.0f;

						if (!loshandler->InLos(pos, gu->myAllyTeam)) {
							m = 0.25f;
						} else {
							const UnitDef* ud = unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
							const BuildInfo bi(ud, pos, guihandler->buildFacing);

							CFeature* f = NULL;

							GML_RECMUTEX_LOCK(quad); // UpdateExtraTexture - testunitbuildsquare accesses features in the quadfield

							if (uh->TestUnitBuildSquare(bi, f, gu->myAllyTeam)) {
								if (f != NULL) {
									m = 0.5f;
								} else {
									m = 1.0f;
								}
							} else {
								m = 0.0f;
							}
						}

						m = int(m * 255.0f);

						texMem[idx + CBaseGroundDrawer::COLOR_R] = 255 - m;
						texMem[idx + CBaseGroundDrawer::COLOR_G] = m;
						texMem[idx + CBaseGroundDrawer::COLOR_B] = 0;
						texMem[idx + CBaseGroundDrawer::COLOR_A] = 255;
					}
				}
			} else {
				const MoveData* md = NULL;

				const bool showBlockedMap = (gs->cheatEnabled || gu->spectating);
				const unsigned int blockMask = (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN);

				{
					GML_RECMUTEX_LOCK(sel); // UpdateExtraTexture

					const CUnitSet& selUnits = selectedUnits.selectedUnits;

					// use the first selected unit, if it has the ability to move
					if (!selUnits.empty()) {
						md = (*selUnits.begin())->unitDef->movedata;
					}
				}

				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x < gs->hmapx; ++x) {
						const int idx = ((y * (gs->pwr2mapx >> 1)) + x) * 4 - offset;

						if (md != NULL) {
							float m = md->moveMath->SpeedMod(*md, x << 1, y << 1);

							if (showBlockedMap && (md->moveMath->IsBlocked2(*md, (x << 1) + 1, (y << 1) + 1) & blockMask)) {
								m = 0.0f;
							}

							m = std::min(1.0f, fastmath::apxsqrt(m));
							m = int(m * 255.0f);

							texMem[idx + CBaseGroundDrawer::COLOR_R] = 255 - m;
							texMem[idx + CBaseGroundDrawer::COLOR_G] = m;
							texMem[idx + CBaseGroundDrawer::COLOR_B] = 0;
							texMem[idx + CBaseGroundDrawer::COLOR_A] = 255;
						} else {
							// we have nothing to show
							// -> draw a dark red overlay
							texMem[idx + CBaseGroundDrawer::COLOR_R] = 100;
							texMem[idx + CBaseGroundDrawer::COLOR_G] = 0;
							texMem[idx + CBaseGroundDrawer::COLOR_B] = 0;
							texMem[idx + CBaseGroundDrawer::COLOR_A] = 255;
						}
					}
				}
			}
		} break;

		case CBaseGroundDrawer::drawPathHeat: {
			for (int y = starty; y < endy; ++y) {
				for (int x = 0; x < gs->hmapx; ++x) {
					const int idx = ((y * (gs->pwr2mapx >> 1)) + x) * 4 - offset;

					texMem[idx + CBaseGroundDrawer::COLOR_R] = Clamp(8 * pm->GetHeatOnSquare(x << 1, y << 1), 32, 255);
					texMem[idx + CBaseGroundDrawer::COLOR_G] = 32;
					texMem[idx + CBaseGroundDrawer::COLOR_B] = 32;
					texMem[idx + CBaseGroundDrawer::COLOR_A] = 255;
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

					const PathNodeState& maxResNode = maxResStates[hy * gs->mapx + hx];
					const PathNodeState& medResNode = medResStates[(hy / medResBlockSize) * medResBlocksX + (hx / medResBlockSize)];
					const PathNodeState& lowResNode = lowResStates[(hy / lowResBlockSize) * lowResBlocksX + (hx / lowResBlockSize)];

					float gCost[3] = {
						maxResNode.gCost,
						medResNode.gCost,
						lowResNode.gCost,
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




void DefaultPathDrawer::Draw(const CPathManager* pm) const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(3);

	const std::map<unsigned int, CPathManager::MultiPath*>& pathMap = pm->pathMap;
	std::map<unsigned int, CPathManager::MultiPath*>::const_iterator pi;

	for (pi = pathMap.begin(); pi != pathMap.end(); ++pi) {
		const CPathManager::MultiPath* path = pi->second;

		glBegin(GL_LINE_STRIP);

			typedef IPath::path_list_type::const_iterator PathIt;

			// draw estimatePath2 (green)
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			for (PathIt pvi = path->lowResPath.path.begin(); pvi != path->lowResPath.path.end(); pvi++) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw estimatePath (blue)
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->medResPath.path.begin(); pvi != path->medResPath.path.end(); pvi++) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw detailPath (red)
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->maxResPath.path.begin(); pvi != path->maxResPath.path.end(); pvi++) {
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

		if (pf->squareStates[square].nodeMask & PATHOPT_START)
			continue;

		float3 p1;
			p1.x = sqr.x * SQUARE_SIZE;
			p1.z = sqr.y * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;
		float3 p2;

		const int dir = pf->squareStates[square].nodeMask & PATHOPT_DIRECTION;
		const int obx = sqr.x - pf->directionVector[dir].x;
		const int obz = sqr.y - pf->directionVector[dir].y;
		const int obsquare =  obz * gs->mapx + obx;

		if (obsquare >= 0) {
			p2.x = obx * SQUARE_SIZE;
			p2.z = obz * SQUARE_SIZE;
			p2.y = ground->GetHeight(p2.x, p2.z) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();
}



void DefaultPathDrawer::Draw(const CPathEstimator* pe) const {
	GML_RECMUTEX_LOCK(sel); // Draw

	MoveData* md = NULL;

	if (!moveinfo->moveData.empty()) {
		md = moveinfo->moveData[0];
	} else {
		return;
	}

	if (!selectedUnits.selectedUnits.empty()) {
		const CUnit* unit = (*selectedUnits.selectedUnits.begin());

		if (unit->unitDef->movedata) {
			md = unit->unitDef->movedata;
		}
	}

	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 0.0f);

	/*
	float blue = BLOCK_SIZE == 32? 1: 0;
	glBegin(GL_LINES);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * 8;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * 8;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			glVertexf3(p1);
			glVertexf3(p1 - UpVector * 10);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * 8;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * 8;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1 / (sqrt(cost/BLOCK_SIZE)), 1 / (cost/BLOCK_SIZE), blue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

	}
	glEnd();


	glEnable(GL_TEXTURE_2D);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1, 1 / (cost/BLOCK_SIZE), blue);

					p2 = (p1 + p2) / 2;
					if (camera->pos.SqDistance(p2) < 250000) {
						font->glWorldPrint(p2,5,"%.0f", cost);
					}
				}
			}
		}
	}
	*/


	if (pe->BLOCK_SIZE == 8) {
		glColor3f(0.2f, 0.7f, 0.2f);
	} else {
		glColor3f(0.2f, 0.2f, 0.7f);
	}

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);

	for (unsigned int idx = 0; idx < pe->openBlockBuffer.GetSize(); idx++) {
		const PathNode* ob = pe->openBlockBuffer.GetNode(idx);
		const int blocknr = ob->nodeNum;

		float3 p1;
			p1.x = (pe->blockStates[blocknr].nodeOffsets[md->pathType].x) * SQUARE_SIZE;
			p1.z = (pe->blockStates[blocknr].nodeOffsets[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;
		float3 p2;

		const int obx = pe->blockStates[ob->nodeNum].parentNodePos.x;
		const int obz = pe->blockStates[ob->nodeNum].parentNodePos.y;
		const int obblocknr = obz * pe->nbrOfBlocksX + obx;

		if (obblocknr >= 0) {
			p2.x = (pe->blockStates[obblocknr].nodeOffsets[md->pathType].x) * SQUARE_SIZE;
			p2.z = (pe->blockStates[obblocknr].nodeOffsets[md->pathType].y) * SQUARE_SIZE;
			p2.y = ground->GetHeight(p2.x, p2.z) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();


	/*
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1,0, blue, 0.7f);
	glAlphaFunc(GL_GREATER, 0.05f);

	for (const CPathEstimator::OpenBlock* ob = pe->openBlockBuffer; ob != pe->openBlockBufferPointer; ++ob) {
		const int blocknr = ob->blocknr;
		float3 p1;
			p1.x = (ob->block.x * BLOCK_SIZE + pe->blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (ob->block.y * BLOCK_SIZE + pe->blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;

		if (camera->pos.SqDistance(p1) < 250000) {
			font->glWorldPrint(p1, 5, "%.0f %.0f", ob->cost, ob->currentCost);
		}
	}
	glDisable(GL_BLEND);
	*/
}
