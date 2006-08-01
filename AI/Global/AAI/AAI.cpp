#include "AAI.h"
#include <math.h>
#include <stdio.h>


AAI::AAI()
{
	// initialize random numbers generator
	srand (time(NULL));

	brain = 0;
	execute = 0;
	bt = 0;
	ut = 0;
	map = 0;
	af = 0;
	
	side = 0;

	for(int i = 0; i <= SEA_BUILDER; i++)
	{
		activeUnits[i] = 0;
		futureUnits[i] = 0;
	}

	activeScouts = futureScouts = 0;

	initialized = false;
}

AAI::~AAI()
{
	if(!cfg->initialized)
		return;

	// save several ai data
	fprintf(file, "\nShutting down....\n\n");
	fprintf(file, "Unit category	active / under construction\n");
	for(int i = 0; i <= SEA_BUILDER; i++)
	{
		fprintf(file, "%-20s: %i / %i\n", bt->GetCategoryString2((UnitCategory)i), activeUnits[i], futureUnits[i]); 
	}
	
	fprintf(file, "\nFuture metal/energy request: %i / %i\n", (int)execute->futureRequestedMetal, (int)execute->futureRequestedEnergy);
	fprintf(file, "Future metal/energy supply:  %i / %i\n", (int)execute->futureAvailableMetal, (int)execute->futureAvailableEnergy);

	fprintf(file, "\nFuture/active scouts: %i / %i\n", futureScouts, activeScouts);

	// delete buildtasks
	for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); task++)
	{
		delete (*task);
	}

	// save mod learning data
	bt->SaveBuildTable();

	delete brain;
	delete execute;
	delete ut;
	delete af;
	delete map;
	delete bt;
	
	// delete unit groups
	for(int i = 0; i <= SEA_BUILDER; i++)
	{
		for(list<AAIGroup*>::iterator group = group_list[i].begin(); group != group_list[i].end(); group++)
		{
			delete *group;
		}
	}

	delete [] group_list;

	fclose(file);
}

void AAI::GotChatMsg (const char *msg, int player){
	//cb->SendTextMsg("I don´t know how to talk yet...",0);
}


void AAI::InitAI(IGlobalAICallback* callback, int team)
{
	aicb = callback;
	cb = callback->GetAICallback();

	// open log file
	char filename[80];
	char buffer[60];
	char team_number[3];
	
	#ifdef WIN32
		itoa(team, team_number, 10);
	#else
		snprintf(team_number,10,"%d",team);
	#endif
	
	strcpy(buffer, AILOG_PATH);
	strcat(buffer, "AAI_log_team_");
	strcat(buffer, team_number);
	strcat(buffer, ".txt");
	ReplaceExtension (buffer, filename, sizeof(filename), ".txt");
	file = fopen(filename,"w");

	fprintf(file, "AAI %s running mod %s\n \n", AAI_VERSION, cb->GetModName());

	// load config file first
	cfg->LoadConfig(this);

	if(!cfg->initialized)
	{
		cb->SendTextMsg("Error: Could not load mod and/or general config file, see .../log/AILog.txt for further information",0);
		return;
	}

	// create buildtable
	bt = new AAIBuildTable(cb, this);
	bt->Init();
	
	// init unit table
	ut = new AAIUnitTable(this, bt);

	// init map
	map = new AAIMap(this);
	map->Init();

	// init brain
	brain = new AAIBrain(this);

	// init executer
	execute = new AAIExecute(this, brain);

	// create unit groups
	group_list = new list<AAIGroup*>[SEA_BUILDER+1];

	// init airforce manager
	af = new AAIAirForceManager(this, cb, bt);

	cb->SendTextMsg("AAI loaded",0);
}

void AAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
	const UnitDef *def, *att_def;
	UnitCategory att_cat, cat;
	
	// filter out commander
	if(ut->cmdr != -1)
	{
		if(damaged == ut->cmdr)
			brain->DefendCommander(attacker);
	}

	def = cb->GetUnitDef(damaged);

	if(def)
		cat =  bt->units_static[def->id].category;
	else
		cat = UNKNOWN;
	
	// known attacker
	if(attacker != -1)
	{
		att_def = cb->GetUnitDef(attacker);

		// filter out friendly fire
		if(cb->GetUnitTeam(attacker) == cb->GetMyTeam())
			return;

		if(att_def)
		{
			att_cat = bt->units_static[att_def->id].category;

			if(bt->IsScout(att_cat))
				return;
			else if(att_cat == AIR_ASSAULT)
			{
				// building has been attacked
				if(cat <= METAL_MAKER)
					execute->DefendVSAir(damaged, 115);
				// unit
				else
					execute->DefendVSAir(damaged, 108);
			}	
			else if(att_cat >= GROUND_ASSAULT && att_cat <= SUBMARINE_ASSAULT)
			{
				// building has been attacked
				if(cat <= METAL_MAKER)
					execute->DefendVS(att_cat, cb->GetUnitPos(attacker), 115);
				else if(bt->IsBuilder(cat))
					execute->DefendVS(att_cat, cb->GetUnitPos(attacker), 110);
			}
		}
	}
	// unknown attacker
	else
	{
		// set default attacker
		float3 pos = cb->GetUnitPos(damaged);

		if(pos.y > 0)
			att_cat = GROUND_ASSAULT;
		else
			att_cat = SEA_ASSAULT;

		// building has been attacked
		if(cat <= METAL_MAKER)
			execute->DefendVS(att_cat, cb->GetUnitPos(damaged), 115);
		else if(bt->IsBuilder(cat))
			execute->DefendVS(att_cat, cb->GetUnitPos(damaged), 110);
	}
}

void AAI::UnitCreated(int unit)
{
	if(!cfg->initialized)
		return;

	// get unit´s id and side
	const UnitDef *def = cb->GetUnitDef(unit);
	int side = bt->GetSideByID(def->id)-1;
	UnitCategory category = bt->units_static[def->id].category;

	// add to unittable 
	ut->AddUnit(unit, def->id);

	// get commander a startup
	if(!initialized && ut->IsDefCommander(def->id))
	{
		++futureUnits[COMMANDER];

		// set side
		this->side = side+1;

		//debug
		fprintf(file, "Playing as %s\n", bt->sideNames[side+1].c_str());

		if(this->side < 1 || this->side > bt->numOfSides)
		{
			cb->SendTextMsg("Error: side not properly set", 0);
			fprintf(file, "ERROR: invalid side id %i\n", this->side);
			return;
		}

		// precache/print builder/factory rating
		fprintf(file, "Builder ratings:\n\n");
		
		for(list<int>::iterator i = bt->units_of_category[GROUND_BUILDER][side].begin(); i != bt->units_of_category[GROUND_BUILDER][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetBuilderRating(*i));

		for(list<int>::iterator i = bt->units_of_category[AIR_BUILDER][side].begin(); i != bt->units_of_category[AIR_BUILDER][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetBuilderRating(*i));

		for(list<int>::iterator i = bt->units_of_category[HOVER_BUILDER][side].begin(); i != bt->units_of_category[HOVER_BUILDER][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetBuilderRating(*i));

		for(list<int>::iterator i = bt->units_of_category[SEA_BUILDER][side].begin(); i != bt->units_of_category[SEA_BUILDER][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetBuilderRating(*i));

		fprintf(file, "Factory ratings:\n");
		
		for(list<int>::iterator i = bt->units_of_category[GROUND_FACTORY][side].begin(); i != bt->units_of_category[GROUND_FACTORY][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetFactoryRating(*i));

		for(list<int>::iterator i = bt->units_of_category[SEA_FACTORY][side].begin(); i != bt->units_of_category[SEA_FACTORY][side].end(); ++i)
			fprintf(file, "%-35s %f\n", bt->unitList[*i-1]->humanName.c_str(), bt->GetFactoryRating(*i));	
	

		// tell the brain about the starting sector
		float3 pos = cb->GetUnitPos(unit);
		int x = pos.x/map->xSectorSize;
		int y = pos.z/map->ySectorSize;

		if(x < 0)
			x = 0;
		if(y < 0 ) 
			y = 0;
		if(x >= map->xSectors)
			x = map->xSectors-1;
		if(y >= map->ySectors)
			y = map->ySectors-1;

		brain->AddSector(&map->sector[x][y]);
		brain->start_pos = pos;
		
		// set sector as part of the base
		map->sector[x][y].SetBase(true);
		map->sector[x][y].importance[0] += 1;

		brain->UpdateNeighbouringSectors();
		brain->UpdateBaseCenter();

		// start with two sectors
		/*if(map->mapType == WATER_MAP)
			brain->ExpandBase(WATER_SECTOR);
		else
			brain->ExpandBase(LAND_SECTOR);*/
	
		// now that we know the side, init buildques

		execute->InitBuildques();

		ut->AddCommander(unit, def);

		// set all buildoptions of comander buildable
		for(list<int>::iterator i = bt->units_static[def->id].canBuildList.begin();  i != bt->units_static[def->id].canBuildList.end(); i++)
		{
			bt->units_dynamic[*i].builderAvailable = true;
		}

		// add the highest rated, buildable factory
		execute->AddStartFactory();

		// get economy working
		execute->CheckRessources();
		execute->CheckConstruction();

		initialized = true;
		return;
	}
	// construction of building started
	else if(category <= METAL_MAKER && category > UNKNOWN)
	{
		// create new buildtask
		AAIBuildTask *task = new AAIBuildTask(this, unit, def->id, cb->GetUnitPos(unit), cb->GetCurrentFrame());
		build_tasks.push_back(task);

		float3 pos = cb->GetUnitPos(unit);
		// find builder and associate building with that builder
		task->builder_id = -1;

		AAIBuilder *builder;

		for(set<int>::iterator i = ut->builders.begin(); i != ut->builders.end(); ++i)
		{
			builder = ut->units[*i].builder;

			if(builder->build_pos.x == pos.x && builder->build_pos.z == pos.z)
			{
				builder->building_unit_id = unit;
				task->builder_id = builder->unit_id;
				builder->build_task = task;
				builder->CheckAssistance();
				break;
			}
		}

		// add defence buildings to the sector
		if(category == STATIONARY_DEF)
		{
			int x = pos.x/map->xSectorSize;
			int y = pos.z/map->ySectorSize;

			if(x >= 0 && y >= 0 && x < map->xSectors && y < map->ySectors)
				map->sector[x][y].AddDefence(unit, def->id, pos);
		}
		else if(category == EXTRACTOR)
		{
			int x = pos.x/map->xSectorSize;
			int y = pos.z/map->ySectorSize;

			map->sector[x][y].AddExtractor(unit, def->id, pos);
		}
	}
	else if(bt->IsBuilder(category))
	{
		//cb->SendTextMsg("active builder",0);
		++bt->units_dynamic[def->id].active;
	}
	else if(bt->IsScout(category))
	{
		++bt->units_dynamic[def->id].active;
	}
}

void AAI::UnitDestroyed(int unit, int attacker) 
{
	// get unit´s id 
	const UnitDef *def = cb->GetUnitDef(unit);
	int side = bt->GetSideByID(def->id)-1;

	if(!def)
	{
		fprintf(file, "Error: UnitDestroyed() called with invalid unit id"); 
		return;
	}

	// get unit's category
	UnitCategory category = bt->units_static[def->id].category;
	// and position
	float3 pos = cb->GetUnitPos(unit);
	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;

	// check if unit pos is within a valid sector (e.g. aircraft flying outside of the map)
	bool validSector = true;

	if(x >= map->xSectors || x < 0 || y < 0 || y >= map->ySectors)
		validSector = false;

	// update threat map
	if(attacker && validSector)
	{
		const UnitDef *att_def = cb->GetUnitDef(attacker);

		if(att_def)	
			map->sector[x][y].UpdateThreatValues((UnitCategory)category, bt->units_static[att_def->id].category);
	}

	// unfinished unit has been killed
	if(cb->UnitBeingBuilt(unit))
	{
		--futureUnits[category];

		// unfinished building
		if(!def->canfly && !def->movedata)
		{
			// delete buildtask
			for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); task++)
			{
				if((*task)->unit_id == unit)
				{	
					AAIBuildTask *build_task = *task;

					build_tasks.erase(task);
					build_task->BuildingDestroyed();
				
					delete build_task;
					break;
				}
			}
		}
		// unfinished unit
		else
		{
			// reorder builders
			if(bt->IsBuilder(category))
			{
				--bt->units_dynamic[def->id].active;

				++bt->units_dynamic[def->id].requested;
				execute->AddUnitToBuildque(def->id);
			}
			else if(bt->IsScout(category))
			{
				--futureScouts;
				--bt->units_dynamic[def->id].active;

				//char c[120];
				//sprintf(c, "unfinished scout destroyed: %i", futureScouts); 
				//cb->SendTextMsg(c, 0);
			}
		}
	}
	else	// finished unit/building has been killed
	{
		--activeUnits[category];	

		// update buildtable
		if(attacker)
		{
			const UnitDef *def_att = cb->GetUnitDef(attacker);
			
			if(def_att)
			{
				int killer = bt->GetIDOfAssaultCategory(bt->units_static[def_att->id].category);
				int killed = bt->GetIDOfAssaultCategory((UnitCategory)category);

				// check if valid id 
				if(killer != -1)		
				{
					brain->AttackedBy(killer);

					if(killed != -1)
					{
						bt->UpdateTable(def_att, killer, def, killed);
						map->UpdateCategoryUsefulness(def_att, killer, def, killed);
					}
				}
			}
		}

		// finished building has been killed
		if(!def->canfly && !def->movedata)
		{
			// decrease number of units of that category in the target sector
			if(validSector)
			{
				--map->sector[x][y].unitsOfType[category];
				map->sector[x][y].own_structures -= bt->units_static[def->id].cost;
			}

			// check if building belongs to one of this groups
			// side -1 because sides start with 1; array of sides with 0
			if(category == STATIONARY_DEF)
			{
				if(validSector)
				{
					map->sector[x][y].RemoveDefence(unit);
					map->sector[x][y].arty_efficiency[0] *= 0.9;

					if(map->sector[x][y].arty_efficiency[0] < 1)
						map->sector[x][y].arty_efficiency[0] = 1;
				}
			}
			else if(category == EXTRACTOR)
			{
				ut->RemoveExtractor(unit);

				// mark spots of destroyed mexes as unoccupied
				map->sector[x][y].FreeMetalSpot(cb->GetUnitPos(unit), def);
			}
			else if(category == POWER_PLANT)
			{
				ut->RemovePowerPlant(unit);
			}
			else if(category == STATIONARY_ARTY)
			{
				ut->RemoveStationaryArty(unit);
			}
			else if(category == STATIONARY_RECON)
			{
				ut->RemoveRecon(unit);
			}
			else if(category == STATIONARY_JAMMER)
			{
				ut->RemoveJammer(unit);
			}
			else if(category == METAL_MAKER)
			{
				ut->RemoveMetalMaker(unit);
			}
			
			// clean up 
			if(category == GROUND_FACTORY || category == SEA_FACTORY)
			{
				ut->RemoveFactory(unit);

				--bt->units_dynamic[def->id].active;
				
				// speed up reconstruction
				execute->urgency[category] += 0.3;
		
				// clear buildmap
				map->Pos2BuildMapPos(&pos, def);

				if(category == GROUND_FACTORY)
				{
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, false);
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 0);
					map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, false, false);
					map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, false);
					map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, false);
				}
				else
				{
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, true);	
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 4);
					map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, false, true);
					map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, true);
					map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, true);
				}
			}
			// hq
			else if(category == COMMANDER)
			{
				ut->RemoveCommander(unit);

				// clear buildmap
				map->Pos2BuildMapPos(&pos, def);

				if(def->minWaterDepth <= 0)
				{
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, false);
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 0);
					map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, false, false);
					map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, false);
					map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, false);
				}
				else
				{
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, true);	
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 4);
					map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, false, true);
					map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, true);
					map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, true);
				}
			}
			// other building
			else
			{
				// clear buildmap
				map->Pos2BuildMapPos(&pos, def);

				if(def->minWaterDepth <= 0)
				{
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 0);
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, false);
				}
				else
				{
					map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 4);
					map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, false, true);
				}
			}
		}
		else // finished unit has been killed
		{
			// scout
			if(bt->IsScout(category))
			{
				--activeScouts;
				--bt->units_dynamic[def->id].active;

				// remove from scout list
				for(list<int>::iterator i = scouts.begin(); i != scouts.end(); i++)
				{
					if(*i == unit)
					{
						scouts.erase(i);
						break;
					}
				}
			}
			// assault units 
			else if(category == GROUND_ASSAULT || category == AIR_ASSAULT || category == HOVER_ASSAULT || category == SEA_ASSAULT)
			{
				if(ut->units[unit].group)
					ut->units[unit].group->RemoveUnit(unit, attacker);
				else
					cb->SendTextMsg("Error: group not found",0);
			}
			// builder 
			else if(bt->IsBuilder(category))
			{
				ut->RemoveBuilder(unit, def->id);
			}
			else if(category == COMMANDER)
			{
				ut->RemoveCommander(unit);
			}
		}
	}

	ut->RemoveUnit(unit);
}

void AAI::UnitFinished(int unit) 
{
	if(!cfg->initialized)
		return;

	// get unit´s id and side
	const UnitDef *def = cb->GetUnitDef(unit);
	int side = bt->GetSideByID(def->id)-1;

	UnitCategory category = bt->units_static[def->id].category;
	--futureUnits[category];
	++activeUnits[category];

	// building was completed
	if(!def->movedata && !def->canfly)
	{
		// delete buildtask
		for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); task++)
		{
			if((*task)->unit_id == unit)
			{
				AAIBuildTask *build_task = *task;
				
				if((*task)->builder_id >= 0)
				{
					AAIBuilder *my_builder = ut->units[(*task)->builder_id].builder;
					
					if(my_builder)
						my_builder->BuildingFinished();
				}
				
				build_tasks.erase(task);
				delete build_task;
				break;
			}
		}

		// check if building belongs to one of this groups
		if(category == EXTRACTOR)
		{
			ut->AddExtractor(unit);

			// order defence if necessary
			execute->DefendMex(unit, def->id);
		}
		else if(category == POWER_PLANT)
		{
			ut->AddPowerPlant(unit, def->id);
		}
		else if(category == STORAGE)
		{
			execute->futureStoredEnergy -= bt->unitList[def->id-1]->energyStorage;
			execute->futureStoredMetal -= bt->unitList[def->id-1]->metalStorage;
		}
		else if(category == METAL_MAKER)
		{
			ut->AddMetalMaker(unit, def->id);
		}
		else if(category == STATIONARY_RECON)
		{
			ut->AddRecon(unit, def->id);
		}
		else if(category == STATIONARY_JAMMER)
		{
			ut->AddJammer(unit, def->id);
		}
		else if(category == STATIONARY_ARTY)
		{
			ut->AddStationaryArty(unit, def->id);
		}
		else if(category == GROUND_FACTORY || category == SEA_FACTORY)
		{
			ut->AddFactory(unit, def->id);
		}
		return;
	}
	else	// unit was completed
	{
		// unit 
		if(category == GROUND_ASSAULT || category == AIR_ASSAULT || category == HOVER_ASSAULT || category == SEA_ASSAULT)
		{
			execute->AddUnitToGroup(unit, def->id, (UnitCategory) category);
		}
		// scout
		else if(bt->IsScout(category))
		{
			++activeScouts;
			--futureScouts;

			//char c[120];
			//sprintf(c, "scout completed: %i", futureScouts); 
			//cb->SendTextMsg(c, 0);

			scouts.push_back(unit);
		}
		// builder 
		else if(bt->IsBuilder(category))
		{
			ut->AddBuilder(unit, def->id);
		}
	}
}

void AAI::UnitIdle(int unit)
{
	// if factory is idle, start construction of further units
	if(ut->units[unit].factory)
		ut->units[unit].factory->Update();
	else if(ut->units[unit].builder)
	{
		ut->units[unit].builder->Idle();

		if(ut->builders.size() < 4)
			execute->CheckConstruction();
	}
}

void AAI::UnitMoveFailed(int unit) 
{
	// not a good solution at all, hopefully not necessary in the future due to bug fixes 
	// in the path finding routines
	float3 pos = cb->GetUnitPos(unit);

	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;
	
	if(x < 0 || y < 0 || x >= map->xSectors || y >= map->ySectors)
		return;

	pos = map->sector[x][y].GetCenter();

	execute->moveUnitTo(unit, &pos);

	// 
	const UnitDef *def = cb->GetUnitDef(unit);

	if(ut->units[unit].builder)
	{
		AAIBuilder* builder = ut->units[unit].builder;

		if(builder->task == BUILDING)
		{
			if(builder->building_unit_id == -1)
			{
				--bt->units_dynamic[builder->building_def_id].active;
				--futureUnits[builder->building_category];

				// clear up buildmap etc.
				execute->ConstructionFailed(-1, builder->build_pos, builder->building_def_id);

				// free builder
				builder->BuildingFinished();
			}
		}
	}
}

void AAI::EnemyEnterLOS(int enemy) {}
void AAI::EnemyLeaveLOS(int enemy) {}
void AAI::EnemyEnterRadar(int enemy) {}
void AAI::EnemyLeaveRadar(int enemy) {}

void AAI::EnemyDestroyed (int enemy, int attacker) 
{
	if(attacker)
	{	
		// get unit´s id 
		const UnitDef *def = cb->GetUnitDef(enemy);
		const UnitDef *def_att = cb->GetUnitDef(attacker);

		if(def_att)
		{
			if(bt->units_static[def_att->id].category == STATIONARY_ARTY)
				map->UpdateArty(attacker);

			// unit was destroyed
			if(def)
			{
				int killer = bt->GetIDOfAssaultCategory(bt->units_static[def_att->id].category);
				int killed = bt->GetIDOfAssaultCategory(bt->units_static[def->id].category);

				if(killer != -1 && killed != -1)
				{
					bt->UpdateTable(def_att, killer, def, killed);
					map->UpdateCategoryUsefulness(def_att, killer, def, killed);
				}
			}
		}
	}
}

void AAI::Update()
{
	int tick = cb->GetCurrentFrame();

	if(!initialized)
	{
		if(!(tick%600))
			cb->SendTextMsg("AAI not properly initialized, please view ai log for further information", 0);

		return;
	}

	// scouting
	if(!(tick%cfg->SCOUT_UPDATE_FREQUENCY))
	{
		execute->UpdateRecon(); // update threat values for all sectors, move scouts...
	}
	
	// update groups
	if(!(tick%219))
	{
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); category++)
		{
			for(list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); group++)
				(*group)->Update();
		}

		return;
	}

	// unit management 
	if(!(tick%649))
	{
		execute->CheckBuildques();
		brain->BuildUnits();
	}

	if(!(tick%611))
	{
		// check attack
		execute->UpdateAttack();
		//af->BombBestUnit(4, 4);
	}

	// ressource management
	if(!(tick%199))
	{
		execute->CheckRessources();
	}

	// unit management
	if(!(tick%419))
	{
		execute->CheckUnits();
	}

	// update sectors
	if(!(tick%423))
	{
		brain->UpdateAttackedByValues();
		map->UpdateSectors();
	}

	// builder management
	if(!(tick%617))
	{
		brain->UpdateDefenceCapabilities();
	}

	// update income 
	if(!(tick%59))
	{
		execute->UpdateRessources();
	}

	// building management
	if(!(tick%97))
	{
		execute->CheckConstruction();
	}

	// builder management
	if(!(tick%677))
	{
		for(set<int>::iterator builder = ut->builders.begin(); builder != ut->builders.end(); ++builder)
			ut->units[(*builder)].builder->Update();
	}

	// factory and defences
	if(!(tick%437))
	{
		execute->CheckFactories();
		execute->CheckDefences(); 

		// update factories
		for(set<int>::iterator i = ut->factories.begin(); i != ut->factories.end(); ++i)
			ut->units[*i].factory->Update();;
	}

	// build radar/jammer
	if(!(tick%1177))
	{
		execute->CheckRecon();
		execute->CheckJammer();
		execute->CheckStationaryArty();
		execute->CheckAirBase();
	}

	// upgrade mexes
	if(!(tick%1973))
	{
		execute->CheckMexUpgrade();
		execute->CheckRadarUpgrade();
		execute->CheckJammerUpgrade();
	}

	// recheck rally points
	if(!(tick%2577))
	{
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); category++)
		{
			for(list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); group++)
				(*group)->rally_point = execute->GetRallyPoint(*category);
		}
	}

	// recalculate efficiency stats
	if(!(tick%2827))
	{
		bt->UpdateMinMaxAvgEfficiency(side-1);
	}
}

int AAI::HandleEvent (int msg, const void *data)
{
	return 0;
}
