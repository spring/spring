/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBUNITDEF_H
#define _CPPWRAPPER_STUBUNITDEF_H

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
class StubUnitDef : public UnitDef {

protected:
	virtual ~StubUnitDef();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitDefId(int unitDefId);
	// @Override
	virtual int GetUnitDefId() const;
	/**
	 * Forces loading of the unit model
	 */
private:
	float height;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHeight(float height);
	// @Override
	virtual float GetHeight();
	/**
	 * Forces loading of the unit model
	 */
private:
	float radius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRadius(float radius);
	// @Override
	virtual float GetRadius();
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	const char* humanName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHumanName(const char* humanName);
	// @Override
	virtual const char* GetHumanName();
private:
	const char* fileName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFileName(const char* fileName);
	// @Override
	virtual const char* GetFileName();
	/**
	 * @deprecated
	 */
private:
	int aiHint;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAiHint(int aiHint);
	// @Override
	virtual int GetAiHint();
private:
	int cobId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCobId(int cobId);
	// @Override
	virtual int GetCobId();
private:
	int techLevel;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTechLevel(int techLevel);
	// @Override
	virtual int GetTechLevel();
	/**
	 * @deprecated
	 */
private:
	const char* gaia;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGaia(const char* gaia);
	// @Override
	virtual const char* GetGaia();
	// @Override
	virtual float GetUpkeep(Resource* resource);
	/**
	 * This amount of the resource will always be created.
	 */
	// @Override
	virtual float GetResourceMake(Resource* resource);
	/**
	 * This amount of the resource will be created when the unit is on and enough
	 * energy can be drained.
	 */
	// @Override
	virtual float GetMakesResource(Resource* resource);
	// @Override
	virtual float GetCost(Resource* resource);
	// @Override
	virtual float GetExtractsResource(Resource* resource);
	// @Override
	virtual float GetResourceExtractorRange(Resource* resource);
	// @Override
	virtual float GetWindResourceGenerator(Resource* resource);
	// @Override
	virtual float GetTidalResourceGenerator(Resource* resource);
	// @Override
	virtual float GetStorage(Resource* resource);
	// @Override
	virtual bool IsSquareResourceExtractor(Resource* resource);
private:
	float buildTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildTime(float buildTime);
	// @Override
	virtual float GetBuildTime();
	/**
	 * This amount of auto-heal will always be applied.
	 */
private:
	float autoHeal;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAutoHeal(float autoHeal);
	// @Override
	virtual float GetAutoHeal();
	/**
	 * This amount of auto-heal will only be applied while the unit is idling.
	 */
private:
	float idleAutoHeal;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetIdleAutoHeal(float idleAutoHeal);
	// @Override
	virtual float GetIdleAutoHeal();
	/**
	 * Time a unit needs to idle before it is considered idling.
	 */
private:
	int idleTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetIdleTime(int idleTime);
	// @Override
	virtual int GetIdleTime();
private:
	float power;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPower(float power);
	// @Override
	virtual float GetPower();
private:
	float health;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHealth(float health);
	// @Override
	virtual float GetHealth();
	/**
	 * Returns the bit field value denoting the categories this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	int category;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCategory(int category);
	// @Override
	virtual int GetCategory();
private:
	float speed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSpeed(float speed);
	// @Override
	virtual float GetSpeed();
private:
	float turnRate;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnRate(float turnRate);
	// @Override
	virtual float GetTurnRate();
private:
	bool isTurnInPlace;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnInPlace(bool isTurnInPlace);
	// @Override
	virtual bool IsTurnInPlace();
	/**
	 * Units above this distance to goal will try to turn while keeping
	 * some of their speed.
	 * 0 to disable
	 */
private:
	float turnInPlaceDistance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnInPlaceDistance(float turnInPlaceDistance);
	// @Override
	virtual float GetTurnInPlaceDistance();
	/**
	 * Units below this speed will turn in place regardless of their
	 * turnInPlace setting.
	 */
private:
	float turnInPlaceSpeedLimit;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnInPlaceSpeedLimit(float turnInPlaceSpeedLimit);
	// @Override
	virtual float GetTurnInPlaceSpeedLimit();
private:
	bool isUpright;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUpright(bool isUpright);
	// @Override
	virtual bool IsUpright();
private:
	bool isCollide;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCollide(bool isCollide);
	// @Override
	virtual bool IsCollide();
private:
	float losRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLosRadius(float losRadius);
	// @Override
	virtual float GetLosRadius();
private:
	float airLosRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAirLosRadius(float airLosRadius);
	// @Override
	virtual float GetAirLosRadius();
private:
	float losHeight;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLosHeight(float losHeight);
	// @Override
	virtual float GetLosHeight();
private:
	int radarRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRadarRadius(int radarRadius);
	// @Override
	virtual int GetRadarRadius();
private:
	int sonarRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSonarRadius(int sonarRadius);
	// @Override
	virtual int GetSonarRadius();
private:
	int jammerRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetJammerRadius(int jammerRadius);
	// @Override
	virtual int GetJammerRadius();
private:
	int sonarJamRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSonarJamRadius(int sonarJamRadius);
	// @Override
	virtual int GetSonarJamRadius();
private:
	int seismicRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSeismicRadius(int seismicRadius);
	// @Override
	virtual int GetSeismicRadius();
private:
	float seismicSignature;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSeismicSignature(float seismicSignature);
	// @Override
	virtual float GetSeismicSignature();
private:
	bool isStealth;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStealth(bool isStealth);
	// @Override
	virtual bool IsStealth();
private:
	bool isSonarStealth;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSonarStealth(bool isSonarStealth);
	// @Override
	virtual bool IsSonarStealth();
private:
	bool isBuildRange3D;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildRange3D(bool isBuildRange3D);
	// @Override
	virtual bool IsBuildRange3D();
private:
	float buildDistance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildDistance(float buildDistance);
	// @Override
	virtual float GetBuildDistance();
private:
	float buildSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildSpeed(float buildSpeed);
	// @Override
	virtual float GetBuildSpeed();
private:
	float reclaimSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimSpeed(float reclaimSpeed);
	// @Override
	virtual float GetReclaimSpeed();
private:
	float repairSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRepairSpeed(float repairSpeed);
	// @Override
	virtual float GetRepairSpeed();
private:
	float maxRepairSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxRepairSpeed(float maxRepairSpeed);
	// @Override
	virtual float GetMaxRepairSpeed();
private:
	float resurrectSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetResurrectSpeed(float resurrectSpeed);
	// @Override
	virtual float GetResurrectSpeed();
private:
	float captureSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCaptureSpeed(float captureSpeed);
	// @Override
	virtual float GetCaptureSpeed();
private:
	float terraformSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTerraformSpeed(float terraformSpeed);
	// @Override
	virtual float GetTerraformSpeed();
private:
	float mass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMass(float mass);
	// @Override
	virtual float GetMass();
private:
	bool isPushResistant;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPushResistant(bool isPushResistant);
	// @Override
	virtual bool IsPushResistant();
	/**
	 * Should the unit move sideways when it can not shoot?
	 */
private:
	bool isStrafeToAttack;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStrafeToAttack(bool isStrafeToAttack);
	// @Override
	virtual bool IsStrafeToAttack();
private:
	float minCollisionSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinCollisionSpeed(float minCollisionSpeed);
	// @Override
	virtual float GetMinCollisionSpeed();
private:
	float slideTolerance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSlideTolerance(float slideTolerance);
	// @Override
	virtual float GetSlideTolerance();
	/**
	 * Build location relevant maximum steepness of the underlaying terrain.
	 * Used to calculate the maxHeightDif.
	 */
private:
	float maxSlope;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxSlope(float maxSlope);
	// @Override
	virtual float GetMaxSlope();
	/**
	 * Maximum terra-form height this building allows.
	 * If this value is 0.0, you can only build this structure on
	 * totally flat terrain.
	 */
private:
	float maxHeightDif;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxHeightDif(float maxHeightDif);
	// @Override
	virtual float GetMaxHeightDif();
private:
	float minWaterDepth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinWaterDepth(float minWaterDepth);
	// @Override
	virtual float GetMinWaterDepth();
private:
	float waterline;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWaterline(float waterline);
	// @Override
	virtual float GetWaterline();
private:
	float maxWaterDepth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxWaterDepth(float maxWaterDepth);
	// @Override
	virtual float GetMaxWaterDepth();
private:
	float armoredMultiple;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetArmoredMultiple(float armoredMultiple);
	// @Override
	virtual float GetArmoredMultiple();
private:
	int armorType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetArmorType(int armorType);
	// @Override
	virtual int GetArmorType();
private:
	float maxWeaponRange;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxWeaponRange(float maxWeaponRange);
	// @Override
	virtual float GetMaxWeaponRange();
	/**
	 * @deprecated
	 */
private:
	const char* type;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetType(const char* type);
	// @Override
	virtual const char* GetType();
private:
	const char* tooltip;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTooltip(const char* tooltip);
	// @Override
	virtual const char* GetTooltip();
private:
	const char* wreckName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWreckName(const char* wreckName);
	// @Override
	virtual const char* GetWreckName();
private:
	springai::WeaponDef* deathExplosion;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDeathExplosion(springai::WeaponDef* deathExplosion);
	// @Override
	virtual springai::WeaponDef* GetDeathExplosion();
private:
	springai::WeaponDef* selfDExplosion;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSelfDExplosion(springai::WeaponDef* selfDExplosion);
	// @Override
	virtual springai::WeaponDef* GetSelfDExplosion();
	/**
	 * Returns the name of the category this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	const char* categoryString;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCategoryString(const char* categoryString);
	// @Override
	virtual const char* GetCategoryString();
private:
	bool isAbleToSelfD;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToSelfD(bool isAbleToSelfD);
	// @Override
	virtual bool IsAbleToSelfD();
private:
	int selfDCountdown;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSelfDCountdown(int selfDCountdown);
	// @Override
	virtual int GetSelfDCountdown();
private:
	bool isAbleToSubmerge;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToSubmerge(bool isAbleToSubmerge);
	// @Override
	virtual bool IsAbleToSubmerge();
private:
	bool isAbleToFly;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToFly(bool isAbleToFly);
	// @Override
	virtual bool IsAbleToFly();
private:
	bool isAbleToMove;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToMove(bool isAbleToMove);
	// @Override
	virtual bool IsAbleToMove();
private:
	bool isAbleToHover;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToHover(bool isAbleToHover);
	// @Override
	virtual bool IsAbleToHover();
private:
	bool isFloater;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFloater(bool isFloater);
	// @Override
	virtual bool IsFloater();
private:
	bool isBuilder;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuilder(bool isBuilder);
	// @Override
	virtual bool IsBuilder();
private:
	bool isActivateWhenBuilt;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetActivateWhenBuilt(bool isActivateWhenBuilt);
	// @Override
	virtual bool IsActivateWhenBuilt();
private:
	bool isOnOffable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOnOffable(bool isOnOffable);
	// @Override
	virtual bool IsOnOffable();
private:
	bool isFullHealthFactory;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFullHealthFactory(bool isFullHealthFactory);
	// @Override
	virtual bool IsFullHealthFactory();
private:
	bool isFactoryHeadingTakeoff;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFactoryHeadingTakeoff(bool isFactoryHeadingTakeoff);
	// @Override
	virtual bool IsFactoryHeadingTakeoff();
private:
	bool isReclaimable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimable(bool isReclaimable);
	// @Override
	virtual bool IsReclaimable();
private:
	bool isCapturable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCapturable(bool isCapturable);
	// @Override
	virtual bool IsCapturable();
private:
	bool isAbleToRestore;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToRestore(bool isAbleToRestore);
	// @Override
	virtual bool IsAbleToRestore();
private:
	bool isAbleToRepair;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToRepair(bool isAbleToRepair);
	// @Override
	virtual bool IsAbleToRepair();
private:
	bool isAbleToSelfRepair;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToSelfRepair(bool isAbleToSelfRepair);
	// @Override
	virtual bool IsAbleToSelfRepair();
private:
	bool isAbleToReclaim;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToReclaim(bool isAbleToReclaim);
	// @Override
	virtual bool IsAbleToReclaim();
private:
	bool isAbleToAttack;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToAttack(bool isAbleToAttack);
	// @Override
	virtual bool IsAbleToAttack();
private:
	bool isAbleToPatrol;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToPatrol(bool isAbleToPatrol);
	// @Override
	virtual bool IsAbleToPatrol();
private:
	bool isAbleToFight;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToFight(bool isAbleToFight);
	// @Override
	virtual bool IsAbleToFight();
private:
	bool isAbleToGuard;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToGuard(bool isAbleToGuard);
	// @Override
	virtual bool IsAbleToGuard();
private:
	bool isAbleToAssist;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToAssist(bool isAbleToAssist);
	// @Override
	virtual bool IsAbleToAssist();
private:
	bool isAssistable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAssistable(bool isAssistable);
	// @Override
	virtual bool IsAssistable();
private:
	bool isAbleToRepeat;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToRepeat(bool isAbleToRepeat);
	// @Override
	virtual bool IsAbleToRepeat();
private:
	bool isAbleToFireControl;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToFireControl(bool isAbleToFireControl);
	// @Override
	virtual bool IsAbleToFireControl();
private:
	int fireState;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFireState(int fireState);
	// @Override
	virtual int GetFireState();
private:
	int moveState;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMoveState(int moveState);
	// @Override
	virtual int GetMoveState();
private:
	float wingDrag;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWingDrag(float wingDrag);
	// @Override
	virtual float GetWingDrag();
private:
	float wingAngle;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWingAngle(float wingAngle);
	// @Override
	virtual float GetWingAngle();
private:
	float drag;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDrag(float drag);
	// @Override
	virtual float GetDrag();
private:
	float frontToSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFrontToSpeed(float frontToSpeed);
	// @Override
	virtual float GetFrontToSpeed();
private:
	float speedToFront;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSpeedToFront(float speedToFront);
	// @Override
	virtual float GetSpeedToFront();
private:
	float myGravity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMyGravity(float myGravity);
	// @Override
	virtual float GetMyGravity();
private:
	float maxBank;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxBank(float maxBank);
	// @Override
	virtual float GetMaxBank();
private:
	float maxPitch;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxPitch(float maxPitch);
	// @Override
	virtual float GetMaxPitch();
private:
	float turnRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnRadius(float turnRadius);
	// @Override
	virtual float GetTurnRadius();
private:
	float wantedHeight;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWantedHeight(float wantedHeight);
	// @Override
	virtual float GetWantedHeight();
private:
	float verticalSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVerticalSpeed(float verticalSpeed);
	// @Override
	virtual float GetVerticalSpeed();
	/**
	 * @deprecated
	 */
private:
	bool isAbleToCrash;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToCrash(bool isAbleToCrash);
	// @Override
	virtual bool IsAbleToCrash();
	/**
	 * @deprecated
	 */
private:
	bool isHoverAttack;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHoverAttack(bool isHoverAttack);
	// @Override
	virtual bool IsHoverAttack();
private:
	bool isAirStrafe;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAirStrafe(bool isAirStrafe);
	// @Override
	virtual bool IsAirStrafe();
	/**
	 * @return  < 0:  it can land
	 *          >= 0: how much the unit will move during hovering on the spot
	 */
private:
	float dlHoverFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDlHoverFactor(float dlHoverFactor);
	// @Override
	virtual float GetDlHoverFactor();
private:
	float maxAcceleration;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxAcceleration(float maxAcceleration);
	// @Override
	virtual float GetMaxAcceleration();
private:
	float maxDeceleration;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxDeceleration(float maxDeceleration);
	// @Override
	virtual float GetMaxDeceleration();
private:
	float maxAileron;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxAileron(float maxAileron);
	// @Override
	virtual float GetMaxAileron();
private:
	float maxElevator;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxElevator(float maxElevator);
	// @Override
	virtual float GetMaxElevator();
private:
	float maxRudder;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxRudder(float maxRudder);
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
	// @Override
	virtual std::vector<short> GetYardMap(int facing);
private:
	int xSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetXSize(int xSize);
	// @Override
	virtual int GetXSize();
private:
	int zSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetZSize(int zSize);
	// @Override
	virtual int GetZSize();
	/**
	 * @deprecated
	 */
private:
	int buildAngle;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildAngle(int buildAngle);
	// @Override
	virtual int GetBuildAngle();
private:
	float loadingRadius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLoadingRadius(float loadingRadius);
	// @Override
	virtual float GetLoadingRadius();
private:
	float unloadSpread;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnloadSpread(float unloadSpread);
	// @Override
	virtual float GetUnloadSpread();
private:
	int transportCapacity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportCapacity(int transportCapacity);
	// @Override
	virtual int GetTransportCapacity();
private:
	int transportSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportSize(int transportSize);
	// @Override
	virtual int GetTransportSize();
private:
	int minTransportSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinTransportSize(int minTransportSize);
	// @Override
	virtual int GetMinTransportSize();
private:
	bool isAirBase;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAirBase(bool isAirBase);
	// @Override
	virtual bool IsAirBase();
private:
	bool isFirePlatform;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFirePlatform(bool isFirePlatform);
	// @Override
	virtual bool IsFirePlatform();
private:
	float transportMass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportMass(float transportMass);
	// @Override
	virtual float GetTransportMass();
private:
	float minTransportMass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinTransportMass(float minTransportMass);
	// @Override
	virtual float GetMinTransportMass();
private:
	bool isHoldSteady;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHoldSteady(bool isHoldSteady);
	// @Override
	virtual bool IsHoldSteady();
private:
	bool isReleaseHeld;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReleaseHeld(bool isReleaseHeld);
	// @Override
	virtual bool IsReleaseHeld();
private:
	bool isNotTransportable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNotTransportable(bool isNotTransportable);
	// @Override
	virtual bool IsNotTransportable();
private:
	bool isTransportByEnemy;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportByEnemy(bool isTransportByEnemy);
	// @Override
	virtual bool IsTransportByEnemy();
	/**
	 * @return  0: land unload
	 *          1: fly-over drop
	 *          2: land flood
	 */
private:
	int transportUnloadMethod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportUnloadMethod(int transportUnloadMethod);
	// @Override
	virtual int GetTransportUnloadMethod();
	/**
	 * Dictates fall speed of all transported units.
	 * This only makes sense for air transports,
	 * if they an drop units while in the air.
	 */
private:
	float fallSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFallSpeed(float fallSpeed);
	// @Override
	virtual float GetFallSpeed();
	/**
	 * Sets the transported units FBI, overrides fallSpeed
	 */
private:
	float unitFallSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitFallSpeed(float unitFallSpeed);
	// @Override
	virtual float GetUnitFallSpeed();
	/**
	 * If the unit can cloak
	 */
private:
	bool isAbleToCloak;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToCloak(bool isAbleToCloak);
	// @Override
	virtual bool IsAbleToCloak();
	/**
	 * If the unit wants to start out cloaked
	 */
private:
	bool isStartCloaked;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStartCloaked(bool isStartCloaked);
	// @Override
	virtual bool IsStartCloaked();
	/**
	 * Energy cost per second to stay cloaked when stationary
	 */
private:
	float cloakCost;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCloakCost(float cloakCost);
	// @Override
	virtual float GetCloakCost();
	/**
	 * Energy cost per second to stay cloaked when moving
	 */
private:
	float cloakCostMoving;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCloakCostMoving(float cloakCostMoving);
	// @Override
	virtual float GetCloakCostMoving();
	/**
	 * If enemy unit comes within this range, decloaking is forced
	 */
private:
	float decloakDistance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDecloakDistance(float decloakDistance);
	// @Override
	virtual float GetDecloakDistance();
	/**
	 * Use a spherical, instead of a cylindrical test?
	 */
private:
	bool isDecloakSpherical;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDecloakSpherical(bool isDecloakSpherical);
	// @Override
	virtual bool IsDecloakSpherical();
	/**
	 * Will the unit decloak upon firing?
	 */
private:
	bool isDecloakOnFire;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDecloakOnFire(bool isDecloakOnFire);
	// @Override
	virtual bool IsDecloakOnFire();
	/**
	 * Will the unit self destruct if an enemy comes to close?
	 */
private:
	bool isAbleToKamikaze;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToKamikaze(bool isAbleToKamikaze);
	// @Override
	virtual bool IsAbleToKamikaze();
private:
	float kamikazeDist;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetKamikazeDist(float kamikazeDist);
	// @Override
	virtual float GetKamikazeDist();
private:
	bool isTargetingFacility;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTargetingFacility(bool isTargetingFacility);
	// @Override
	virtual bool IsTargetingFacility();
	// @Override
	virtual bool CanManualFire();
private:
	bool isNeedGeo;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNeedGeo(bool isNeedGeo);
	// @Override
	virtual bool IsNeedGeo();
private:
	bool isFeature;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFeature(bool isFeature);
	// @Override
	virtual bool IsFeature();
private:
	bool isHideDamage;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHideDamage(bool isHideDamage);
	// @Override
	virtual bool IsHideDamage();
private:
	bool isCommander;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCommander(bool isCommander);
	// @Override
	virtual bool IsCommander();
private:
	bool isShowPlayerName;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShowPlayerName(bool isShowPlayerName);
	// @Override
	virtual bool IsShowPlayerName();
private:
	bool isAbleToResurrect;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToResurrect(bool isAbleToResurrect);
	// @Override
	virtual bool IsAbleToResurrect();
private:
	bool isAbleToCapture;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToCapture(bool isAbleToCapture);
	// @Override
	virtual bool IsAbleToCapture();
	/**
	 * Indicates the trajectory types supported by this unit.
	 * 
	 * @return  0: (default) = only low
	 *          1: only high
	 *          2: choose
	 */
private:
	int highTrajectoryType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHighTrajectoryType(int highTrajectoryType);
	// @Override
	virtual int GetHighTrajectoryType();
	/**
	 * Returns the bit field value denoting the categories this unit shall not
	 * chase.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	int noChaseCategory;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNoChaseCategory(int noChaseCategory);
	// @Override
	virtual int GetNoChaseCategory();
private:
	bool isLeaveTracks;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLeaveTracks(bool isLeaveTracks);
	// @Override
	virtual bool IsLeaveTracks();
private:
	float trackWidth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrackWidth(float trackWidth);
	// @Override
	virtual float GetTrackWidth();
private:
	float trackOffset;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrackOffset(float trackOffset);
	// @Override
	virtual float GetTrackOffset();
private:
	float trackStrength;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrackStrength(float trackStrength);
	// @Override
	virtual float GetTrackStrength();
private:
	float trackStretch;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrackStretch(float trackStretch);
	// @Override
	virtual float GetTrackStretch();
private:
	int trackType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrackType(int trackType);
	// @Override
	virtual int GetTrackType();
private:
	bool isAbleToDropFlare;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToDropFlare(bool isAbleToDropFlare);
	// @Override
	virtual bool IsAbleToDropFlare();
private:
	float flareReloadTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareReloadTime(float flareReloadTime);
	// @Override
	virtual float GetFlareReloadTime();
private:
	float flareEfficiency;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareEfficiency(float flareEfficiency);
	// @Override
	virtual float GetFlareEfficiency();
private:
	float flareDelay;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareDelay(float flareDelay);
	// @Override
	virtual float GetFlareDelay();
private:
	springai::AIFloat3 flareDropVector;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareDropVector(springai::AIFloat3 flareDropVector);
	// @Override
	virtual springai::AIFloat3 GetFlareDropVector();
private:
	int flareTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareTime(int flareTime);
	// @Override
	virtual int GetFlareTime();
private:
	int flareSalvoSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareSalvoSize(int flareSalvoSize);
	// @Override
	virtual int GetFlareSalvoSize();
private:
	int flareSalvoDelay;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlareSalvoDelay(int flareSalvoDelay);
	// @Override
	virtual int GetFlareSalvoDelay();
	/**
	 * Only matters for fighter aircraft
	 */
private:
	bool isAbleToLoopbackAttack;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToLoopbackAttack(bool isAbleToLoopbackAttack);
	// @Override
	virtual bool IsAbleToLoopbackAttack();
	/**
	 * Indicates whether the ground will be leveled/flattened out
	 * after this building has been built on it.
	 * Only matters for buildings.
	 */
private:
	bool isLevelGround;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLevelGround(bool isLevelGround);
	// @Override
	virtual bool IsLevelGround();
private:
	bool isUseBuildingGroundDecal;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUseBuildingGroundDecal(bool isUseBuildingGroundDecal);
	// @Override
	virtual bool IsUseBuildingGroundDecal();
private:
	int buildingDecalType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildingDecalType(int buildingDecalType);
	// @Override
	virtual int GetBuildingDecalType();
private:
	int buildingDecalSizeX;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildingDecalSizeX(int buildingDecalSizeX);
	// @Override
	virtual int GetBuildingDecalSizeX();
private:
	int buildingDecalSizeY;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildingDecalSizeY(int buildingDecalSizeY);
	// @Override
	virtual int GetBuildingDecalSizeY();
private:
	float buildingDecalDecaySpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildingDecalDecaySpeed(float buildingDecalDecaySpeed);
	// @Override
	virtual float GetBuildingDecalDecaySpeed();
	/**
	 * Maximum flight time in seconds before the aircraft needs
	 * to return to an air repair bay to refuel.
	 */
private:
	float maxFuel;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxFuel(float maxFuel);
	// @Override
	virtual float GetMaxFuel();
	/**
	 * Time to fully refuel the unit
	 */
private:
	float refuelTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRefuelTime(float refuelTime);
	// @Override
	virtual float GetRefuelTime();
	/**
	 * Minimum build power of airbases that this aircraft can land on
	 */
private:
	float minAirBasePower;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinAirBasePower(float minAirBasePower);
	// @Override
	virtual float GetMinAirBasePower();
	/**
	 * Number of units of this type allowed simultaneously in the game
	 */
private:
	int maxThisUnit;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxThisUnit(int maxThisUnit);
	// @Override
	virtual int GetMaxThisUnit();
private:
	springai::UnitDef* decoyDef;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDecoyDef(springai::UnitDef* decoyDef);
	// @Override
	virtual springai::UnitDef* GetDecoyDef();
private:
	bool isDontLand;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDontLand(bool isDontLand);
	// @Override
	virtual bool IsDontLand();
private:
	springai::WeaponDef* shieldDef;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShieldDef(springai::WeaponDef* shieldDef);
	// @Override
	virtual springai::WeaponDef* GetShieldDef();
private:
	springai::WeaponDef* stockpileDef;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStockpileDef(springai::WeaponDef* stockpileDef);
	// @Override
	virtual springai::WeaponDef* GetStockpileDef();
private:
	std::vector<springai::UnitDef*> buildOptions;/* = std::vector<springai::UnitDef*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildOptions(std::vector<springai::UnitDef*> buildOptions);
	// @Override
	virtual std::vector<springai::UnitDef*> GetBuildOptions();
private:
	std::map<std::string,std::string> customParams;/* = std::map<std::string,std::string>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCustomParams(std::map<std::string,std::string> customParams);
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();
private:
	std::vector<springai::WeaponMount*> weaponMounts;/* = std::vector<springai::WeaponMount*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponMounts(std::vector<springai::WeaponMount*> weaponMounts);
	// @Override
	virtual std::vector<springai::WeaponMount*> GetWeaponMounts();
private:
	springai::MoveData* moveData;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMoveData(springai::MoveData* moveData);
	// @Override
	virtual springai::MoveData* GetMoveData();
private:
	springai::FlankingBonus* flankingBonus;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlankingBonus(springai::FlankingBonus* flankingBonus);
	// @Override
	virtual springai::FlankingBonus* GetFlankingBonus();
}; // class StubUnitDef

}  // namespace springai

#endif // _CPPWRAPPER_STUBUNITDEF_H

