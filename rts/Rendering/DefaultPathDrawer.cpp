/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"

// FIXME
#define private public
#include "Sim/Path/Default/IPath.h"
#include "Sim/Path/Default/PathFinder.h"
#include "Sim/Path/Default/PathFinderDef.h"
#include "Sim/Path/Default/PathEstimator.h"
#include "Sim/Path/Default/PathManager.h"
#include "Sim/Path/Default/PathHeatMap.hpp"
#include "Sim/Path/Default/PathFlowMap.hpp"
#undef private

#include "Rendering/Fonts/glFont.h"
#include "Rendering/DefaultPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Map/InfoTexture/Legacy/LegacyInfoTextureHandler.h"
#include "System/myMath.h"
#include "System/Util.h"

#define PE_EXTRA_DEBUG_OVERLAYS 1

static const SColor buildColors[] = {
	SColor(138, 138, 138), // nolos
	SColor(  0, 210,   0), // free
	SColor(190, 180,   0), // objblocked
	SColor(210,   0,   0), // terrainblocked
};

static inline const SColor& GetBuildColor(const DefaultPathDrawer::BuildSquareStatus& status) {
	return buildColors[status];
}




DefaultPathDrawer::DefaultPathDrawer(): IPathDrawer()
{
	pm = dynamic_cast<CPathManager*>(pathManager);
}

void DefaultPathDrawer::DrawAll() const {
	// CPathManager is not thread-safe
	if (enabled && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT);

		Draw();
		Draw(pm->maxResPF);
		Draw(pm->medResPE);
		Draw(pm->lowResPE);

		glPopAttrib();
	}
}


void DefaultPathDrawer::DrawInMiniMap()
{
	const CPathEstimator* pe = pm->medResPE;

	if (!IsEnabled())
		return;

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

	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 0.0f, 0.7f);

	for (const int2& sb: pe->updatedBlocks) {
		const int blockIdxX = sb.x * pe->GetBlockSize();
		const int blockIdxY = sb.y * pe->GetBlockSize();
		glRectf(blockIdxX, blockIdxY, blockIdxX + pe->GetBlockSize(), blockIdxY + pe->GetBlockSize());
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
}


void DefaultPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CLegacyInfoTextureHandler::drawPathTrav: {
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
					for (int tx = 0; tx < mapDims.hmapx; ++tx) {
						const float3 pos(tx * (SQUARE_SIZE << 1) + SQUARE_SIZE, 0.0f, ty * (SQUARE_SIZE << 1) + SQUARE_SIZE);
						const int idx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;

						BuildSquareStatus status = FREE;

						if (!losHandler->InLos(pos, gu->myAllyTeam)) {
							status = NOLOS;
						} else {
							const UnitDef* ud = unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
							const BuildInfo bi(ud, pos, guihandler->buildFacing);

							CFeature* f = NULL;

							if (CGameHelper::TestUnitBuildSquare(bi, f, gu->myAllyTeam, false)) {
								if (f != NULL) {
									status = OBJECTBLOCKED;
								}
							} else {
								status = TERRAINBLOCKED;
							}
						}

						const SColor& col = GetBuildColor(status);
						texMem[idx + CLegacyInfoTextureHandler::COLOR_R] = col.r;
						texMem[idx + CLegacyInfoTextureHandler::COLOR_G] = col.g;
						texMem[idx + CLegacyInfoTextureHandler::COLOR_B] = col.b;
						texMem[idx + CLegacyInfoTextureHandler::COLOR_A] = col.a;
					}
				}
			} else {
				const MoveDef* md = GetSelectedMoveDef();

				if (md != NULL) {
					const bool los = (gs->cheatEnabled || gu->spectating);

					for (int ty = starty; ty < endy; ++ty) {
						for (int tx = 0; tx < mapDims.hmapx; ++tx) {
							const int sqx = (tx << 1);
							const int sqy = (ty << 1);
							const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
							const bool losSqr = losHandler->InLos(sqx, sqy, gu->myAllyTeam);

							float scale = 1.0f;

							if (los || losSqr) {
								if (CMoveMath::IsBlocked(*md, sqx,     sqy    , NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx + 1, sqy    , NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx,     sqy + 1, NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx + 1, sqy + 1, NULL) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							}

							// NOTE: raw speedmods are not necessarily clamped to [0, 1]
							const float sm = CMoveMath::GetPosSpeedMod(*md, sqx, sqy);
							const SColor& smc = GetSpeedModColor(sm * scale);

							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = smc.r;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = smc.g;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = smc.b;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = smc.a;
						}
					}
				} else {
					// we have nothing to show -> draw a dark red overlay
					for (int ty = starty; ty < endy; ++ty) {
						for (int tx = 0; tx < mapDims.hmapx; ++tx) {
							const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;

							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = 100;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = 0;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = 0;
							texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = 255;
						}
					}
				}
			}
		} break;

		case CLegacyInfoTextureHandler::drawPathHeat: {
			const PathHeatMap* phm = pm->pathHeatMap;

			for (int ty = starty; ty < endy; ++ty) {
				for (int tx = 0; tx < mapDims.hmapx; ++tx) {
					const unsigned int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;

					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = Clamp(8 * phm->GetHeatValue(tx << 1, ty << 1), 32, 255);
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = 32;
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = 32;
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = 255;
				}
			}
		} break;

		case CLegacyInfoTextureHandler::drawPathFlow: {
			const PathFlowMap* pfm = pm->pathFlowMap;
			const float maxFlow = pfm->GetMaxFlow();

			if (maxFlow > 0.0f) {
				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < mapDims.hmapx; ++tx) {
						const unsigned int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
						const float3& flow = pfm->GetFlowVec(tx << 1, ty << 1);

						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = (((flow.x + 1.0f) * 0.5f) * 255);
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = (((flow.z + 1.0f) * 0.5f) * 255);
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = (( flow.y               ) * 255);
						texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = 255;
					}
				}
			}
		} break;

		case CLegacyInfoTextureHandler::drawPathCost: {
			const PathNodeStateBuffer& maxResStates = pm->maxResPF->blockStates;
			const PathNodeStateBuffer& medResStates = pm->medResPE->blockStates;
			const PathNodeStateBuffer& lowResStates = pm->lowResPE->blockStates;

			const unsigned int medResBlockSize = pm->medResPE->BLOCK_SIZE, medResBlocksX = pm->medResPE->GetNumBlocks().x;
			const unsigned int lowResBlockSize = pm->lowResPE->BLOCK_SIZE, lowResBlocksX = pm->lowResPE->GetNumBlocks().x;

			const float gCostMax[3] = {
				std::max(1.0f, maxResStates.GetMaxCost(NODE_COST_G)),
				std::max(1.0f, medResStates.GetMaxCost(NODE_COST_G)),
				std::max(1.0f, lowResStates.GetMaxCost(NODE_COST_G)),
			};

			for (int ty = starty; ty < endy; ++ty) {
				for (int tx = 0; tx < mapDims.hmapx; ++tx) {
					const unsigned int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
					// NOTE:
					//    tx is in [0, mapDims.hmapx>
					//    ty is in [0, mapDims.hmapy> (highResInfoTexWanted == false)
					const unsigned int hx = tx << 1;
					const unsigned int hy = ty << 1;

					float gCost[3] = {
						maxResStates.gCost[hy * mapDims.mapx + hx],
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
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_R] = (gCost[0] / gCostMax[0]) * 255;
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_G] = (gCost[1] / gCostMax[1]) * 255;
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_B] = (gCost[2] / gCostMax[2]) * 255;
					texMem[texIdx + CLegacyInfoTextureHandler::COLOR_A] = 255;
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
	if (pfd->synced) {
		glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	} else {
		glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	}
	glSurfaceCircle(pfd->goal, math::sqrt(pfd->sqGoalRadius), 20);
}

void DefaultPathDrawer::Draw(const CPathFinder* pf) const {
	glColor3f(0.7f, 0.2f, 0.2f);
	glDisable(GL_TEXTURE_2D);
	CVertexArray* va = GetVertexArray();
	va->Initialize();

	for (unsigned int idx = 0; idx < pf->openBlockBuffer.GetSize(); idx++) {
		const PathNode* os = pf->openBlockBuffer.GetNode(idx);
		const int2 sqr = os->nodePos;
		const int square = os->nodeNum;
		float3 p1;
			p1.x = sqr.x * SQUARE_SIZE;
			p1.z = sqr.y * SQUARE_SIZE;
			p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 15.0f;

		const unsigned int dir = pf->blockStates.nodeMask[square] & PATHOPT_CARDINALS;
		const int2 obp = sqr - (CPathFinder::GetDirectionVectorsTable2D())[dir];
		float3 p2;
			p2.x = obp.x * SQUARE_SIZE;
			p2.z = obp.y * SQUARE_SIZE;
			p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 15.0f;

		if (!camera->InView(p1) && !camera->InView(p2))
			continue;

		va->AddVertex0(p1);
		va->AddVertex0(p2);
	}

	va->DrawArray0(GL_LINES);
}



void DefaultPathDrawer::Draw(const CPathEstimator* pe) const {
	const MoveDef* md = GetSelectedMoveDef();
	const PathNodeStateBuffer& blockStates = pe->blockStates;

	if (md == NULL)
		return;

	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 0.0f);

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	const int overlayPeriod = GAME_SPEED * 5;
	const int overlayNumber = (gs->frameNum % (overlayPeriod * 2)) / overlayPeriod;
	const bool extraOverlay =
		(overlayNumber == 0 && pe == pm->medResPE) ||
		(overlayNumber == 1 && pe == pm->lowResPE);
	const int peNumBlocks = pe->GetNumBlocks().x * pe->GetNumBlocks().y;
	const float peBlueValue = (pe == pm->lowResPE)? 0.75f: 0.0f;

	// alternate between the extra debug-overlays
	// (normally TMI, but useful to keep the code
	// compiling)
	if (extraOverlay) {
		glBegin(GL_LINES);

		for (int z = 0; z < pe->GetNumBlocks().y; z++) {
			for (int x = 0; x < pe->GetNumBlocks().x; x++) {
				const int blockNr = pe->BlockPosToIdx(int2(x,z));

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				glColor3f(1.0f, 1.0f, peBlueValue);
				glVertexf3(p1);
				glVertexf3(p1 - UpVector * 10.0f);

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + (CPathEstimator::GetDirectionVectorsTable())[dir].x;
					const int obz = z + (CPathEstimator::GetDirectionVectorsTable())[dir].y;

					if (obx <                 0) continue;
					if (obz <                 0) continue;
					if (obx >= pe->GetNumBlocks().x) continue;
					if (obz >= pe->GetNumBlocks().y) continue;

					const int obBlockNr = obz * pe->GetNumBlocks().x + obx;
					const int vertexNr =
						md->pathType * peNumBlocks * PATH_DIRECTION_VERTICES +
						blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, pe->GetNumBlocks().x);
					const float cost = pe->vertexCosts[vertexNr] / pe->BLOCK_SIZE;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					glColor3f(1.0f / math::sqrt(cost), 1.0f / cost, peBlueValue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

		glEnd();

		for (int z = 0; z < pe->GetNumBlocks().y; z++) {
			for (int x = 0; x < pe->GetNumBlocks().x; x++) {
				const int blockNr = z * pe->GetNumBlocks().x + x;

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + (CPathEstimator::GetDirectionVectorsTable())[dir].x;
					const int obz = z + (CPathEstimator::GetDirectionVectorsTable())[dir].y;

					if (obx <                 0) continue;
					if (obz <                 0) continue;
					if (obx >= pe->GetNumBlocks().x) continue;
					if (obz >= pe->GetNumBlocks().y) continue;

					const int obBlockNr = obz * pe->GetNumBlocks().x + obx;
					const int vertexNr =
						md->pathType * peNumBlocks * PATH_DIRECTION_VERTICES +
						blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, pe->GetNumBlocks().x);
					const float cost = pe->vertexCosts[vertexNr] / pe->BLOCK_SIZE;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					p2 = (p1 + p2) / 2.0f;

					if (!camera->InView(p2))
						continue;
					if (camera->GetPos().SqDistance(p2) >= (4000.0f * 4000.0f))
						continue;

					font->SetTextColor(1.0f, 1.0f / cost, peBlueValue, 1.0f);
					font->glWorldPrint(p2, 5.0f, FloatToString(cost, "f(%.2f)"));
				}
			}
		}
	}
	#endif


	if (pe == pm->medResPE) {
		glColor3f(0.7f, 0.2f, 0.7f);
	} else {
		glColor3f(0.2f, 0.7f, 0.7f);
	}

	{
		CVertexArray* va = GetVertexArray();
		va->Initialize();

		for (unsigned int idx = 0; idx < pe->openBlockBuffer.GetSize(); idx++) {
			const PathNode* ob = pe->openBlockBuffer.GetNode(idx);
			const int blockNr = ob->nodeNum;

			auto pathOptDir = blockStates.nodeMask[blockNr] & PATHOPT_CARDINALS;
			auto pathDir = PathOpt2PathDir(pathOptDir);
			const int2 obp = pe->BlockIdxToPos(blockNr) - pe->PE_DIRECTION_VECTORS[pathDir];
			const int obBlockNr = pe->BlockPosToIdx(obp);

			if (obBlockNr < 0)
				continue;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
				p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 15.0f;
			float3 p2;
				p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
				p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
				p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 15.0f;

			if (!camera->InView(p1) && !camera->InView(p2))
				continue;

			va->AddVertex0(p1);
			va->AddVertex0(p2);
		}
		va->DrawArray0(GL_LINES);
	}

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	if (extraOverlay && false) {
		const PathNodeBuffer& openBlockBuffer = pe->openBlockBuffer;
		char blockCostsStr[32];

		for (unsigned int blockIdx = 0; blockIdx < openBlockBuffer.GetSize(); blockIdx++) {
			const PathNode* ob = openBlockBuffer.GetNode(blockIdx);
			const int blockNr = ob->nodeNum;

			float3 p1;
				p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
				p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
				p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 35.0f;

			if (!camera->InView(p1))
				continue;
			if (camera->GetPos().SqDistance(p1) >= (4000.0f * 4000.0f))
				continue;

			SNPRINTF(blockCostsStr, sizeof(blockCostsStr), "f(%.2f) g(%.2f)", ob->fCost, ob->gCost);
			font->SetTextColor(1.0f, 0.7f, peBlueValue, 1.0f);
			font->glWorldPrint(p1, 5.0f, blockCostsStr);
		}
	}
	#endif
}

