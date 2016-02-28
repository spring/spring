/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DEF_H
#define UNIT_DEF_H

#include <string>
#include <vector>
#include <map>

#include "System/float3.h"


namespace springLegacyAI {

struct MoveData;
struct WeaponDef;

struct UnitModelDef
{
	UnitModelDef() {}

	std::string modelPath;
	std::string modelName;
	std::map<std::string, std::string> modelTextures;
};


struct UnitDef
{
public:
	UnitDef();
	~UnitDef();

	bool DontLand() const { return dlHoverFactor >= 0.0f; }

	bool valid;
	std::string name;
	std::string humanName;
	std::string filename;
	int id;					// unique id for this type of unit

	std::string decoyName;
	const UnitDef* decoyDef;

	int aihint;
	int cobID;				// associated with the COB <GET COB_ID unitID> call

	int techLevel;
	std::string gaia;

	float metalUpkeep;
	float energyUpkeep;
	float metalMake;		// metal will always be created
	float makesMetal;		// metal will be created when unit is on and enough energy can be drained
	float energyMake;
	float metalCost;
	float energyCost;
	float buildTime;
	float extractsMetal;
	float extractRange;
	float windGenerator;
	float tidalGenerator;
	float metalStorage;
	float energyStorage;

	bool extractSquare;

	float autoHeal;     // amount autohealed
	float idleAutoHeal; // amount autohealed only during idling
	int idleTime;       // time a unit needs to idle before its considered idling

	float power;
	float health;
	unsigned int category;

	float speed;        // maximum forward speed the unit can attain (elmos/sec)
	float rSpeed;       // maximum reverse speed the unit can attain (elmos/sec)
	float turnRate;
	bool turnInPlace;
	float turnInPlaceDistance; //!< units above this distance to goal will try to turn while keeping
				   //!< some of their speed. 0 to disable
	float turnInPlaceSpeedLimit; //!< units below this speed will turn in place regardless of their
				   //!< turnInPlace setting

	bool upright;
	bool collide;

	float losRadius;
	float airLosRadius;
	float losHeight;

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

	float mass;

	bool pushResistant;
	bool strafeToAttack;  /// should the unit move sideways when it can't shoot?
	float minCollisionSpeed;
	float slideTolerance;
	float maxSlope;
	float maxHeightDif;   /// maximum terraform height this building allows
	float minWaterDepth;
	float waterline;

	float maxWaterDepth;

	float armoredMultiple;
	int armorType;

	int flankingBonusMode; // 0: no flanking bonus
	                       // 1: global coords, mobile
	                       // 2: unit coords, mobile
	                       // 3: unit coords, locked
	float3 flankingBonusDir; // units takes less damage when attacked from this dir (encourage flanking fire)
	float  flankingBonusMax; // damage factor for the least protected direction
	float  flankingBonusMin; // damage factor for the most protected direction
	float  flankingBonusMobilityAdd; // how much the ability of the flanking bonus direction to move builds up each frame

	UnitModelDef modelDef;

	std::string objectName;     // raw name of the unit's model without objects3d prefix, eg. "armjeth.s3o"
	std::string scriptName;     // the name of the unit's script, e.g. "armjeth.cob"
	std::string scriptPath;     // the path of the unit's script, e.g. "scripts/armjeth.cob"

	float3 modelCenterOffset;	// offset from the unit model's default center point (unit-space)

	bool usePieceCollisionVolumes;		// if true, collisions are checked per-piece


	struct UnitDefWeapon {
		UnitDefWeapon();
		UnitDefWeapon(std::string name, const WeaponDef* def, int slavedTo,
		              float3 mainDir, float maxAngleDif, unsigned int badTargetCat,
		              unsigned int onlyTargetCat, float fuelUse);
		std::string name;
		const WeaponDef* def;
		int slavedTo;
		float3 mainDir;
		float maxAngleDif;
		float fuelUsage; /// How many seconds of fuel it costs for the owning unit to fire this weapon
		unsigned int badTargetCat;
		unsigned int onlyTargetCat;
	};
	std::vector<UnitDefWeapon> weapons;
	const WeaponDef* shieldWeaponDef;
	const WeaponDef* stockpileWeaponDef;
	float maxWeaponRange;
	float maxCoverage;

	std::map<int, std::string> buildOptions;

	std::string type;
	std::string tooltip;
	std::string wreckName;

	std::string deathExplosion;
	std::string selfDExplosion;

	std::string categoryString;

	std::string buildPicName;

	bool canSelfD;
	int selfDCountdown;

	bool canSubmerge;
	bool canfly;
	bool canmove;
	bool canhover;
	bool floater;
	bool builder;
	bool activateWhenBuilt;
	bool onoffable;
	bool fullHealthFactory;
	bool factoryHeadingTakeoff;

	bool reclaimable;
	bool capturable;
	bool repairable;

	bool canRestore;
	bool canRepair;
	bool canSelfRepair;
	bool canReclaim;
	bool canAttack;
	bool canPatrol;
	bool canFight;
	bool canGuard;
	bool canAssist;
	bool canBeAssisted;
	bool canRepeat;
	bool canFireControl;

	int fireState;
	int moveState;

	//aircraft stuff
	float wingDrag;
	float wingAngle;
	float drag;
	float frontToSpeed;
	float speedToFront;
	float myGravity;

	float maxBank;
	float maxPitch;
	float turnRadius;
	float wantedHeight;
	float verticalSpeed;
	bool useSmoothMesh;
	bool canCrash;
	bool hoverAttack;
	bool airStrafe;
	float dlHoverFactor; // < 0 means it can land, >= 0 indicates how much the unit will move during hovering on the spot
	bool bankingAllowed;

	float maxAcc;
	float maxDec;
	float maxAileron;
	float maxElevator;
	float maxRudder;
	float crashDrag;

	MoveData* movedata;
	std::vector<unsigned char> yardmaps[4];         // Iterations of the Ymap for building rotation

	int xsize;										// each size is 8 units
	int zsize;										// each size is 8 units

	int buildangle;

	float loadingRadius;							// for transports
	float unloadSpread;
	int transportCapacity;
	int transportSize;
	int minTransportSize;
	bool isAirBase;
	bool isFirePlatform;							// should the carried units still be able to shoot?
	float transportMass;
	float minTransportMass;
	bool holdSteady;
	bool releaseHeld;
	bool cantBeTransported;
	bool transportByEnemy;
	int transportUnloadMethod;						// 0 - land unload, 1 - flyover drop, 2 - land flood
	float fallSpeed;								// dictates fall speed of all transported units
	float unitFallSpeed;							// sets the transported units fbi, overrides fallSpeed

	bool canCloak;									// if the unit can cloak
	bool startCloaked;								// if the units want to start out cloaked
	float cloakCost;								// energy cost per second to stay cloaked when stationary
	float cloakCostMoving;							// energy cost per second when moving
	float decloakDistance;							// if enemy unit come within this range decloaking is forced
	bool decloakSpherical;							// use a spherical test instead of a cylindrical test?
	bool decloakOnFire;								// should the unit decloak upon firing
	int cloakTimeout;								// minimum time between decloak and subsequent cloak

	bool canKamikaze;								//self destruct if enemy come to close
	float kamikazeDist;
	bool kamikazeUseLOS;

	bool targfac;
	bool canDGun;
	bool needGeo;
	bool isFeature;
	bool hideDamage;
	bool isCommander;
	bool showPlayerName;

	bool canResurrect;
	bool canCapture;
	int highTrajectoryType; // 0 (default) = only low, 1 = only high, 2 = choose

	unsigned int noChaseCategory;

	bool leaveTracks;
	std::string trackTypeName;
	float trackWidth;
	float trackOffset;
	float trackStrength;
	float trackStretch;
	int trackType;

	bool canDropFlare;
	float flareReloadTime;
	float flareEfficiency;
	float flareDelay;
	float3 flareDropVector;
	int flareTime;
	int flareSalvoSize;
	int flareSalvoDelay;

	bool smoothAnim;         // True if the unit should use interpolated animation
	bool canLoopbackAttack;  // only matters for fighter aircraft
	bool levelGround;        // only matters for buildings

	bool useBuildingGroundDecal;
	std::string buildingDecalTypeName;
	int buildingDecalType;
	int buildingDecalSizeX;
	int buildingDecalSizeY;
	float buildingDecalDecaySpeed;

	bool showNanoFrame;								// Does the nano frame animation get shown during construction?
	bool showNanoSpray;								// Does nano spray get shown at all?
	float3 nanoColor;								// If nano spray is displayed what color is it?

	float maxFuel;									// max flight time in seconds before the aircraft needs to return to a air repair bay to refuel
	float refuelTime;								// time to fully refuel unit

	std::vector<std::string> sfxExplGenNames;

	std::string pieceTrailCEGTag;					// base tag (eg. "flame") of CEG attached to pieces of exploding units
	int pieceTrailCEGRange;							// range of piece CEGs (0-based, range 8 ==> tags "flame0", ..., "flame7")

	int maxThisUnit;								// number of units of this type allowed simultaneously in the game

	std::map<std::string, std::string> customParams;

private:
//	float realMetalCost;
//	float realEnergyCost;
//	float realMetalUpkeep;
//	float realEnergyUpkeep;
//	float realBuildTime;
};

} // namespace springLegacyAI

#endif /* UNIT_DEF_H */
