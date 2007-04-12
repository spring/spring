// ------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// ------------------------------------------------------------------------

#include "AAISector.h"
#include "AAI.h"
#include "AAIMap.h"

AAISector::AAISector()
{
	this->ai = ai;
	left = right = top = bottom = 0;
	x = y = 0;
	freeMetalSpots = false;
	interior = false;
	distance_to_base = -1;
	last_scout = 1;
	map = 0;

	// nothing sighted in that sector
	enemy_structures = 0;
	own_structures = 0;
	allied_structures = 0;
	threat = 0;
	
	for(int i = 0; i < 4; i++)
	{
		defCoverage[i].direction = (Direction) i;
		defCoverage[i].defence = 0;
	}

	for(int i = 0; i <= (int)MOBILE_CONSTRUCTOR; i++)
	{
		enemyUnitsOfType[i] = 0;
		unitsOfType[i] = 0;
	}
}

AAISector::~AAISector(void)
{	
	defences.clear();
	
	attacked_by_this_game.clear(); 
	attacked_by_learned.clear(); 

	stat_combat_power.clear(); 
	mobile_combat_power.clear(); 

	combats_learned.clear();
	combats_this_game.clear();

	lost_units.clear();
}

void AAISector::SetAI(AAI *ai)
{
	this->ai = ai;
	this->ut = ai->ut;

	// init all kind of stuff
	int categories = ai->bt->assault_categories.size();

	combats_learned.resize(categories, 0);
	combats_this_game.resize(categories, 0);

	importance_this_game = 1.0f + (rand()%5)/20.0f;

	attacked_by_this_game.resize(categories, 0);
	attacked_by_learned.resize(categories, 0);

	lost_units.resize(MOBILE_CONSTRUCTOR-COMMANDER, 0);

	stat_combat_power.resize(categories, 0);
	mobile_combat_power.resize(categories+1, 0);
}

void AAISector::SetGridLocation(int x, int y)
{
	this->x = x;
	this->y = y;
}

void AAISector::SetCoordinates(int left, int right, int top, int bottom)
{
	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
}

void AAISector::AddMetalSpot(AAIMetalSpot *spot)
{
	metalSpots.push_back(spot);
	freeMetalSpots = true;
}

int AAISector::GetNumberOfMetalSpots()
{
	return metalSpots.size();
}

void AAISector::SetBase(bool base)
{
	if(base)
	{
		this->distance_to_base = 0;

		// if free metal spots in this sectors, base has free spots
		for(list<AAIMetalSpot*>::iterator spot = metalSpots.begin(); spot != metalSpots.end(); spot++)
		{
			if(!(*spot)->occupied)
			{
				ai->brain->freeBaseSpots = true;
				break;
			}
		}

		// increase importance
		++importance_this_game;

		if(importance_this_game > cfg->MAX_SECTOR_IMPORTANCE)
			importance_this_game = cfg->MAX_SECTOR_IMPORTANCE;
	}
}

void AAISector::Update()
{
	// decrease values (so the ai "forgets" values from time to time)...
	//ground_threat *= 0.995;
	//air_threat *= 0.995;
	for(int i = 0; i < MOBILE_CONSTRUCTOR-COMMANDER; ++i)
		lost_units[i] *= 0.92;
}

// return null pointer if no empty spot found
AAIMetalSpot* AAISector::GetFreeMetalSpot(float3 pos)
{
	// just some high value 
	float dist;
	float shortest_dist = 100000000.0f;
	AAIMetalSpot *nearest = 0;

	// look for the first unoccupied metalspot
	for(list<AAIMetalSpot*>::iterator i = metalSpots.begin(); i != metalSpots.end(); i++)
	{
		// if metalspot is occupied, try next one
		if(!(*i)->occupied)
		{
			// get distance to pos
			dist = sqrt( pow(((*i)->pos.x - pos.x), 2) + pow(((*i)->pos.z - pos.z),2) );

			if(dist < shortest_dist)
			{
				nearest = *i;
				shortest_dist = dist;
			}
		}
	}
	
	if(!nearest)
	{
		// no empty metalspot found in that sector
		this->freeMetalSpots = false;
		return 0;
	}
	else
		return nearest;
}

AAIMetalSpot* AAISector::GetFreeMetalSpot()
{
	// look for the first unoccupied metalspot
	for(list<AAIMetalSpot*>::iterator i = metalSpots.begin(); i != metalSpots.end(); i++)
	{
		// if metalspot is occupied, try next one
		if(!(*i)->occupied)
			return *i;
	}
	
	
	return 0;
}
void AAISector::FreeMetalSpot(float3 pos, const UnitDef *extractor)
{
	float3 spot_pos;

	// get metalspot according to position
	for(list<AAIMetalSpot*>::iterator spot = metalSpots.begin(); spot != metalSpots.end(); ++spot)
	{
		// only check occupied spots
		if((*spot)->occupied)
		{
			// compare positions
			spot_pos = (*spot)->pos;
			ai->map->Pos2FinalBuildPos(&spot_pos, extractor);

			if(pos.x == spot_pos.x && pos.z == spot_pos.z)
			{
				(*spot)->occupied = false;
				(*spot)->extractor = -1;
				(*spot)->extractor_def = -1;

				freeMetalSpots = true;
				
				// if part of the base, tell the brain that the base has now free spots again
				if(distance_to_base == 0)
					ai->brain->freeBaseSpots = true;

				return;
			}
		}
	}
}

void AAISector::AddExtractor(int unit_id, int def_id, float3 pos)
{
	float3 spot_pos;

	// get metalspot according to position
	for(list<AAIMetalSpot*>::iterator spot = metalSpots.begin(); spot != metalSpots.end(); ++spot)
	{
		// only check occupied spots
		if((*spot)->occupied)
		{
			// compare positions
			spot_pos = (*spot)->pos;
			ai->map->Pos2FinalBuildPos(&spot_pos, ai->bt->unitList[def_id-1]);

			if(pos.x == spot_pos.x && pos.z == spot_pos.z)	
			{
				(*spot)->extractor = unit_id;
				(*spot)->extractor_def = def_id;
			}
		}
	}
}

float3 AAISector::GetCenter()
{
	float3 pos;
	pos.x = (left + right)/2.0;
	pos.z = (top + bottom)/2.0;

	return pos;
}

float3 AAISector::GetBuildsite(int building, bool water)
{
	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetBuildSiteInRect(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, water);
}

float3 AAISector::GetDefenceBuildsite(int building, UnitCategory category, bool water)
{
	float3 pos;
	const UnitDef *def = ai->bt->unitList[building-1];

	list<Direction> directions;
	list<AAISector*> neighbours;

	// get possible directions 
	
	// filter out frontiers to other base sectors
	if(x > 0 && map->sector[x-1][y].distance_to_base > 0 && map->sector[x-1][y].allied_structures < 300)
		directions.push_back(WEST);
	
	if(x < map->xSectors-1 && map->sector[x+1][y].distance_to_base > 0 && map->sector[x+1][y].allied_structures < 300)
		directions.push_back(EAST);

	if(y > 0 && map->sector[x][y-1].distance_to_base > 0 && map->sector[x][y-1].allied_structures < 300)
		directions.push_back(NORTH);
	
	if(y < map->ySectors-1 && map->sector[x][y+1].distance_to_base > 0 && map->sector[x][y+1].allied_structures < 300)
		directions.push_back(SOUTH);
	
	// determine weakest direction
	Direction weakest_dir = NO_DIRECTION;
	
	// build anti air defence in the center of the sector
	if(category == AIR_ASSAULT || distance_to_base > 0)
	{
		weakest_dir = CENTER;
	}
	else
	{
		for(int i = 0; i < 4; i++)
			defCoverage[i].defence = 0;

		// determine which direction is defended against category how much 
		for(list<AAIDefence>::iterator def = defences.begin(); def != defences.end(); def++)
		{
			if(def->direction != CENTER)
				defCoverage[def->direction].defence += ai->bt->GetEfficiencyAgainst(def->def_id, category);
		}

		// get weakest direction
		float weakest = 0, current;
	
		for(list<Direction>::iterator dir = directions.begin(); dir != directions.end(); dir++)
		{	
			if(defCoverage[(int)(*dir)].defence != 0)
			{
				current = GetMapBorderDist() * 100.0 / defCoverage[(int)(*dir)].defence;

				if(current > weakest)
				{
					weakest = current;
					weakest_dir = *dir;
				}
			}
			else	// no defence in that direction yet
			{
				weakest_dir = *dir;
				break;
			}
		}
	}

	// now we know where the building should be placed, find suitable spot
	if(weakest_dir != NO_DIRECTION)
	{
		int xStart = 0;
		int xEnd = 0; 

		int yStart = 0;
		int yEnd = 0; 

		// calculate size of building
		int xSize = def->xsize;
		int ySize = def->ysize;

		if(weakest_dir == CENTER)
		{
			xStart = x * map->xSectorSizeMap + map->xSectorSizeMap/8.0f;
			xEnd = (x+1) * map->xSectorSizeMap - map->xSectorSizeMap/8.0f; 

			yStart = y * map->ySectorSizeMap + map->ySectorSizeMap/8.0f;
			yEnd = (y+1) * map->ySectorSizeMap - map->ySectorSizeMap/8.0f; 
		}
		else if(weakest_dir == WEST)
		{
			xStart = x * map->xSectorSizeMap;
			xEnd = x * map->xSectorSizeMap + map->xSectorSizeMap/8.0f;

			yStart = y * map->ySectorSizeMap;
			yEnd = (y+1) * map->ySectorSizeMap - map->ySectorSizeMap/8.0f; 
		}
		else if(weakest_dir == EAST)
		{
			xStart = (x+1) * map->xSectorSizeMap - map->xSectorSizeMap/8.0f;
			xEnd = (x+1) * map->xSectorSizeMap;

			yStart = y * map->ySectorSizeMap + map->ySectorSizeMap/8.0f;
			yEnd = (y+1) * map->ySectorSizeMap; 
		}
		else if(weakest_dir == NORTH)
		{
			xStart = x * map->xSectorSizeMap + map->xSectorSizeMap/8.0f;
			xEnd = (x+1) * map->xSectorSizeMap;

			yStart = y * map->ySectorSizeMap;
			yEnd = y * map->ySectorSizeMap + map->ySectorSizeMap/8.0f; 
		}
		else if(weakest_dir == SOUTH)
		{
			xStart = x * map->xSectorSizeMap ;
			xEnd = (x+1) * map->xSectorSizeMap - map->xSectorSizeMap/8.0f;

			yStart = (y+1) * map->ySectorSizeMap - map->ySectorSizeMap/8.0f;
			yEnd = (y+1) * map->ySectorSizeMap; 
		}

		if(!xStart || !xEnd || !yStart || !yEnd)
			return ZeroVector;

		// try to find random pos first
		pos = map->GetRandomBuildsite(def, xStart, xEnd, yStart, yEnd, 50, water);

		// otherwise get pos in rect
		if(!pos.x)
			pos = map->GetBuildSiteInRect(def, xStart, xEnd, yStart, yEnd, water);
		
		return pos;	
	}

	// entire sector checked, no free site found
	return ZeroVector;
}

float3 AAISector::GetCenterBuildsite(int building, bool water)
{
	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetCenterBuildsite(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, water);
}

float3 AAISector::GetHighestBuildsite(int building)
{
	if(building < 1)
	{
		fprintf(ai->file, "ERROR: Invalid building def id %i passed to AAISector::GetRadarBuildsite()\n", building);
		return ZeroVector;
	}

	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetHighestBuildsite(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd);
}

float3 AAISector::GetRandomBuildsite(int building, int tries, bool water)
{
	if(building < 1)
	{
		fprintf(ai->file, "ERROR: Invalid building def id %i passed to AAISector::GetRadarBuildsite()\n", building);
		return ZeroVector;
	}

	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetRandomBuildsite(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, tries, water);
}

void AAISector::GetBuildsiteRectangle(int *xStart, int *xEnd, int *yStart, int *yEnd)
{
	*xStart = x * map->xSectorSizeMap;
	*xEnd = *xStart + map->xSectorSizeMap; 

	if(*xStart == 0)
		*xStart = 8;

	*yStart = y * map->ySectorSizeMap;
	*yEnd = *yStart + map->ySectorSizeMap; 
	
	if(*yStart == 0)
		*yStart = 8;

	// reserve buildspace for def. buildings
	if(x > 0 && map->sector[x-1][y].distance_to_base > 0 )
		*xStart += map->xSectorSizeMap/8;
	
	if(x < map->xSectors-1 && map->sector[x+1][y].distance_to_base > 0)
		*xEnd -= map->xSectorSizeMap/8;

	if(y > 0 && map->sector[x][y-1].distance_to_base > 0)
		*yStart += map->ySectorSizeMap/8;
	
	if(y < map->ySectors-1 && map->sector[x][y+1].distance_to_base > 0)
		*yEnd -= map->ySectorSizeMap/8;
}

// converts unit positions to cell coordinates
void AAISector::Pos2SectorMapPos(float3 *pos, const UnitDef* def)
{
	// get cell index of middlepoint
	pos->x = ((int) pos->x/SQUARE_SIZE)%ai->map->xSectorSizeMap;
	pos->z = ((int) pos->z/SQUARE_SIZE)%ai->map->ySectorSizeMap;

	// shift to the leftmost uppermost cell
	pos->x -= def->xsize/2;
	pos->z -= def->ysize/2;

	// check if pos is still in that scetor, otherwise retun 0
	if(pos->x < 0 && pos->z < 0)
		pos->x = pos->z = 0;
}

void AAISector::SectorMapPos2Pos(float3 *pos, const UnitDef *def)
{
	// shift to middlepoint
	pos->x += def->xsize/2;
	pos->z += def->ysize/2;

	// get cell position on complete map
	pos->x += x * ai->map->xSectorSizeMap;
	pos->z += y * ai->map->ySectorSizeMap;

	// back to unit coordinates
	pos->x *= SQUARE_SIZE;
	pos->z *= SQUARE_SIZE;
}

float AAISector::GetDefencePowerVs(UnitCategory category)
{
	float power = 0.5;

	for(list<AAIDefence>::iterator i = defences.begin(); i != defences.end(); ++i)	
		power += ai->bt->GetEfficiencyAgainst(i->def_id, category);
	
	return power;
}

float AAISector::GetDefencePowerVsID(int combat_cat_id)
{
	float power = 0.5;

	for(list<AAIDefence>::iterator i = defences.begin(); i != defences.end(); ++i)	
		power +=  ai->bt->units_static[i->def_id].efficiency[combat_cat_id];
	
	return power;
}

UnitCategory AAISector::GetWeakestCategory()
{
	UnitCategory weakest = UNKNOWN;
	float importance, most_important = 0;

	if(defences.size() > cfg->MAX_DEFENCES)
		return UNKNOWN;
	
	float learned = 60000 / (ai->cb->GetCurrentFrame() + 30000) + 0.5;
	float current = 2.5 - learned;

	if(interior)
	{
		weakest = AIR_ASSAULT;
	}
	else
	{
		for(list<UnitCategory>::iterator cat = ai->bt->assault_categories.begin(); cat != ai->bt->assault_categories.end(); ++cat)
		{
			importance = GetThreatBy(*cat, learned, current)/GetDefencePowerVs(*cat);

			if(importance > most_important)
			{	
				most_important = importance;
				weakest = *cat;
			}
		}
	}

	return weakest;
}

float AAISector::GetThreatBy(UnitCategory category, float learned, float current)
{
	if(category == GROUND_ASSAULT)
		return 1.0f + (learned * attacked_by_learned[0] + current * attacked_by_this_game[0] ) / (learned + current);
	if(category == AIR_ASSAULT)
		return 1.0f + (learned * attacked_by_learned[1] + current * attacked_by_this_game[1] ) / (learned + current);
	if(category == HOVER_ASSAULT)
		return 1.0f + (learned * attacked_by_learned[2] + current * attacked_by_this_game[2] ) / (learned + current);
	if(category == SEA_ASSAULT)
		return 1.0f + (learned * attacked_by_learned[3] + current * attacked_by_this_game[3] ) / (learned + current);
	if(category == SUBMARINE_ASSAULT)
		return 1.0f + (learned * attacked_by_learned[4] + current * attacked_by_this_game[4] ) / (learned + current);
	else
		return -1;
}

float AAISector::GetThreatByID(int combat_cat_id, float learned, float current)
{
	return 1.0f + (learned * attacked_by_learned[combat_cat_id] + current * attacked_by_this_game[combat_cat_id] ) / (learned + current);
}

float AAISector::GetThreatTo(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * stat_combat_power[0] + air * stat_combat_power[1] + hover * stat_combat_power[2] + sea * stat_combat_power[3] + submarine * stat_combat_power[4]);
}

float AAISector::GetLostUnits(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * lost_units[GROUND_ASSAULT-COMMANDER] + air * lost_units[AIR_ASSAULT-COMMANDER] 
		  + hover * lost_units[HOVER_ASSAULT-COMMANDER] + sea * lost_units[SEA_ASSAULT-COMMANDER] 
		  + submarine * lost_units[SUBMARINE_ASSAULT-COMMANDER]);
}

float AAISector::GetOverallThreat(float learned, float current)
{
	return (learned * (attacked_by_learned[0] + attacked_by_learned[1] + attacked_by_learned[2] + attacked_by_learned[3])
		+ current *	(attacked_by_this_game[0] + attacked_by_this_game[1] + attacked_by_this_game[2] + attacked_by_this_game[3])) 
		/(learned + current);
}

void AAISector::RemoveDefence(int unit_id)
{
	for(list<AAIDefence>::iterator i = defences.begin(); i != defences.end(); i++)
	{
		if(i->unit_id == unit_id)
		{
			defences.erase(i);
			return;
		}
	}
}

void AAISector::AddDefence(int unit_id, int def_id, float3 pos)
{
	AAIDefence def;
	def.unit_id = unit_id;
	def.def_id = def_id;

	if(pos.x >= left && pos.x <= (left + ai->map->xSectorSize/6))
	{
		def.direction = WEST;
		//ai->cb->SendTextMsg("new defence: west",0);
	}
	else if(pos.z >= top && pos.z <= (top + ai->map->ySectorSize/6))
	{
		def.direction = NORTH;
		//ai->cb->SendTextMsg("new defence: north",0);
	}
	else if(pos.x >= (right - ai->map->xSectorSize/6) && pos.x <= right)
	{
		def.direction = EAST;
		//ai->cb->SendTextMsg("new defence: east",0);
	}
	else if(pos.z >= (bottom - ai->map->xSectorSize/6) && pos.x <= bottom)
	{
		def.direction = SOUTH;
		//ai->cb->SendTextMsg("new defence: south",0);
	}
	else
		def.direction = CENTER;

	defences.push_back(def);
}

float AAISector::GetWaterRatio()
{
	float water_ratio = 0;

	for(int xPos = x * map->xSectorSizeMap; xPos < (x+1) * map->xSectorSizeMap; ++xPos)
	{
		for(int yPos = y * map->ySectorSizeMap; yPos < (y+1) * map->ySectorSizeMap; ++yPos)
		{
			if(map->buildmap[xPos + yPos * map->xMapSize] == 4)
				++water_ratio;
		}
	}

	return water_ratio / ((float)(map->xSectorSizeMap * map->ySectorSizeMap));
}

float AAISector::GetFlatRatio()
{
	// get number of cliffy tiles
	float flat_ratio = ai->map->GetCliffyCells(left/SQUARE_SIZE, top/SQUARE_SIZE, ai->map->xSectorSizeMap, ai->map->ySectorSizeMap);
	
	// get number of flat tiles
	flat_ratio = (ai->map->xSectorSizeMap * ai->map->ySectorSizeMap) - flat_ratio;

	flat_ratio /= (ai->map->xSectorSizeMap * ai->map->ySectorSizeMap);

	return flat_ratio;
}

float AAISector::GetMapBorderDist()
{
	float result = 2;

	if(x == 0 || x == ai->map->xSectors-1)
		result -= 0.5;

	if(y == 0 || y == ai->map->ySectors-1)
		result -= 0.5;

	return result;
}

void AAISector::UpdateThreatValues(UnitCategory unit, UnitCategory attacker)
{
	// if lost unit is a building, increase attacked_by 
	if(unit <= METAL_MAKER)
	{
		float change;

		if(this->interior)
			change = 0.3;
		else
			change = 1;

		// determine type of attacker
		if(attacker == AIR_ASSAULT)
			attacked_by_this_game[1] += change;
		else if(attacker == GROUND_ASSAULT)
			attacked_by_this_game[0] += change;
		else if(attacker == HOVER_ASSAULT)
			attacked_by_this_game[2] += change;
		else if(attacker == SEA_ASSAULT)
			attacked_by_this_game[3] += change;
		else if(attacker == SUBMARINE_ASSAULT)
			attacked_by_this_game[4] += change;
	}
	else // unit was lost
	{
		if(attacker == AIR_ASSAULT)
			++combats_this_game[1];
		else if(attacker == GROUND_ASSAULT)
			++combats_this_game[0];
		else if(attacker == HOVER_ASSAULT)
			++combats_this_game[2];
		else if(attacker == SEA_ASSAULT)
			++combats_this_game[3];
		else if(attacker == SUBMARINE_ASSAULT)
			++combats_this_game[4];

		++lost_units[unit-COMMANDER];
	}
}


float AAISector::GetAreaCombatPowerVs(int combat_category, float neighbour_importance)
{
	float result = mobile_combat_power[combat_category];

	// take neighbouring sectors into account (if possible)
	if(x > 0)
		result += neighbour_importance * ai->map->sector[x-1][y].mobile_combat_power[combat_category];

	if(x < map->xSectors-1)
		result += neighbour_importance * ai->map->sector[x+1][y].mobile_combat_power[combat_category];

	if(y > 0)
		result += neighbour_importance * ai->map->sector[x][y-1].mobile_combat_power[combat_category];

	if(y < map->ySectors-1)
		result += neighbour_importance * ai->map->sector[x][y+1].mobile_combat_power[combat_category];

	return result;
}