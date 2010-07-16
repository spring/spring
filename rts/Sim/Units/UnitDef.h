/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEF_H
#define UNITDEF_H

#include <string>
#include <vector>
#include <map>

#include "Rendering/Icon.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "System/float3.h"

struct MoveData;
struct WeaponDef;
struct S3DModel;
struct UnitDefImage;
struct CollisionVolume;
class CExplosionGenerator;

struct UnitModelDef
{
	UnitModelDef(): model(NULL) {}

	S3DModel* model;

	std::string modelPath;
	std::string modelName;
	std::map<std::string, std::string> modelTextures;
};

struct UnitDef
{
public:
	UnitDef()
		: valid(false)
		, id(-1)
		, collisionVolume(NULL)
		, decoyDef(NULL)
		, aihint(0)
		, cobID(-1)
		, techLevel(0)
		, gaia("")
		, metalUpkeep(0.0f)
		, energyUpkeep(0.0f)
		, metalMake(0.0f)
		, makesMetal(0.0f)
		, energyMake(0.0f)
		, metalCost(0.0f)
		, energyCost(0.0f)
		, buildTime(0.0f)
		, extractsMetal(0.0f)
		, extractRange(0.0f)
		, windGenerator(0.0f)
		, tidalGenerator(0.0f)
		, metalStorage(0.0f)
		, energyStorage(0.0f)
		, extractSquare(false)
		, autoHeal(0.0f)
		, idleAutoHeal(0.0f)
		, idleTime(0)
		, power(0.0f)
		, health(0.0f)
		, category(-1)
		, speed(0.0f)
		, rSpeed(0.0f)
		, turnRate(0.0f)
		, turnInPlace(false)
		, turnInPlaceDistance(0.0f)
		, turnInPlaceSpeedLimit(0.0f)
		, upright(false)
		, collide(false)
		, controlRadius(0.0f)
		, losRadius(0.0f)
		, airLosRadius(0.0f)
		, losHeight(0.0f)
		, radarRadius(0.0f)
		, sonarRadius(0.0f)
		, jammerRadius(0.0f)
		, sonarJamRadius(0.0f)
		, seismicRadius(0.0f)
		, seismicSignature(0.0f)
		, stealth(false)
		, sonarStealth(false)
		, buildRange3D(false)
		, buildDistance(0.0f)
		, buildSpeed(0.0f)
		, reclaimSpeed(0.0f)
		, repairSpeed(0.0f)
		, maxRepairSpeed(0.0f)
		, resurrectSpeed(0.0f)
		, captureSpeed(0.0f)
		, terraformSpeed(0.0f)
		, mass(0.0f)
		, pushResistant(false)
		, strafeToAttack(false)
		, minCollisionSpeed(0.0f)
		, slideTolerance(0.0f)
		, maxSlope(0.0f)
		, maxHeightDif(0.0f)
		, minWaterDepth(0.0f)
		, waterline(0.0f)
		, maxWaterDepth(0.0f)
		, armoredMultiple(0.0f)
		, armorType(0)
		, flankingBonusMode(0)
		, flankingBonusDir(ZeroVector)
		, flankingBonusMax(0.0f)
		, flankingBonusMin(0.0f)
		, flankingBonusMobilityAdd(0.0f)
		, modelCenterOffset(ZeroVector)
		, usePieceCollisionVolumes(false)
		, shieldWeaponDef(NULL)
		, stockpileWeaponDef(NULL)
		, maxWeaponRange(0.0f)
		, maxCoverage(0.0f)
		, buildPic(NULL)
		, canSelfD(true)
		, selfDCountdown(0)
		, canSubmerge(false)
		, canfly(false)
		, canmove(false)
		, canhover(false)
		, floater(false)
		, builder(false)
		, activateWhenBuilt(false)
		, onoffable(false)
		, fullHealthFactory(false)
		, factoryHeadingTakeoff(false)
		, reclaimable(false)
		, capturable(false)
		, repairable(false)
		, canRestore(false)
		, canRepair(false)
		, canSelfRepair(false)
		, canReclaim(false)
		, canAttack(false)
		, canPatrol(false)
		, canFight(false)
		, canGuard(false)
		, canAssist(false)
		, canBeAssisted(false)
		, canRepeat(false)
		, canFireControl(false)
		, fireState(0)
		, moveState(0)
		, wingDrag(0.0f)
		, wingAngle(0.0f)
		, drag(0.0f)
		, frontToSpeed(0.0f)
		, speedToFront(0.0f)
		, myGravity(0.0f)
		, maxBank(0.0f)
		, maxPitch(0.0f)
		, turnRadius(0.0f)
		, wantedHeight(0.0f)
		, verticalSpeed(0.0f)
		, useSmoothMesh(false)
		, canCrash(false)
		, hoverAttack(false)
		, airStrafe(false)
		, dlHoverFactor(0.0f)
		, bankingAllowed(false)
		, maxAcc(0.0f)
		, maxDec(0.0f)
		, maxAileron(0.0f)
		, maxElevator(0.0f)
		, maxRudder(0.0f)
		, crashDrag(0.0f)
		, movedata(NULL)
		, xsize(0)
		, zsize(0)
		, buildangle(0)
		, loadingRadius(0.0f)
		, unloadSpread(0.0f)
		, transportCapacity(0)
		, transportSize(0)
		, minTransportSize(0)
		, isAirBase(false)
		, isFirePlatform(false)
		, transportMass(0.0f)
		, minTransportMass(0.0f)
		, holdSteady(false)
		, releaseHeld(false)
		, cantBeTransported(false)
		, transportByEnemy(false)
		, transportUnloadMethod(0)
		, fallSpeed(0.0f)
		, unitFallSpeed(0.0f)
		, canCloak(false)
		, startCloaked(false)
		, cloakCost(0.0f)
		, cloakCostMoving(0.0f)
		, decloakDistance(0.0f)
		, decloakSpherical(false)
		, decloakOnFire(false)
		, cloakTimeout(0)
		, canKamikaze(false)
		, kamikazeDist(0.0f)
		, kamikazeUseLOS(false)
		, targfac(false)
		, canDGun(false)
		, needGeo(false)
		, isFeature(false)
		, hideDamage(false)
		, isCommander(false)
		, showPlayerName(false)
		, canResurrect(false)
		, canCapture(false)
		, highTrajectoryType(0)
		, noChaseCategory(0)
		, leaveTracks(false)
		, trackWidth(0.0f)
		, trackOffset(0.0f)
		, trackStrength(0.0f)
		, trackStretch(0.0f)
		, trackType(0)
		, canDropFlare(false)
		, flareReloadTime(0.0f)
		, flareEfficiency(0.0f)
		, flareDelay(0.0f)
		, flareDropVector(ZeroVector)
		, flareTime(0)
		, flareSalvoSize(0)
		, flareSalvoDelay(0)
		, smoothAnim(false)
		, canLoopbackAttack(false)
		, levelGround(false)
		, useBuildingGroundDecal(false)
		, buildingDecalType(0)
		, buildingDecalSizeX(0)
		, buildingDecalSizeY(0)
		, buildingDecalDecaySpeed(0.0f)
		, showNanoFrame(false)
		, showNanoSpray(false)
		, nanoColor(ZeroVector)
		, maxFuel(0.0f)
		, refuelTime(0.0f)
		, minAirBasePower(0.0f)
		, pieceTrailCEGRange(-1)
		, maxThisUnit(0)
		, realMetalCost(0.0f)
		, realEnergyCost(0.0f)
		, realMetalUpkeep(0.0f)
		, realEnergyUpkeep(0.0f)
		, realBuildTime(0.0f)
	{
		yardmaps[0] = NULL;
		yardmaps[1] = NULL;
		yardmaps[2] = NULL;
		yardmaps[3] = NULL;
	}

	~UnitDef();
	S3DModel* LoadModel() const;

	bool DontLand() const { return dlHoverFactor >= 0.0f; }

	void SetNoCost(bool noCost) {
		if (noCost) {
			realMetalCost    = metalCost;
			realEnergyCost   = energyCost;
			realMetalUpkeep  = metalUpkeep;
			realEnergyUpkeep = energyUpkeep;
			realBuildTime    = buildTime;

			metalCost    =  1.0f;
			energyCost   =  1.0f;
			buildTime    = 10.0f;
			metalUpkeep  =  0.0f;
			energyUpkeep =  0.0f;
		} else {
			metalCost    = realMetalCost;
			energyCost   = realEnergyCost;
			buildTime    = realBuildTime;
			metalUpkeep  = realMetalUpkeep;
			energyUpkeep = realEnergyUpkeep;
		}
	}


	bool valid;
	std::string name;
	std::string humanName;
	std::string filename;
	int id;					// unique id for this type of unit

	CollisionVolume* collisionVolume;
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

	float controlRadius;
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
	/// should the unit move sideways when it can't shoot?
	bool strafeToAttack;
	float minCollisionSpeed;
	float slideTolerance;
	float maxSlope;
	/// maximum terraform height this building allows
	float maxHeightDif;
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
		/// How many seconds of fuel it costs for the owning unit to fire this weapon
		float fuelUsage;
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
	mutable UnitDefImage* buildPic;

	mutable CIcon iconType;

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
//	unsigned char* yardmapLevels[6];
	unsigned char* yardmaps[4];						// Iterations of the Ymap for building rotation

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
	int highTrajectoryType;							// 0 (default) = only low, 1 = only high, 2 = choose

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

	bool smoothAnim;								// True if the unit should use interpolated animation
	bool canLoopbackAttack;							// only matters for fighter aircraft
	bool levelGround;								// only matters for buildings

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
	float minAirBasePower;							// min build power for airbases that this aircraft can land on

	std::vector<std::string> sfxExplGenNames;
	std::vector<CExplosionGenerator*> sfxExplGens;	// list of explosion generators for use in scripts

	std::string pieceTrailCEGTag;					// base tag (eg. "flame") of CEG attached to pieces of exploding units
	int pieceTrailCEGRange;							// range of piece CEGs (0-based, range 8 ==> tags "flame0", ..., "flame7")

	int maxThisUnit;								// number of units of this type allowed simultaneously in the game

	std::map<std::string, std::string> customParams;

private:
	float realMetalCost;
	float realEnergyCost;
	float realMetalUpkeep;
	float realEnergyUpkeep;
	float realBuildTime;
};


struct Command;

struct BuildInfo
{
	BuildInfo()
		: def(NULL)
		, pos(ZeroVector)
		, buildFacing(0)
	{}

	BuildInfo(const UnitDef *def, const float3& p, int facing) :
		def(def), pos(p), buildFacing(facing) {}
	BuildInfo(const Command& c) { Parse(c); }
	BuildInfo(const std::string& name, const float3& p, int facing);

	int GetXSize() const { return (buildFacing&1)==0 ? def->xsize : def->zsize; }
	int GetZSize() const { return (buildFacing&1)==1 ? def->xsize : def->zsize; }
	bool Parse(const Command& c);
	void FillCmd(Command& c) const;

	const UnitDef* def;
	float3 pos;
	int buildFacing;
};

#endif /* UNITDEF_H */
