#include <limits>

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"

// FIXME
#define private public
#define protected public
#include "Sim/Path/QTPFS/Path.hpp"
#include "Sim/Path/QTPFS/Node.hpp"
#include "Sim/Path/QTPFS/NodeLayer.hpp"
#include "Sim/Path/QTPFS/PathCache.hpp"
#include "Sim/Path/QTPFS/PathManager.hpp"
#undef protected
#undef private

#include "Rendering/Fonts/glFont.h"
#include "Rendering/QTPFSPathDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Map/InfoTexture/Legacy/LegacyInfoTextureHandler.h"
#include "System/Util.h"

QTPFSPathDrawer::QTPFSPathDrawer(): IPathDrawer() {
	pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
}

void QTPFSPathDrawer::DrawAll() const {
	const MoveDef* md = GetSelectedMoveDef();

	if (md == NULL)
		return;

	if (enabled && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		DrawNodeTree(md);
		DrawPaths(md);

		glPopAttrib();
	}
}

void QTPFSPathDrawer::DrawNodeTree(const MoveDef* md) const {
	QTPFS::QTNode* nt = pm->nodeTrees[md->pathType];
	CVertexArray* va = GetVertexArray();

	std::list<const QTPFS::QTNode*> nodes;
	std::list<const QTPFS::QTNode*>::const_iterator nodesIt;

	GetVisibleNodes(nt, nodes);

	va->Initialize();
	va->EnlargeArrays(nodes.size() * 4, 0, VA_SIZE_C);

	for (nodesIt = nodes.begin(); nodesIt != nodes.end(); ++nodesIt) {
		DrawNode(*nodesIt, md, va, false, true, true);
	}

	glLineWidth(2);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	va->DrawArrayC(GL_QUADS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1);
}

void QTPFSPathDrawer::DrawNodeTreeRec(
	const QTPFS::QTNode* nt,
	const MoveDef* md,
	CVertexArray* va
) const {
	if (nt->IsLeaf()) {
		DrawNode(nt, md, va, false, true, false);
	} else {
		for (unsigned int i = 0; i < nt->children.size(); i++) {
			const QTPFS::QTNode* n = nt->children[i];
			const float3 mins = float3(n->xmin() * SQUARE_SIZE, 0.0f, n->zmin() * SQUARE_SIZE);
			const float3 maxs = float3(n->xmax() * SQUARE_SIZE, 0.0f, n->zmax() * SQUARE_SIZE);

			if (!camera->InView(mins, maxs))
				continue;

			DrawNodeTreeRec(nt->children[i], md, va);
		}
	}
}

void QTPFSPathDrawer::GetVisibleNodes(const QTPFS::QTNode* nt, std::list<const QTPFS::QTNode*>& nodes) const {
	if (nt->IsLeaf()) {
		nodes.push_back(nt);
	} else {
		for (unsigned int i = 0; i < nt->children.size(); i++) {
			const QTPFS::QTNode* n = nt->children[i];
			const float3 mins = float3(n->xmin() * SQUARE_SIZE, 0.0f, n->zmin() * SQUARE_SIZE);
			const float3 maxs = float3(n->xmax() * SQUARE_SIZE, 0.0f, n->zmax() * SQUARE_SIZE);

			if (!camera->InView(mins, maxs))
				continue;

			GetVisibleNodes(nt->children[i], nodes);
		}
	}
}



void QTPFSPathDrawer::DrawPaths(const MoveDef* md) const {
	const QTPFS::PathCache& pathCache = pm->pathCaches[md->pathType];
	const QTPFS::PathCache::PathMap& paths = pathCache.GetLivePaths();

	QTPFS::PathCache::PathMap::const_iterator pathsIt;

	CVertexArray* va = GetVertexArray();

	for (pathsIt = paths.begin(); pathsIt != paths.end(); ++pathsIt) {
		DrawPath(pathsIt->second, va);

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		#define PM QTPFS::PathManager
		const PM::PathTypeMap::const_iterator typeIt = pm->pathTypes.find(pathsIt->first);
		const PM::PathTraceMap::const_iterator traceIt = pm->pathTraces.find(pathsIt->first);
		#undef PM

		if (typeIt == pm->pathTypes.end() || traceIt == pm->pathTraces.end())
			continue;
		// this only happens if source-node was equal to target-node
		if (traceIt->second == NULL)
			continue;

		DrawSearchExecution(typeIt->second, traceIt->second);
		#endif
	}
}

void QTPFSPathDrawer::DrawPath(const QTPFS::IPath* path, CVertexArray* va) const {
	glLineWidth(4);

	{
		va->Initialize();
		va->EnlargeArrays(path->NumPoints() * 2, 0, VA_SIZE_C);

		static const unsigned char color[4] = {
			0 * 255, 0 * 255, 1 * 255, 1 * 255,
		};

		for (unsigned int n = 0; n < path->NumPoints() - 1; n++) {
			float3 p0 = path->GetPoint(n + 0);
			float3 p1 = path->GetPoint(n + 1);

			if (!camera->InView(p0) && !camera->InView(p1))
				continue;

			p0.y = CGround::GetHeightReal(p0.x, p0.z, false);
			p1.y = CGround::GetHeightReal(p1.x, p1.z, false);

			va->AddVertexQC(p0, color);
			va->AddVertexQC(p1, color);
		}

		va->DrawArrayC(GL_LINES);
	}

	#ifdef QTPFS_DRAW_WAYPOINT_GROUND_CIRCLES
	{
		glColor4ub(color[0], color[1], color[2], color[3]);

		for (unsigned int n = 0; n < path->NumPoints(); n++) {
			glSurfaceCircle(path->GetPoint(n), path->GetRadius(), 16);
		}

		glColor4ub(255, 255, 255, 255);
	}
	#endif

	glLineWidth(1);
}

void QTPFSPathDrawer::DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* searchExec) const {
	// TODO: prevent overdraw so oft-visited nodes do not become darker
	const std::list<QTPFS::PathSearchTrace::Iteration>& searchIters = searchExec->GetIterations();
	      std::list<QTPFS::PathSearchTrace::Iteration>::const_iterator it;
	const unsigned searchFrame = searchExec->GetFrame();

	// number of frames reserved to draw the entire trace
	static const unsigned int MAX_DRAW_TIME = GAME_SPEED * 5;

	unsigned int itersPerFrame = (searchIters.size() / MAX_DRAW_TIME) + 1;
	unsigned int curSearchIter = 0;
	unsigned int maxSearchIter = ((gs->frameNum - searchFrame) + 1) * itersPerFrame;

	CVertexArray* va = GetVertexArray();

	for (it = searchIters.begin(); it != searchIters.end(); ++it) {
		if (curSearchIter++ >= maxSearchIter)
			break;

		const QTPFS::PathSearchTrace::Iteration& searchIter = *it;
		const std::list<unsigned int>& nodeIndices = searchIter.GetNodeIndices();

		DrawSearchIteration(pathType, nodeIndices, va);
	}
}

void QTPFSPathDrawer::DrawSearchIteration(unsigned int pathType, const std::list<unsigned int>& nodeIndices, CVertexArray* va) const {
	std::list<unsigned int>::const_iterator it = nodeIndices.begin();

	unsigned int hmx = (*it) % mapDims.mapx;
	unsigned int hmz = (*it) / mapDims.mapx;

	const QTPFS::NodeLayer& nodeLayer = pm->nodeLayers[pathType];
	const QTPFS::QTNode* poppedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz));
	const QTPFS::QTNode* pushedNode = NULL;

	DrawNode(poppedNode, NULL, va, true, false, false);

	for (++it; it != nodeIndices.end(); ++it) {
		hmx = (*it) % mapDims.mapx;
		hmz = (*it) / mapDims.mapx;
		pushedNode = static_cast<const QTPFS::QTNode*>(nodeLayer.GetNode(hmx, hmz));

		DrawNode(pushedNode, NULL, va, true, false, false);
		DrawNodeLink(pushedNode, poppedNode, va);
	}
}

void QTPFSPathDrawer::DrawNode(
	const QTPFS::QTNode* node,
	const MoveDef* md,
	CVertexArray* va,
	bool fillQuad,
	bool showCost,
	bool batchDraw
) const {
	#define xminw (node->xmin() * SQUARE_SIZE)
	#define xmaxw (node->xmax() * SQUARE_SIZE)
	#define zminw (node->zmin() * SQUARE_SIZE)
	#define zmaxw (node->zmax() * SQUARE_SIZE)
	#define xmidw (node->xmid() * SQUARE_SIZE)
	#define zmidw (node->zmid() * SQUARE_SIZE)

	const float3 verts[5] = {
		float3(xminw, CGround::GetHeightReal(xminw, zminw, false) + 4.0f, zminw),
		float3(xmaxw, CGround::GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw),
		float3(xmaxw, CGround::GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw),
		float3(xminw, CGround::GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw),
		float3(xmidw, CGround::GetHeightReal(xmidw, zmidw, false) + 4.0f, zmidw),
	};
	static const unsigned char colors[3][4] = {
		{1 * 255, 0 * 255, 0 * 255, 1 * 255}, // red --> blocked
		{0 * 255, 1 * 255, 0 * 255, 1 * 255}, // green --> passable
		{0 * 255, 0 * 255, 1 *  64, 1 *  64}, // light blue --> pushed
	};

	const unsigned char* color =
		(!showCost)?
			&colors[2][0]:
		(node->moveCostAvg == QTPFS_POSITIVE_INFINITY)?
			&colors[0][0]:
			&colors[1][0];

	if (!batchDraw) {
		if (!camera->InView(verts[4])) {
			return;
		}

		va->Initialize();
		va->EnlargeArrays(4, 0, VA_SIZE_C);

		if (!fillQuad) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	}

	va->AddVertexQC(verts[0], color);
	va->AddVertexQC(verts[1], color);
	va->AddVertexQC(verts[2], color);
	va->AddVertexQC(verts[3], color);

	if (!batchDraw) {
		va->DrawArrayC(GL_QUADS);

		if (!fillQuad) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	if (showCost && camera->GetPos().SqDistance(verts[4]) < (1000.0f * 1000.0f)) {
		font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);
		font->glWorldPrint(verts[4], 5.0f, FloatToString(node->GetMoveCost(), "%8.2f"));
	}

	#undef xminw
	#undef xmaxw
	#undef zminw
	#undef zmaxw
	#undef xmidw
	#undef zmidw
}

void QTPFSPathDrawer::DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, CVertexArray* va) const {
	#define xmidw(n) (n->xmid() * SQUARE_SIZE)
	#define zmidw(n) (n->zmid() * SQUARE_SIZE)

	const float3 verts[2] = {
		float3(xmidw(pushedNode), CGround::GetHeightReal(xmidw(pushedNode), zmidw(pushedNode), false) + 4.0f, zmidw(pushedNode)),
		float3(xmidw(poppedNode), CGround::GetHeightReal(xmidw(poppedNode), zmidw(poppedNode), false) + 4.0f, zmidw(poppedNode)),
	};
	static const unsigned char color[4] = {
		1 * 255, 0 * 255, 1 * 255, 1 * 128,
	};

	if (!camera->InView(verts[0]) && !camera->InView(verts[1]))
		return;

	va->Initialize();
	va->EnlargeArrays(2, 0, VA_SIZE_C);
	va->AddVertexQC(verts[0], color);
	va->AddVertexQC(verts[1], color);
	glLineWidth(2);
	va->DrawArrayC(GL_LINES);
	glLineWidth(1);

	#undef xmidw
	#undef zmidw
}



void QTPFSPathDrawer::UpdateExtraTexture(int extraTex, int starty, int endy, int offset, unsigned char* texMem) const {
	switch (extraTex) {
		case CLegacyInfoTextureHandler::drawPathTrav: {
			const MoveDef* md = GetSelectedMoveDef();

			if (md != NULL) {
				const QTPFS::NodeLayer& nl = pm->nodeLayers[md->pathType];

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

