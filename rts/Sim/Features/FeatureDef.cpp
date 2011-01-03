/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDef.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/CollisionVolume.h"

CR_BIND(FeatureDef, );

CR_REG_METADATA(FeatureDef, (
	CR_MEMBER(myName),
	CR_MEMBER(description),
	CR_MEMBER(metal),
	CR_MEMBER(id),
	CR_MEMBER(energy),
	CR_MEMBER(maxHealth),
	CR_MEMBER(reclaimTime),
	CR_MEMBER(mass),
	CR_MEMBER(upright),
	CR_MEMBER(drawType),
	//CR_MEMBER(model), FIXME
	CR_MEMBER(modelname),
	CR_MEMBER(resurrectable),
	CR_MEMBER(destructable),
	CR_MEMBER(blocking),
	CR_MEMBER(burnable),
	CR_MEMBER(floating),
	CR_MEMBER(geoThermal),
	CR_MEMBER(deathFeature),
	CR_MEMBER(smokeTime),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize)
));

FeatureDef::FeatureDef()
	: collisionVolume(NULL)
	, id(-1)
	, metal(0)
	, energy(0)
	, maxHealth(0)
	, reclaimTime(0)
	, mass(0)
	, upright(false)
	, drawType(0)
	, model(NULL)
	, resurrectable(false)
	, smokeTime(0)
	, destructable(false)
	, reclaimable(true)
	, autoreclaim(true)
	, blocking(false)
	, burnable(false)
	, floating(false)
	, noSelect(false)
	, geoThermal(false)
	, xsize(0)
	, zsize(0)
{
}

FeatureDef::~FeatureDef() {
	delete collisionVolume;
	collisionVolume = NULL;
}



S3DModel* FeatureDef::LoadModel() const
{
	// not exactly kosher, but...
	FeatureDef* fdef = const_cast<FeatureDef*>(this);

	if (fdef->model == NULL) {
		fdef->model = modelParser->Load3DModel(modelname);
	}

	return (fdef->model);
}
