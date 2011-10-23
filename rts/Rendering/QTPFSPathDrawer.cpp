#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

// FIXME
#define private public
#define protected public
#include "Sim/Path/QTPFS/Path.hpp"
#include "Sim/Path/QTPFS/Node.hpp"
#include "Sim/Path/QTPFS/NodeLayer.hpp"
#include "Sim/Path/QTPFS/PathCache.hpp"
#include "Sim/Path/QTPFS/PathManager.hpp"
#undef private

#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/QTPFSPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Util.h"

static QTPFS::PathManager* pm = NULL;

static const MoveData* GetMoveData() {
	const MoveData* md = NULL;
	const CUnitSet& unitSet = selectedUnits.selectedUnits;

	if (moveinfo->moveData.empty()) {
		return md;
	}

	if (!unitSet.empty()) {
		const CUnit* unit = *(unitSet.begin());
		const UnitDef* unitDef = unit->unitDef;

		if (unitDef->movedata != NULL) {
			md = unitDef->movedata;
		}
	}

	return md;
}



QTPFSPathDrawer::QTPFSPathDrawer() {
	pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
}

void QTPFSPathDrawer::DrawAll() const {
	// QTPFS::PathManager is not thread-safe
	#if !defined(USE_GML) || !GML_ENABLE_SIM
	if (globalRendering->drawdebug && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);

		const MoveData* md = GetMoveData();

		if (md != NULL) {
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			DrawNodeTree(md);
			DrawPaths(md);
		}

		glPopAttrib();
	}
	#endif
}

void QTPFSPathDrawer::DrawNodeTree(const MoveData* md) const {
	QTPFS::QTNode* nt = pm->nodeTrees[md->pathType];
	CVertexArray* va = GetVertexArray();

	glLineWidth(2);
	DrawNodeTreeRec(nt, md, md->moveMath, va);
	glLineWidth(1);
}

void QTPFSPathDrawer::DrawNodeTreeRec(
	const QTPFS::QTNode* nt,
	const MoveData* md,
	const CMoveMath* mm,
	CVertexArray* va
) const {
	if (nt->IsLeaf()) {
		DrawNode(nt, md, mm, va, false, true);
	} else {
		for (unsigned int i = 0; i < QTPFS::QTNode::CHILD_COUNT; i++) {
			DrawNodeTreeRec(nt->children[i], md, mm, va);
		}
	}
}

void QTPFSPathDrawer::DrawPaths(const MoveData* md) const {
	const QTPFS::PathCache& pathCache = pm->pathCaches[md->pathType];
	const QTPFS::PathCache::PathMap& paths = pathCache.GetLivePaths();

	QTPFS::PathCache::PathMap::const_iterator pathsIt;

	CVertexArray* va = GetVertexArray();

	for (pathsIt = paths.begin(); pathsIt != paths.end(); ++pathsIt) {
		DrawPath(pathsIt->second, va);

		#ifdef TRACE_PATH_SEARCHES
		#define PM QTPFS::PathManager
		const PM::PathTypeMap::const_iterator typeIt = pm->pathTypes.find(pathsIt->first);
		const PM::PathTraceMap::const_iterator traceIt = pm->pathTraces.find(pathsIt->first);
		#undef PM

		if (typeIt == pm->pathTypes.end() || traceIt == pm->pathTraces.end())
			continue;

		DrawSearchExecution(typeIt->second, traceIt->second);
		#endif
	}
}

void QTPFSPathDrawer::DrawPath(const QTPFS::IPath* path, CVertexArray* va) const {
	va->Initialize();
	va->EnlargeArrays(path->NumPoints() * 2, 0, VA_SIZE_C);

	static const unsigned char color[4] = {
		0 * 255, 0 * 255, 1 * 255, 1 * 255,
	};

	for (unsigned int n = 0; n < path->NumPoints() - 1; n++) {
		float3 p0 = path->GetPoint(n + 0); p0.y = ground->GetHeightReal(p0.x, p0.z, false);
		float3 p1 = path->GetPoint(n + 1); p1.y = ground->GetHeightReal(p1.x, p1.z, false);

		va->AddVertexQC(p0, color);
		va->AddVertexQC(p1, color);
	}

	glLineWidth(4);
	va->DrawArrayC(GL_LINES);
	glLineWidth(1);
}

void QTPFSPathDrawer::DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* searchExec) const {
	// TODO: prevent overdraw so oft-visited nodes do not become darker
	const std::list<QTPFS::PathSearchTrace::Iteration>& searchIters = searchExec->GetIterations();
	      std::list<QTPFS::PathSearchTrace::Iteration>::const_iterator it;
	const unsigned searchFrame = searchExec->GetFrame();

	// advance one iteration every N=1 frames
	unsigned int curSearchIter = 0;
	unsigned int maxSearchIter = 1 + ((gs->frameNum - searchFrame) / 1);

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

	unsigned int hmx = (*it) % gs->mapx;
	unsigned int hmz = (*it) / gs->mapx;

	const QTPFS::NodeLayer& nodeLayer = pm->nodeLayers[pathType];
	const QTPFS::QTNode* poppedNode = (const QTPFS::QTNode*) nodeLayer.GetNode(hmx, hmz);
	const QTPFS::QTNode* pushedNode = NULL;

	DrawNode(poppedNode, NULL, NULL, va, true, false);

	for (++it; it != nodeIndices.end(); ++it) {
		hmx = (*it) % gs->mapx;
		hmz = (*it) / gs->mapx;
		pushedNode = (const QTPFS::QTNode*) nodeLayer.GetNode(hmx, hmz);

		DrawNode(pushedNode, NULL, NULL, va, true, false);
		DrawNodeLink(pushedNode, poppedNode, va);
	}
}

void QTPFSPathDrawer::DrawNode(
	const QTPFS::QTNode* node,
	const MoveData* md,
	const CMoveMath* mm,
	CVertexArray* va,
	bool fillQuad,
	bool showCost
) const {
	#define xminw (node->xmin() * SQUARE_SIZE)
	#define xmaxw (node->xmax() * SQUARE_SIZE)
	#define zminw (node->zmin() * SQUARE_SIZE)
	#define zmaxw (node->zmax() * SQUARE_SIZE)
	#define xmidw (node->xmid() * SQUARE_SIZE)
	#define zmidw (node->zmid() * SQUARE_SIZE)

	const float3 verts[5] = {
		float3(xminw, ground->GetHeightReal(xminw, zminw, false) + 4.0f, zminw),
		float3(xmaxw, ground->GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw),
		float3(xmaxw, ground->GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw),
		float3(xminw, ground->GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw),
		float3(xmidw, ground->GetHeightReal(xmidw, zmidw, false) + 4.0f, zmidw),
	};
	static const unsigned char colors[3][4] = {
		{1 * 255, 0 * 255, 0 * 255, 1 * 255}, // red --> blocked
		{0 * 255, 1 * 255, 0 * 255, 1 * 255}, // green --> passable
		{0 * 255, 0 * 255, 1 *  64, 1 *  64}, // light blue --> pushed
	};

	const unsigned char* color = NULL;

	if (!camera->InView(verts[4])) {
		return;
	}

	if (!showCost) {
		color = &colors[2][0];
	} else {
		color = (mm->GetPosSpeedMod(*md, node->xmid(), node->zmid()) <= 0.001f)?
			&colors[0][0]:
			&colors[1][0];
	}

	if (!fillQuad)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	va->Initialize();
	va->EnlargeArrays(4, 0, VA_SIZE_C);
	va->AddVertexQC(verts[0], color);
	va->AddVertexQC(verts[1], color);
	va->AddVertexQC(verts[2], color);
	va->AddVertexQC(verts[3], color);
	va->DrawArrayC(GL_QUADS);

	if (!fillQuad)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (showCost) {
		font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);
		font->glWorldPrint(verts[4], 4.0f, FloatToString(node->GetMoveCost(), "%8.2f"));
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
		float3(xmidw(pushedNode), ground->GetHeightReal(xmidw(pushedNode), zmidw(pushedNode), false) + 4.0f, zmidw(pushedNode)),
		float3(xmidw(poppedNode), ground->GetHeightReal(xmidw(poppedNode), zmidw(poppedNode), false) + 4.0f, zmidw(poppedNode)),
	};
	static const unsigned char color[4] = {
		1 * 255, 0 * 255, 1 * 255, 1 * 128,
	};

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

