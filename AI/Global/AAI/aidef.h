// AAI          alexander.seizinger@gmx.net	 
// 
// standard headers 

#ifndef AIDEF_H
#define AIDEF_H

#include <set>
#include <list>
#include <stdio.h>
#include <time.h>
#include <string>
#include "ExternalAI/IAICheats.h"
#include "ExternalAI/IGlobalAI.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "ExternalAI/IAICallback.h"
#include "ExternalAI/aibase.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "GlobalStuff.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "AAIConfig.h"


#ifdef _MSC_VER
#pragma warning(disable: 4244 4018) // signed/unsigned and loss of precision...
#endif 

void ReplaceExtension (const char *n, char *dst,int s, const char *ext);

#define AAI_VERSION "0.761"
#define MAP_FILE_VERSION "MAP_LEARN_0_68"
#define TABLE_FILE_VERSION "MOD_LEARN_0_75"
#define MAP_DATA_VERSION "MAP_DATA_0_60"

// all paths 
#ifdef WIN32
    #define MAIN_PATH "ai\\aai\\"
	#define AILOG_PATH "log\\"
	#define MOD_CFG_PATH "cfg\\mod\\"
	#define GENERAL_CFG_FILE "cfg\\general.cfg"
	#define MOD_LEARN_PATH "learn\\mod\\"
	#define MAP_CFG_PATH "cfg\\map\\"
	#define MAP_CACHE_PATH "cache\\"
	#define MAP_LEARN_PATH "learn\\map\\"
#else
	#define MAIN_PATH "AI/AAI/"
	#define AILOG_PATH "log/"
	#define MOD_CFG_PATH "cfg/mod/"
	#define GENERAL_CFG_FILE "cfg/general.cfg"
	#define MOD_LEARN_PATH "learn/mod/"
	#define MAP_CFG_PATH "cfg/map/"
	#define MAP_CACHE_PATH "cache/"
	#define MAP_LEARN_PATH "learn/map/"
#endif

extern AAIConfig *cfg;

class AAIMetalSpot
{
public:
	AAIMetalSpot(float3 pos, float amount) {this->pos = pos; this->amount = amount; occupied = false; extractor = -1; extractor_def = -1;}
	AAIMetalSpot() {pos = ZeroVector; amount = 0; occupied = false; extractor = -1; extractor_def = -1;}
	
	float3 pos;
	bool occupied;
	int extractor;		// -1 if unocuppied
	int extractor_def;	// -1 if unocuppied
	float amount;
};

enum Direction {WEST, EAST, SOUTH, NORTH, CENTER, NO_DIRECTION};

enum UnitMoveType {GROUND, AIR, HOVER,  SEA};

enum MapType {UNKNOWN_MAP, LAND_MAP, AIR_MAP, LAND_WATER_MAP, WATER_MAP};

enum SectorType {UNKNOWN_SECTOR, LAND_SECTOR, LAND_WATER_SECTOR, WATER_SECTOR};

enum UnitCategory {UNKNOWN, STATIONARY_DEF, STATIONARY_ARTY, STORAGE, GROUND_FACTORY, SEA_FACTORY, AIR_BASE,
STATIONARY_RECON, STATIONARY_JAMMER, STATIONARY_LAUNCHER, BARRICADE, DEFLECTION_SHIELD, POWER_PLANT, EXTRACTOR, METAL_MAKER, 
COMMANDER, GROUND_ASSAULT, AIR_ASSAULT, HOVER_ASSAULT, SEA_ASSAULT, SUBMARINE_ASSAULT, GROUND_ARTY, SEA_ARTY, HOVER_ARTY, GROUND_SCOUT, AIR_SCOUT, HOVER_SCOUT, 
SEA_SCOUT, MOBILE_JAMMER, MOBILE_LAUNCHER, GROUND_BUILDER, AIR_BUILDER, HOVER_BUILDER, SEA_BUILDER}; 

enum GroupTask {GROUP_IDLE, GROUP_ATTACKING, GROUP_DEFENDING, GROUP_BOMBING};

enum UnitType {UNKNOWN_UNIT, ASSAULT_UNIT, ANTI_AIR_UNIT, BOMBER_UNIT, ARTY_UNIT};

enum UnitTask {UNIT_IDLE, UNIT_ATTACKING, DEFENDING, GUARDING, MOVING, BUILDING, SCOUTING, ASSISTING, RECLAIMING, HEADING_TO_RALLYPOINT, UNIT_KILLED, ENEMY_UNIT};

enum AttackType {NO_ATTACK, BASE_ATTACK, OUTPOST_ATTACK};

enum BuildOrderStatus {BUILDORDER_FAILED, BUILDORDER_NOBUILDPOS, BUILDORDER_NOBUILDER, BUILDORDER_SUCCESFUL};

struct AAIDefence
{
	int unit_id;
	int def_id;
	Direction direction;
};

struct AAIAirTarget
{
	float3 pos;
	int def_id;
	float cost;
	float health;
	UnitCategory category;
};

struct UnitTypeDynamic
{
	int requested;			// how many units of that type have been requested
	int active;				// how many units of that type are currently alive
	bool builderAvailable;	// is ai already able to build that unit
	bool builderRequested;	// has ai already requested a builder for that unit
};

struct UnitTypeStatic
{
	int id;
	int side; // 1 means arm, 2 core; 0 if side has not been set 
	list<int> canBuildList;
	list<int> builtByList;
	float *efficiency;	// 0 -> ground assault, 1 -> air assault, 2 -> hover assault
							// 3 -> sea assault, 4 -> arty, 5 -> stat. defences
	float range;
	float cost;
	float builder_cost;
	float builder_metal_cost;  // cost of the factory/builder which is requiered
	float builder_energy_cost;
	UnitCategory category;
};

class AAIGroup;
class AAIBuilder;
class AAIFactory;

struct AAIUnit
{
	int unit_id;
	int def_id;
	AAIGroup *group;
	AAIBuilder *builder;
	AAIFactory *factory;
	UnitTask status;
};


struct ProductionRequest
{
	int builder_id;		// id of that building/builder/mine layer etc.
	int built;			// how many facs/builder of that type have been build
	int requested;		// how many units/buildings need this fac. to be built
};

typedef unsigned char uchar;

#endif
