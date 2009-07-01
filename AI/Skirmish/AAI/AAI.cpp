// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include <set>
#include <math.h>
#include <stdio.h>

#include "AAI.h"


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
	am = 0;

	side = 0;

	initialized = false;
}

AAI::~AAI()
{
	if(!cfg->initialized)
		return;

	// save several ai data
	fprintf(file, "\nShutting down....\n\n");
	fprintf(file, "Unit category	active / under construction / requested\n");
	for(int i = 0; i <= MOBILE_CONSTRUCTOR; ++i)
	{
		fprintf(file, "%-20s: %i / %i / %i\n", bt->GetCategoryString2((UnitCategory)i), ut->activeUnits[i], ut->futureUnits[i], ut->requestedUnits[i]);
	}

	fprintf(file, "\nGround Groups:    %i\n", group_list[GROUND_ASSAULT].size());
	fprintf(file, "\nAir Groups:       %i\n", group_list[AIR_ASSAULT].size());
	fprintf(file, "\nHover Groups:     %i\n", group_list[HOVER_ASSAULT].size());
	fprintf(file, "\nSea Groups:       %i\n", group_list[SEA_ASSAULT].size());
	fprintf(file, "\nSubmarine Groups: %i\n\n", group_list[SUBMARINE_ASSAULT].size());

	fprintf(file, "Future metal/energy request: %i / %i\n", (int)execute->futureRequestedMetal, (int)execute->futureRequestedEnergy);
	fprintf(file, "Future metal/energy supply:  %i / %i\n\n", (int)execute->futureAvailableMetal, (int)execute->futureAvailableEnergy);

	fprintf(file, "Future/active builders:      %i / %i\n", ut->futureBuilders, ut->activeBuilders);
	fprintf(file, "Future/active factories:     %i / %i\n\n", ut->futureFactories, ut->activeFactories);

	fprintf(file, "Unit production rate: %i\n\n", execute->unitProductionRate);

	fprintf(file, "Requested constructors:\n");
	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].end(); ++fac)
		fprintf(file, "%-24s: %i\n", bt->unitList[*fac-1]->humanName.c_str(), bt->units_dynamic[*fac].requested);
	for(list<int>::iterator fac = bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].end(); ++fac)
		fprintf(file, "%-24s: %i\n", bt->unitList[*fac-1]->humanName.c_str(), bt->units_dynamic[*fac].requested);

	fprintf(file, "Factory ratings:\n");
	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].end(); ++fac)
		fprintf(file, "%-24s: %f\n", bt->unitList[*fac-1]->humanName.c_str(), bt->GetFactoryRating(*fac));

	fprintf(file, "Mobile constructor ratings:\n");
	for(list<int>::iterator cons = bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].begin(); cons != bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].end(); ++cons)
		fprintf(file, "%-24s: %f\n", bt->unitList[*cons-1]->humanName.c_str(), bt->GetBuilderRating(*cons));


	// delete buildtasks
	for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
	{
		delete (*task);
	}

	// save mod learning data
	bt->SaveBuildTable(brain->GetGamePeriod(), map->map_type);

	// delete unit groups
	for(int i = 0; i <= MOBILE_CONSTRUCTOR; i++)
	{
		for(list<AAIGroup*>::iterator group = group_list[i].begin(); group != group_list[i].end(); ++group)
		{
			(*group)->attack = 0;
			delete (*group);
		}
	}


	delete am;
	delete brain;
	delete execute;
	delete ut;
	delete af;
	delete map;
	delete bt;

	fclose(file);
}

void AAI::GotChatMsg(const char *msg, int player) {}

void AAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir) {}


void AAI::InitAI(IGlobalAICallback* callback, int team)
{
	aicb = callback;
	cb = callback->GetAICallback();

	// open log file
	char filename[500];
	char buffer[500];
	char team_number[3];

	SNPRINTF(team_number, 3, "%d", team);

	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, AILOG_PATH);
	STRCAT(buffer, "AAI_log_team_");
	STRCAT(buffer, team_number);
	STRCAT(buffer, ".txt");
	ReplaceExtension (buffer, filename, sizeof(filename), ".txt");

	cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	file = fopen(filename,"w");

	fprintf(file, "AAI %s running mod %s\n \n", AAI_VERSION, cb->GetModName());

	// load config file first
	cfg->LoadConfig(this);

	if(!cfg->initialized)
	{
		cb->SendTextMsg("Error: Could not load mod and/or general config file, see .../log/AILog.txt for further information",0);
		throw 1;
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
	group_list.resize(MOBILE_CONSTRUCTOR+1);

	// init airforce manager
	af = new AAIAirForceManager(this, cb, bt);

	// init attack manager
	am = new AAIAttackManager(this, cb, bt, map->continents.size());

	cb->SendTextMsg("AAI loaded", 0);
}

void AAI::UnitDamaged(int damaged, int attacker, float damage, float3 dir)
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

	// assault grups may be ordered to retreat
	if(cat >= GROUND_ASSAULT && cat <= SUBMARINE_ASSAULT)
			execute->CheckFallBack(damaged, def->id);

	// known attacker
	if(attacker >= 0)
	{
		// filter out friendly fire
		if(cb->GetUnitTeam(attacker) == cb->GetMyTeam())
			return;

		att_def = cb->GetUnitDef(attacker);

		if(att_def)
		{
			unsigned int att_movement_type = bt->units_static[att_def->id].movement_type;
			att_cat = bt->units_static[att_def->id].category;

			// retreat builders
			if(ut->IsBuilder(damaged))
				ut->units[damaged].cons->Retreat(att_cat);
			else
			{
				//if(att_cat >= GROUND_ASSAULT && att_cat <= SUBMARINE_ASSAULT)

				float3 pos = cb->GetUnitPos(attacker);
				AAISector *sector = map->GetSectorOfPos(&pos);

				if(sector && !am->SufficientDefencePowerAt(sector, 1.2f))
				{
					// building has been attacked
					if(cat <= METAL_MAKER)
						execute->DefendUnitVS(damaged, att_movement_type, &pos, 115);
					// builder
					else if(ut->IsBuilder(damaged))
						execute->DefendUnitVS(damaged, att_movement_type, &pos, 110);
					// normal units
					else
						execute->DefendUnitVS(damaged, att_movement_type, &pos, 105);
				}
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

		// retreat builders
		if(ut->IsBuilder(damaged))
			ut->units[damaged].cons->Retreat(att_cat);

		// building has been attacked
		//if(cat <= METAL_MAKER)
		//	execute->DefendUnitVS(damaged, def, att_cat, NULL, 115);
		//else if(ut->IsBuilder(damaged))
		//	execute->DefendUnitVS(damaged, def, att_cat, NULL, 110);
	}
}

void AAI::UnitCreated(int unit, int builder)
{
	if(!cfg->initialized)
		return;

	// get unit's id
	const UnitDef *def = cb->GetUnitDef(unit);
	UnitCategory category = bt->units_static[def->id].category;

	bt->units_dynamic[def->id].requested -= 1;
	bt->units_dynamic[def->id].under_construction += 1;

	ut->UnitCreated(category);

	// add to unittable
	ut->AddUnit(unit, def->id);

	// get commander a startup
	if(!initialized && ut->IsDefCommander(def->id))
	{
		// UnitFinished() will decrease it later -> prevents AAI from having -1 future commanders
		ut->requestedUnits[COMMANDER] += 1;
		ut->futureBuilders += 1;
		bt->units_dynamic[def->id].under_construction += 1;

		execute->InitAI(unit, def);

		initialized = true;
		return;
	}

	// resurrected units will be handled differently
	if( !cb->UnitBeingBuilt(unit))
	{
		cb->SendTextMsg("ressurected", 0);

		UnitCategory category = bt->units_static[def->id].category;

		// UnitFinished() will decrease it later
		ut->requestedUnits[category] += 1;
		bt->units_dynamic[def->id].requested += 1;

		if(bt->IsFactory(def->id))
			ut->futureFactories += 1;

		if(category <= METAL_MAKER && category > UNKNOWN)
		{
			float3 pos = cb->GetUnitPos(unit);
			execute->InitBuildingAt(def, &pos, pos.y < 0);
		}
	}
	else
	{
		// construction of building started
		if(category <= METAL_MAKER && category > UNKNOWN)
		{
			float3 pos = cb->GetUnitPos(unit);

			// create new buildtask
			execute->CreateBuildTask(unit, def, &pos);

			// add extractor to the sector
			if(category == EXTRACTOR)
			{
				int x = pos.x/map->xSectorSize;
				int y = pos.z/map->ySectorSize;

				map->sector[x][y].AddExtractor(unit, def->id, &pos);
			}
		}
	}
}

void AAI::UnitFinished(int unit)
{
	if(!initialized)
		return;

	// get unit's id
	const UnitDef *def = cb->GetUnitDef(unit);

	UnitCategory category = bt->units_static[def->id].category;

	ut->UnitFinished(category);

	bt->units_dynamic[def->id].under_construction -= 1;
	bt->units_dynamic[def->id].active += 1;

	// building was completed
	if(!def->movedata && !def->canfly)
	{
		// delete buildtask
		for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
		{
			if((*task)->unit_id == unit)
			{
				AAIBuildTask *build_task = *task;

				if((*task)->builder_id >= 0 && ut->units[(*task)->builder_id].cons)
					ut->units[(*task)->builder_id].cons->ConstructionFinished();

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
		else if(category == STATIONARY_CONSTRUCTOR)
		{
			ut->AddConstructor(unit, def->id);

			ut->units[unit].cons->Update();
		}
		return;
	}
	else	// unit was completed
	{
		// unit
		if(category >= GROUND_ASSAULT && category <= SUBMARINE_ASSAULT)
		{
			execute->AddUnitToGroup(unit, def->id, category);

			brain->AddDefenceCapabilities(def->id, category);

			ut->SetUnitStatus(unit, HEADING_TO_RALLYPOINT);
		}
		// scout
		else if(category == SCOUT)
		{
			ut->AddScout(unit);

			// cloak scout if cloakable
			if(def->canCloak)
			{
				Command c;
				c.id = CMD_CLOAK;
				c.params.push_back(1);

				cb->GiveOrder(unit, &c);
			}
		}
		// builder
		else if(bt->IsBuilder(def->id))
		{
			ut->AddConstructor(unit, def->id);

			ut->units[unit].cons->Update();
		}
	}
}

void AAI::UnitDestroyed(int unit, int attacker)
{
	// get unit's id
	const UnitDef *def = cb->GetUnitDef(unit);

	// get unit's category and position
	UnitCategory category = bt->units_static[def->id].category;
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
		ut->FutureUnitKilled(category);
		bt->units_dynamic[def->id].under_construction -= 1;

		// unfinished building
		if(!def->canfly && !def->movedata)
		{
			// delete buildtask
			for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
			{
				if((*task)->unit_id == unit)
				{
					(*task)->BuildtaskFailed();
					delete *task;

					build_tasks.erase(task);
					break;
				}
			}
		}
		// unfinished unit
		else
		{
			if(bt->IsBuilder(def->id))
			{
				--ut->futureBuilders;

				for(list<int>::iterator unit = bt->units_static[def->id].canBuildList.begin();  unit != bt->units_static[def->id].canBuildList.end(); ++unit)
					--bt->units_dynamic[*unit].constructorsRequested;
			}
			else if(bt->IsFactory(def->id))
			{
				if(category == STATIONARY_CONSTRUCTOR)
					--ut->futureFactories;

				for(list<int>::iterator unit = bt->units_static[def->id].canBuildList.begin();  unit != bt->units_static[def->id].canBuildList.end(); ++unit)
					--bt->units_dynamic[*unit].constructorsRequested;
			}
		}
	}
	else	// finished unit/building has been killed
	{
		ut->ActiveUnitKilled(category);

		bt->units_dynamic[def->id].active -= 1;

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
						bt->UpdateTable(def_att, killer, def, killed);
				}
			}
		}

		// finished building has been killed
		if(!def->canfly && !def->movedata)
		{
			// decrease number of units of that category in the target sector
			if(validSector)
				map->sector[x][y].RemoveBuildingType(def->id);

			// check if building belongs to one of this groups
			if(category == STATIONARY_DEF)
			{
				// remove defence from map
				map->RemoveDefence(&pos, def->id);
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

			// clean up buildmap & some other stuff
			if(category == STATIONARY_CONSTRUCTOR)
			{
				ut->RemoveConstructor(unit, def->id);

				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), true);

				// speed up reconstruction
				execute->urgency[STATIONARY_CONSTRUCTOR] += 1.5;
			}
			// hq
			else if(category == COMMANDER)
			{
				ut->RemoveCommander(unit, def->id);

				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), true);
			}
			// other building
			else
				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), false);

			// if no buildings left in that sector, remove from base sectors
			if(map->sector[x][y].own_structures == 0 && brain->sectors[0].size() > 2)
			{
				brain->RemoveSector(&map->sector[x][y]);

				brain->UpdateNeighbouringSectors();
				brain->UpdateBaseCenter();

				brain->expandable = true;

				fprintf(file, "\nRemoving sector %i,%i from base; base size: %i \n", x, y, brain->sectors[0].size());
			}
		}
		else // finished unit has been killed
		{
			// scout
			if(category == SCOUT)
			{
				ut->RemoveScout(unit);

				// add enemy building to sector
				if(validSector && map->sector[x][y].distance_to_base > 0)
					map->sector[x][y].enemy_structures += 5.0f;

			}
			// assault units
			else if(category >= GROUND_ASSAULT && category <= SUBMARINE_ASSAULT)
			{
				// look for a safer rallypoint if units get killed on their way
				if(ut->units[unit].status == HEADING_TO_RALLYPOINT)
					ut->units[unit].group->GetNewRallyPoint();

				ut->units[unit].group->RemoveUnit(unit, attacker);
			}
			// builder
			else if(bt->IsBuilder(def->id))
			{
				ut->RemoveConstructor(unit, def->id);
			}
			else if(category == COMMANDER)
			{
				ut->RemoveCommander(unit, def->id);
			}
		}
	}

	ut->RemoveUnit(unit);
}

void AAI::UnitIdle(int unit)
{
	// if factory is idle, start construction of further units
	if(ut->units[unit].cons)
	{
		if(ut->units[unit].cons->assistance < 0 && ut->units[unit].cons->construction_unit_id < 0 )
		{
			ut->SetUnitStatus(unit, UNIT_IDLE);

			ut->units[unit].cons->Idle();

			if(ut->constructors.size() < 4)
				execute->CheckConstruction();
		}
	}
	// idle combat units will report to their groups
	else if(ut->units[unit].group)
	{
		//ut->SetUnitStatus(unit, UNIT_IDLE);
		ut->units[unit].group->UnitIdle(unit);
	}
	else if(bt->units_static[ut->units[unit].def_id].category == SCOUT)
	{
		execute->SendScoutToNewDest(unit);
	}
	else
		ut->SetUnitStatus(unit, UNIT_IDLE);
}

void AAI::UnitMoveFailed(int unit)
{
	if(ut->units[unit].cons)
	{
		if(ut->units[unit].cons->task == BUILDING && ut->units[unit].cons->construction_unit_id == -1)
			ut->units[unit].cons->ConstructionFailed();
	}

	float3 pos = cb->GetUnitPos(unit);

	pos.x = pos.x - 64 + 32 * (rand()%5);
	pos.z = pos.z - 64 + 32 * (rand()%5);

	if(pos.x < 0)
		pos.x = 0;

	if(pos.z < 0)
		pos.z = 0;

	// workaround: prevent flooding the interface with move orders if a unit gets stuck
	if(cb->GetCurrentFrame() - ut->units[unit].last_order < 5)
		return;
	else
		execute->MoveUnitTo(unit, &pos);
}

void AAI::EnemyEnterLOS(int enemy) {}
void AAI::EnemyLeaveLOS(int enemy) {}
void AAI::EnemyEnterRadar(int enemy) {}
void AAI::EnemyLeaveRadar(int enemy) {}

void AAI::EnemyDestroyed(int enemy, int attacker)
{
	// remove enemy from unittable
	ut->EnemyKilled(enemy);

	if(attacker)
	{
		// get unit�s id
		const UnitDef *def = cb->GetUnitDef(enemy);
		const UnitDef *def_att = cb->GetUnitDef(attacker);

		if(def_att)
		{
			// unit was destroyed
			if(def)
			{
				int killer = bt->GetIDOfAssaultCategory(bt->units_static[def_att->id].category);
				int killed = bt->GetIDOfAssaultCategory(bt->units_static[def->id].category);

				if(killer != -1 && killed != -1)
					bt->UpdateTable(def_att, killer, def, killed);
			}
		}
	}
}

void AAI::Update()
{
	int tick = cb->GetCurrentFrame();

	if(tick < 0)
		return;

	if(!initialized)
	{
		if(!(tick%450))
			cb->SendTextMsg("Failed to initialize AAI! Please view ai log for further information and check if AAI supports this mod", 0);

		return;
	}

	// scouting
	if(!(tick%cfg->SCOUT_UPDATE_FREQUENCY))
		map->UpdateRecon();

	if(!((tick+5)%cfg->SCOUT_UPDATE_FREQUENCY))
		map->UpdateEnemyScoutingData();

	// update groups
	if(!(tick%169))
	{
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); ++group)
				(*group)->Update();
		}

		return;
	}

	// unit management
	if(!(tick%649))
	{
		execute->CheckBuildqueues();
		brain->BuildUnits();
		execute->BuildScouts();
	}

	if(!(tick%911))
	{
		// check attack
		am->Update();
		af->BombBestUnit(2, 2);
		return;
	}

	// ressource management
	if(!(tick%199))
	{
		execute->CheckRessources();
	}

	// update sectors
	if(!(tick%423))
	{
		brain->UpdateAttackedByValues();
		map->UpdateSectors();

		brain->UpdatePressureByEnemy();

		/*if(brain->enemy_pressure_estimation > 0.01f)
		{
			char c[20];
			SNPRINTF(c, 20, "%f", brain->enemy_pressure_estimation);
			cb->SendTextMsg(c, 0);
		}*/
	}

	// builder management
	if(!(tick%917))
		brain->UpdateDefenceCapabilities();

	// update income
	if(!(tick%45))
		execute->UpdateRessources();

	// building management
	if(!(tick%97))
		execute->CheckConstruction();

	// builder/factory management
	if(!(tick%677))
	{
		for(set<int>::iterator builder = ut->constructors.begin(); builder != ut->constructors.end(); ++builder)
			ut->units[(*builder)].cons->Update();
	}

	if(!(tick%337))
		execute->CheckFactories();

	if(!(tick%1079))
		execute->CheckDefences();

	// build radar/jammer
	if(!(tick%1177))
	{
		execute->CheckRecon();
		execute->CheckJammer();
		execute->CheckStationaryArty();
		execute->CheckAirBase();
	}

	// upgrade mexes
	if(!(tick%1573))
	{
		if(brain->enemy_pressure_estimation < 0.05f)
		{
			execute->CheckMexUpgrade();
			execute->CheckRadarUpgrade();
			execute->CheckJammerUpgrade();
		}
	}

	// recheck rally points
	if(!(tick%1877))
	{
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); ++group)
				(*group)->UpdateRallyPoint();
		}
	}

	// recalculate efficiency stats
	if(!(tick%2927))
	{
		if(aai_instance == 1)
			bt->UpdateMinMaxAvgEfficiency();
	}
}

int AAI::HandleEvent(int msg, const void* data)
{
	switch (msg)
	{
		case AI_EVENT_UNITGIVEN: // 1
			{
				const IGlobalAI::ChangeTeamEvent* cte =
						(const IGlobalAI::ChangeTeamEvent*) data;
				if(cte->newteam == cb->GetMyTeam())
				{
					UnitCreated(cte->unit, -1);
					UnitFinished(cte->unit);
				}
				break;
			}
		case AI_EVENT_UNITCAPTURED: // 2
			{
				const IGlobalAI::ChangeTeamEvent* cte =
						(const IGlobalAI::ChangeTeamEvent*) data;
					if ((cte->oldteam) == (cb->GetMyTeam())) {
					UnitDestroyed(cte->unit, -1);
				}
				break;
			}
	}
	return 0;
}
