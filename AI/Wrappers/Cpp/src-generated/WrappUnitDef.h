/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPUNITDEF_H
#define _CPPWRAPPER_WRAPPUNITDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "UnitDef.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappUnitDef : public UnitDef {

private:
	int skirmishAIId;
	int unitDefId;

	WrappUnitDef(int skirmishAIId, int unitDefId);
	virtual ~WrappUnitDef();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitDefId() const;
public:
	static UnitDef* GetInstance(int skirmishAIId, int unitDefId);

	/**
	 * Forces loading of the unit model
	 */
public:
	// @Override
	virtual float GetHeight();

	/**
	 * Forces loading of the unit model
	 */
public:
	// @Override
	virtual float GetRadius();

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual const char* GetHumanName();

public:
	// @Override
	virtual const char* GetFileName();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual int GetAiHint();

public:
	// @Override
	virtual int GetCobId();

public:
	// @Override
	virtual int GetTechLevel();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual const char* GetGaia();

public:
	// @Override
	virtual float GetUpkeep(Resource* resource);

	/**
	 * This amount of the resource will always be created.
	 */
public:
	// @Override
	virtual float GetResourceMake(Resource* resource);

	/**
	 * This amount of the resource will be created when the unit is on and enough
	 * energy can be drained.
	 */
public:
	// @Override
	virtual float GetMakesResource(Resource* resource);

public:
	// @Override
	virtual float GetCost(Resource* resource);

public:
	// @Override
	virtual float GetExtractsResource(Resource* resource);

public:
	// @Override
	virtual float GetResourceExtractorRange(Resource* resource);

public:
	// @Override
	virtual float GetWindResourceGenerator(Resource* resource);

public:
	// @Override
	virtual float GetTidalResourceGenerator(Resource* resource);

public:
	// @Override
	virtual float GetStorage(Resource* resource);

public:
	// @Override
	virtual bool IsSquareResourceExtractor(Resource* resource);

public:
	// @Override
	virtual float GetBuildTime();

	/**
	 * This amount of auto-heal will always be applied.
	 */
public:
	// @Override
	virtual float GetAutoHeal();

	/**
	 * This amount of auto-heal will only be applied while the unit is idling.
	 */
public:
	// @Override
	virtual float GetIdleAutoHeal();

	/**
	 * Time a unit needs to idle before it is considered idling.
	 */
public:
	// @Override
	virtual int GetIdleTime();

public:
	// @Override
	virtual float GetPower();

public:
	// @Override
	virtual float GetHealth();

	/**
	 * Returns the bit field value denoting the categories this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual int GetCategory();

public:
	// @Override
	virtual float GetSpeed();

public:
	// @Override
	virtual float GetTurnRate();

public:
	// @Override
	virtual bool IsTurnInPlace();

	/**
	 * Units above this distance to goal will try to turn while keeping
	 * some of their speed.
	 * 0 to disable
	 */
public:
	// @Override
	virtual float GetTurnInPlaceDistance();

	/**
	 * Units below this speed will turn in place regardless of their
	 * turnInPlace setting.
	 */
public:
	// @Override
	virtual float GetTurnInPlaceSpeedLimit();

public:
	// @Override
	virtual bool IsUpright();

public:
	// @Override
	virtual bool IsCollide();

public:
	// @Override
	virtual float GetLosRadius();

public:
	// @Override
	virtual float GetAirLosRadius();

public:
	// @Override
	virtual float GetLosHeight();

public:
	// @Override
	virtual int GetRadarRadius();

public:
	// @Override
	virtual int GetSonarRadius();

public:
	// @Override
	virtual int GetJammerRadius();

public:
	// @Override
	virtual int GetSonarJamRadius();

public:
	// @Override
	virtual int GetSeismicRadius();

public:
	// @Override
	virtual float GetSeismicSignature();

public:
	// @Override
	virtual bool IsStealth();

public:
	// @Override
	virtual bool IsSonarStealth();

public:
	// @Override
	virtual bool IsBuildRange3D();

public:
	// @Override
	virtual float GetBuildDistance();

public:
	// @Override
	virtual float GetBuildSpeed();

public:
	// @Override
	virtual float GetReclaimSpeed();

public:
	// @Override
	virtual float GetRepairSpeed();

public:
	// @Override
	virtual float GetMaxRepairSpeed();

public:
	// @Override
	virtual float GetResurrectSpeed();

public:
	// @Override
	virtual float GetCaptureSpeed();

public:
	// @Override
	virtual float GetTerraformSpeed();

public:
	// @Override
	virtual float GetMass();

public:
	// @Override
	virtual bool IsPushResistant();

	/**
	 * Should the unit move sideways when it can not shoot?
	 */
public:
	// @Override
	virtual bool IsStrafeToAttack();

public:
	// @Override
	virtual float GetMinCollisionSpeed();

public:
	// @Override
	virtual float GetSlideTolerance();

	/**
	 * Build location relevant maximum steepness of the underlaying terrain.
	 * Used to calculate the maxHeightDif.
	 */
public:
	// @Override
	virtual float GetMaxSlope();

	/**
	 * Maximum terra-form height this building allows.
	 * If this value is 0.0, you can only build this structure on
	 * totally flat terrain.
	 */
public:
	// @Override
	virtual float GetMaxHeightDif();

public:
	// @Override
	virtual float GetMinWaterDepth();

public:
	// @Override
	virtual float GetWaterline();

public:
	// @Override
	virtual float GetMaxWaterDepth();

public:
	// @Override
	virtual float GetArmoredMultiple();

public:
	// @Override
	virtual int GetArmorType();

public:
	// @Override
	virtual float GetMaxWeaponRange();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual const char* GetType();

public:
	// @Override
	virtual const char* GetTooltip();

public:
	// @Override
	virtual const char* GetWreckName();

public:
	// @Override
	virtual springai::WeaponDef* GetDeathExplosion();

public:
	// @Override
	virtual springai::WeaponDef* GetSelfDExplosion();

	/**
	 * Returns the name of the category this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual const char* GetCategoryString();

public:
	// @Override
	virtual bool IsAbleToSelfD();

public:
	// @Override
	virtual int GetSelfDCountdown();

public:
	// @Override
	virtual bool IsAbleToSubmerge();

public:
	// @Override
	virtual bool IsAbleToFly();

public:
	// @Override
	virtual bool IsAbleToMove();

public:
	// @Override
	virtual bool IsAbleToHover();

public:
	// @Override
	virtual bool IsFloater();

public:
	// @Override
	virtual bool IsBuilder();

public:
	// @Override
	virtual bool IsActivateWhenBuilt();

public:
	// @Override
	virtual bool IsOnOffable();

public:
	// @Override
	virtual bool IsFullHealthFactory();

public:
	// @Override
	virtual bool IsFactoryHeadingTakeoff();

public:
	// @Override
	virtual bool IsReclaimable();

public:
	// @Override
	virtual bool IsCapturable();

public:
	// @Override
	virtual bool IsAbleToRestore();

public:
	// @Override
	virtual bool IsAbleToRepair();

public:
	// @Override
	virtual bool IsAbleToSelfRepair();

public:
	// @Override
	virtual bool IsAbleToReclaim();

public:
	// @Override
	virtual bool IsAbleToAttack();

public:
	// @Override
	virtual bool IsAbleToPatrol();

public:
	// @Override
	virtual bool IsAbleToFight();

public:
	// @Override
	virtual bool IsAbleToGuard();

public:
	// @Override
	virtual bool IsAbleToAssist();

public:
	// @Override
	virtual bool IsAssistable();

public:
	// @Override
	virtual bool IsAbleToRepeat();

public:
	// @Override
	virtual bool IsAbleToFireControl();

public:
	// @Override
	virtual int GetFireState();

public:
	// @Override
	virtual int GetMoveState();

public:
	// @Override
	virtual float GetWingDrag();

public:
	// @Override
	virtual float GetWingAngle();

public:
	// @Override
	virtual float GetDrag();

public:
	// @Override
	virtual float GetFrontToSpeed();

public:
	// @Override
	virtual float GetSpeedToFront();

public:
	// @Override
	virtual float GetMyGravity();

public:
	// @Override
	virtual float GetMaxBank();

public:
	// @Override
	virtual float GetMaxPitch();

public:
	// @Override
	virtual float GetTurnRadius();

public:
	// @Override
	virtual float GetWantedHeight();

public:
	// @Override
	virtual float GetVerticalSpeed();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual bool IsAbleToCrash();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual bool IsHoverAttack();

public:
	// @Override
	virtual bool IsAirStrafe();

	/**
	 * @return  < 0:  it can land
	 *          >= 0: how much the unit will move during hovering on the spot
	 */
public:
	// @Override
	virtual float GetDlHoverFactor();

public:
	// @Override
	virtual float GetMaxAcceleration();

public:
	// @Override
	virtual float GetMaxDeceleration();

public:
	// @Override
	virtual float GetMaxAileron();

public:
	// @Override
	virtual float GetMaxElevator();

public:
	// @Override
	virtual float GetMaxRudder();

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
	// @Override
	virtual std::vector<short> GetYardMap(int facing);

public:
	// @Override
	virtual int GetXSize();

public:
	// @Override
	virtual int GetZSize();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual int GetBuildAngle();

public:
	// @Override
	virtual float GetLoadingRadius();

public:
	// @Override
	virtual float GetUnloadSpread();

public:
	// @Override
	virtual int GetTransportCapacity();

public:
	// @Override
	virtual int GetTransportSize();

public:
	// @Override
	virtual int GetMinTransportSize();

public:
	// @Override
	virtual bool IsAirBase();

public:
	// @Override
	virtual bool IsFirePlatform();

public:
	// @Override
	virtual float GetTransportMass();

public:
	// @Override
	virtual float GetMinTransportMass();

public:
	// @Override
	virtual bool IsHoldSteady();

public:
	// @Override
	virtual bool IsReleaseHeld();

public:
	// @Override
	virtual bool IsNotTransportable();

public:
	// @Override
	virtual bool IsTransportByEnemy();

	/**
	 * @return  0: land unload
	 *          1: fly-over drop
	 *          2: land flood
	 */
public:
	// @Override
	virtual int GetTransportUnloadMethod();

	/**
	 * Dictates fall speed of all transported units.
	 * This only makes sense for air transports,
	 * if they an drop units while in the air.
	 */
public:
	// @Override
	virtual float GetFallSpeed();

	/**
	 * Sets the transported units FBI, overrides fallSpeed
	 */
public:
	// @Override
	virtual float GetUnitFallSpeed();

	/**
	 * If the unit can cloak
	 */
public:
	// @Override
	virtual bool IsAbleToCloak();

	/**
	 * If the unit wants to start out cloaked
	 */
public:
	// @Override
	virtual bool IsStartCloaked();

	/**
	 * Energy cost per second to stay cloaked when stationary
	 */
public:
	// @Override
	virtual float GetCloakCost();

	/**
	 * Energy cost per second to stay cloaked when moving
	 */
public:
	// @Override
	virtual float GetCloakCostMoving();

	/**
	 * If enemy unit comes within this range, decloaking is forced
	 */
public:
	// @Override
	virtual float GetDecloakDistance();

	/**
	 * Use a spherical, instead of a cylindrical test?
	 */
public:
	// @Override
	virtual bool IsDecloakSpherical();

	/**
	 * Will the unit decloak upon firing?
	 */
public:
	// @Override
	virtual bool IsDecloakOnFire();

	/**
	 * Will the unit self destruct if an enemy comes to close?
	 */
public:
	// @Override
	virtual bool IsAbleToKamikaze();

public:
	// @Override
	virtual float GetKamikazeDist();

public:
	// @Override
	virtual bool IsTargetingFacility();

public:
	// @Override
	virtual bool CanManualFire();

public:
	// @Override
	virtual bool IsNeedGeo();

public:
	// @Override
	virtual bool IsFeature();

public:
	// @Override
	virtual bool IsHideDamage();

public:
	// @Override
	virtual bool IsCommander();

public:
	// @Override
	virtual bool IsShowPlayerName();

public:
	// @Override
	virtual bool IsAbleToResurrect();

public:
	// @Override
	virtual bool IsAbleToCapture();

	/**
	 * Indicates the trajectory types supported by this unit.
	 * 
	 * @return  0: (default) = only low
	 *          1: only high
	 *          2: choose
	 */
public:
	// @Override
	virtual int GetHighTrajectoryType();

	/**
	 * Returns the bit field value denoting the categories this unit shall not
	 * chase.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual int GetNoChaseCategory();

public:
	// @Override
	virtual bool IsLeaveTracks();

public:
	// @Override
	virtual float GetTrackWidth();

public:
	// @Override
	virtual float GetTrackOffset();

public:
	// @Override
	virtual float GetTrackStrength();

public:
	// @Override
	virtual float GetTrackStretch();

public:
	// @Override
	virtual int GetTrackType();

public:
	// @Override
	virtual bool IsAbleToDropFlare();

public:
	// @Override
	virtual float GetFlareReloadTime();

public:
	// @Override
	virtual float GetFlareEfficiency();

public:
	// @Override
	virtual float GetFlareDelay();

public:
	// @Override
	virtual springai::AIFloat3 GetFlareDropVector();

public:
	// @Override
	virtual int GetFlareTime();

public:
	// @Override
	virtual int GetFlareSalvoSize();

public:
	// @Override
	virtual int GetFlareSalvoDelay();

	/**
	 * Only matters for fighter aircraft
	 */
public:
	// @Override
	virtual bool IsAbleToLoopbackAttack();

	/**
	 * Indicates whether the ground will be leveled/flattened out
	 * after this building has been built on it.
	 * Only matters for buildings.
	 */
public:
	// @Override
	virtual bool IsLevelGround();

public:
	// @Override
	virtual bool IsUseBuildingGroundDecal();

public:
	// @Override
	virtual int GetBuildingDecalType();

public:
	// @Override
	virtual int GetBuildingDecalSizeX();

public:
	// @Override
	virtual int GetBuildingDecalSizeY();

public:
	// @Override
	virtual float GetBuildingDecalDecaySpeed();

	/**
	 * Maximum flight time in seconds before the aircraft needs
	 * to return to an air repair bay to refuel.
	 */
public:
	// @Override
	virtual float GetMaxFuel();

	/**
	 * Time to fully refuel the unit
	 */
public:
	// @Override
	virtual float GetRefuelTime();

	/**
	 * Minimum build power of airbases that this aircraft can land on
	 */
public:
	// @Override
	virtual float GetMinAirBasePower();

	/**
	 * Number of units of this type allowed simultaneously in the game
	 */
public:
	// @Override
	virtual int GetMaxThisUnit();

public:
	// @Override
	virtual springai::UnitDef* GetDecoyDef();

public:
	// @Override
	virtual bool IsDontLand();

public:
	// @Override
	virtual springai::WeaponDef* GetShieldDef();

public:
	// @Override
	virtual springai::WeaponDef* GetStockpileDef();

public:
	// @Override
	virtual std::vector<springai::UnitDef*> GetBuildOptions();

public:
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();

public:
	// @Override
	virtual std::vector<springai::WeaponMount*> GetWeaponMounts();

public:
	// @Override
	virtual springai::MoveData* GetMoveData();

public:
	// @Override
	virtual springai::FlankingBonus* GetFlankingBonus();
}; // class WrappUnitDef

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPUNITDEF_H

