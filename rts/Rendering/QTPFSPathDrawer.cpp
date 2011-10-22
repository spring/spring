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

			DrawNodeTree(md);
			DrawPaths(md);
		}

		glPopAttrib();
	}
	#endif
}

void QTPFSPathDrawer::DrawNodeTree(const MoveData* md) const {
	glLineWidth(2);

	QTPFS::QTNode* nt = pm->nodeTrees[md->pathType];
	CVertexArray* va = GetVertexArray();

	DrawNodeTreeRec(nt, md, md->moveMath, va);
	glLineWidth(1);
}

void QTPFSPathDrawer::DrawNodeTreeRec(
	const QTPFS::QTNode* nt,
	const MoveData* md,
	const CMoveMath* mm,
	CVertexArray* va
) const {
	#define xminw (nt->xmin() * SQUARE_SIZE)
	#define xmaxw (nt->xmax() * SQUARE_SIZE)
	#define zminw (nt->zmin() * SQUARE_SIZE)
	#define zmaxw (nt->zmax() * SQUARE_SIZE)
	#define xmidw (nt->xmid() * SQUARE_SIZE)
	#define zmidw (nt->zmid() * SQUARE_SIZE)

	if (nt->IsLeaf()) {
		const float3 verts[5] = {
			float3(xminw, ground->GetHeightReal(xminw, zminw, false) + 4.0f, zminw),
			float3(xmaxw, ground->GetHeightReal(xmaxw, zminw, false) + 4.0f, zminw),
			float3(xmaxw, ground->GetHeightReal(xmaxw, zmaxw, false) + 4.0f, zmaxw),
			float3(xminw, ground->GetHeightReal(xminw, zmaxw, false) + 4.0f, zmaxw),
			float3(xmidw, ground->GetHeightReal(xmidw, zmidw, false) + 4.0f, zmidw),
		};
		static const unsigned char colors[2][4] = {
			{1 * 255, 0 * 255, 0 * 255, 1 * 255}, // blocked
			{0 * 255, 1 * 255, 0 * 255, 1 * 255}, // passable
		};
		const unsigned char* color = (mm->GetPosSpeedMod(*md, nt->xmid(), nt->zmid()) <= 0.001f)?
			&colors[0][0]:
			&colors[1][0];

		if (camera->InView(verts[4])) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			va->Initialize();
			va->EnlargeArrays(4, 0, VA_SIZE_C);
			va->AddVertexQC(verts[0], color);
			va->AddVertexQC(verts[1], color);
			va->AddVertexQC(verts[2], color);
			va->AddVertexQC(verts[3], color);
			va->DrawArrayC(GL_QUADS);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);
			font->glWorldPrint(verts[4], 4.0f, FloatToString(nt->GetMoveCost(), "%8.2f"));
		}
	} else {
		for (unsigned int i = 0; i < QTPFS::QTNode::CHILD_COUNT; i++) {
			DrawNodeTreeRec(nt->children[i], md, mm, va);
		}
	}

	#undef xminw
	#undef xmaxw
	#undef zminw
	#undef zmaxw
	#undef xmidw
	#undef zmidw
}

void QTPFSPathDrawer::DrawPaths(const MoveData* md) const {
	const QTPFS::PathCache& pathCache = pm->pathCaches[md->pathType];
	const QTPFS::PathCache::PathMap& paths = pathCache.GetLivePaths();

	QTPFS::PathCache::PathMap::const_iterator pathsIt;

	CVertexArray* va = GetVertexArray();

	for (pathsIt = paths.begin(); pathsIt != paths.end(); ++pathsIt) {
		DrawPath(pathsIt->second, va);
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

