#include "AAIMap.h"

#include "AAI.h"
#include "AAISector.h"
#include "AAIBuildTable.h"

AAIMap::AAIMap(AAI *ai)
{
	// initialize random numbers generator
	srand ( time(NULL) );

	this->ai = ai;
	bt = ai->bt;
	cb = ai->cb;

	sector = 0;
	xSectors = ySectors = 0;

	initialized = false;
	
	blockmap = 0;
	buildmap = 0;
	mapType = LAND_MAP;

	unitsInLos = new int[cfg->MAX_UNITS];
	
	// set all values to 0 (to be able to detect num. of enemies in los later
	for(int i = 0; i < cfg->MAX_UNITS; i++)
		unitsInLos[i] = 0;

	map_usefulness = new float*[bt->assault_categories.size()];

	for(int i = 0; i < bt->assault_categories.size(); ++i)
		map_usefulness[i] = new float[cfg->SIDES];

	units_spotted = new float[bt->combat_categories];
}

AAIMap::~AAIMap(void)
{
	delete [] unitsInLos;
	delete [] unitsInSector;
	delete [] units_spotted;

	Learn();

	// save map data
	char filename[120];
	char buffer[120];
	strcpy(buffer, AI_PATH);
	strcat(buffer, MAP_LEARN_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), "_");
	strcat(filename, cb->GetModName());
	ReplaceExtension(filename, buffer, sizeof(filename), ".dat");
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
			fprintf(save_file, "%f %f %f %f ", sector[x][y].flat_ratio, sector[x][y].water_ratio, 
											sector[x][y].importance[0], sector[x][y].arty_efficiency[0]);
			// save combat data
			for(int cat = 0; cat < bt->assault_categories.size(); cat++)
				fprintf(save_file, "%f %f ", sector[x][y].attacked_by[0][cat], sector[x][y].combats[0][cat]);
		}

		fprintf(save_file, "\n");
	}

	fclose(save_file);

	// clean up sectors..
	if(sector)
	{
		for(int i = 0; i < xSectors; i++)
			delete [] sector[i];
		
		delete [] sector;
	}

	// clean up buildmap
	if(buildmap)	
		delete [] buildmap;

	if(blockmap)	
		delete [] blockmap;

	for(int i = 0; i < bt->assault_categories.size(); ++i)
		delete [] map_usefulness[i];

	delete [] map_usefulness;
}

void AAIMap::Init()
{
	// get size
	xMapSize = cb->GetMapWidth();
	yMapSize = cb->GetMapHeight();

	// calculate number of sectors
	xSectors = floor(0.5 + ((float) xMapSize)/cfg->SECTOR_SIZE);
	ySectors = floor(0.5 + ((float) yMapSize)/cfg->SECTOR_SIZE);

	// calculate effective sector size
	xSectorSizeMap = floor( ((float) xMapSize) / ((float) xSectors) );
	ySectorSizeMap = floor( ((float) yMapSize) / ((float) ySectors) ); 

	xSectorSize = 8 * xSectorSizeMap;
	ySectorSize = 8 * ySectorSizeMap;
	
	// create field of sectors
	sector = new AAISector*[xSectors];

	for(int x = 0; x < xSectors; x++)
		sector[x] = new AAISector[ySectors];

	// for scouting
	unitsInSector = new int[xSectors*ySectors];

	// create buildmap
	buildmap = new char[xMapSize*yMapSize];

	// create blockmap
	blockmap = new char[xMapSize*yMapSize];

	for(int i = 0; i < xMapSize*yMapSize; i++)
	{
		buildmap[i] = 0;
		blockmap[i] = 0;
	}

	// try to read cache file
	bool loaded = false;
	char filename[100];
	char buffer[100];
	strcpy(buffer, AI_PATH);
	strcat(buffer, MAP_CACHE_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

	FILE *file;

	if(file = fopen(filename, "r"))
	{
		// check if correct version
		fscanf(file, "%s ", buffer);

		if(strcmp(buffer, MAP_DATA_VERSION))
		{
			cb->SendTextMsg("Mapcache out of date - creating new one", 0);
			fprintf(ai->file, "Map cache-file out of date - new one has been created\n");
		}
		else
		{
			int temp;

			// load if its a metal map
			fscanf(file, "%i ", &temp);
			metalMap = (bool)temp;
			
			// load buildmap first
			fread(buildmap, sizeof(char), xMapSize*yMapSize, file);
			
			AAIMetalSpot spot;
			int spots;
			fscanf(file, "%i ", &spots);

			// load mex spots
			for(int i = 0; i < spots; i++)
			{
				fscanf(file, "%f %f %f %f ", &(spot.pos.x), &(spot.pos.y), &(spot.pos.z), &(spot.amount));
				spot.occupied = false;
				metal_spots.push_back(spot);
			}	

			fprintf(ai->file, "Map cache file succesfully loaded\n");

			loaded = true;

			fclose(file);
		}
	}
	if(!loaded)  // create new map data
	{
		// look for metalspots
		SearchMetalSpots();

		// scan buildmap for cliffs
		DetectCliffs();

		// scan for water
		DetectWater();

		// save data
		file = fopen(filename, "w+");

		fprintf(file, "%s\n",  MAP_DATA_VERSION);

		// save if its a metal map
		fprintf(file, "%i\n", (int)metalMap);

		// save buildmap
		fwrite(buildmap, sizeof(char), xMapSize*yMapSize, file);
	
		// save mex spots
		fprintf(file, " %i \n", metal_spots.size());

		for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); spot++)
			fprintf(file, "%f %f %f %f \n", spot->pos.x, spot->pos.y, spot->pos.z, spot->amount);
		

		fclose(file);

		fprintf(ai->file, "New map cache-file created\n");
	}

	// determine map type
	loaded = true;
	strcpy(buffer, AI_PATH);
	strcat(buffer, MAP_CFG_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".cfg");

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

		if(water_ratio > 0.80)
			this->mapType = WATER_MAP;
		else if(water_ratio > 0.25)
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
		strcpy(buffer, AI_PATH);
		strcat(buffer, MAP_CFG_PATH);
		strcat(buffer, cb->GetMapName());
		ReplaceExtension(buffer, filename, sizeof(filename), ".cfg");

		file = fopen(filename, "w+");
		fprintf(file, "%s\n", GetMapTypeString(this->mapType));
		fclose(file);
	}

	// add metalspots to their sectors
	int k, l;
	for(list<AAIMetalSpot>::iterator i = metal_spots.begin(); i != metal_spots.end(); i++)
	{
		k = i->pos.x/xSectorSize;
		l = i->pos.z/ySectorSize;
		
		if(k < xSectors && l < ySectors)
			sector[k][l].AddMetalSpot(&(*i));
	}

	// read data from learning file  
	ReadMapLearnFile(true);

	initialized = true;
	
	// for log file
	fprintf(ai->file, "Map: %s\n",cb->GetMapName());
	fprintf(ai->file, "Mapsize is %i x %i\n", cb->GetMapWidth(),cb->GetMapHeight());
	fprintf(ai->file, "%i sectors in x direction\n", xSectors);
	fprintf(ai->file, "%i sectors in y direction\n", ySectors);
	fprintf(ai->file, "x-sectorsize is %i (Map %i)\n", xSectorSize, xSectorSizeMap);
	fprintf(ai->file, "y-sectorsize is %i (Map %i)\n", ySectorSize, ySectorSizeMap);
	fprintf(ai->file, "%i metal spots found \n \n",metal_spots.size());

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

void AAIMap::ReadMapLearnFile(bool auto_set)
{
	// get filename
	char filename[120];
	char buffer[120];

	strcpy(buffer, AI_PATH);
	strcat(buffer, MAP_LEARN_PATH);
	strcat(buffer, cb->GetMapName());
	ReplaceExtension(buffer, filename, sizeof(filename), "_");
	strcpy(buffer, filename);
	strcat(buffer, cb->GetModName());
	ReplaceExtension(buffer, filename, sizeof(filename), ".dat");

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
				if(auto_set)
				{
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

	if(!load_file)
	{
		for(int i = 0; i < cfg->SIDES; ++i)
		{
			map_usefulness[0][i] = bt->mod_usefulness[0][i][mapType];
			map_usefulness[1][i] = bt->mod_usefulness[1][i][mapType];
			map_usefulness[2][i] = bt->mod_usefulness[2][i][mapType];
			map_usefulness[3][i] = bt->mod_usefulness[3][i][mapType];
			map_usefulness[4][i] = bt->mod_usefulness[4][i][mapType];
		}
	}

	for(int j = 0; j < ySectors; ++j)
	{
		for(int i = 0; i < xSectors; ++i)
		{
			if(auto_set)
			{
				// set coordinates of the sectors
				sector[i][j].SetCoordinates(xSectorSize*i, xSectorSize*(i+1), ySectorSize * j, ySectorSize * (j+1));
				sector[i][j].SetGridLocation(i,j);
				// provide ai callback to sectors
				sector[i][j].SetAI(ai);
				sector[i][j].map = this;
			}

			// load sector data from file or init with default values
			if(load_file)
			{
				// load sector data
				fscanf(load_file, "%f %f %f %f ", &sector[i][j].flat_ratio, &sector[i][j].water_ratio, 
									&sector[i][j].importance[1], &sector[i][j].arty_efficiency[1]);
				// load combat data
				for(int cat = 0; cat < bt->assault_categories.size(); cat++)
					fscanf(load_file, "%f %f ", &sector[i][j].attacked_by[1][cat], &sector[i][j].combats[1][cat]);

				if(sector[i][j].importance[1] == 1)
					sector[i][j].importance[1] += (rand()%5)/20.0;	
			}
			else
			{
				sector[i][j].importance[1] = 1 + (rand()%5)/20.0;
				sector[i][j].flat_ratio = sector[i][j].GetFlatRatio();
				sector[i][j].water_ratio = sector[i][j].GetWaterRatio();
				sector[i][j].arty_efficiency[1] = 5 + 15 * sector[i][j].GetMapBorderDist();

				for(int cat = 0; cat < bt->assault_categories.size(); cat++)
				{
					sector[i][j].attacked_by[1][cat] = 1;
					sector[i][j].combats[1][cat] = 1;
				}
			}

			if(auto_set)
			{
				sector[i][j].importance[0] = sector[i][j].importance[1];
				sector[i][j].arty_efficiency[0] = sector[i][j].arty_efficiency[1];

				for(int cat = 0; cat < bt->assault_categories.size(); cat++)
				{
					sector[i][j].attacked_by[0][cat] = sector[i][j].attacked_by[1][cat];
					sector[i][j].combats[0][cat] = sector[i][j].combats[1][cat];
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

	// load current learned values from file
	ReadMapLearnFile(false);

	// go
	for(int y = 0; y < ySectors; y++)
	{
		for(int x = 0; x < xSectors; x++)
		{
			sector = &this->sector[x][y];

			sector->importance[0] = 0.93 * (sector->importance[0] + 3 * sector->importance[1])/4.0;
			
			if(sector->importance[0] < 1)
				sector->importance[0] = 1;

			sector->arty_efficiency[0] = 0.96 * (sector->arty_efficiency[0] + 3 * sector->arty_efficiency[1])/4.0;
		
			if(sector->arty_efficiency[0] < 1)
				sector->arty_efficiency[0]  = 1;

			for(int cat = 0; cat < bt->assault_categories.size(); cat++)
			{
				sector->attacked_by[0][cat] = 0.9 * (sector->attacked_by[0][cat] + 3 * sector->attacked_by[1][cat])/4.0;
				if(sector->attacked_by[0][cat] < 1)
					sector->attacked_by[0][cat] = 1;

				sector->combats[0][cat] = 0.9 * (sector->combats[0][cat] + 3 * sector->combats[1][cat])/4.0;
			
				if(sector->combats[0][cat] < 1)
					sector->combats[0][cat] = 1;
			}
		}
	}
}

// converts unit positions to cell coordinates
void AAIMap::Pos2BuildMapPos(float3 *pos, const UnitDef* def)
{
	if(!pos)
	{
		fprintf(ai->file, "ERROR: Null-pointer float3 *pos in Pos2BuildMapPos()");
		return;
	}

	if(!def)
	{
		fprintf(ai->file, "ERROR: Null-pointer UnitDef *def in Pos2BuildMapPos()");
		return;
	}

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
				if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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
				if(CanBuildAt(pos.x, pos.z - vIterator, xSize, ySize, water))
				{
					temp_pos = float3(pos.x, 0, pos.z - vIterator);

					if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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
				else if(CanBuildAt(pos.x, pos.z + vIterator, xSize, ySize, water))
				{
					temp_pos = float3(pos.x, 0, pos.z + vIterator);

					if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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

		hIterator += 2;

		if(hCenter - hIterator < xStart || hCenter + hIterator > xEnd)
			hStop = true;

		if(!hStop)
		{
			while(pos.z < vCenter+vIterator)
			{
				// check if buildmap allows construction
				if(CanBuildAt(pos.x - hIterator, pos.z, xSize, ySize, water))
				{
					temp_pos = float3(pos.x - hIterator, 0, pos.z);

					if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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
				else if(CanBuildAt(pos.x + hIterator, pos.z + vIterator, xSize, ySize, water))
				{
					temp_pos = float3(pos.x + hIterator, 0, pos.z);

					if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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
			if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)		 
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

	if(xEnd > xSectors * xSectorSizeMap) 
		xEnd = xSectors * xSectorSizeMap;

	if(yStart < 0) 
		yStart = 0;

	if(yEnd > ySectors * ySectorSizeMap) 
		yEnd = ySectors * ySectorSizeMap;

	return GetCenterBuildsite(def, xStart, xEnd, yStart, yEnd, water);
}

bool AAIMap::CanBuildAt(int xPos, int yPos, int xSize, int ySize, bool water)
{
	if(xPos+xSize <= xMapSize && yPos+ySize <= yMapSize)
	{
		// check if all squares the building needs are empty
		for(int x = xPos; x < xSize+xPos; x++)
		{
			for(int y = yPos; y < ySize+yPos; y++)
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
	if(bt->units_static[def->id].category == GROUND_FACTORY || bt->units_static[def->id].category == SEA_FACTORY)
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
				cliffs++;
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

void AAIMap::DetectCliffs()
{
	float3 my_pos;

	float slope;

	const float *height_map = cb->GetHeightMap();

	for(int x = 0; x < (xMapSize-4); x++)
	{
		for(int y = 0; y < (yMapSize-4); y++)
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

void AAIMap::DetectWater()
{
	float3 my_pos;

	const float *height_map = cb->GetHeightMap();

	for(int x = 0; x < xMapSize; x++)
	{
		for(int y = 0; y < yMapSize; y++)
		{
			// check heighth
			if(height_map[y * xMapSize + x] < 0)
			{
				buildmap[x+y*xMapSize] = 4;

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
	int type, myTeam = cb->GetMyAllyTeam();
	float3 pos;
	int x, y;
	AAIAirTarget target;
	int combat_category_id;

	// reset spotted units
	for(int i = 0; i < bt->combat_categories; ++i)
		units_spotted[i] = 0;

	// first update all sectors with friendly units
	cb->GetFriendlyUnits(unitsInLos);

	// go through the list 
	for(int i = 0; i < cfg->MAX_UNITS; ++i)
	{
		// if 0 end of array reached
		if(unitsInLos[i])
		{
			pos = cb->GetUnitPos(unitsInLos[i]);
			AAISector *sector;

			if(pos.x != 0)
			{
				// calculate in which sector unit is located
				x = pos.x/xSectorSize;
				y = pos.z/ySectorSize;
				
				if(x >= 0 && y >= 0 && x < xSectors && y < ySectors)
				{	
					sector = &this->sector[x][y];

					if(!unitsInSector[x+y*xSectors] && sector->distance_to_base > 0)
					{
						++unitsInSector[x+y*xSectors];
						sector->enemy_structures = 0;

						sector->threat_against[0] = 0;
						sector->threat_against[1] = 0;
						sector->threat_against[2] = 0;
						sector->threat_against[3] = 0;

						//for(int i = 1; i <= (int)BUILDER; i++)
						//	sector[x][y].enemyUnitsOfType[i] = 0;
					}
				}
			}

			unitsInLos[i] = 0;
		}
		else 	// end of arry reached
			break;	
	}

	// get all enemies in los
	cb->GetEnemyUnits(unitsInLos);

	// go through the list 
	for(int i = 0; i < cfg->MAX_UNITS; ++i)
	{
		// if 0 end of array reached
		if(unitsInLos[i])
		{
			pos = cb->GetUnitPos(unitsInLos[i]);

			// calculate in which sector unit is located
			x = pos.x/xSectorSize;
			y = pos.z/ySectorSize;

			if(x < xSectors && x >= 0 && y >= 0 && y < ySectors)
			{	
				if(unitsInSector[x+y*ySectors])
				{
					def = cb->GetUnitDef(unitsInLos[i]);
					type = bt->units_static[def->id].category;
					combat_category_id = bt->GetIDOfAssaultCategory((UnitCategory)type);

					sector[x][y].enemy_structures += bt->units_static[def->id].cost;
					//sector[x][y].enemyUnitsOfType[type]++;

					// check if combat unit
					if(combat_category_id >= 0)
						++units_spotted[combat_category_id];
			
					// check if promising bombing target
					if(type == STATIONARY_ARTY || type == EXTRACTOR || type == STATIONARY_LAUNCHER)
					{
						/*char c[80];
						sprintf(c, "Adding target: %f %f", pos.x, pos.z);
						cb->SendTextMsg(c,0);*/
						//ai->af->AddTarget(pos, def->id, bt->units[def->id].cost, cb->GetUnitHealth(unitsInLos[i]), (UnitCategory)type);
						//cb->SendTextMsg("possible target",0);
						ai->af->CheckTarget(unitsInLos[i], def);
					}
					// add defences to sector threat
					else if(type == STATIONARY_DEF)
					{
						sector[x][y].threat_against[0] += bt->units_static[def->id].efficiency[0];
						sector[x][y].threat_against[1] += bt->units_static[def->id].efficiency[1];
						sector[x][y].threat_against[2] += bt->units_static[def->id].efficiency[2];
						sector[x][y].threat_against[3] += bt->units_static[def->id].efficiency[3];
					}
				}
			}
			unitsInLos[i] = 0;
		}
		else 	// end of arry reached
			break;	
	}

	ai->brain->UpdateMaxCombatUnitsSpotted(units_spotted);

	for(x = 0; x < xSectors; ++x)
	{
		for(y = 0; y < ySectors; ++y)
			unitsInSector[x+y*xSectors] = 0;
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
		change = 0.2;
	
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


char* AAIMap::GetMapTypeString(int mapType)
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

char* AAIMap::GetMapTypeTextString(int mapType)
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

void AAIMap::UpdateArty(int attacker)
{
	float3 pos = cb->GetUnitPos(attacker);
				
	int x = pos.x/xSectorSize;
	int y = pos.z/ySectorSize;

	if(x >= 0 && y >= 0 && x < xSectors && y < ySectors)
		sector[x][y].arty_efficiency[0] += 0.5;
}
