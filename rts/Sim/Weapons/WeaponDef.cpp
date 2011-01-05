/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Rendering/Models/IModelParser.h"

WeaponDef::~WeaponDef()
{
	delete explosionGenerator; explosionGenerator = NULL;
	delete bounceExplosionGenerator; bounceExplosionGenerator = NULL;
}


S3DModel* WeaponDef::LoadModel()
{
	if ((visuals.model==NULL) && (!visuals.modelName.empty())) {
		std::string modelname = "objects3d/" + visuals.modelName;
		if (modelname.find(".") == std::string::npos) {
			modelname += ".3do";
		}
		visuals.model = modelParser->Load3DModel(modelname);
	}
	return visuals.model;
}
