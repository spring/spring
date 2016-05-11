/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BasicMapDamage.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Features/FeatureHandler.h"
#include "System/TimeProfiler.h"


CBasicMapDamage::CBasicMapDamage()
{
	for (int a = 0; a <= CRATER_TABLE_SIZE; ++a) {
		const float r  = a / float(CRATER_TABLE_SIZE);
		const float c1 = math::cos((r - 0.1f) * (PI + 0.3f));
		const float c2 = math::cos(std::max(0.0f, r * 3.0f - 2.0f) * PI);

		// faked Sombrero hat, cut in half
		craterTable[a] = c1 * (1.0f - r) * (0.5f + 0.5f * c2);
	}

	for (int a = 0; a < CMapInfo::NUM_TERRAIN_TYPES; ++a) {
		invHardness[a] = 1.0f / mapInfo->terrainTypes[a].hardness;
	}

	mapHardness = mapInfo->map.hardness;
	disabled = false;
}

void CBasicMapDamage::Explosion(const float3& pos, float strength, float radius)
{
	if (!pos.IsInMap())
		return;

	if (strength < 10.0f || radius < 8.0f)
		return;

	explosions.emplace_back();

	Explo& e = explosions.back();
	e.pos = pos;
	e.strength = strength;
	e.radius = radius;
	e.ttl = EXPLOSION_LIFETIME;
	e.x1 = Clamp<int>((pos.x - radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e.x2 = Clamp<int>((pos.x + radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e.y1 = Clamp<int>((pos.z - radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e.y2 = Clamp<int>((pos.z + radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e.squares.reserve((e.y2 - e.y1 + 1) * (e.x2 - e.x1 + 1));

	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();
	const unsigned char* typeMap = readMap->GetTypeMapSynced();

	const float baseStrength = -math::pow(strength, 0.6f) * 3 / mapHardness;
	const float invRadius = 1.0f / radius;

	// figure out how much height to add to each square
	for (int y = e.y1; y <= e.y2; ++y) {
		for (int x = e.x1; x <= e.x2; ++x) {
			const CSolidObject* so = groundBlockingObjectMap->GroundBlockedUnsafe(y * mapDims.mapx + x);

			// do not change squares with buildings on them here
			if (so != nullptr && so->blockHeightChanges) {
				e.squares.push_back(0.0f);
				continue;
			}

			// calculate the distance and normalize it
			const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0, y * SQUARE_SIZE));
			const float relDist = std::min(1.0f, expDist * invRadius);

			const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;
			const unsigned int ttypeIdx = typeMap[(y / 2) * mapDims.hmapx + x / 2];

			float dif = baseStrength;
			dif *= craterTable[tableIdx];
			dif *= invHardness[ttypeIdx];

			// FIXME: compensate for flattened ground under dead buildings
			const float prevDif =
				curHeightMap[y * mapDims.mapxp1 + x] -
				orgHeightMap[y * mapDims.mapxp1 + x];

			if ((prevDif * dif) > 0.0f)
				dif /= ((math::fabs(prevDif) / EXPLOSION_LIFETIME) + 1);

			if (dif < -0.3f && strength > 200.0f)
				grassDrawer->RemoveGrass(float3(x * SQUARE_SIZE, 0.0f, y * SQUARE_SIZE));

			e.squares.push_back(dif);
		}
	}

	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);

	// calculate how much to raise or lower buildings in the explosion radius
	// (while still keeping the ground below them flat)
	for (const CUnit* unit: units) {
		if (!unit->blockHeightChanges)
			continue;
		if (!unit->IsBlocking())
			continue;

		float totalDif = 0.0f;

		for (int z = unit->mapPos.y; z < unit->mapPos.y + unit->zsize; z++) {
			for (int x = unit->mapPos.x; x < unit->mapPos.x + unit->xsize; x++) {
				// calculate the distance and normalize it
				const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0.0f, z * SQUARE_SIZE));
				const float relDist = std::min(1.0f, expDist * invRadius);

				const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;
				const unsigned int ttypeIdx = typeMap[(z / 2) * mapDims.hmapx + x / 2];

				float dif = baseStrength;
				dif *= craterTable[tableIdx];
				dif *= invHardness[ttypeIdx];

				const float prevDif =
					curHeightMap[z * mapDims.mapxp1 + x] -
					orgHeightMap[z * mapDims.mapxp1 + x];

				if ((prevDif * dif) > 0.0f)
					dif /= ((math::fabs(prevDif) / EXPLOSION_LIFETIME) + 1);

				totalDif += dif;
			}
		}

		totalDif /= (unit->xsize * unit->zsize);

		if (totalDif != 0.0f) {
			e.buildings.emplace_back();

			ExploBuilding& eb = e.buildings.back();
			eb.id = unit->id;
			eb.dif = totalDif;
			eb.tx1 = unit->mapPos.x;
			eb.tx2 = unit->mapPos.x + unit->xsize;
			eb.tz1 = unit->mapPos.y;
			eb.tz2 = unit->mapPos.y + unit->zsize;
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

	for (Explo& e: explosions) {
		if (e.ttl <= 0)
			continue;

		--e.ttl;

		auto si = e.squares.cbegin();

		for (int y = e.y1; y <= e.y2; ++y) {
			for (int x = e.x1; x <= e.x2; ++x) {
				readMap->AddHeight(y * mapDims.mapxp1 + x, *(si++));
			}
		}

		for (const ExploBuilding& b: e.buildings) {
			CUnit* unit = unitHandler->GetUnit(b.id);

			if (unit != nullptr) {
				// only change ground level if building is still here
				for (int z = b.tz1; z < b.tz2; z++) {
					for (int x = b.tx1; x < b.tx2; x++) {
						readMap->AddHeight(z * mapDims.mapxp1 + x, b.dif);
					}
				}

				unit->Move(UpVector * b.dif, true);
			}
		}

		if (e.ttl == 0) {
			RecalcArea(e.x1 - 1, e.x2 + 1, e.y1 - 1, e.y2 + 1);
		}
	}

	while (!explosions.empty()) {
		const Explo& explosion = explosions.front();

		if (explosion.ttl > 0)
			break;

		explosions.pop_front();
	}
}

