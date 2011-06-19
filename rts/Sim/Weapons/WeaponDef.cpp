/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Rendering/Models/IModelParser.h"
#include "lib/gml/gmlcnf.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/Platform/Threading.h"

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
#if defined(USE_GML) && GML_ENABLE_SIM && GML_SHARE_LISTS 
	else if (!Threading::IsSimThread())
		texturehandlerS3O->UpdateDraw();
#endif
	return visuals.model;
}

S3DModel* WeaponDef::LoadModel() const {
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

