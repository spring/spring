#ifndef UNITDEF_H
#define UNITDEF_H

#include <string>
#include <vector>
#include <map>

#include "creg/creg.h"

struct MoveData;
struct WeaponDef;

const int MAX_UNITS=5000;

struct GuiSound
{
	std::string name;
	int id;
	float volume;
};

struct UnitModelDef
{
	std::string modelpath;
	std::string modelname;
	std::map<std::string, std::string> textures;
};

struct UnitDef
{
	CR_DECLARE(UnitDef);
        virtual ~UnitDef();

	std::string name;
	std::string humanName;
	std::string filename;
	bool loaded;
	int id;											//unique id for this type of unit
	unsigned int unitimage; // don't read this directly use CUnitDefHandler::GetUnitImage instead
	std::string buildpicname;

	int aihint;
	int techLevel;

	float metalUpkeep;
	float energyUpkeep;
	float metalMake;	//metal will allways be created
	float makesMetal;	//metal will be created when unit is on and enough energy can be drained
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

	float autoHeal;		//amount autohealed
	float idleAutoHeal;	//amount autohealed only during idling
	int idleTime;		//time a unit needs to idle before its considered idling

	float power;
	float health;
	unsigned int category;

	float speed;
	float turnRate;
	int moveType;
	bool upright;

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

	float buildSpeed;
	float buildDistance;
	float mass;

	float maxSlope;
	float maxHeightDif;									//maximum terraform height this building allows
	float minWaterDepth;
	float waterline;

	float maxWaterDepth;

	float armoredMultiple;
	int armorType;

	UnitModelDef model;

	struct UnitDefWeapon {
		UnitDefWeapon(std::string name,WeaponDef* def,int slavedTo,float3 mainDir,float maxAngleDif,unsigned int badTargetCat,unsigned int onlyTargetCat,float fuelUse)
			: name(name),
				def(def),
				slavedTo(slavedTo),
				mainDir(mainDir),
				maxAngleDif(maxAngleDif),
				badTargetCat(badTargetCat),
				onlyTargetCat(onlyTargetCat),
				fuelUsage(fuelUse) {}

		std::string name;
		WeaponDef* def;
		int slavedTo;
		float3 mainDir;
		float maxAngleDif;
		float fuelUsage;							//how many seconds of fuel it costs for the owning unit to fire this weapon
		unsigned int badTargetCat;
		unsigned int onlyTargetCat;
	};
	std::vector<UnitDefWeapon> weapons;

	std::map<int,std::string> buildOptions;

	std::string type;
	std::string tooltip;
	std::string wreckName;

	std::string deathExplosion;
	std::string selfDExplosion;

	std::string TEDClassString;	//these might be changed later for something better
	std::string categoryString;

	std::string iconType;

	int selfDCountdown;

	bool canfly;
	bool canmove;
	bool canhover;
	bool floater;
	bool builder;
	bool activateWhenBuilt;
	bool onoffable;

	bool reclaimable;
	bool canRestore;
	bool canRepair;
	bool canReclaim;
	bool noAutoFire;
	bool canAttack;
	bool canPatrol;
	bool canGuard;
	bool canBuild;
	bool canAssist;

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
	bool hoverAttack;
	float dlHoverFactor; // < 0 means it can land, >= 0 indicates how much the unit will move during hovering on the spot
	bool DontLand () { return dlHoverFactor >= 0.0f; }

	float maxAcc;
	float maxDec;
	float maxAileron;
	float maxElevator;
	float maxRudder;

	MoveData* movedata;
//	unsigned char* yardmapLevels[6];
	unsigned char* yardmaps[4];			//Iterations of the Ymap for building rotation
	
	int xsize;									//each size is 8 units
	int ysize;									//each size is 8 units

	int buildangle;

	float loadingRadius;	//for transports
	int transportCapacity;
	int transportSize;
	bool isAirBase;
	float transportMass;

	bool canCloak;							//if the unit can cloak
	bool startCloaked;					//if the units want to start out cloaked
	float cloakCost;						//energy cost per second to stay cloaked when stationary
	float cloakCostMoving;			//energy cost per second when moving
	float decloakDistance;			//if enemy unit come within this range decloaking is forced

	bool canKamikaze;						//self destruct if enemy come to close
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
	int highTrajectoryType;			//0(default)=only low,1=only high,2=choose

	unsigned int noChaseCategory;

	struct SoundStruct {
		GuiSound select;
		GuiSound ok;
		GuiSound arrived;
		GuiSound build;
		GuiSound repair;
		GuiSound working;
		GuiSound underattack;
		GuiSound cant;
		GuiSound activate;
		GuiSound deactivate;
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
	float flareEfficieny;
	float flareDelay;
	float3 flareDropVector;
	int flareTime;
	int flareSalvoSize;
	int flareSalvoDelay;

	bool smoothAnim;			// True if the unit should use interpolated animation
	bool isMetalMaker;
	bool canLoopbackAttack;		//only matters for fighter aircrafts
	bool levelGround;			//only matters for buildings

	bool useBuildingGroundDecal;
	int buildingDecalType;
	int buildingDecalSizeX;
	int buildingDecalSizeY;
	float buildingDecalDecaySpeed;
	bool isfireplatform;// should the carried units still be able to shoot?

	bool showNanoFrame; // Does the nano frame animation get shown during cosntruction?
	bool showNanoSpray; // Does nano spray get shown at all
	float3 nanoColor; // If nano spray is displayed what color is it?

	float maxFuel;					//max flight time in seconds before the aircraft needs to return to a air repair bay to refuel
	float refuelTime;				//time to fully refuel unit
	float minAirBasePower;	//min build power for airbases that this aircraft can land on
};

struct Command;

struct BuildInfo
{
	BuildInfo() { def=0; buildFacing=0; }
	BuildInfo(const UnitDef *def, const float3& p, int facing) :
		def(def), pos(p), buildFacing(facing) {}
	BuildInfo(Command& c) { Parse(c); }
	BuildInfo(const std::string& name, const float3& p, int facing);

	int GetXSize() const { return (buildFacing&1)==0 ? def->xsize : def->ysize; }
	int GetYSize() const { return (buildFacing&1)==1 ? def->xsize : def->ysize; }
	bool Parse(const Command& c);
	void FillCmd(Command& c) const;

	const UnitDef* def;
	int buildFacing;
	float3 pos;
};


#endif /* UNITDEF_H */
