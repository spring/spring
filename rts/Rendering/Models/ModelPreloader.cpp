#include "ModelPreloader.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Misc/UnfreezeSpring.h"

void ModelPreloader::Load()
{
	// map features are loaded earlier in featureHandler.LoadFeaturesFromMap(); - not a big deal
	// Functions below are multithreaded inside because modelLoader.PreloadModel() doesn't deal with OpenGL functions
	LoadUnitDefs();
	LoadFeatureDefs();
	LoadWeaponDefs();

	modelLoader.DrainPreloadFutures(0);

	if (!globalRendering->haveGL4 || !enabled)
		return;

	// after that point we should've loaded all models, it's time to dispatch VBO/EBO/VAO creation
	S3DModelVAO::Init();
}

void ModelPreloader::Clean()
{
	if (!globalRendering->haveGL4 || !enabled)
		return;

	S3DModelVAO::Kill();
}

void ModelPreloader::LoadUnitDefs()
{
	for (const auto& def : unitDefHandler->GetUnitDefsVec()) {
		def.PreloadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}

void ModelPreloader::LoadFeatureDefs()
{
	for (const auto& def : featureDefHandler->GetFeatureDefsVec()) {
		def.PreloadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}

void ModelPreloader::LoadWeaponDefs()
{
	for (const auto& def : weaponDefHandler->GetWeaponDefsVec()) {
		def.PreloadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}