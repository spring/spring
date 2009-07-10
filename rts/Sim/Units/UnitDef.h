#ifndef UNITDEF_H
#define UNITDEF_H

#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include "creg/creg_cond.h"
#include "float3.h"
#include "Rendering/Icon.h"

struct MoveData;
struct WeaponDef;
struct S3DModel;
struct UnitDefImage;
struct CollisionVolume;
class CExplosionGenerator;


struct GuiSoundSet
{
	struct Data {
		Data() : name(""), id(0), volume(0.0f) {}
		Data(const std::string& n, int i, float v) : name(n), id(i), volume(v) {}
		std::string name;
		int id;
		float volume;
	};
	std::vector<Data> sounds;

	// return a random sound index if more than one sound was loaded
	// (used for unit acknowledgements, could be called for weapons too)
	int getRandomIdx(void) const {
		switch (sounds.size()) {
			case 0:  { return -1; break; }
			case 1:  { return  0; break; }
			default: { return rand() % sounds.size(); }
		}
	}

	bool ValidIndex(int idx) const {
		return ((idx >= 0) && (idx < (int)sounds.size()));
	}

	// get a (loaded) sound's name for index <idx>
	std::string getName(int idx) const {
		return ValidIndex(idx) ? sounds[idx].name : "";
	}
	// get a (loaded) sound's ID for index <idx>
	int getID(int idx) const {
		return ValidIndex(idx) ? sounds[idx].id : 0;
	}
	// get a (loaded) sound's volume for index <idx>
	float getVolume(int idx) const {
		return ValidIndex(idx) ? sounds[idx].volume : 0.0f;
	}

	// set a (loaded) sound's name for index <idx>
	void setName(int idx, std::string name) {
		if (ValidIndex(idx)) {
			sounds[idx].name = name;
		}
	}
	// set a (loaded) sound's ID for index <idx>
	void setID(int idx, int id) {
		if (ValidIndex(idx)) {
			sounds[idx].id = id;
		}
	}
	// set a (loaded) sound's volume for index <idx>
	void setVolume(int idx, float volume) {
		if (ValidIndex(idx)) {
			sounds[idx].volume = volume;
		}
	}
};



struct UnitModelDef
{
	UnitModelDef():model(NULL) {};
	S3DModel* model;
	std::string modelpath;
	std::string modelname;
	std::map<std::string, std::string> textures;
};

struct UnitDef
{
	UnitDef() : valid(false) {}
	~UnitDef();
	S3DModel* LoadModel() const;

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
	bool isMetalMaker;

	float autoHeal;		//amount autohealed
	float idleAutoHeal;	//amount autohealed only during idling
	int idleTime;		//time a unit needs to idle before its considered idling

	float power;
	float health;
	unsigned int category;

	float speed;
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

	std::string scriptName;		// the name of the unit's script, e.g. "armjeth.cob"
	std::string scriptPath;		// the path of the unit's script, e.g. "scripts/armjeth.cob"

	float3 modelCenterOffset;	// offset from the unit model's default center point

	std::string collisionVolumeTypeStr;	// can be "Ell", "CylT" (where T is one of "XYZ"), or "Box"
	float3 collisionVolumeScales;		// the collision volume's full axis lengths
	float3 collisionVolumeOffsets;		// relative to the unit's center position
	int collisionVolumeTest;			// 0: discrete, 1: continuous
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

	std::string TEDClassString;	// these might be changed later for something better
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
	bool canCrash;
	bool hoverAttack;
	bool airStrafe;
	float dlHoverFactor; // < 0 means it can land, >= 0 indicates how much the unit will move during hovering on the spot
	bool DontLand () const { return dlHoverFactor >= 0.0f; }
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

	bool canKamikaze;								//self destruct if enemy come to close
	float kamikazeDist;

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

	std::vector<CExplosionGenerator*> sfxExplGens;	// list of explosion generators for use in scripts
	std::string pieceTrailCEGTag;					// base tag (eg. "flame") of CEG attached to pieces of exploding units
	int pieceTrailCEGRange;							// range of piece CEGs (0-based, range 8 ==> tags "flame0", ..., "flame7")

	int maxThisUnit;								// number of units of this type allowed simultaneously in the game

	std::map<std::string, std::string> customParams;
};


struct Command;

struct BuildInfo
{
	BuildInfo() { def=0; buildFacing=0; }
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
