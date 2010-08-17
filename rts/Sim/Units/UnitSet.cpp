#include "UnitSet.h"
#include "Unit.h"

bool UnitComparator::operator() (const CUnit* u1, const CUnit* u2) const
{
	return (u1->id < u2->id);
}
