/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRANSPORT_UNIT_H
#define TRANSPORT_UNIT_H

#include "Sim/Units/Unit.h"
#include <list>

class CTransportUnit : public CUnit
{
public:
	CR_DECLARE(CTransportUnit);
	CR_DECLARE_SUB(TransportedUnit);

	struct TransportedUnit {
		CR_DECLARE_STRUCT(TransportedUnit);
		CUnit* unit;
		int piece;
		int size;
		float mass;
	};

	CTransportUnit(): transportCapacityUsed(0), transportMassUsed(0) {
	}

	void PostLoad() {}
	void Update();
	void DependentDied(CObject* object);
	void KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSeq = false);
	bool AttachUnit(CUnit* unit, int piece);
	bool CanTransport(const CUnit* unit) const;
	bool CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit, float* wantedHeightPtr = NULL) const;

	float GetTransporteeWantedHeight(const float3& wantedPos, const CUnit* unit, bool* ok = NULL) const;
	short GetTransporteeWantedHeading(const CUnit* unit) const;

	bool DetachUnit(CUnit* unit);
	bool DetachUnitFromAir(CUnit* unit, const float3& pos); ///< moves to position after

	const std::list<TransportedUnit>& GetTransportedUnits() const { return transportedUnits; }

private:
	std::list<TransportedUnit> transportedUnits;

	int transportCapacityUsed;
	float transportMassUsed;

	bool DetachUnitCore(CUnit* unit);
};

#endif /* TRANSPORT_UNIT_H */
