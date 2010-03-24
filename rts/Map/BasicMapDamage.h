/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASICMAPDAMAGE_H
#define BASICMAPDAMAGE_H

#include "MapDamage.h"

#include <deque>
#include <vector>

class CUnit;

class CBasicMapDamage : public IMapDamage
{
public:
	CBasicMapDamage(void);
	~CBasicMapDamage(void);

	struct ExploBuilding {
		int id;			//searching for building pointers inside these on dependentdied could be messy so we use the id
		float dif;		//how much to move the building and the ground under it
		int tx1,tx2,tz1,tz2;
	};

	struct Explo {
		float3 pos;
		float strength;
		float radius;
		std::vector<float> squares;
		std::vector<ExploBuilding> buildings;
		int ttl;
		int x1,x2,y1,y2;
	};

	std::deque<Explo*> explosions;

	struct RelosSquare{
		int x;
		int y;
		int neededUpdate;
		int numUnits;
	};
	std::deque<RelosSquare> relosQue;

	bool* inRelosQue;
	int relosSize;
	int neededLosUpdate;
	std::deque<int> relosUnits;

	float craterTable[10000];
	float invHardness[/*CMapInfo::NUM_TERRAIN_TYPES*/ 256];

	void Explosion(const float3& pos, float strength,float radius);
	void RecalcArea(int x1, int x2, int y1, int y2);
	void Update(void);

	void UpdateLos(void);
};

#endif /* BASICMAPDAMAGE_H */
