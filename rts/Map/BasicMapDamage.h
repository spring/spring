/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_MAP_DAMAGE_H
#define _BASIC_MAP_DAMAGE_H

#include "MapDamage.h"

#include <deque>
#include <vector>

class CUnit;

class CBasicMapDamage : public IMapDamage
{
public:
	CBasicMapDamage();

	void Explosion(const float3& pos, float strength, float radius);
	void RecalcArea(int x1, int x2, int y1, int y2);
	void Update();

private:
	struct ExploBuilding {
		/**
		 * Searching for building pointers inside these on DependentDied
		 * could be messy, so we use the id.
		 */
		int id;
		/// How much to move the building and the ground below it.
		float dif;
		int tx1, tx2, tz1, tz2;
	};

	struct Explo {
		float3 pos;

		float strength;
		float radius;

		int ttl;
		int x1, x2, y1, y2;

		std::vector<float> squares;
		std::vector<ExploBuilding> buildings;
	};

	std::deque<Explo> explosions;

	static const unsigned int CRATER_TABLE_SIZE = 200;
	static const unsigned int EXPLOSION_LIFETIME = 10;

	float craterTable[CRATER_TABLE_SIZE + 1];
	float invHardness[/*CMapInfo::NUM_TERRAIN_TYPES*/ 256];
};

#endif /* _BASIC_MAP_DAMAGE_H */
