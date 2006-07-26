#include "AAIConfig.h"

#include "AAI.h"


AAIConfig::AAIConfig(void)
{
	SIDES = 2;
	SECTOR_SIZE = 100.0;
	MIN_ENERGY = 18;  // min energy make value to be considered beeing a power plant 
	MAX_UNITS = 5000;
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
	MIN_EFFICIENCY = 0.001;
	MAX_BUILDERS = 40;
	MAX_BUILDERS_PER_TYPE = 5;
	MAX_FACTORIES_PER_TYPE = 3;
	MAX_BUILDQUE_SIZE = 16;
	MAX_ASSISTANTS = 4;
	MIN_ASSISTANCE_BUILDTIME = 15;
	MIN_ASSISTANCE_BUILDSPEED = 20;
	MAX_BASE_SIZE = 9;
	SCOUT_SPEED = 95.0;
	MOBILE_ARTY_RANGE = 900.0;
	STATIONARY_ARTY_RANGE = 1500;
	AIR_DEFENCE = 8;
	MIN_ENERGY_STORAGE = 500;
	MIN_METAL_STORAGE = 100;
	MAX_METAL_COST = 10000;
	MIN_AIR_ATTACK_COST = 150;
	MAX_AIR_TARGETS = 20;
	AIRCRAFT_RATE = 6;
	HIGH_RANGE_UNITS_RATE = 5;
	FAST_UNITS_RATE = 5;
	METAL_ENERGY_RATIO = 25;
	MAX_DEFENCES = 12;
	MIN_SECTOR_THREAT = 0.5;
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
	MIN_AIR_SUPPORT_EFFICIENCY = 2.5;
	UNIT_SPEED_SUBGROUPS = 4;
	MIN_SUBMARINE_WATERLINE = 15;

	MAX_COST_LIGHT_ASSAULT = 0.025;
	MAX_COST_MEDIUM_ASSAULT = 0.13;
	MAX_COST_HEAVY_ASSAULT = 0.55;
	
	LIGHT_ASSAULT_RATIO = 40;
	MEDIUM_ASSAULT_RATIO = 30;
	HEAVY_ASSAULT_RATIO = 25;
	SUPER_HEAVY_ASSAULT_RATIO = 5;
	
	LEARN_SPEED = 0.15;
	LEARN_RATE = 5;
	CONSTRUCTION_TIMEOUT = 1500;
	CLIFF_SLOPE = 0.085;
	SCOUT_UPDATE_FREQUENCY = 127;
	WATER_MAP_RATIO = 0.8;
	LAND_WATER_MAP_RATIO = 0.3;
	
	initialized = false;
}

AAIConfig::~AAIConfig(void)
{
	for(int i = 0; i < SIDES; i++) 
	{
		delete [] START_UNITS[i];
		delete [] SIDE_NAMES[i];
	}

	delete [] START_UNITS;
	delete [] SIDE_NAMES;
}

void AAIConfig::LoadConfig(AAI *ai)
{
	char filename[100];
	char buffer[120];
	strcpy(buffer, AI_PATH);
	strcat(buffer, MOD_CFG_PATH);
	strcat(buffer, ai->cb->GetModName());
	ReplaceExtension (buffer, filename, sizeof(filename), ".cfg");
	strcpy(cfg_file, filename);

	FILE *file = fopen(filename, "r");
	char keyword[50];
	int ival;
	float fval;
	
	bool error = false;
	bool loaded = false;

	if(file)
	{
		while(EOF != fscanf(file, "%s", keyword))
		{
			if(!strcmp(keyword,"SIDES"))
			{
				fscanf(file, "%i", &ival);
				SIDES = ival;
			}
			else if(!strcmp(keyword, "START_UNITS"))
			{
				START_UNITS = new char*[SIDES];

				for(int i = 0; i < SIDES; i++)
				{
					START_UNITS[i] = new char[20];
					fscanf(file, "%s", filename);
					strcpy(START_UNITS[i], filename);

					if(!ai->cb->GetUnitDef(START_UNITS[i]))
					{
						fprintf(ai->file, "ERROR: loading starting units - could not find unit %s\n", START_UNITS[i]);
						error = true;
						break;
					}
				}
			}
			else if(!strcmp(keyword, "SIDE_NAMES"))
			{
				SIDE_NAMES = new char*[SIDES];

				for(int i = 0; i < SIDES; i++)
				{
					SIDE_NAMES[i] = new char[80];
					fscanf(file, "%s", filename);
					strcpy(SIDE_NAMES[i], filename);
				}
			}
			else if(!strcmp(keyword, "SCOUTS"))
			{
				// get number of scouts
				fscanf(file, "%i", &ival);

				for(int i = 0; i < ival; i++)
				{
					fscanf(file, "%s", filename);
					if(ai->cb->GetUnitDef(filename))
						SCOUTS.push_back(ai->cb->GetUnitDef(filename)->id);
					else 
					{
						fprintf(ai->file, "ERROR: loading scouts - could not find unit %s\n", filename);
						error = true;
						break;
					}
				}
			}
			else if(!strcmp(keyword,"SECTOR_SIZE"))
			{
				fscanf(file, "%f", &fval);
				SECTOR_SIZE = fval;
				printf("SECTOR_SIZE set to %f", SECTOR_SIZE);
			}
			else if(!strcmp(keyword,"MIN_ENERGY"))
			{
				fscanf(file, "%i", &ival);
				MIN_ENERGY = ival;
			}
			else if(!strcmp(keyword, "MAX_UNITS"))
			{
				fscanf(file, "%i", &ival);
				MAX_UNITS = ival;
			}
			else if(!strcmp(keyword, "MAX_SCOUTS"))
			{
				fscanf(file, "%i", &ival);
				MAX_SCOUTS = ival;
			}
			else if(!strcmp(keyword, "MAX_SECTOR_IMPORTANCE"))
			{
				fscanf(file, "%i", &ival);
				MAX_SECTOR_IMPORTANCE = ival;
			}
			else if(!strcmp(keyword, "MAX_XROW"))
			{
				fscanf(file, "%i", &ival);
				MAX_XROW = ival;
			}
			else if(!strcmp(keyword, "MAX_YROW"))
			{
				fscanf(file, "%i", &ival);
				MAX_YROW = ival;
			}
			else if(!strcmp(keyword, "X_SPACE"))
			{
				fscanf(file, "%i", &ival);
				X_SPACE = ival;
			}
			else if(!strcmp(keyword, "Y_SPACE"))
			{
				fscanf(file, "%i", &ival);
				Y_SPACE = ival;
			}
			else if(!strcmp(keyword, "MAX_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_AIR_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_AIR_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_NAVAL_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_NAVAL_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_SUBMARINE_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_SUBMARINE_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_ANTI_AIR_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_ANTI_AIR_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_ARTY_GROUP_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_ARTY_GROUP_SIZE = ival;
			}
			else if(!strcmp(keyword, "UNIT_SPEED_SUBGROUPS"))
			{
				fscanf(file, "%i", &ival);
				UNIT_SPEED_SUBGROUPS = ival;
			}
			else if(!strcmp(keyword, "MIN_EFFICIENCY"))
			{
				fscanf(file, "%f", &fval);
				MIN_EFFICIENCY = fval;
			}
			else if(!strcmp(keyword, "MIN_AIR_SUPPORT_EFFICIENCY"))
			{
				fscanf(file, "%f", &fval);
				MIN_AIR_SUPPORT_EFFICIENCY = fval;
			}
			else if(!strcmp(keyword, "MAX_BUILDERS"))
			{
				fscanf(file, "%i", &ival);
				MAX_BUILDERS = ival;
			}
			else if(!strcmp(keyword, "MAX_BUILDQUE_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_BUILDQUE_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_ASSISTANTS"))
			{
				fscanf(file, "%i", &ival);
				MAX_ASSISTANTS = ival;
			}
			else if(!strcmp(keyword, "MIN_ASSISTANCE_BUILDSPEED"))
			{
				fscanf(file, "%i", &ival);
				MIN_ASSISTANCE_BUILDSPEED = ival;
			}
			else if(!strcmp(keyword, "MAX_BASE_SIZE"))
			{
				fscanf(file, "%i", &ival);
				MAX_BASE_SIZE = ival;
			}
			else if(!strcmp(keyword, "MAX_AIR_TARGETS"))
			{
				fscanf(file, "%i", &ival);
				MAX_AIR_TARGETS = ival;
			}
			else if(!strcmp(keyword, "MIN_AIR_ATTACK_COST"))
			{
				fscanf(file, "%i", &ival);
				MIN_AIR_ATTACK_COST = ival;
			}
			else if(!strcmp(keyword, "SCOUT_SPEED"))
			{
				fscanf(file, "%f", &fval);
				SCOUT_SPEED = fval;
			}
			else if(!strcmp(keyword, "MOBILE_ARTY_RANGE"))
			{
				fscanf(file, "%f", &fval);
				MOBILE_ARTY_RANGE = fval;
			}
			else if(!strcmp(keyword, "STATIONARY_ARTY_RANGE"))
			{
				fscanf(file, "%f", &fval);
				STATIONARY_ARTY_RANGE = fval;
			}
			else if(!strcmp(keyword, "MAX_BUILDERS_PER_TYPE"))
			{
				fscanf(file, "%i", &ival);
				MAX_BUILDERS_PER_TYPE = ival;
			}
			else if(!strcmp(keyword, "MAX_FACTORIES_PER_TYPE"))
			{
				fscanf(file, "%i", &ival);
				MAX_FACTORIES_PER_TYPE = ival;
			}
			else if(!strcmp(keyword, "MIN_ASSISTANCE_BUILDTIME"))
			{
				fscanf(file, "%i", &ival);
				MIN_ASSISTANCE_BUILDTIME = ival;
			}
			else if(!strcmp(keyword, "AIR_DEFENCE"))
			{
				fscanf(file, "%i", &ival);
				AIR_DEFENCE = ival;
			}
			else if(!strcmp(keyword, "AIRCRAFT_RATE"))
			{
				fscanf(file, "%i", &ival);
				AIRCRAFT_RATE = ival;
			}
			else if(!strcmp(keyword, "HIGH_RANGE_UNITS_RATE"))
			{
				fscanf(file, "%i", &ival);
				HIGH_RANGE_UNITS_RATE = ival;
			}
			else if(!strcmp(keyword, "FAST_UNITS_RATE"))
			{
				fscanf(file, "%i", &ival);
				FAST_UNITS_RATE = ival;
			}
			else if(!strcmp(keyword, "MAX_METAL_COST"))
			{
				fscanf(file, "%i", &ival);
				MAX_METAL_COST = ival;
			}
			else if(!strcmp(keyword, "MAX_DEFENCES"))
			{
				fscanf(file, "%i", &ival);
				MAX_DEFENCES = ival;
			}
			else if(!strcmp(keyword, "MIN_SECTOR_THREAT"))
			{
				fscanf(file, "%f", &fval);
				MIN_SECTOR_THREAT = fval;
			}
			else if(!strcmp(keyword, "MAX_STAT_ARTY"))
			{
				fscanf(file, "%i", &ival);
				MAX_STAT_ARTY = ival;
			}
			else if(!strcmp(keyword, "MAX_AIR_BASE"))
			{
				fscanf(file, "%i", &ival);
				MAX_AIR_BASE = ival;
			}
			else if(!strcmp(keyword, "AIR_ONLY_MOD"))
			{
				fscanf(file, "%i", &ival);
				AIR_ONLY_MOD = (bool)ival;
			}
			else if(!strcmp(keyword, "METAL_ENERGY_RATIO"))
			{
				fscanf(file, "%f", &fval);
				METAL_ENERGY_RATIO = fval;
			}
			else if(!strcmp(keyword, "MAX_METAL_MAKERS"))
			{
				fscanf(file, "%i", &ival);
				MAX_METAL_MAKERS = ival;
			}
			else if(!strcmp(keyword, "MAX_STORAGE"))
			{
				fscanf(file, "%i", &ival);
				MAX_STORAGE = ival;
			}
			else if(!strcmp(keyword, "MIN_METAL_MAKER_ENERGY"))
			{
				fscanf(file, "%f", &fval);
				MIN_METAL_MAKER_ENERGY = fval;
			}
			else if(!strcmp(keyword, "MAX_MEX_DISTANCE"))
			{
				fscanf(file, "%i", &ival);
				MAX_MEX_DISTANCE = ival;
			}
			else if(!strcmp(keyword, "MAX_MEX_DEFENCE_DISTANCE"))
			{
				fscanf(file, "%i", &ival);
				MAX_MEX_DEFENCE_DISTANCE = ival;
			}
			else if(!strcmp(keyword, "MIN_FACTORIES_FOR_DEFENCES"))
			{
				fscanf(file, "%i", &ival);
				MIN_FACTORIES_FOR_DEFENCES = ival;
			}
			else if(!strcmp(keyword, "MIN_FACTORIES_FOR_STORAGE"))
			{
				fscanf(file, "%i", &ival);
				MIN_FACTORIES_FOR_STORAGE = ival;
			}
			else if(!strcmp(keyword, "MIN_FACTORIES_FOR_RADAR_JAMMER"))
			{
				fscanf(file, "%i", &ival);
				MIN_FACTORIES_FOR_RADAR_JAMMER = ival;
			}
			else if(!strcmp(keyword, "MIN_SUBMARINE_WATERLINE"))
			{
				fscanf(file, "%i", &ival);
				MIN_SUBMARINE_WATERLINE = ival;
			}
			else
			{
				error = true;
				break;
			}
		}
		
		if(error)
		{
			fprintf(ai->file, "Mod config file %s contains erroneous keyword %s\n", filename, keyword);
			initialized = false;
			return;
		}
		else
		{
			loaded = true;
			fclose(file);
			fprintf(ai->file, "Mod config file loaded\n");
		}
	}
	else
	{
		fprintf(ai->file, "Mod config file %s not found\n", filename);
		initialized = false;
		return;
	}
	

	// load general settings
	strcpy(buffer, AI_PATH);
	strcat(buffer, GENERAL_CFG_FILE);
	ReplaceExtension (buffer, filename, sizeof(filename), ".cfg");

	file = fopen(filename, "r");

	if(file)
	{
		while(EOF != fscanf(file, "%s", keyword))
		{
			if(!strcmp(keyword, "LEARN_RATE"))
			{
				fscanf(file, "%i", &ival);
				LEARN_RATE = ival;
			}
			else if(!strcmp(keyword, "LEARN_SPEED"))
			{
				fscanf(file, "%f", &fval);
				LEARN_SPEED = fval;
			}
			else if(!strcmp(keyword, "WATER_MAP_RATIO"))
			{
				fscanf(file, "%f", &fval);
				WATER_MAP_RATIO = fval;
			}
			else if(!strcmp(keyword, "LAND_WATER_MAP_RATIO"))
			{
				fscanf(file, "%f", &fval);
				LAND_WATER_MAP_RATIO = fval;
			}
			else if(!strcmp(keyword, "SCOUT_UPDATE_FREQUENCY"))
			{
				fscanf(file, "%i", &ival);
				SCOUT_UPDATE_FREQUENCY = ival;
			}
			else
			{
				error = true;
				break;
			}
		}

		fclose(file);

		if(error)
		{
			fprintf(ai->file, "General config file % contains erroneous keyword %s\n", filename, keyword);
			initialized = false;
			return;
		}

		else
		{
			fprintf(ai->file, "General config file loaded\n");
			initialized = true;
			return;
		}
	}
	else 
	{
		fprintf(ai->file, "General config file %s not found\n", filename);
		initialized = false;
		return;
	}
}

AAIConfig *cfg = new AAIConfig();
