/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEF_H
#define UNITDEF_H

#include <string>
#include <vector>
#include <map>

#include "Rendering/Icon.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "System/float3.h"


struct Command;
struct MoveData;
struct WeaponDef;
struct S3DModel;
struct UnitDefImage;
struct CollisionVolume;
class IExplosionGenerator;
class LuaTable;


struct UnitModelDef
{
	UnitModelDef(): model(NULL) {}

	S3DModel* model;

	std::string modelPath;
	std::string modelName;
	std::map<std::string, std::string> modelTextures;
};


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


struct UnitDef
{
public:
	UnitDef(const LuaTable& udTable, const std::string& unitName, int id);
	UnitDef();
	~UnitDef();

	S3DModel* LoadModel() const;

	bool DontLand() const { return dlHoverFactor >= 0.0f; }
	void SetNoCost(bool noCost);
	bool IsAllowedTerrainHeight(float rawHeight, float* clampedHeight = NULL) const;

	bool IsExtractorUnit()      const { return (extractsMetal > 0.0f); }
	bool IsTransportUnit()      const { return (transportCapacity > 0 && transportMass > 0.0f); }
	bool IsImmobileUnit()       const { return (movedata == NULL && !canfly && speed <= 0.0f); }
	bool IsMobileBuilderUnit()  const { return (builder && !IsImmobileUnit()); }
	bool IsStaticBuilderUnit()  const { return (builder &&  IsImmobileUnit()); }
	bool IsFactoryUnit()        const { return (IsStaticBuilderUnit() && canmove); }
	bool IsGroundUnit()         const { return (movedata != NULL && !canfly); }
	bool IsAirUnit()            const { return (movedata == NULL &&  canfly); }
	bool IsFighterUnit()        const { return (IsAirUnit() && !hoverAttack && !HasBomberWeapon()); }
	bool IsBomberUnit()         const { return (IsAirUnit() && !hoverAttack &&  HasBomberWeapon()); }

	bool WantsMoveType() const { return (canmove && speed > 0.0f); }
	bool HasBomberWeapon() const;
	const std::vector<unsigned char>& GetYardMap(unsigned int facing) const { return (yardmaps[facing % /*NUM_FACINGS*/ 4]); }

	std::string name;
	std::string humanName;
	std::string filename;

	///< unique id for this type of unit
	int id;
	int cobID;				///< associated with the COB <GET COB_ID unitID> call

	CollisionVolume* collisionVolume;

	std::string decoyName;
	const UnitDef* decoyDef;

	int techLevel;

	float metalUpkeep;
	float energyUpkeep;
	float metalMake;		///< metal will always be created
	float makesMetal;		///< metal will be created when unit is on and enough energy can be drained
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

	float autoHeal;     ///< amount autohealed
	float idleAutoHeal; ///< amount autohealed only during idling
	int idleTime;       ///< time a unit needs to idle before its considered idling

	float power;
	float health;
	unsigned int category;

	float speed;        ///< maximum forward speed the unit can attain (elmos/sec)
	float rSpeed;       ///< maximum reverse speed the unit can attain (elmos/sec)
	float turnRate;
	bool turnInPlace;
	/**
	 * units above this distance to goal will try to turn while keeping
	 * some of their speed. 0 to disable
	 */
	float turnInPlaceDistance;
	/**
	 * units below this speed will turn in place regardless of their
	 * turnInPlace setting
	 */
	float turnInPlaceSpeedLimit;

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

	UnitModelDef modelDef;

	std::string objectName;     ///< raw name of the unit's model without objects3d prefix, eg. "armjeth.s3o"
	std::string scriptName;     ///< the name of the unit's script, e.g. "armjeth.cob"
	std::string scriptPath;     ///< the path of the unit's script, e.g. "scripts/armjeth.cob"

	float3 modelCenterOffset;	///< offset from the unit model's default center point

	bool usePieceCollisionVolumes;		///< if true, projectile collisions are checked per-piece

	std::vector<UnitDefWeapon> weapons;
	const WeaponDef* shieldWeaponDef;
	const WeaponDef* stockpileWeaponDef;
	float maxWeaponRange;
	float maxCoverage;

	std::map<int, std::string> buildOptions;

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
	float dlHoverFactor; ///< < 0 means it can land, >= 0 indicates how much the unit will move during hovering on the spot
	bool bankingAllowed;

	float maxAcc;
	float maxDec;
	float maxAileron;
	float maxElevator;
	float maxRudder;
	float crashDrag;

	MoveData* movedata;

	///< Iterations of the yardmap for building rotation
	///< (only non-mobile ground units can have these)
	std::vector<unsigned char> yardmaps[/*NUM_FACINGS*/ 4];

	///< both sizes expressed in heightmap coordinates; M x N
	///< footprint covers M*SQUARE_SIZE x N*SQUARE_SIZE elmos
	int xsize;
	int zsize;

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
	bool cantBeTransported;                         /// defaults to true for buildings, false for all other unit-types
	bool transportByEnemy;
	int transportUnloadMethod;						///< 0 - land unload, 1 - flyover drop, 2 - land flood
	float fallSpeed;								///< dictates fall speed of all transported units
	float unitFallSpeed;							///< sets the transported units fbi, overrides fallSpeed

	bool canCloak;									///< if the unit can cloak
	bool startCloaked;								///< if the units want to start out cloaked
	float cloakCost;								///< energy cost per second to stay cloaked when stationary
	float cloakCostMoving;							///< energy cost per second when moving
	float decloakDistance;							///< if enemy unit come within this range decloaking is forced
	bool decloakSpherical;							///< use a spherical test instead of a cylindrical test?
	bool decloakOnFire;								///< should the unit decloak upon firing
	int cloakTimeout;								///< minimum time between decloak and subsequent cloak

	bool canKamikaze;
	float kamikazeDist;
	bool kamikazeUseLOS;

	bool targfac;
	bool canDGun;
	bool needGeo;
	bool isFeature;
	bool hideDamage;
	bool isCommander;								///< @deprecated, has no meaning to the engine (AI's and gadgets should not rely on it)
	bool showPlayerName;

	bool canResurrect;
	bool canCapture;
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

	bool smoothAnim;         ///< True if the unit should use interpolated animation
	bool canLoopbackAttack;  ///< only matters for fighter aircraft
	bool levelGround;        ///< only matters for buildings

	bool useBuildingGroundDecal;
	std::string buildingDecalTypeName;
	int buildingDecalType;
	int buildingDecalSizeX;
	int buildingDecalSizeY;
	float buildingDecalDecaySpeed;

	bool showNanoFrame;								///< Does the nano frame animation get shown during construction?
	bool showNanoSpray;								///< Does nano spray get shown at all?
	float3 nanoColor;								///< If nano spray is displayed what color is it?

	float maxFuel;									///< max flight time in seconds before the aircraft needs to return to a air repair bay to refuel
	float refuelTime;								///< time to fully refuel unit
	float minAirBasePower;							///< min build power for airbases that this aircraft can land on

	std::vector<std::string> sfxExplGenNames;
	std::vector<IExplosionGenerator*> sfxExplGens;	//< list of explosion generators for use in scripts

	std::string pieceTrailCEGTag;					//< base tag (eg. "flame") of CEG attached to pieces of exploding units
	int pieceTrailCEGRange;							//< range of piece CEGs (0-based, range 8 ==> tags "flame0", ..., "flame7")

	int maxThisUnit;								///< number of units of this type allowed simultaneously in the game

	std::map<std::string, std::string> customParams;

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
