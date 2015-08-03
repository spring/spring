/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappWeaponDef.h"

#include "IncludesSources.h"

	springai::WrappWeaponDef::WrappWeaponDef(int skirmishAIId, int weaponDefId) {

		this->skirmishAIId = skirmishAIId;
		this->weaponDefId = weaponDefId;
	}

	springai::WrappWeaponDef::~WrappWeaponDef() {

	}

	int springai::WrappWeaponDef::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappWeaponDef::GetWeaponDefId() const {

		return weaponDefId;
	}

	springai::WrappWeaponDef::WeaponDef* springai::WrappWeaponDef::GetInstance(int skirmishAIId, int weaponDefId) {

		if (weaponDefId < 0) {
			return NULL;
		}

		springai::WeaponDef* internal_ret = NULL;
		internal_ret = new springai::WrappWeaponDef(skirmishAIId, weaponDefId);
		return internal_ret;
	}


	const char* springai::WrappWeaponDef::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getName(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	const char* springai::WrappWeaponDef::GetType() {

		const char* internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getType(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	const char* springai::WrappWeaponDef::GetDescription() {

		const char* internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDescription(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	const char* springai::WrappWeaponDef::GetFileName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getFileName(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	const char* springai::WrappWeaponDef::GetCegTag() {

		const char* internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCegTag(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetRange() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getRange(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetHeightMod() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getHeightMod(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetAccuracy() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getAccuracy(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetSprayAngle() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getSprayAngle(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetMovingAccuracy() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getMovingAccuracy(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetTargetMoveError() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getTargetMoveError(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetLeadLimit() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getLeadLimit(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetLeadBonus() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getLeadBonus(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetPredictBoost() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getPredictBoost(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetNumDamageTypes() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getNumDamageTypes(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetAreaOfEffect() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getAreaOfEffect(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsNoSelfDamage() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isNoSelfDamage(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetFireStarter() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getFireStarter(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetEdgeEffectiveness() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getEdgeEffectiveness(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetSize() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getSize(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetSizeGrowth() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getSizeGrowth(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCollisionSize() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCollisionSize(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetSalvoSize() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getSalvoSize(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetSalvoDelay() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getSalvoDelay(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetReload() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getReload(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetBeamTime() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getBeamTime(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsBeamBurst() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isBeamBurst(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsWaterBounce() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isWaterBounce(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsGroundBounce() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isGroundBounce(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetBounceRebound() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getBounceRebound(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetBounceSlip() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getBounceSlip(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetNumBounce() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getNumBounce(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetMaxAngle() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getMaxAngle(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetUpTime() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getUpTime(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetFlightTime() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getFlightTime(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCost(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_WeaponDef_getCost(this->GetSkirmishAIId(), this->GetWeaponDefId(), resourceId);
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetProjectilesPerShot() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getProjectilesPerShot(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsTurret() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isTurret(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsOnlyForward() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isOnlyForward(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsFixedLauncher() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isFixedLauncher(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsWaterWeapon() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isWaterWeapon(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsFireSubmersed() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isFireSubmersed(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsSubMissile() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isSubMissile(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsTracks() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isTracks(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsDropped() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isDropped(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsParalyzer() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isParalyzer(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsImpactOnly() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isImpactOnly(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsNoAutoTarget() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isNoAutoTarget(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsManualFire() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isManualFire(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetInterceptor() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getInterceptor(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetTargetable() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getTargetable(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsStockpileable() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isStockpileable(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCoverageRange() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCoverageRange(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetStockpileTime() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getStockpileTime(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetIntensity() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getIntensity(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetThickness() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getThickness(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetLaserFlareSize() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getLaserFlareSize(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCoreThickness() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCoreThickness(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetDuration() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDuration(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetLodDistance() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getLodDistance(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetFalloffRate() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getFalloffRate(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetGraphicsType() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getGraphicsType(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsSoundTrigger() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isSoundTrigger(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsSelfExplode() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isSelfExplode(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsGravityAffected() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isGravityAffected(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetHighTrajectory() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getHighTrajectory(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetMyGravity() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getMyGravity(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsNoExplode() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isNoExplode(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetStartVelocity() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getStartVelocity(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetWeaponAcceleration() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getWeaponAcceleration(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetTurnRate() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getTurnRate(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetMaxVelocity() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getMaxVelocity(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetProjectileSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getProjectileSpeed(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetExplosionSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getExplosionSpeed(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetOnlyTargetCategory() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getOnlyTargetCategory(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetWobble() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getWobble(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetDance() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDance(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetTrajectoryHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getTrajectoryHeight(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsLargeBeamLaser() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isLargeBeamLaser(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsShield() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isShield(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsShieldRepulser() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isShieldRepulser(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsSmartShield() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isSmartShield(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsExteriorShield() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isExteriorShield(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsVisibleShield() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isVisibleShield(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsVisibleShieldRepulse() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isVisibleShieldRepulse(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetVisibleShieldHitFrames() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getVisibleShieldHitFrames(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetInterceptedByShieldType() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getInterceptedByShieldType(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsAvoidFriendly() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isAvoidFriendly(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsAvoidFeature() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isAvoidFeature(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsAvoidNeutral() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isAvoidNeutral(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetTargetBorder() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getTargetBorder(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCylinderTargetting() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCylinderTargetting(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetMinIntensity() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getMinIntensity(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetHeightBoostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getHeightBoostFactor(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetProximityPriority() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getProximityPriority(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	int springai::WrappWeaponDef::GetCollisionFlags() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCollisionFlags(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsSweepFire() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isSweepFire(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsAbleToAttackGround() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isAbleToAttackGround(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetCameraShake() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getCameraShake(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetDynDamageExp() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDynDamageExp(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetDynDamageMin() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDynDamageMin(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappWeaponDef::GetDynDamageRange() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_getDynDamageRange(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	bool springai::WrappWeaponDef::IsDynDamageInverted() {

		bool internal_ret_int;

		internal_ret_int = bridged_WeaponDef_isDynDamageInverted(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	std::map<std::string,std::string> springai::WrappWeaponDef::GetCustomParams() {

		std::map<std::string,std::string> internal_map;
		int internal_size;
		int internal_ret_int;

		internal_size = bridged_WeaponDef_getCustomParams(this->GetSkirmishAIId(), this->GetWeaponDefId(), NULL, NULL);
		const char** keys = new const char*[internal_size];
		const char** values = new const char*[internal_size];

		internal_ret_int = bridged_WeaponDef_getCustomParams(this->GetSkirmishAIId(), this->GetWeaponDefId(), keys, values);
		for (int i=0; i < internal_size; i++) {
			internal_map[keys[i]] = values[i];
		}

		delete[] keys;
		delete[] values;

		return internal_map;
	}

	springai::Damage* springai::WrappWeaponDef::GetDamage() {

		Damage* internal_ret_int_out;

		internal_ret_int_out = springai::WrappDamage::GetInstance(skirmishAIId, weaponDefId);

		return internal_ret_int_out;
	}

	springai::Shield* springai::WrappWeaponDef::GetShield() {

		Shield* internal_ret_int_out;

		internal_ret_int_out = springai::WrappShield::GetInstance(skirmishAIId, weaponDefId);

		return internal_ret_int_out;
	}
