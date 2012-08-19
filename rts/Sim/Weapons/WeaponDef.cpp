/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Rendering/Models/IModelParser.h"
#include "System/EventHandler.h"

WeaponDef::~WeaponDef()
{
	if (explosionGenerator != NULL) {
		explGenHandler->UnloadGenerator(explosionGenerator);
		explosionGenerator = NULL;
	}
	if (bounceExplosionGenerator != NULL) {
		explGenHandler->UnloadGenerator(bounceExplosionGenerator);
		bounceExplosionGenerator = NULL;
	}
}


S3DModel* WeaponDef::LoadModel()
{
	if ((visuals.model == NULL) && !visuals.modelName.empty()) {
		visuals.model = modelParser->Load3DModel(visuals.modelName);
	} else {
		eventHandler.LoadedModelRequested();
	}

	return visuals.model;
}

S3DModel* WeaponDef::LoadModel() const {
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

