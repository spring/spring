#ifndef TRANSPORTUNIT_H
#define TRANSPORTUNIT_H
/*pragma once removed*/
#include "Unit.h"
#include <list>

class CTransportUnit :
	public CUnit
{
public:
	CTransportUnit(const float3 &pos,int team,UnitDef* unitDef);
	~CTransportUnit(void);

	struct TransportedUnit{
		CUnit* unit;
		int piece;
	};

	std::list<TransportedUnit> transported;
	int transportCapacityUsed;

	void Update(void);
	void DependentDied(CObject* object);
	void KillUnit(bool selfDestruct,bool reclaimed);
	void AttachUnit(CUnit* unit, int piece);
	void DetachUnit(CUnit* unit);
};


#endif /* TRANSPORTUNIT_H */
