/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEF_H
#define UNITDEF_H

#include <vector>

#include "Rendering/Icon.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "System/float3.h"


struct Command;
struct WeaponDef;
struct MoveDef;
struct UnitDefImage;
class LuaTable;


struct UnitDefWeapon {
	UnitDefWeapon();
	UnitDefWeapon(const WeaponDef* weaponDef);
	UnitDefWeapon(const WeaponDef* weaponDef, const LuaTable& weaponTable);
	UnitDefWeapon(const UnitDefWeapon& udw) { *this = udw; }

	// unused
	std::string name;

	const WeaponDef* def;
	int slavedTo;

	float maxMainDirAngleDif;

	unsigned int badTargetCat;
	unsigned int onlyTargetCat;

	float3 mainDir;
};


struct UnitDef: public SolidObjectDef
{
public:
	UnitDef(const LuaTable& udTable, const std::string& unitName, int id);
	UnitDef();
	~UnitDef();

	bool DontLand() const { return dlHoverFactor >= 0.0f; }
	void SetNoCost(bool noCost);
	bool CheckTerrainConstraints(const MoveDef* moveDef, float rawHeight, float* clampedHeight = NULL) const;

	bool IsTransportUnit()     const { return (transportCapacity > 0 && transportMass > 0.0f); }
	bool IsImmobileUnit()      const { return (pathType == -1U && !canfly && speed <= 0.0f); }
	bool IsBuildingUnit()      const { return (IsImmobileUnit() && !yardmap.empty()); }
	bool IsBuilderUnit()       const { return (builder && buildSpeed > 0.0f && buildDistance > 0.0f); }
	bool IsMobileBuilderUnit() const { return (IsBuilderUnit() && !IsImmobileUnit()); }
	bool IsStaticBuilderUnit() const { return (IsBuilderUnit() &&  IsImmobileUnit()); }
	bool IsFactoryUnit()       const { return (IsBuilderUnit() &&  IsBuildingUnit()); }
	bool IsExtractorUnit()     const { return (extractsMetal > 0.0f && extractRange > 0.0f); }
	bool IsGroundUnit()        const { return (pathType != -1U && !canfly); }
	bool IsAirUnit()           const { return (pathType == -1U &&  canfly); }
	bool IsStrafingAirUnit()   const { return (IsAirUnit() && !(IsBuilderUnit() || IsTransportUnit() || hoverAttack)); }
	bool IsHoveringAirUnit()   const { return (IsAirUnit() &&  (IsBuilderUnit() || IsTransportUnit() || hoverAttack)); }
	bool IsFighterAirUnit()    const { return (IsStrafingAirUnit() && !weapons.empty() && !HasBomberWeapon()); }
	bool IsBomberAirUnit()     const { return (IsStrafingAirUnit() && !weapons.empty() &&  HasBomberWeapon()); }

	bool RequireMoveDef() const { return (canmove && speed > 0.0f && !canfly); }
	bool HasBomberWeapon() const;
	const std::vector<YardMapStatus>& GetYardMap() const { return yardmap; }

	void SetModelExplosionGeneratorID(unsigned int idx, unsigned int egID) { modelExplGenIDs[idx] = egID; }
	void SetPieceExplosionGeneratorID(unsigned int idx, unsigned int egID) { pieceExplGenIDs[idx] = egID; }

	unsigned int GetModelExplosionGeneratorID(unsigned int idx) const {
		if (modelExplGenIDs.empty())
			return -1u;
		return (modelExplGenIDs[idx % modelExplGenIDs.size()]);
	}
	unsigned int GetPieceExplosionGeneratorID(unsigned int idx) const {
		if (pieceExplGenIDs.empty())
			return -1u;
		return (pieceExplGenIDs[idx % pieceExplGenIDs.size()]);
	}

public:
	int cobID;              ///< associated with the COB \<GET COB_ID unitID\> call

	const UnitDef* decoyDef;

	int techLevel;

	float metalUpkeep;
	float energyUpkeep;
	float metalMake;		///< metal will always be created
	float makesMetal;		///< metal will be created when unit is on and enough energy can be drained
	float energyMake;
	float buildTime;
	float extractsMetal;
	float extractRange;
	float windGenerator;
	float tidalGenerator;
	float metalStorage;
	float energyStorage;
	float harvestMetalStorage;
	float harvestEnergyStorage;

	float autoHeal;     ///< amount autohealed
	float idleAutoHeal; ///< amount autohealed only during idling
	int idleTime;       ///< time a unit needs to idle before its considered idling

	float power;
	unsigned int category;

	float speed;        ///< maximum forward speed the unit can attain (elmos/sec)
	float rSpeed;       ///< maximum reverse speed the unit can attain (elmos/sec)
	float turnRate;
	bool turnInPlace;

	///< for units with turnInPlace=false, defines the
	///< minimum speed to slow down to while turning
	float turnInPlaceSpeedLimit;
	///< for units with turnInPlace=true, defines the
	///< maximum angle of a turn that can be entered
	///< without slowing down
	float turnInPlaceAngleLimit;

	bool collide;

	float losHeight;
	float radarHeight;

	float losRadius;
	float airLosRadius;
	int radarRadius;
	int sonarRadius;
	int jammerRadius;
	int sonarJamRadius;
	int seismicRadius;
	float seismicSignature;
	bool stealth;
	bool sonarStealth;

	bool  buildRange3D;
	float buildDistance;
	float buildSpeed;
	float reclaimSpeed;
	float repairSpeed;
	float maxRepairSpeed;
	float resurrectSpeed;
	float captureSpeed;
	float terraformSpeed;

	bool canSubmerge;
	bool canfly;
	bool floatOnWater;
	bool pushResistant;
	bool strafeToAttack;  /// should the unit move sideways when it can't shoot?
	float minCollisionSpeed;
	float slideTolerance;
	float maxHeightDif;   /// maximum terraform height this building allows
	float waterline;
	float minWaterDepth;
	float maxWaterDepth;

	unsigned int pathType;

	float armoredMultiple;
	int armorType;

	/**
	 * 0: no flanking bonus
	 * 1: global coords, mobile
	 * 2: unit coords, mobile
	 * 3: unit coords, locked
	 */
	int flankingBonusMode;
	float3 flankingBonusDir; ///< units takes less damage when attacked from this dir (encourage flanking fire)
	float  flankingBonusMax; ///< damage factor for the least protected direction
	float  flankingBonusMin; ///< damage factor for the most protected direction
	float  flankingBonusMobilityAdd; ///< how much the ability of the flanking bonus direction to move builds up each frame

	std::string humanName;
	std::string decoyName;
	std::string scriptName;     ///< the name of the unit's script, e.g. "armjeth.cob"
	std::string tooltip;
	std::string wreckName;
	std::string categoryString;
	std::string buildPicName;

	std::vector<UnitDefWeapon> weapons;

	///< The unrotated yardmap for buildings
	///< (only non-mobile ground units can have these)
	std::vector<YardMapStatus> yardmap;

	///< buildingMask used to disallow construction on certain map squares
	boost::uint16_t buildingMask;

	std::vector<std::string> modelCEGTags;
	std::vector<std::string> pieceCEGTags;

	// TODO: privatize
	std::vector<unsigned int> modelExplGenIDs;
	std::vector<unsigned int> pieceExplGenIDs;

	std::map<int, std::string> buildOptions;

	const WeaponDef* shieldWeaponDef;
	const WeaponDef* stockpileWeaponDef;
	float maxWeaponRange;
	float maxCoverage;

	const WeaponDef* deathExpWeaponDef;
	const WeaponDef* selfdExpWeaponDef;

	mutable UnitDefImage* buildPic;
	mutable icon::CIcon iconType;

	int selfDCountdown;

	bool builder;
	bool activateWhenBuilt;
	bool onoffable;
	bool fullHealthFactory;
	bool factoryHeadingTakeoff;

	bool capturable;
	bool repairable;

	// order-capabilities for CommandAI
	bool canmove;
	bool canAttack;
	bool canFight;
	bool canPatrol;
	bool canGuard;
	bool canRepeat;
	bool canResurrect;
	bool canCapture;
	bool canCloak;
	bool canSelfD;
	bool canKamikaze;

	bool canRestore;
	bool canRepair;
	bool canReclaim;
	bool canAssist;

	bool canBeAssisted;
	bool canSelfRepair;

	bool canFireControl;
	bool canManualFire;

	int fireState;
	int moveState;

	//aircraft stuff
	float wingDrag;
	float wingAngle;
	float frontToSpeed;
	float speedToFront;
	float myGravity;

	float maxBank;
	float maxPitch;
	float turnRadius;
	float wantedHeight;
	float verticalSpeed;

	bool useSmoothMesh;
	bool hoverAttack;
	bool airStrafe;
	float dlHoverFactor; ///< < 0 means it can land, >= 0 indicates how much the unit will move during hovering on the spot
	bool bankingAllowed;

	float maxAcc;
	float maxDec;
	float maxAileron;
	float maxElevator;
	float maxRudder;
	float crashDrag;

	float loadingRadius;							///< for transports
	float unloadSpread;
	int transportCapacity;
	int transportSize;
	int minTransportSize;
	bool isAirBase;
	bool isFirePlatform;							///< should the carried units still be able to shoot?
	float transportMass;
	float minTransportMass;
	bool holdSteady;
	bool releaseHeld;
	bool cantBeTransported;                         /// defaults to true for immobile units, false for all other unit-types
	bool transportByEnemy;
	int transportUnloadMethod;						///< 0 - land unload, 1 - flyover drop, 2 - land flood
	float fallSpeed;								///< dictates fall speed of all transported units
	float unitFallSpeed;							///< sets the transported units fbi, overrides fallSpeed

	bool startCloaked;								///< if the units want to start out cloaked
	float cloakCost;								///< energy cost per second to stay cloaked when stationary
	float cloakCostMoving;							///< energy cost per second when moving
	float decloakDistance;							///< if enemy unit come within this range decloaking is forced
	bool decloakSpherical;							///< use a spherical test instead of a cylindrical test?
	bool decloakOnFire;								///< should the unit decloak upon firing
	int cloakTimeout;								///< minimum time between decloak and subsequent cloak

	float kamikazeDist;
	bool kamikazeUseLOS;

	bool targfac;
	bool needGeo;
	bool isFeature;
	bool hideDamage;
	bool showPlayerName;

	int highTrajectoryType;							///< 0 (default) = only low, 1 = only high, 2 = choose

	unsigned int noChaseCategory;

	struct SoundStruct {
		GuiSoundSet select;
		GuiSoundSet ok;
		GuiSoundSet arrived;
		GuiSoundSet build;
		GuiSoundSet repair;
		GuiSoundSet working;
		GuiSoundSet underattack;
		GuiSoundSet cant;
		GuiSoundSet activate;
		GuiSoundSet deactivate;
	};
	SoundStruct sounds;

	bool canDropFlare;
	float flareReloadTime;
	float flareEfficiency;
	float flareDelay;
	float3 flareDropVector;
	int flareTime;
	int flareSalvoSize;
	int flareSalvoDelay;

	bool canLoopbackAttack;                         ///< only matters for fighter aircraft
	bool levelGround;                               ///< only matters for buildings

	bool showNanoFrame;								///< Does the nano frame animation get shown during construction?
	bool showNanoSpray;								///< Does nano spray get shown at all?
	float3 nanoColor;								///< If nano spray is displayed what color is it?

	int maxThisUnit;                                ///< number of units of this type allowed simultaneously in the game

private:
	void ParseWeaponsTable(const LuaTable& weaponsTable);
	void CreateYardMap(std::string yardMapStr);

	float realMetalCost;
	float realEnergyCost;
	float realMetalUpkeep;
	float realEnergyUpkeep;
	float realBuildTime;
};

#endif /* UNITDEF_H */
