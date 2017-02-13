/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ProjectileFunctors.h"
#include "Projectile.h"


bool ProjectileDistanceComparator::operator() (const CProjectile* arg1, const CProjectile* arg2) const {
	if (arg1->GetSortDist() != arg2->GetSortDist()) // strict ordering required
		return (arg1->GetSortDist() > arg2->GetSortDist());
	return (arg1 > arg2);
}
