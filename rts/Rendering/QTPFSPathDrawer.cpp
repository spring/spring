#include <limits>

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

#include "Sim/Path/QTPFS/Path.hpp"
#include "Sim/Path/QTPFS/Node.hpp"
#include "Sim/Path/QTPFS/NodeLayer.hpp"
#include "Sim/Path/QTPFS/PathCache.hpp"
#include "Sim/Path/QTPFS/PathManager.hpp"

#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/QTPFSPathDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "System/StringUtil.h"

static std::vector<const QTPFS::QTNode*> visibleNodes;

static constexpr unsigned char LINK_COLOR[4] = {1 * 255, 0 * 255, 1 * 255, 1 * 128};
static constexpr unsigned char PATH_COLOR[4] = {0 * 255, 0 * 255, 1 * 255, 1 * 255};
static constexpr unsigned char NODE_COLORS[3][4] = {
	{1 * 255, 0 * 255, 0 * 255, 1 * 255}, // red --> blocked
	{0 * 255, 1 * 255, 0 * 255, 1 * 255}, // green --> passable
	{0 * 255, 0 * 255, 1 *  64, 1 *  64}, // light blue --> pushed
};


QTPFSPathDrawer::QTPFSPathDrawer() {
	pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
}

void QTPFSPathDrawer::DrawAll() const {
	const MoveDef* md = GetSelectedMoveDef();

	if (md == nullptr)
		return;

	if (!enabled)
		return;

	if (!gs->cheatEnabled && !gu->spectating)
		return;

	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask();

	visibleNodes.clear();
	visibleNodes.reserve(256);

	GetVisibleNodes(pm->GetNodeTree(md->pathType), pm->GetNodeLayer(md->pathType), visibleNodes);

	if (!visibleNodes.empty()) {
		GL::RenderDataBufferC* rdb = GL::GetRenderBufferC();
		Shader::IProgramObject* ipo = rdb->GetShader();
		GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();

		ipo->Enable();
		ipo->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
		ipo->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
		wla->Setup(rdb, globalRendering->viewSizeX, globalRendering->viewSizeY, 1.0f, camera->GetViewProjectionMatrix());

		DrawNodes(wla, visibleNodes);
		DrawPaths(md, rdb, wla);

		ipo->Disable();

		// text has its own shader, draw it last
		DrawCosts(visibleNodes);
	}

	glAttribStatePtr->PopBits();
}

void QTPFSPathDrawer::DrawNodes(GL::WideLineAdapterC* wla, const std::vector<const QTPFS::QTNode*>& nodes) const {
	for (const QTPFS::QTNode* node: nodes) {
		DrawNodeW(node, wla, &NODE_COLORS[!node->AllSquaresImpassable()][0]);
	}

	wla->SetWidth(2.0f);
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	wla->Submit(GL_QUADS);

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	wla->SetWidth(1.0f);
}

void QTPFSPathDrawer::DrawCosts(const std::vector<const QTPFS::QTNode*>& nodes) const {
	#define xmidw (node->xmid() * SQUARE_SIZE)
	#define zmidw (node->zmid() * SQUARE_SIZE)
	font->glWorldBegin();

	for (const QTPFS::QTNode* node: nodes) {
		const float3 pos = {xmidw * 1.0f, CGround::GetHeightReal(xmidw, zmidw, false) + 4.0f, zmidw * 1.0f};

		if (pos.SqDistance(camera->GetPos()) >= Square(1000.0f))
			continue;

		font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);
		font->glWorldPrint(pos, 5.0f, FloatToString(node->GetMoveCost(), "%8.2f"));
	}

	font->glWorldEnd();
	#undef zmidw
	#undef xmidw
}



void QTPFSPathDrawer::GetVisibleNodes(const QTPFS::QTNode* nt, const QTPFS::NodeLayer& nl, std::vector<const QTPFS::QTNode*>& nodes) const {
	if (nt->IsLeaf()) {
		nodes.push_back(nt);
		return;
	}

	for (unsigned int i = 0; i < QTNODE_CHILD_COUNT; i++) {
		const QTPFS::QTNode* cn = nl.GetPoolNode(nt->GetChildBaseIndex() + i);
		const float3 mins = float3(cn->xmin() * SQUARE_SIZE, 0.0f, cn->zmin() * SQUARE_SIZE);
		const float3 maxs = float3(cn->xmax() * SQUARE_SIZE, 0.0f, cn->zmax() * SQUARE_SIZE);

		if (!camera->InView(mins, maxs))
			continue;

		GetVisibleNodes(cn, nl, nodes);
	}
}



void QTPFSPathDrawer::DrawPaths(const MoveDef* md, GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla) const {
	const QTPFS::PathCache& pathCache = pm->GetPathCache(md->pathType);
	const QTPFS::PathCache::PathMap& paths = pathCache.GetLivePaths();

	wla->SetWidth(4.0f);

	{
		for (const auto& pair: paths) {
			DrawPath(pair.second, wla);
		}
	}

	{
		#ifdef QTPFS_DRAW_WAYPOINT_GROUND_CIRCLES
		constexpr float4 color = {0.0f, 0.0f, 1.0f, 1.0f};

		for (const auto& pair: paths) {
			const QTPFS::IPath* path = pair.second;

			for (unsigned int n = 0; n < path->NumPoints(); n++) {
				glSurfaceCircleW(wla, {path->GetPoint(n), path->GetRadius()}, color, 16);
			}
		}

		wla->Submit(GL_LINES);
		#endif
	}

	wla->SetWidth(1.0f);

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	const auto& pathTypes = pm->GetPathTypes();
	const auto& pathTraces = pm->GetPathTraces();

	for (const auto& pair: paths) {
		const auto typeIt = pathTypes.find(pair.first);
		const auto traceIt = pathTraces.find(pair.first);

		if (typeIt == pathTypes.end() || traceIt == pathTraces.end())
			continue;
		// this only happens if source-node was equal to target-node
		if (traceIt->second == nullptr)
			continue;

		DrawSearchExecution(typeIt->second, traceIt->second, rdb, wla);
	}
	#endif
}

void QTPFSPathDrawer::DrawPath(const QTPFS::IPath* path, GL::WideLineAdapterC* wla) const {
	for (unsigned int n = 0; n < path->NumPoints() - 1; n++) {
		float3 p0 = path->GetPoint(n + 0);
		float3 p1 = path->GetPoint(n + 1);

		if (!camera->InView(p0) && !camera->InView(p1))
			continue;

		p0.y = CGround::GetHeightReal(p0.x, p0.z, false);
		p1.y = CGround::GetHeightReal(p1.x, p1.z, false);

		wla->SafeAppend({p0, PATH_COLOR});
		wla->SafeAppend({p1, PATH_COLOR});
	}

	wla->Submit(GL_LINES);
}

void QTPFSPathDrawer::DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* se, GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla) const {
	// TODO: prevent overdraw so oft-visited nodes do not become darker
	const std::vector<QTPFS::PathSearchTrace::Iteration>& searchIters = se->GetIterations();
	      std::vector<QTPFS::PathSearchTrace::Iteration>::const_iterator it;
	const unsigned searchFrame = se->GetFrame();

	// number of frames reserved to draw the entire trace
	constexpr unsigned int MAX_DRAW_TIME = GAME_SPEED * 5;

	unsigned int itersPerFrame = (searchIters.size() / MAX_DRAW_TIME) + 1;
	unsigned int curSearchIter = 0;
	unsigned int maxSearchIter = ((gs->frameNum - searchFrame) + 1) * itersPerFrame;

	for (it = searchIters.begin(); it != searchIters.end(); ++it) {
		if (curSearchIter++ >= maxSearchIter)
			break;

		const QTPFS::PathSearchTrace::Iteration& searchIter = *it;
		const std::vector<unsigned int>& nodeIndices = searchIter.GetNodeIndices();

		DrawSearchIteration(pathType, nodeIndices, rdb, wla);
	}
}

void QTPFSPathDrawer::DrawSearchIteration(unsigned int pathType, const std::vector<unsigned int>& nodeIndices, GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla) const {
	unsigned int hmx = nodeIndices[0] % mapDims.mapx;
	unsigned int hmz = nodeIndices[0] / mapDims.mapx;

	const QTPFS::NodeLayer& nodeLayer = pm->GetNodeLayer(pathType);

	const QTPFS::QTNode* pushedNode = nullptr;
	const QTPFS::QTNode* poppedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz));

	{
		// popped node
		DrawNode(poppedNode, rdb, &NODE_COLORS[2][0]);

		// pushed nodes
		for (size_t i = 1, n = nodeIndices.size(); i < n; i++) {
			hmx = nodeIndices[i] % mapDims.mapx;
			hmz = nodeIndices[i] / mapDims.mapx;

			DrawNode(pushedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz)), rdb, &NODE_COLORS[2][0]);
		}

		rdb->Submit(GL_TRIANGLES);
	}
	{
		wla->SetWidth(2.0f);

		for (size_t i = 1, n = nodeIndices.size(); i < n; i++) {
			hmx = nodeIndices[i] % mapDims.mapx;
			hmz = nodeIndices[i] / mapDims.mapx;

			DrawNodeLink(pushedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz)), poppedNode, wla);
		}

		wla->Submit(GL_LINES);
		wla->SetWidth(1.0f);
	}
}


#define xminw (node->xmin() * SQUARE_SIZE)
#define xmaxw (node->xmax() * SQUARE_SIZE)
#define zminw (node->zmin() * SQUARE_SIZE)
#define zmaxw (node->zmax() * SQUARE_SIZE)

void QTPFSPathDrawer::DrawNode(const QTPFS::QTNode* node, GL::RenderDataBufferC* rdb, const unsigned char* color) const {
	const float3 v0 = float3(xminw, CGround::GetHeightReal(xminw, zminw, false) + 4.0f, zminw);
	const float3 v1 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw);
	const float3 v2 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw);
	const float3 v3 = float3(xminw, CGround::GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw);

	rdb->SafeAppend({v0, color}); // tl
	rdb->SafeAppend({v1, color}); // tr
	rdb->SafeAppend({v2, color}); // br

	rdb->SafeAppend({v2, color}); // br
	rdb->SafeAppend({v3, color}); // bl
	rdb->SafeAppend({v0, color}); // tl
}

void QTPFSPathDrawer::DrawNodeW(const QTPFS::QTNode* node, GL::WideLineAdapterC* wla, const unsigned char* color) const {
	const float3 v0 = float3(xminw, CGround::GetHeightReal(xminw, zminw, false) + 4.0f, zminw);
	const float3 v1 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw);
	const float3 v2 = float3(xmaxw, CGround::GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw);
	const float3 v3 = float3(xminw, CGround::GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw);

	wla->SafeAppend({v0, color});
	wla->SafeAppend({v1, color});
	wla->SafeAppend({v2, color});
	wla->SafeAppend({v3, color});
}

#undef xminw
#undef xmaxw
#undef zminw
#undef zmaxw


void QTPFSPathDrawer::DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, GL::WideLineAdapterC* wla) const {
	#define xmidw(n) (n->xmid() * SQUARE_SIZE)
	#define zmidw(n) (n->zmid() * SQUARE_SIZE)

	const float3 v0 = {xmidw(pushedNode) * 1.0f, CGround::GetHeightReal(xmidw(pushedNode), zmidw(pushedNode), false) + 4.0f, zmidw(pushedNode) * 1.0f};
	const float3 v1 = {xmidw(poppedNode) * 1.0f, CGround::GetHeightReal(xmidw(poppedNode), zmidw(poppedNode), false) + 4.0f, zmidw(poppedNode) * 1.0f};


	if (!camera->InView(v0) && !camera->InView(v1))
		return;

	wla->SafeAppend({v0, LINK_COLOR});
	wla->SafeAppend({v1, LINK_COLOR});

	#undef xmidw
	#undef zmidw
}



#if 0
// part of LegacyInfoTexHandler, no longer called
void QTPFSPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CLegacyInfoTextureHandler::drawPathTrav: {
			const MoveDef* md = GetSelectedMoveDef();

			if (md != nullptr) {
				const QTPFS::NodeLayer& nl = pm->GetNodeLayer(md->pathType);

				const float smr = 1.0f / nl.GetMaxRelSpeedMod();
				const bool los = (gs->cheatEnabled || gu->spectating);

				for (int ty = starty; ty < endy; ++ty) {
					for (int tx = 0; tx < mapDims.hmapx; ++tx) {
						const int sqx = (tx << 1);
						const int sqz = (ty << 1);
						const int texIdx = ((ty * (mapDims.pwr2mapx >> 1)) + tx) * 4 - offset;
						const bool losSqr = losHandler->InLos(SquareToFloat3(sqx, sqz), gu->myAllyTeam);

						#if 1
						// use node-modifiers as baseline so visualisation is in sync with alt+B
						const QTPFS::QTNode* node = static_cast<const QTPFS::QTNode*>(nl.GetNode(sqx, sqz));

						const float sm = CMoveMath::GetPosSpeedMod(*md, sqx, sqz);
						const SColor& smc = GetSpeedModColor((los || losSqr)? node->speedModAvg * smr: sm);
						#else
						float scale = 1.0f;

						if (los || losSqr) {
							if (CMoveMath::IsBlocked(*md, sqx,     sqz    ) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx + 1, sqz    ) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx,     sqz + 1) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
							if (CMoveMath::IsBlocked(*md, sqx + 1, sqz + 1) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
						}

						const float sm = CMoveMath::GetPosSpeedMod(md, sqx, sqz);
						const SColor& smc = GetSpeedModColor(sm * scale);
						#endif

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
		} break;

		case CLegacyInfoTextureHandler::drawPathCost: {
		} break;
	}
}
#else
void QTPFSPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {}
#endif

