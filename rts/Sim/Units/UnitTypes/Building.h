/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BUILDING_H__
#define __BUILDING_H__

#include "Sim/Units/Unit.h"
#include "System/float3.h"

struct BuildingGroundDecal;

class CBuilding : public CUnit
{
public:
	CR_DECLARE(CBuilding);

	CBuilding();
	virtual ~CBuilding() {}
	void PostLoad() {}

	void PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build);
	void PostInit(const CUnit* builder);
	void ForcedMove(const float3& newPos, int facing);

	BuildingGroundDecal* buildingDecal;
};

#endif // __BUILDING_H__
