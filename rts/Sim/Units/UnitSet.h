/** @file UnitSet.h
 *  @brief Defines STL like container wrapper for storing CUnit pointers.
 *  @author Tobi Vollebregt
 *
 *  This file has a strong resemblence to Sim/Features/FeatureSet.h, if you find a
 *  bug in this one don't forget to update the other too. Or refactor them both
 *  using one set of template code.
 */

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
