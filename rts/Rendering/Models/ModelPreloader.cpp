#include "ModelPreloader.h"

#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

void ModelPreloader::LoadUnitDefs()
{
	for (const auto& def : unitDefHandler->GetUnitDefsVec()) {
		def.LoadModel();
	}
}

void ModelPreloader::LoadFeatureDefs()
{
	for (const auto& def : featureDefHandler->GetFeatureDefsVec()) {
		def.LoadModel();
	}
}

void ModelPreloader::LoadWeaponDefs()
{
	for (const auto& def : weaponDefHandler->GetWeaponDefsVec()) {
		def.LoadModel();
	}
}