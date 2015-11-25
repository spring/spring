/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDef.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/EventHandler.h"

CR_BIND(FeatureDef, )

CR_REG_METADATA(FeatureDef, (
	CR_MEMBER(description),
	CR_MEMBER(reclaimTime),
	CR_MEMBER(drawType),
	CR_MEMBER(resurrectable),
	CR_MEMBER(destructable),
	CR_MEMBER(autoreclaim),
	CR_MEMBER(burnable),
	CR_MEMBER(floating),
	CR_MEMBER(geoThermal),
	CR_MEMBER(deathFeatureDefID),
	CR_MEMBER(lockPosition),
	CR_MEMBER(smokeTime)
))

FeatureDef::FeatureDef()
	: deathFeatureDefID(-1)
	, reclaimTime(0)
	, drawType(DRAWTYPE_NONE)
	, resurrectable(false)
	, lockPosition(-1)
	, smokeTime(0)
	, destructable(false)
	, autoreclaim(true)
	, burnable(false)
	, floating(false)
	, geoThermal(false)
{
}

