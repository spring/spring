// Building.h: interface for the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDING_H__
#define __BUILDING_H__

#include "archdef.h"

#include "Unit.h"

class CBuilding : public CUnit  
{
public:
	CBuilding(const float3 &pos,int team,UnitDef* unitDef);
	virtual ~CBuilding();

	void Init(void);
};

#endif // __BUILDING_H__
