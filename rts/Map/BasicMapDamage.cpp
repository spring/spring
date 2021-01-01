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


void CBasicMapDamage::Init()
{
	mapHardness = mapInfo->map.hardness;

	for (int a = 0; a <= CRATER_TABLE_SIZE; ++a) {
		const float r  = a / float(CRATER_TABLE_SIZE);
		const float c1 = math::cos((r - 0.1f) * (math::PI + 0.3f));
		const float c2 = math::cos(std::max(0.0f, r * 3.0f - 2.0f) * math::PI);

		// faked Sombrero hat, cut in half
		craterTable[a] = c1 * (1.0f - r) * (0.5f + 0.5f * c2);
	}

	for (int a = 0; a < CMapInfo::NUM_TERRAIN_TYPES; ++a) {
		TerrainTypeHardnessChanged(a);
	}

	// 3x3 smoothing kernel
	weightTable[0] = 1.0f / 16.0f;
	weightTable[1] = 2.0f / 16.0f;
	weightTable[2] = 1.0f / 16.0f;
	weightTable[3] = 2.0f / 16.0f;
	weightTable[4] = 4.0f / 16.0f;
	weightTable[5] = 2.0f / 16.0f;
	weightTable[6] = 1.0f / 16.0f;
	weightTable[7] = 2.0f / 16.0f;
	weightTable[8] = 1.0f / 16.0f;


	explSquaresPoolIdx = 0;
	explUpdateQueueIdx = 0;

	explosionSquaresPool.clear();
	explosionSquaresPool.resize(4 * 1024 * 1024);
	explosionUpdateQueue.clear();
	explosionUpdateQueue.reserve(64);

	std::fill(explosionSquaresPool.begin(), explosionSquaresPool.end(), 0.0f);
}


void CBasicMapDamage::TerrainTypeHardnessChanged(int ttIndex)
{
	// table should contain only positive or only negative values, never both
	rawHardness[ttIndex] = mapHardness * std::max(0.001f, mapInfo->terrainTypes[ttIndex].hardness);
	invHardness[ttIndex] = 1.0f / rawHardness[ttIndex];
}

void CBasicMapDamage::TerrainTypeSpeedModChanged(int ttIndex)
{
	const unsigned char* typeMap = readMap->GetTypeMapSynced();

	// update all map-squares that reference this terrain-type (slow)
	for (int tz = 0; tz < mapDims.hmapy; tz++) {
		for (int tx = 0; tx < mapDims.hmapx; tx++) {
			if (typeMap[tz * mapDims.hmapx + tx] != ttIndex)
				continue;

			pathManager->TerrainChange((tx << 1), (tz << 1),  (tx << 1) + 1, (tz << 1) + 1,  TERRAINCHANGE_TYPEMAP_SPEED_VALUES);
		}
	}
}


void CBasicMapDamage::Explosion(const float3& pos, float strength, float radius)
{
	if (!pos.IsInMap())
		return;

	if (strength < 10.0f || radius < 8.0f)
		return;

	explosionUpdateQueue.emplace_back();

	Explo& e = explosionUpdateQueue.back();
	e.pos = pos;
	e.strength = strength;
	e.radius = radius;
	e.ttl = EXPLOSION_LIFETIME;
	e.x1 = Clamp<int>((pos.x - radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e.x2 = Clamp<int>((pos.x + radius) / SQUARE_SIZE, 1, mapDims.mapxm1);
	e.y1 = Clamp<int>((pos.z - radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e.y2 = Clamp<int>((pos.z + radius) / SQUARE_SIZE, 1, mapDims.mapym1);
	e.idx = explSquaresPoolIdx;

	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();
	const unsigned char* typeMap = readMap->GetTypeMapSynced();

	const float baseStrength = -math::pow(strength, 0.6f) * 3.0f;
	const float invRadius = 1.0f / radius;

	// figure out how much height to add to each square
	for (int y = e.y1; y <= e.y2; ++y) {
		for (int x = e.x1; x <= e.x2; ++x) {
			const CSolidObject* so = groundBlockingObjectMap.GroundBlockedUnsafe(y * mapDims.mapx + x);

			// do not change squares with buildings on them here
			if (so != nullptr && so->blockHeightChanges) {
				SetExplosionSquare(0.0f);
				continue;
			}

			// calculate the distance and normalize it
			const float expDist = pos.distance2D(float3(x * SQUARE_SIZE, 0.0f, y * SQUARE_SIZE));
			const float relDist = std::min(1.0f, expDist * invRadius);

			const unsigned int tableIdx = relDist * CRATER_TABLE_SIZE;
			// const unsigned int ttypeIdx = typeMap[(y >> 1) * mapDims.hmapx + (x >> 1)];


			// prevent formation of spikes from isolated "soft spots"
			// (one or two random squares with extremely low hardness
			// surrounded by high-strength terrain)
			float sumRawHardness = 0.0f;
			float avgInvHardness = 0.0f;

			for (int j = -1; j <= 1; j++) {
				for (int i = -1; i <= 1; i++) {
					const int tmz = Clamp((y >> 1) + j, 0, mapDims.hmapy - 1);
					const int tmx = Clamp((x >> 1) + i, 0, mapDims.hmapx - 1);
					const int tti = typeMap[tmz * mapDims.hmapx + tmx];

					sumRawHardness += (rawHardness[tti] * weightTable[(j + 1) * 3 + (i + 1)]);
				}
			}

			avgInvHardness = 1.0f / sumRawHardness;


			// FIXME: compensate for flattened ground under dead buildings
			const float prevDif = curHeightMap[y * mapDims.mapxp1 + x] - orgHeightMap[y * mapDims.mapxp1 + x];
			      float explDif = baseStrength;

			explDif *= craterTable[tableIdx];
			explDif *= avgInvHardness;
			// explDif *= invHardness[ttypeIdx];

			if ((prevDif * explDif) > 0.0f)
				explDif /= ((math::fabs(prevDif) / EXPLOSION_LIFETIME) + 1);

			if (explDif < -0.3f && strength > 200.0f)
				grassDrawer->RemoveGrass(float3(x * SQUARE_SIZE, 0.0f, y * SQUARE_SIZE));

			SetExplosionSquare(explDif);
		}
	}

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);

	// calculate how much to raise or lower buildings in the explosion radius
	// (while still keeping the ground below them flat)
	for (const CUnit* unit: *qfQuery.units) {
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
				const unsigned int ttypeIdx = typeMap[(z >> 1) * mapDims.hmapx + (x >> 1)];

				const float prevDif = curHeightMap[z * mapDims.mapxp1 + x] - orgHeightMap[z * mapDims.mapxp1 + x];
				      float explDif = baseStrength;

				explDif *= craterTable[tableIdx];
				explDif *= invHardness[ttypeIdx];

				if ((prevDif * explDif) > 0.0f)
					explDif /= ((math::fabs(prevDif) / EXPLOSION_LIFETIME) + 1);

				totalDif += explDif;
			}
		}

		if (totalDif != 0.0f) {
			e.buildings.emplace_back();

			ExploBuilding& eb = e.buildings.back();
			eb.id = unit->id;
			eb.dif = totalDif / (unit->xsize * unit->zsize);
			eb.tx1 = unit->mapPos.x;
			eb.tx2 = unit->mapPos.x + unit->xsize;
			eb.tz1 = unit->mapPos.y;
			eb.tz2 = unit->mapPos.y + unit->zsize;
		}
	}
}

void CBasicMapDamage::RecalcArea(int x1, int x2, int y1, int y2)
{
	readMap->UpdateHeightMapSynced(SRectangle(x1, y1, x2, y2));
	featureHandler.TerrainChanged(x1, y1, x2, y2);
	{
		SCOPED_TIMER("Sim::BasicMapDamage::Los");
		losHandler->UpdateHeightMapSynced(SRectangle(x1, y1, x2, y2));
	}
	{
		SCOPED_TIMER("Sim::BasicMapDamage::Path");
		pathManager->TerrainChange(x1, y1, x2, y2, TERRAINCHANGE_DAMAGE_RECALCULATION);
	}
}


void CBasicMapDamage::Update()
{
	SCOPED_TIMER("Sim::BasicMapDamage");

	for (unsigned int i = explUpdateQueueIdx, n = explosionUpdateQueue.size(); i < n; i++) {
		Explo& e = explosionUpdateQueue[i];

		if ((e.ttl--) <= 0)
			continue;


		unsigned int expSquarePoolIdx = e.idx;

		for (int y = e.y1; y <= e.y2; ++y) {
			for (int x = e.x1; x <= e.x2; ++x) {
				readMap->AddHeight(y * mapDims.mapxp1 + x, explosionSquaresPool[ (expSquarePoolIdx++) % explosionSquaresPool.size() ]);
			}
		}


		for (const ExploBuilding& b: e.buildings) {
			CUnit* unit = unitHandler.GetUnit(b.id);

			if (unit == nullptr)
				continue;

			// only change ground level if building is still here
			for (int z = b.tz1; z < b.tz2; z++) {
				for (int x = b.tx1; x < b.tx2; x++) {
					readMap->AddHeight(z * mapDims.mapxp1 + x, b.dif);
				}
			}

			unit->Move(UpVector * b.dif, true);
		}

		if (e.ttl != 0)
			continue;

		RecalcArea(e.x1 - 1, e.x2 + 1, e.y1 - 1, e.y2 + 1);
	}


	// pop explosions that are no longer being processed
	while (explUpdateQueueIdx < explosionUpdateQueue.size()) {
		if (explosionUpdateQueue[explUpdateQueueIdx].ttl > 0)
			return;

		explUpdateQueueIdx += 1;
	}

	// no more explosions left to process, clear queue for real
	explosionUpdateQueue.clear();
	explUpdateQueueIdx = 0;
}

