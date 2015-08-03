/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappUnitDef.h"

#include "IncludesSources.h"

	springai::WrappUnitDef::WrappUnitDef(int skirmishAIId, int unitDefId) {

		this->skirmishAIId = skirmishAIId;
		this->unitDefId = unitDefId;
	}

	springai::WrappUnitDef::~WrappUnitDef() {

	}

	int springai::WrappUnitDef::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappUnitDef::GetUnitDefId() const {

		return unitDefId;
	}

	springai::WrappUnitDef::UnitDef* springai::WrappUnitDef::GetInstance(int skirmishAIId, int unitDefId) {

		if (unitDefId < 0) {
			return NULL;
		}

		springai::UnitDef* internal_ret = NULL;
		internal_ret = new springai::WrappUnitDef(skirmishAIId, unitDefId);
		return internal_ret;
	}


	float springai::WrappUnitDef::GetHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getHeight(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetRadius() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetHumanName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getHumanName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetFileName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFileName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetAiHint() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getAiHint(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetCobId() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCobId(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetTechLevel() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTechLevel(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetGaia() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getGaia(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetUpkeep(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getUpkeep(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetResourceMake(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getResourceMake(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMakesResource(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getMakesResource(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetCost(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getCost(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetExtractsResource(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getExtractsResource(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetResourceExtractorRange(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getResourceExtractorRange(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetWindResourceGenerator(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getWindResourceGenerator(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTidalResourceGenerator(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getTidalResourceGenerator(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetStorage(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_getStorage(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsSquareResourceExtractor(Resource* resource) {

		bool internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_UnitDef_isSquareResourceExtractor(this->GetSkirmishAIId(), this->GetUnitDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetBuildTime() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildTime(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetAutoHeal() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getAutoHeal(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetIdleAutoHeal() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getIdleAutoHeal(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetIdleTime() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getIdleTime(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetPower() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getPower(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetHealth() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getHealth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetCategory() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCategory(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTurnRate() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTurnRate(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsTurnInPlace() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isTurnInPlace(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTurnInPlaceDistance() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTurnInPlaceDistance(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTurnInPlaceSpeedLimit() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTurnInPlaceSpeedLimit(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsUpright() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isUpright(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsCollide() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isCollide(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetLosRadius() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getLosRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetAirLosRadius() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getAirLosRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetLosHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getLosHeight(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetRadarRadius() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getRadarRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetSonarRadius() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSonarRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetJammerRadius() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getJammerRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetSonarJamRadius() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSonarJamRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetSeismicRadius() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSeismicRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetSeismicSignature() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSeismicSignature(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsStealth() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isStealth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsSonarStealth() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isSonarStealth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsBuildRange3D() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isBuildRange3D(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetBuildDistance() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildDistance(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetBuildSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetReclaimSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getReclaimSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetRepairSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getRepairSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxRepairSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxRepairSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetResurrectSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getResurrectSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetCaptureSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCaptureSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTerraformSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTerraformSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMass() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMass(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsPushResistant() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isPushResistant(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsStrafeToAttack() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isStrafeToAttack(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMinCollisionSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMinCollisionSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetSlideTolerance() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSlideTolerance(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxSlope() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxSlope(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxHeightDif() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxHeightDif(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMinWaterDepth() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMinWaterDepth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetWaterline() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWaterline(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxWaterDepth() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxWaterDepth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetArmoredMultiple() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getArmoredMultiple(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetArmorType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getArmorType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxWeaponRange() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxWeaponRange(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetType() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetTooltip() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTooltip(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitDef::GetWreckName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWreckName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	springai::WeaponDef* springai::WrappUnitDef::GetDeathExplosion() {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getDeathExplosion(this->GetSkirmishAIId(), this->GetUnitDefId());
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::WeaponDef* springai::WrappUnitDef::GetSelfDExplosion() {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSelfDExplosion(this->GetSkirmishAIId(), this->GetUnitDefId());
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	const char* springai::WrappUnitDef::GetCategoryString() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCategoryString(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToSelfD() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToSelfD(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetSelfDCountdown() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSelfDCountdown(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToSubmerge() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToSubmerge(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToFly() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToFly(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToMove() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToMove(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToHover() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToHover(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsFloater() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isFloater(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsBuilder() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isBuilder(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsActivateWhenBuilt() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isActivateWhenBuilt(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsOnOffable() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isOnOffable(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsFullHealthFactory() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isFullHealthFactory(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsFactoryHeadingTakeoff() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isFactoryHeadingTakeoff(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsReclaimable() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isReclaimable(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsCapturable() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isCapturable(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToRestore() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToRestore(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToRepair() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToRepair(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToSelfRepair() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToSelfRepair(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToReclaim() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToReclaim(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToAttack() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToAttack(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToPatrol() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToPatrol(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToFight() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToFight(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToGuard() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToGuard(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToAssist() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToAssist(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAssistable() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAssistable(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToRepeat() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToRepeat(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToFireControl() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToFireControl(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetFireState() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFireState(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetMoveState() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMoveState(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetWingDrag() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWingDrag(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetWingAngle() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWingAngle(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetDrag() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getDrag(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetFrontToSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFrontToSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetSpeedToFront() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getSpeedToFront(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMyGravity() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMyGravity(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxBank() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxBank(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxPitch() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxPitch(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTurnRadius() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTurnRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetWantedHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWantedHeight(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetVerticalSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getVerticalSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToCrash() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToCrash(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsHoverAttack() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isHoverAttack(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAirStrafe() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAirStrafe(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetDlHoverFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getDlHoverFactor(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxAcceleration() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxAcceleration(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxDeceleration() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxDeceleration(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxAileron() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxAileron(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxElevator() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxElevator(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxRudder() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxRudder(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	std::vector<short> springai::WrappUnitDef::GetYardMap(int facing) {

		int yardMap_sizeMax;
		int yardMap_raw_size;
		short* yardMap;
		std::vector<short> yardMap_list;
		int yardMap_size;
		int internal_ret_int;

		yardMap_sizeMax = INT_MAX;
		yardMap = NULL;
		yardMap_size = bridged_UnitDef_getYardMap(this->GetSkirmishAIId(), this->GetUnitDefId(), facing, yardMap, yardMap_sizeMax);
		yardMap_sizeMax = yardMap_size;
		yardMap_raw_size = yardMap_size;
		yardMap = new short[yardMap_raw_size];

		internal_ret_int = bridged_UnitDef_getYardMap(this->GetSkirmishAIId(), this->GetUnitDefId(), facing, yardMap, yardMap_sizeMax);
		yardMap_list.reserve(yardMap_size);
		for (int i=0; i < yardMap_sizeMax; ++i) {
			yardMap_list.push_back(yardMap[i]);
		}
		delete[] yardMap;

		return yardMap_list;
	}

	int springai::WrappUnitDef::GetXSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getXSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetZSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getZSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetBuildAngle() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildAngle(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetLoadingRadius() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getLoadingRadius(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetUnloadSpread() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getUnloadSpread(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetTransportCapacity() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTransportCapacity(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetTransportSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTransportSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetMinTransportSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMinTransportSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAirBase() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAirBase(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsFirePlatform() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isFirePlatform(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTransportMass() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTransportMass(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMinTransportMass() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMinTransportMass(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsHoldSteady() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isHoldSteady(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsReleaseHeld() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isReleaseHeld(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsNotTransportable() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isNotTransportable(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsTransportByEnemy() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isTransportByEnemy(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetTransportUnloadMethod() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTransportUnloadMethod(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetFallSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFallSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetUnitFallSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getUnitFallSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToCloak() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToCloak(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsStartCloaked() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isStartCloaked(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetCloakCost() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCloakCost(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetCloakCostMoving() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getCloakCostMoving(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetDecloakDistance() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getDecloakDistance(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsDecloakSpherical() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isDecloakSpherical(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsDecloakOnFire() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isDecloakOnFire(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToKamikaze() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToKamikaze(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetKamikazeDist() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getKamikazeDist(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsTargetingFacility() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isTargetingFacility(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::CanManualFire() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_canManualFire(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsNeedGeo() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isNeedGeo(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsFeature() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isFeature(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsHideDamage() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isHideDamage(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsCommander() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isCommander(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsShowPlayerName() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isShowPlayerName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToResurrect() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToResurrect(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToCapture() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToCapture(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetHighTrajectoryType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getHighTrajectoryType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetNoChaseCategory() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getNoChaseCategory(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsLeaveTracks() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isLeaveTracks(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTrackWidth() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTrackWidth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTrackOffset() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTrackOffset(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTrackStrength() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTrackStrength(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetTrackStretch() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTrackStretch(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetTrackType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getTrackType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToDropFlare() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToDropFlare(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetFlareReloadTime() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareReloadTime(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetFlareEfficiency() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareEfficiency(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetFlareDelay() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareDelay(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappUnitDef::GetFlareDropVector() {

		float return_posF3_out[3];

		bridged_UnitDef_getFlareDropVector(this->GetSkirmishAIId(), this->GetUnitDefId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	int springai::WrappUnitDef::GetFlareTime() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareTime(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetFlareSalvoSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareSalvoSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetFlareSalvoDelay() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getFlareSalvoDelay(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsAbleToLoopbackAttack() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isAbleToLoopbackAttack(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsLevelGround() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isLevelGround(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappUnitDef::IsUseBuildingGroundDecal() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isUseBuildingGroundDecal(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetBuildingDecalType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildingDecalType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetBuildingDecalSizeX() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildingDecalSizeX(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetBuildingDecalSizeY() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildingDecalSizeY(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetBuildingDecalDecaySpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getBuildingDecalDecaySpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMaxFuel() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxFuel(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetRefuelTime() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getRefuelTime(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappUnitDef::GetMinAirBasePower() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMinAirBasePower(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappUnitDef::GetMaxThisUnit() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getMaxThisUnit(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	springai::UnitDef* springai::WrappUnitDef::GetDecoyDef() {

		UnitDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getDecoyDef(this->GetSkirmishAIId(), this->GetUnitDefId());
		internal_ret_int_out = springai::WrappUnitDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	bool springai::WrappUnitDef::IsDontLand() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_isDontLand(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	springai::WeaponDef* springai::WrappUnitDef::GetShieldDef() {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getShieldDef(this->GetSkirmishAIId(), this->GetUnitDefId());
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::WeaponDef* springai::WrappUnitDef::GetStockpileDef() {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getStockpileDef(this->GetSkirmishAIId(), this->GetUnitDefId());
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	std::vector<springai::UnitDef*> springai::WrappUnitDef::GetBuildOptions() {

		int unitDefIds_sizeMax;
		int unitDefIds_raw_size;
		int* unitDefIds;
		std::vector<springai::UnitDef*> unitDefIds_list;
		int unitDefIds_size;
		int internal_ret_int;

		unitDefIds_sizeMax = INT_MAX;
		unitDefIds = NULL;
		unitDefIds_size = bridged_UnitDef_getBuildOptions(this->GetSkirmishAIId(), this->GetUnitDefId(), unitDefIds, unitDefIds_sizeMax);
		unitDefIds_sizeMax = unitDefIds_size;
		unitDefIds_raw_size = unitDefIds_size;
		unitDefIds = new int[unitDefIds_raw_size];

		internal_ret_int = bridged_UnitDef_getBuildOptions(this->GetSkirmishAIId(), this->GetUnitDefId(), unitDefIds, unitDefIds_sizeMax);
		unitDefIds_list.reserve(unitDefIds_size);
		for (int i=0; i < unitDefIds_sizeMax; ++i) {
			unitDefIds_list.push_back(springai::WrappUnitDef::GetInstance(skirmishAIId, unitDefIds[i]));
		}
		delete[] unitDefIds;

		return unitDefIds_list;
	}

	std::map<std::string,std::string> springai::WrappUnitDef::GetCustomParams() {

		std::map<std::string,std::string> internal_map;
		int internal_size;
		int internal_ret_int;

		internal_size = bridged_UnitDef_getCustomParams(this->GetSkirmishAIId(), this->GetUnitDefId(), NULL, NULL);
		const char** keys = new const char*[internal_size];
		const char** values = new const char*[internal_size];

		internal_ret_int = bridged_UnitDef_getCustomParams(this->GetSkirmishAIId(), this->GetUnitDefId(), keys, values);
		for (int i=0; i < internal_size; i++) {
			internal_map[keys[i]] = values[i];
		}

		delete[] keys;
		delete[] values;

		return internal_map;
	}

	std::vector<springai::WeaponMount*> springai::WrappUnitDef::GetWeaponMounts() {

		std::vector<springai::WeaponMount*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_getWeaponMounts(this->GetSkirmishAIId(), this->GetUnitDefId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappWeaponMount::GetInstance(skirmishAIId, unitDefId, i));
		}

		return RETURN_SIZE_list;
	}

	springai::MoveData* springai::WrappUnitDef::GetMoveData() {

		MoveData* internal_ret_int_out;

		internal_ret_int_out = springai::WrappMoveData::GetInstance(skirmishAIId, unitDefId);

		return internal_ret_int_out;
	}

	springai::FlankingBonus* springai::WrappUnitDef::GetFlankingBonus() {

		FlankingBonus* internal_ret_int_out;

		internal_ret_int_out = springai::WrappFlankingBonus::GetInstance(skirmishAIId, unitDefId);

		return internal_ret_int_out;
	}
