/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITSET_H
#define UNITSET_H

#include <set>
#include "Unit.h"

class UnitComparator
{
public:
	bool operator()(CUnit* u1, CUnit* u2) const
	{
		return u1->id < u2->id;
	}
};

typedef std::set<CUnit*, UnitComparator> CUnitSet;

#endif // !defined(UNITSET_H)
