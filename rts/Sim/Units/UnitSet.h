/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_SET_H
#define UNIT_SET_H

#include <set>

class CUnit;

class UnitComparator
{
public:
	bool operator() (const CUnit* u1, const CUnit* u2) const;
};

typedef std::set<CUnit*, UnitComparator> CUnitSet;

#endif // UNIT_SET_H
