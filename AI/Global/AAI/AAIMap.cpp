// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger

// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIMap.h"
#include "AAI.h"
#include "AAISector.h"
#include "AAIBuildTable.h"

// all the static vars
int AAIMap::aai_instances = 0;
int AAIMap::xSize;
int AAIMap::ySize;	
int AAIMap::xMapSize;
int AAIMap::yMapSize;				
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

bool AAIMap::metalMap;
MapType AAIMap::mapType;	

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

list<UnitCategory> AAIMap::map_categories;
list<int> AAIMap::map_categories_id;
vector<vector<float> > AAIMap::map_usefulness;



AAIMap::AAIMap(AAI *ai)
{
	// initialize random numbers generator
	srand ( time(NULL) );

	this->ai = ai;
	bt = ai->bt;
	cb = ai->cb;

	initialized = false;

	unitsInLos.resize(cfg->MAX_UNITS);

	// set all values to 0 (to be able to detect num. of enemies in los later
	for(int i = 0; i < cfg->MAX_UNITS; i++)
		unitsInLos[i] = 0;

	units_spotted.resize(bt->combat_categories);
}

AAIMap::~AAIMap(void)
{
	--aai_instances;

	// delete common data only if last aai instace has gone
	if(aai_instances == 0)
	{
		Learn();

		// save map data
		char filename[500];
		char buffer[500];
		strcpy(buffer, MAIN_PATH);
		strcat(buffer, MAP_LEARN_PATH);
		strcat(buffer, cb->GetMapName());
		ReplaceExtension(buffer, filename, sizeof(filename), "_");
		strcat(filename, cb->GetModName());
		ReplaceExtension(filename, buffer, sizeof(filename), ".dat");

		ai->cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);
	
		FILE *save_file = fopen(buffer, "w+");

		fprintf(save_file, "%s \n",MAP_FILE_VERSION);

		// save map type
		fprintf(save_file, "%s \n", GetMapTypeString(mapType));

		// save units map_usefulness
		float sum;

		for(int i = 0; i < cfg->SIDES; ++i)
		{
			// rebalance map_usefulness
			if(mapType == LAND_MAP)
			{
				sum = map_usefulness[0][i] + map_usefulness[2][i];
				map_usefulness[0][i] *= (100.0/sum);
				map_usefulness[2][i] *= (100.0/sum);
			}
			else if(mapType == LAND_WATER_MAP)
			{
				sum = map_usefulness[0][i] + map_usefulness[2][i] + map_usefulness[3][i] + map_usefulness[4][i];
				map_usefulness[0][i] *= (100.0/sum);
				map_usefulness[2][i] *= (100.0/sum);
				map_usefulness[3][i] *= (100.0/sum);
				map_usefulness[4][i] *= (100.0/sum);
			}
			else if(mapType == WATER_MAP)
			{
				sum = map_usefulness[2][i] + map_usefulness[3][i] + map_usefulness[4][i];
				map_usefulness[2][i] *= (100.0/sum);
				map_usefulness[3][i] *= (100.0/sum);
				map_usefulness[4][i] *= (100.0/sum);
			}

			// save
			for(int j = 0; j < bt->assault_categories.size(); ++j)
				fprintf(save_file, "%f ", map_usefulness[j][i]);
		}

		fprintf(save_file, "\n");

		for(int y = 0; y < ySectors; y++)
		{
			for(int x = 0; x < xSectors; x++)
			{
				// save sector data
				fprintf(save_file, "%f %f %f", sector[x][y].flat_ratio, sector[x][y].water_ratio, sector[x][y].importance_this_game);
				// save combat data
				for(int cat = 0; cat < bt->assault_categories.size(); ++cat)
					fprintf(save_file, "%f %f ", sector[x][y].attacked_by_this_game[cat], sector[x][y].combats_this_game[cat]);
			}

			fprintf(save_file, "\n");
		}

		fclose(save_file);

		buildmap.clear();
		blockmap.clear();
		defence_map.clear();
		air_defence_map.clear();
		plateau_map.clear();
		continent_map.clear();
	}
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

		ReadCacheFile();

		ReadContinentFile();
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
	unitsInSector.resize(xSectors*ySectors);

	// create defence
	defence_map.resize(xDefMapSize*yDefMapSize, 0);
	air_defence_map.resize(xDefMapSize*yDefMapSize, 0); 
	submarine_defence_map.resize(xDefMapSize*yDefMapSize, 0); 

	initialized = true;

	// for log file
	fprintf(ai->file, "Map: %s\n",cb->GetMapName());
	fprintf(ai->file, "Mapsize is %i x %i\n", cb->GetMapWidth(),cb->GetMapHeight());
	fprintf(ai->file, "%i sectors in x direction\n", xSectors);
	fprintf(ai->file, "%i sectors in y direction\n", ySectors);
	fprintf(ai->file, "x-sectorsize is %i (Map %i)\n", xSectorSize, xSectorSizeMap);
	fprintf(ai->file, "y-sectorsize is %i (Map %i)\n", ySectorSize, ySectorSizeMap);
	fprintf(ai->file, "%i metal spots found \n \n",metal_spots.size());
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

void AAIMap::ReadCacheFile()
{
	// try to read cache file
	bool loaded = false;
	
	char filename[500];
	char buffer[500];
	strcpy(buffer, MAIN_PATH);
	strcat(buffer, MAP_CACHE_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

	FILE *file;

	if(file = fopen(filename, "r"))
	{
		// check if correct version
		fscanf(file, "%s ", buffer);

		if(strcmp(buffer, MAP_DATA_VERSION))
		{
			cb->SendTextMsg("Mapcache out of date - creating new one", 0);
			fprintf(ai->file, "Map cache-file out of date - new one has been created\n");
			fclose(file);
		}
		else
		{
			int temp;
			float temp_float;

			// load if its a metal map
			fscanf(file, "%i ", &temp);
			metalMap = (bool)temp;

			// load buildmap 
			for(int i = 0; i < xMapSize*yMapSize; ++i)
			{
				//fread(&temp, sizeof(int), 1, file);
				fscanf(file, "%i ", &temp); 
				buildmap[i] = temp;
			}
		
			// load plateau map
			for(int i = 0; i < xMapSize*yMapSize/16; ++i)
			{
				//fread(&temp_float, sizeof(float), 1, file);
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

			fclose(file);

			fprintf(ai->file, "Map cache file succesfully loaded\n");

			loaded = true;
		}
	}

	if(!loaded)  // create new map data
	{
		// look for metalspots
		SearchMetalSpots();
	
		// detect cliffs/water and create plateau map
		AnalyseMap();

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// save mod independent map data
		strcpy(buffer, MAIN_PATH);
		strcat(buffer, MAP_CACHE_PATH);
		strcat(buffer, cb->GetMapName());
		ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

		ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

		file = fopen(filename, "w+");

		fprintf(file, "%s\n", MAP_DATA_VERSION);

		// save if its a metal map
		fprintf(file, "%i\n", (int)metalMap);

		// save buildmap
		for(int i = 0; i < xMapSize*yMapSize; ++i)
			fprintf(file, "%i ", buildmap[i]); 

		fprintf(file, "\n");

		// save plateau map
		for(int i = 0; i < xMapSize*yMapSize/16; ++i)
			fprintf(file, "%f ", plateau_map[i]); 

		// save mex spots
		fprintf(file, "\n%i \n", metal_spots.size());

		for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); spot++)
			fprintf(file, "%f %f %f %f \n", spot->pos.x, spot->pos.y, spot->pos.z, spot->amount);

		fclose(file);

		fprintf(ai->file, "New map cache-file created\n");
	}

	// determine map type
	loaded = true;
	strcpy(buffer, MAIN_PATH);
	strcat(buffer, MAP_CFG_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".cfg");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

	if(file = fopen(filename, "r"))
	{
		// read map type
		fscanf(file, "%s ", buffer);

		if(!strcmp(buffer, "LAND_MAP"))
			mapType = LAND_MAP;
		else if(!strcmp(buffer, "AIR_MAP"))
			mapType = AIR_MAP;
		else if(!strcmp(buffer, "LAND_WATER_MAP"))
			mapType = LAND_WATER_MAP;
		else if(!strcmp(buffer, "WATER_MAP"))
			mapType = WATER_MAP;
		else
			mapType = UNKNOWN_MAP;

		if(mapType >= 0 && mapType <= WATER_MAP)
		{
			this->mapType = (MapType) mapType;

			// logging
			sprintf(buffer, "%s detected", GetMapTypeTextString(mapType));
			fprintf(ai->file, "\nLoading map type:\n");
			fprintf(ai->file, buffer);
			fprintf(ai->file, "\n\n");

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
		float water_ratio = 0;

		for(int x = 0; x < xMapSize; ++x)
		{
			for(int y = 0; y < yMapSize; ++y)
			{
				if(buildmap[x + y*xMapSize] == 4)
					++water_ratio;
			}
		}

		water_ratio = water_ratio / ((float)(xMapSize*yMapSize));

		if(water_ratio > 0.80f)
			this->mapType = WATER_MAP;
		else if(water_ratio > 0.25f)
			this->mapType = LAND_WATER_MAP;
		else
			this->mapType = LAND_MAP;


		// logging
		sprintf(buffer, "%s detected", GetMapTypeTextString(this->mapType));
		ai->cb->SendTextMsg(buffer, 0);
		fprintf(ai->file, "\nAutodetecting map type:\n");
		fprintf(ai->file, buffer);
		fprintf(ai->file, "\n\n");

		// save results to cfg file
		strcpy(buffer, MAIN_PATH);
		strcat(buffer, MAP_CFG_PATH);
		strcat(buffer, cb->GetMapName());
		ReplaceExtension(buffer, filename, sizeof(filename), ".cfg");

		ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

		file = fopen(filename, "w+");
		fprintf(file, "%s\n", GetMapTypeString(this->mapType));
		fclose(file);
	}

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
		if(mapType == LAND_MAP)
		{
			map_categories.push_back(GROUND_ASSAULT);
			map_categories.push_back(AIR_ASSAULT);
			map_categories.push_back(HOVER_ASSAULT);

			map_categories_id.push_back(0);
			map_categories_id.push_back(1);
			map_categories_id.push_back(2);
		}
		else if(mapType == LAND_WATER_MAP)
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
		else if(mapType == WATER_MAP)
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
	char filename[500];
	char buffer[500];
	strcpy(buffer, MAIN_PATH);
	strcat(buffer, MAP_CACHE_PATH);
	strcat(buffer, cb->GetMapName());
	strcat(buffer, "_");
	strcat(buffer, cb->GetModName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

	FILE *file;

	if(file = fopen(filename, "r"))
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
			fscanf(file, "%i %i %i %i", &land_continents, &water_continents, &avg_land_continent_size, &avg_water_continent_size);

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
	strcpy(buffer, MAIN_PATH);
	strcat(buffer, MAP_CACHE_PATH);
	strcat(buffer, cb->GetMapName());
	strcat(buffer, "_");
	strcat(buffer, cb->GetModName());
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

	for(int c = 0; c < continents.size(); ++c)
		fprintf(file, "%i %i \n", continents[c].size, (int)continents[c].water);

	// save statistical data
	fprintf(file, "%i %i %i %i\n", land_continents, water_continents, avg_land_continent_size, avg_water_continent_size);

	fclose(file);
}

void AAIMap::ReadMapLearnFile(bool auto_set)
{
	// get filename
	char filename[500];
	char buffer[500];

	strcpy(buffer, MAIN_PATH);
	strcat(buffer, MAP_LEARN_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), "_");
	strcpy(buffer, filename);
	strcat(buffer, cb->GetModName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

	// open learning files
	FILE *load_file = fopen(filename, "r");

	// check if correct map file version
	if(load_file)
	{
		fscanf(load_file, "%s", buffer);

		// file version out of date
		if(strcmp(buffer, MAP_FILE_VERSION))
		{
			cb->SendTextMsg("Map learning file version out of date, creating new one", 0);
			fclose(load_file);
			load_file = 0;
		}
		else
		{
			// check if map type matches (aai will recreate map learn files if maptype has changed)
			fscanf(load_file, "%s", buffer);

			// map type does not match
			if(strcmp(buffer, GetMapTypeString(mapType)))
			{
				cb->SendTextMsg("Map type has changed - creating new map learning file", 0);
				fclose(load_file);
				load_file = 0;
			}
			else
			{
				if(auto_set && aai_instances == 1)
				{
					map_usefulness.resize(bt->assault_categories.size());

					for(int i = 0; i < bt->assault_categories.size(); ++i)
						map_usefulness[i].resize(cfg->SIDES);

					// load units map_usefulness
					for(int i = 0; i < cfg->SIDES; ++i)
					{
						for(int j = 0; j < bt->assault_categories.size(); ++j)
							fscanf(load_file, "%f ", &map_usefulness[j][i]);
					}
				}
				else
				{
					// pseudo-load to make sure to be at the right spot in the file
					float dummy;

					for(int i = 0; i < cfg->SIDES; ++i)
					{
						for(int j = 0; j < bt->assault_categories.size(); ++j)
							fscanf(load_file, "%f ", &dummy);
					}
				}
			}
		}
	}

	if(!load_file && aai_instances == 1)
	{
		map_usefulness.resize(bt->assault_categories.size());

		for(int i = 0; i < bt->assault_categories.size(); ++i)
			map_usefulness[i].resize(cfg->SIDES);

		for(int i = 0; i < cfg->SIDES; ++i)
		{
			for(int j = 0; j < bt->assault_categories.size(); ++j)
				map_usefulness[j][i] = bt->mod_usefulness[j][i][mapType];
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
				for(int cat = 0; cat < bt->assault_categories.size(); cat++)
					fscanf(load_file, "%f %f ", &sector[i][j].attacked_by_learned[cat], &sector[i][j].combats_learned[cat]);

				if(auto_set)
				{
					sector[i][j].importance_this_game = sector[i][j].importance_learned;

					for(int cat = 0; cat < bt->assault_categories.size(); ++cat)
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

					for(int cat = 0; cat < bt->assault_categories.size(); ++cat)
					{
						sector[i][j].attacked_by_this_game[cat] = sector[i][j].attacked_by_learned[cat];
						sector[i][j].combats_this_game[cat] = sector[i][j].combats_learned[cat];
					}
				}
			}
		}
	}

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

			for(int cat = 0; cat < bt->assault_categories.size(); ++cat)
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
	pos->z -= def->ysize/2;

	// check if pos is still in that map, otherwise retun 0
	if(pos->x < 0 && pos->z < 0)
		pos->x = pos->z = 0;
}

void AAIMap::BuildMapPos2Pos(float3 *pos, const UnitDef *def)
{
	// shift to middlepoint
	pos->x += def->xsize/2;
	pos->z += def->ysize/2;

	// back to unit coordinates
	pos->x *= SQUARE_SIZE;
	pos->z *= SQUARE_SIZE;
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
		for(int y = yPos; y < yPos + ySize; y++)
		{
			if(y>= yMapSize)
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
			for(int y = yPos+ySize; y < yPos+ySize+cfg->MAX_YROW; y++)
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

				for(int y = yPos-1; y >= yPos - cfg->MAX_YROW; y--)
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
	// check range
	if(xPos < 0 || yPos < 0 || xPos+width > xMapSize || yPos + height > yMapSize)
		return;

	//float3 my_pos;
	int empty, cell;

	if(water)
		empty = 4;
	else
		empty = 0;

	if(block)	// block cells
	{
		for(int x = xPos; x < xPos + width; x++)
		{
			for(int y = yPos; y < yPos + height; y++)
			{
				cell = x + xMapSize*y;

				// if no building ordered that cell to be blocked, update buildmap
				// (only if space is not already occupied by a building)
				if(!(blockmap[cell]++))
				{
					if(buildmap[cell] == empty)
					{
						buildmap[cell] = 2;

						/*// debug
						if(x%2 == 0 && y%2 == 0)
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
	else		// unblock cells
	{
		for(int x = xPos; x < xPos + width; x++)
		{
			for(int y = yPos; y < yPos + height; y++)
			{
				cell = x + xMapSize*y;

				if(blockmap[cell])
				{
					// if cell is not blocked anymore, update buildmap
					if(!(--blockmap[cell]))
					{
						if(buildmap[cell] == 2)
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
}


void AAIMap::Pos2FinalBuildPos(float3 *pos, const UnitDef* def)
{
	if(def->xsize&2) // check if xsize is a multiple of 4
		pos->x=floor((pos->x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos->x=floor((pos->x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(def->ysize&2)
		pos->z=floor((pos->z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos->z=floor((pos->z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
}

int AAIMap::GetNextX(int direction, int xPos, int yPos, int value)
{
	int x = xPos;
	// scan line until next free cell found
	while(buildmap[x+yPos*xMapSize] == value)
	{
		if(direction)
			x++;
		else
			x--;

		// search went out of map
		if(x < 0 || x >= xMapSize)
			return -1;
	}

	return x;
}

int AAIMap::GetNextY(int direction, int xPos, int yPos, int value)
{
	int y = yPos;
	// scan line until next free cell found
	while(buildmap[xPos+y*xMapSize] == value)
	{
		if(direction)
			y++;
		else
			y--;

		// search went out of map
		if(y < 0 || y >= yMapSize)
			return -1;
	}

	return y;
}

void AAIMap::GetSize(const UnitDef *def, int *xSize, int *ySize)
{
	// calculate size of building
	*xSize = def->xsize;
	*ySize = def->ysize;

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
	for(int x = xPos; x < xPos + xSize; x++)
	{
		for(int y = yPos; y < yPos + ySize; y++)
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
	for(int x = xPos; x < xPos + xSectorSizeMap; x++)
	{
		for(int y = yPos; y < yPos + ySectorSizeMap; y++)
		{
			if(buildmap[x+y*xMapSize] == 3)
				cliffs++;
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

	for(int i = 0; i < continents.size(); ++i)
	{
		if(continents[i].water)
		{
			++water_continents;
			avg_water_continent_size += continents[i].size;
		}
		else
		{
			++land_continents;
			avg_land_continent_size += continents[i].size;
		}
	}

	if(water_continents > 0)
		avg_water_continent_size /= water_continents;

	if(land_continents > 0)
		avg_land_continent_size /= land_continents;
}

// algorithm more or less by krogothe
// thx very much
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

	TotalMetal = MaxMetal = Stopme =  SpotsFound = 0; //clear variables just in case!

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
					if(CanBuildAt(pos.x, pos.z, def->xsize, def->ysize))
					{
						metal_spots.push_back(temp);
						SpotsFound++;

						if(pos.y >= 0)
							SetBuildMap(pos.x-2, pos.z-2, def->xsize+4, def->ysize+4, 1);
						else
							SetBuildMap(pos.x-2, pos.z-2, def->xsize+4, def->ysize+4, 5);
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
		fprintf(ai->file, "Map is considered to be a metal map\n",0);
	}
	else
	{
		metalMap = false;

		// debug
		/*for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); spot++)
		{
			cb->DrawUnit("ARMMEX", spot->pos, 0.0f, 200000, cb->GetMyAllyTeam(), false, true);
		}*/
	}


	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] TempAverage;
}

void AAIMap::GetMapData()
{
}

void AAIMap::UpdateRecon()
{
	const UnitDef *def;
	int team, my_team = cb->GetMyAllyTeam();
	float3 pos;
	int x, y;
	AAIAirTarget target;
	int combat_category_id;
	AAISector *sector;
	UnitCategory cat;

	// prevent aai from wasting too much time with checking too many targets
	int targets_checked = 0;

	// reset number of friendly units per sector && decrease mobile combat power (to reflect possible movement the units)
	for(x = 0; x < xSectors; ++x)
	{
		for(y = 0; y < ySectors; ++y)
		{
			unitsInSector[x+y*xSectors] = 0;

			for(int i = 0; i < bt->combat_categories; ++i)
			{
				this->sector[x][y].mobile_combat_power[i] *= 0.98f;

				if(this->sector[x][y].mobile_combat_power[i] < 0.5f)
					this->sector[x][y].mobile_combat_power[i] = 0;
			}
		}
	}

	// reset spotted units
	fill(units_spotted.begin(), units_spotted.end(), 0);

	// first update all sectors with friendly units
	int number_of_units = cb->GetFriendlyUnits(&(unitsInLos.front()));

	// go through the list
	for(int i = 0; i < number_of_units; ++i)
	{
		pos = cb->GetUnitPos(unitsInLos[i]);

		if(pos.x != 0)
		{
			// calculate in which sector unit is located
			x = pos.x/xSectorSize;
			y = pos.z/ySectorSize;

			if(x >= 0 && y >= 0 && x < xSectors && y < ySectors)
			{
				sector = &this->sector[x][y];

				// we have a unit in that sector, reset values if not already done
				if(unitsInSector[x+y*xSectors] == 0)
				{
					++unitsInSector[x+y*xSectors];
						
					fill(sector->enemyUnitsOfType.begin(), sector->enemyUnitsOfType.end(), 0);
					fill(sector->stat_combat_power.begin(), sector->stat_combat_power.end(), 0);
					fill(sector->mobile_combat_power.begin(), sector->mobile_combat_power.end(), 0);
				
					sector->threat = 0;
					sector->allied_structures = 0;
					sector->enemy_structures = 0;
					sector->own_structures = 0;
				}

				// check if building
				def = cb->GetUnitDef(unitsInLos[i]);

				if(def && bt->units_static[def->id].category <= METAL_MAKER)
				{
					team = cb->GetUnitTeam(unitsInLos[i]);

					if(team == my_team)
						sector->own_structures += bt->units_static[def->id].cost;
					else
						sector->allied_structures += bt->units_static[def->id].cost;
				}
			}
		}
	}

	// get all enemies in los
	number_of_units = cb->GetEnemyUnits(&(unitsInLos.front()));

	// go through the list
	for(int i = 0; i < number_of_units; ++i)
	{	
		pos = cb->GetUnitPos(unitsInLos[i]);

		// calculate in which sector unit is located
		sector = GetSectorOfPos(&pos);

		if(sector)
		{
			if(unitsInSector[sector->x + sector->y*xSectors])
			{
				def = cb->GetUnitDef(unitsInLos[i]);
				cat = bt->units_static[def->id].category;
				combat_category_id = (int) bt->GetIDOfAssaultCategory(cat);

				sector->enemyUnitsOfType[(int)cat] += 1;

					// check if combat unit
				if(combat_category_id >= 0)
				{
					sector->threat += 1;
					units_spotted[combat_category_id] += 1;

					// count combat power of combat units
					for(int i = 0; i < bt->combat_categories; ++i)
						sector->mobile_combat_power[i] += bt->units_static[def->id].efficiency[i];
				}
				else	// building or scout etc.
				{
					// check if promising bombing target
					if(targets_checked < 3 && ( cat ==  EXTRACTOR || cat == STATIONARY_CONSTRUCTOR || cat == STATIONARY_ARTY || cat == STATIONARY_LAUNCHER ) )
					{
						// dont check targets that are already on bombing list
						if(ai->ut->units[unitsInLos[i]].status != BOMB_TARGET)
						{
							ai->af->CheckBombTarget(unitsInLos[i], def->id);
							targets_checked += 1;

							cb->SendTextMsg("Checking target", 0);
						}
					}

					// count combat power of stat defences
					if(cat == STATIONARY_DEF)
					{
						for(int i = 0; i < bt->ass_categories; ++i)
							sector->stat_combat_power[i] += bt->units_static[def->id].efficiency[i];
					}

					// add enemy buildings
					if(cat <= METAL_MAKER)
						sector->enemy_structures += bt->units_static[def->id].cost;
				}
			}
		}
	}

	ai->brain->UpdateMaxCombatUnitsSpotted(units_spotted);
}

void AAIMap::UpdateSectors()
{
	for(int x = 0; x < xSectors; ++x)
	{
		for(int y = 0; y < ySectors; ++y)
			sector[x][y].Update();
	}
}


void AAIMap::UpdateCategoryUsefulness(const UnitDef *killer_def, int killer, const UnitDef *killed_def, int killed)
{
	// filter out aircraft
	if(killer == 1 || killed == 1)
		return;

	UnitTypeStatic *killer_type = &bt->units_static[killer_def->id];
	UnitTypeStatic *killed_type = &bt->units_static[killed_def->id];

	float change =  killed_type->cost / killer_type->cost;

	change /= 4;

	if(change > 4)
		change = 4;
	else if(change < 0.2)
		change = 0.2f;

	if(killer < 5)
	{
		bt->mod_usefulness[killer][killer_type->side-1][mapType] += change;
		map_usefulness[killer][killer_type->side-1] += change;
	}

	if(killed < 5)
	{
		map_usefulness[killed][killed_type->side-1] -= change;
		bt->mod_usefulness[killed][killed_type->side-1][mapType] -= change;

		if(map_usefulness[killed][killed_type->side-1] < 1)
			map_usefulness[killed][killed_type->side-1] = 1;

		if(bt->mod_usefulness[killed][killed_type->side-1][mapType] < 1)
			bt->mod_usefulness[killed][killed_type->side-1][mapType] = 1;
	}
}


const char* AAIMap::GetMapTypeString(int mapType)
{
	if(mapType == 1)
		return "LAND_MAP";
	else if(mapType == 2)
		return "AIR_MAP";
	else if(mapType == 3)
		return "LAND_WATER_MAP";
	else if(mapType == 4)
		return "WATER_MAP";
	else
		return "UNKNOWN_MAP";
}

const char* AAIMap::GetMapTypeTextString(int mapType)
{
	if(mapType == 1)
		return "land map";
	else if(mapType == 2)
		return "air map";
	else if(mapType == 3)
		return "land-water map";
	else if(mapType == 4)
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
	int range = bt->units_static[defence].range / 32;
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

	int xPos = pos->x / 32;
	int yPos = pos->z / 32;

	// x range will change from line to line 
	int xStart;
	int xEnd;
	int xRange;

	// y range is const
	int yStart = yPos - range;
	int yEnd = yPos + range;

	if(yStart < 0)
		yStart = 0;
	if(yEnd >= yDefMapSize)
		yEnd = yDefMapSize-1;

	for(int y = yStart; y <= yEnd; ++y)
	{
		// determine x-range
		xRange = (int) floor( sqrt( (float) ( range * range - (y - yPos) * (y - yPos) ) ) + 0.5f );
		
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
	xStart = xPos - 2;
	xEnd = xPos + 2;
	yStart = yPos - 2;
	yEnd = yPos + 2;

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
			
			defence_map[cell] += 128.0f;
			air_defence_map[cell] += 128.0f;
			submarine_defence_map[cell] += 128.0f;
		}
	}
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
	
	int xPos = pos->x / 32;
	int yPos = pos->z / 32;

	// further decrease values close around the bulding (to prevent aai from packing buildings too close together)
	int xStart = xPos - 2;
	int xEnd = xPos + 2;
	int yStart = yPos - 2;
	int yEnd = yPos + 2;

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
			
			defence_map[cell] -= 128.0f;
			air_defence_map[cell] -= 128.0f;
			submarine_defence_map[cell] -= 128.0f;
		}
	}

	// y range is const
	int xRange;
	yStart = yPos - range;
	yEnd = yPos + range;

	if(yStart < 0)
		yStart = 0;
	if(yEnd >= yDefMapSize)
		yEnd = yDefMapSize-1;

	for(int y = yStart; y <= yEnd; ++y)
	{
		// determine x-range
		xRange = (int) floor( sqrt( (float) ( range * range - (y - yPos) * (y - yPos) ) ) + 0.5f );
		
		xStart = xPos - xRange;
		xEnd = xPos + xRange;

		if(xStart < 0)
			xStart = 0;
		if(xEnd > xDefMapSize)
			xEnd = xDefMapSize;

		for(int x = xStart; x < xEnd; ++x)
		{
			cell= x + xDefMapSize*y;
			
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
	float my_rating, best_rating = -10000;
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

	// check rect
	for(yPos = yStart; yPos < yEnd; yPos += 4)
	{
		for(xPos = xStart; xPos < xEnd; xPos += 4)
		{
			// check if buildmap allows construction
			if(CanBuildAt(xPos, yPos, xSize, ySize, water))
			{
				cell = (xPos + xDefMapSize * yPos) / 4;

				my_rating = terrain_modifier * plateau_map[cell] - (*map)[cell] + 0.15f *  (float)(rand()%20);

				// determine minimum distance from buildpos to the edges of the map
				edge_distance = xPos;

				if(xMapSize - xPos < edge_distance)
					edge_distance = xMapSize - xPos;

				if(yPos < edge_distance)
					edge_distance = yPos;

				if(yMapSize - yPos < edge_distance)
					edge_distance = yMapSize - yPos;

				// prevent aai from building defences too close to the edges of the map
				if( (float)edge_distance < range)
					my_rating -= (range - (float)edge_distance);	

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
