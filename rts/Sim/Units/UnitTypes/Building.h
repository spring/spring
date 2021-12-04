/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUILDING_H
#define _BUILDING_H

#include "Sim/Units/Unit.h"
#include "System/float3.h"

struct SolidObjectGroundDecal;

class CBuilding : public CUnit
{
public:
	CR_DECLARE(CBuilding)

	CBuilding(): CUnit() { immobile = true; }
	virtual ~CBuilding() {}

	void PreInit(const UnitLoadParams& params);
	void PostInit(const CUnit* builder);
	void ForcedMove(const float3& newPos);
};

#endif // _BUILDING_H
