#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

// FIXME
#define private public
#define protected public
#include "Sim/Path/QTPFS/Path.hpp"
#include "Sim/Path/QTPFS/Node.hpp"
#include "Sim/Path/QTPFS/NodeLayer.hpp"
#include "Sim/Path/QTPFS/PathManager.hpp"
#undef private

#include "Rendering/GlobalRendering.h"
#include "Rendering/QTPFSPathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"

static QTPFS::PathManager* pm = NULL;



QTPFSPathDrawer::QTPFSPathDrawer() {
	pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
}

void QTPFSPathDrawer::DrawAll() const {
	// QTPFS::PathManager is not thread-safe
	#if !defined(USE_GML) || !GML_ENABLE_SIM
	if (globalRendering->drawdebug && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
		Draw();
		glPopAttrib();
	}
	#endif
}

void QTPFSPathDrawer::Draw() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(3);

	MoveData* md = NULL;

	if (!moveinfo->moveData.empty()) {
		md = moveinfo->moveData[0];
	} else {
		return;
	}

	if (!selectedUnits.selectedUnits.empty()) {
		const CUnitSet& unitSet = selectedUnits.selectedUnits;
		const CUnit* unit = *(unitSet.begin());
		const UnitDef* unitDef = unit->unitDef;

		if (unitDef->movedata) {
			md = unitDef->movedata;
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_DEPTH_TEST);
	DrawNodeTree(pm->nodeTrees[md->pathType]);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glLineWidth(1);
}

void QTPFSPathDrawer::DrawNodeTree(const QTPFS::QTNode* node) const {
	if (node->IsLeaf()) {
		const float3 verts[4] = {
			float3(node->xmin() * SQUARE_SIZE, 0.0f, node->zmin() * SQUARE_SIZE),
			float3(node->xmax() * SQUARE_SIZE, 0.0f, node->zmin() * SQUARE_SIZE),
			float3(node->xmax() * SQUARE_SIZE, 0.0f, node->zmax() * SQUARE_SIZE),
			float3(node->xmin() * SQUARE_SIZE, 0.0f, node->zmax() * SQUARE_SIZE),
		};
		const unsigned char color[4] = {
			1 * 255, 0 * 255, 0 * 255, 1 * 255,
		};

		CVertexArray* va = GetVertexArray();

		va->Initialize();
		va->EnlargeArrays(4, 0, VA_SIZE_C);
		va->AddVertexQC(verts[0], color);
		va->AddVertexQC(verts[1], color);
		va->AddVertexQC(verts[2], color);
		va->AddVertexQC(verts[3], color);
		va->DrawArrayC(GL_QUADS);
	} else {
		for (unsigned int i = 0; i < QTPFS::QTNode::CHILD_COUNT; i++) {
			DrawNodeTree(node->children[i]);
		}
	}
}

