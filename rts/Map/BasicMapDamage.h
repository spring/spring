/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_MAP_DAMAGE_H
#define _BASIC_MAP_DAMAGE_H

#include "MapDamage.h"

#include <vector>

class CBasicMapDamage : public IMapDamage
{
public:
	void Explosion(const float3& pos, float strength, float radius) override;
	void RecalcArea(int x1, int x2, int y1, int y2) override;
	void TerrainTypeHardnessChanged(int ttIndex) override;
	void TerrainTypeSpeedModChanged(int ttIndex) override;

	void Init() override;
	void Update() override;

	bool Disabled() const override { return false; }

private:
	void SetExplosionSquare(float v) {
		explosionSquaresPool[explSquaresPoolIdx] = v;

		explSquaresPoolIdx += 1;
		explSquaresPoolIdx %= explosionSquaresPool.size();
	}

	struct ExploBuilding {
		/**
		 * Searching for building pointers inside these on DependentDied
		 * could be messy, so we use the id.
		 */
		int id;

		// how much to move the building and the ground below it
		float dif;

		int tx1, tx2;
		int tz1, tz2;
	};

	struct Explo {
		float3 pos;

		float strength = 0.0f;
		float radius = 0.0f;

		int ttl = 0;
		int x1 = 0, x2 = 0;
		int y1 = 0, y2 = 0;

		// index (into explosionSquaresPool) of first square touched by us
		unsigned int idx = 0;

		std::vector<ExploBuilding> buildings;
	};

	std::vector<float> explosionSquaresPool;
	std::vector<Explo> explosionUpdateQueue;

	static constexpr unsigned int CRATER_TABLE_SIZE = 200;
	static constexpr unsigned int EXPLOSION_LIFETIME = 10;

	unsigned int explSquaresPoolIdx = 0;
	unsigned int explUpdateQueueIdx = 0;

	float craterTable[CRATER_TABLE_SIZE + 1];
	float rawHardness[/*CMapInfo::NUM_TERRAIN_TYPES*/ 256];
	float invHardness[/*CMapInfo::NUM_TERRAIN_TYPES*/ 256];
	float weightTable[9];
};

#endif /* _BASIC_MAP_DAMAGE_H */
