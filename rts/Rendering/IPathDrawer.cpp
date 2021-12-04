/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "IPathDrawer.h"
#include "DefaultPathDrawer.h"
#include "QTPFSPathDrawer.h"
#include "Game/SelectedUnitsHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/Default/PathManager.h"
#include "Sim/Path/QTPFS/PathManager.hpp"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "System/EventHandler.h"

IPathDrawer* pathDrawer = nullptr;

IPathDrawer* IPathDrawer::GetInstance() {
	if (pathDrawer == nullptr) {
		if (dynamic_cast<QTPFS::PathManager*>(pathManager) != nullptr)
			return (pathDrawer = new QTPFSPathDrawer());

		if (dynamic_cast<CPathManager*>(pathManager) != nullptr)
			return (pathDrawer = new DefaultPathDrawer());

		pathDrawer = new IPathDrawer();
	}

	return pathDrawer;
}

void IPathDrawer::FreeInstance(IPathDrawer* pd) {
	assert(pd == pathDrawer);
	delete pd;
	pathDrawer = nullptr;
}



IPathDrawer::IPathDrawer(): CEventClient("[IPathDrawer]", 271991, false), enabled(false) {
	eventHandler.AddClient(this);
}
IPathDrawer::~IPathDrawer() {
	eventHandler.RemoveClient(this);
}

const MoveDef* IPathDrawer::GetSelectedMoveDef() {
	const MoveDef* md = nullptr;
	const auto& unitSet = selectedUnitsHandler.selectedUnits;

	if (!unitSet.empty()) {
		const CUnit* unit = unitHandler.GetUnit(*unitSet.begin());
		md = unit->moveDef;
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

	const int hmIdx = sqz * mapDims.mapxp1 + sqx;
	const int cnIdx = sqz * mapDims.mapx   + sqx;

	const float height = hm[hmIdx];
	const float slope = 1.0f - cn[cnIdx].y;

	if (md->speedModClass == MoveDef::Ship) {
		// only check water depth
		m = (height >= (-md->depth))? 0.0f: m;
	} else {
		// check depth and slope (if hover, only over land)
		m = std::max(0.0f, 1.0f - (slope / (md->maxSlope + 0.1f)));
		m = (height < (-md->depth))? 0.0f: m;
		m = (height <= 0.0f && md->speedModClass == MoveDef::Hover)? 1.0f: m;
	}

	return m;
}
#endif

