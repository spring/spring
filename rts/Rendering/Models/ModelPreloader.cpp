#include "ModelPreloader.h"

#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Misc/UnfreezeSpring.h"

void ModelPreloader::LoadUnitDefs()
{
	for (const auto& def : unitDefHandler->GetUnitDefsVec()) {
		def.LoadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}

void ModelPreloader::LoadFeatureDefs()
{
	for (const auto& def : featureDefHandler->GetFeatureDefsVec()) {
		def.LoadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}

void ModelPreloader::LoadWeaponDefs()
{
	for (const auto& def : weaponDefHandler->GetWeaponDefsVec()) {
		def.LoadModel();
		spring::UnfreezeSpring(WDT_LOAD, 10);
	}
}