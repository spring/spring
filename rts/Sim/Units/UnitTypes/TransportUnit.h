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

	CTransportUnit();
	~CTransportUnit() {}
	void PostLoad() {}

	struct TransportedUnit {
		CR_DECLARE_STRUCT(TransportedUnit);
		CUnit* unit;
		int piece;
		int size;
		float mass;
	};

	void Update();
	void DependentDied(CObject* object);
	void KillUnit(bool selfDestruct, bool reclaimed, CUnit* attacker, bool showDeathSeq = false);
	void AttachUnit(CUnit* unit, int piece);
	bool CanTransport(const CUnit* unit) const;
	float GetLoadUnloadHeight(const float3& wantedPos, const CUnit* unit, bool* ok = NULL) const;
	bool CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit) const;
	float GetLoadUnloadHeading(const CUnit* unit) const;

	void DetachUnit(CUnit* unit);
	void DetachUnitFromAir(CUnit* unit, float3 pos); ///< moves to position after

	const std::list<TransportedUnit>& GetTransportedUnits() const { return transported; }

private:
	std::list<TransportedUnit> transported;
	int transportCapacityUsed;
	float transportMassUsed;

	bool DetachUnitCore(CUnit* unit);
};

#endif /* TRANSPORT_UNIT_H */
