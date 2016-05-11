/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_DEF_H
#define FEATURE_DEF_H

#include "Sim/Objects/SolidObjectDef.h"
#include "System/float3.h"

enum {
	DRAWTYPE_MODEL = 0,
	DRAWTYPE_TREE  = 1, // >= different types of trees
	DRAWTYPE_NONE = -1,
};



struct FeatureDef: public SolidObjectDef
{
	FeatureDef();

	std::string description;
	/// feature that this turn into when killed (not reclaimed)
	int deathFeatureDefID;

	float reclaimTime;

	int drawType;

	/// -1 := only if it is the 1st wreckage of the unitdef (default), 0 := no it isn't, 1 := yes it is
	int resurrectable;

	int smokeTime;

	bool destructable;
	bool autoreclaim;
	bool burnable;
	bool floating;
	bool geoThermal;
};

#endif
