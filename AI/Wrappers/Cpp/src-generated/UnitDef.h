/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_UNITDEF_H
#define _CPPWRAPPER_UNITDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class UnitDef {

public:
	virtual ~UnitDef(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitDefId() const = 0;

	/**
	 * Forces loading of the unit model
	 */
public:
	virtual float GetHeight() = 0;

	/**
	 * Forces loading of the unit model
	 */
public:
	virtual float GetRadius() = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual const char* GetHumanName() = 0;

public:
	virtual const char* GetFileName() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual int GetAiHint() = 0;

public:
	virtual int GetCobId() = 0;

public:
	virtual int GetTechLevel() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual const char* GetGaia() = 0;

public:
	virtual float GetUpkeep(Resource* resource) = 0;

	/**
	 * This amount of the resource will always be created.
	 */
public:
	virtual float GetResourceMake(Resource* resource) = 0;

	/**
	 * This amount of the resource will be created when the unit is on and enough
	 * energy can be drained.
	 */
public:
	virtual float GetMakesResource(Resource* resource) = 0;

public:
	virtual float GetCost(Resource* resource) = 0;

public:
	virtual float GetExtractsResource(Resource* resource) = 0;

public:
	virtual float GetResourceExtractorRange(Resource* resource) = 0;

public:
	virtual float GetWindResourceGenerator(Resource* resource) = 0;

public:
	virtual float GetTidalResourceGenerator(Resource* resource) = 0;

public:
	virtual float GetStorage(Resource* resource) = 0;

public:
	virtual bool IsSquareResourceExtractor(Resource* resource) = 0;

public:
	virtual float GetBuildTime() = 0;

	/**
	 * This amount of auto-heal will always be applied.
	 */
public:
	virtual float GetAutoHeal() = 0;

	/**
	 * This amount of auto-heal will only be applied while the unit is idling.
	 */
public:
	virtual float GetIdleAutoHeal() = 0;

	/**
	 * Time a unit needs to idle before it is considered idling.
	 */
public:
	virtual int GetIdleTime() = 0;

public:
	virtual float GetPower() = 0;

public:
	virtual float GetHealth() = 0;

	/**
	 * Returns the bit field value denoting the categories this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual int GetCategory() = 0;

public:
	virtual float GetSpeed() = 0;

public:
	virtual float GetTurnRate() = 0;

public:
	virtual bool IsTurnInPlace() = 0;

	/**
	 * Units above this distance to goal will try to turn while keeping
	 * some of their speed.
	 * 0 to disable
	 */
public:
	virtual float GetTurnInPlaceDistance() = 0;

	/**
	 * Units below this speed will turn in place regardless of their
	 * turnInPlace setting.
	 */
public:
	virtual float GetTurnInPlaceSpeedLimit() = 0;

public:
	virtual bool IsUpright() = 0;

public:
	virtual bool IsCollide() = 0;

public:
	virtual float GetLosRadius() = 0;

public:
	virtual float GetAirLosRadius() = 0;

public:
	virtual float GetLosHeight() = 0;

public:
	virtual int GetRadarRadius() = 0;

public:
	virtual int GetSonarRadius() = 0;

public:
	virtual int GetJammerRadius() = 0;

public:
	virtual int GetSonarJamRadius() = 0;

public:
	virtual int GetSeismicRadius() = 0;

public:
	virtual float GetSeismicSignature() = 0;

public:
	virtual bool IsStealth() = 0;

public:
	virtual bool IsSonarStealth() = 0;

public:
	virtual bool IsBuildRange3D() = 0;

public:
	virtual float GetBuildDistance() = 0;

public:
	virtual float GetBuildSpeed() = 0;

public:
	virtual float GetReclaimSpeed() = 0;

public:
	virtual float GetRepairSpeed() = 0;

public:
	virtual float GetMaxRepairSpeed() = 0;

public:
	virtual float GetResurrectSpeed() = 0;

public:
	virtual float GetCaptureSpeed() = 0;

public:
	virtual float GetTerraformSpeed() = 0;

public:
	virtual float GetMass() = 0;

public:
	virtual bool IsPushResistant() = 0;

	/**
	 * Should the unit move sideways when it can not shoot?
	 */
public:
	virtual bool IsStrafeToAttack() = 0;

public:
	virtual float GetMinCollisionSpeed() = 0;

public:
	virtual float GetSlideTolerance() = 0;

	/**
	 * Build location relevant maximum steepness of the underlaying terrain.
	 * Used to calculate the maxHeightDif.
	 */
public:
	virtual float GetMaxSlope() = 0;

	/**
	 * Maximum terra-form height this building allows.
	 * If this value is 0.0, you can only build this structure on
	 * totally flat terrain.
	 */
public:
	virtual float GetMaxHeightDif() = 0;

public:
	virtual float GetMinWaterDepth() = 0;

public:
	virtual float GetWaterline() = 0;

public:
	virtual float GetMaxWaterDepth() = 0;

public:
	virtual float GetArmoredMultiple() = 0;

public:
	virtual int GetArmorType() = 0;

public:
	virtual float GetMaxWeaponRange() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual const char* GetType() = 0;

public:
	virtual const char* GetTooltip() = 0;

public:
	virtual const char* GetWreckName() = 0;

public:
	virtual springai::WeaponDef* GetDeathExplosion() = 0;

public:
	virtual springai::WeaponDef* GetSelfDExplosion() = 0;

	/**
	 * Returns the name of the category this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual const char* GetCategoryString() = 0;

public:
	virtual bool IsAbleToSelfD() = 0;

public:
	virtual int GetSelfDCountdown() = 0;

public:
	virtual bool IsAbleToSubmerge() = 0;

public:
	virtual bool IsAbleToFly() = 0;

public:
	virtual bool IsAbleToMove() = 0;

public:
	virtual bool IsAbleToHover() = 0;

public:
	virtual bool IsFloater() = 0;

public:
	virtual bool IsBuilder() = 0;

public:
	virtual bool IsActivateWhenBuilt() = 0;

public:
	virtual bool IsOnOffable() = 0;

public:
	virtual bool IsFullHealthFactory() = 0;

public:
	virtual bool IsFactoryHeadingTakeoff() = 0;

public:
	virtual bool IsReclaimable() = 0;

public:
	virtual bool IsCapturable() = 0;

public:
	virtual bool IsAbleToRestore() = 0;

public:
	virtual bool IsAbleToRepair() = 0;

public:
	virtual bool IsAbleToSelfRepair() = 0;

public:
	virtual bool IsAbleToReclaim() = 0;

public:
	virtual bool IsAbleToAttack() = 0;

public:
	virtual bool IsAbleToPatrol() = 0;

public:
	virtual bool IsAbleToFight() = 0;

public:
	virtual bool IsAbleToGuard() = 0;

public:
	virtual bool IsAbleToAssist() = 0;

public:
	virtual bool IsAssistable() = 0;

public:
	virtual bool IsAbleToRepeat() = 0;

public:
	virtual bool IsAbleToFireControl() = 0;

public:
	virtual int GetFireState() = 0;

public:
	virtual int GetMoveState() = 0;

public:
	virtual float GetWingDrag() = 0;

public:
	virtual float GetWingAngle() = 0;

public:
	virtual float GetDrag() = 0;

public:
	virtual float GetFrontToSpeed() = 0;

public:
	virtual float GetSpeedToFront() = 0;

public:
	virtual float GetMyGravity() = 0;

public:
	virtual float GetMaxBank() = 0;

public:
	virtual float GetMaxPitch() = 0;

public:
	virtual float GetTurnRadius() = 0;

public:
	virtual float GetWantedHeight() = 0;

public:
	virtual float GetVerticalSpeed() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual bool IsAbleToCrash() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual bool IsHoverAttack() = 0;

public:
	virtual bool IsAirStrafe() = 0;

	/**
	 * @return  < 0:  it can land
	 *          >= 0: how much the unit will move during hovering on the spot
	 */
public:
	virtual float GetDlHoverFactor() = 0;

public:
	virtual float GetMaxAcceleration() = 0;

public:
	virtual float GetMaxDeceleration() = 0;

public:
	virtual float GetMaxAileron() = 0;

public:
	virtual float GetMaxElevator() = 0;

public:
	virtual float GetMaxRudder() = 0;

	/**
	 * The yard map defines which parts of the square a unit occupies
	 * can still be walked on by other units.
	 * Example:
	 * In the BA Arm T2 K-Bot lab, htere is a line in hte middle where units
	 * walk, otherwise they would not be able ot exit the lab once they are
	 * built.
	 * @return 0 if invalid facing or the unit has no yard-map defined,
	 *         the size of the yard-map otherwise: getXSize() * getXSize()
	 */
public:
	virtual std::vector<short> GetYardMap(int facing) = 0;

public:
	virtual int GetXSize() = 0;

public:
	virtual int GetZSize() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual int GetBuildAngle() = 0;

public:
	virtual float GetLoadingRadius() = 0;

public:
	virtual float GetUnloadSpread() = 0;

public:
	virtual int GetTransportCapacity() = 0;

public:
	virtual int GetTransportSize() = 0;

public:
	virtual int GetMinTransportSize() = 0;

public:
	virtual bool IsAirBase() = 0;

public:
	virtual bool IsFirePlatform() = 0;

public:
	virtual float GetTransportMass() = 0;

public:
	virtual float GetMinTransportMass() = 0;

public:
	virtual bool IsHoldSteady() = 0;

public:
	virtual bool IsReleaseHeld() = 0;

public:
	virtual bool IsNotTransportable() = 0;

public:
	virtual bool IsTransportByEnemy() = 0;

	/**
	 * @return  0: land unload
	 *          1: fly-over drop
	 *          2: land flood
	 */
public:
	virtual int GetTransportUnloadMethod() = 0;

	/**
	 * Dictates fall speed of all transported units.
	 * This only makes sense for air transports,
	 * if they an drop units while in the air.
	 */
public:
	virtual float GetFallSpeed() = 0;

	/**
	 * Sets the transported units FBI, overrides fallSpeed
	 */
public:
	virtual float GetUnitFallSpeed() = 0;

	/**
	 * If the unit can cloak
	 */
public:
	virtual bool IsAbleToCloak() = 0;

	/**
	 * If the unit wants to start out cloaked
	 */
public:
	virtual bool IsStartCloaked() = 0;

	/**
	 * Energy cost per second to stay cloaked when stationary
	 */
public:
	virtual float GetCloakCost() = 0;

	/**
	 * Energy cost per second to stay cloaked when moving
	 */
public:
	virtual float GetCloakCostMoving() = 0;

	/**
	 * If enemy unit comes within this range, decloaking is forced
	 */
public:
	virtual float GetDecloakDistance() = 0;

	/**
	 * Use a spherical, instead of a cylindrical test?
	 */
public:
	virtual bool IsDecloakSpherical() = 0;

	/**
	 * Will the unit decloak upon firing?
	 */
public:
	virtual bool IsDecloakOnFire() = 0;

	/**
	 * Will the unit self destruct if an enemy comes to close?
	 */
public:
	virtual bool IsAbleToKamikaze() = 0;

public:
	virtual float GetKamikazeDist() = 0;

public:
	virtual bool IsTargetingFacility() = 0;

public:
	virtual bool CanManualFire() = 0;

public:
	virtual bool IsNeedGeo() = 0;

public:
	virtual bool IsFeature() = 0;

public:
	virtual bool IsHideDamage() = 0;

public:
	virtual bool IsCommander() = 0;

public:
	virtual bool IsShowPlayerName() = 0;

public:
	virtual bool IsAbleToResurrect() = 0;

public:
	virtual bool IsAbleToCapture() = 0;

	/**
	 * Indicates the trajectory types supported by this unit.
	 * 
	 * @return  0: (default) = only low
	 *          1: only high
	 *          2: choose
	 */
public:
	virtual int GetHighTrajectoryType() = 0;

	/**
	 * Returns the bit field value denoting the categories this unit shall not
	 * chase.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual int GetNoChaseCategory() = 0;

public:
	virtual bool IsLeaveTracks() = 0;

public:
	virtual float GetTrackWidth() = 0;

public:
	virtual float GetTrackOffset() = 0;

public:
	virtual float GetTrackStrength() = 0;

public:
	virtual float GetTrackStretch() = 0;

public:
	virtual int GetTrackType() = 0;

public:
	virtual bool IsAbleToDropFlare() = 0;

public:
	virtual float GetFlareReloadTime() = 0;

public:
	virtual float GetFlareEfficiency() = 0;

public:
	virtual float GetFlareDelay() = 0;

public:
	virtual springai::AIFloat3 GetFlareDropVector() = 0;

public:
	virtual int GetFlareTime() = 0;

public:
	virtual int GetFlareSalvoSize() = 0;

public:
	virtual int GetFlareSalvoDelay() = 0;

	/**
	 * Only matters for fighter aircraft
	 */
public:
	virtual bool IsAbleToLoopbackAttack() = 0;

	/**
	 * Indicates whether the ground will be leveled/flattened out
	 * after this building has been built on it.
	 * Only matters for buildings.
	 */
public:
	virtual bool IsLevelGround() = 0;

public:
	virtual bool IsUseBuildingGroundDecal() = 0;

public:
	virtual int GetBuildingDecalType() = 0;

public:
	virtual int GetBuildingDecalSizeX() = 0;

public:
	virtual int GetBuildingDecalSizeY() = 0;

public:
	virtual float GetBuildingDecalDecaySpeed() = 0;

	/**
	 * Maximum flight time in seconds before the aircraft needs
	 * to return to an air repair bay to refuel.
	 */
public:
	virtual float GetMaxFuel() = 0;

	/**
	 * Time to fully refuel the unit
	 */
public:
	virtual float GetRefuelTime() = 0;

	/**
	 * Minimum build power of airbases that this aircraft can land on
	 */
public:
	virtual float GetMinAirBasePower() = 0;

	/**
	 * Number of units of this type allowed simultaneously in the game
	 */
public:
	virtual int GetMaxThisUnit() = 0;

public:
	virtual springai::UnitDef* GetDecoyDef() = 0;

public:
	virtual bool IsDontLand() = 0;

public:
	virtual springai::WeaponDef* GetShieldDef() = 0;

public:
	virtual springai::WeaponDef* GetStockpileDef() = 0;

public:
	virtual std::vector<springai::UnitDef*> GetBuildOptions() = 0;

public:
	virtual std::map<std::string,std::string> GetCustomParams() = 0;

public:
	virtual std::vector<springai::WeaponMount*> GetWeaponMounts() = 0;

public:
	virtual springai::MoveData* GetMoveData() = 0;

public:
	virtual springai::FlankingBonus* GetFlankingBonus() = 0;

}; // class UnitDef

}  // namespace springai

#endif // _CPPWRAPPER_UNITDEF_H

