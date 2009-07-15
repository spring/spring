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
}

AAISector::~AAISector(void)
{
	attacked_by_this_game.clear();
	attacked_by_learned.clear();

	my_stat_combat_power.clear();
	enemy_stat_combat_power.clear();

	my_mobile_combat_power.clear();
	enemy_mobile_combat_power.clear();

	my_combat_units.clear();
	enemy_combat_units.clear();

	my_buildings.clear();

	combats_learned.clear();
	combats_this_game.clear();

	lost_units.clear();
}

void AAISector::Init(AAI *ai, int x, int y, int left, int right, int top, int bottom)
{
	this->ai = ai;
	this->ut = ai->ut;
	this->map = ai->map;

	// set coordinates of the corners
	this->x = x;
	this->y = y;

	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;

	// determine map border distance
	map_border_dist = x;

	if(map->xSectors - x < map_border_dist)
		map_border_dist = map->xSectors - x;

	if(y < map_border_dist)
		map_border_dist = y;

	if(map->ySectors - y < map_border_dist)
		map_border_dist = map->ySectors - y;

	float3 center = GetCenter();
	continent = map->GetContinentID(&center);

	// init all kind of stuff
	freeMetalSpots = false;
	interior = false;
	distance_to_base = -1;
	last_scout = 1;
	rally_points = 0;

	// nothing sighted in that sector
	enemy_structures = 0;
	own_structures = 0;
	allied_structures = 0;
	failed_defences = 0;

	int categories = ai->bt->assault_categories.size();

	combats_learned.resize(categories, 0);
	combats_this_game.resize(categories, 0);

	importance_this_game = 1.0f + (rand()%5)/20.0f;

	attacked_by_this_game.resize(categories, 0);
	attacked_by_learned.resize(categories, 0);

	lost_units.resize((int)MOBILE_CONSTRUCTOR-(int)COMMANDER+1.0);

	my_stat_combat_power.resize(categories, 0);
	enemy_stat_combat_power.resize(categories, 0);

	my_mobile_combat_power.resize(categories+1, 0);
	enemy_mobile_combat_power.resize(categories+1, 0);

	my_combat_units.resize(categories, 0);
	enemy_combat_units.resize(categories + 1, 0);

	my_buildings.resize(METAL_MAKER+1, 0);
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

bool AAISector::SetBase(bool base)
{
	if(base)
	{
		// check if already occupied (may happen if two coms start in same sector)
		if(map->team_sector_map[x][y] >= 0)
		{
			fprintf(ai->file, "\nTeam %i could not add sector %i,%i to base, already occupied by ally team %i!\n\n",ai->cb->GetMyTeam(), x, y, map->team_sector_map[x][y]);
			return false;
		}

		distance_to_base = 0;

		// if free metal spots in this sectors, base has free spots
		for(list<AAIMetalSpot*>::iterator spot = metalSpots.begin(); spot != metalSpots.end(); ++spot)
		{
			if(!(*spot)->occupied)
			{
				ai->brain->freeBaseSpots = true;
				break;
			}
		}

		// increase importance
		importance_this_game += 1;

		map->team_sector_map[x][y] = ai->cb->GetMyAllyTeam();

		if(importance_this_game > cfg->MAX_SECTOR_IMPORTANCE)
			importance_this_game = cfg->MAX_SECTOR_IMPORTANCE;

		return true;
	}
	else	// remove from base
	{
		distance_to_base = 1;

		map->team_sector_map[x][y] = -1;

		return true;
	}
}

void AAISector::Update()
{
	// decrease values (so the ai "forgets" values from time to time)...
	//ground_threat *= 0.995;
	//air_threat *= 0.995;
	for(int i = 0; i < MOBILE_CONSTRUCTOR-COMMANDER; ++i)
		lost_units[i] *= 0.92f;
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

void AAISector::AddExtractor(int unit_id, int def_id, float3 *pos)
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

			if(pos->x == spot_pos.x && pos->z == spot_pos.z)
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

/*float3 AAISector::GetCenterRallyPoint()
{

	return ZeroVector;
}*/


float3 AAISector::GetBuildsite(int building, bool water)
{
	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetBuildSiteInRect(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, water);
}

float3 AAISector::GetDefenceBuildsite(int building, UnitCategory category, float terrain_modifier, bool water)
{
	float3 best_pos = ZeroVector, pos;
	const UnitDef *def = ai->bt->unitList[building-1];

	int my_team = ai->cb->GetMyAllyTeam();

	float my_rating, best_rating = -10000;

	list<Direction> directions;

	// get possible directions
	if(category == AIR_ASSAULT && !cfg->AIR_ONLY_MOD)
	{
		directions.push_back(CENTER);
	}
	else
	{
		if(distance_to_base > 0)
			directions.push_back(CENTER);
		else
		{
			// filter out frontiers to other base sectors
			if(x > 0 && map->sector[x-1][y].distance_to_base > 0 && map->sector[x-1][y].allied_structures < 100 && map->team_sector_map[x-1][y] != my_team )
				directions.push_back(WEST);

			if(x < map->xSectors-1 && map->sector[x+1][y].distance_to_base > 0 && map->sector[x+1][y].allied_structures < 100 && map->team_sector_map[x+1][y] != my_team)
				directions.push_back(EAST);

			if(y > 0 && map->sector[x][y-1].distance_to_base > 0 && map->sector[x][y-1].allied_structures < 100 && map->team_sector_map[x][y-1] != my_team)
				directions.push_back(NORTH);

			if(y < map->ySectors-1 && map->sector[x][y+1].distance_to_base > 0 && map->sector[x][y+1].allied_structures < 100 && map->team_sector_map[x][y+1] != my_team)
				directions.push_back(SOUTH);
		}
	}

	int xStart = 0;
	int xEnd = 0;
	int yStart = 0;
	int yEnd = 0;

	// check possible directions
	for(list<Direction>::iterator dir =directions.begin(); dir != directions.end(); ++dir)
	{
		// get area to perform search
		if(*dir == CENTER)
		{
			xStart = x * map->xSectorSizeMap;
			xEnd = (x+1) * map->xSectorSizeMap;
			yStart = y * map->ySectorSizeMap;
			yEnd = (y+1) * map->ySectorSizeMap;
		}
		else if(*dir == WEST)
		{
			xStart = x * map->xSectorSizeMap;
			xEnd = x * map->xSectorSizeMap + map->xSectorSizeMap/4.0f;
			yStart = y * map->ySectorSizeMap;
			yEnd = (y+1) * map->ySectorSizeMap;
		}
		else if(*dir == EAST)
		{
			xStart = (x+1) * map->xSectorSizeMap - map->xSectorSizeMap/4.0f;
			xEnd = (x+1) * map->xSectorSizeMap;
			yStart = y * map->ySectorSizeMap;
			yEnd = (y+1) * map->ySectorSizeMap;
		}
		else if(*dir == NORTH)
		{
			xStart = x * map->xSectorSizeMap;
			xEnd = (x+1) * map->xSectorSizeMap;
			yStart = y * map->ySectorSizeMap;
			yEnd = y * map->ySectorSizeMap + map->ySectorSizeMap/4.0f;
		}
		else if(*dir == SOUTH)
		{
			xStart = x * map->xSectorSizeMap ;
			xEnd = (x+1) * map->xSectorSizeMap;
			yStart = (y+1) * map->ySectorSizeMap - map->ySectorSizeMap/4.0f;
			yEnd = (y+1) * map->ySectorSizeMap;
		}

		//
		my_rating = map->GetDefenceBuildsite(&pos, def, xStart, xEnd, yStart, yEnd, category, terrain_modifier, water);

		if(my_rating > best_rating)
		{
			best_pos = pos;
			best_rating = my_rating;
		}
	}

	return best_pos;
}

float3 AAISector::GetCenterBuildsite(int building, bool water)
{
	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetCenterBuildsite(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, water);
}

float3 AAISector::GetRadarArtyBuildsite(int building, float range, bool water)
{
	int xStart, xEnd, yStart, yEnd;

	GetBuildsiteRectangle(&xStart, &xEnd, &yStart, &yEnd);

	return map->GetRadarArtyBuildsite(ai->bt->unitList[building-1], xStart, xEnd, yStart, yEnd, range, water);
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
	pos->z -= def->zsize/2;

	// check if pos is still in that scetor, otherwise retun 0
	if(pos->x < 0 && pos->z < 0)
		pos->x = pos->z = 0;
}

void AAISector::SectorMapPos2Pos(float3 *pos, const UnitDef *def)
{
	// shift to middlepoint
	pos->x += def->xsize/2;
	pos->z += def->zsize/2;

	// get cell position on complete map
	pos->x += x * ai->map->xSectorSizeMap;
	pos->z += y * ai->map->ySectorSizeMap;

	// back to unit coordinates
	pos->x *= SQUARE_SIZE;
	pos->z *= SQUARE_SIZE;
}

UnitCategory AAISector::GetWeakestCategory()
{
	UnitCategory weakest = UNKNOWN;
	float importance, most_important = 0;

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
			importance = GetThreatBy(*cat, learned, current) / ( 0.1f + GetMyDefencePowerAgainstAssaultCategory(*cat));

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
	return (learned * attacked_by_learned[combat_cat_id] + current * attacked_by_this_game[combat_cat_id] ) / (learned + current);
}

float AAISector::GetMyCombatPower(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * my_mobile_combat_power[0] + air * my_mobile_combat_power[1] + hover * my_mobile_combat_power[2] + sea * my_mobile_combat_power[3] + submarine * my_mobile_combat_power[4]);
}

float AAISector::GetEnemyCombatPower(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * enemy_mobile_combat_power[0] + air * enemy_mobile_combat_power[1] + hover * enemy_mobile_combat_power[2] + sea * enemy_mobile_combat_power[3] + submarine * enemy_mobile_combat_power[4]);
}

float AAISector::GetMyCombatPowerAgainstCombatCategory(int combat_category)
{
	return my_mobile_combat_power[combat_category];
}

float AAISector::GetEnemyCombatPowerAgainstCombatCategory(int combat_category)
{
	return enemy_stat_combat_power[combat_category];
}

float AAISector::GetMyDefencePower(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * my_stat_combat_power[0] + air * my_stat_combat_power[1] + hover * my_stat_combat_power[2] + sea * my_stat_combat_power[3] + submarine * my_stat_combat_power[4]);
}

float AAISector::GetEnemyDefencePower(float ground, float air, float hover, float sea, float submarine)
{
	return (ground * (enemy_stat_combat_power[0] + enemy_mobile_combat_power[0])
		+ air * (enemy_stat_combat_power[1] + enemy_mobile_combat_power[1])
		+ hover * (enemy_stat_combat_power[2] + enemy_mobile_combat_power[2])
		+ sea * (enemy_stat_combat_power[3] + enemy_mobile_combat_power[3])
		+ submarine * (enemy_stat_combat_power[4] + enemy_mobile_combat_power[4]) );
}

float AAISector::GetMyDefencePowerAgainstAssaultCategory(int assault_category)
{
	return my_stat_combat_power[assault_category];
}

float AAISector::GetEnemyDefencePowerAgainstAssaultCategory(int assault_category)
{
	return enemy_stat_combat_power[assault_category];
}

float AAISector::GetEnemyThreatToMovementType(unsigned int movement_type)
{
	if(movement_type && MOVE_TYPE_GROUND)
		return enemy_stat_combat_power[0] + enemy_mobile_combat_power[0];
	else if(movement_type && MOVE_TYPE_AIR)
		return enemy_stat_combat_power[1] + enemy_mobile_combat_power[1];
	else if(movement_type && MOVE_TYPE_HOVER)
		return enemy_stat_combat_power[2] + enemy_mobile_combat_power[2];
	else if(movement_type && MOVE_TYPE_FLOATER)
		return enemy_stat_combat_power[3] + enemy_mobile_combat_power[3];
	else if(movement_type && MOVE_TYPE_UNDERWATER)
		return enemy_stat_combat_power[4] + enemy_mobile_combat_power[4];
	else if(movement_type && MOVE_TYPE_SEA)
		return 0.5 * (enemy_stat_combat_power[4] + enemy_mobile_combat_power[4] + enemy_stat_combat_power[3] + enemy_mobile_combat_power[3]);
	else
		return 0;
}

float AAISector::GetEnemyAreaCombatPowerVs(int combat_category, float neighbour_importance)
{
	float result = enemy_mobile_combat_power[combat_category];

	// take neighbouring sectors into account (if possible)
	if(x > 0)
		result += neighbour_importance * ai->map->sector[x-1][y].enemy_mobile_combat_power[combat_category];

	if(x < map->xSectors-1)
		result += neighbour_importance * ai->map->sector[x+1][y].enemy_mobile_combat_power[combat_category];

	if(y > 0)
		result += neighbour_importance * ai->map->sector[x][y-1].enemy_mobile_combat_power[combat_category];

	if(y < map->ySectors-1)
		result += neighbour_importance * ai->map->sector[x][y+1].enemy_mobile_combat_power[combat_category];

	return result;
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

void AAISector::RemoveBuildingType(int def_id)
{
	my_buildings[ai->bt->units_static[def_id].category] -= 1;
}

float AAISector::GetWaterRatio()
{
	float water_ratio = 0;

	for(int xPos = x * map->xSectorSizeMap; xPos < (x+1) * map->xSectorSizeMap; ++xPos)
	{
		for(int yPos = y * map->ySectorSizeMap; yPos < (y+1) * map->ySectorSizeMap; ++yPos)
		{
			if(map->buildmap[xPos + yPos * map->xMapSize] == 4)
				water_ratio +=1;
		}
	}

	return water_ratio / ((float)(map->xSectorSizeMap * map->ySectorSizeMap));
}

float AAISector::GetFlatRatio()
{
	// get number of cliffy tiles
	float flat_ratio = map->GetCliffyCells(left/SQUARE_SIZE, top/SQUARE_SIZE, map->xSectorSizeMap, map->ySectorSizeMap);

	// get number of flat tiles
	flat_ratio = (float)(map->xSectorSizeMap * map->ySectorSizeMap) - flat_ratio;

	flat_ratio /= (float)(map->xSectorSizeMap * map->ySectorSizeMap);

	return flat_ratio;
}

void AAISector::UpdateThreatValues(UnitCategory unit, UnitCategory attacker)
{
	// if lost unit is a building, increase attacked_by
	if(unit <= METAL_MAKER)
	{
		float change;

		if(this->interior)
			change = 0.3f;
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

bool AAISector::PosInSector(float3 *pos)
{
	if(pos->x < left || pos->x > right)
		return false;
	else if(pos->z < top || pos->z > bottom)
		return false;
	else
		return true;
}

bool AAISector::ConnectedToOcean()
{
	if(water_ratio < 0.2)
		return false;

	// find water cell
	int x_cell = (left + right) / 16.0f;
	int y_cell = (top + bottom) / 16.0f;

	// get continent
	int cont = map->GetContinentID(x_cell, y_cell);

	if(map->continents[cont].water)
	{
		if(map->continents[cont].size > 1200 && map->continents[cont].size > 0.5f * (float)map->avg_water_continent_size )
			return true;
	}

	return false;
}

void AAISector::GetMovePos(float3 *pos)
{
	int x,y;
	*pos = ZeroVector;

	// try to get random spot
	for(int i = 0; i < 6; ++i)
	{
		pos->x = left + map->xSectorSize * (0.2f + 0.06f * (float)(rand()%11) );
		pos->z = top + map->ySectorSize * (0.2f + 0.06f * (float)(rand()%11) );

		// check if blocked by  building
		x = (int) (pos->x / SQUARE_SIZE);
		y = (int) (pos->z / SQUARE_SIZE);

		if(map->buildmap[x + y * map->xMapSize] != 1)
			return;
	}

	// search systematically
	for(int i = 0; i < map->xSectorSizeMap; i += 8)
	{
		for(int j = 0; j < map->ySectorSizeMap; j += 8)
		{
			pos->x = left + i * SQUARE_SIZE;
			pos->z = top + j * SQUARE_SIZE;

			// get cell index of middlepoint
			x = (int) (pos->x / SQUARE_SIZE);
			y = (int) (pos->z / SQUARE_SIZE);

			if(map->buildmap[x + y * map->xMapSize] != 1)
				return;
		}
	}

	// no free cell found (should not happen)
	*pos = ZeroVector;
}

void AAISector::GetMovePosOnContinent(float3 *pos, unsigned int movement_type, int continent)
{
	int x,y;
	*pos = ZeroVector;

	// try to get random spot
	for(int i = 0; i < 6; ++i)
	{
		pos->x = left + map->xSectorSize * (0.2f + 0.06f * (float)(rand()%11) );
		pos->z = top + map->ySectorSize * (0.2f + 0.06f * (float)(rand()%11) );

		// check if blocked by  building
		x = (int) (pos->x / SQUARE_SIZE);
		y = (int) (pos->z / SQUARE_SIZE);

		if(map->buildmap[x + y * map->xMapSize] != 1)
		{
			//check continent
			if(map->GetContinentID(pos) == continent)
				return;
		}
	}

	// search systematically
	for(int i = 0; i < map->xSectorSizeMap; i += 8)
	{
		for(int j = 0; j < map->ySectorSizeMap; j += 8)
		{
			pos->x = left + i * SQUARE_SIZE;
			pos->z = top + j * SQUARE_SIZE;

			// get cell index of middlepoint
			x = (int) (pos->x / SQUARE_SIZE);
			y = (int) (pos->z / SQUARE_SIZE);

			if(map->buildmap[x + y * map->xMapSize] != 1)
			{
				if(map->GetContinentID(pos) == continent)
					return;
			}
		}
	}

	*pos = ZeroVector;
}

int AAISector::GetEdgeDistance()
{
	if(x > y)
	{
		if(y < map->ySectors - y)
			return y;
		else
			return map->ySectors - y;
	}
	else
	{
		if(x < map->xSectors - x)
			return x;
		else
			return map->xSectors - x;
	}
}
