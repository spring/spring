/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "IPathDrawer.h"
#include "DefaultPathDrawer.h"
#include "QTPFSPathDrawer.h"
#include "Game/SelectedUnitsHandler.h"
#include "lib/gml/gmlmut.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/Default/PathManager.h"
#include "Sim/Path/QTPFS/PathManager.hpp"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

IPathDrawer* pathDrawer = NULL;

IPathDrawer* IPathDrawer::GetInstance() {
	static IPathDrawer* pd = NULL;

	if (pd == NULL) {
		if (dynamic_cast<QTPFS::PathManager*>(pathManager) != NULL) {
			return (pd = new QTPFSPathDrawer());
		}
		if (dynamic_cast<CPathManager*>(pathManager) != NULL) {
			return (pd = new DefaultPathDrawer());
		}

		pd = new IPathDrawer();
	}

	return pd;
}

void IPathDrawer::FreeInstance(IPathDrawer* pd) {
	delete pd;
}



const MoveDef* IPathDrawer::GetSelectedMoveDef() {
	GML_RECMUTEX_LOCK(sel); // UpdateExtraTexture

	const MoveDef* md = NULL;
	const CUnitSet& unitSet = selectedUnitsHandler.selectedUnits;

	if (!unitSet.empty()) {
		const CUnit* unit = *(unitSet.begin());
		const MoveDef* moveDef = unit->moveDef;

		md = moveDef;
	}

	return md;
}

SColor IPathDrawer::GetSpeedModColor(const float sm) {
	SColor col(120, 0, 80);

	if (sm > 0.0f) {
		col.r = 255 - std::min(sm * 255.0f, 255.0f);
		col.g = 255 - col.r;
		col.b =   0;
	}

	return col;
}

#if 0
float IPathDrawer::GetSpeedModNoObstacles(const MoveDef* md, int sqx, int sqz) {
	float m = 0.0f;

	const int hmIdx = sqz * gs->mapxp1 + sqx;
	const int cnIdx = sqz * gs->mapx   + sqx;

	const float height = hm[hmIdx];
	const float slope = 1.0f - cn[cnIdx].y;

	if (md->moveFamily == MoveDef::Ship) {
		// only check water depth
		m = (height >= (-md->depth))? 0.0f: m;
	} else {
		// check depth and slope (if hover, only over land)
		m = std::max(0.0f, 1.0f - (slope / (md->maxSlope + 0.1f)));
		m = (height < (-md->depth))? 0.0f: m;
		m = (height <= 0.0f && md->moveFamily == MoveDef::Hover)? 1.0f: m;
	}

	return m;
}
#endif

