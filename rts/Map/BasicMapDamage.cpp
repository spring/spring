/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "BasicMapDamage.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "BaseGroundDrawer.h"
#include "HeightMapTexture.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitDef.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"


CBasicMapDamage::CBasicMapDamage()
{
	const int numQuads = qf->GetNumQuadsX() * qf->GetNumQuadsZ();
	inRelosQue = new bool[numQuads];
	for (int a = 0; a < numQuads; ++a) {
		inRelosQue[a] = false;
	}
	relosSize = 0;
	neededLosUpdate = 0;

	for (int a = 0; a <= CRATER_TABLE_SIZE; ++a) {
		const float r = a / float(CRATER_TABLE_SIZE);
		const float d = cos((r - 0.1f) * (PI + 0.3f)) * (1 - r) * (0.5f + 0.5f * cos(std::max(0.0f, r * 3 - 2) * PI));
		craterTable[a] = d;
	}

	for (int a = 0; a < CMapInfo::NUM_TERRAIN_TYPES; ++a) {
		invHardness[a] = 1.0f / mapInfo->terrainTypes[a].hardness;
	}

	mapHardness = mapInfo->map.hardness;

	disabled = false;
}

CBasicMapDamage::~CBasicMapDamage()
{
	while (!explosions.empty()) {
		delete explosions.front();
		explosions.pop_front();
	}
	delete[] inRelosQue;
}

void CBasicMapDamage::Explosion(const float3& pos, float strength, float radius)
{
	if ((pos.x < 0.0f) || (pos.x > gs->mapx * SQUARE_SIZE)) {
		return;
	}
	if ((pos.z < 0.0f) || (pos.z > gs->mapy * SQUARE_SIZE)) {
		return;
	}

	if (strength < 10.0f || radius < 8.0f) {
		return;
	}

	radius *= 1.5f;

	Explo* e = new Explo;
	e->pos = pos;
	e->strength = strength;
	e->ttl = 10;
	e->x1 = std::max((int) (pos.x - radius) / SQUARE_SIZE,            2);
	e->x2 = std::min((int) (pos.x + radius) / SQUARE_SIZE, gs->mapx - 3);
	e->y1 = std::max((int) (pos.z - radius) / SQUARE_SIZE,            2);
	e->y2 = std::min((int) (pos.z + radius) / SQUARE_SIZE, gs->mapy - 3);
	e->squares.reserve((e->y2 - e->y1 + 1) * (e->x2 - e->x1 + 1));

	const float* curHeightMap = readmap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readmap->GetOriginalHeightMapSynced();
	const unsigned char* typeMap = readmap->GetTypeMapSynced();
	const float baseStrength = -pow(strength, 0.6f) * 3 / mapHardness;
	const float invRadius = 1.0f / radius;

	for (int y = e->y1; y <= e->y2; ++y) {
		for (int x = e->x1; x <= e->x2; ++x) {
			const CSolidObject* so = groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x);

			// do not change squares with buildings on them here
			if (so && so->blockHeightChanges) {
				e->squares.push_back(0.0f);
				continue;
			}

			// calculate the distance and normalize it
			const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0, y * SQUARE_SIZE));
			const float relDist = std::min(1.0f, expDist * invRadius);
			const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;

			float dif =
					baseStrength * craterTable[tableIdx] *
					invHardness[typeMap[(y / 2) * gs->hmapx + x / 2]];

			// FIXME: compensate for flattened ground under dead buildings
			const float prevDif =
				curHeightMap[y * gs->mapxp1 + x] -
				orgHeightMap[y * gs->mapxp1 + x];

			if (prevDif * dif > 0.0f) {
				dif /= fabs(prevDif) * 0.1f + 1;
			}

			e->squares.push_back(dif);

			if (dif < -0.3f && strength > 200.0f) {
				treeDrawer->RemoveGrass(x, y);
			}
		}
	}

	// calculate how much to offset the buildings in the explosion radius with
	// (while still keeping the ground below them flat)
	const std::vector<CUnit*>& units = qf->GetUnitsExact(pos, radius);
	for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = *ui;

		if (!unit->blockHeightChanges) { continue; }
		if (!unit->isMarkedOnBlockingMap) { continue; }

		float totalDif = 0.0f;

		for (int z = unit->mapPos.y; z < unit->mapPos.y + unit->zsize; z++) {
			for (int x = unit->mapPos.x; x < unit->mapPos.x + unit->xsize; x++) {
				// calculate the distance and normalize it
				const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0, z * SQUARE_SIZE));
				const float relDist = std::min(1.0f, expDist * invRadius);
				const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;

				float dif =
						baseStrength * craterTable[tableIdx] *
						invHardness[typeMap[(z / 2) * gs->hmapx + x / 2]];
				const float prevDif =
						curHeightMap[z * gs->mapxp1 + x] -
						orgHeightMap[z * gs->mapxp1 + x];

				if (prevDif * dif > 0.0f) {
					dif /= fabs(prevDif) * 0.1f + 1;
				}

				totalDif += dif;
			}
		}

		totalDif /= (unit->xsize * unit->zsize);

		if (totalDif != 0.0f) {
			ExploBuilding eb;
			eb.id = (*ui)->id;
			eb.dif = totalDif;
			eb.tx1 = unit->mapPos.x;
			eb.tx2 = unit->mapPos.x + unit->xsize;
			eb.tz1 = unit->mapPos.y;
			eb.tz2 = unit->mapPos.y + unit->zsize;
			e->buildings.push_back(eb);
		}
	}

	explosions.push_back(e);
	readmap->Explosion(pos.x, pos.z, strength);
}

void CBasicMapDamage::RecalcArea(int x1, int x2, int y1, int y2)
{
	const int decy = std::max(                     0, (y1 * SQUARE_SIZE - CQuadField::QUAD_SIZE / 2) / CQuadField::QUAD_SIZE);
	const int incy = std::min(qf->GetNumQuadsZ() - 1, (y2 * SQUARE_SIZE + CQuadField::QUAD_SIZE / 2) / CQuadField::QUAD_SIZE);
	const int decx = std::max(                     0, (x1 * SQUARE_SIZE - CQuadField::QUAD_SIZE / 2) / CQuadField::QUAD_SIZE);
	const int incx = std::min(qf->GetNumQuadsX() - 1, (x2 * SQUARE_SIZE + CQuadField::QUAD_SIZE / 2) / CQuadField::QUAD_SIZE);

	const int numQuadsX = qf->GetNumQuadsX();
	const int frameNum  = gs->frameNum;

	for (int y = decy; y <= incy; y++) {
		for (int x = decx; x <= incx; x++) {
			if (inRelosQue[y * numQuadsX + x]) {
				continue;
			}

			RelosSquare rs;
			rs.x = x;
			rs.y = y;
			rs.neededUpdate = frameNum;
			rs.numUnits = qf->GetQuadAt(x, y).units.size();
			relosSize += rs.numUnits;
			inRelosQue[y * numQuadsX + x] = true;
			relosQue.push_back(rs);
		}
	}

	readmap->UpdateHeightMapSynced(x1, y1, x2, y2);
	pathManager->TerrainChange(x1, y1, x2, y2);
	featureHandler->TerrainChanged(x1, y1, x2, y2);
	CBaseWater::PushHeightmapChange(x1, y1, x2, y2);
	heightMapTexture.UpdateArea(x1, y1, x2, y2);
}


void CBasicMapDamage::Update()
{
	SCOPED_TIMER("BasicMapDamage::Update");

	std::deque<Explo*>::iterator ei;

	for (ei = explosions.begin(); ei != explosions.end(); ++ei) {
		Explo* e = *ei;
		if (e->ttl <= 0) {
			continue;
		}
		--e->ttl;

		const int x1 = e->x1;
		const int x2 = e->x2;
		const int y1 = e->y1;
		const int y2 = e->y2;
		std::vector<float>::const_iterator si = e->squares.begin();

		for (int y = y1; y <= y2; ++y) {
			for (int x = x1; x<= x2; ++x) {
				const float dif = *(si++);
				readmap->AddHeight(y * gs->mapxp1 + x, dif);
			}
		}
		std::vector<ExploBuilding>::const_iterator bi;
		for (bi = e->buildings.begin(); bi != e->buildings.end(); ++bi) {
			const float dif = bi->dif;
			const int tx1 = bi->tx1;
			const int tx2 = bi->tx2;
			const int tz1 = bi->tz1;
			const int tz2 = bi->tz2;

			for (int z = tz1; z < tz2; z++) {
				for (int x = tx1; x < tx2; x++) {
					readmap->AddHeight(z * gs->mapxp1 + x, dif);
				}
			}

			CUnit* unit = uh->units[bi->id];
			if (unit) {
				unit->pos.y += dif;
				unit->midPos.y += dif;
			}
		}
		if (e->ttl == 0) {
			RecalcArea(x1 - 2, x2 + 2, y1 - 2, y2 + 2);
		}
	}

	while (!explosions.empty() && explosions.front()->ttl == 0) {
		delete explosions.front();
		explosions.pop_front();
	}

	UpdateLos();
}

void CBasicMapDamage::UpdateLos()
{
	const int updateSpeed = (int) (relosSize * 0.01f) + 1;

	if (relosUnits.empty()) {
		if (relosQue.empty()) {
			return;
		}

		RelosSquare* rs = &relosQue.front();
		const std::list<CUnit*>& units = qf->GetQuadAt(rs->x, rs->y).units;

		std::list<CUnit*>::const_iterator ui;
		for (ui = units.begin(); ui != units.end(); ++ui) {
			relosUnits.push_back((*ui)->id);
		}
		relosSize -= rs->numUnits;
		neededLosUpdate = rs->neededUpdate;
		inRelosQue[rs->y * qf->GetNumQuadsX() + rs->x] = false;
		relosQue.pop_front();
	}

	for (int a = 0; a < updateSpeed; ++a) {
		if (relosUnits.empty()) {
			return;
		}

		CUnit* unit = uh->units[relosUnits.front()];
		relosUnits.pop_front();

		if (unit == NULL || unit->lastLosUpdate >= neededLosUpdate) {
			continue;
		}

		loshandler->MoveUnit(unit, true);
	}
}
