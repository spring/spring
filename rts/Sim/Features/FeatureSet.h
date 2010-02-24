/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURESET_H
#define FEATURESET_H

#include <set>
#include "Feature.h"

class FeatureComparator
{
public:
	bool operator()(CFeature* f1, CFeature* f2) const
	{
		return f1->id < f2->id;
	}
};

typedef std::set<CFeature*, FeatureComparator> CFeatureSet;

#endif // !defined(FEATURESET_H)
