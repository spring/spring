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
	~CBasicMapDamage();

	void Explosion(const float3& pos, float strength, float radius);
	void RecalcArea(int x1, int x2, int y1, int y2);
	void Update();

private:
	void UpdateLos();

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
		std::vector<float> squares;
		std::vector<ExploBuilding> buildings;
		int ttl;
		int x1, x2, y1, y2;
	};

	std::deque<Explo*> explosions;

	struct RelosSquare {
		int x;
		int y;
		/// frame number
		int neededUpdate;
		int numUnits;
	};
	std::deque<RelosSquare> relosQue;

	bool* inRelosQue;
	int relosSize;
	/// frame number when LOS-update-required was triggered
	int neededLosUpdate;
	std::deque<int> relosUnits;

	static const unsigned int CRATER_TABLE_SIZE = 200;

	float craterTable[CRATER_TABLE_SIZE + 1];
	float invHardness[/*CMapInfo::NUM_TERRAIN_TYPES*/ 256];
};

#endif /* _BASIC_MAP_DAMAGE_H */
