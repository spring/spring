// Building.h: interface for the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDING_H__
#define __BUILDING_H__

#include "Sim/Units/Unit.h"

struct BuildingGroundDecal;

class CBuilding : public CUnit
{
public:
	CR_DECLARE(CBuilding);

	CBuilding();
	virtual ~CBuilding();
	void PostLoad();

	void Init(const CUnit* builder);
	void UnitInit(const UnitDef* def, int team, const float3& position);

	BuildingGroundDecal* buildingDecal;
};

#endif // __BUILDING_H__
