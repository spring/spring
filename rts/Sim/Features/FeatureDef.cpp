/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDef.h"

FeatureDef::FeatureDef()
	: SolidObjectDef()
	, deathFeatureDefID(-1)
	, reclaimTime(0)
	, drawType(DRAWTYPE_NONE)
	, resurrectable(false)
	, smokeTime(0)
	, destructable(false)
	, autoreclaim(true)
	, burnable(false)
	, floating(false)
	, geoThermal(false)
{
}

