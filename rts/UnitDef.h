#ifndef UNIT_DEF_STRUCT
#define UNIT_DEF_STRUCT

#pragma once

#include <string>
#include <vector>
#include <map>

struct MoveData;
struct WeaponDef;

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
	std::string name;
	std::string humanName;
	std::string filename;
	bool loaded;
	int id;											//unique id for this type of unit
	unsigned int unitimage;

	int aihint;

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

	std::vector<std::string> sweapons;
	std::vector<WeaponDef*> weapons;

	std::map<int,std::string> buildOptions;

	std::string type;
	std::string tooltip;
	std::string wreckName;

	std::string deathExplosion;
	std::string selfDExplosion;

	int selfDCountdown;

	bool canfly;
	bool canmove;
	bool canhover;
	bool floater;
	bool builder;
	bool activateWhenBuilt;
	bool onoffable;

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

	float maxAcc;
	float maxDec;
	float maxAileron;
	float maxElevator;
	float maxRudder;

	MoveData* movedata;
//	unsigned char* yardmapLevels[6];
	unsigned char* yardmap;				//Used for open ground blocking maps.

	int xsize;									//each size is 8 units
	int ysize;									//each size is 8 units

	float loadingRadius;	//for transports
	int transportCapacity;
	int transportSize;

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

	unsigned int noChaseCategory;

	struct
	{
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
	}sounds;
};


#endif