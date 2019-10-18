/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEF_H
#define UNITDEF_H

#include <vector>

#include "Rendering/Icon.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "System/float3.h"
#include "System/UnorderedMap.hpp"

#define MAX_UNITDEF_EXPGEN_IDS 8


struct Command;
struct WeaponDef;
struct MoveDef;
struct UnitDefImage;
class LuaTable;


struct UnitDefWeapon {
	UnitDefWeapon() = default;
	UnitDefWeapon(const WeaponDef* weaponDef);
	UnitDefWeapon(const WeaponDef* weaponDef, const LuaTable& weaponTable);
	UnitDefWeapon(const UnitDefWeapon& udw) { *this = udw; }

	// unused
	// std::string name;

	const WeaponDef* def = nullptr;

	int slavedTo = 0;

	float maxMainDirAngleDif = -1.0f;

	unsigned int badTargetCat = 0;
	unsigned int onlyTargetCat = 0;

	float3 mainDir = FwdVector;
};


struct UnitDef: public SolidObjectDef
{
public:
	UnitDef(const LuaTable& udTable, const std::string& unitName, int id);
	UnitDef();

	void SetNoCost(bool noCost);

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
	bool IsFighterAirUnit()    const { return (IsStrafingAirUnit() && HasWeapon(0) && !HasBomberWeapon(0)); }
	bool IsBomberAirUnit()     const { return (IsStrafingAirUnit() && HasWeapon(0) &&  HasBomberWeapon(0)); }

	bool DontLand() const { return (dlHoverFactor >= 0.0f); }
	bool RequireMoveDef() const { return (canmove && speed > 0.0f && !canfly); }
	bool CanChangeFireState() const { return (canFireControl && (canKamikaze || HasWeapons() || IsFactoryUnit())); }

	bool HasWeapons() const { return (HasWeapon(0)); }
	bool HasWeapon(unsigned int idx) const { return (weapons[idx].def != nullptr); }
	bool HasBomberWeapon(unsigned int idx) const;

	bool CanAttack() const { return (canAttack && (canKamikaze || HasWeapons() || IsFactoryUnit())); }
	bool CanDamage() const { return (canKamikaze || (canAttack && HasWeapons())); }

	unsigned int NumWeapons() const {
		unsigned int n = 0;

		while (n < weapons.size() && HasWeapon(n)) {
			n++;
		}

		return n;
	}

	const UnitDefWeapon& GetWeapon(unsigned int idx) const { return weapons[idx]; }
	const YardMapStatus* GetYardMapPtr() const { return (yardmap.data()); }


	void AddModelExpGenID(unsigned int egID) { modelExplGenIDs[1 + modelExplGenIDs[0]] = egID; modelExplGenIDs[0] += (egID != -1u); }
	void AddPieceExpGenID(unsigned int egID) { pieceExplGenIDs[1 + pieceExplGenIDs[0]] = egID; pieceExplGenIDs[0] += (egID != -1u); }
	void AddCrashExpGenID(unsigned int egID) { crashExplGenIDs[1 + crashExplGenIDs[0]] = egID; crashExplGenIDs[0] += (egID != -1u); }

	// UnitScript::EmitSFX can pass in any index, unlike PieceProjectile and AAirMoveType code
	unsigned int GetModelExpGenID(unsigned int idx) const { return modelExplGenIDs[1 + (idx % MAX_UNITDEF_EXPGEN_IDS)]; }
	unsigned int GetPieceExpGenID(unsigned int idx) const { return pieceExplGenIDs[1 + (idx                         )]; }
	unsigned int GetCrashExpGenID(unsigned int idx) const { return crashExplGenIDs[1 + (idx                         )]; }

	unsigned int GetModelExpGenCount() const { return modelExplGenIDs[0]; }
	unsigned int GetPieceExpGenCount() const { return pieceExplGenIDs[0]; }
	unsigned int GetCrashExpGenCount() const { return crashExplGenIDs[0]; }

public:
	int cobID;              ///< associated with the COB \<GET COB_ID unitID\> call

	const UnitDef* decoyDef;

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

	std::array<UnitDefWeapon, MAX_WEAPONS_PER_UNIT> weapons;

	///< The unrotated yardmap for buildings
	///< (only non-mobile ground units can have these)
	std::vector<YardMapStatus> yardmap;

	///< buildingMask used to disallow construction on certain map squares
	std::uint16_t buildingMask;

	std::array<char[64], MAX_UNITDEF_EXPGEN_IDS> modelCEGTags;
	std::array<char[64], MAX_UNITDEF_EXPGEN_IDS> pieceCEGTags;
	std::array<char[64], MAX_UNITDEF_EXPGEN_IDS> crashCEGTags;

	// *ExplGenIDs[0] stores the number of valid CEG's (TODO: privatize)
	// valid CEG id's are all in front s.t. they can be randomly sampled
	std::array<unsigned int, 1 + MAX_UNITDEF_EXPGEN_IDS> modelExplGenIDs;
	std::array<unsigned int, 1 + MAX_UNITDEF_EXPGEN_IDS> pieceExplGenIDs;
	std::array<unsigned int, 1 + MAX_UNITDEF_EXPGEN_IDS> crashExplGenIDs;

	spring::unordered_map<int, std::string> buildOptions;

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
	void CreateYardMap(std::string&& yardMapStr);

	float realMetalCost;
	float realEnergyCost;
	float realMetalUpkeep;
	float realEnergyUpkeep;
	float realBuildTime;
};

#endif /* UNITDEF_H */
