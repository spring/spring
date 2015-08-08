/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BasicMapDamage.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "BaseGroundDrawer.h"
#include "HeightMapTexture.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitDef.h"
#include "System/TimeProfiler.h"


CBasicMapDamage::CBasicMapDamage()
{
	for (int a = 0; a <= CRATER_TABLE_SIZE; ++a) {
		const float r = a / float(CRATER_TABLE_SIZE);
		const float d = math::cos((r - 0.1f) * (PI + 0.3f)) * (1 - r) * (0.5f + 0.5f * math::cos(std::max(0.0f, r * 3 - 2) * PI));
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
}

void CBasicMapDamage::Explosion(const float3& pos, float strength, float radius)
{
	if (!pos.IsInMap()) {
		return;
	}
	if (strength < 10.0f || radius < 8.0f) {
		return;
	}

	Explo* e = new Explo;
	e->pos = pos;
	e->strength = strength;
	e->ttl = 10;
	e->x1 = Clamp<int>((pos.x - radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e->x2 = Clamp<int>((pos.x + radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e->y1 = Clamp<int>((pos.z - radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e->y2 = Clamp<int>((pos.z + radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e->squares.reserve((e->y2 - e->y1 + 1) * (e->x2 - e->x1 + 1));

	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();
	const unsigned char* typeMap = readMap->GetTypeMapSynced();
	const float baseStrength = -math::pow(strength, 0.6f) * 3 / mapHardness;
	const float invRadius = 1.0f / radius;

	for (int y = e->y1; y <= e->y2; ++y) {
		for (int x = e->x1; x <= e->x2; ++x) {
			const CSolidObject* so = groundBlockingObjectMap->GroundBlockedUnsafe(y * mapDims.mapx + x);

			// do not change squares with buildings on them here
			if (so && so->blockHeightChanges) {
				e->squares.push_back(0.0f);
				continue;
			}

			// calculate the distance and normalize it
			const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0, y * SQUARE_SIZE));
			const float relDist = std::min(1.0f, expDist * invRadius);
			const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;

			float dif = baseStrength;
			dif *= craterTable[tableIdx];
			dif *= invHardness[typeMap[(y / 2) * mapDims.hmapx + x / 2]];

			// FIXME: compensate for flattened ground under dead buildings
			const float prevDif =
				curHeightMap[y * mapDims.mapxp1 + x] -
				orgHeightMap[y * mapDims.mapxp1 + x];

			if (prevDif * dif > 0.0f) {
				dif /= math::fabs(prevDif) * 0.1f + 1;
			}

			e->squares.push_back(dif);

			if (dif < -0.3f && strength > 200.0f) {
				grassDrawer->RemoveGrass(float3(x * SQUARE_SIZE, 0.0f, y * SQUARE_SIZE));
			}
		}
	}

	// calculate how much to offset the buildings in the explosion radius with
	// (while still keeping the ground below them flat)
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
	for (const CUnit* unit: units) {
		if (!unit->blockHeightChanges) { continue; }
		if (!unit->IsBlocking()) { continue; }

		float totalDif = 0.0f;

		for (int z = unit->mapPos.y; z < unit->mapPos.y + unit->zsize; z++) {
			for (int x = unit->mapPos.x; x < unit->mapPos.x + unit->xsize; x++) {
				// calculate the distance and normalize it
				const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0, z * SQUARE_SIZE));
				const float relDist = std::min(1.0f, expDist * invRadius);
				const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;

				float dif =
						baseStrength * craterTable[tableIdx] *
						invHardness[typeMap[(z / 2) * mapDims.hmapx + x / 2]];
				const float prevDif =
						curHeightMap[z * mapDims.mapxp1 + x] -
						orgHeightMap[z * mapDims.mapxp1 + x];

				if (prevDif * dif > 0.0f) {
					dif /= math::fabs(prevDif) * 0.1f + 1;
				}

				totalDif += dif;
			}
		}

		totalDif /= (unit->xsize * unit->zsize);

		if (totalDif != 0.0f) {
			ExploBuilding eb;
			eb.id = unit->id;
			eb.dif = totalDif;
			eb.tx1 = unit->mapPos.x;
			eb.tx2 = unit->mapPos.x + unit->xsize;
			eb.tz1 = unit->mapPos.y;
			eb.tz2 = unit->mapPos.y + unit->zsize;
			e->buildings.push_back(eb);
		}
	}

	explosions.push_back(e);
}

void CBasicMapDamage::RecalcArea(int x1, int x2, int y1, int y2)
{
	readMap->UpdateHeightMapSynced(SRectangle(x1, y1, x2, y2));
	featureHandler->TerrainChanged(x1, y1, x2, y2);
	losHandler->UpdateHeightMapSynced(SRectangle(x1, y1, x2, y2));
	pathManager->TerrainChange(x1, y1, x2, y2, TERRAINCHANGE_DAMAGE_RECALCULATION);
}


void CBasicMapDamage::Update()
{
	SCOPED_TIMER("BasicMapDamage::Update");

	for (Explo* e: explosions) {
		if (e->ttl <= 0) {
			continue;
		}
		--e->ttl;

		std::vector<float>::const_iterator si = e->squares.begin();
		for (int y = e->y1; y <= e->y2; ++y) {
			for (int x = e->x1; x <= e->x2; ++x) {
				const float dif = *(si++);
				readMap->AddHeight(y * mapDims.mapxp1 + x, dif);
			}
		}

		for (ExploBuilding& b: e->buildings) {
			for (int z = b.tz1; z < b.tz2; z++) {
				for (int x = b.tx1; x < b.tx2; x++) {
					readMap->AddHeight(z * mapDims.mapxp1 + x, b.dif);
				}
			}

			CUnit* unit = unitHandler->GetUnit(b.id);
			if (unit != NULL) {
				unit->Move(UpVector * b.dif, true);
			}
		}

		if (e->ttl == 0) {
			RecalcArea(e->x1 - 1, e->x2 + 1, e->y1 - 1, e->y2 + 1);
		}
	}

	while (!explosions.empty() && explosions.front()->ttl == 0) {
		delete explosions.front();
		explosions.pop_front();
	}
}

