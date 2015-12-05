// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include <math.h>
#include <stdarg.h>
#include <time.h>

#include "AAI.h"
#include "AAIBuildTable.h"
#include "AAIAirForceManager.h"
#include "AAIExecute.h"
#include "AAIUnitTable.h"
#include "AAIBuildTask.h"
#include "AAIBrain.h"
#include "AAIConstructor.h"
#include "AAIAttackManager.h"
#include "AIExport.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAIGroup.h"
#include "AAISector.h"


#include "System/Util.h"

#include "LegacyCpp/IGlobalAICallback.h"
#include "LegacyCpp/UnitDef.h"
using namespace springLegacyAI;



#include "CUtils/SimpleProfiler.h"
#define AAI_SCOPED_TIMER(part) SCOPED_TIMER(part, profiler);

int AAI::aai_instance = 0;

AAI::AAI() :
	cb(NULL),
	aicb(NULL),
	side(0),
	brain(NULL),
	execute(NULL),
	ut(NULL),
	bt(NULL),
	map(NULL),
	af(NULL),
	am(NULL),
	profiler(NULL),
	file(NULL),
	initialized(false)
{
	// initialize random numbers generator
	srand (time(NULL));
}

AAI::~AAI()
{
	aai_instance--;
	if (!initialized)
		return;

	// save several AI data
	Log("\nShutting down....\n\n");

	Log("\nProfiling summary:\n");
	Log("%s\n", profiler->ToString().c_str());

	Log("Unit category active / under construction / requested\n");
	for(int i = 0; i <= MOBILE_CONSTRUCTOR; ++i)
	{
		Log("%-20s: %i / %i / %i\n", bt->GetCategoryString2((UnitCategory)i), ut->activeUnits[i], ut->futureUnits[i], ut->requestedUnits[i]);
	}

	Log("\nGround Groups:    " _STPF_ "\n", group_list[GROUND_ASSAULT].size());
	Log("\nAir Groups:       " _STPF_ "\n", group_list[AIR_ASSAULT].size());
	Log("\nHover Groups:     " _STPF_ "\n", group_list[HOVER_ASSAULT].size());
	Log("\nSea Groups:       " _STPF_ "\n", group_list[SEA_ASSAULT].size());
	Log("\nSubmarine Groups: " _STPF_ "\n\n", group_list[SUBMARINE_ASSAULT].size());

	Log("Future metal/energy request: %i / %i\n", (int)execute->futureRequestedMetal, (int)execute->futureRequestedEnergy);
	Log("Future metal/energy supply:  %i / %i\n\n", (int)execute->futureAvailableMetal, (int)execute->futureAvailableEnergy);

	Log("Future/active builders:      %i / %i\n", ut->futureBuilders, ut->activeBuilders);
	Log("Future/active factories:     %i / %i\n\n", ut->futureFactories, ut->activeFactories);

	Log("Unit production rate: %i\n\n", execute->unitProductionRate);

	Log("Requested constructors:\n");
	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].end(); ++fac) {
		assert((*fac)   < bt->units_dynamic.size());
		Log("%-24s: %i\n", bt->GetUnitDef(*fac).humanName.c_str(), bt->units_dynamic[*fac].requested);
	}
	for(list<int>::iterator fac = bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].end(); ++fac)
		Log("%-24s: %i\n", bt->GetUnitDef(*fac).humanName.c_str(), bt->units_dynamic[*fac].requested);

	Log("Factory ratings:\n");
	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][side-1].end(); ++fac)
		Log("%-24s: %f\n", bt->GetUnitDef(*fac).humanName.c_str(), bt->GetFactoryRating(*fac));

	Log("Mobile constructor ratings:\n");
	for(list<int>::iterator cons = bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].begin(); cons != bt->units_of_category[MOBILE_CONSTRUCTOR][side-1].end(); ++cons)
		Log("%-24s: %f\n", bt->GetUnitDef(*cons).humanName.c_str(), bt->GetBuilderRating(*cons));


	// delete buildtasks
	for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
	{
		delete (*task);
	}
	build_tasks.clear();

	// save mod learning data
	bt->SaveBuildTable(brain->GetGamePeriod(), map->map_type);

	SafeDelete(am);
	SafeDelete(af);

	// delete unit groups
	for(int i = 0; i <= MOBILE_CONSTRUCTOR; i++)
	{
		for(list<AAIGroup*>::iterator group = group_list[i].begin(); group != group_list[i].end(); ++group)
		{
			(*group)->attack = 0;
			delete (*group);
		}
		group_list[i].clear();
	}


	SafeDelete(brain);
	SafeDelete(execute);
	SafeDelete(ut);
	SafeDelete(map);
	SafeDelete(bt);
	SafeDelete(profiler);

	initialized = false;
	fclose(file);
	file = NULL;
}


//void AAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir) {}


void AAI::InitAI(IGlobalAICallback* callback, int team)
{
	aai_instance++;
	char profilerName[16];
	SNPRINTF(profilerName, sizeof(profilerName), "%s:%i", "AAI", team);
	profiler = new Profiler(profilerName);

	AAI_SCOPED_TIMER("InitAI")
	aicb = callback;
	cb = callback->GetAICallback();

	// open log file
	// this size equals the one used in "AIAICallback::GetValue(AIVAL_LOCATE_FILE_..."
	char filename[2048];
	char buffer[500];
	char team_number[3];

	SNPRINTF(team_number, 3, "%d", team);

	STRCPY(buffer, "");
	STRCAT(buffer, AILOG_PATH);
	STRCAT(buffer, "AAI_log_team_");
	STRCAT(buffer, team_number);
	STRCAT(buffer, ".txt");
	ReplaceExtension (buffer, filename, sizeof(filename), ".txt");

	cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	file = fopen(filename,"w");

	Log("AAI %s running mod %s\n \n", AAI_VERSION, cb->GetModHumanName());

	// load config file first
	cfg->LoadConfig(this);

	if (!cfg->initialized)
	{
		std::string errorMsg =
				std::string("Error: Could not load mod and/or general config file."
					" For further information see the config file under: ") +
				filename;
		LogConsole("%s", errorMsg.c_str());
		throw 1;
	}

	// create buildtable
	bt = new AAIBuildTable(this);
	bt->Init();

	// init unit table
	ut = new AAIUnitTable(this);

	// init map
	map = new AAIMap(this);
	map->Init();

	// init brain
	brain = new AAIBrain(this);

	// init executer
	execute = new AAIExecute(this);

	// create unit groups
	group_list.resize(MOBILE_CONSTRUCTOR+1);

	// init airforce manager
	af = new AAIAirForceManager(this);

	// init attack manager
	am = new AAIAttackManager(this, map->continents.size());

	LogConsole("AAI loaded");
}

void AAI::UnitDamaged(int damaged, int attacker, float /*damage*/, float3 /*dir*/)
{
	AAI_SCOPED_TIMER("UnitDamaged")
	const UnitDef* def;
	const UnitDef* att_def;
	UnitCategory att_cat, cat;

	// filter out commander
	if (ut->cmdr != -1)
	{
		if (damaged == ut->cmdr)
			brain->DefendCommander(attacker);
	}

	def = cb->GetUnitDef(damaged);

	if (def)
		cat =  bt->units_static[def->id].category;
	else
		cat = UNKNOWN;

	// assault grups may be ordered to retreat
	// (range check prevents a NaN)
	if (cat >= GROUND_ASSAULT && cat <= SUBMARINE_ASSAULT && bt->units_static[def->id].range > 0.0f)
			execute->CheckFallBack(damaged, def->id);

	// known attacker
	if (attacker >= 0)
	{
		// filter out friendly fire
		if (cb->GetUnitTeam(attacker) == cb->GetMyTeam())
			return;

		att_def = cb->GetUnitDef(attacker);

		if (att_def)
		{
			unsigned int att_movement_type = bt->units_static[att_def->id].movement_type;
			att_cat = bt->units_static[att_def->id].category;

			// retreat builders
			if (ut->IsBuilder(damaged))
				ut->units[damaged].cons->Retreat(att_cat);
			else
			{
				//if (att_cat >= GROUND_ASSAULT && att_cat <= SUBMARINE_ASSAULT)

				float3 pos = cb->GetUnitPos(attacker);
				AAISector *sector = map->GetSectorOfPos(&pos);

				if (sector && !am->SufficientDefencePowerAt(sector, 1.2f))
				{
					// building has been attacked
					if (cat <= METAL_MAKER)
						execute->DefendUnitVS(damaged, att_movement_type, &pos, 115);
					// builder
					else if (ut->IsBuilder(damaged))
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

		if (pos.y > 0)
			att_cat = GROUND_ASSAULT;
		else
			att_cat = SEA_ASSAULT;

		// retreat builders
		if (ut->IsBuilder(damaged))
			ut->units[damaged].cons->Retreat(att_cat);

		// building has been attacked
		//if (cat <= METAL_MAKER)
		//	execute->DefendUnitVS(damaged, def, att_cat, NULL, 115);
		//else if (ut->IsBuilder(damaged))
		//	execute->DefendUnitVS(damaged, def, att_cat, NULL, 110);
	}
}

void AAI::UnitCreated(int unit, int /*builder*/)
{
	AAI_SCOPED_TIMER("UnitCreated")
	if (!cfg->initialized)
		return;

	// get unit's id
	const UnitDef* def = cb->GetUnitDef(unit);
	UnitCategory category = bt->units_static[def->id].category;

	bt->units_dynamic[def->id].requested -= 1;
	bt->units_dynamic[def->id].under_construction += 1;

	ut->UnitCreated(category);

	// add to unittable
	ut->AddUnit(unit, def->id);

	// get commander a startup
	if (!initialized && ut->IsDefCommander(def->id))
	{
		// UnitFinished() will decrease it later -> prevents AAI from having -1 future commanders
		ut->requestedUnits[COMMANDER] += 1;
		ut->futureBuilders += 1;
		bt->units_dynamic[def->id].under_construction += 1;

		// set side
		side = bt->GetSideByID(def->id);

		execute->InitAI(unit, def);

		initialized = true;
		return;
	}

	// resurrected units will be handled differently
	if ( !cb->UnitBeingBuilt(unit))
	{
		LogConsole("ressurected", 0);

		UnitCategory category = bt->units_static[def->id].category;

		// UnitFinished() will decrease it later
		ut->requestedUnits[category] += 1;
		bt->units_dynamic[def->id].requested += 1;

		if (bt->IsFactory(def->id))
			ut->futureFactories += 1;

		if (category <= METAL_MAKER && category > UNKNOWN)
		{
			float3 pos = cb->GetUnitPos(unit);
			execute->InitBuildingAt(def, &pos, pos.y < 0);
		}
	}
	else
	{
		// construction of building started
		if (category <= METAL_MAKER && category > UNKNOWN)
		{
			float3 pos = cb->GetUnitPos(unit);

			// create new buildtask
			execute->CreateBuildTask(unit, def, &pos);

			// add extractor to the sector
			if (category == EXTRACTOR)
			{
				const int x = pos.x / map->xSectorSize;
				const int y = pos.z / map->ySectorSize;

				map->sector[x][y].AddExtractor(unit, def->id, &pos);
			}
		}
	}
}

void AAI::UnitFinished(int unit)
{
	AAI_SCOPED_TIMER("UnitFinished")
	if (!initialized)
		return;

	// get unit's id
	const UnitDef* def = cb->GetUnitDef(unit);

	UnitCategory category = bt->units_static[def->id].category;

	ut->UnitFinished(category);

	bt->units_dynamic[def->id].under_construction -= 1;
	bt->units_dynamic[def->id].active += 1;

	// building was completed
	if (!def->movedata && !def->canfly)
	{
		// delete buildtask
		for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
		{
			if ((*task)->unit_id == unit)
			{
				AAIBuildTask *build_task = *task;

				if ((*task)->builder_id >= 0 && ut->units[(*task)->builder_id].cons)
					ut->units[(*task)->builder_id].cons->ConstructionFinished();

				build_tasks.erase(task);
				SafeDelete(build_task);
				break;
			}
		}

		// check if building belongs to one of this groups
		if (category == EXTRACTOR)
		{
			ut->AddExtractor(unit);

			// order defence if necessary
			execute->DefendMex(unit, def->id);
		}
		else if (category == POWER_PLANT)
		{
			ut->AddPowerPlant(unit, def->id);
		}
		else if (category == STORAGE)
		{
			execute->futureStoredEnergy -= bt->GetUnitDef(def->id).energyStorage;
			execute->futureStoredMetal -= bt->GetUnitDef(def->id).metalStorage;
		}
		else if (category == METAL_MAKER)
		{
			ut->AddMetalMaker(unit, def->id);
		}
		else if (category == STATIONARY_RECON)
		{
			ut->AddRecon(unit, def->id);
		}
		else if (category == STATIONARY_JAMMER)
		{
			ut->AddJammer(unit, def->id);
		}
		else if (category == STATIONARY_ARTY)
		{
			ut->AddStationaryArty(unit, def->id);
		}
		else if (category == STATIONARY_CONSTRUCTOR)
		{
			ut->AddConstructor(unit, def->id);

			ut->units[unit].cons->Update();
		}
		return;
	}
	else	// unit was completed
	{
		// unit
		if (category >= GROUND_ASSAULT && category <= SUBMARINE_ASSAULT)
		{
			execute->AddUnitToGroup(unit, def->id, category);

			brain->AddDefenceCapabilities(def->id, category);

			ut->SetUnitStatus(unit, HEADING_TO_RALLYPOINT);
		}
		// scout
		else if (category == SCOUT)
		{
			ut->AddScout(unit);

			// cloak scout if cloakable
			if (def->canCloak)
			{
				Command c;
				c.id = CMD_CLOAK;
				c.params.push_back(1);

				cb->GiveOrder(unit, &c);
			}
		}
		// builder
		else if (bt->IsBuilder(def->id))
		{
			ut->AddConstructor(unit, def->id);

			ut->units[unit].cons->Update();
		}
	}
}

void AAI::UnitDestroyed(int unit, int attacker)
{
	AAI_SCOPED_TIMER("UnitDestroyed")
	// get unit's id
	const UnitDef* def = cb->GetUnitDef(unit);

	// get unit's category and position
	UnitCategory category = bt->units_static[def->id].category;
	float3 pos = cb->GetUnitPos(unit);

	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;

	// check if unit pos is within a valid sector (e.g. aircraft flying outside of the map)
	bool validSector = true;

	if (x >= map->xSectors || x < 0 || y < 0 || y >= map->ySectors)
		validSector = false;

	// update threat map
	if (attacker && validSector)
	{
		const UnitDef* att_def = cb->GetUnitDef(attacker);

		if (att_def)
			map->sector[x][y].UpdateThreatValues((UnitCategory)category, bt->units_static[att_def->id].category);
	}

	// unfinished unit has been killed
	if (cb->UnitBeingBuilt(unit))
	{
		ut->FutureUnitKilled(category);
		bt->units_dynamic[def->id].under_construction -= 1;

		// unfinished building
		if (!def->canfly && !def->movedata)
		{
			// delete buildtask
			for(list<AAIBuildTask*>::iterator task = build_tasks.begin(); task != build_tasks.end(); ++task)
			{
				if ((*task)->unit_id == unit)
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
			if (bt->IsBuilder(def->id))
			{
				--ut->futureBuilders;

				for(list<int>::iterator unit = bt->units_static[def->id].canBuildList.begin();  unit != bt->units_static[def->id].canBuildList.end(); ++unit)
					--bt->units_dynamic[*unit].constructorsRequested;
			}
			else if (bt->IsFactory(def->id))
			{
				if (category == STATIONARY_CONSTRUCTOR)
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
		assert(bt->units_dynamic[def->id].active >= 0);

		// update buildtable
		if (attacker)
		{
			const UnitDef* def_att = cb->GetUnitDef(attacker);

			if (def_att)
			{
				int killer = bt->GetIDOfAssaultCategory(bt->units_static[def_att->id].category);
				int killed = bt->GetIDOfAssaultCategory((UnitCategory)category);

				// check if valid id
				if (killer != -1)
				{
					brain->AttackedBy(killer);

					if (killed != -1)
						bt->UpdateTable(def_att, killer, def, killed);
				}
			}
		}

		// finished building has been killed
		if (!def->canfly && !def->movedata)
		{
			// decrease number of units of that category in the target sector
			if (validSector)
				map->sector[x][y].RemoveBuildingType(def->id);

			// check if building belongs to one of this groups
			if (category == STATIONARY_DEF)
			{
				// remove defence from map
				map->RemoveDefence(&pos, def->id);
			}
			else if (category == EXTRACTOR)
			{
				ut->RemoveExtractor(unit);

				// mark spots of destroyed mexes as unoccupied
				map->sector[x][y].FreeMetalSpot(cb->GetUnitPos(unit), def);
			}
			else if (category == POWER_PLANT)
			{
				ut->RemovePowerPlant(unit);
			}
			else if (category == STATIONARY_ARTY)
			{
				ut->RemoveStationaryArty(unit);
			}
			else if (category == STATIONARY_RECON)
			{
				ut->RemoveRecon(unit);
			}
			else if (category == STATIONARY_JAMMER)
			{
				ut->RemoveJammer(unit);
			}
			else if (category == METAL_MAKER)
			{
				ut->RemoveMetalMaker(unit);
			}

			// clean up buildmap & some other stuff
			if (category == STATIONARY_CONSTRUCTOR)
			{
				ut->RemoveConstructor(unit, def->id);

				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), true);

				// speed up reconstruction
				execute->urgency[STATIONARY_CONSTRUCTOR] += 1.5;
			}
			// hq
			else if (category == COMMANDER)
			{
				ut->RemoveCommander(unit, def->id);

				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), true);
			}
			// other building
			else
				map->UpdateBuildMap(pos, def, false, bt->CanPlacedWater(def->id), false);

			// if no buildings left in that sector, remove from base sectors
			if (map->sector[x][y].own_structures == 0 && brain->sectors[0].size() > 2)
			{
				brain->RemoveSector(&map->sector[x][y]);

				brain->UpdateNeighbouringSectors();
				brain->UpdateBaseCenter();

				brain->expandable = true;

				Log("\nRemoving sector %i,%i from base; base size: " _STPF_ " \n", x, y, brain->sectors[0].size());
			}
		}
		else // finished unit has been killed
		{
			// scout
			if (category == SCOUT)
			{
				ut->RemoveScout(unit);

				// add enemy building to sector
				if (validSector && map->sector[x][y].distance_to_base > 0)
					map->sector[x][y].enemy_structures += 5.0f;

			}
			// assault units
			else if (category >= GROUND_ASSAULT && category <= SUBMARINE_ASSAULT)
			{
				// look for a safer rallypoint if units get killed on their way
				if (ut->units[unit].status == HEADING_TO_RALLYPOINT)
					ut->units[unit].group->GetNewRallyPoint();

				ut->units[unit].group->RemoveUnit(unit, attacker);
			}
			// builder
			else if (bt->IsBuilder(def->id))
			{
				ut->RemoveConstructor(unit, def->id);
			}
			else if (category == COMMANDER)
			{
				ut->RemoveCommander(unit, def->id);
			}
		}
	}

	ut->RemoveUnit(unit);
}

void AAI::UnitIdle(int unit)
{
	AAI_SCOPED_TIMER("UnitIdle")
	// if factory is idle, start construction of further units
	if (ut->units[unit].cons)
	{
		if (ut->units[unit].cons->assistance < 0 && ut->units[unit].cons->construction_unit_id < 0 )
		{
			ut->SetUnitStatus(unit, UNIT_IDLE);

			ut->units[unit].cons->Idle();

			if (ut->constructors.size() < 4)
				execute->CheckConstruction();
		}
	}
	// idle combat units will report to their groups
	else if (ut->units[unit].group)
	{
		//ut->SetUnitStatus(unit, UNIT_IDLE);
		ut->units[unit].group->UnitIdle(unit);
	}
	else if (bt->units_static[ut->units[unit].def_id].category == SCOUT)
	{
		execute->SendScoutToNewDest(unit);
	}
	else
		ut->SetUnitStatus(unit, UNIT_IDLE);
}

void AAI::UnitMoveFailed(int unit)
{
	AAI_SCOPED_TIMER("UnitMoveFailed")
	if (ut->units[unit].cons)
	{
		if (ut->units[unit].cons->task == BUILDING && ut->units[unit].cons->construction_unit_id == -1)
			ut->units[unit].cons->ConstructionFailed();
	}

	float3 pos = cb->GetUnitPos(unit);

	pos.x = pos.x - 64 + 32 * (rand()%5);
	pos.z = pos.z - 64 + 32 * (rand()%5);

	if (pos.x < 0)
		pos.x = 0;

	if (pos.z < 0)
		pos.z = 0;

	// workaround: prevent flooding the interface with move orders if a unit gets stuck
	if (cb->GetCurrentFrame() - ut->units[unit].last_order < 5)
		return;
	else
		execute->MoveUnitTo(unit, &pos);
}

void AAI::EnemyEnterLOS(int /*enemy*/) {}
void AAI::EnemyLeaveLOS(int /*enemy*/) {}
void AAI::EnemyEnterRadar(int /*enemy*/) {}
void AAI::EnemyLeaveRadar(int /*enemy*/) {}

void AAI::EnemyDestroyed(int enemy, int attacker)
{
	AAI_SCOPED_TIMER("EnemyDestroyed")
	// remove enemy from unittable
	ut->EnemyKilled(enemy);

	if (attacker)
	{
		// get unit's id
		const UnitDef* def = cb->GetUnitDef(enemy);
		const UnitDef* def_att = cb->GetUnitDef(attacker);

		if (def_att)
		{
			// unit was destroyed
			if (def)
			{
				const int killer = bt->GetIDOfAssaultCategory(bt->units_static[def_att->id].category);
				const int killed = bt->GetIDOfAssaultCategory(bt->units_static[def->id].category);

				if (killer != -1 && killed != -1)
					bt->UpdateTable(def_att, killer, def, killed);
			}
		}
	}
}

void AAI::Update()
{
	const int tick = cb->GetCurrentFrame();

	if (tick < 0)
	{
		return;
	}

	if (!initialized)
	{
		if (!(tick % 450))
		{
			LogConsole("Failed to initialize AAI! Please view ai log for further information and check if AAI supports this mod");
		}

		return;
	}

	// scouting
	if (!(tick % cfg->SCOUT_UPDATE_FREQUENCY))
	{
		AAI_SCOPED_TIMER("Scouting_1")
		map->UpdateRecon();
	}

	if (!((tick + 5) % cfg->SCOUT_UPDATE_FREQUENCY))
	{
		AAI_SCOPED_TIMER("Scouting_2")
		map->UpdateEnemyScoutingData();
	}

	// update groups
	if (!(tick % 169))
	{
		AAI_SCOPED_TIMER("Groups")
		for(list<UnitCategory>::const_iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); ++group)
			{
				(*group)->Update();
			}
		}

		return;
	}

	// unit management
	if (!(tick % 649))
	{
		AAI_SCOPED_TIMER("Unit-Management")
		execute->CheckBuildqueues();
		brain->BuildUnits();
		execute->BuildScouts();
	}

	if (!(tick % 911))
	{
		AAI_SCOPED_TIMER("Check-Attack")
		// check attack
		am->Update();
		af->BombBestUnit(2, 2);
		return;
	}

	// ressource management
	if (!(tick % 199))
	{
		AAI_SCOPED_TIMER("Resource-Management")
		execute->CheckRessources();
	}

	// update sectors
	if (!(tick % 423))
	{
		AAI_SCOPED_TIMER("Update-Sectors")
		brain->UpdateAttackedByValues();
		map->UpdateSectors();

		brain->UpdatePressureByEnemy();

		/*if (brain->enemy_pressure_estimation > 0.01f)
		{
			LogConsole("%f", brain->enemy_pressure_estimation);
		}*/
	}

	// builder management
	if (!(tick % 917))
	{
		AAI_SCOPED_TIMER("Builder-Management")
		brain->UpdateDefenceCapabilities();
	}

	// update income
	if (!(tick % 45))
	{
		AAI_SCOPED_TIMER("Update-Income")
		execute->UpdateRessources();
	}

	// building management
	if (!(tick % 97))
	{
		AAI_SCOPED_TIMER("Building-Management")
		execute->CheckConstruction();
	}

	// builder/factory management
	if (!(tick % 677))
	{
		AAI_SCOPED_TIMER("BuilderAndFactory-Management")
		for (set<int>::iterator builder = ut->constructors.begin(); builder != ut->constructors.end(); ++builder)
		{
			ut->units[(*builder)].cons->Update();
		}
	}

	if (!(tick % 337))
	{
		AAI_SCOPED_TIMER("Check-Factories")
		execute->CheckFactories();
	}

	if (!(tick % 1079))
	{
		AAI_SCOPED_TIMER("Check-Defenses")
		execute->CheckDefences();
	}

	// build radar/jammer
	if (!(tick % 1177))
	{
		execute->CheckRecon();
		execute->CheckJammer();
		execute->CheckStationaryArty();
		execute->CheckAirBase();
	}

	// upgrade mexes
	if (!(tick % 1573))
	{
		AAI_SCOPED_TIMER("Upgrade-Mexes")
		if (brain->enemy_pressure_estimation < 0.05f)
		{
			execute->CheckMexUpgrade();
			execute->CheckRadarUpgrade();
			execute->CheckJammerUpgrade();
		}
	}

	// recheck rally points
	if (!(tick % 1877))
	{
		AAI_SCOPED_TIMER("Recheck-Rally-Points")
		for (list<UnitCategory>::const_iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for (list<AAIGroup*>::iterator group = group_list[*category].begin(); group != group_list[*category].end(); ++group)
			{
				(*group)->UpdateRallyPoint();
			}
		}
	}

	// recalculate efficiency stats
	if (!(tick % 2927))
	{
		AAI_SCOPED_TIMER("Recalculate-Efficiency-Stats")
		if (aai_instance == 1)
		{
			bt->UpdateMinMaxAvgEfficiency();
		}
	}
}

int AAI::HandleEvent(int msg, const void* data)
{
	AAI_SCOPED_TIMER("HandleEvent")
	switch (msg)
	{
		case AI_EVENT_UNITGIVEN: // 1
		case AI_EVENT_UNITCAPTURED: // 2
			{
				const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;

				const int myAllyTeamId = cb->GetMyAllyTeam();
				const bool oldEnemy = !cb->IsAllied(myAllyTeamId, cb->GetTeamAllyTeam(cte->oldteam));
				const bool newEnemy = !cb->IsAllied(myAllyTeamId, cb->GetTeamAllyTeam(cte->newteam));

				if (oldEnemy && !newEnemy) {
					// unit changed from an enemy to an allied team
					// we got a new friend! :)
					EnemyDestroyed(cte->unit, -1);
				} else if (!oldEnemy && newEnemy) {
					// unit changed from an ally to an enemy team
					// we lost a friend! :(
					EnemyCreated(cte->unit);
					if (!cb->UnitBeingBuilt(cte->unit)) {
						EnemyFinished(cte->unit);
					}
				}

				if (cte->oldteam == cb->GetMyTeam()) {
					// we lost a unit
					UnitDestroyed(cte->unit, -1);
				} else if (cte->newteam == cb->GetMyTeam()) {
					// we have a new unit
					UnitCreated(cte->unit, -1);
					if (!cb->UnitBeingBuilt(cte->unit)) {
						UnitFinished(cte->unit);
						UnitIdle(cte->unit);
					}
				}
				break;
			}
	}
	return 0;
}

void AAI::Log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const int bytes = vfprintf(file, format, args);
	if (bytes<0) { //write to stderr if write to file failed
		vfprintf(stderr, format, args);
	}
	va_end(args);
}

void AAI::LogConsole(const char* format, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, 1024, format, args);
	va_end(args);

	cb->SendTextMsg(buf, 0);
	Log("%s\n", &buf);
}

