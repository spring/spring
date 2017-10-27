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
#include "Sim/Units/CommandAI/CommandDescription.h"
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
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/myMath.h"
#include "System/StringUtil.h"

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

		Draw(); // draw paths and goals
		Draw(pm->maxResPF); // draw PF grid-overlay
		Draw(pm->medResPE); // draw PE grid-overlay (med-res)
		Draw(pm->lowResPE); // draw PE grid-overlay (low-res)

		glPopAttrib();
	}
}


void DefaultPathDrawer::DrawInMiniMap()
{
	const CPathEstimator* pe = pm->medResPE;

	if (!IsEnabled())
		return;

	glSpringMatrix2dSetupVP(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f,  true, true);
	minimap->ApplyConstraintsMatrix();

	glMatrixMode(GL_MODELVIEW);
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

	glSpringMatrix2dResetPV(true, true);
}


#if 0
// part of LegacyInfoTexHandler, no longer called
void DefaultPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CLegacyInfoTextureHandler::drawPathTrav: {
			bool useCurrentBuildOrder = true;

			if (guihandler->inCommand <= 0)
				useCurrentBuildOrder = false;

			if (guihandler->inCommand >= guihandler->commands.size())
				useCurrentBuildOrder = false;

			if (useCurrentBuildOrder && guihandler->commands[guihandler->inCommand].type != CMDTYPE_ICON_BUILDING)
				useCurrentBuildOrder = false;

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

							CFeature* f = nullptr;

							if (CGameHelper::TestUnitBuildSquare(bi, f, gu->myAllyTeam, false)) {
								if (f != nullptr) {
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

				if (md != nullptr) {
					const bool los = (gs->cheatEnabled || gu->spectating);

					for (int ty = starty; ty < endy; ++ty) {
						for (int tx = 0; tx < mapDims.hmapx; ++tx) {
							const int sqx = (tx << 1);
							const int sqy = (ty << 1);
							const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
							const bool losSqr = losHandler->InLos(SquareToFloat3(sqx, sqy), gu->myAllyTeam);

							float scale = 1.0f;

							if (los || losSqr) {
								if (CMoveMath::IsBlocked(*md, sqx,     sqy    , nullptr) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx + 1, sqy    , nullptr) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx,     sqy + 1, nullptr) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
								if (CMoveMath::IsBlocked(*md, sqx + 1, sqy + 1, nullptr) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
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

					if (std::isinf(gCost[0])) { gCost[0] = gCostMax[0]; }
					if (std::isinf(gCost[1])) { gCost[1] = gCostMax[1]; }
					if (std::isinf(gCost[2])) { gCost[2] = gCostMax[2]; }

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
#else
void DefaultPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {}
#endif




void DefaultPathDrawer::Draw() const {
	glDisable(GL_TEXTURE_2D);
	glLineWidth(3);

	GL::TRenderDataBuffer<VA_TYPE_C>* vrdb = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = vrdb->GetShader();

	prog->Enable();
	prog->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
	prog->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

	// draw paths
	for (const auto& p: pm->pathMap) {
		const CPathManager::MultiPath& multiPath = p.second;

		// draw low-res segments of <path> (green)
		for (const float3& pos: multiPath.lowResPath.path) {
			vrdb->Append({pos + UpVector * 5.0f, SColor(0, 0, 255, 255)});
		}

		// draw med-res segments of <path> (blue)
		for (const float3& pos: multiPath.medResPath.path) {
			vrdb->Append({pos + UpVector * 5.0f, SColor(0, 255, 0, 255)});
		}

		// draw max-res segments of <path> (red)
		for (const float3& pos: multiPath.maxResPath.path) {
			vrdb->Append({pos + UpVector * 5.0f, SColor(255, 0, 0, 255)});
		}

		vrdb->Submit(GL_LINE_STRIP);
	}

	prog->Disable();

	// draw path definitions (goal, radius)
	for (const auto& p: pm->pathMap) {
		Draw(&p.second.peDef);
	}

	glLineWidth(1);
}



void DefaultPathDrawer::Draw(const CPathFinderDef* pfd) const {
	if (pfd->synced) {
		glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	} else {
		glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	}
	glSurfaceCircle(pfd->wsGoalPos, std::sqrt(pfd->sqGoalRadius), 20);
}

void DefaultPathDrawer::Draw(const CPathFinder* pf) const {
	glDisable(GL_TEXTURE_2D);

	GL::TRenderDataBuffer<VA_TYPE_C>* vrdb = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = vrdb->GetShader();

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

		vrdb->Append({p1, SColor(0.7f, 0.2f, 0.2f, 1.0f)});
		vrdb->Append({p2, SColor(0.7f, 0.2f, 0.2f, 1.0f)});
	}

	prog->Enable();
	vrdb->Submit(GL_LINES);
	prog->Disable();
}



void DefaultPathDrawer::Draw(const CPathEstimator* pe) const {
	const MoveDef* md = GetSelectedMoveDef();
	const PathNodeStateBuffer& blockStates = pe->blockStates;

	if (md == nullptr)
		return;

	GL::TRenderDataBuffer<VA_TYPE_C>* vrdb = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = vrdb->GetShader();

	glDisable(GL_TEXTURE_2D);

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	const int overlayPeriod = GAME_SPEED * 5;
	const int overlayNumber = (gs->frameNum % (overlayPeriod * 2)) / overlayPeriod;

	const bool drawLowResPE = (overlayNumber == 1 && pe == pm->lowResPE);
	const bool drawMedResPE = (overlayNumber == 0 && pe == pm->medResPE);

	// alternate between the extra debug-overlays
	// (normally TMI, but useful to keep the code
	// compiling)
	if (drawLowResPE || drawMedResPE) {
		const int2 peNumBlocks = pe->GetNumBlocks();
		const int vertexBaseNr = md->pathType * peNumBlocks.x * peNumBlocks.y * PATH_DIRECTION_VERTICES;

		for (int z = 0; z < peNumBlocks.y; z++) {
			for (int x = 0; x < peNumBlocks.x; x++) {
				const int blockNr = pe->BlockPosToIdx(int2(x, z));

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				vrdb->Append({p1                   , SColor(1.0f, 1.0f, 0.75f * drawLowResPE, 1.0f)});
				vrdb->Append({p1 - UpVector * 10.0f, SColor(1.0f, 1.0f, 0.75f * drawLowResPE, 1.0f)});

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + (CPathEstimator::GetDirectionVectorsTable())[dir].x;
					const int obz = z + (CPathEstimator::GetDirectionVectorsTable())[dir].y;

					if (obx <              0) continue;
					if (obz <              0) continue;
					if (obx >= peNumBlocks.x) continue;
					if (obz >= peNumBlocks.y) continue;

					const int obBlockNr = obz * peNumBlocks.x + obx;
					const int vertexNr = vertexBaseNr + blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, peNumBlocks.x);

					const float rawCost = pe->vertexCosts[vertexNr];
					const float nrmCost = (rawCost * PATH_NODE_SPACING) / pe->BLOCK_SIZE;

					if (rawCost >= PATHCOST_INFINITY)
						continue;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					vrdb->Append({p1, SColor(1.0f / std::sqrt(nrmCost), 1.0f / nrmCost, 0.75f * drawLowResPE, 1.0f)});
					vrdb->Append({p2, SColor(1.0f / std::sqrt(nrmCost), 1.0f / nrmCost, 0.75f * drawLowResPE, 1.0f)});
				}
			}
		}

		prog->Enable();
		vrdb->Submit(GL_LINES);
		prog->Disable();
		font->glWorldBegin();

		for (int z = 0; z < peNumBlocks.y; z++) {
			for (int x = 0; x < peNumBlocks.x; x++) {
				const int blockNr = pe->BlockPosToIdx(int2(x, z));

				float3 p1;
					p1.x = (blockStates.peNodeOffsets[md->pathType][blockNr].x) * SQUARE_SIZE;
					p1.z = (blockStates.peNodeOffsets[md->pathType][blockNr].y) * SQUARE_SIZE;
					p1.y = CGround::GetHeightAboveWater(p1.x, p1.z, false) + 10.0f;

				if (!camera->InView(p1))
					continue;

				for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
					const int obx = x + (CPathEstimator::GetDirectionVectorsTable())[dir].x;
					const int obz = z + (CPathEstimator::GetDirectionVectorsTable())[dir].y;

					if (obx <              0) continue;
					if (obz <              0) continue;
					if (obx >= peNumBlocks.x) continue;
					if (obz >= peNumBlocks.y) continue;

					const int obBlockNr = obz * peNumBlocks.x + obx;
					const int vertexNr = vertexBaseNr + blockNr * PATH_DIRECTION_VERTICES + GetBlockVertexOffset(dir, peNumBlocks.x);

					// rescale so numbers remain near 1.0 (more readable)
					const float rawCost = pe->vertexCosts[vertexNr];
					const float nrmCost = (rawCost * PATH_NODE_SPACING) / pe->BLOCK_SIZE;

					if (rawCost >= PATHCOST_INFINITY)
						continue;

					float3 p2;
						p2.x = (blockStates.peNodeOffsets[md->pathType][obBlockNr].x) * SQUARE_SIZE;
						p2.z = (blockStates.peNodeOffsets[md->pathType][obBlockNr].y) * SQUARE_SIZE;
						p2.y = CGround::GetHeightAboveWater(p2.x, p2.z, false) + 10.0f;

					// draw cost at middle of edge
					p2 = (p1 + p2) * 0.5f;

					if (!camera->InView(p2))
						continue;
					if (camera->GetPos().SqDistance(p2) >= (1000.0f * 1000.0f))
						continue;

					font->SetTextColor(1.0f, 1.0f / nrmCost, 0.75f * drawLowResPE, 1.0f);
					font->glWorldPrint(p2, 5.0f, FloatToString(nrmCost, "f(%.2f)"));
				}
			}
		}

		font->glWorldEnd();
	}
	#endif

	// [0] := low-res, [1] := med-res
	const SColor colors[2] = {SColor(0.2f, 0.7f, 0.7f, 1.0f), SColor(0.7f, 0.2f, 0.7f, 1.0f)};
	const SColor& color = colors[pe == pm->medResPE];

	{
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

			vrdb->Append({p1, color});
			vrdb->Append({p2, color});
		}

		prog->Enable();
		vrdb->Submit(GL_LINES);
		prog->Disable();
	}

	#if (PE_EXTRA_DEBUG_OVERLAYS == 1)
	if ((drawLowResPE || drawMedResPE) && false) {
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
			font->SetTextColor(1.0f, 0.7f, 0.75f * drawLowResPE, 1.0f);
			font->glWorldPrint(p1, 5.0f, blockCostsStr);
		}
	}
	#endif
}

