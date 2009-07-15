// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIMap.h"
#include "AAI.h"
#include "AAISector.h"
#include "AAIBuildTable.h"

// all the static vars
int AAIMap::aai_instances = 0;
char AAIMap::map_filename[500];
int AAIMap::xSize;
int AAIMap::ySize;
int AAIMap::xMapSize;
int AAIMap::yMapSize;
int AAIMap::losMapRes;
int AAIMap::xLOSMapSize;
int AAIMap::yLOSMapSize;
int AAIMap::xDefMapSize;
int AAIMap::yDefMapSize;
int AAIMap::xContMapSize;
int AAIMap::yContMapSize;
int AAIMap::xSectors;
int AAIMap::ySectors;
int AAIMap::xSectorSize;
int AAIMap::ySectorSize;
int AAIMap::xSectorSizeMap;
int AAIMap::ySectorSizeMap;

list<AAIMetalSpot>  AAIMap::metal_spots;

int AAIMap::land_metal_spots;
int AAIMap::water_metal_spots;

float AAIMap::land_ratio;
float AAIMap::flat_land_ratio;
float AAIMap::water_ratio;

bool AAIMap::metalMap;
MapType AAIMap::map_type;

vector< vector<int> > AAIMap::team_sector_map;
vector<int> AAIMap::buildmap;
vector<int> AAIMap::blockmap;
vector<float> AAIMap::plateau_map;
vector<int> AAIMap::continent_map;

vector<AAIContinent> AAIMap::continents;
int AAIMap::land_continents;
int AAIMap::water_continents;
int AAIMap::avg_land_continent_size;
int AAIMap::avg_water_continent_size;
int AAIMap::max_land_continent_size;
int AAIMap::max_water_continent_size;
int AAIMap::min_land_continent_size;
int AAIMap::min_water_continent_size;
list<UnitCategory> AAIMap::map_categories;
list<int> AAIMap::map_categories_id;

AAIMap::AAIMap(AAI *ai)
{
	// initialize random numbers generator
	srand ( time(NULL) );

	this->ai = ai;
	bt = ai->bt;
	cb = ai->cb;

	initialized = false;
}

AAIMap::~AAIMap(void)
{
	--aai_instances;

	// delete common data only if last aai instace has gone
	if(aai_instances == 0)
	{
		Learn();

		// save map data
		FILE *save_file = fopen(map_filename, "w+");

		fprintf(save_file, "%s \n", MAP_LEARN_VERSION);

		for(int y = 0; y < ySectors; y++)
		{
			for(int x = 0; x < xSectors; x++)
			{
				// save sector data
				fprintf(save_file, "%f %f %f", sector[x][y].flat_ratio, sector[x][y].water_ratio, sector[x][y].importance_this_game);
				// save combat data
				for(size_t cat = 0; cat < bt->assault_categories.size(); ++cat)
					fprintf(save_file, "%f %f ", sector[x][y].attacked_by_this_game[cat], sector[x][y].combats_this_game[cat]);
			}

			fprintf(save_file, "\n");
		}

		fclose(save_file);

		buildmap.clear();
		blockmap.clear();
		plateau_map.clear();
		continent_map.clear();
	}

	defence_map.clear();
	air_defence_map.clear();
	submarine_defence_map.clear();

	scout_map.clear();
	last_updated_map.clear();

	sector_in_los.clear();
	sector_in_los_with_enemies.clear();

	units_in_los.clear();
	enemy_combat_units_spotted.clear();
}

void AAIMap::Init()
{
	++aai_instances;

	// all static vars are only initialized by the first aai instance
	if(aai_instances == 1)
	{
		// get size
		xMapSize = cb->GetMapWidth();
		yMapSize = cb->GetMapHeight();

		xSize = xMapSize * SQUARE_SIZE;
		ySize = yMapSize * SQUARE_SIZE;

		losMapRes = cb->GetLosMapResolution();
		xLOSMapSize = xMapSize / losMapRes;
		yLOSMapSize = yMapSize / losMapRes;

		xDefMapSize = xMapSize / 4;
		yDefMapSize = yMapSize / 4;

		xContMapSize = xMapSize / 4;
		yContMapSize = yMapSize / 4;

		// calculate number of sectors
		xSectors = floor(0.5f + ((float) xMapSize)/cfg->SECTOR_SIZE);
		ySectors = floor(0.5f + ((float) yMapSize)/cfg->SECTOR_SIZE);

		// calculate effective sector size
		xSectorSizeMap = floor( ((float) xMapSize) / ((float) xSectors) );
		ySectorSizeMap = floor( ((float) yMapSize) / ((float) ySectors) );

		xSectorSize = 8 * xSectorSizeMap;
		ySectorSize = 8 * ySectorSizeMap;

		buildmap.resize(xMapSize*yMapSize, 0);
		blockmap.resize(xMapSize*yMapSize, 0);
		continent_map.resize(xContMapSize*yContMapSize, -1);
		plateau_map.resize(xContMapSize*yContMapSize, 0);

		// create map that stores which aai player has occupied which sector (visible to all aai players)
		team_sector_map.resize(xSectors);

		for(int x = 0; x < xSectors; ++x)
			team_sector_map[x].resize(ySectors, -1);

		ReadContinentFile();

		ReadMapCacheFile();
	}

	// create field of sectors
	sector.resize(xSectors);

	for(int x = 0; x < xSectors; ++x)
		sector[x].resize(ySectors);

	for(int j = 0; j < ySectors; ++j)
	{
		for(int i = 0; i < xSectors; ++i)
			// provide ai callback to sectors & set coordinates of the sectors
			sector[i][j].Init(ai, i, j, xSectorSize*i, xSectorSize*(i+1), ySectorSize * j, ySectorSize * (j+1));
	}

	// add metalspots to their sectors
	int k, l;
	for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); ++spot)
	{
		k = spot->pos.x/xSectorSize;
		l = spot->pos.z/ySectorSize;

		if(k < xSectors && l < ySectors)
			sector[k][l].AddMetalSpot(&(*spot));
	}

	ReadMapLearnFile(true);

	// for scouting
	scout_map.resize(xLOSMapSize*yLOSMapSize, 0);
	last_updated_map.resize(xLOSMapSize*yLOSMapSize, 0);

	sector_in_los.resize( (xSectors+1) * (ySectors+1) );
	sector_in_los_with_enemies.resize( (xSectors+1) * (ySectors+1) );

	units_in_los.resize(cfg->MAX_UNITS, 0);

	enemy_combat_units_spotted.resize(bt->ass_categories, 0);

	// create defence
	defence_map.resize(xDefMapSize*yDefMapSize, 0);
	air_defence_map.resize(xDefMapSize*yDefMapSize, 0);
	submarine_defence_map.resize(xDefMapSize*yDefMapSize, 0);

	initialized = true;

	// for log file
	fprintf(ai->file, "Map: %s\n",cb->GetMapName());
	fprintf(ai->file, "Maptype: %s\n", GetMapTypeTextString(map_type));
	fprintf(ai->file, "Mapsize is %i x %i\n", cb->GetMapWidth(),cb->GetMapHeight());
	fprintf(ai->file, "%i sectors in x direction\n", xSectors);
	fprintf(ai->file, "%i sectors in y direction\n", ySectors);
	fprintf(ai->file, "x-sectorsize is %i (Map %i)\n", xSectorSize, xSectorSizeMap);
	fprintf(ai->file, "y-sectorsize is %i (Map %i)\n", ySectorSize, ySectorSizeMap);
	fprintf(ai->file, "%i metal spots found (%i are on land, %i under water) \n \n",metal_spots.size(), land_metal_spots, water_metal_spots);
	fprintf(ai->file, "%i continents found on map\n", continents.size());
	fprintf(ai->file, "%i land and %i water continents\n", land_continents, water_continents);
	fprintf(ai->file, "Average land continent size is %i\n", avg_land_continent_size);
	fprintf(ai->file, "Average water continent size is %i\n", avg_water_continent_size);

	//debug
	/*float3 my_pos;
	for(int x = 0; x < xMapSize; x+=2)
	{
		for(int y = 0; y < yMapSize; y+=2)
		{
			if(buildmap[x + y*xMapSize] == 1 || buildmap[x + y*xMapSize] == 5)
			{
				my_pos.x = x * 8;
				my_pos.z = y * 8;
				my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
				cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 8000, cb->GetMyAllyTeam(), true, true);
			}
		}
	}*/
}

void AAIMap::ReadMapCacheFile()
{
	// try to read cache file
	bool loaded = false;

	static const size_t buffer_sizeMax = 500;
	char buffer[buffer_sizeMax];
	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, MAP_CACHE_PATH);
	STRCAT(buffer, cb->GetMapName());
	ReplaceExtension(buffer, map_filename, sizeof(map_filename), ".dat");

	// as we will have to write to the file later on anyway,
	// we want it writeable
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, map_filename);

	FILE *file;

	if((file = fopen(map_filename, "r")) != NULL)
	{
		// check if correct version
		fscanf(file, "%s ", buffer);

		if(strcmp(buffer, MAP_CACHE_VERSION))
		{
			cb->SendTextMsg("Mapcache out of date - creating new one", 0);
			fprintf(ai->file, "Map cache file out of date - creating new one\n");
			fclose(file);
		}
		else
		{
			int temp;
			float temp_float;

			// load if its a metal map
			fscanf(file, "%i ", &temp);
			metalMap = (bool)temp;

			// load map type
			fscanf(file, "%s ", buffer);

			if(!strcmp(buffer, "LAND_MAP"))
				map_type = LAND_MAP;
			else if(!strcmp(buffer, "LAND_WATER_MAP"))
				map_type = LAND_WATER_MAP;
			else if(!strcmp(buffer, "WATER_MAP"))
				map_type = WATER_MAP;
			else
				map_type = UNKNOWN_MAP;

			SNPRINTF(buffer, buffer_sizeMax, "%s detected", GetMapTypeTextString(map_type));
			ai->cb->SendTextMsg(buffer, 0);

			// load water ratio
			fscanf(file, "%f ", &water_ratio);

			// load buildmap
			for(int i = 0; i < xMapSize*yMapSize; ++i)
			{
				fscanf(file, "%i ", &temp);
				buildmap[i] = temp;
			}

			// load plateau map
			for(int i = 0; i < xMapSize*yMapSize/16; ++i)
			{
				fscanf(file, "%f ", &temp_float);
				plateau_map[i] = temp_float;
			}

			// load mex spots
			AAIMetalSpot spot;
			fscanf(file, "%i ", &temp);

			for(int i = 0; i < temp; ++i)
			{
				fscanf(file, "%f %f %f %f ", &(spot.pos.x), &(spot.pos.y), &(spot.pos.z), &(spot.amount));
				spot.occupied = false;
				metal_spots.push_back(spot);
			}

			fscanf(file, "%i %i ", &land_metal_spots, &water_metal_spots);

			fclose(file);

			fprintf(ai->file, "Map cache file succesfully loaded\n");

			loaded = true;
		}
	}

	if(!loaded)  // create new map data
	{
		// look for metalspots
		SearchMetalSpots();

		CalculateWaterRatio();

		// detect cliffs/water and create plateau map
		AnalyseMap();

		DetectMapType();

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// save mod independent map data
		STRCPY(buffer, MAIN_PATH);
		STRCAT(buffer, MAP_CACHE_PATH);
		STRCAT(buffer, cb->GetMapName());
		ReplaceExtension(buffer, map_filename, sizeof(map_filename), ".dat");

		ai->cb->GetValue(AIVAL_LOCATE_FILE_W, map_filename);

		file = fopen(map_filename, "w+");

		fprintf(file, "%s\n", MAP_CACHE_VERSION);

		// save if its a metal map
		fprintf(file, "%i\n", (int)metalMap);

		const char *temp_buffer = GetMapTypeString(map_type);

		// save map type
		fprintf(file, "%s\n", temp_buffer);

		// save water ratio
		fprintf(file, "%f\n", water_ratio);

		// save buildmap
		for(int i = 0; i < xMapSize*yMapSize; ++i)
			fprintf(file, "%i ", buildmap[i]);

		fprintf(file, "\n");

		// save plateau map
		for(int i = 0; i < xMapSize*yMapSize/16; ++i)
			fprintf(file, "%f ", plateau_map[i]);

		// save mex spots
		land_metal_spots = 0;
		water_metal_spots = 0;

		fprintf(file, "\n%i \n", metal_spots.size());

		for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); ++spot)
		{
			fprintf(file, "%f %f %f %f \n", spot->pos.x, spot->pos.y, spot->pos.z, spot->amount);

			if(spot->pos.y >= 0)
				++land_metal_spots;
			else
				++water_metal_spots;
		}

		fprintf(file, "%i %i\n", land_metal_spots, water_metal_spots);

		fclose(file);

		fprintf(ai->file, "New map cache-file created\n");
	}


	/*char filename[500];

	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, MAP_CFG_PATH);
	STRCAT(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".cfg");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

	if((file = fopen(filename, "r")) != NULL)
	{
		// read map type
		fscanf(file, "%s ", buffer);

		if(!strcmp(buffer, "LAND_MAP"))
			map_type = LAND_MAP;
		else if(!strcmp(buffer, "LAND_WATER_MAP"))
			map_type = LAND_WATER_MAP;
		else if(!strcmp(buffer, "WATER_MAP"))
			map_type = WATER_MAP;
		else
			map_type = UNKNOWN_MAP;

		if(map_type >= 0 && map_type <= WATER_MAP)
		{
			this->map_type = (MapType) map_type;

			SNPRINTF(buffer, buffer_sizeMax, "%s detected", GetMapTypeTextString(map_type));

			if(bt->aai_instances == 1)
				ai->cb->SendTextMsg(buffer, 0);
		}
		else
			loaded = false;

		fclose(file);
	}
	else
		loaded = false;

	if(!loaded)
	{

		// logging
		SNPRINTF(buffer, buffer_sizeMax, "%s detected", GetMapTypeTextString(map_type));
		ai->cb->SendTextMsg(buffer, 0);
		fprintf(ai->file, "\nAutodetecting map type:\n");
		fprintf(ai->file, "%s", buffer);
		fprintf(ai->file, "\n\n");

		// save results to cfg file
		STRCPY(buffer, MAIN_PATH);
		STRCAT(buffer, MAP_CFG_PATH);
		STRCAT(buffer, cb->GetMapName());
		ReplaceExtension(buffer, map_filename, sizeof(map_filename), ".cfg");

		ai->cb->GetValue(AIVAL_LOCATE_FILE_W, map_filename);

		file = fopen(map_filename, "w+");
		fprintf(file, "%s\n", GetMapTypeString(this->map_type));
		fclose(file);
	}*/

	// determine important unit categories on this map
	if(cfg->AIR_ONLY_MOD)
	{
		map_categories.push_back(GROUND_ASSAULT);
		map_categories.push_back(AIR_ASSAULT);
		map_categories.push_back(HOVER_ASSAULT);
		map_categories.push_back(SEA_ASSAULT);

		map_categories_id.push_back(0);
		map_categories_id.push_back(1);
		map_categories_id.push_back(2);
		map_categories_id.push_back(3);
	}
	else
	{
		if(map_type == LAND_MAP)
		{
			map_categories.push_back(GROUND_ASSAULT);
			map_categories.push_back(AIR_ASSAULT);
			map_categories.push_back(HOVER_ASSAULT);

			map_categories_id.push_back(0);
			map_categories_id.push_back(1);
			map_categories_id.push_back(2);
		}
		else if(map_type == LAND_WATER_MAP)
		{
			map_categories.push_back(GROUND_ASSAULT);
			map_categories.push_back(AIR_ASSAULT);
			map_categories.push_back(HOVER_ASSAULT);
			map_categories.push_back(SEA_ASSAULT);
			map_categories.push_back(SUBMARINE_ASSAULT);

			map_categories_id.push_back(0);
			map_categories_id.push_back(1);
			map_categories_id.push_back(2);
			map_categories_id.push_back(3);
			map_categories_id.push_back(4);
		}
		else if(map_type == WATER_MAP)
		{
			map_categories.push_back(AIR_ASSAULT);
			map_categories.push_back(HOVER_ASSAULT);
			map_categories.push_back(SEA_ASSAULT);
			map_categories.push_back(SUBMARINE_ASSAULT);

			map_categories_id.push_back(1);
			map_categories_id.push_back(2);
			map_categories_id.push_back(3);
			map_categories_id.push_back(4);
		}
		else
		{
			map_categories.push_back(AIR_ASSAULT);

			map_categories_id.push_back(1);
		}
	}
}

void AAIMap::ReadContinentFile()
{
	static const size_t buffer_sizeMax = 500;
	char buffer[buffer_sizeMax];
	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, MAP_CACHE_PATH);
	STRCAT(buffer, cb->GetMapName());
	STRCAT(buffer, "_");
	STRCAT(buffer, cb->GetModName());
	char filename[buffer_sizeMax];
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	// as we will have to write to the file later on anyway,
	// we want it writeable
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	FILE* file = fopen(filename, "r");

	if(file != NULL)
	{
		// check if correct version
		fscanf(file, "%s ", buffer);

		if(strcmp(buffer, CONTINENT_DATA_VERSION))
		{
			cb->SendTextMsg("Continent cache out of date - creating new one", 0);
			fprintf(ai->file, "Continent cache-file out of date - new one has been created\n");
			fclose(file);
		}
		else
		{
			int temp, temp2;

			// load continent map
			for(int j = 0; j < yContMapSize; ++j)
			{
				for(int i = 0; i < xContMapSize; ++i)
				{
					fscanf(file, "%i ", &temp);
					continent_map[j * xContMapSize + i] = temp;
				}
			}

			// load continents
			fscanf(file, "%i ", &temp);

			continents.resize(temp);

			for(int i = 0; i < temp; ++i)
			{
				fscanf(file, "%i %i ", &continents[i].size, &temp2);

				continents[i].water = (bool) temp2;
				continents[i].id = i;
			}

			// load statistical data
			fscanf(file, "%i %i %i %i %i %i %i %i", &land_continents, &water_continents, &avg_land_continent_size, &avg_water_continent_size,
																			&max_land_continent_size, &max_water_continent_size,
																			&min_land_continent_size, &min_water_continent_size);

			fclose(file);

			fprintf(ai->file, "Continent cache file succesfully loaded\n");

			return;
		}
	}


	// loading has not been succesful -> create new continent maps

	// create continent/movement map
	CalculateContinentMaps();

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// save movement maps
	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, MAP_CACHE_PATH);
	STRCAT(buffer, cb->GetMapName());
	STRCAT(buffer, "_");
	STRCAT(buffer, cb->GetModName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	file = fopen(filename, "w+");

	fprintf(file, "%s\n",  CONTINENT_DATA_VERSION);

	// save continent map
	for(int j = 0; j < yContMapSize; ++j)
	{
		for(int i = 0; i < xContMapSize; ++i)
			fprintf(file, "%i ", continent_map[j * xContMapSize + i]);

		fprintf(file, "\n");
	}

	// save continents
	fprintf(file, "\n%i \n", continents.size());

	for(size_t c = 0; c < continents.size(); ++c)
		fprintf(file, "%i %i \n", continents[c].size, (int)continents[c].water);

	// save statistical data
	fprintf(file, "%i %i %i %i %i %i %i %i\n", land_continents, water_continents, avg_land_continent_size, avg_water_continent_size,
																	max_land_continent_size, max_water_continent_size,
																	min_land_continent_size, min_water_continent_size);

	fclose(file);
}

void AAIMap::ReadMapLearnFile(bool auto_set)
{
	// get filename
	static const size_t buffer_sizeMax = 500;
	char buffer[buffer_sizeMax];

	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, MAP_LEARN_PATH);
	STRCAT(buffer, cb->GetMapName());
	ReplaceExtension(buffer, map_filename, sizeof(map_filename), "_");
	STRCPY(buffer, map_filename);
	STRCAT(buffer, cb->GetModName());
	ReplaceExtension(buffer, map_filename, sizeof(map_filename), ".dat");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, map_filename);

	// open learning files
	FILE *load_file = fopen(map_filename, "r");

	// check if correct map file version
	if(load_file)
	{
		fscanf(load_file, "%s", buffer);

		// file version out of date
		if(strcmp(buffer, MAP_LEARN_VERSION))
		{
			cb->SendTextMsg("Map learning file version out of date, creating new one", 0);
			fclose(load_file);
			load_file = 0;
		}
	}

	// load sector data from file or init with default values
	if(load_file)
	{
		for(int j = 0; j < ySectors; ++j)
		{
			for(int i = 0; i < xSectors; ++i)
			{
				// load sector data
				fscanf(load_file, "%f %f %f", &sector[i][j].flat_ratio, &sector[i][j].water_ratio, &sector[i][j].importance_learned);

				// set movement types that may enter this sector
				// always: MOVE_TYPE_AIR, MOVE_TYPE_AMPHIB, MOVE_TYPE_HOVER;
				sector[i][j].allowed_movement_types = 22;

				if(sector[i][j].water_ratio < 0.3)
					sector[i][j].allowed_movement_types |= MOVE_TYPE_GROUND;
				else if(sector[i][j].water_ratio < 0.7)
				{
					sector[i][j].allowed_movement_types |= MOVE_TYPE_GROUND;
					sector[i][j].allowed_movement_types |= MOVE_TYPE_SEA;
				}
				else
					sector[i][j].allowed_movement_types |= MOVE_TYPE_SEA;

				if(sector[i][j].importance_learned <= 1)
					sector[i][j].importance_learned += (rand()%5)/20.0;

				// load combat data
				for(size_t cat = 0; cat < bt->assault_categories.size(); cat++)
					fscanf(load_file, "%f %f ", &sector[i][j].attacked_by_learned[cat], &sector[i][j].combats_learned[cat]);

				if(auto_set)
				{
					sector[i][j].importance_this_game = sector[i][j].importance_learned;

					for(size_t cat = 0; cat < bt->assault_categories.size(); ++cat)
					{
						sector[i][j].attacked_by_this_game[cat] = sector[i][j].attacked_by_learned[cat];
						sector[i][j].combats_this_game[cat] = sector[i][j].combats_learned[cat];
					}
				}
			}
		}
	}
	else
	{
		for(int j = 0; j < ySectors; ++j)
		{
			for(int i = 0; i < xSectors; ++i)
			{
				sector[i][j].importance_learned = 1 + (rand()%5)/20.0;
				sector[i][j].flat_ratio = sector[i][j].GetFlatRatio();
				sector[i][j].water_ratio = sector[i][j].GetWaterRatio();

				// set movement types that may enter this sector
				// always: MOVE_TYPE_AIR, MOVE_TYPE_AMPHIB, MOVE_TYPE_HOVER;
				sector[i][j].allowed_movement_types = 22;

				if(sector[i][j].water_ratio < 0.3)
					sector[i][j].allowed_movement_types |= MOVE_TYPE_GROUND;
				else if(sector[i][j].water_ratio < 0.7)
				{
					sector[i][j].allowed_movement_types |= MOVE_TYPE_GROUND;
					sector[i][j].allowed_movement_types |= MOVE_TYPE_SEA;
				}
				else
					sector[i][j].allowed_movement_types |= MOVE_TYPE_SEA;

				if(auto_set)
				{
					sector[i][j].importance_this_game = sector[i][j].importance_learned;

					for(size_t cat = 0; cat < bt->assault_categories.size(); ++cat)
					{
						// init with higher values in the center of the map
						sector[i][j].attacked_by_learned[cat] = 2 * sector[i][j].GetEdgeDistance();

						sector[i][j].attacked_by_this_game[cat] = sector[i][j].attacked_by_learned[cat];
						sector[i][j].combats_this_game[cat] = sector[i][j].combats_learned[cat];
					}
				}
			}
		}
	}

	// determine land/water ratio of total map
	flat_land_ratio = 0;
	water_ratio = 0;

	for(int j = 0; j < ySectors; ++j)
	{
		for(int i = 0; i < xSectors; ++i)
		{
			flat_land_ratio += sector[i][j].flat_ratio;
			water_ratio += sector[i][j].water_ratio;
		}
	}

	flat_land_ratio /= (float)(xSectors * ySectors);
	water_ratio /= (float)(xSectors * ySectors);
	land_ratio = 1.0f - water_ratio;

	if(load_file)
		fclose(load_file);
	else
		cb->SendTextMsg("New map-learning file created", 0);
}

void AAIMap::Learn()
{
	AAISector *sector;

	for(int y = 0; y < ySectors; ++y)
	{
		for(int x = 0; x < xSectors; ++x)
		{
			sector = &this->sector[x][y];

			sector->importance_this_game = 0.93f * (sector->importance_this_game + 3.0f * sector->importance_learned)/4.0f;

			if(sector->importance_this_game < 1)
				sector->importance_this_game = 1;

			for(size_t cat = 0; cat < bt->assault_categories.size(); ++cat)
			{
				sector->attacked_by_this_game[cat] = 0.90f * (sector->attacked_by_this_game[cat] + 3.0f * sector->attacked_by_learned[cat])/4.0f;

				sector->combats_this_game[cat] = 0.90f * (sector->combats_this_game[cat] + 3.0f * sector->combats_learned[cat])/4.0f;
			}
		}
	}
}

// converts unit positions to cell coordinates
void AAIMap::Pos2BuildMapPos(float3 *pos, const UnitDef* def)
{
	// get cell index of middlepoint
	pos->x = (int) (pos->x/SQUARE_SIZE);
	pos->z = (int) (pos->z/SQUARE_SIZE);

	// shift to the leftmost uppermost cell
	pos->x -= def->xsize/2;
	pos->z -= def->zsize/2;

	// check if pos is still in that map, otherwise retun 0
	if(pos->x < 0 && pos->z < 0)
		pos->x = pos->z = 0;
}

void AAIMap::BuildMapPos2Pos(float3 *pos, const UnitDef *def)
{
	// shift to middlepoint
	pos->x += def->xsize/2;
	pos->z += def->zsize/2;

	// back to unit coordinates
	pos->x *= SQUARE_SIZE;
	pos->z *= SQUARE_SIZE;
}

void AAIMap::Pos2FinalBuildPos(float3 *pos, const UnitDef* def)
{
	if(def->xsize&2) // check if xsize is a multiple of 4
		pos->x=floor((pos->x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos->x=floor((pos->x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(def->zsize&2)
		pos->z=floor((pos->z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos->z=floor((pos->z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
}

bool AAIMap::SetBuildMap(int xPos, int yPos, int xSize, int ySize, int value, int ignore_value)
{
	//float3 my_pos;

	if(xPos+xSize <= xMapSize && yPos+ySize <= yMapSize)
	{
		for(int x = xPos; x < xSize+xPos; x++)
		{
			for(int y = yPos; y < ySize+yPos; y++)
			{
				if(buildmap[x+y*xMapSize] != ignore_value)
				{
					buildmap[x+y*xMapSize] = value;

					// debug
					/*if(x%2 == 0 && y%2 == 0)
					{
						my_pos.x = x * 8;
						my_pos.z = y * 8;
						my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
						cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 1500, cb->GetMyAllyTeam(), true, true);
					}*/
				}
			}
		}
		return true;
	}
	return false;
}

float3 AAIMap::GetBuildSiteInRect(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water)
{
	float3 pos;

	// get required cell-size of the building
	int xSize, ySize, xPos, yPos;
	GetSize(def, &xSize, &ySize);

	// check rect
	for(yPos = yStart; yPos < yEnd; yPos += 2)
	{
		for(xPos = xStart; xPos < xEnd; xPos += 2)
		{
			// check if buildmap allows construction
			if(CanBuildAt(xPos, yPos, xSize, ySize, water))
			{
				if(bt->IsFactory(def->id))
					yPos += 8;

				pos.x = xPos;
				pos.z = yPos;

				// buildmap allows construction, now check if otherwise blocked
				BuildMapPos2Pos(&pos, def);
				Pos2FinalBuildPos(&pos, def);

				if(ai->cb->CanBuildAt(def, pos))
				{
					int x = pos.x/xSectorSize;
					int y = pos.z/ySectorSize;

					if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
						return pos;
				}
			}
		}
	}

	return ZeroVector;
}

float3 AAIMap::GetRadarArtyBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, float range, bool water)
{
	float3 pos;
	float3 best_pos = ZeroVector;

	float my_rating;
	float best_rating = -10000.0f;

	// convert range from unit coordinates to build map coordinates
	range /= 8.0f;

	// get required cell-size of the building
	int xSize, ySize, xPos, yPos;
	GetSize(def, &xSize, &ySize);

	// go through rect
	for(yPos = yStart; yPos < yEnd; yPos += 2)
	{
		for(xPos = xStart; xPos < xEnd; xPos += 2)
		{
			if(CanBuildAt(xPos, yPos, xSize, ySize, water))
			{
				if(water)
					my_rating = 1.0f + 0.01f * (float)(rand()%100) - range / (float)(1 + GetEdgeDistance(xPos, yPos));
				else
					my_rating = 0.01f * (float)(rand()%50) + plateau_map[xPos + yPos * xSize] - range / (float)(1 + GetEdgeDistance(xPos, yPos));

				if(my_rating > best_rating)
				{
					pos.x = xPos;
					pos.z = yPos;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&pos, def);
					Pos2FinalBuildPos(&pos, def);

					if(ai->cb->CanBuildAt(def, pos))
					{
						best_pos = pos;
						best_rating = my_rating;
					}
				}
			}
		}
	}

	return best_pos;
}

float3 AAIMap::GetHighestBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd)
{
	float3 best_pos = ZeroVector, pos;

	// get required cell-size of the building
	int xSize, ySize, xPos, yPos, x, y;
	GetSize(def, &xSize, &ySize);

	// go through rect
	for(xPos = xStart; xPos < xEnd; xPos += 2)
	{
		for(yPos = yStart; yPos < yEnd; yPos += 2)
		{
			if(CanBuildAt(xPos, yPos, xSize, ySize))
			{
				pos.x = xPos;
				pos.z = yPos;

				// buildmap allows construction, now check if otherwise blocked
				BuildMapPos2Pos(&pos, def);
				Pos2FinalBuildPos(&pos, def);

				if(ai->cb->CanBuildAt(def, pos))
				{
					x = pos.x/xSectorSize;
					y = pos.z/ySectorSize;

					if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
					{
						pos.y = cb->GetElevation(pos.x, pos.z);

						if(pos.y > best_pos.y)
							best_pos = pos;
					}
				}
			}
		}
	}

	return best_pos;
}

float3 AAIMap::GetCenterBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water)
{
	float3 pos, temp_pos;
	bool vStop = false, hStop = false;
	int vCenter = yStart + (yEnd-yStart)/2;
	int hCenter = xStart + (xEnd-xStart)/2;
	int hIterator = 1, vIterator = 1;

	// get required cell-size of the building
	int xSize, ySize;
	GetSize(def, &xSize, &ySize);

	// check rect
	while(!vStop || !hStop)
	{

		pos.z = vCenter - vIterator;
		pos.x = hCenter - hIterator;

		if(!vStop)
		{
			while(pos.x < hCenter+hIterator)
			{
				// check if buildmap allows construction
				if(CanBuildAt(pos.x, pos.z, xSize, ySize, water))
				{
					temp_pos.x = pos.x;
					temp_pos.y = 0;
					temp_pos.z = pos.z;

					if(bt->IsFactory(def->id))
						temp_pos.z += 8;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&temp_pos, def);
					Pos2FinalBuildPos(&temp_pos, def);

					if(ai->cb->CanBuildAt(def, temp_pos))
					{
						int	x = temp_pos.x/xSectorSize;
						int	y = temp_pos.z/ySectorSize;

						if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
							return temp_pos;
					}

				}
				else if(CanBuildAt(pos.x, pos.z + 2 * vIterator, xSize, ySize, water))
				{
					temp_pos.x = pos.x;
					temp_pos.y = 0;
					temp_pos.z = pos.z + 2 * vIterator;

					if(bt->IsFactory(def->id))
						temp_pos.z += 8;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&temp_pos, def);
					Pos2FinalBuildPos(&temp_pos, def);

					if(ai->cb->CanBuildAt(def, temp_pos))
					{
						int x = temp_pos.x/xSectorSize;
						int y = temp_pos.z/ySectorSize;

						if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
							return temp_pos;
					}
				}

				pos.x += 2;
			}
		}

		if (!hStop)
		{
			hIterator += 2;

			if (hCenter - hIterator < xStart || hCenter + hIterator > xEnd)
			{
				hStop = true;
				hIterator -= 2;
			}
		}

		if(!hStop)
		{
			while(pos.z < vCenter+vIterator)
			{
				// check if buildmap allows construction
				if(CanBuildAt(pos.x, pos.z, xSize, ySize, water))
				{
					temp_pos.x = pos.x;
					temp_pos.y = 0;
					temp_pos.z = pos.z;

					if(bt->IsFactory(def->id))
						temp_pos.z += 8;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&temp_pos, def);
					Pos2FinalBuildPos(&temp_pos, def);

					if(ai->cb->CanBuildAt(def, temp_pos))
					{
						int x = temp_pos.x/xSectorSize;
						int y = temp_pos.z/ySectorSize;

						if(x < xSectors || x  >= 0 || y < ySectors || y >= 0)
							return temp_pos;
					}
				}
				else if(CanBuildAt(pos.x + 2 * hIterator, pos.z, xSize, ySize, water))
				{
					temp_pos.x = pos.x + 2 * hIterator;
					temp_pos.y = 0;
					temp_pos.z = pos.z;

					if(bt->IsFactory(def->id))
						temp_pos.z += 8;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&temp_pos, def);
					Pos2FinalBuildPos(&temp_pos, def);

					if(ai->cb->CanBuildAt(def, temp_pos))
					{
						int x = temp_pos.x/xSectorSize;
						int y = temp_pos.z/ySectorSize;

						if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
							return temp_pos;
					}
				}

				pos.z += 2;
			}
		}

		vIterator += 2;

		if(vCenter - vIterator < yStart || vCenter + vIterator > yEnd)
			vStop = true;
	}

	return ZeroVector;
}

float3 AAIMap::GetRandomBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, int tries, bool water)
{
	float3 pos;

	// get required cell-size of the building
	int xSize, ySize;
	GetSize(def, &xSize, &ySize);

	for(int i = 0; i < tries; i++)
	{

		// get random pos within rectangle
		if(xEnd - xStart - xSize < 1)
			pos.x = xStart;
		else
			pos.x = xStart + rand()%(xEnd - xStart - xSize);

		if(yEnd - yStart - ySize < 1)
			pos.z = yStart;
		else
			pos.z = yStart + rand()%(yEnd - yStart - ySize);

		// check if buildmap allows construction
		if(CanBuildAt(pos.x, pos.z, xSize, ySize, water))
		{
			if(bt->IsFactory(def->id))
				pos.z += 8;

			// buildmap allows construction, now check if otherwise blocked
			BuildMapPos2Pos(&pos, def);
			Pos2FinalBuildPos(&pos, def);

			if(ai->cb->CanBuildAt(def, pos))
			{
				int x = pos.x/xSectorSize;
				int y = pos.z/ySectorSize;

				if(x < xSectors && x  >= 0 && y < ySectors && y >= 0)
					return pos;
			}
		}
	}

	return ZeroVector;
}

float3 AAIMap::GetClosestBuildsite(const UnitDef *def, float3 pos, int max_distance, bool water)
{
	Pos2BuildMapPos(&pos, def);

	int xStart = pos.x - max_distance;
	int xEnd = pos.x + max_distance;
	int yStart = pos.z - max_distance;
	int yEnd = pos.z + max_distance;

	if(xStart < 0)
		xStart = 0;

	if(xEnd >= xSectors * xSectorSizeMap)
		xEnd = xSectors * xSectorSizeMap - 1;

	if(yStart < 0)
		yStart = 0;

	if(yEnd >= ySectors * ySectorSizeMap)
		yEnd = ySectors * ySectorSizeMap - 1;

	return GetCenterBuildsite(def, xStart, xEnd, yStart, yEnd, water);
}

bool AAIMap::CanBuildAt(int xPos, int yPos, int xSize, int ySize, bool water)
{
	if(xPos+xSize <= xMapSize && yPos+ySize <= yMapSize)
	{
		// check if all squares the building needs are empty
		for(int x = xPos; x < xSize+xPos; ++x)
		{
			for(int y = yPos; y < ySize+yPos; ++y)
			{
				// check if cell already blocked by something
				if(!water && buildmap[x+y*xMapSize] != 0)
					return false;
				else if(water && buildmap[x+y*xMapSize] != 4)
					return false;
			}
		}
		return true;
	}
	else
		return false;
}

void AAIMap::CheckRows(int xPos, int yPos, int xSize, int ySize, bool add, bool water)
{
	bool insert_space;
	int cell;
	int building;

	if(water)
		building = 5;
	else
		building = 1;

	// check horizontal space
	if(xPos+xSize+cfg->MAX_XROW <= xMapSize && xPos - cfg->MAX_XROW >= 0)
	{
		for(int y = yPos; y < yPos + ySize; ++y)
		{
			if(y >= yMapSize)
			{
				fprintf(ai->file, "ERROR: y = %i index out of range when checking horizontal rows", y);
				return;
			}

			// check to the right
			insert_space = true;
			for(int x = xPos+xSize; x < xPos+xSize+cfg->MAX_XROW; x++)
			{
				if(buildmap[x+y*xMapSize] != building)
				{
					insert_space = false;
					break;
				}
			}

			// check to the left
			if(!insert_space)
			{
				insert_space = true;

				for(int x = xPos-1; x >= xPos - cfg->MAX_XROW; x--)
				{
					if(buildmap[x+y*xMapSize] != building)
					{
						insert_space = false;
						break;
					}
				}
			}

			if(insert_space)
			{
				// right side
				cell = GetNextX(1, xPos+xSize, y, building);

				if(cell != -1 && xPos+xSize+cfg->X_SPACE <= xMapSize)
				{
					BlockCells(cell, y, cfg->X_SPACE, 1, add, water);

					//add blocking of the edges
					if(y == yPos && (yPos - cfg->Y_SPACE) >= 0)
						BlockCells(cell, yPos - cfg->Y_SPACE, cfg->X_SPACE, cfg->Y_SPACE, add, water);
					if(y == yPos + ySize - 1)
						BlockCells(cell, yPos + ySize, cfg->X_SPACE, cfg->Y_SPACE, add, water);
				}

				// left side
				cell = GetNextX(0, xPos-1, y, building);

				if(cell != -1 && cell-cfg->X_SPACE >= 0)
				{
					BlockCells(cell-cfg->X_SPACE, y, cfg->X_SPACE, 1, add, water);

					// add diagonal blocks
					if(y == yPos && (yPos - cfg->Y_SPACE) >= 0)
							BlockCells(cell-cfg->X_SPACE, yPos - cfg->Y_SPACE, cfg->X_SPACE, cfg->Y_SPACE, add, water);
					if(y == yPos + ySize - 1)
							BlockCells(cell-cfg->X_SPACE, yPos + ySize, cfg->X_SPACE, cfg->Y_SPACE, add, water);

				}
			}
		}
	}

	// check vertical space
	if(yPos+ySize+cfg->MAX_YROW <= yMapSize && yPos - cfg->MAX_YROW >= 0)
	{
		for(int x = xPos; x < xPos + xSize; x++)
		{
			if(x >= xMapSize)
			{
				fprintf(ai->file, "ERROR: x = %i index out of range when checking vertical rows", x);
				return;
			}

			// check downwards
			insert_space = true;
			for(int y = yPos+ySize; y < yPos+ySize+cfg->MAX_YROW; ++y)
			{
				if(buildmap[x+y*xMapSize] != building)
				{
					insert_space = false;
					break;
				}
			}

			// check upwards
			if(!insert_space)
			{
				insert_space = true;

				for(int y = yPos-1; y >= yPos - cfg->MAX_YROW; --y)
				{
					if(buildmap[x+y*xMapSize] != building)
					{
						insert_space = false;
						break;
					}
				}
			}

			if(insert_space)
			{
				// downwards
				cell = GetNextY(1, x, yPos+ySize, building);

				if(cell != -1 && yPos+ySize+cfg->Y_SPACE <= yMapSize)
				{
					BlockCells(x, cell, 1, cfg->Y_SPACE, add, water);

					// add diagonal blocks
					if(x == xPos && (xPos - cfg->X_SPACE) >= 0)
						BlockCells(xPos-cfg->X_SPACE, cell, cfg->X_SPACE, cfg->Y_SPACE, add, water);
					if(x == xPos + xSize - 1)
						BlockCells(xPos + xSize, cell, cfg->X_SPACE, cfg->Y_SPACE, add, water);
				}

				// upwards
				cell = GetNextY(0, x, yPos-1, building);

				if(cell != -1 && cell-cfg->Y_SPACE >= 0)
				{
					BlockCells(x, cell-cfg->Y_SPACE, 1, cfg->Y_SPACE, add, water);

					// add diagonal blocks
					if(x == xPos && (xPos - cfg->X_SPACE) >= 0)
						BlockCells(xPos-cfg->X_SPACE, cell-cfg->Y_SPACE, cfg->X_SPACE, cfg->Y_SPACE, add, water);
					if(x == xPos + xSize - 1)
						BlockCells(xPos + xSize, cell-cfg->Y_SPACE, cfg->X_SPACE, cfg->Y_SPACE, add, water);
				}
			}
		}
	}
}

void AAIMap::BlockCells(int xPos, int yPos, int width, int height, bool block, bool water)
{
	// make sure to stay within map if too close to the edges
	int xEnd = xPos + width;
	int yEnd = yPos + height;

	if(xEnd > xMapSize)
		xEnd = xMapSize;

	if(yEnd > yMapSize)
		yEnd = yMapSize;

	//float3 my_pos;
	int empty, cell;

	if(water)
		empty = 4;
	else
		empty = 0;

	if(block)	// block cells
	{
		for(int y = yPos; y < yEnd; ++y)
		{
			for(int x = xPos; x < xEnd; ++x)
			{
				cell = x + xMapSize*y;

				// if no building ordered that cell to be blocked, update buildmap
				// (only if space is not already occupied by a building)
				if(blockmap[cell] == 0 && buildmap[cell] == empty)
				{
					buildmap[cell] = 2;

					// debug
					/*if(x%2 == 0 && y%2 == 0)
					{
						my_pos.x = x * 8;
						my_pos.z = y * 8;
						my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
						cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 1500, cb->GetMyAllyTeam(), true, true);
					}*/
				}

				++blockmap[cell];
			}
		}
	}
	else		// unblock cells
	{
		for(int y = yPos; y < yEnd; ++y)
		{
			for(int x = xPos; x < xEnd; ++x)
			{
				cell = x + xMapSize*y;

				if(blockmap[cell] > 0)
				{
					--blockmap[cell];

					// if cell is not blocked anymore, mark cell on buildmap as empty (only if it has been marked bloked
					//					- if it is not marked as blocked its occupied by another building or unpassable)
					if(blockmap[cell] == 0 && buildmap[cell] == 2)
					{
						buildmap[cell] = empty;

						// debug
						/*if(x%2 == 0 && y%2 == 0)
						{
							my_pos.x = x * 8;
							my_pos.z = y * 8;
							my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
							cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 1500, cb->GetMyAllyTeam(), true, true);
						}*/
					}
				}
			}
		}
	}
}

void AAIMap::UpdateBuildMap(float3 build_pos, const UnitDef *def, bool block, bool water, bool factory)
{
	Pos2BuildMapPos(&build_pos, def);

	if(block)
	{
		if(water)
			SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->zsize, 5);
		else
			SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->zsize, 1);
	}
	else
	{
		// remove spaces before freeing up buildspace
		CheckRows(build_pos.x, build_pos.z, def->xsize, def->zsize, block, water);

		if(water)
			SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->zsize, 4);
		else
			SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->zsize, 0);
	}

	if(factory)
	{
		// extra space for factories to keep exits clear
		BlockCells(build_pos.x, build_pos.z - 8, def->xsize, 8, block, water);
		BlockCells(build_pos.x + def->xsize, build_pos.z - 8, cfg->X_SPACE, def->zsize + 1.5f * (float)cfg->Y_SPACE, block, water);
		BlockCells(build_pos.x, build_pos.z + def->zsize, def->xsize, 1.5f * (float)cfg->Y_SPACE - 8, block, water);
	}

	// add spaces after blocking buildspace
	if(block)
		CheckRows(build_pos.x, build_pos.z, def->xsize, def->zsize, block, water);
}



int AAIMap::GetNextX(int direction, int xPos, int yPos, int value)
{
	int x = xPos;

	if(direction)
	{
		while(buildmap[x+yPos*xMapSize] == value)
		{
			++x;

			// search went out of map
			if(x >= xMapSize)
				return -1;
		}
	}
	else
	{
		while(buildmap[x+yPos*xMapSize] == value)
		{
			--x;

			// search went out of map
			if(x < 0)
				return -1;
		}
	}

	return x;
}

int AAIMap::GetNextY(int direction, int xPos, int yPos, int value)
{
	int y = yPos;

	if(direction)
	{
		// scan line until next free cell found
		while(buildmap[xPos+y*xMapSize] == value)
		{
			++y;

			// search went out of map
			if(y >= yMapSize)
				return -1;
		}
	}
	else
	{
		// scan line until next free cell found
		while(buildmap[xPos+y*xMapSize] == value)
		{
			--y;

			// search went out of map
			if(y < 0)
				return -1;
		}
	}

	return y;
}

void AAIMap::GetSize(const UnitDef *def, int *xSize, int *ySize)
{
	// calculate size of building
	*xSize = def->xsize;
	*ySize = def->zsize;

	// if building is a factory additional vertical space is needed
	if(bt->IsFactory(def->id))
	{
		*xSize += cfg->X_SPACE;
		*ySize += ((float)cfg->Y_SPACE)*1.5;
	}
}

int AAIMap::GetCliffyCells(int xPos, int yPos, int xSize, int ySize)
{
	int cliffs = 0;

	// count cells with big slope
	for(int x = xPos; x < xPos + xSize; ++x)
	{
		for(int y = yPos; y < yPos + ySize; ++y)
		{
			if(buildmap[x+y*xMapSize] == 3)
				++cliffs;
		}
	}

	return cliffs;
}

int AAIMap::GetCliffyCellsInSector(AAISector *sector)
{
	int cliffs = 0;

	int xPos = sector->x * xSectorSize;
	int yPos = sector->y * ySectorSize;

	// count cells with big slope
	for(int x = xPos; x < xPos + xSectorSizeMap; ++x)
	{
		for(int y = yPos; y < yPos + ySectorSizeMap; ++y)
		{
			if(buildmap[x+y*xMapSize] == 3)
				++cliffs;
		}
	}

	return cliffs;
}

void AAIMap::AnalyseMap()
{
	float3 my_pos;

	float slope;

	const float *height_map = cb->GetHeightMap();

	// get water/cliffs
	for(int x = 0; x < xMapSize; ++x)
	{
		for(int y = 0; y < yMapSize; ++y)
		{
			// check for water
			if(height_map[y * xMapSize + x] < 0)
				buildmap[x+y*xMapSize] = 4;
			else if(x < xMapSize - 4 && y < yMapSize - 4)
			// check slope
			{
				slope = (height_map[y * xMapSize + x] - height_map[y * xMapSize + x + 4])/64.0;

				// check x-direction
				if(slope > cfg->CLIFF_SLOPE || -slope > cfg->CLIFF_SLOPE)
					buildmap[x+y*xMapSize] = 3;
				else	// check y-direction
				{
					slope = (height_map[y * xMapSize + x] - height_map[(y+4) * xMapSize + x])/64.0;

					if(slope > cfg->CLIFF_SLOPE || -slope > cfg->CLIFF_SLOPE)
						buildmap[x+y*xMapSize] = 3;
				}
			}
		}
	}

	// calculate plateu map
	int xSize = xMapSize/4;
	int ySize = yMapSize/4;

	int TERRAIN_DETECTION_RANGE = 6;
	float my_height, diff;

	for(int x = TERRAIN_DETECTION_RANGE; x < xSize - TERRAIN_DETECTION_RANGE; ++x)
	{
		for(int y = TERRAIN_DETECTION_RANGE; y < ySize - TERRAIN_DETECTION_RANGE; ++y)
		{
			my_height = height_map[4 * (x + y * xMapSize)];

			for(int i = x - TERRAIN_DETECTION_RANGE; i < x + TERRAIN_DETECTION_RANGE; ++i)
			{
				for(int j = y - TERRAIN_DETECTION_RANGE; j < y + TERRAIN_DETECTION_RANGE; ++j)
				{
					 diff = (height_map[4 * (i + j * xMapSize)] - my_height);

					 if(diff > 0)
					 {
						 if(buildmap[4 * (i + j * xMapSize)] != 3)
							 plateau_map[i + j * xSize] += diff;
					 }
					 else
						 plateau_map[i + j * xSize] += diff;
				}
			}
		}
	}

	for(int x = 0; x < xSize; ++x)
	{
		for(int y = 0; y < ySize; ++y)
		{
			if(plateau_map[x + y * xSize] >= 0)
				plateau_map[x + y * xSize] = sqrt(plateau_map[x + y * xSize]);
			else
				plateau_map[x + y * xSize] = -1.0f * sqrt((-1.0f) * plateau_map[x + y * xSize]);
		}
	}
}

void AAIMap::DetectMapType()
{
	if( (float)max_land_continent_size < 0.5f * (float)max_water_continent_size || water_ratio > 0.80f)
		map_type = WATER_MAP;
	else if(water_ratio > 0.25f)
		map_type = LAND_WATER_MAP;
	else
		map_type = LAND_MAP;
}

void AAIMap::CalculateWaterRatio()
{
	water_ratio = 0;

	for(int y = 0; y < yMapSize; ++y)
	{
		for(int x = 0; x < xMapSize; ++x)
		{
			if(buildmap[x + y*xMapSize] == 4)
				++water_ratio;
		}
	}

	water_ratio = water_ratio / ((float)(xMapSize*yMapSize));
}

void AAIMap::CalculateContinentMaps()
{
	vector<int> *new_edge_cells;
	vector<int> *old_edge_cells;

	vector<int> a, b;

	old_edge_cells = &a;
	new_edge_cells = &b;

	const float *height_map = cb->GetHeightMap();

	int x, y;

	AAIContinent temp;
	int continent_id = 0;


	for(int i = 0; i < xContMapSize; i += 1)
	{
		for(int j = 0; j < yContMapSize; j += 1)
		{
			// add new continent if cell has not been visited yet
			if(continent_map[j * xContMapSize + i] < 0 && height_map[4 * (j * xMapSize + i)] >= 0)
			{
				temp.id = continent_id;
				temp.size = 1;
				temp.water = false;

				continents.push_back(temp);

				continent_map[j * xContMapSize + i] = continent_id;

				old_edge_cells->push_back(j * xContMapSize + i);

				// check edges of the continent as long as new cells have been added to the continent during the last loop
				while(old_edge_cells->size() > 0)
				{
					for(vector<int>::iterator cell = old_edge_cells->begin(); cell != old_edge_cells->end(); ++cell)
					{
						// get cell indizes
						x = (*cell)%xContMapSize;
						y = ((*cell) - x) / xContMapSize;

						// check edges
						if(x > 0 && continent_map[y * xContMapSize + x - 1] == -1)
						{
							if(height_map[4 * (y * xMapSize + x - 1)] >= 0)
							{
								continent_map[y * xContMapSize + x - 1] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( y * xContMapSize + x - 1 );
							}
							else if(height_map[4 * (y * xMapSize + x - 1)] >= - cfg->NON_AMPHIB_MAX_WATERDEPTH)
							{
								continent_map[y * xContMapSize + x - 1] = -2;
								new_edge_cells->push_back( y * xContMapSize + x - 1 );
							}
						}

						if(x < xContMapSize-1 && continent_map[y * xContMapSize + x + 1] == -1)
						{
							if(height_map[4 * (y * xMapSize + x + 1)] >= 0)
							{
								continent_map[y * xContMapSize + x + 1] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( y * xContMapSize + x + 1 );
							}
							else if(height_map[4 * (y * xMapSize + x + 1)] >= - cfg->NON_AMPHIB_MAX_WATERDEPTH)
							{
								continent_map[y * xContMapSize + x + 1] = -2;
								new_edge_cells->push_back( y * xContMapSize + x + 1 );
							}
						}

						if(y > 0 && continent_map[(y - 1) * xContMapSize + x] == -1)
						{
							if(height_map[4 * ( (y - 1) * xMapSize + x)] >= 0)
							{
								continent_map[(y - 1) * xContMapSize + x] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( (y - 1) * xContMapSize + x);
							}
							else if(height_map[4 * ( (y - 1) * xMapSize + x)] >= - cfg->NON_AMPHIB_MAX_WATERDEPTH)
							{
								continent_map[(y - 1) * xContMapSize + x] = -2;
								new_edge_cells->push_back( (y - 1) * xContMapSize + x );
							}
						}

						if(y < yContMapSize-1 && continent_map[(y + 1 ) * xContMapSize + x] == -1)
						{
							if(height_map[4 * ( (y + 1) * xMapSize + x)] >= 0)
							{
								continent_map[(y + 1) * xContMapSize + x] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( (y + 1) * xContMapSize + x );
							}
							else if(height_map[4 * ( (y + 1) * xMapSize + x)] >= - cfg->NON_AMPHIB_MAX_WATERDEPTH)
							{
								continent_map[(y + 1) * xContMapSize + x] = -2;
								new_edge_cells->push_back( (y + 1) * xContMapSize + x );
							}
						}
					}

					old_edge_cells->clear();

					// invert pointers to new/old edge cells
					if(new_edge_cells == &a)
					{
						new_edge_cells = &b;
						old_edge_cells = &a;
					}
					else
					{
						new_edge_cells = &a;
						old_edge_cells = &b;
					}
				}

				// finished adding continent
				++continent_id;
				old_edge_cells->clear();
				new_edge_cells->clear();
			}
		}
	}

	// water continents
	for(int i = 0; i < xContMapSize; i += 1)
	{
		for(int j = 0; j < yContMapSize; j += 1)
		{
			// add new continent if cell has not been visited yet
			if(continent_map[j * xContMapSize + i] < 0)
			{
				temp.id = continent_id;
				temp.size = 1;
				temp.water = true;

				continents.push_back(temp);

				continent_map[j * xContMapSize + i] = continent_id;

				old_edge_cells->push_back(j * xContMapSize + i);

				// check edges of the continent as long as new cells have been added to the continent during the last loop
				while(old_edge_cells->size() > 0)
				{
					for(vector<int>::iterator cell = old_edge_cells->begin(); cell != old_edge_cells->end(); ++cell)
					{
						// get cell indizes
						x = (*cell)%xContMapSize;
						y = ((*cell) - x) / xContMapSize;

						// check edges
						if(x > 0 && continent_map[y * xContMapSize + x - 1] < 0)
						{
							if(height_map[4 * (y * xMapSize + x - 1)] < 0)
								{
								continent_map[y * xContMapSize + x - 1] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( y * xContMapSize + x - 1 );
							}
						}

						if(x < xContMapSize-1 && continent_map[y * xContMapSize + x + 1] < 0)
						{
							if(height_map[4 * (y * xMapSize + x + 1)] < 0)
							{
								continent_map[y * xContMapSize + x + 1] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( y * xContMapSize + x + 1 );
							}
						}

						if(y > 0 && continent_map[(y - 1) * xContMapSize + x ] < 0)
						{
							if(height_map[4 * ( (y - 1) * xMapSize + x )] < 0)
							{
								continent_map[(y - 1) * xContMapSize + x ] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( (y - 1) * xContMapSize + x );
							}
						}

						if(y < yContMapSize-1 && continent_map[(y + 1) * xContMapSize + x ] < 0)
						{
							if(height_map[4 * ( (y + 1) * xMapSize + x)] < 0)
							{
								continent_map[(y + 1) * xContMapSize + x ] = continent_id;
								continents[continent_id].size += 1;
								new_edge_cells->push_back( (y + 1) * xContMapSize + x  );
							}
						}
					}

					old_edge_cells->clear();

					// invert pointers to new/old edge cells
					if(new_edge_cells == &a)
					{
						new_edge_cells = &b;
						old_edge_cells = &a;
					}
					else
					{
						new_edge_cells = &a;
						old_edge_cells = &b;
					}
				}

				// finished adding continent
				++continent_id;
				old_edge_cells->clear();
				new_edge_cells->clear();
			}
		}
	}

	// calculate some statistical data
	land_continents = 0;
	water_continents = 0;

	avg_land_continent_size = 0;
	avg_water_continent_size = 0;
	max_land_continent_size = 0;
	max_water_continent_size = 0;
	min_land_continent_size = xContMapSize * yContMapSize;
	min_water_continent_size = xContMapSize * yContMapSize;

	for(size_t i = 0; i < continents.size(); ++i)
	{
		if(continents[i].water)
		{
			++water_continents;
			avg_water_continent_size += continents[i].size;

			if(continents[i].size > max_water_continent_size)
				max_water_continent_size = continents[i].size;

			if(continents[i].size < min_water_continent_size)
				min_water_continent_size = continents[i].size;
		}
		else
		{
			++land_continents;
			avg_land_continent_size += continents[i].size;

			if(continents[i].size > max_land_continent_size)
				max_land_continent_size = continents[i].size;

			if(continents[i].size < min_land_continent_size)
				min_land_continent_size = continents[i].size;
		}
	}

	if(water_continents > 0)
		avg_water_continent_size /= water_continents;

	if(land_continents > 0)
		avg_land_continent_size /= land_continents;
}

// algorithm more or less by krogothe - thx very much
void AAIMap::SearchMetalSpots()
{
	const UnitDef* def = bt->unitList[bt->GetBiggestMex()-1];

	metalMap = false;
	bool Stopme = false;
	int TotalMetal = 0;
	int MaxMetal = 0;
	int TempMetal = 0;
	int SpotsFound = 0;
	int coordx, coordy;
	float AverageMetal;

	AAIMetalSpot temp;
	float3 pos;

	int MinMetalForSpot = 30; // from 0-255, the minimum percentage of metal a spot needs to have
							//from the maximum to be saved. Prevents crappier spots in between taken spaces.
							//They are still perfectly valid and will generate metal mind you!
	int MaxSpots = 5000; //If more spots than that are found the map is considered a metalmap, tweak this as needed

	int MetalMapHeight = cb->GetMapHeight() / 2; //metal map has 1/2 resolution of normal map
	int MetalMapWidth = cb->GetMapWidth() / 2;
	int TotalCells = MetalMapHeight * MetalMapWidth;
	unsigned char XtractorRadius = cb->GetExtractorRadius()/ 16.0;
	unsigned char DoubleRadius = cb->GetExtractorRadius() / 8.0;
	int SquareRadius = (cb->GetExtractorRadius() / 16.0) * (cb->GetExtractorRadius() / 16.0); //used to speed up loops so no recalculation needed
	int DoubleSquareRadius = (cb->GetExtractorRadius() / 8.0) * (cb->GetExtractorRadius() / 8.0); // same as above
	int CellsInRadius = PI * XtractorRadius * XtractorRadius; //yadda yadda
	unsigned char* MexArrayA = new unsigned char [TotalCells];
	unsigned char* MexArrayB = new unsigned char [TotalCells];
	int* TempAverage = new int [TotalCells];

	// clear variables, just in case!
	TotalMetal = 0;
	MaxMetal = 0;
	Stopme = 0;
	SpotsFound = 0;

	//Load up the metal Values in each pixel
	for (int i = 0; i != TotalCells - 1; i++)
	{
		MexArrayA[i] = *(cb->GetMetalMap() + i);
		TotalMetal += MexArrayA[i];		// Count the total metal so you can work out an average of the whole map
	}

	AverageMetal = ((float)TotalMetal) / ((float)TotalCells);  //do the average

	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y != MetalMapHeight; y++)
	{
		for (int x = 0; x != MetalMapWidth; x++)
		{
			TotalMetal = 0;
			for (int myx = x - XtractorRadius; myx != x + XtractorRadius; myx++)
			{
				if (myx >= 0 && myx < MetalMapWidth)
				{
					for (int myy = y - XtractorRadius; myy != y + XtractorRadius; myy++)
					{
						if (myy >= 0 && myy < MetalMapHeight && ((x - myx)*(x - myx) + (y - myy)*(y - myy)) <= SquareRadius)
						{
							TotalMetal += MexArrayA[myy * MetalMapWidth + myx]; //get the metal from all pixels around the extractor radius
						}
					}
				}
			}
			TempAverage[y * MetalMapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxMetal < TotalMetal)
				MaxMetal = TotalMetal;  //find the spot with the highest metal to set as the map's max
		}
	}
	for (int i = 0; i != TotalCells; i++) // this will get the total metal a mex placed at each spot would make
	{
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;  //scale the metal so any map will have values 0-255, no matter how much metal it has
	}


	for (int a = 0; a != MaxSpots; a++)
	{
		if(!Stopme)
			TempMetal = 0; //reset tempmetal so it can find new spots
		for (int i = 0; i != TotalCells; i=i+2)
		{			//finds the best spot on the map and gets its coords
			if (MexArrayB[i] > TempMetal && !Stopme)
			{
				TempMetal = MexArrayB[i];
				coordx = i%MetalMapWidth;
				coordy = i/MetalMapWidth;
			}
		}
		if (TempMetal < MinMetalForSpot)
			Stopme = 1; // if the spots get too crappy it will stop running the loops to speed it all up

		if (!Stopme)
		{
			pos.x = coordx * 2 * SQUARE_SIZE;
			pos.z = coordy * 2 * SQUARE_SIZE;
			pos.y = cb->GetElevation(pos.x, pos.z);

			Pos2FinalBuildPos(&pos, def);

			temp.amount = TempMetal * cb->GetMaxMetal() * MaxMetal / 255.0;
			temp.occupied = false;
			temp.pos = pos;

			//if(cb->CanBuildAt(def, pos))
			//{
				Pos2BuildMapPos(&pos, def);

				if(pos.z >= 2 && pos.x >= 2 && pos.x < xMapSize-2 && pos.z < yMapSize-2)
				{
					if(CanBuildAt(pos.x, pos.z, def->xsize, def->zsize))
					{
						metal_spots.push_back(temp);
						SpotsFound++;

						if(pos.y >= 0)
							SetBuildMap(pos.x-2, pos.z-2, def->xsize+4, def->zsize+4, 1);
						else
							SetBuildMap(pos.x-2, pos.z-2, def->xsize+4, def->zsize+4, 5);
					}
				}
			//}

			for (int myx = coordx - XtractorRadius; myx != coordx + XtractorRadius; myx++)
			{
				if (myx >= 0 && myx < MetalMapWidth)
				{
					for (int myy = coordy - XtractorRadius; myy != coordy + XtractorRadius; myy++)
					{
						if (myy >= 0 && myy < MetalMapHeight && ((coordx - myx)*(coordx - myx) + (coordy - myy)*(coordy - myy)) <= SquareRadius)
						{
							MexArrayA[myy * MetalMapWidth + myx] = 0; //wipes the metal around the spot so its not counted twice
							MexArrayB[myy * MetalMapWidth + myx] = 0;
						}
					}
				}
			}

			// Redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordy - DoubleRadius; y != coordy + DoubleRadius; y++)
			{
				if(y >=0 && y < MetalMapHeight)
				{
					for (int x = coordx - DoubleRadius; x != coordx + DoubleRadius; x++)
					{//funcion below is optimized so it will only update spots between r and 2r, greatly speeding it up
						if((coordx - x)*(coordx - x) + (coordy - y)*(coordy - y) <= DoubleSquareRadius && x >=0 && x < MetalMapWidth && MexArrayB[y * MetalMapWidth + x])
						{
							TotalMetal = 0;
							for (int myx = x - XtractorRadius; myx != x + XtractorRadius; myx++)
							{
								if (myx >= 0 && myx < MetalMapWidth)
								{
									for (int myy = y - XtractorRadius; myy != y + XtractorRadius; myy++)
									{
										if (myy >= 0 && myy < MetalMapHeight && ((x - myx)*(x - myx) + (y - myy)*(y - myy)) <= SquareRadius)
										{
											TotalMetal += MexArrayA[myy * MetalMapWidth + myx]; //recalculate nearby spots to account for deleted metal from chosen spot
										}
									}
								}
							}
							MexArrayB[y * MetalMapWidth + x] = TotalMetal * 255 / MaxMetal;; //set that spots metal amount
						}
					}
				}
			}
		}
	}

	if(SpotsFound > 500)
	{
		metalMap = true;
		metal_spots.clear();
		fprintf(ai->file, "Map is considered to be a metal map\n");
	}
	else
		metalMap = false;

	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] TempAverage;
}

void AAIMap::UpdateRecon()
{
	const UnitDef *def;
	UnitCategory cat;
	float3 pos;

	int frame = cb->GetCurrentFrame();

	fill(sector_in_los.begin(), sector_in_los.end(), 0);
	fill(sector_in_los_with_enemies.begin(), sector_in_los_with_enemies.end(), 0);
	fill(enemy_combat_units_spotted.begin(), enemy_combat_units_spotted.end(), 0);

	//
	// reset scouted buildings for all cells within current los
	//
	const unsigned short *los_map = cb->GetLosMap();

	for(int y = 0; y < yLOSMapSize; ++y)
	{
		for(int x = 0; x < xLOSMapSize; ++x)
		{
			if(los_map[x + y * xLOSMapSize])
			{
				scout_map[x + y * xLOSMapSize] = 0;
				last_updated_map[x + y * xLOSMapSize] = frame;
				++sector_in_los[(losMapRes*x / xSectorSizeMap) + (losMapRes*y / ySectorSizeMap) * (xSectors+1)];
			}
		}
	}


	for(int y = 0; y < ySectors; ++y)
	{
		for(int x = 0; x < xSectors; ++x)
			sector[x][y].enemies_on_radar = 0;
	}

	// update enemy units
	int number_of_units = cb->GetEnemyUnitsInRadarAndLos(&(units_in_los.front()));
	int x_pos, y_pos;

	for(int i = 0; i < number_of_units; ++i)
	{
		//pos = cb->GetUnitPos(units_in_los[i]);
		def = cb->GetUnitDef(units_in_los[i]);

		if(def) // unit is within los
		{
			x_pos = (int)pos.x / (losMapRes * SQUARE_SIZE);
			y_pos = (int)pos.z / (losMapRes * SQUARE_SIZE);

			// make sure unit is within the map (e.g. no aircraft that has flown outside of the map)
			if(x_pos >= 0 && x_pos < xLOSMapSize && y_pos >= 0 && y_pos < yLOSMapSize)
			{
				cat = bt->units_static[def->id].category;

				// add buildings/combat units to scout map
				if(cat >= STATIONARY_DEF && cat <= SUBMARINE_ASSAULT)
				{
					scout_map[x_pos + y_pos * xLOSMapSize] = def->id;
					++sector_in_los_with_enemies[(losMapRes * x_pos) / xSectorSizeMap + (xSectors + 1) * ((losMapRes * y_pos) / ySectorSizeMap) ];
				}

				if(cat >= GROUND_ASSAULT && cat <= SUBMARINE_ASSAULT)
					++enemy_combat_units_spotted[cat - GROUND_ASSAULT];
			}
		}
		else // unit on radar only
		{
			pos = cb->GetUnitPos(units_in_los[i]);

			x_pos = pos.x/xSectorSize;
			y_pos = pos.z/ySectorSize;

			if(x_pos >= 0 && y_pos >= 0 && x_pos < xSectors && y_pos < ySectors)
				sector[x_pos][y_pos].enemies_on_radar += 1;
		}
	}

	// map of known enemy buildings has been updated -> update sector data
	for(int y = 0; y < ySectors; ++y)
	{
		for(int x = 0; x < xSectors; ++x)
		{
			// only update sector data if its within los
			if(sector_in_los[x + y * (xSectors+1)])
			{
				sector[x][y].own_structures = 0;
				sector[x][y].allied_structures = 0;

				fill(sector[x][y].my_combat_units.begin(), sector[x][y].my_combat_units.end(), 0);
				fill(sector[x][y].my_mobile_combat_power.begin(), sector[x][y].my_mobile_combat_power.end(), 0);
				fill(sector[x][y].my_stat_combat_power.begin(), sector[x][y].my_stat_combat_power.end(), 0);
			}
		}
	}

	// update own/friendly units
	int x, y;
	int my_team = cb->GetMyTeam();

	number_of_units = cb->GetFriendlyUnits(&(units_in_los.front()));

	for(int i = 0; i < number_of_units; ++i)
	{
		// get unit def & category
		def = cb->GetUnitDef(units_in_los[i]);
		cat = bt->units_static[def->id].category;

		if(cat >= STATIONARY_DEF && cat <= SUBMARINE_ASSAULT)
		{
			pos = cb->GetUnitPos(units_in_los[i]);

			x = pos.x/xSectorSize;
			y = pos.z/ySectorSize;

			if(x >= 0 && y >= 0 && x < xSectors && y < ySectors)
			{
				// add building to sector (and update stat_combat_power if it's a stat defence)
				if(cat <= METAL_MAKER)
				{
					if(cb->GetUnitTeam(units_in_los[i]) == my_team)
						++sector[x][y].own_structures;
					else
						++sector[x][y].allied_structures;

					if(cat == STATIONARY_DEF)
					{
						for(int i = 0; i < bt->ass_categories; ++i)
							sector[x][y].my_stat_combat_power[i] += bt->units_static[def->id].efficiency[i];
					}
				}
				// add unit to sector and update mobile_combat_power
				else if(cat >= GROUND_ASSAULT)
				{
					++sector[x][y].my_combat_units[bt->units_static[def->id].category - GROUND_ASSAULT];

					for(int i = 0; i < bt->combat_categories; ++i)
						sector[x][y].my_mobile_combat_power[i] += bt->units_static[def->id].efficiency[i];
				}
			}
		}
	}

	ai->brain->UpdateMaxCombatUnitsSpotted(enemy_combat_units_spotted);
}

void AAIMap::UpdateEnemyScoutingData()
{
	int def_id;
	int frame = cb->GetCurrentFrame();
	float last_seen;
	AAISector *sector;

	// map of known enemy buildings has been updated -> update sector data
	for(int y = 0; y < ySectors; ++y)
	{
		for(int x = 0; x < xSectors; ++x)
		{
			sector = &this->sector[x][y];
			sector->enemy_structures = 0;

			fill(sector->enemy_combat_units.begin(), sector->enemy_combat_units.end(), 0);
			fill(sector->enemy_stat_combat_power.begin(), sector->enemy_stat_combat_power.end(), 0);
			fill(sector->enemy_mobile_combat_power.begin(), sector->enemy_mobile_combat_power.end(), 0);

			for(int y = sector->y * ySectorSizeMap/losMapRes; y < (sector->y + 1) * ySectorSizeMap/losMapRes; ++y)
			{
				for(int x = sector->x * xSectorSizeMap/losMapRes; x < (sector->x + 1) * xSectorSizeMap/losMapRes; ++x)
				{
					def_id = scout_map[x + y * xLOSMapSize];

					if(def_id)
					{
						// add building to sector (and update stat_combat_power if it's a stat defence)
						if(bt->units_static[def_id].category <= METAL_MAKER)
						{
							++sector->enemy_structures;

							if(bt->units_static[def_id].category == STATIONARY_DEF)
							{
								for(int i = 0; i < bt->ass_categories; ++i)
									sector->enemy_stat_combat_power[i] += bt->units_static[def_id].efficiency[i];
							}
						}
						// add unit to sector and update mobile_combat_power
						else if(bt->units_static[def_id].category >= GROUND_ASSAULT)
						{
							// units that have been scouted long time ago matter less
							last_seen = exp(cfg->SCOUTING_MEMORY_FACTOR * ((float)(last_updated_map[x + y * xLOSMapSize] - frame)) / 3600.0f  );

							sector->enemy_combat_units[bt->units_static[def_id].category - GROUND_ASSAULT] += last_seen;
							sector->enemy_combat_units[5] += last_seen;

							for(int i = 0; i < bt->combat_categories; ++i)
								sector->enemy_mobile_combat_power[i] += last_seen * bt->units_static[def_id].efficiency[i];
						}
					}
				}
			}
		}
	}
}

void AAIMap::UpdateSectors()
{
	for(int x = 0; x < xSectors; ++x)
	{
		for(int y = 0; y < ySectors; ++y)
			sector[x][y].Update();
	}
}

const char* AAIMap::GetMapTypeString(MapType map_type)
{
	if(map_type == LAND_MAP)
		return "LAND_MAP";
	else if(map_type == LAND_WATER_MAP)
		return "LAND_WATER_MAP";
	else if(map_type == WATER_MAP)
		return "WATER_MAP";
	else
		return "UNKNOWN_MAP";
}

const char* AAIMap::GetMapTypeTextString(MapType map_type)
{
	if(map_type == LAND_MAP)
		return "land map";
	else if(map_type == LAND_WATER_MAP)
		return "land-water map";
	else if(map_type == WATER_MAP)
		return "water map";
	else
		return "unknown map type";
}


bool AAIMap::ValidSector(int x, int y)
{
	if(x >= 0 && y >= 0 && x < xSectors && y < ySectors)
		return true;
	else
		return false;
}

AAISector* AAIMap::GetSectorOfPos(float3 *pos)
{
	int x = pos->x/xSectorSize;
	int y = pos->z/ySectorSize;

	if(ValidSector(x,y))
		return &(sector[x][y]);
	else
		return 0;
}

void AAIMap::AddDefence(float3 *pos, int defence)
{
	int range = bt->units_static[defence].range / (SQUARE_SIZE * 4);
	int cell;

	float power;
	float air_power;
	float submarine_power;

	if(cfg->AIR_ONLY_MOD)
	{
		power = bt->fixed_eff[defence][0];
		air_power = (bt->fixed_eff[defence][1] + bt->fixed_eff[defence][2])/2.0f;
		submarine_power = bt->fixed_eff[defence][3];
	}
	else
	{
		if(bt->unitList[defence-1]->minWaterDepth > 0)
			power = (bt->fixed_eff[defence][2] + bt->fixed_eff[defence][3]) / 2.0f;
		else
			power = bt->fixed_eff[defence][0];

		air_power = bt->fixed_eff[defence][1];
		submarine_power = bt->fixed_eff[defence][4];
	}

	int xPos = (pos->x + bt->unitList[defence-1]->xsize/2)/ (SQUARE_SIZE * 4);
	int yPos = (pos->z + bt->unitList[defence-1]->zsize/2)/ (SQUARE_SIZE * 4);

	// x range will change from line to line
	int xStart;
	int xEnd;
	int xRange;

	// y range is const
	int yStart = yPos - range;
	int yEnd = yPos + range;

	if(yStart < 0)
		yStart = 0;
	if(yEnd > yDefMapSize)
		yEnd = yDefMapSize;

	for(int y = yStart; y < yEnd; ++y)
	{
		// determine x-range
		xRange = (int) floor( fastmath::sqrt2( (float) ( range * range - (y - yPos) * (y - yPos) ) ) + 0.5f );

		xStart = xPos - xRange;
		xEnd = xPos + xRange;

		if(xStart < 0)
			xStart = 0;
		if(xEnd > xDefMapSize)
			xEnd = xDefMapSize;

		for(int x = xStart; x < xEnd; ++x)
		{
			cell = x + xDefMapSize*y;

			defence_map[cell] += power;
			air_defence_map[cell] += air_power;
			submarine_defence_map[cell] += submarine_power;
		}
	}

	// further increase values close around the bulding (to prevent aai from packing buildings too close together)
	xStart = xPos - 3;
	xEnd = xPos + 3;
	yStart = yPos - 3;
	yEnd = yPos + 3;

	if(xStart < 0)
		xStart = 0;
	if(xEnd >= xDefMapSize)
		xEnd = xDefMapSize-1;

	if(yStart < 0)
		yStart = 0;
	if(yEnd >= yDefMapSize)
		yEnd = yDefMapSize-1;

	float3 my_pos;

	for(int y = yStart; y <= yEnd; ++y)
	{
		for(int x = xStart; x <= xEnd; ++x)
		{
			cell = x + xDefMapSize*y;

			defence_map[cell] += 5000.0f;
			air_defence_map[cell] += 5000.0f;
			submarine_defence_map[cell] += 5000.0f;

			/*my_pos.x = x * 32;
			my_pos.z = y * 32;
			my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
			cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 8000, cb->GetMyAllyTeam(), false, true);
			my_pos.x = (x+1) * 32;
			my_pos.z = (y+1) * 32;
			my_pos.y = cb->GetElevation(my_pos.x, my_pos.z);
			cb->DrawUnit("ARMMINE1", my_pos, 0.0f, 8000, cb->GetMyAllyTeam(), false, true);*/
		}
	}

	static const size_t filename_sizeMax = 500;
	char filename[filename_sizeMax];
	STRCPY(filename, "AAIDefMap.txt");
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);
	FILE* file = fopen(filename, "w+");
	for(int y = 0; y < yDefMapSize; ++y)
	{
		for(int x = 0; x < xDefMapSize; ++x)
		{
			fprintf(file, "%i ", (int) defence_map[x + y *xDefMapSize]);
		}

		fprintf(file, "\n");
	}
	fclose(file);
}

void AAIMap::RemoveDefence(float3 *pos, int defence)
{
	int cell;
	int range = bt->units_static[defence].range / 32;

	float power;
	float air_power;
	float submarine_power;

	if(cfg->AIR_ONLY_MOD)
	{
		power = bt->fixed_eff[defence][0];
		air_power = (bt->fixed_eff[defence][1] + bt->fixed_eff[defence][2])/2.0f;
		submarine_power = bt->fixed_eff[defence][3];
	}
	else
	{
		if(bt->unitList[defence-1]->minWaterDepth > 0)
			power = (bt->fixed_eff[defence][2] + bt->fixed_eff[defence][3]) / 2.0f;
		else
			power = bt->fixed_eff[defence][0];

		air_power = bt->fixed_eff[defence][1];
		submarine_power = bt->fixed_eff[defence][4];
	}

	int xPos = (pos->x + bt->unitList[defence-1]->xsize/2) / (SQUARE_SIZE * 4);
	int yPos = (pos->z + bt->unitList[defence-1]->zsize/2) / (SQUARE_SIZE * 4);

	// further decrease values close around the bulding (to prevent aai from packing buildings too close together)
	int xStart = xPos - 3;
	int xEnd = xPos + 3;
	int yStart = yPos - 3;
	int yEnd = yPos + 3;

	if(xStart < 0)
		xStart = 0;
	if(xEnd >= xDefMapSize)
		xEnd = xDefMapSize-1;

	if(yStart < 0)
		yStart = 0;
	if(yEnd >= yDefMapSize)
		yEnd = yDefMapSize-1;

	for(int y = yStart; y <= yEnd; ++y)
	{
		for(int x = xStart; x <= xEnd; ++x)
		{
			cell = x + xDefMapSize*y;

			defence_map[cell] -= 5000.0f;
			air_defence_map[cell] -= 5000.0f;
			submarine_defence_map[cell] -= 5000.0f;
		}
	}

	// y range is const
	int xRange;
	yStart = yPos - range;
	yEnd = yPos + range;

	if(yStart < 0)
		yStart = 0;
	if(yEnd > yDefMapSize)
		yEnd = yDefMapSize;

	for(int y = yStart; y < yEnd; ++y)
	{
		// determine x-range
		xRange = (int) floor( fastmath::sqrt2( (float) ( range * range - (y - yPos) * (y - yPos) ) ) + 0.5f );

		xStart = xPos - xRange;
		xEnd = xPos + xRange;

		if(xStart < 0)
			xStart = 0;
		if(xEnd > xDefMapSize)
			xEnd = xDefMapSize;

		for(int x = xStart; x < xEnd; ++x)
		{
			cell = x + xDefMapSize*y;

			defence_map[cell] -= power;
			air_defence_map[cell] -= air_power;
			submarine_defence_map[cell] -= submarine_power;

			if(defence_map[cell] < 0)
				defence_map[cell] = 0;

			if(air_defence_map[cell] < 0)
				air_defence_map[cell] = 0;

			if(submarine_defence_map[cell] < 0)
				submarine_defence_map[cell] = 0;
		}
	}
}

float AAIMap::GetDefenceBuildsite(float3 *best_pos, const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, UnitCategory category, float terrain_modifier, bool water)
{
	float3 pos;
	*best_pos = ZeroVector;
	float my_rating, best_rating = -100000;
	int edge_distance;

	// get required cell-size of the building
	int xSize, ySize, xPos, yPos, cell;
	GetSize(def, &xSize, &ySize);

	vector<float> *map = &defence_map;

	if(cfg->AIR_ONLY_MOD)
	{
		if(category == AIR_ASSAULT || category == HOVER_ASSAULT)
			map = &air_defence_map;
		else if(category == SEA_ASSAULT)
			map = &submarine_defence_map;
	}
	else if(category == AIR_ASSAULT)
		map = &air_defence_map;
	else if(category == SUBMARINE_ASSAULT)
		map = &submarine_defence_map;

	float range =  bt->units_static[def->id].range / 8.0;

	static const size_t filename_sizeMax = 500;
	char filename[filename_sizeMax];
	STRCPY(filename, "AAIDebug.txt");
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);
	FILE* file = fopen(filename, "w+");
	fprintf(file, "Search area: (%i, %i) x (%i, %i)\n", xStart, yStart, xEnd, yEnd);
	fprintf(file, "Range: %g\n", range);

	// check rect
	for(yPos = yStart; yPos < yEnd; yPos += 4)
	{
		for(xPos = xStart; xPos < xEnd; xPos += 4)
		{
			// check if buildmap allows construction
			if(CanBuildAt(xPos, yPos, xSize, ySize, water))
			{

				cell = (xPos/4 + xDefMapSize * yPos/4);

				my_rating = terrain_modifier * plateau_map[cell] - (*map)[cell] + 0.5f *  (float)(rand()%10);
				//my_rating = - (*map)[cell];

				// determine minimum distance from buildpos to the edges of the map
				edge_distance = GetEdgeDistance(xPos, yPos);

				fprintf(file, "Pos: (%i,%i) -> Def map cell %i -> rating: %i  , edge_dist: %i\n",xPos, yPos, cell, (int)my_rating, edge_distance);

				// prevent aai from building defences too close to the edges of the map
				if( (float)edge_distance < range)
					my_rating -= (range - (float)edge_distance) * (range - (float)edge_distance);

				if(my_rating > best_rating)
				{
					pos.x = xPos;
					pos.z = yPos;

					// buildmap allows construction, now check if otherwise blocked
					BuildMapPos2Pos(&pos, def);
					Pos2FinalBuildPos(&pos, def);

					if(cb->CanBuildAt(def, pos))
					{
						*best_pos = pos;
						best_rating = my_rating;
					}
				}
			}
		}
	}

	fclose(file);

	return best_rating;
}

int AAIMap::GetContinentID(int x, int y)
{
	return continent_map[(y/4) * xContMapSize + x / 4];
}

int AAIMap::GetContinentID(float3 *pos)
{
	int x = pos->x / 32;
	int y = pos->z / 32;

	// check if pos inside of the map
	if(x < 0)
		x = 0;
	else if(x >= xContMapSize)
		x = xContMapSize - 1;

	if(y < 0)
		y = 0;
	else if(y >= yContMapSize)
		y = yContMapSize - 1;

	return continent_map[y * xContMapSize + x];
}

int AAIMap::GetSmartContinentID(float3 *pos, unsigned int unit_movement_type)
{
	// check if non sea/amphib unit in shallow water
	if(ai->cb->GetElevation(pos->x, pos->z) < 0 && unit_movement_type & MOVE_TYPE_GROUND)
	{
		//look for closest land cell
		for(int k = 1; k < 10; ++k)
		{
			if(ai->cb->GetElevation(pos->x + k * 16, pos->z) > 0)
			{
				pos->x += k *16;
				break;
			}
			else if(ai->cb->GetElevation(pos->x - k * 16, pos->z) > 0)
			{
				pos->x -= k *16;
				break;
			}
			else if(ai->cb->GetElevation(pos->x, pos->z + k * 16) > 0)
			{
				pos->z += k *16;
				break;
			}
			else if(ai->cb->GetElevation(pos->x, pos->z - k * 16) > 0)
			{
				pos->z -= k *16;
				break;
			}
		}
	}

	int x = pos->x / 32;
	int y = pos->z / 32;

	// check if pos inside of the map
	if(x < 0)
		x = 0;
	else if(x >= xContMapSize)
		x = xContMapSize - 1;

	if(y < 0)
		y = 0;
	else if(y >= yContMapSize)
		y = yContMapSize - 1;

	return continent_map[y * xContMapSize + x];
}

bool AAIMap::LocatedOnSmallContinent(float3 *pos)
{
	return continents[GetContinentID(pos)].size < 0.25f * (avg_land_continent_size + avg_water_continent_size);
}


void AAIMap::UnitKilledAt(float3 *pos, UnitCategory category)
{
	int x = pos->x / xSectorSize;
	int y = pos->z / ySectorSize;

	if(sector[x][y].distance_to_base > 0)
		sector[x][y].lost_units[category-COMMANDER] += 1;
}

int AAIMap::GetEdgeDistance(int xPos, int yPos)
{
	int edge_distance = xPos;

	if(xMapSize - xPos < edge_distance)
		edge_distance = xMapSize - xPos;

	if(yPos < edge_distance)
		edge_distance = yPos;

	if(yMapSize - yPos < edge_distance)
		edge_distance = yMapSize - yPos;

	return edge_distance;
}

float AAIMap::GetEdgeDistance(float3 *pos)
{
	float edge_distance = pos->x;

	if(xSize - pos->x < edge_distance)
		edge_distance = xSize - pos->x;

	if(pos->z < edge_distance)
		edge_distance = pos->z;

	if(ySize - pos->z < edge_distance)
		edge_distance = ySize - pos->z;

	return edge_distance;
}
