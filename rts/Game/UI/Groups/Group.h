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
class CGroupHandler;

/**
 * Logic group of units denoted by a number.
 * A group-ID/-number is unique per team (-> per groupHandler).
 */
class CGroup
{
	CR_DECLARE_STRUCT(CGroup)

public:
	CGroup(int id, CGroupHandler* groupHandler);
	~CGroup();
	void PostLoad();

	void Update();

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
	int id;
	spring::unordered_set<int> units;

private:
	CGroupHandler* handler;
};

#endif // GROUP_H
