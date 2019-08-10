/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUP_H
#define GROUP_H

#include <vector>

#include "Sim/Units/CommandAI/Command.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/UnorderedSet.hpp"

class CUnit;
class CFeature;

/**
 * Logic group of units denoted by a number.
 * A group-ID/-number is unique per team (-> per groupHandler).
 */
class CGroup
{
	CR_DECLARE_STRUCT(CGroup)

public:
	CGroup() = default;
	CGroup(int _id, int _ghIndex): id(_id), ghIndex(_ghIndex) {}

	CGroup(const CGroup& ) = delete;
	CGroup(      CGroup&&) = default;

	~CGroup() = default;

	CGroup& operator = (const CGroup& ) = delete;
	CGroup& operator = (      CGroup&&) = default;

	void PostLoad();

	void Update() { RemoveIfEmptySpecialGroup(); }

	/**
	 * Note: Call unit.SetGroup(NULL) instead of calling this directly.
	 */
	void RemoveUnit(CUnit* unit);
	/**
	 * Note: Call unit.SetGroup(group) instead of calling this directly.
	 */
	bool AddUnit(CUnit* unit);
	void ClearUnits();

	float3 CalculateCenter() const;

private:
	void RemoveIfEmptySpecialGroup();

public:
	int id = -1;
	int ghIndex = -1;

	spring::unordered_set<int> units;
};

#endif // GROUP_H
