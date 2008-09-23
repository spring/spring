#ifndef TRANSPORTUNIT_H
#define TRANSPORTUNIT_H

#include "Sim/Units/Unit.h"
#include <list>

class CTransportUnit :
	public CUnit
{
public:
	CR_DECLARE(CTransportUnit);
	CR_DECLARE_SUB(TransportedUnit);

	CTransportUnit();
	~CTransportUnit();
	void PostLoad();

	struct TransportedUnit{
		CR_DECLARE_STRUCT(TransportedUnit);
		CUnit* unit;
		int piece;
		int size;
		float mass;
	};

	std::list<TransportedUnit> transported;
	int transportCapacityUsed;
	float transportMassUsed;

	void Update();
	void DependentDied(CObject* object);
	void KillUnit(bool selfDestruct,bool reclaimed, CUnit *attacker, bool showDeathSeq = false);
	void AttachUnit(CUnit* unit, int piece);
	void DetachUnit(CUnit* unit);
	bool CanTransport(CUnit* unit);
	void DetachUnitFromAir(CUnit* unit,float3 pos); //moves to position after
	void DetachUnitFromAir(CUnit* unit);
};


#endif /* TRANSPORTUNIT_H */
