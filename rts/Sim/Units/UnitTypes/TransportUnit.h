#ifndef TRANSPORTUNIT_H
#define TRANSPORTUNIT_H

#include "Sim/Units/Unit.h"
#include <list>

class CTransportUnit :
	public CUnit
{
public:
	CR_DECLARE(CTransportUnit);

	CTransportUnit();
	~CTransportUnit(void);

	struct TransportedUnit{
		CUnit* unit;
		int piece;
		int size;
		float mass;
	};

	std::list<TransportedUnit> transported;
	int transportCapacityUsed;
	float transportMassUsed;

	void Update(void);
	void DependentDied(CObject* object);
	void KillUnit(bool selfDestruct,bool reclaimed, CUnit *attacker);
	void AttachUnit(CUnit* unit, int piece);
	void DetachUnit(CUnit* unit);
	bool CanTransport(CUnit* unit);
};


#endif /* TRANSPORTUNIT_H */
