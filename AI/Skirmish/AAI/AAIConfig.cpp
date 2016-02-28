// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIConfig.h"
#include "AAI.h"
#include "System/SafeCStrings.h"
#include "System/Util.h"

// all paths
#define CFG_PATH "cfg/"
#define MOD_CFG_PATH CFG_PATH "mod/"
#define CONFIG_SUFFIX ".cfg"
#define GENERAL_CFG_FILE "general" CONFIG_SUFFIX


#include "LegacyCpp/UnitDef.h"
using namespace springLegacyAI;


static bool IsFSGoodChar(const char c) {

	if ((c >= '0') && (c <= '9')) {
		return true;
	} else if ((c >= 'a') && (c <= 'z')) {
		return true;
	} else if ((c >= 'A') && (c <= 'Z')) {
		return true;
	} else if ((c == '.') || (c == '_') || (c == '-')) {
		return true;
	}

	return false;
}

// declaration is in aidef.h
std::string MakeFileSystemCompatible(const std::string& str) {

	std::string cleaned = str;

	for (std::string::size_type i=0; i < cleaned.size(); i++) {
		if (!IsFSGoodChar(cleaned[i])) {
			cleaned[i] = '_';
		}
	}

	return cleaned;
}


int AAIConfig::GetInt(AAI* ai, FILE* file)
{
	int val = 0;
	int res = fscanf(file, "%i", &val);
	if (res != 1) {
		ai->Log("Error while parsing config");
	}
	return val;
}

std::string AAIConfig::GetString(AAI* ai, FILE* file)
{
	char buf[128];
	buf[0] = 0; //safety!
	int res = fscanf(file, "%s", buf);
	if (res != 1) {
		ai->Log("Error while parsing config");
	}
	return std::string(buf);
}

float AAIConfig::GetFloat(AAI* ai, FILE* file)
{
	float val = 0.0f;
	int res = fscanf(file, "%f", &val);
	if (res != 1) {
		ai->Log("Error while parsing config");
	}
	return val;
}


AAIConfig::AAIConfig(void)
{
	SIDES = 2;
	SECTOR_SIZE = 100.0;
	MIN_ENERGY = 18;  // min energy make value to be considered beeing a power plant
	MAX_UNITS = 30000;
	MAX_SCOUTS = 4;
	MAX_SECTOR_IMPORTANCE = 6;
	MAX_XROW = 8;
	MAX_YROW = 8;
	X_SPACE = 16;
	Y_SPACE = 16;
	MAX_GROUP_SIZE = 12;
	MAX_AIR_GROUP_SIZE = 4;
	MAX_ANTI_AIR_GROUP_SIZE = 4;
	MAX_SUBMARINE_GROUP_SIZE = 4;
	MAX_NAVAL_GROUP_SIZE = 4;
	MAX_ARTY_GROUP_SIZE = 4;
	MIN_EFFICIENCY = 0.001f;
	MAX_BUILDERS = 50;
	MAX_BUILDERS_PER_TYPE = 5;
	MAX_FACTORIES_PER_TYPE = 3;
	MAX_BUILDQUE_SIZE = 12;
	MAX_ASSISTANTS = 4;
	MIN_ASSISTANCE_BUILDTIME = 15;
	MIN_ASSISTANCE_BUILDSPEED = 20;
	MAX_BASE_SIZE = 10;
	SCOUT_SPEED = 95.0;
	GROUND_ARTY_RANGE = 1000.0;
	SEA_ARTY_RANGE = 1300.0;
	HOVER_ARTY_RANGE = 1000.0;
	STATIONARY_ARTY_RANGE = 2000;
	AIR_DEFENCE = 8;
	MIN_ENERGY_STORAGE = 500;
	MIN_METAL_STORAGE = 100;
	MAX_METAL_COST = 10000;
	MIN_AIR_ATTACK_COST = 150;
	MAX_AIR_TARGETS = 20;
	AIRCRAFT_RATE = 6;
	HIGH_RANGE_UNITS_RATE = 4;
	FAST_UNITS_RATE = 5;
	METAL_ENERGY_RATIO = 25;
	MAX_DEFENCES = 12;
	MIN_SECTOR_THREAT = 6;
	MAX_STAT_ARTY = 3;
	MAX_STORAGE = 6;
	MAX_AIR_BASE = 1;
	AIR_ONLY_MOD = false;
	MAX_METAL_MAKERS = 20;
	MIN_METAL_MAKER_ENERGY = 100;
	MAX_MEX_DISTANCE = 7;
	MAX_MEX_DEFENCE_DISTANCE = 5;
	MIN_FACTORIES_FOR_DEFENCES = 1;
	MIN_FACTORIES_FOR_STORAGE = 2;
	MIN_FACTORIES_FOR_RADAR_JAMMER = 2;
	MIN_AIR_SUPPORT_EFFICIENCY = 2.5f;
	UNIT_SPEED_SUBGROUPS = 3;
	MIN_SUBMARINE_WATERLINE = 15;
	MAX_ATTACKS = 4;

	NON_AMPHIB_MAX_WATERDEPTH = 15.0f;

	MAX_COST_LIGHT_ASSAULT = 0.025f;
	MAX_COST_MEDIUM_ASSAULT = 0.13f;
	MAX_COST_HEAVY_ASSAULT = 0.55f;

	LIGHT_ASSAULT_RATIO = 40.0f;
	MEDIUM_ASSAULT_RATIO = 30.0f;
	HEAVY_ASSAULT_RATIO = 25.0f;
	SUPER_HEAVY_ASSAULT_RATIO = 5.0f;

	FALLBACK_DIST_RATIO = 0.9f;
	MIN_FALLBACK_RANGE = 450.0f;
	MAX_FALLBACK_RANGE = 800.0f;
	MIN_FALLBACK_TURNRATE = 250.0f;

	LEARN_SPEED = 0.2f;
	LEARN_RATE = 5;
	CONSTRUCTION_TIMEOUT = 1500;
	CLIFF_SLOPE = 0.085f;
	SCOUT_UPDATE_FREQUENCY = 127;
	SCOUTING_MEMORY_FACTOR = 1.0f;
	WATER_MAP_RATIO = 0.8f;
	LAND_WATER_MAP_RATIO = 0.3f;

	GAME_PERIODS = 4;
	initialized = false;
}

AAIConfig::~AAIConfig(void)
{
	START_UNITS.clear();
	SIDE_NAMES.clear();
}


std::string AAIConfig::GetFileName(AAI* ai, const std::string& filename, const std::string& prefix, const std::string& suffix, bool write) const
{
	std::string name = prefix + MakeFileSystemCompatible(filename) + suffix;

	// this size equals the one used in "AIAICallback::GetValue(AIVAL_LOCATE_FILE_..."
	char buffer[2048];
	STRCPY_T(buffer, sizeof(buffer), name.c_str());
	if (write) {
		ai->Getcb()->GetValue(AIVAL_LOCATE_FILE_W, buffer);
	} else {
		ai->Getcb()->GetValue(AIVAL_LOCATE_FILE_R, buffer);
	}
	name.assign(buffer, sizeof(buffer));
	return name;
}

void AAIConfig::LoadConfig(AAI *ai)
{
	MAX_UNITS = ai->Getcb()->GetMaxUnits();


	std::list<string> paths;
	paths.push_back(GetFileName(ai, ai->Getcb()->GetModHumanName(), MOD_CFG_PATH, CONFIG_SUFFIX));
	paths.push_back(GetFileName(ai, ai->Getcb()->GetModName(), MOD_CFG_PATH, CONFIG_SUFFIX));
	paths.push_back(GetFileName(ai, ai->Getcb()->GetModShortName(), MOD_CFG_PATH, CONFIG_SUFFIX));
	FILE* file = NULL;
	std::string configfile;
	for(const std::string& path: paths) {
		file = fopen(path.c_str(), "r");
		if (file == NULL) {
			ai->Log("Couldn't open config file %s\n", path.c_str());
		} else {
			configfile = path;
			break;
		}
	}

	if (file == NULL) {
		ai->Log("Give up trying to find mod config file (required).\n");
		initialized = false;
		return;
	}

	char keyword[50];

	bool error = false;
//	bool loaded = false;

	if(file == NULL)
		return;

	while(EOF != fscanf(file, "%s", keyword))
	{
		if(!strcmp(keyword,"SIDES")) {
			SIDES = GetInt(ai, file);
		}
		else if(!strcmp(keyword, "START_UNITS")) {
			START_UNITS.resize(SIDES);
			for(int i = 0; i < SIDES; i++) {
				START_UNITS[i] = GetString(ai, file);
				if(!GetUnitDef(ai, START_UNITS[i].c_str())) {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "SIDE_NAMES")) {
			SIDE_NAMES.resize(SIDES);
			for(int i = 0; i < SIDES; i++) {
				SIDE_NAMES[i] = GetString(ai, file);
			}
		} else if(!strcmp(keyword, "SCOUTS")) {
			// get number of scouts
			const int count = GetInt(ai, file);
			for(int i = 0; i < count; ++i) {
				const std::string unitdef = GetString(ai, file);
				if(GetUnitDef(ai, unitdef)) {
					SCOUTS.push_back(GetUnitDef(ai, unitdef)->id);
				} else {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "ATTACKERS")) {
			// get number of attackers
			const int count = GetInt(ai, file);

			for(int i = 0; i < count; ++i) {
				const std::string unitdef = GetString(ai, file);
				if(GetUnitDef(ai, unitdef))
					ATTACKERS.push_back(GetUnitDef(ai, unitdef)->id);
				else {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "TRANSPORTERS")) {
			// get number of transporters
			const int count = GetInt(ai, file);

			for(int i = 0; i < count; ++i) {
				const std::string unitdef = GetString(ai, file);
				if(GetUnitDef(ai, unitdef))
					TRANSPORTERS.push_back(GetUnitDef(ai, unitdef)->id);
				else {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "METAL_MAKERS")) {
			// get number of transporters
			const int count = GetInt(ai, file);
			for(int i = 0; i < count; ++i) {
				const std::string unitdef = GetString(ai, file);
				if(GetUnitDef(ai, unitdef))
					METAL_MAKERS.push_back(GetUnitDef(ai, unitdef)->id);
				else {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "DONT_BUILD")) {
			// get number of units that should not be built
			const int count = GetInt(ai, file);
			for(int i = 0; i < count; ++i) {
				const std::string unitdef = GetString(ai, file);
				if(GetUnitDef(ai, unitdef))
					DONT_BUILD.push_back(GetUnitDef(ai, unitdef)->id);
				else {
					error = true;
					break;
				}
			}
		} else if(!strcmp(keyword, "COST_MULTIPLIER")) {
			// get the unit def
			const std::string unitdef = GetString(ai, file);
			const UnitDef* def = GetUnitDef(ai, unitdef);

			if(def)
			{
				CostMultiplier temp;
				temp.id = def->id;
				temp.multiplier = GetFloat(ai, file);

				cost_multipliers.push_back(temp);
			} else {
				error = true;
				break;
			}
		} else if(!strcmp(keyword,"SECTOR_SIZE")) {
			SECTOR_SIZE = GetFloat(ai, file);
			ai->Log("SECTOR_SIZE set to %f", SECTOR_SIZE);
		} else if(!strcmp(keyword,"MIN_ENERGY")) {
			MIN_ENERGY = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_UNITS")) {
			MAX_UNITS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_SCOUTS")) {
			MAX_SCOUTS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_SECTOR_IMPORTANCE")) {
			MAX_SECTOR_IMPORTANCE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_XROW")) {
			MAX_XROW = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_YROW")) {
			MAX_YROW = GetInt(ai, file);
		} else if(!strcmp(keyword, "X_SPACE")) {
			X_SPACE = GetInt(ai, file);
		} else if(!strcmp(keyword, "Y_SPACE")) {
			Y_SPACE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_GROUP_SIZE")) {
			MAX_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_AIR_GROUP_SIZE")) {
			MAX_AIR_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_NAVAL_GROUP_SIZE")) {
			MAX_NAVAL_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_SUBMARINE_GROUP_SIZE")) {
			MAX_SUBMARINE_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_ANTI_AIR_GROUP_SIZE")) {
			MAX_ANTI_AIR_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_ARTY_GROUP_SIZE")) {
			MAX_ARTY_GROUP_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "UNIT_SPEED_SUBGROUPS")) {
			UNIT_SPEED_SUBGROUPS = GetInt(ai, file);
		} else if(!strcmp(keyword, "FALLBACK_DIST_RATIO")) {
			FALLBACK_DIST_RATIO = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_FALLBACK_RANGE")) {
			MIN_FALLBACK_RANGE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_FALLBACK_RANGE")) {
			MAX_FALLBACK_RANGE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_FALLBACK_TURNRATE")) {
			MIN_FALLBACK_TURNRATE = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MIN_EFFICIENCY")) {
			MIN_EFFICIENCY = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MIN_AIR_SUPPORT_EFFICIENCY")) {
			MIN_AIR_SUPPORT_EFFICIENCY = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MAX_BUILDERS")) {
			MAX_BUILDERS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_BUILDQUE_SIZE")) {
			MAX_BUILDQUE_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_ASSISTANTS")) {
			MAX_ASSISTANTS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_ASSISTANCE_BUILDSPEED")) {
			MIN_ASSISTANCE_BUILDSPEED = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_BASE_SIZE")) {
			MAX_BASE_SIZE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_AIR_TARGETS")) {
			MAX_AIR_TARGETS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_AIR_ATTACK_COST")) {
			MIN_AIR_ATTACK_COST = GetInt(ai, file);
		} else if(!strcmp(keyword, "SCOUT_SPEED")) {
			SCOUT_SPEED = GetFloat(ai, file);
		} else if(!strcmp(keyword, "GROUND_ARTY_RANGE")) {
			GROUND_ARTY_RANGE = GetInt(ai, file);
		} else if(!strcmp(keyword, "SEA_ARTY_RANGE")) {
			SEA_ARTY_RANGE = GetFloat(ai, file);
		} else if(!strcmp(keyword, "HOVER_ARTY_RANGE")) {
			HOVER_ARTY_RANGE = GetFloat(ai, file);
		} else if(!strcmp(keyword, "STATIONARY_ARTY_RANGE")) {
			STATIONARY_ARTY_RANGE = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MAX_BUILDERS_PER_TYPE")) {
			MAX_BUILDERS_PER_TYPE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_FACTORIES_PER_TYPE")) {
			MAX_FACTORIES_PER_TYPE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_ASSISTANCE_BUILDTIME")) {
			MIN_ASSISTANCE_BUILDTIME = GetInt(ai, file);
		} else if(!strcmp(keyword, "AIR_DEFENCE")) {
			AIR_DEFENCE = GetInt(ai, file);
		} else if(!strcmp(keyword, "AIRCRAFT_RATE")) {
			AIRCRAFT_RATE = GetInt(ai, file);
		} else if(!strcmp(keyword, "HIGH_RANGE_UNITS_RATE")) {
			HIGH_RANGE_UNITS_RATE = GetInt(ai, file);
		} else if(!strcmp(keyword, "FAST_UNITS_RATE")) {
			FAST_UNITS_RATE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_METAL_COST")) {
			MAX_METAL_COST = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_DEFENCES")) {
			MAX_DEFENCES = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_SECTOR_THREAT")) {
			MIN_SECTOR_THREAT = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MAX_STAT_ARTY")) {
			MAX_STAT_ARTY = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_AIR_BASE")) {
			MAX_AIR_BASE = GetInt(ai, file);
		} else if(!strcmp(keyword, "AIR_ONLY_MOD")) {
			AIR_ONLY_MOD = (bool)GetInt(ai, file);
		} else if(!strcmp(keyword, "METAL_ENERGY_RATIO")) {
			METAL_ENERGY_RATIO = GetFloat(ai, file);
		} else if(!strcmp(keyword, "NON_AMPHIB_MAX_WATERDEPTH")) {
			NON_AMPHIB_MAX_WATERDEPTH = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MAX_METAL_MAKERS")) {
			MAX_METAL_MAKERS = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_STORAGE")) {
			MAX_STORAGE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_METAL_MAKER_ENERGY")) {
			MIN_METAL_MAKER_ENERGY = GetFloat(ai, file);
		} else if(!strcmp(keyword, "MAX_MEX_DISTANCE")) {
			MAX_MEX_DISTANCE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_MEX_DEFENCE_DISTANCE")) {
			MAX_MEX_DEFENCE_DISTANCE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_FACTORIES_FOR_DEFENCES")) {
			MIN_FACTORIES_FOR_DEFENCES = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_FACTORIES_FOR_STORAGE")) {
			MIN_FACTORIES_FOR_STORAGE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_FACTORIES_FOR_RADAR_JAMMER")) {
			MIN_FACTORIES_FOR_RADAR_JAMMER = GetInt(ai, file);
		} else if(!strcmp(keyword, "MIN_SUBMARINE_WATERLINE")) {
			MIN_SUBMARINE_WATERLINE = GetInt(ai, file);
		} else if(!strcmp(keyword, "MAX_ATTACKS")) {
			MAX_ATTACKS = GetInt(ai, file);
		} else {
			error = true;
			break;
		}
	}

	if(error) {
		ai->Log("Mod config file %s contains erroneous keyword: %s\n", configfile.c_str(), keyword);
		initialized = false;
		return;
	}

	fclose(file);
	ai->Log("Mod config file %s loaded\n", configfile.c_str());

	// load general settings
	const std::string generalcfg = GetFileName(ai, GENERAL_CFG_FILE, CFG_PATH);
	file = fopen(generalcfg.c_str(), "r");
	if(file == NULL) {
		ai->Log("Couldn't load general config file %s\n", generalcfg.c_str());
		return;
	}

	while(EOF != fscanf(file, "%s", keyword))
	{
		if(!strcmp(keyword, "LEARN_RATE")) {
			LEARN_RATE = GetInt(ai, file);
		} else if(!strcmp(keyword, "LEARN_SPEED")) {
			LEARN_SPEED = GetFloat(ai, file);
		} else if(!strcmp(keyword, "WATER_MAP_RATIO")) {
			WATER_MAP_RATIO = GetFloat(ai, file);
		} else if(!strcmp(keyword, "LAND_WATER_MAP_RATIO")) {
			LAND_WATER_MAP_RATIO = GetFloat(ai, file);
		} else if(!strcmp(keyword, "SCOUT_UPDATE_FREQUENCY")) {
			SCOUT_UPDATE_FREQUENCY = GetInt(ai, file);;
		} else if(!strcmp(keyword, "SCOUTING_MEMORY_FACTOR")) {
			SCOUTING_MEMORY_FACTOR = GetFloat(ai, file);
		} else {
			error = true;
			break;
		}
	}

	fclose(file);

	if(error) {
		ai->Log("General config file contains erroneous keyword %s\n", keyword);
		return;
	}
	ai->Log("General config file loaded\n");
	initialized = true;
}

const UnitDef* AAIConfig::GetUnitDef(AAI* ai, const std::string& name)
{
	const UnitDef* res = ai->Getcb()->GetUnitDef(name.c_str());
	if (res == NULL) {
		ai->Log("ERROR: loading unit - could not find unit %s\n", name.c_str());
	}
	return res;
}

std::string AAIConfig::getUniqueName(AAI* ai, bool game, bool gamehash, bool map, bool maphash) const
{
	std::string res;
	if (map) {
		if (!res.empty())
			res += "-";
		std::string mapName = MakeFileSystemCompatible(ai->Getcb()->GetMapName());
		mapName.resize(mapName.size() - 4); // cut off extension
		res += mapName;
	}
	if (maphash) {
		if (!res.empty())
			res += "-";
		res += IntToString(ai->Getcb()->GetMapHash(), "%x");
	}
	if (game) {
		if (!res.empty())
			res += "_";
		res += MakeFileSystemCompatible(ai->Getcb()->GetModHumanName());
	}
	if (gamehash) {
		if (!res.empty())
			res += "-";
		res += IntToString(ai->Getcb()->GetModHash(), "%x");
	}
	return res;
}

AAIConfig *cfg = new AAIConfig();
