// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "System/FastMath.h"
#include "AAIBuildTable.h"
#include "AAI.h"

// all the static vars
const UnitDef** AAIBuildTable::unitList = 0;
list<int>* AAIBuildTable::units_of_category[MOBILE_CONSTRUCTOR+1];
int AAIBuildTable::aai_instances = 0;
char AAIBuildTable::buildtable_filename[500];
float* AAIBuildTable::avg_cost[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::avg_buildtime[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::avg_value[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::max_cost[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::max_buildtime[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::max_value[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::min_cost[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::min_buildtime[MOBILE_CONSTRUCTOR+1];
float* AAIBuildTable::min_value[MOBILE_CONSTRUCTOR+1];
float**	AAIBuildTable::avg_speed;
float**	AAIBuildTable::min_speed;
float**	AAIBuildTable::max_speed;
float**	AAIBuildTable::group_speed;
vector< vector< vector<float> > > AAIBuildTable::attacked_by_category_learned;
vector< vector<float> > AAIBuildTable::attacked_by_category_current;
vector<UnitTypeStatic> AAIBuildTable::units_static;
vector<vector<double> >AAIBuildTable::def_power;
vector<double>AAIBuildTable::max_pplant_eff;
/*float* AAIBuildTable::max_builder_buildtime;
float* AAIBuildTable::max_builder_cost;
float* AAIBuildTable::max_builder_buildspeed;*/
vector< vector< vector<float> > > AAIBuildTable::avg_eff;
vector< vector< vector<float> > > AAIBuildTable::max_eff;
vector< vector< vector<float> > > AAIBuildTable::min_eff;
vector< vector< vector<float> > > AAIBuildTable::total_eff;
vector< vector<float> > AAIBuildTable::fixed_eff;

AAIBuildTable::AAIBuildTable(IAICallback *cb, AAI* ai)
{
	this->cb = cb;
	this->ai = ai;

	initialized = false;
	numOfUnits = 0;

	numOfSides = cfg->SIDES;
	startUnits.resize(numOfSides);
	sideNames.resize(numOfSides+1);
	sideNames[0] = "Neutral";

	const UnitDef *temp;

	for(int i = 0; i < numOfSides; ++i)
	{
		temp = cb->GetUnitDef(cfg->START_UNITS[i]);

		if(temp)
			startUnits[i] = temp->id;
		else
		{
			startUnits[i] = -1;

			static const size_t c_maxSize = 120;
			char c[c_maxSize];
			SNPRINTF(c, c_maxSize, "Error: starting unit %s not found\n",
					cfg->START_UNITS[i]);
			cb->SendTextMsg(c, 0);
			fprintf(ai->file, "%s", c);
		}

		sideNames[i+1].assign(cfg->SIDE_NAMES[i]);
	}

	// add assault categories
	assault_categories.push_back(GROUND_ASSAULT);
	assault_categories.push_back(AIR_ASSAULT);
	assault_categories.push_back(HOVER_ASSAULT);
	assault_categories.push_back(SEA_ASSAULT);
	assault_categories.push_back(SUBMARINE_ASSAULT);

	// one more instance
	++aai_instances;

	ai->aai_instance = aai_instances;

	// only set up static things if first aai intsance is iniatialized
	if(aai_instances == 1)
	{
		for(int i = 0; i <= MOBILE_CONSTRUCTOR; ++i)
		{
			// set up the unit lists
			units_of_category[i] = new list<int>[numOfSides];

			// statistical values (mod sepcific)
			avg_cost[i] = new float[numOfSides];
			avg_buildtime[i] = new float[numOfSides];
			avg_value[i] = new float[numOfSides];
			max_cost[i] = new float[numOfSides];
			max_buildtime[i] = new float[numOfSides];
			max_value[i] = new float[numOfSides];
			min_cost[i] = new float[numOfSides];
			min_buildtime[i] = new float[numOfSides];
			min_value[i] = new float[numOfSides];

			for(int s = 0; s < numOfSides; ++s)
			{
				avg_cost[i][s] = -1;
				avg_buildtime[i][s] = -1;
				avg_value[i][s] = -1;
				max_cost[i][s] = -1;
				max_buildtime[i][s] = -1;
				max_value[i][s] = -1;
				min_cost[i][s] = -1;
				min_buildtime[i][s] = -1;
				min_value[i][s] = -1;
			}
		}

		// statistical values for builders (map specific)
		/*max_builder_buildtime = new float[numOfSides];
		max_builder_cost = new float[numOfSides];
		max_builder_buildspeed = new float[numOfSides];

		for(int s = 0; s < numOfSides; ++s)
		{
			max_builder_buildtime[s] = -1;
			max_builder_cost[s] = -1;
			max_builder_buildspeed[s] = -1;
		}*/

		// set up speed and attacked_by table
		avg_speed = new float*[combat_categories];
		max_speed = new float*[combat_categories];
		min_speed = new float*[combat_categories];
		group_speed = new float*[combat_categories];

		attacked_by_category_current.resize(cfg->GAME_PERIODS, vector<float>(combat_categories, 0));
		attacked_by_category_learned.resize(3,  vector< vector<float> >(cfg->GAME_PERIODS, vector<float>(combat_categories, 0)));

		for(int i = 0; i < combat_categories; ++i)
		{
			avg_speed[i] = new float[numOfSides];
			max_speed[i] = new float[numOfSides];
			min_speed[i] = new float[numOfSides];
			group_speed[i] = new float[numOfSides];
		}

		// init eff stats
		avg_eff.resize(numOfSides, vector< vector<float> >(combat_categories, vector<float>(combat_categories, 1.0f)));
		max_eff.resize(numOfSides, vector< vector<float> >(combat_categories, vector<float>(combat_categories, 1.0f)));
		min_eff.resize(numOfSides, vector< vector<float> >(combat_categories, vector<float>(combat_categories, 1.0f)));
		total_eff.resize(numOfSides, vector< vector<float> >(combat_categories, vector<float>(combat_categories, 1.0f)));
	}
}

AAIBuildTable::~AAIBuildTable(void)
{
	// one instance less
	--aai_instances;

	// delete common data only if last aai instace has gone
	if(aai_instances == 0)
	{

		delete [] unitList;

		for(int i = 0; i <= MOBILE_CONSTRUCTOR; ++i)
		{
			delete [] units_of_category[i];

			delete [] avg_cost[i];
			delete [] avg_buildtime[i];
			delete [] avg_value[i];
			delete [] max_cost[i];
			delete [] max_buildtime[i];
			delete [] max_value[i];
			delete [] min_cost[i];
			delete [] min_buildtime[i];
			delete [] min_value[i];
		}

		/*delete [] max_builder_buildtime;
		delete [] max_builder_cost;
		delete [] max_builder_buildspeed;*/

		for(int i = 0; i < combat_categories; ++i)
		{
			delete [] avg_speed[i];
			delete [] max_speed[i];
			delete [] min_speed[i];
			delete [] group_speed[i];
		}

		delete [] avg_speed;
		delete [] max_speed;
		delete [] min_speed;
		delete [] group_speed;

		attacked_by_category_learned.clear();
		attacked_by_category_current.clear();

		avg_eff.clear();
		max_eff.clear();
		min_eff.clear();
		total_eff.clear();
	}
}

void AAIBuildTable::Init()
{
	float max_cost = 0, min_cost = 1000000, eff;

	// initialize random numbers generator
	srand ( time(NULL) );

	// get number of units and alloc memory for unit list
	numOfUnits = cb->GetNumUnitDefs();

	// one more than needed because 0 is dummy object (so UnitDef->id can be used to adress that unit in the array)
	units_dynamic.resize(numOfUnits+1);

	for(int i = 0; i <= numOfUnits; ++i)
	{
		units_dynamic[i].active = 0;
		units_dynamic[i].requested = 0;
		units_dynamic[i].constructorsAvailable = 0;
		units_dynamic[i].constructorsRequested = 0;
	}

	// get unit defs from game
	if(!unitList)
	{
		unitList = new const UnitDef*[numOfUnits];
		cb->GetUnitDefList(unitList);
	}

	// Try to load buildtable, if not possible create new one
	if(!LoadBuildTable())
	{
		// one more than needed because 0 is dummy object (so UnitDef->id can be used to adress that unit in the array)
		units_static.resize(numOfUnits+1);
		fixed_eff.resize(numOfUnits+1, vector<float>(combat_categories));

		// temporary list to sort air unit in air only mods
		list<int> *temp_list;
		temp_list = new list<int>[numOfSides];

		units_static[0].def_id = 0;
		units_static[0].side = 0;

		// add units to buildtable
		for(int i = 1; i <= numOfUnits; ++i)
		{
			// get id
			units_static[i].def_id = unitList[i-1]->id;
			units_static[i].cost = (unitList[i-1]->metalCost + (unitList[i-1]->energyCost / 75.0f)) / 10.0f;

			if(units_static[i].cost > max_cost)
				max_cost = units_static[i].cost;

			if(units_static[i].cost < min_cost)
				min_cost = units_static[i].cost;

			units_static[i].builder_cost = 0; // will be added later when calculating the buildtree

			// side has not been assigned - will be done later
			units_static[i].side = 0;
			units_static[i].range = 0;

			units_static[i].category = UNKNOWN;

			units_static[i].unit_type = 0;

			// get build options
			for(map<int, string>::const_iterator j = unitList[i-1]->buildOptions.begin(); j != unitList[i-1]->buildOptions.end(); ++j)
				units_static[i].canBuildList.push_back(cb->GetUnitDef(j->second.c_str())->id);
		}

		// now set the sides and create buildtree
		for(int s = 0; s < numOfSides; s++)
		{
			// set side of the start unit (eg commander) and continue recursively
			units_static[startUnits[s]].side = s+1;
			CalcBuildTree(startUnits[s]);
		}

		// now calculate efficiency of combat units and get max range
		for(int i = 1; i <= numOfUnits; i++)
		{
			// effiency has starting value of 1
			if(!unitList[i-1]->weapons.empty())
			{
				// get range
				units_static[i].range = GetMaxRange(i);

				// get memory for eff
				units_static[i].efficiency.resize(combat_categories);

				eff = 5 + 25 * (units_static[i].cost - min_cost)/(max_cost - min_cost);

				for(int k = 0; k < combat_categories; ++k)
				{
					units_static[i].efficiency[k] = eff;
					fixed_eff[i][k] = eff;
				}
			}
			else
			{
				units_static[i].range = 0;

				// get memory for eff
				units_static[i].efficiency.resize(combat_categories, -1.0f);
			}
		}

		//
		// determine movement type
		//
		for(int i = 1; i <= numOfUnits; i++)
		{
			units_static[i].movement_type = 0;

			if(unitList[i-1]->movedata)
			{
				if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move)
				{
					// check for amphibious units
					if(unitList[i-1]->movedata->depth > 250)
						units_static[i].movement_type |= MOVE_TYPE_AMPHIB;
					else
						units_static[i].movement_type |= MOVE_TYPE_GROUND;
				}
				else if(unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
					units_static[i].movement_type |= MOVE_TYPE_HOVER;
				// ship
				else if(unitList[i-1]->movedata->moveType == MoveData::Ship_Move)
				{
					units_static[i].movement_type |= MOVE_TYPE_SEA;

					if(unitList[i-1]->categoryString.find("UNDERWATER") != string::npos)
						units_static[i].movement_type |= MOVE_TYPE_UNDERWATER;
					else
						units_static[i].movement_type |= MOVE_TYPE_FLOATER;
				}
			}
			// aircraft
			else if(unitList[i-1]->canfly)
				units_static[i].movement_type |= MOVE_TYPE_AIR;
			// stationary
			else
			{
				units_static[i].movement_type |= MOVE_TYPE_STATIC;

				if(unitList[i-1]->minWaterDepth <= 0)
				{
					units_static[i].movement_type |= MOVE_TYPE_STATIC_LAND;
				}
				else
				{
					units_static[i].movement_type |= MOVE_TYPE_STATIC_WATER;

					if(unitList[i-1]->floater)
						units_static[i].movement_type |= MOVE_TYPE_FLOATER;
					else
						units_static[i].movement_type |= MOVE_TYPE_UNDERWATER;
				}
			}
		}

		//
		// put units into the different categories
		//
		for(int i = 1; i <= numOfUnits; ++i)
		{
			if(!units_static[i].side || !AllowedToBuild(i))
			{
			}
			// get scouts
			else if(IsScout(i))
			{
				units_of_category[SCOUT][units_static[i].side-1].push_back(unitList[i-1]->id);
				units_static[i].category = SCOUT;
			}
			// get mobile transport
			else if(IsTransporter(i))
			{
				units_of_category[MOBILE_TRANSPORT][units_static[i].side-1].push_back(unitList[i-1]->id);
				units_static[i].category = MOBILE_TRANSPORT;
			}
			// check if builder or factory
			else if(unitList[i-1]->buildOptions.size() > 0 && !IsAttacker(i))
			{
				// stationary constructors
				if(units_static[i].movement_type & MOVE_TYPE_STATIC)
				{
					// ground factory or sea factory
					units_of_category[STATIONARY_CONSTRUCTOR][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = STATIONARY_CONSTRUCTOR;
				}
				// mobile constructors
				else
				{
					units_of_category[MOBILE_CONSTRUCTOR][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = MOBILE_CONSTRUCTOR;
				}
			}
			// no builder or factory
			// check if other building
			else if(units_static[i].movement_type & MOVE_TYPE_STATIC)
			{
				// check if extractor
				if(unitList[i-1]->extractsMetal)
				{
					units_of_category[EXTRACTOR][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = EXTRACTOR;
				}
				// check if repair pad
				else if(unitList[i-1]->isAirBase)
				{
					units_of_category[AIR_BASE][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = AIR_BASE;
				}
				// check if powerplant
				else if(unitList[i-1]->energyMake > cfg->MIN_ENERGY || unitList[i-1]->tidalGenerator || unitList[i-1]->energyUpkeep < -cfg->MIN_ENERGY)
				{
					if(!unitList[i-1]->isAirBase && unitList[i-1]->radarRadius == 0 && unitList[i-1]->sonarRadius == 0)
					{
						units_of_category[POWER_PLANT][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = POWER_PLANT;
					}
				}
				// check if defence building
				else if(!unitList[i-1]->weapons.empty() && GetMaxDamage(i) > 1)
				{
					// filter out nuke silos, antinukes and stuff like that
					if(IsMissileLauncher(i))
					{
						units_of_category[STATIONARY_LAUNCHER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = STATIONARY_LAUNCHER;
					}
					else if(IsDeflectionShieldEmitter(i))
					{
						units_of_category[DEFLECTION_SHIELD][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = DEFLECTION_SHIELD;
					}
					else
					{
						if(GetMaxRange(unitList[i-1]->id) < cfg->STATIONARY_ARTY_RANGE)
						{
							units_of_category[STATIONARY_DEF][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = STATIONARY_DEF;
						}
						else
						{
							units_of_category[STATIONARY_ARTY][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = STATIONARY_ARTY;
						}
					}

				}
				// check if radar or sonar
				else if(unitList[i-1]->radarRadius > 0 || unitList[i-1]->sonarRadius > 0)
				{
					units_of_category[STATIONARY_RECON][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = STATIONARY_RECON;
				}
				// check if jammer
				else if(unitList[i-1]->jammerRadius > 0 || unitList[i-1]->sonarJamRadius > 0)
				{
					units_of_category[STATIONARY_JAMMER][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = STATIONARY_JAMMER;
				}
				// check storage or converter
				else if( unitList[i-1]->energyStorage > cfg->MIN_ENERGY_STORAGE && !unitList[i-1]->energyMake)
				{
					units_of_category[STORAGE][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = STORAGE;
				}
				else if(unitList[i-1]->metalStorage > cfg->MIN_METAL_STORAGE && !unitList[i-1]->extractsMetal)
				{
					units_of_category[STORAGE][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = STORAGE;
				}
				else if(unitList[i-1]->makesMetal > 0)
				{
					units_of_category[METAL_MAKER][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = METAL_MAKER;
				}
			}
			// units that are not builders
			else if(unitList[i-1]->movedata)
			{
				// ground units
				if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move || unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
				{
					// units with weapons
					if((!unitList[i-1]->weapons.empty() && GetMaxDamage(i) > 1) || IsAttacker(i))
					{
						if(IsMissileLauncher(i))
						{
							units_of_category[MOBILE_LAUNCHER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_LAUNCHER;
						}
						else if(GetMaxDamage(unitList[i-1]->id) > 1)
						{
							// switch between arty and assault
							if(IsArty(i))
							{
								if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move)
								{
									units_of_category[GROUND_ARTY][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = GROUND_ARTY;
								}
								else
								{
									units_of_category[HOVER_ARTY][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = HOVER_ARTY;
								}
							}
							else if(unitList[i-1]->speed > 0)
							{
								if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move)
								{
									units_of_category[GROUND_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = GROUND_ASSAULT;
								}
								else
								{
									units_of_category[HOVER_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = HOVER_ASSAULT;
								}
							}
						}

						else if(unitList[i-1]->sonarJamRadius > 0 || unitList[i-1]->jammerRadius > 0)
						{
							units_of_category[MOBILE_JAMMER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_JAMMER;
						}
					}
					// units without weapons
					else
					{
						if(unitList[i-1]->sonarJamRadius > 0 || unitList[i-1]->jammerRadius > 0)
						{
							units_of_category[MOBILE_JAMMER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_JAMMER;
						}
					}
				}
				else if(unitList[i-1]->movedata->moveType == MoveData::Ship_Move)
				{
					// ship
					if(!unitList[i-1]->weapons.empty())
					{
						if(IsMissileLauncher(i))
						{
							units_of_category[MOBILE_LAUNCHER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_LAUNCHER;
						}
						else if(GetMaxDamage(unitList[i-1]->id) > 1 || IsAttacker(i))
						{
							if(unitList[i-1]->categoryString.find("UNDERWATER") != string::npos)
							{
								units_of_category[SUBMARINE_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
								units_static[i].category = SUBMARINE_ASSAULT;
							}
							else
							{
								// switch between arty and assault
								if(IsArty(i))
								{	units_of_category[SEA_ARTY][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = SEA_ARTY;
								}
								else
								{
									units_of_category[SEA_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
									units_static[i].category = SEA_ASSAULT;
								}
							}
						}
						else if(unitList[i-1]->sonarJamRadius > 0 || unitList[i-1]->jammerRadius > 0)
						{
							units_of_category[MOBILE_JAMMER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_JAMMER;
						}
					}
					else
					{
						if(unitList[i-1]->sonarJamRadius > 0 || unitList[i-1]->jammerRadius > 0)
						{
							units_of_category[MOBILE_JAMMER][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = MOBILE_JAMMER;
						}
					}
				}
			}
			// aircraft
			else if(unitList[i-1]->canfly)
			{
				// units with weapons
				if((!unitList[i-1]->weapons.empty() && GetMaxDamage(unitList[i-1]->id) > 1) || IsAttacker(i))
				{
					if(unitList[i-1]->weapons.begin()->def->stockpile)
					{
						units_of_category[MOBILE_LAUNCHER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = MOBILE_LAUNCHER;
					}
					else
					{
						// to apply different sorting rules later
						if(cfg->AIR_ONLY_MOD)
							temp_list[units_static[i].side-1].push_back(unitList[i-1]->id);

						units_of_category[AIR_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = AIR_ASSAULT;
					}
				}
			}

			// get commander
			if(IsStartingUnit(unitList[i-1]->id))
			{
				units_static[i].category = COMMANDER;
				units_of_category[COMMANDER][units_static[i].side-1].push_back(unitList[i-1]->id);
			}
		}

		//
		// determine unit type
		//
		for(int i = 1; i <= numOfUnits; i++)
		{
			// check for factories and builders
			if(units_static[i].canBuildList.size() > 0)
			{
				for(list<int>::iterator unit = units_static[i].canBuildList.begin(); unit != units_static[i].canBuildList.end(); ++unit)
				{
					// filter out neutral and unknown units
					if(units_static[*unit].side > 0 && units_static[*unit].category != UNKNOWN)
					{
						if(units_static[*unit].movement_type & MOVE_TYPE_STATIC)
							units_static[i].unit_type |= UNIT_TYPE_BUILDER;
						else
							units_static[i].unit_type |= UNIT_TYPE_FACTORY;
					}
				}

				if(!(units_static[i].movement_type & MOVE_TYPE_STATIC) && unitList[i-1]->canAssist)
					units_static[i].unit_type |= UNIT_TYPE_ASSISTER;
			}

			if(unitList[i-1]->canResurrect)
				units_static[i].unit_type |= UNIT_TYPE_RESURRECTOR;

			if(IsStartingUnit(unitList[i-1]->id))
				units_static[i].unit_type |= UNIT_TYPE_COMMANDER;
		}


		if(!cfg->AIR_ONLY_MOD)
		{
			UnitCategory cat;
			float eff;

			for(int i = 0; i <= numOfUnits; ++i)
			{
				cat = units_static[i].category;
				eff = 1.5 + 7 * (units_static[i].cost - min_cost)/(max_cost - min_cost);

				if(cat == AIR_ASSAULT)
				{
					for(int k = 0; k < combat_categories; ++k)
						units_static[i].efficiency[k] = eff;
				}
				else if(cat == GROUND_ASSAULT ||  cat == HOVER_ASSAULT || cat == SEA_ASSAULT || cat == SUBMARINE_ASSAULT || cat == GROUND_ARTY || cat == SEA_ARTY || cat == HOVER_ARTY || cat == STATIONARY_DEF)
				{
					units_static[i].efficiency[1] = eff;
				}
			}
		}

		// precache stats
		PrecacheStats();

		// apply specific sort rules
		if(cfg->AIR_ONLY_MOD)
		{
			float total_cost, my_cost;

			for(int s = 0; s < numOfSides; ++s)
			{
				total_cost = this->max_cost[AIR_ASSAULT][s] - this->min_cost[AIR_ASSAULT][s];

				if(total_cost <= 0)
					break;

				// clear list
				units_of_category[AIR_ASSAULT][s].clear();

				for(list<int>::iterator unit = temp_list[s].begin(); unit != temp_list[s].end(); ++unit)
				{
					my_cost = (units_static[*unit].cost - this->min_cost[AIR_ASSAULT][s]) / total_cost;

					if(my_cost < cfg->MAX_COST_LIGHT_ASSAULT)
					{
						units_of_category[GROUND_ASSAULT][s].push_back(*unit);
						units_static[*unit].category = GROUND_ASSAULT;
					}
					else if(my_cost < cfg->MAX_COST_MEDIUM_ASSAULT)
					{
						units_of_category[AIR_ASSAULT][s].push_back(*unit);
						units_static[*unit].category = AIR_ASSAULT;
					}
					else if(my_cost < cfg->MAX_COST_HEAVY_ASSAULT)
					{
						units_of_category[HOVER_ASSAULT][s].push_back(*unit);
						units_static[*unit].category = HOVER_ASSAULT;
					}
					else
					{
						units_of_category[SEA_ASSAULT][s].push_back(*unit);
						units_static[*unit].category = SEA_ASSAULT;
					}
				}
			}

			// recalculate stats
			PrecacheStats();
		}

		// save to cache file
		SaveBuildTable(0, LAND_MAP);

		cb->SendTextMsg("New BuildTable has been created", 0);
	}



	// only once
	if(aai_instances == 1)
	{
		// apply possible cost multipliers
		if(cfg->cost_multipliers.size() > 0)
		{
			for(size_t i = 0; i < cfg->cost_multipliers.size(); ++i)
				units_static[cfg->cost_multipliers[i].id].cost *= cfg->cost_multipliers[i].multiplier;

			// recalculate costs
			PrecacheCosts();
		}

		UpdateMinMaxAvgEfficiency();

		float temp;

		def_power.resize(numOfSides);
		max_pplant_eff.resize(numOfSides);

		for(int s = 0; s < numOfSides; ++s)
		{
			def_power[s].resize(units_of_category[STATIONARY_DEF][s].size());

			// power plant max eff
			max_pplant_eff[s] = 0;

			for(list<int>::iterator pplant = units_of_category[POWER_PLANT][s].begin(); pplant != units_of_category[POWER_PLANT][s].end(); ++pplant)
			{
				temp = units_static[*pplant].efficiency[1];

				// eff. of tidal generators have not been calculated yet (depend on map)
				if(temp <= 0)
				{
					temp = cb->GetTidalStrength() / units_static[*pplant].cost;

					units_static[*pplant].efficiency[0] = cb->GetTidalStrength();
					units_static[*pplant].efficiency[1] = temp;
				}

				if(temp > max_pplant_eff[s])
					max_pplant_eff[s] = temp;
			}
		}

		DebugPrint();
	}

	// buildtable is initialized
	initialized = true;
}

void AAIBuildTable::InitCombatEffCache(int side)
{
	side--;

	size_t max_size = 0;

	UnitCategory category;

	for(int cat = 0; cat < combat_categories; ++cat)
	{
		category = GetAssaultCategoryOfID(cat);

		if(units_of_category[(int)category][side].size() > max_size)
			max_size = units_of_category[(int)category][side].size();
	}

	combat_eff.resize(max_size, 0);
}

void AAIBuildTable::PrecacheStats()
{
	for(int s = 0; s < numOfSides; s++)
	{
		// precache efficiency of power plants
		for(list<int>::iterator i = units_of_category[POWER_PLANT][s].begin(); i != units_of_category[POWER_PLANT][s].end(); i++)
		{
			if(unitList[*i-1]->tidalGenerator)
				units_static[*i].efficiency[0] = 0;
			else if(unitList[*i-1]->energyMake >= cfg->MIN_ENERGY)
				units_static[*i].efficiency[0] = unitList[*i-1]->energyMake;
			else if(unitList[*i-1]->energyUpkeep <= -cfg->MIN_ENERGY)
				units_static[*i].efficiency[0] = - unitList[*i-1]->energyUpkeep;

			units_static[*i].efficiency[1] = units_static[*i].efficiency[0] / units_static[*i].cost;
		}

		// precache efficiency of extractors
		for(list<int>::iterator i = units_of_category[EXTRACTOR][s].begin(); i != units_of_category[EXTRACTOR][s].end(); i++)
			units_static[*i].efficiency[0] = unitList[*i-1]->extractsMetal;

		// precache efficiency of metalmakers
		for(list<int>::iterator i = units_of_category[METAL_MAKER][s].begin(); i != units_of_category[METAL_MAKER][s].end(); ++i)
			units_static[*i].efficiency[0] = unitList[*i-1]->makesMetal/(unitList[*i-1]->energyUpkeep+1);


		// precache average metal and energy consumption of factories
		float average_metal, average_energy;
		for(list<int>::iterator i = units_of_category[STATIONARY_CONSTRUCTOR][s].begin(); i != units_of_category[STATIONARY_CONSTRUCTOR][s].end(); ++i)
		{
			average_metal = average_energy = 0;

			for(list<int>::iterator unit = units_static[*i].canBuildList.begin(); unit != units_static[*i].canBuildList.end(); unit++)
			{
				average_metal += ( unitList[*unit-1]->metalCost * unitList[*i-1]->buildSpeed ) / unitList[*unit-1]->buildTime;
				average_energy += ( unitList[*unit-1]->energyCost * unitList[*i-1]->buildSpeed ) / unitList[*unit-1]->buildTime;
			}

			units_static[*i].efficiency[0] = average_metal / units_static[*i].canBuildList.size();
			units_static[*i].efficiency[1] = average_energy / units_static[*i].canBuildList.size();
		}

		// precache range of arty
		for(list<int>::iterator i = units_of_category[STATIONARY_ARTY][s].begin(); i != units_of_category[STATIONARY_ARTY][s].end(); ++i)
		{
			units_static[*i].efficiency[1] = GetMaxRange(*i);
			units_static[*i].efficiency[0] = 1 + units_static[*i].cost/100.0;
		}

		// precache costs and buildtime
		float buildtime;

		for(int i = 1; i <= MOBILE_CONSTRUCTOR; ++i)
		{
			// precache costs
			avg_cost[i][s] = 0;
			this->min_cost[i][s] = 10000;
			this->max_cost[i][s] = 0;

			for(list<int>::iterator unit = units_of_category[i][s].begin(); unit != units_of_category[i][s].end(); ++unit)
			{
				avg_cost[i][s] += units_static[*unit].cost;

				if(units_static[*unit].cost > this->max_cost[i][s])
					this->max_cost[i][s] = units_static[*unit].cost;

				if(units_static[*unit].cost < this->min_cost[i][s] )
					this->min_cost[i][s] = units_static[*unit].cost;
			}

			if(units_of_category[i][s].size() > 0)
				avg_cost[i][s] /= units_of_category[i][s].size();
			else
			{
				avg_cost[i][s] = -1;
				this->min_cost[i][s] = -1;
				this->max_cost[i][s] = -1;
			}

			// precache buildtime
			min_buildtime[i][s] = 10000;
			avg_buildtime[i][s] = 0;
			max_buildtime[i][s] = 0;

			for(list<int>::iterator unit = units_of_category[i][s].begin(); unit != units_of_category[i][s].end(); ++unit)
			{
				buildtime = unitList[*unit-1]->buildTime;

				avg_buildtime[i][s] += buildtime;

				if(buildtime > max_buildtime[i][s])
					max_buildtime[i][s] = buildtime;

				if(buildtime < min_buildtime[i][s])
					min_buildtime[i][s] = buildtime;
			}

			if(units_of_category[i][s].size() > 0)
				avg_buildtime[i][s] /= units_of_category[i][s].size();
			else
			{
				avg_buildtime[i][s] = -1;
				min_buildtime[i][s] = -1;
				max_buildtime[i][s] = -1;
			}
		}

		// precache radar ranges
		min_value[STATIONARY_RECON][s] = 10000;
		avg_value[STATIONARY_RECON][s] = 0;
		max_value[STATIONARY_RECON][s] = 0;

		for(list<int>::iterator unit = units_of_category[STATIONARY_RECON][s].begin(); unit != units_of_category[STATIONARY_RECON][s].end(); ++unit)
		{
			avg_value[STATIONARY_RECON][s] += unitList[*unit-1]->radarRadius;

			if(unitList[*unit-1]->radarRadius > max_value[STATIONARY_RECON][s])
				max_value[STATIONARY_RECON][s] = unitList[*unit-1]->radarRadius;

			if(unitList[*unit-1]->radarRadius < min_value[STATIONARY_RECON][s])
				min_value[STATIONARY_RECON][s] = unitList[*unit-1]->radarRadius;
		}

		if(units_of_category[STATIONARY_RECON][s].size() > 0)
			avg_value[STATIONARY_RECON][s] /= units_of_category[STATIONARY_RECON][s].size();
		else
		{
			min_value[STATIONARY_RECON][s] = -1;
			avg_value[STATIONARY_RECON][s] = -1;
			max_value[STATIONARY_RECON][s] = -1;
		}

		// precache jammer ranges
		min_value[STATIONARY_JAMMER][s] = 10000;
		avg_value[STATIONARY_JAMMER][s] = 0;
		max_value[STATIONARY_JAMMER][s] = 0;

		for(list<int>::iterator unit = units_of_category[STATIONARY_JAMMER][s].begin(); unit != units_of_category[STATIONARY_JAMMER][s].end(); ++unit)
		{
			avg_value[STATIONARY_JAMMER][s] += unitList[*unit-1]->jammerRadius;

			if(unitList[*unit-1]->jammerRadius > max_value[STATIONARY_JAMMER][s])
				max_value[STATIONARY_JAMMER][s] = unitList[*unit-1]->jammerRadius;

			if(unitList[*unit-1]->jammerRadius < min_value[STATIONARY_JAMMER][s])
				min_value[STATIONARY_JAMMER][s] = unitList[*unit-1]->jammerRadius;
		}

		if(units_of_category[STATIONARY_JAMMER][s].size() > 0)
			avg_value[STATIONARY_JAMMER][s] /= units_of_category[STATIONARY_JAMMER][s].size();
		else
		{
			min_value[STATIONARY_JAMMER][s] = -1;
			avg_value[STATIONARY_JAMMER][s] = -1;
			max_value[STATIONARY_JAMMER][s] = -1;
		}

		// precache usage of jammers
		for(list<int>::iterator i = units_of_category[STATIONARY_JAMMER][s].begin(); i != units_of_category[STATIONARY_JAMMER][s].end(); i++)
		{
			if(unitList[*i-1]->energyUpkeep - unitList[*i-1]->energyMake > 0)
				units_static[*i].efficiency[0] = unitList[*i-1]->energyUpkeep - unitList[*i-1]->energyMake;
		}

		// precache usage of radar
		for(list<int>::iterator i = units_of_category[STATIONARY_RECON][s].begin(); i != units_of_category[STATIONARY_RECON][s].end(); i++)
		{
			if(unitList[*i-1]->energyUpkeep - unitList[*i-1]->energyMake > 0)
				units_static[*i].efficiency[0] = unitList[*i-1]->energyUpkeep - unitList[*i-1]->energyMake;
		}

		// precache extractor efficiency
		min_value[EXTRACTOR][s] = 10000;
		avg_value[EXTRACTOR][s] = 0;
		max_value[EXTRACTOR][s] = 0;

		for(list<int>::iterator unit = units_of_category[EXTRACTOR][s].begin(); unit != units_of_category[EXTRACTOR][s].end(); ++unit)
		{
			avg_value[EXTRACTOR][s] += unitList[*unit-1]->extractsMetal;

			if(unitList[*unit-1]->extractsMetal > max_value[EXTRACTOR][s])
				max_value[EXTRACTOR][s] = unitList[*unit-1]->extractsMetal;

			if(unitList[*unit-1]->extractsMetal < min_value[EXTRACTOR][s])
				min_value[EXTRACTOR][s] = unitList[*unit-1]->extractsMetal;
		}

		if(units_of_category[EXTRACTOR][s].size() > 0)
			avg_value[EXTRACTOR][s] /= units_of_category[EXTRACTOR][s].size();
		else
		{
			min_value[EXTRACTOR][s] = -1;
			avg_value[EXTRACTOR][s] = -1;
			max_value[EXTRACTOR][s] = -1;
		}

		// precache power plant energy production
		min_value[POWER_PLANT][s] = 10000;
		avg_value[POWER_PLANT][s] = 0;
		max_value[POWER_PLANT][s] = 0;

		for(list<int>::iterator unit = units_of_category[POWER_PLANT][s].begin(); unit != units_of_category[POWER_PLANT][s].end(); ++unit)
		{
			avg_value[POWER_PLANT][s] += units_static[*unit].efficiency[0];

			if(units_static[*unit].efficiency[0] > max_value[POWER_PLANT][s])
				max_value[POWER_PLANT][s] = units_static[*unit].efficiency[0];

			if(units_static[*unit].efficiency[0] < min_value[POWER_PLANT][s])
				min_value[POWER_PLANT][s] = units_static[*unit].efficiency[0];
		}

		if(units_of_category[POWER_PLANT][s].size() > 0)
			avg_value[POWER_PLANT][s] /= units_of_category[POWER_PLANT][s].size();
		else
		{
			min_value[POWER_PLANT][s] = -1;
			avg_value[POWER_PLANT][s] = -1;
			max_value[POWER_PLANT][s] = -1;
		}

		// precache stationary arty range
		min_value[STATIONARY_ARTY][s] = 100000;
		avg_value[STATIONARY_ARTY][s] = 0;
		max_value[STATIONARY_ARTY][s] = 0;

		for(list<int>::iterator unit = units_of_category[STATIONARY_ARTY][s].begin(); unit != units_of_category[STATIONARY_ARTY][s].end(); ++unit)
		{
			avg_value[STATIONARY_ARTY][s] += units_static[*unit].efficiency[1];

			if(units_static[*unit].efficiency[1] > max_value[STATIONARY_ARTY][s])
				max_value[STATIONARY_ARTY][s] = units_static[*unit].efficiency[1];

			if(units_static[*unit].efficiency[1] < min_value[STATIONARY_ARTY][s])
				min_value[STATIONARY_ARTY][s] = units_static[*unit].efficiency[1];
		}

		if(units_of_category[STATIONARY_ARTY][s].size() > 0)
			avg_value[STATIONARY_ARTY][s] /= units_of_category[STATIONARY_ARTY][s].size();
		else
		{
			min_value[STATIONARY_ARTY][s] = -1;
			avg_value[STATIONARY_ARTY][s] = -1;
			max_value[STATIONARY_ARTY][s] = -1;
		}

		// precache scout los
		min_value[SCOUT][s] = 100000;
		avg_value[SCOUT][s] = 0;
		max_value[SCOUT][s] = 0;

		for(list<int>::iterator unit = units_of_category[SCOUT][s].begin(); unit != units_of_category[SCOUT][s].end(); ++unit)
		{
			avg_value[SCOUT][s] += unitList[*unit-1]->losRadius;

			if(unitList[*unit-1]->losRadius > max_value[SCOUT][s])
				max_value[SCOUT][s] = unitList[*unit-1]->losRadius;

			if(unitList[*unit-1]->losRadius < min_value[SCOUT][s])
				min_value[SCOUT][s] = unitList[*unit-1]->losRadius;
		}

		if(units_of_category[SCOUT][s].size() > 0)
			avg_value[SCOUT][s] /= units_of_category[SCOUT][s].size();
		else
		{
			min_value[SCOUT][s] = -1;
			avg_value[SCOUT][s] = -1;
			max_value[SCOUT][s] = -1;
		}

		// precache stationary defences weapon range
		min_value[STATIONARY_DEF][s] = 100000;
		avg_value[STATIONARY_DEF][s] = 0;
		max_value[STATIONARY_DEF][s] = 0;

		float range;

		if(units_of_category[STATIONARY_DEF][s].size() > 0)
		{
			for(list<int>::iterator unit = units_of_category[STATIONARY_DEF][s].begin(); unit != units_of_category[STATIONARY_DEF][s].end(); ++unit)
			{
				range = units_static[*unit].range;

				avg_value[STATIONARY_DEF][s] += range;

				if(range > max_value[STATIONARY_DEF][s])
					max_value[STATIONARY_DEF][s] = range;

				if(range < min_value[STATIONARY_DEF][s])
					min_value[STATIONARY_DEF][s] = range;
			}

			avg_value[STATIONARY_DEF][s] /= (float)units_of_category[STATIONARY_DEF][s].size();
		}
		else
		{
			min_value[STATIONARY_DEF][s] = -1;
			avg_value[STATIONARY_DEF][s] = -1;
			max_value[STATIONARY_DEF][s] = -1;
		}

		// precache builders' buildspeed
		float buildspeed;

		if(units_of_category[MOBILE_CONSTRUCTOR][s].size() > 0)
		{
			min_value[MOBILE_CONSTRUCTOR][s] = 100000;
			avg_value[MOBILE_CONSTRUCTOR][s] = 0;
			max_value[MOBILE_CONSTRUCTOR][s] = 0;

			for(list<int>::iterator unit = units_of_category[MOBILE_CONSTRUCTOR][s].begin(); unit != units_of_category[MOBILE_CONSTRUCTOR][s].end(); ++unit)
			{
				buildspeed = unitList[*unit-1]->buildSpeed;

				avg_value[MOBILE_CONSTRUCTOR][s] += buildspeed;

				if(buildspeed > max_value[MOBILE_CONSTRUCTOR][s])
					max_value[MOBILE_CONSTRUCTOR][s] = buildspeed;

				if(buildspeed < min_value[MOBILE_CONSTRUCTOR][s])
					min_value[MOBILE_CONSTRUCTOR][s] = buildspeed;
			}

			avg_value[MOBILE_CONSTRUCTOR][s] /= (float)units_of_category[MOBILE_CONSTRUCTOR][s].size();
		}
		else
		{
			min_value[MOBILE_CONSTRUCTOR][s] = -1;
			avg_value[MOBILE_CONSTRUCTOR][s] = -1;
			max_value[MOBILE_CONSTRUCTOR][s] = -1;
		}


		// precache unit speed and weapons range
		int cat;

		for(list<UnitCategory>::iterator category = assault_categories.begin(); category != assault_categories.end(); ++category)
		{
			// precache range
			min_value[*category][s] = 10000;
			avg_value[*category][s] = 0;
			max_value[*category][s] = 0;

			if(units_of_category[*category][s].size() > 0)
			{
				for(list<int>::iterator unit = units_of_category[*category][s].begin(); unit != units_of_category[*category][s].end(); ++unit)
				{
					range = GetMaxRange(*unit);

					avg_value[*category][s] += range;

					if(range > max_value[*category][s])
						max_value[*category][s] = range;

					if(range < min_value[*category][s])
						min_value[*category][s] = range;
				}

				avg_value[*category][s] /= (float)units_of_category[*category][s].size();
			}
			else
			{
				min_value[*category][s] = -1;
				avg_value[*category][s] = -1;
				max_value[*category][s] = -1;
			}

			// precache speed
			cat = GetIDOfAssaultCategory(*category);

			if(cat != -1)
			{
				if(units_of_category[*category][s].size() > 0)
				{
					min_speed[cat][s] = 10000;
					max_speed[cat][s] = 0;
					group_speed[cat][s] = 0;
					avg_speed[cat][s] = 0;

					for(list<int>::iterator unit = units_of_category[*category][s].begin(); unit != units_of_category[*category][s].end(); ++unit)
					{
						avg_speed[cat][s] += unitList[*unit-1]->speed;

						if(unitList[*unit-1]->speed < min_speed[cat][s])
							min_speed[cat][s] = unitList[*unit-1]->speed;

						if(unitList[*unit-1]->speed > max_speed[cat][s])
							max_speed[cat][s] = unitList[*unit-1]->speed;
					}

					avg_speed[cat][s] /= (float)units_of_category[*category][s].size();

					group_speed[cat][s] = (1 + max_speed[cat][s] - min_speed[cat][s]) / ((float)cfg->UNIT_SPEED_SUBGROUPS);
				}
				else
				{
					min_speed[cat][s] = -1;
					max_speed[cat][s] = -1;
					group_speed[cat][s] = -1;
					avg_speed[cat][s] = -1;
				}
			}
		}
	}
}

void AAIBuildTable::PrecacheCosts()
{
	for(int s = 0; s < numOfSides; ++s)
	{
		for(int i = 1; i <= MOBILE_CONSTRUCTOR; ++i)
		{
			// precache costs
			avg_cost[i][s] = 0;
			this->min_cost[i][s] = 10000;
			this->max_cost[i][s] = 0;

			for(list<int>::iterator unit = units_of_category[i][s].begin(); unit != units_of_category[i][s].end(); ++unit)
			{
				avg_cost[i][s] += units_static[*unit].cost;

				if(units_static[*unit].cost > this->max_cost[i][s])
					this->max_cost[i][s] = units_static[*unit].cost;

				if(units_static[*unit].cost < this->min_cost[i][s] )
					this->min_cost[i][s] = units_static[*unit].cost;
			}

			if(units_of_category[i][s].size() > 0)
				avg_cost[i][s] /= units_of_category[i][s].size();
			else
			{
				avg_cost[i][s] = -1;
				this->min_cost[i][s] = -1;
				this->max_cost[i][s] = -1;
			}
		}
	}
}


int AAIBuildTable::GetSide(int unit)
{
	return units_static[cb->GetUnitDef(unit)->id].side;
}

int AAIBuildTable::GetSideByID(int unit_id)
{
	return units_static[unit_id].side;
}

UnitType AAIBuildTable::GetUnitType(int def_id)
{
	if(cfg->AIR_ONLY_MOD)
	{
		return ASSAULT_UNIT;
	}
	else
	{
		if (units_static.empty()) return UNKNOWN_UNIT;
		UnitCategory cat = units_static[def_id].category;
		int side = units_static[def_id].side-1;

		if(cat == GROUND_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[side][0][1]  > 6 * units_static[def_id].efficiency[0] / max_eff[side][0][0] )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == AIR_ASSAULT)
		{
			float vs_building = units_static[def_id].efficiency[5] / max_eff[side][1][5];

			float vs_units = (units_static[def_id].efficiency[0] / max_eff[side][1][0]
							+ units_static[def_id].efficiency[3] / max_eff[side][1][3]) / 2.0f;

			if( units_static[def_id].efficiency[1]  / max_eff[side][1][1] > 2 * (vs_building + vs_units) )
				return ANTI_AIR_UNIT;
			else
			{
				if(vs_building > 4 * vs_units || unitList[def_id-1]->type == string("Bomber"))
					return BOMBER_UNIT;
				else
					return ASSAULT_UNIT;
			}
		}
		else if(cat == HOVER_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[side][2][1] > 6 * units_static[def_id].efficiency[0] / max_eff[side][2][0] )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == SEA_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[side][3][1] > 6 * units_static[def_id].efficiency[3] / max_eff[side][3][3] )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == SUBMARINE_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[side][4][1] > 6 * units_static[def_id].efficiency[3] / max_eff[side][4][3] )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat >= GROUND_ARTY && cat <= HOVER_ARTY)
		{
			return ARTY_UNIT;
		} else //throw "AAIBuildTable::GetUnitType: invalid unit category";
			return UNKNOWN_UNIT;
	}
}

bool AAIBuildTable::MemberOf(int unit_id, list<int> unit_list)
{
	// test all units in list
	for(list<int>::iterator i = unit_list.begin(); i != unit_list.end(); i++)
	{
		if(*i == unit_id)
			return true;
	}

	// unitid not found
	return false;
}

int AAIBuildTable::GetPowerPlant(int side, float cost, float urgency, float power, float current_energy, bool water, bool geo, bool canBuild)
{
	UnitTypeStatic *unit;

	int best_unit = 0;

	float best_ranking = -10000, my_ranking;

	//debug
	//fprintf(ai->file, "Selecting power plant:     power %f    cost %f    urgency %f   energy %f \n", power, cost, urgency, current_energy);

	for(list<int>::iterator pplant = units_of_category[POWER_PLANT][side-1].begin(); pplant != units_of_category[POWER_PLANT][side-1].end(); ++pplant)
	{
		unit = &units_static[*pplant];

		if(canBuild && units_dynamic[*pplant].constructorsAvailable <= 0)
			my_ranking = -10000;
		else if(!geo && unitList[*pplant-1]->needGeo)
			my_ranking = -10000;
		else if( (!water && unitList[*pplant-1]->minWaterDepth <= 0) || (water && unitList[*pplant-1]->minWaterDepth > 0) )
		{
			my_ranking = cost * unit->efficiency[1] / max_pplant_eff[side-1] + power * unit->efficiency[0] / max_value[POWER_PLANT][side-1]
						- urgency * (unitList[*pplant-1]->buildTime / max_buildtime[POWER_PLANT][side-1]);

			//
			if(unit->cost >= max_cost[POWER_PLANT][side-1])
				my_ranking -= (cost + urgency + power)/2.0f;

			//fprintf(ai->file, "%-20s: %f\n", unitList[*pplant-1]->humanName.c_str(), my_ranking);
		}
		else
			my_ranking = -10000;

		if(my_ranking > best_ranking)
		{
				best_ranking = my_ranking;
				best_unit = *pplant;
		}
	}

	// 0 if no unit found (list was probably empty)
	return best_unit;
}

int AAIBuildTable::GetMex(int side, float cost, float effiency, bool armed, bool water, bool canBuild)
{
	int best_unit = 0;
	float best_ranking = -10000, my_ranking;

	side -= 1;

	for(list<int>::iterator i = units_of_category[EXTRACTOR][side].begin(); i != units_of_category[EXTRACTOR][side].end(); i++)
	{
		if(canBuild && units_dynamic[*i].constructorsAvailable <= 0)
			my_ranking = -10000;
		// check if under water or ground || water = true and building under water
		else if( ( (!water) && unitList[*i-1]->minWaterDepth <= 0 ) || ( water && unitList[*i-1]->minWaterDepth > 0 ) )
		{
			my_ranking = effiency * (unitList[*i-1]->extractsMetal - avg_value[EXTRACTOR][side]) / max_value[EXTRACTOR][side]
						- cost * (units_static[*i].cost - avg_cost[EXTRACTOR][side]) / max_cost[EXTRACTOR][side];

			if(armed && !unitList[*i-1]->weapons.empty())
				my_ranking += 1;
		}
		else
			my_ranking = -10000;

		if(my_ranking > best_ranking)
		{
			best_ranking = my_ranking;
			best_unit = *i;
		}
	}

	// 0 if no unit found (list was probably empty)
	return best_unit;
}

int AAIBuildTable::GetBiggestMex()
{
	int biggest_mex = 0, biggest_yard_map = 0;

	for(int s = 0; s < cfg->SIDES; ++s)
	{
		for(list<int>::iterator mex = units_of_category[EXTRACTOR][s].begin();  mex != units_of_category[EXTRACTOR][s].end(); ++mex)
		{
			if(unitList[*mex-1]->xsize * unitList[*mex-1]->zsize > biggest_yard_map)
			{
				biggest_yard_map = unitList[*mex-1]->xsize * unitList[*mex-1]->zsize;
				biggest_mex = *mex;
			}
		}
	}

	return biggest_mex;
}

int AAIBuildTable::GetStorage(int side, float cost, float metal, float energy, float urgency, bool water, bool canBuild)
{
	int best_storage = 0;
	float best_rating = 0, my_rating;

	for(list<int>::iterator storage = units_of_category[STORAGE][side-1].begin(); storage != units_of_category[STORAGE][side-1].end(); storage++)
	{
		if(canBuild && units_dynamic[*storage].constructorsAvailable <= 0)
			my_rating = 0;
		else if(!water && unitList[*storage-1]->minWaterDepth <= 0)
		{
			my_rating = (metal * unitList[*storage-1]->metalStorage + energy * unitList[*storage-1]->energyStorage)
				/(cost * units_static[*storage].cost + urgency * unitList[*storage-1]->buildTime);
		}
		else if(water && unitList[*storage-1]->minWaterDepth > 0)
		{
			my_rating = (metal * unitList[*storage-1]->metalStorage + energy * unitList[*storage-1]->energyStorage)
				/(cost * units_static[*storage].cost + urgency * unitList[*storage-1]->buildTime);
		}
		else
			my_rating = 0;


		if(my_rating > best_rating)
		{
			best_rating = my_rating;
			best_storage = *storage;
		}
	}

	return best_storage;
}

int AAIBuildTable::GetMetalMaker(int side, float cost, float efficiency, float metal, float urgency, bool water, bool canBuild)
{
	int best_maker = 0;
	float best_rating = 0, my_rating;

	for(list<int>::iterator maker = units_of_category[METAL_MAKER][side-1].begin(); maker != units_of_category[METAL_MAKER][side-1].end(); maker++)
	{
		if(canBuild && units_dynamic[*maker].constructorsAvailable <= 0)
			my_rating = 0;
		else if(!water && unitList[*maker-1]->minWaterDepth <= 0)
		{
			my_rating = (pow((long double) efficiency * units_static[*maker].efficiency[0], (long double) 1.4) + pow((long double) metal * unitList[*maker-1]->makesMetal, (long double) 1.6))
				/(pow((long double) cost * units_static[*maker].cost,(long double) 1.4) + pow((long double) urgency * unitList[*maker-1]->buildTime,(long double) 1.4));
		}
		else if(water && unitList[*maker-1]->minWaterDepth > 0)
		{
			my_rating = (pow((long double) efficiency * units_static[*maker].efficiency[0], (long double) 1.4) + pow((long double) metal * unitList[*maker-1]->makesMetal, (long double) 1.6))
				/(pow((long double) cost * units_static[*maker].cost,(long double) 1.4) + pow((long double) urgency * unitList[*maker-1]->buildTime,(long double) 1.4));
		}
		else
			my_rating = 0;


		if(my_rating > best_rating)
		{
			best_rating = my_rating;
			best_maker = *maker;
		}
	}

	return best_maker;
}

int AAIBuildTable::GetDefenceBuilding(int side, double efficiency, double combat_power, double cost, double ground_eff, double air_eff, double hover_eff, double sea_eff, double submarine_eff, double urgency, double range, int randomness, bool water, bool canBuild)
{
	--side;

	double best_ranking = -100000, my_ranking;
	int best_defence = 0;

	UnitTypeStatic *unit;

	double my_power;

	double total_eff = ground_eff + air_eff + hover_eff + sea_eff + submarine_eff;
	double max_eff_selection = 0;
	double max_power = 0;

	int k = 0;

	// use my_power as temp var
	for(list<int>::iterator defence = units_of_category[STATIONARY_DEF][side].begin(); defence != units_of_category[STATIONARY_DEF][side].end(); ++defence)
	{
		if(!canBuild || units_dynamic[*defence].constructorsAvailable > 0)
		{
			unit = &units_static[*defence];

			// calculate eff.
			my_power = ground_eff * unit->efficiency[0] / max_eff[side][5][0] + air_eff * unit->efficiency[1] / max_eff[side][5][1]
					+ hover_eff * unit->efficiency[2] / max_eff[side][5][2] + sea_eff * unit->efficiency[3] / max_eff[side][5][3]
					+ submarine_eff * unit->efficiency[4] / max_eff[side][5][4];
			my_power /= total_eff;

			// store result
			def_power[side][k] = my_power;

			if(my_power > max_power)
				max_power = my_power;

			// calculate eff
			my_power /= unit->cost;

			if(my_power > max_eff_selection)
				max_eff_selection = my_power;

			++k;
		}
	}

	// something went wrong
	if(max_eff_selection <= 0)
		return 0;

	//fprintf(ai->file, "\nSelecting defence: eff %f   power %f   urgency %f  range %f\n", efficiency, combat_power, urgency, range);

	// reset counter
	k = 0;

	// calculate rating
	for(list<int>::iterator defence = units_of_category[STATIONARY_DEF][side].begin(); defence != units_of_category[STATIONARY_DEF][side].end(); ++defence)
	{
		if(canBuild && units_dynamic[*defence].constructorsAvailable <= 0)
			my_ranking = -100000;
		else if( (!water && unitList[*defence-1]->minWaterDepth <= 0) || (water && unitList[*defence-1]->minWaterDepth > 0) )
		{
			unit = &units_static[*defence];

			my_ranking = efficiency * (def_power[side][k] / unit->cost) / max_eff_selection
						+ combat_power * def_power[side][k] / max_power
						+ range * unit->range / max_value[STATIONARY_DEF][side]
						- cost * unit->cost / max_cost[STATIONARY_DEF][side]
						- urgency * unitList[*defence-1]->buildTime / max_buildtime[STATIONARY_DEF][side];

			my_ranking += (0.1 * ((double)(rand()%randomness)));

			//fprintf(ai->file, "%-20s: %f %f %f %f %f\n", unitList[unit->id-1]->humanName.c_str(), t1, t2, t3, t4, my_ranking);
		}
		else
			my_ranking = -100000;

		if(my_ranking > best_ranking)
		{
			best_ranking = my_ranking;
			best_defence = *defence;
		}

		++k;
	}

	return best_defence;
}

int AAIBuildTable::GetCheapDefenceBuilding(int side, double efficiency, double combat_power, double cost, double urgency, double ground_eff, double air_eff, double hover_eff, double sea_eff, double submarine_eff, bool water)
{
	--side;

	double best_ranking = -100000, my_ranking;
	int best_defence = 0;

	UnitTypeStatic *unit;

	double my_power;

	double total_eff = ground_eff + air_eff + hover_eff + sea_eff + submarine_eff;
	double max_eff_selection = 0;
	double max_power = 0;

	unsigned int building_type;

	if(water)
		building_type = MOVE_TYPE_STATIC_WATER;
	else
		building_type = MOVE_TYPE_STATIC_LAND;

	int k = 0;

	// use my_power as temp var
	for(list<int>::iterator defence = units_of_category[STATIONARY_DEF][side].begin(); defence != units_of_category[STATIONARY_DEF][side].end(); ++defence)
	{
		if( units_dynamic[*defence].constructorsAvailable > 0 && building_type & units_static[*defence].movement_type)
		{
			unit = &units_static[*defence];

			// calculate eff.
			my_power = ground_eff * unit->efficiency[0] / avg_eff[side][5][0] + air_eff * unit->efficiency[1] / avg_eff[side][5][1]
					+ hover_eff * unit->efficiency[2] / avg_eff[side][5][2] + sea_eff * unit->efficiency[3] / avg_eff[side][5][3]
					+ submarine_eff * unit->efficiency[4] / avg_eff[side][5][4];
			my_power /= total_eff;

			// store result
			def_power[side][k] = my_power;

			if(my_power > max_power)
				max_power = my_power;

			// calculate eff
			my_power /= unit->cost;

			if(my_power > max_eff_selection)
				max_eff_selection = my_power;

			++k;
		}
	}

	// something went wrong
	if(max_eff_selection <= 0)
		return 0;

	// reset counter
	k = 0;

	// calculate rating
	for(list<int>::iterator defence = units_of_category[STATIONARY_DEF][side].begin(); defence != units_of_category[STATIONARY_DEF][side].end(); ++defence)
	{
		if( units_dynamic[*defence].constructorsAvailable > 0 && building_type & units_static[*defence].movement_type)
		{
			unit = &units_static[*defence];

			my_ranking = efficiency * (def_power[side][k] / unit->cost) / max_eff_selection
						+ combat_power * def_power[side][k] / max_power
						- cost * unit->cost / avg_cost[STATIONARY_DEF][side]
						- urgency * unitList[*defence-1]->buildTime / max_buildtime[STATIONARY_DEF][side];

			if(my_ranking > best_ranking)
			{
				best_ranking = my_ranking;
				best_defence = *defence;
			}

			++k;
			//fprintf(ai->file, "%-20s: %f %f %f %f %f\n", unitList[unit->id-1]->humanName.c_str(), t1, t2, t3, t4, my_ranking);
		}
	}

	return best_defence;
}

int AAIBuildTable::GetRandomDefence(int side, UnitCategory category)
{
	float best_rating = 0, my_rating;

	int best_defence = 0;

	for(list<int>::iterator i = units_of_category[STATIONARY_DEF][side-1].begin(); i != units_of_category[STATIONARY_DEF][side-1].end(); i++)
	{
		my_rating = rand()%512;

		if(my_rating >best_rating)
		{
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_defence = *i;
				best_rating = my_rating;
			}
		}
	}
	return best_defence;
}

int AAIBuildTable::GetAirBase(int side, float cost, bool water, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_airbase = 0;

	for(list<int>::iterator airbase = units_of_category[AIR_BASE][side-1].begin(); airbase != units_of_category[AIR_BASE][side-1].end(); ++airbase)
	{
		// check if water
		if(canBuild && units_dynamic[*airbase].constructorsAvailable <= 0)
			my_ranking = 0;
		else if(!water && unitList[*airbase-1]->minWaterDepth <= 0)
		{
			my_ranking = 100.f / (units_dynamic[*airbase].active + 1);
		}
		else if(water && unitList[*airbase-1]->minWaterDepth > 0)
		{
			//my_ranking =  100 / (cost * units_static[*airbase].cost);
			my_ranking = 100.f / (units_dynamic[*airbase].active + 1);
		}
		else
			my_ranking = 0;

		if(my_ranking > best_ranking)
		{
			best_ranking = my_ranking;
			best_airbase = *airbase;
		}
	}
	return best_airbase;
}

int AAIBuildTable::GetStationaryArty(int side, float cost, float range, float efficiency, bool water, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_arty = 0;

	for(list<int>::iterator arty = units_of_category[STATIONARY_ARTY][side-1].begin(); arty != units_of_category[STATIONARY_ARTY][side-1].end(); ++arty)
	{
		// check if water
		if(canBuild && units_dynamic[*arty].constructorsAvailable <= 0)
			my_ranking = 0;
		else if(!water && unitList[*arty-1]->minWaterDepth <= 0)
		{
			my_ranking =  (range * units_static[*arty].efficiency[1] + efficiency * units_static[*arty].efficiency[0]) / (cost * units_static[*arty].cost);
		}
		else if(water && unitList[*arty-1]->minWaterDepth > 0)
		{
			my_ranking =  (range * units_static[*arty].efficiency[1] + efficiency * units_static[*arty].efficiency[0]) / (cost * units_static[*arty].cost);
		}
		else
			my_ranking = 0;

		if(my_ranking > best_ranking)
		{
			best_ranking = my_ranking;
			best_arty = *arty;
		}
	}
	return best_arty;
}

int AAIBuildTable::GetRadar(int side, float cost, float range, bool water, bool canBuild)
{
	int best_radar = 0;
	float my_rating, best_rating = -10000;
	side -= 1;

	for(list<int>::iterator i = units_of_category[STATIONARY_RECON][side].begin(); i != units_of_category[STATIONARY_RECON][side].end(); i++)
	{
		if(unitList[*i-1]->radarRadius > 0)
		{
			if(canBuild && units_dynamic[*i].constructorsAvailable <= 0)
				my_rating = -10000;
			else if(water && unitList[*i-1]->minWaterDepth > 0)
				my_rating = cost * (avg_cost[STATIONARY_RECON][side] - units_static[*i].cost)/max_cost[STATIONARY_RECON][side]
						+ range * (unitList[*i-1]->radarRadius - avg_value[STATIONARY_RECON][side])/max_value[STATIONARY_RECON][side];
			else if (!water && unitList[*i-1]->minWaterDepth <= 0)
				my_rating = cost * (avg_cost[STATIONARY_RECON][side] - units_static[*i].cost)/max_cost[STATIONARY_RECON][side]
						+ range * (unitList[*i-1]->radarRadius - avg_value[STATIONARY_RECON][side])/max_value[STATIONARY_RECON][side];
			else
				my_rating = -10000;
		}
		else
			my_rating = 0;

		if(my_rating > best_rating)
		{
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_radar = *i;
				best_rating = my_rating;
			}
		}
	}

	return best_radar;
}

int AAIBuildTable::GetJammer(int side, float cost, float range, bool water, bool canBuild)
{
	int best_jammer = 0;
	float my_rating, best_rating = -10000;
	side -= 1;

	for(list<int>::iterator i = units_of_category[STATIONARY_JAMMER][side].begin(); i != units_of_category[STATIONARY_JAMMER][side].end(); i++)
	{
		if(canBuild && units_dynamic[*i].constructorsAvailable <= 0)
			my_rating = -10000;
		else if(water && unitList[*i-1]->minWaterDepth > 0)
			my_rating = cost * (avg_cost[STATIONARY_JAMMER][side] - units_static[*i].cost)/max_cost[STATIONARY_JAMMER][side]
						+ range * (unitList[*i-1]->jammerRadius - avg_value[STATIONARY_JAMMER][side])/max_value[STATIONARY_JAMMER][side];
		else if (!water &&  unitList[*i-1]->minWaterDepth <= 0)
			my_rating = cost * (avg_cost[STATIONARY_JAMMER][side] - units_static[*i].cost)/max_cost[STATIONARY_JAMMER][side]
						+ range * (unitList[*i-1]->jammerRadius - avg_value[STATIONARY_JAMMER][side])/max_value[STATIONARY_JAMMER][side];
		else
			my_rating = -10000;


		if(my_rating > best_rating)
		{
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_jammer = *i;
				best_rating = my_rating;
			}
		}
	}

	return best_jammer;
}

int AAIBuildTable::GetScout(int side, float los, float cost, unsigned int allowed_movement_types, int randomness, bool cloakable, bool canBuild)
{
	side -= 1;

	float best_ranking = -10000, my_ranking;
	int best_scout = 0;

	for(list<int>::iterator i = units_of_category[SCOUT][side].begin(); i != units_of_category[SCOUT][side].end(); ++i)
	{
		if(units_static[*i].movement_type & allowed_movement_types)
		{
			if(!canBuild || (canBuild && units_dynamic[*i].constructorsAvailable > 0))
			{
				my_ranking = los * ( unitList[*i-1]->losRadius - avg_value[SCOUT][side]) / max_value[SCOUT][side];
				my_ranking += cost * (avg_cost[SCOUT][side] - units_static[*i].cost) / max_cost[SCOUT][side];

				if(cloakable && unitList[*i-1]->canCloak)
					my_ranking += 8.0f;

				my_ranking *= (1 + 0.05 * ((float)(rand()%randomness)));

				if(my_ranking > best_ranking)
				{
					best_ranking = my_ranking;
					best_scout = *i;
				}
			}
		}
	}


	return best_scout;
}

int AAIBuildTable::GetRandomUnit(list<int> unit_list)
{
	float best_rating = 0, my_rating;

	int best_unit = 0;

	for(list<int>::iterator i = unit_list.begin(); i != unit_list.end(); i++)
	{
		my_rating = rand()%512;

		if(my_rating >best_rating)
		{
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_unit = *i;
				best_rating = my_rating;
			}
		}
	}
	return best_unit;
}

int AAIBuildTable::GetGroundAssault(int side, float power, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float efficiency, float speed, float range, float cost, int randomness, bool canBuild)
{
	--side;

	float best_ranking = -10000, my_ranking;
	int best_unit = 0;

	float max_cost = this->max_cost[GROUND_ASSAULT][side];
	float max_range = max_value[GROUND_ASSAULT][side];
	float max_speed = this->max_speed[0][side];

	float max_power = 0;
	float max_efficiency  = 0;

	UnitTypeStatic *unit;

	// precache eff
	int c = 0;

	for(list<int>::iterator i = units_of_category[GROUND_ASSAULT][side].begin(); i != units_of_category[GROUND_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		combat_eff[c] = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

		if(combat_eff[c] > max_power)
			max_power = combat_eff[c];

		if(combat_eff[c] / unit->cost > max_efficiency)
			max_efficiency = combat_eff[c] / unit->cost;

		++c;
	}

	c = 0;

	if(max_power <= 0)
		max_power = 1;

	if(max_efficiency <= 0)
		max_efficiency = 1;

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[GROUND_ASSAULT][side].begin(); i != units_of_category[GROUND_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].constructorsAvailable > 0)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else if(!canBuild)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else
			my_ranking = -10000;


		if(my_ranking > best_ranking)
		{
			// check max metal cost
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
		}

		++c;
	}

	return best_unit;
}

int AAIBuildTable::GetHoverAssault(int side,  float power, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float efficiency, float speed, float range, float cost, int randomness, bool canBuild)
{
	UnitTypeStatic *unit;

	--side;

	float best_ranking = -10000, my_ranking;
	int best_unit = 0;

	int c = 0;

	float max_cost = this->max_cost[HOVER_ASSAULT][side];
	float max_range = max_value[HOVER_ASSAULT][side];
	float max_speed = this->max_speed[2][side];

	float max_power = 0;
	float max_efficiency  = 0;

	// precache eff
	for(list<int>::iterator i = units_of_category[HOVER_ASSAULT][side].begin(); i != units_of_category[HOVER_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		combat_eff[c] = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

		if(combat_eff[c] > max_power)
			max_power = combat_eff[c];

		if(combat_eff[c] / unit->cost > max_efficiency)
			max_efficiency = combat_eff[c] / unit->cost;

		++c;
	}

	c = 0;

	if(max_power <= 0)
		max_power = 1;

	if(max_efficiency <= 0)
		max_efficiency = 0;

	for(list<int>::iterator i = units_of_category[HOVER_ASSAULT][side].begin(); i != units_of_category[HOVER_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].constructorsAvailable > 0)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else if(!canBuild)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else
			my_ranking = -10000;

		if(my_ranking > best_ranking)
		{
			// check max metal cost
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
		}
	}

	return best_unit;
}


int AAIBuildTable::GetAirAssault(int side,  float power, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float efficiency, float speed, float range, float cost, int randomness, bool canBuild)
{
	UnitTypeStatic *unit;

	--side;

	float best_ranking = -10000, my_ranking;
	int best_unit = 0;

	int c = 0;

	float max_cost = this->max_cost[AIR_ASSAULT][side];
	float max_range = max_value[AIR_ASSAULT][side];
	float max_speed = this->max_speed[1][side];

	float max_power = 0;
	float max_efficiency  = 0;

	// precache eff
	for(list<int>::iterator i = units_of_category[AIR_ASSAULT][side].begin(); i != units_of_category[AIR_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		combat_eff[c] = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

		if(combat_eff[c] > max_power)
			max_power = combat_eff[c];

		if(combat_eff[c] / unit->cost > max_efficiency)
			max_efficiency = combat_eff[c] / unit->cost;

		++c;
	}

	c = 0;

	if(max_power <= 0)
		max_power = 1;

	if(max_efficiency <= 0)
		max_efficiency = 0;

	for(list<int>::iterator i = units_of_category[AIR_ASSAULT][side].begin(); i != units_of_category[AIR_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].constructorsAvailable > 0)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else if(!canBuild)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else
			my_ranking = 0;

		if(my_ranking > best_ranking)
		{
			// check max metal cost
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
		}
	}

	return best_unit;
}

int AAIBuildTable::GetSeaAssault(int side,  float power, float gr_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float efficiency, float speed, float range, float cost, int randomness, bool canBuild)
{
	UnitTypeStatic *unit;

	--side;

	float best_ranking = -10000, my_ranking;
	int best_unit = 0;

	int c = 0;

	float max_cost = this->max_cost[SEA_ASSAULT][side];
	float max_range = max_value[SEA_ASSAULT][side];
	float max_speed = this->max_speed[3][side];

	float max_power = 0;
	float max_efficiency  = 0;

	// precache eff
	for(list<int>::iterator i = units_of_category[SEA_ASSAULT][side].begin(); i != units_of_category[SEA_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		combat_eff[c] = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
		+ sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

		if(combat_eff[c] > max_power)
			max_power = combat_eff[c];

		if(combat_eff[c] / unit->cost > max_efficiency)
			max_efficiency = combat_eff[c] / unit->cost;

		++c;
	}

	c = 0;

	if(max_power <= 0)
		max_power = 1;

	if(max_efficiency <= 0)
		max_efficiency = 0;

	for(list<int>::iterator i = units_of_category[SEA_ASSAULT][side].begin(); i != units_of_category[SEA_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].constructorsAvailable > 0)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else if(!canBuild)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else
			my_ranking = -10000;

		if(my_ranking > best_ranking)
		{
			// check max metal cost
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
		}
	}

	return best_unit;
}

int AAIBuildTable::GetSubmarineAssault(int side, float power, float sea_eff, float submarine_eff, float stat_eff, float efficiency, float speed, float range, float cost, int randomness, bool canBuild)
{
	UnitTypeStatic *unit;

	--side;

	float best_ranking = -10000, my_ranking;
	int best_unit = 0;

	int c = 0;

	float max_cost = this->max_cost[SUBMARINE_ASSAULT][side];
	float max_range = max_value[SUBMARINE_ASSAULT][side];
	float max_speed = this->max_speed[4][side];

	float max_power = 0;
	float max_efficiency  = 0;

	// precache eff
	for(list<int>::iterator i = units_of_category[SUBMARINE_ASSAULT][side].begin(); i != units_of_category[SUBMARINE_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		combat_eff[c] = sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

		if(combat_eff[c] > max_power)
			max_power = combat_eff[c];

		if(combat_eff[c] / unit->cost > max_efficiency)
			max_efficiency = combat_eff[c] / unit->cost;

		++c;
	}

	c = 0;

	if(max_power <= 0)
		max_power = 1;

	if(max_efficiency <= 0)
		max_efficiency = 0;

	for(list<int>::iterator i = units_of_category[SUBMARINE_ASSAULT][side].begin(); i != units_of_category[SUBMARINE_ASSAULT][side].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].constructorsAvailable > 0)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else if(!canBuild)
		{
			my_ranking = power * combat_eff[c] / max_power;
			my_ranking -= cost * unit->cost / max_cost;
			my_ranking += efficiency * (combat_eff[c] / unit->cost) / max_efficiency;
			my_ranking += range * unit->range / max_range;
			my_ranking += speed * unitList[*i-1]->speed / max_speed;
			my_ranking += 0.1f * ((float)(rand()%randomness));
		}
		else
			my_ranking = -10000;

		if(my_ranking > best_ranking)
		{
			// check max metal cost
			if(unitList[*i-1]->metalCost < cfg->MAX_METAL_COST)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
		}
	}

	return best_unit;
}

void AAIBuildTable::UpdateTable(const UnitDef* def_killer, int killer, const UnitDef *def_killed, int killed)
{
	// buidling killed
	if(killed == 5)
	{
		// stationary defence killed
		if(units_static[def_killed->id].category == STATIONARY_DEF)
		{
			float change = cfg->LEARN_SPEED * units_static[def_killed->id].efficiency[killer] / units_static[def_killer->id].efficiency[killed];

			if(change > 0.5)
				change = 0.5;
			else if(change < cfg->MIN_EFFICIENCY/2.0f)
				change = cfg->MIN_EFFICIENCY/2.0f;

			units_static[def_killer->id].efficiency[killed] += change;
			units_static[def_killed->id].efficiency[killer] -= change;

			if(units_static[def_killed->id].efficiency[killer] < cfg->MIN_EFFICIENCY)
				units_static[def_killed->id].efficiency[killer] = cfg->MIN_EFFICIENCY;
		}
		// other building killed
		else
		{
			if(units_static[def_killer->id].efficiency[5] < 8)
			{
				if(killer == 1)	// aircraft
					units_static[def_killer->id].efficiency[5] += cfg->LEARN_SPEED / 3.0f;
				else			// other assault units
					units_static[def_killer->id].efficiency[5] += cfg->LEARN_SPEED / 9.0f;
			}
		}
	}
	// unit killed
	else
	{
		float change = cfg->LEARN_SPEED * units_static[def_killed->id].efficiency[killer] / units_static[def_killer->id].efficiency[killed];

		if(change > 0.5)
			change = 0.5;
		else if(change < cfg->MIN_EFFICIENCY/2.0f)
			change = cfg->MIN_EFFICIENCY/2.0f;

		units_static[def_killer->id].efficiency[killed] += change;
		units_static[def_killed->id].efficiency[killer] -= change;

		if(units_static[def_killed->id].efficiency[killer] < cfg->MIN_EFFICIENCY)
			units_static[def_killed->id].efficiency[killer] = cfg->MIN_EFFICIENCY;
	}
}

void AAIBuildTable::UpdateMinMaxAvgEfficiency()
{
	int counter;
	float min, max, sum;
	UnitCategory killer, killed;

	for(int side = 0; side < numOfSides; ++side)
	{
		for(int i = 0; i < combat_categories; ++i)
		{
			for(int j = 0; j < combat_categories; ++j)
			{
				killer = GetAssaultCategoryOfID(i);
				killed = GetAssaultCategoryOfID(j);
				counter = 0;

				// update max and avg efficiency of i versus j
				max = 0;
				min = 100000;
				sum = 0;

				for(list<int>::iterator unit = units_of_category[killer][side].begin(); unit != units_of_category[killer][side].end(); ++unit)
				{
					// only count anti air units vs air and assault units vs non air
					if( (killed == AIR_ASSAULT && units_static[*unit].unit_type == ANTI_AIR_UNIT) || (killed != AIR_ASSAULT && units_static[*unit].unit_type != ANTI_AIR_UNIT))
					{
						sum += units_static[*unit].efficiency[j];

						if(units_static[*unit].efficiency[j] > max)
							max = units_static[*unit].efficiency[j];

						if(units_static[*unit].efficiency[j] < min)
							min = units_static[*unit].efficiency[j];

						++counter;
					}
				}

				if(counter > 0)
				{
					avg_eff[side][i][j] = sum / counter;
					max_eff[side][i][j] = max;
					min_eff[side][i][j] = min;

					total_eff[side][i][j] = max - min;

					if(total_eff[side][i][j] <= 0)
						total_eff[side][i][j] = 1;

					if(max_eff[side][i][j] <= 0)
						max_eff[side][i][j] = 1;

					if(avg_eff[side][i][j] <= 0)
						avg_eff[side][i][j] = 1;

					if(min_eff[side][i][j] <= 0)
						min_eff[side][i][j] = 1;
				}
				else
				{
					// set to 1 to prevent division by zero crashes
					max_eff[side][i][j] = 1;
					min_eff[side][i][j] = 1;
					avg_eff[side][i][j] = 1;
					total_eff[side][i][j] = 1;
				}

				//fprintf(ai->file, "min_eff[%i][%i] %f;  max_eff[%i][%i] %f\n", i, j, this->min_eff[i][j], i, j, this->max_eff[i][j]);
			}
		}
	}
}

// returns true if unitdef->id can build unit with unitdef->id
bool AAIBuildTable::CanBuildUnit(int id_builder, int id_unit)
{
	// look in build options of builder for unit
	for(list<int>::iterator i = units_static[id_builder].canBuildList.begin(); i != units_static[id_builder].canBuildList.end(); ++i)
	{
		if(*i == id_unit)
			return true;
	}

	// unit not found in builder�s buildoptions
	return false;
}

// dtermines sides of units by recursion
void AAIBuildTable::CalcBuildTree(int unit)
{
	// go through all possible build options and set side if necessary
	for(list<int>::iterator i = units_static[unit].canBuildList.begin(); i != units_static[unit].canBuildList.end(); ++i)
	{
		// add this unit to targets builtby-list
		units_static[*i].builtByList.push_back(unit);

		// calculate builder_costs
		if( units_static[unit].cost < units_static[*i].builder_cost || units_static[*i].builder_cost <= 0)
			units_static[*i].builder_cost = units_static[unit].cost;

		// continue with all builldoptions (if they have not been visited yet)
		if(!units_static[*i].side && AllowedToBuild(*i))
		{
			// unit has not been checked yet, set side as side of its builder and continue
			units_static[*i].side = units_static[unit].side;

			CalcBuildTree(*i);
		}
	}
}


// returns true if cache found
bool AAIBuildTable::LoadBuildTable()
{
	// stop further loading if already done
	if(!units_static.empty())
	{
		units_dynamic.resize(numOfUnits+1);

		for(int i = 0; i <= numOfUnits; ++i)
		{
			units_dynamic[i].active = 0;
			units_dynamic[i].requested = 0;
			units_dynamic[i].constructorsAvailable = 0;
			units_dynamic[i].constructorsRequested = 0;
		}

		return true;
	}
	else	// load data
	{
		// get filename
		char buffer[500];
		STRCPY(buffer, MAIN_PATH);
		STRCAT(buffer, MOD_LEARN_PATH);
		STRCAT(buffer, cb->GetModName());
		ReplaceExtension (buffer, buildtable_filename, sizeof(buildtable_filename), ".dat");

		char buildtable_filename_r[500];
		STRCPY(buildtable_filename_r, buildtable_filename);
		ai->cb->GetValue(AIVAL_LOCATE_FILE_R, buildtable_filename_r);

		FILE *load_file;

		int tmp = 0, bo = 0, bb = 0, cat = 0;

		// load units if file exists
		if((load_file = fopen(buildtable_filename_r, "r")))
		{
			// check if correct version
			fscanf(load_file, "%s", buffer);

			if(strcmp(buffer, MOD_LEARN_VERSION))
			{
				cb->SendTextMsg("Buildtable version out of date - creating new one",0);
				return false;
			}

			int counter = 0;

			// load attacked_by table
			for(int map = 0; map <= (int)WATER_MAP; ++map)
			{
				for(int t = 0; t < 4; ++t)
				{
					for(int category = 0; category < combat_categories; ++category)
					{
						++counter;
						fscanf(load_file, "%f ", &attacked_by_category_learned[map][t][category]);
					}
				}
			}

			units_static.resize(numOfUnits+1);
			units_dynamic.resize(numOfUnits+1);
			fixed_eff.resize(numOfUnits+1, vector<float>(combat_categories));

			units_static[0].def_id = 0;
			units_static[0].side = 0;

			for(int i = 1; i <= numOfUnits; ++i)
			{
				fscanf(load_file, "%i %i %u %u %f %f %f %i %i %i ",&units_static[i].def_id, &units_static[i].side,
									&units_static[i].unit_type, &units_static[i].movement_type,
									&units_static[i].range, &units_static[i].cost, &units_static[i].builder_cost,
									&cat, &bo, &bb);

				// get memory for eff
				units_static[i].efficiency.resize(combat_categories);

				// load eff
				for(int k = 0; k < combat_categories; ++k)
				{
					fscanf(load_file, "%f ", &units_static[i].efficiency[k]);
					fixed_eff[i][k] = units_static[i].efficiency[k];
				}

				units_static[i].category = (UnitCategory) cat;

				units_dynamic[i].active = 0;
				units_dynamic[i].requested = 0;
				units_dynamic[i].constructorsAvailable = 0;
				units_dynamic[i].constructorsRequested = 0;

				// load buildoptions
				for(int j = 0; j < bo; j++)
				{
					fscanf(load_file, "%i ", &tmp);
					units_static[i].canBuildList.push_back(tmp);
				}

				// load builtby-list
				for(int k = 0; k < bb; ++k)
				{
					fscanf(load_file, "%i ", &tmp);
					units_static[i].builtByList.push_back(tmp);
				}
			}

			// now load unit lists
			for(int s = 0; s < numOfSides; ++s)
			{
				for(int cat = 0; cat <= MOBILE_CONSTRUCTOR; ++cat)
				{
					// load number of buildoptions
					fscanf(load_file, "%i ", &bo);

					for(int i = 0; i < bo; ++i)
					{
						fscanf(load_file, "%i ", &tmp);
						units_of_category[cat][s].push_back(tmp);
					}

					// load pre cached values
					fscanf(load_file, "%f %f %f %f %f %f %f %f %f \n",
						&max_cost[cat][s], &min_cost[cat][s], &avg_cost[cat][s],
						&max_buildtime[cat][s], &min_buildtime[cat][s], &avg_buildtime[cat][s],
						&max_value[cat][s], &min_value[cat][s], &avg_value[cat][s]);
				}

				// load cached speed stats
				for(int cat = 0; cat < combat_categories; ++cat)
				{
					fscanf(load_file, "%f %f %f %f \n", &min_speed[cat][s], &max_speed[cat][s], &group_speed[cat][s], &avg_speed[cat][s]);
				}

				// update group speed (in case UNIT_SPEED_SUBGROUPS changed)
				for(int cat = 0; cat < combat_categories; ++cat)
					group_speed[cat][s] = (1 + max_speed[cat][s] - min_speed[cat][s]) / ((float)cfg->UNIT_SPEED_SUBGROUPS);
			}

			fclose(load_file);
			return true;
		}
	}

	return false;
}

void AAIBuildTable::SaveBuildTable(int game_period, MapType map_type)
{
	// reset factory ratings
	for(int s = 0; s < cfg->SIDES; ++s)
	{
		for(list<int>::iterator fac = units_of_category[STATIONARY_CONSTRUCTOR][s].begin(); fac != units_of_category[STATIONARY_CONSTRUCTOR][s].end(); ++fac)
		{
			units_static[*fac].efficiency[5] = -1;
			units_static[*fac].efficiency[4] = 0;
		}
	}
	// reset builder ratings
	for(int s = 0; s < cfg->SIDES; ++s)
	{
		for(list<int>::iterator builder = units_of_category[MOBILE_CONSTRUCTOR][s].begin(); builder != units_of_category[MOBILE_CONSTRUCTOR][s].end(); ++builder)
			units_static[*builder].efficiency[5] = -1;
	}

	// get filename
	char buildtable_filename_w[500];
	STRCPY(buildtable_filename_w, buildtable_filename);
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, buildtable_filename_w);
	FILE *save_file = fopen(buildtable_filename_w, "w+");

	// file version
	fprintf(save_file, "%s \n", MOD_LEARN_VERSION);

	// update attacked_by values
	// FIXME: using t two times as the for-loop-var?
	for(int t = 0; t < 4; ++t)
	{
		for(int cat = 0; cat < combat_categories; ++cat)
		{
			for(int t = 0; t < game_period; ++t)
			{
				attacked_by_category_learned[map_type][t][cat] =
						0.75f * attacked_by_category_learned[map_type][t][cat] +
						0.25f * attacked_by_category_current[t][cat];
			}
		}
	}

	// save attacked_by table
	for(int map = 0; map <= WATER_MAP; ++map)
	{
		for(int t = 0; t < 4; ++t)
		{
			for(int cat = 0; cat < combat_categories; ++cat)
			{
				fprintf(save_file, "%f ", attacked_by_category_learned[map][t][cat]);
				fprintf(save_file, "\n");
			}
		}
	}

	int tmp;

	for(int i = 1; i <= numOfUnits; ++i)
	{
		tmp = units_static[i].canBuildList.size();

		fprintf(save_file, "%i %i %u %u %f %f %f %i %i %i ", units_static[i].def_id, units_static[i].side,
								units_static[i].unit_type, units_static[i].movement_type, units_static[i].range,
								units_static[i].cost, units_static[i].builder_cost, (int) units_static[i].category,
								units_static[i].canBuildList.size(), units_static[i].builtByList.size());

		// save combat eff
		for(int k = 0; k < combat_categories; ++k)
			fprintf(save_file, "%f ", units_static[i].efficiency[k]);

		// save buildoptions
		for(list<int>::iterator j = units_static[i].canBuildList.begin(); j != units_static[i].canBuildList.end(); ++j)
			fprintf(save_file, "%i ", *j);

		// save builtby-list
		for(list<int>::iterator k = units_static[i].builtByList.begin(); k != units_static[i].builtByList.end(); ++k)
			fprintf(save_file, "%i ", *k);

		fprintf(save_file, "\n");
	}

	for(int s = 0; s < numOfSides; ++s)
	{
		// now save unit lists
		for(int cat = 0; cat <= MOBILE_CONSTRUCTOR; ++cat)
		{
			// save number of units
			fprintf(save_file, "%i ", units_of_category[cat][s].size());

			for(list<int>::iterator unit = units_of_category[cat][s].begin(); unit != units_of_category[cat][s].end(); ++unit)
				fprintf(save_file, "%i ", *unit);

			fprintf(save_file, "\n");

			// save pre cached values
			fprintf(save_file, "%f %f %f %f %f %f %f %f %f \n",
						max_cost[cat][s], min_cost[cat][s], avg_cost[cat][s],
						max_buildtime[cat][s], min_buildtime[cat][s], avg_buildtime[cat][s],
						max_value[cat][s], min_value[cat][s], avg_value[cat][s]);

			fprintf(save_file, "\n");
		}

		// save cached speed stats
		for(int cat = 0; cat < combat_categories; ++cat)
		{
			fprintf(save_file, "%f %f %f %f \n", min_speed[cat][s], max_speed[cat][s], group_speed[cat][s], avg_speed[cat][s]);

			fprintf(save_file, "\n");
		}
	}

	fclose(save_file);
}

void AAIBuildTable::DebugPrint()
{
	if(!unitList)
		return;

	// for debugging
	UnitType unitType;
	char filename[500];
	char buffer[500];
	STRCPY(buffer, MAIN_PATH);
	STRCAT(buffer, AILOG_PATH);
	STRCAT(buffer, "BuildTable_");
	STRCAT(buffer, cb->GetModName());
	ReplaceExtension (buffer, filename, sizeof(filename), ".txt");

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	FILE *file = fopen(filename, "w");

	if(file)
	{

	for(int i = 1; i <= numOfUnits; i++)
	{
		// unit type
		unitType = GetUnitType(i);

		if(cfg->AIR_ONLY_MOD)
		{
			fprintf(file, "ID: %-3i %-16s %-40s %-25s %s\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
		}
		else
		{
			fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());

			if(units_static[i].category == GROUND_ASSAULT ||units_static[i].category == SEA_ASSAULT || units_static[i].category == HOVER_ASSAULT)
			{
				if(unitType == ANTI_AIR_UNIT)
					fprintf(file, " anti air unit");
				else if(unitType == ASSAULT_UNIT)
					fprintf(file, " assault unit");
			}
			else if(units_static[i].category == AIR_ASSAULT)
			{
				if(unitType == ANTI_AIR_UNIT)
					fprintf(file, " fighter");
				else if(unitType == ASSAULT_UNIT)
					fprintf(file, " gunship");
				else
					fprintf(file, " bomber");
			}
			else if(units_static[i].category == SUBMARINE_ASSAULT)
				fprintf(file, " assault unit");

			if(IsBuilder(i))
				fprintf(file, " builder");

			if(IsFactory(i))
				fprintf(file, " factory");

			if(IsCommander(i))
				fprintf(file, " commander");

			if(units_static[i].movement_type & MOVE_TYPE_AMPHIB)
				fprintf(file, " amphibious");

			fprintf(file, "\n");
		}

		//fprintf(file, "Max damage: %f\n", GetMaxDamage(i));

		//fprintf(file, "Max damage: %f\n", GetMaxDamage(i));
		/*fprintf(file, "Can Build:\n");

		for(list<int>::iterator j = units_static[i].canBuildList.begin(); j != units_static[i].canBuildList.end(); ++j)
			fprintf(file, "%s ", unitList[*j-1]->humanName.c_str());

		fprintf(file, "\n Built by: ");

		for(list<int>::iterator k = units_static[i].builtByList.begin(); k != units_static[i].builtByList.end(); ++k)
			fprintf(file, "%s ", unitList[*k-1]->humanName.c_str());*/

		fprintf(file, "\n \n");
	}

	for(int s = 1; s <= numOfSides; s++)
	{
		// print unit lists
		for(int cat = 1; cat <= MOBILE_CONSTRUCTOR; ++cat)
		{
			if(units_of_category[cat][s-1].size() > 0)
			{
				fprintf(file, "\n%s %s:\n",GetCategoryString2((UnitCategory) cat), sideNames[s].c_str());

				for(list<int>::iterator unit = units_of_category[cat][s-1].begin(); unit != units_of_category[cat][s-1].end(); ++unit)
					fprintf(file, "%s    ", unitList[*unit-1]->humanName.c_str());

				fprintf(file, "\n");
			}
		}

		// print average costs/speed, etc
		/*if(units_of_category[cat][s-1].size() > 0)
		{
			fprintf(file, "\nMin/Max/Average unit cost\n");

			for(int cat = 0; cat <= SEA_BUILDER; ++cat)
				fprintf(file, "\n%s %s: %f, %f, %f \n",GetCategoryString2((UnitCategory) cat), sideNames[s].c_str(), min_cost[cat][s-1], max_cost[cat][s-1], avg_cost[cat][s-1]);

			fprintf(file, "\nMin/Max/Average unit buildtime\n");

			for(int cat = 0; cat <= SEA_BUILDER; ++cat)
				fprintf(file, "\n%s %s: %f, %f, %f \n",GetCategoryString2((UnitCategory) cat), sideNames[s].c_str(), min_buildtime[cat][s-1], max_buildtime[cat][s-1], avg_buildtime[cat][s-1]);

			fprintf(file, "\nMin/Max/Average unit value\n");

			for(int cat = 0; cat <= SEA_BUILDER; ++cat)
				fprintf(file, "\n%s %s: %f, %f, %f \n",GetCategoryString2((UnitCategory) cat), sideNames[s].c_str(), min_value[cat][s-1], max_value[cat][s-1], avg_value[cat][s-1]);
		}*/

		//fprintf(file, "\nAverage/Min speed\n\n");
		//fprintf(file, "\n%s %s: %f, %f \n",GetCategoryString2(GROUND_ASSAULT), sideNames[s].c_str(), avg_value[GROUND_ASSAULT][s-1], max_value[GROUND_ASSAULT][s-1]);
		//fprintf(file, "\n%s %s: %f, %f \n",GetCategoryString2(SEA_ASSAULT), sideNames[s].c_str(), avg_value[SEA_ASSAULT][s-1], max_value[SEA_ASSAULT][s-1]);
	}

	fclose(file);
	}
	// error
	else
	{
	}
}

float AAIBuildTable::GetMaxRange(int unit_id)
{
	float max_range = 0;

	for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unitList[unit_id -1]->weapons.begin(); i != unitList[unit_id -1]->weapons.end(); i++)
	{
		if((*i).def->range > max_range)
			max_range = (*i).def->range;
	}

	return max_range;
}

float AAIBuildTable::GetMaxDamage(int unit_id)
{
	float max_damage = 0;

	int armor_types;
	cb->GetValue(AIVAL_NUMDAMAGETYPES,&armor_types);

	for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unitList[unit_id -1]->weapons.begin(); i != unitList[unit_id -1]->weapons.end(); ++i)
	{
		for(int k = 0; k < armor_types; ++k)
		{
			if((*i).def->damages[k] > max_damage)
				max_damage = (*i).def->damages[k];
		}
	}

	return max_damage;
}

void ReplaceExtension(const char *n, char *dst, int s, const char *ext)
{
	unsigned int l = strlen (n);

	unsigned int a=l-1;
	while (n[a] && n[a]!='.' && a>0)
		a--;

	strncpy (dst, "", s);
	if (a>s-sizeof(MAIN_PATH)) a=s-sizeof("");
	memcpy (&dst [sizeof ("")-1], n, a);
	dst[a+sizeof("")]=0;

	strncat (dst, ext, s);
}

float AAIBuildTable::GetFactoryRating(int def_id)
{
	// check if value already chached
	if(units_static[def_id].efficiency[5] != -1)
		return units_static[def_id].efficiency[5];

	// calculate rating and cache result
	bool builder = false;
	bool scout = false;
	float rating = 1.0f;
	float combat_units = 0;
	float ground = 0.1f + 0.01f * (attacked_by_category_learned[ai->map->map_type][0][0] + attacked_by_category_learned[ai->map->map_type][1][0] + attacked_by_category_learned[ai->map->map_type][2][0]);
	float air = 0.1f + 0.01f * (attacked_by_category_learned[ai->map->map_type][0][1] + attacked_by_category_learned[ai->map->map_type][1][1] + attacked_by_category_learned[ai->map->map_type][2][1]);
	float hover = 0.1f + 0.01f * (attacked_by_category_learned[ai->map->map_type][0][2] + attacked_by_category_learned[ai->map->map_type][1][2] + attacked_by_category_learned[ai->map->map_type][2][2]);
	float sea = 0.1f + 0.01f * (attacked_by_category_learned[ai->map->map_type][0][3] + attacked_by_category_learned[ai->map->map_type][1][3] + attacked_by_category_learned[ai->map->map_type][2][3]);
	float submarine = 0.1f + 0.01f * (attacked_by_category_learned[ai->map->map_type][0][4] + attacked_by_category_learned[ai->map->map_type][1][4] + attacked_by_category_learned[ai->map->map_type][2][4]);

	if(cfg->AIR_ONLY_MOD)
	{
		for(list<int>::iterator unit = units_static[def_id].canBuildList.begin(); unit != units_static[def_id].canBuildList.end(); unit++)
		{
			if(units_static[*unit].category >= GROUND_ASSAULT && units_static[*unit].category <= SEA_ASSAULT)
			{
				rating += ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3];
				combat_units += 1;
			}
			else if(IsBuilder(def_id))
			{
				rating += 256.0 * GetBuilderRating(*unit);
				builder = true;
			}
			else if(IsScout(def_id))
			{
				scout = true;
			}
		}
	}
	else if(ai->map->map_type == LAND_MAP)
	{
		for(list<int>::iterator unit = units_static[def_id].canBuildList.begin(); unit != units_static[def_id].canBuildList.end(); ++unit)
		{
			if(units_static[*unit].category == GROUND_ASSAULT || units_static[*unit].category == HOVER_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.5f * (ground * units_static[*unit].efficiency[0] + hover * units_static[*unit].efficiency[2]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.5f * (ground * units_static[*unit].efficiency[0] + hover * units_static[*unit].efficiency[2]);

				combat_units += 1;
			}
			else if(IsBuilder(def_id) && !IsSea(def_id))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
			else if(units_static[*unit].category == SCOUT && !(units_static[*unit].movement_type & MOVE_TYPE_SEA) )
			{
				scout = true;
			}
		}
	}
	else if(ai->map->map_type == LAND_WATER_MAP)
	{
		for(list<int>::iterator unit = units_static[def_id].canBuildList.begin(); unit != units_static[def_id].canBuildList.end(); ++unit)
		{
			if(units_static[*unit].category == GROUND_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.5f * (ground * units_static[*unit].efficiency[0] + hover * units_static[*unit].efficiency[2]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == HOVER_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.33f * (ground * units_static[*unit].efficiency[0] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.33f * (ground * units_static[*unit].efficiency[0] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == SEA_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.33f * (hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == SUBMARINE_ASSAULT)
			{
				rating += 0.5f * (sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(IsBuilder(def_id))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
			else if(units_static[*unit].category == SCOUT)
			{
				scout = true;
			}
		}
	}
	else if(ai->map->map_type == WATER_MAP)
	{
		for(list<int>::iterator unit = units_static[def_id].canBuildList.begin(); unit != units_static[def_id].canBuildList.end(); ++unit)
		{
			if(units_static[*unit].category == HOVER_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.5f * (hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.5f * (hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == SEA_ASSAULT)
			{
				if(units_static[*unit].unit_type == ANTI_AIR_UNIT)
					rating += air * units_static[*unit].efficiency[1];
				else
					rating += 0.33f * (hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);

				combat_units += 1;
			}
			else if(units_static[*unit].category == SUBMARINE_ASSAULT)
			{
				rating += 0.5f * (sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(IsBuilder(def_id))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
			else if(units_static[*unit].category == SCOUT)
			{
				scout = true;
			}
		}
	}

	if(combat_units > 0)
	{
		rating /= (combat_units * units_static[def_id].cost);
		rating *= fastmath::sqrt((float) (4 + combat_units) );

		if(scout)
			rating += 8.0f;

		units_static[def_id].efficiency[5] = rating;
		return rating;
	}
	else if(builder)
	{
		if(scout)
		{

			units_static[def_id].efficiency[5] = 1.0f;
			return 1.0f;
		}
		else
		{
			units_static[def_id].efficiency[5] = 0.5f;
			return 0.5f;
		}
	}
	else
	{
		units_static[def_id].efficiency[5] = 0;
		return 0;
	}
}

float AAIBuildTable::GetBuilderRating(int def_id)
{
	// check if value already chached
	if(units_static[def_id].efficiency[5] != -1)
		return units_static[def_id].efficiency[5];
	else
	{
		// calculate rating and cache result
		int buildings = 10;

		// only cout buildings that are likely to be built on that type of map
		if(ai->map->map_type == LAND_MAP)
		{
			for(list<int>::iterator building = units_static[def_id].canBuildList.begin(); building != units_static[def_id].canBuildList.end(); ++building)
			{
				if(unitList[*building-1]->minWaterDepth <= 0)
					++buildings;
			}
		}
		else if(ai->map->map_type == WATER_MAP)
		{
			for(list<int>::iterator building = units_static[def_id].canBuildList.begin(); building != units_static[def_id].canBuildList.end(); ++building)
			{
				if(unitList[*building-1]->minWaterDepth > 0)
					++buildings;
			}
		}
		else
		{
			buildings = units_static[def_id].canBuildList.size();
		}

		units_static[def_id].efficiency[5] = sqrt((double)buildings);

		return units_static[def_id].efficiency[5];
	}
}

void AAIBuildTable::BuildFactoryFor(int unit_def_id)
{
	int constructor = 0;
	float best_rating = -100000.0f, my_rating;

	float cost = 1.0f;
	float buildspeed = 1.0f;

	// determine max values
	float max_buildtime = 0;
	float max_buildspeed = 0;
	float max_cost = 0;

	for(list<int>::iterator factory = units_static[unit_def_id].builtByList.begin();  factory != units_static[unit_def_id].builtByList.end(); ++factory)
	{
		if(units_static[*factory].cost > max_cost)
			max_cost = units_static[*factory].cost;

		if(unitList[*factory-1]->buildTime > max_buildtime)
			max_buildtime = unitList[*factory-1]->buildTime;

		if(unitList[*factory-1]->buildSpeed > max_buildspeed)
			max_buildspeed = unitList[*factory-1]->buildSpeed;
	}

	// look for best builder to do the job
	for(list<int>::iterator factory = units_static[unit_def_id].builtByList.begin();  factory != units_static[unit_def_id].builtByList.end(); ++factory)
	{
		if(units_dynamic[*factory].active + units_dynamic[*factory].requested + units_dynamic[*factory].under_construction < cfg->MAX_FACTORIES_PER_TYPE)
		{
			my_rating = buildspeed * (unitList[*factory-1]->buildSpeed / max_buildspeed)
				- (unitList[*factory-1]->buildTime / max_buildtime)
				- cost * (units_static[*factory].cost / max_cost);

			//my_rating += GetBuilderRating(*unit, cost, buildspeed) / ;

			// prefer builders that can be built atm
			if(units_dynamic[*factory].constructorsAvailable > 0)
				my_rating += 2.0f;

			// prevent AAI from requesting factories that cannot be built within the current base
			if(units_static[*factory].movement_type & MOVE_TYPE_STATIC_LAND)
			{
				if(ai->brain->baseLandRatio > 0.1f)
					my_rating *= ai->brain->baseLandRatio;
				else
					my_rating = -100000.0f;
			}
			else if(units_static[*factory].movement_type & MOVE_TYPE_STATIC_WATER)
			{
				if(ai->brain->baseWaterRatio > 0.1f)
					my_rating *= ai->brain->baseWaterRatio;
				else
					my_rating = -100000.0f;
			}

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				constructor = *factory;
			}
		}
	}

	if(constructor && units_dynamic[constructor].requested + units_dynamic[constructor].under_construction <= 0)
	{
		for(list<int>::iterator j = units_static[constructor].canBuildList.begin(); j != units_static[constructor].canBuildList.end(); ++j)
		{
			//// only set to true, if the factory is not built by that unit itself
			//if(!MemberOf(*j, units_static[*i].builtByList))
			units_dynamic[*j].constructorsRequested += 1;
		}

		units_dynamic[constructor].requested += 1;

		// factory requested
		if(IsStatic(constructor))
		{
			if(units_dynamic[constructor].constructorsAvailable + units_dynamic[constructor].constructorsRequested <= 0)
			{
				fprintf(ai->file, "BuildFactoryFor(%s) is requesting builder for %s\n", unitList[unit_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());
				BuildBuilderFor(constructor);
			}

			// debug
			fprintf(ai->file, "BuildFactoryFor(%s) requested %s\n", unitList[unit_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());
		}
		// mobile constructor requested
		else
		{
			if(ai->execute->AddUnitToBuildqueue(constructor, 1, true))
			{
				// increase counter if mobile factory is a builder as well
				if(units_static[constructor].unit_type & UNIT_TYPE_BUILDER)
					ai->ut->futureBuilders += 1;

				if(units_dynamic[constructor].constructorsAvailable + units_dynamic[constructor].constructorsRequested <= 0)
				{
					fprintf(ai->file, "BuildFactoryFor(%s) is requesting factory for %s\n", unitList[unit_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());
					BuildFactoryFor(constructor);
				}

				// debug
				fprintf(ai->file, "BuildFactoryFor(%s) requested %s\n", unitList[unit_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());
			}
			else
			{
				//something went wrong -> decrease values
				units_dynamic[constructor].requested -= 1;

				for(list<int>::iterator j = units_static[constructor].canBuildList.begin(); j != units_static[constructor].canBuildList.end(); ++j)
					units_dynamic[*j].constructorsRequested -= 1;
			}
		}
	}
}

// tries to build another builder for a certain building
void AAIBuildTable::BuildBuilderFor(int building_def_id)
{
	int constructor = 0;
	float best_rating = -10000.0f, my_rating;

	float cost = 1.0f;
	float buildspeed = 1.0f;

	// determine max values
	float max_buildtime = 0;
	float max_buildspeed = 0;
	float max_cost = 0;

	for(list<int>::iterator builder = units_static[building_def_id].builtByList.begin();  builder != units_static[building_def_id].builtByList.end(); ++builder)
	{
		if(units_static[*builder].cost > max_cost)
			max_cost = units_static[*builder].cost;

		if(unitList[*builder-1]->buildTime > max_buildtime)
			max_buildtime = unitList[*builder-1]->buildTime;

		if(unitList[*builder-1]->buildSpeed > max_buildspeed)
			max_buildspeed = unitList[*builder-1]->buildSpeed;
	}

	// look for best builder to do the job
	for(list<int>::iterator builder = units_static[building_def_id].builtByList.begin();  builder != units_static[building_def_id].builtByList.end(); ++builder)
	{
		// prevent ai from ordering too many builders of the same type/commanders/builders that cant be built atm
		if(units_dynamic[*builder].active + units_dynamic[*builder].under_construction + units_dynamic[*builder].requested < cfg->MAX_BUILDERS_PER_TYPE)
		{
			my_rating = buildspeed * (unitList[*builder-1]->buildSpeed / max_buildspeed)
				- (unitList[*builder-1]->buildTime / max_buildtime)
				- cost * (units_static[*builder].cost / max_cost);

			// prefer builders that can be built atm
			if(units_dynamic[*builder].constructorsAvailable > 0)
				my_rating += 1.5f;

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				constructor = *builder;
			}
		}
	}

	if(constructor && units_dynamic[constructor].under_construction + units_dynamic[constructor].requested <= 0)
	{
		// build factory if necessary
		if(units_dynamic[constructor].constructorsAvailable + units_dynamic[constructor].constructorsRequested <= 0)
		{
			fprintf(ai->file, "BuildBuilderFor(%s) is requesting factory for %s\n", unitList[building_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());

			BuildFactoryFor(constructor);
		}

		if(ai->execute->AddUnitToBuildqueue(constructor, 1, true))
		{
			units_dynamic[constructor].requested += 1;
			ai->ut->futureBuilders += 1;
			ai->ut->UnitRequested(MOBILE_CONSTRUCTOR);

			// set all its buildoptions buildable
			for(list<int>::iterator j = units_static[constructor].canBuildList.begin(); j != units_static[constructor].canBuildList.end(); j++)
				units_dynamic[*j].constructorsRequested += 1;

			// debug
			fprintf(ai->file, "BuildBuilderFor(%s) requested %s\n", unitList[building_def_id-1]->humanName.c_str(), unitList[constructor-1]->humanName.c_str());
		}
	}
}


void AAIBuildTable::AddAssistant(unsigned int allowed_movement_types, bool canBuild)
{
	int builder = 0;
	float best_rating = -10000, my_rating;

	int side = ai->side-1;

	float cost = 1.0f;
	float buildspeed = 2.0f;
	float urgency = 1.0f;

	for(list<int>::iterator unit = units_of_category[MOBILE_CONSTRUCTOR][side].begin();  unit != units_of_category[MOBILE_CONSTRUCTOR][side].end(); ++unit)
	{
		if(units_static[*unit].movement_type & allowed_movement_types)
		{
			if( (!canBuild || units_dynamic[*unit].constructorsAvailable > 0)
				&& units_dynamic[*unit].active + units_dynamic[*unit].under_construction + units_dynamic[*unit].requested < cfg->MAX_BUILDERS_PER_TYPE)
			{
				if( unitList[*unit-1]->buildSpeed >= (float)cfg->MIN_ASSISTANCE_BUILDTIME && unitList[*unit-1]->canAssist)
				{
					my_rating = cost * (units_static[*unit].cost / max_cost[MOBILE_CONSTRUCTOR][ai->side-1])
								+ buildspeed * (unitList[*unit-1]->buildSpeed / max_value[MOBILE_CONSTRUCTOR][ai->side-1])
								- urgency * (unitList[*unit-1]->buildTime / max_buildtime[MOBILE_CONSTRUCTOR][ai->side-1]);

					if(my_rating > best_rating)
					{
						best_rating = my_rating;
						builder = *unit;
					}
				}
			}
		}
	}

	if(builder && units_dynamic[builder].under_construction + units_dynamic[builder].requested < 1)
	{
		// build factory if necessary
		if(units_dynamic[builder].constructorsAvailable <= 0)
			BuildFactoryFor(builder);

		if(ai->execute->AddUnitToBuildqueue(builder, 1, true))
		{
			units_dynamic[builder].requested += 1;
			ai->ut->futureBuilders += 1;
			ai->ut->UnitRequested(MOBILE_CONSTRUCTOR);

			// increase number of requested builders of all buildoptions
			for(list<int>::iterator j = units_static[builder].canBuildList.begin(); j != units_static[builder].canBuildList.end(); ++j)
				units_dynamic[*j].constructorsRequested += 1;

			//fprintf(ai->file, "AddAssister() requested: %s %i \n", unitList[builder-1]->humanName.c_str(), units_dynamic[builder].requested);
		}
	}
}


bool AAIBuildTable::IsArty(int id)
{
	if(!unitList[id-1]->weapons.empty())
	{
		float max_range = 0;
		const WeaponDef *longest = 0;

		for(vector<UnitDef::UnitDefWeapon>::const_iterator weapon = unitList[id-1]->weapons.begin(); weapon != unitList[id-1]->weapons.end(); weapon++)
		{
			if(weapon->def->range > max_range)
			{
				max_range = weapon->def->range;
				longest = weapon->def;
			}
		}

		// veh, kbot, hover or ship
		if(unitList[id-1]->movedata)
		{
			if(unitList[id-1]->movedata->moveType == MoveData::Ground_Move)
			{
				if(max_range > cfg->GROUND_ARTY_RANGE)
					return true;
			}
			else if(unitList[id-1]->movedata->moveType == MoveData::Ship_Move)
			{
				if(max_range > cfg->SEA_ARTY_RANGE)
					return true;
			}
			else if(unitList[id-1]->movedata->moveType == MoveData::Hover_Move)
			{
				if(max_range > cfg->HOVER_ARTY_RANGE)
					return true;
			}
		}
		else // aircraft
		{
			if(cfg->AIR_ONLY_MOD)
			{
				if(max_range > cfg->GROUND_ARTY_RANGE)
					return true;
			}
		}

		if(unitList[id-1]->highTrajectoryType == 1)
			return true;
	}

	return false;
}

bool AAIBuildTable::IsScout(int id)
{
	if(unitList[id-1]->speed > cfg->SCOUT_SPEED && !unitList[id-1]->canfly)
		return true;
	else
	{
		for(list<int>::iterator i = cfg->SCOUTS.begin(); i != cfg->SCOUTS.end(); ++i)
		{
			if(*i == id)
				return true;
		}
	}

	return false;
}

bool AAIBuildTable::IsAttacker(int id)
{
	for(list<int>::iterator i = cfg->ATTACKERS.begin(); i != cfg->ATTACKERS.end(); ++i)
	{
		if(*i == id)
			return true;
	}

	return false;
}


bool AAIBuildTable::IsTransporter(int id)
{
	for(list<int>::iterator i = cfg->TRANSPORTERS.begin(); i != cfg->TRANSPORTERS.end(); ++i)
	{
		if(*i == id)
			return true;
	}

	return false;
}

bool AAIBuildTable::AllowedToBuild(int id)
{
	for(list<int>::iterator i = cfg->DONT_BUILD.begin(); i != cfg->DONT_BUILD.end(); ++i)
	{
		if(*i == id)
			return false;
	}

	return true;
}

bool AAIBuildTable::IsMissileLauncher(int def_id)
{
	for(vector<UnitDef::UnitDefWeapon>::const_iterator weapon = unitList[def_id-1]->weapons.begin(); weapon != unitList[def_id-1]->weapons.end(); ++weapon)
	{
		if(weapon->def->stockpile)
			return true;
	}

	return false;
}

bool AAIBuildTable::IsDeflectionShieldEmitter(int def_id)
{
	for(vector<UnitDef::UnitDefWeapon>::const_iterator weapon = unitList[def_id-1]->weapons.begin(); weapon != unitList[def_id-1]->weapons.end(); ++weapon)
	{
		if(weapon->def->isShield)
			return true;
	}

	return false;
}

bool AAIBuildTable::IsCommander(int def_id)
{
	if(units_static[def_id].unit_type & UNIT_TYPE_COMMANDER)
		return true;
	else
		return false;
}

bool AAIBuildTable::IsGround(int def_id)
{
	if(units_static[def_id].movement_type & (MOVE_TYPE_GROUND + MOVE_TYPE_AMPHIB) )
		return true;
	else
		return false;
}

bool AAIBuildTable::IsAir(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_AIR)
		return true;
	else
		return false;
}

bool AAIBuildTable::IsHover(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_HOVER)
		return true;
	else
		return false;
}

bool AAIBuildTable::IsSea(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_SEA)
		return true;
	else
		return false;
}

bool AAIBuildTable::IsStatic(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_STATIC)
		return true;
	else
		return false;
}

bool AAIBuildTable::CanMoveLand(int def_id)
{
	if( !(units_static[def_id].movement_type & MOVE_TYPE_SEA)
		&& !(units_static[def_id].movement_type & MOVE_TYPE_STATIC))
		return true;
	else
		return false;
}

bool AAIBuildTable::CanMoveWater(int def_id)
{
	if( !(units_static[def_id].movement_type & MOVE_TYPE_GROUND) && !(units_static[def_id].movement_type & MOVE_TYPE_STATIC))
		return true;
	else
		return false;
}

bool AAIBuildTable::CanPlacedLand(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_STATIC_LAND)
		return true;
	else
		return false;
}

bool AAIBuildTable::CanPlacedWater(int def_id)
{
	if(units_static[def_id].movement_type & MOVE_TYPE_STATIC_WATER)
		return true;
	else
		return false;
}

float AAIBuildTable::GetEfficiencyAgainst(int unit_def_id, UnitCategory category)
{
	if(category == GROUND_ASSAULT)
		return units_static[unit_def_id].efficiency[0];
	else if(category == AIR_ASSAULT)
		return units_static[unit_def_id].efficiency[1];
	else if(category == HOVER_ASSAULT)
		return units_static[unit_def_id].efficiency[2];
	else if(category == SEA_ASSAULT)
		return units_static[unit_def_id].efficiency[3];
	else if(category == SUBMARINE_ASSAULT)
		return units_static[unit_def_id].efficiency[4];
	else if(category >= STATIONARY_DEF && category <= METAL_MAKER)
		return units_static[unit_def_id].efficiency[5];
	else
		return 0;
}

const char* AAIBuildTable::GetCategoryString(int def_id)
{
	def_id = units_static[def_id].category;

	if(def_id == UNKNOWN)
		return "unknown";
	else if(def_id == GROUND_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "light air assault";
		else
			return "ground assault";
	}
	else if(def_id == AIR_ASSAULT)
		return "air assault";
	else if(def_id == HOVER_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "heavy air assault";
		else
			return "hover assault";
	}
	else if(def_id == SEA_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "super heavy air assault";
		else
			return "sea assault";
	}
	else if(def_id == SUBMARINE_ASSAULT)
		return "submarine assault";
	else if(def_id == MOBILE_CONSTRUCTOR)
		return "builder";

	else if(def_id == SCOUT)
		return "scout";
	else if(def_id == MOBILE_TRANSPORT)
		return "transport";
	else if(def_id == GROUND_ARTY)
	{
		if(cfg->AIR_ONLY_MOD)
			return "mobile artillery";
		else
			return "ground artillery";
	}
	else if(def_id == SEA_ARTY)
		return "naval artillery";
	else if(def_id == HOVER_ARTY)
		return "hover artillery";
	else if(def_id == STATIONARY_DEF)
		return "defence building";
	else if(def_id == STATIONARY_ARTY)
		return "stationary arty";
	else if(def_id == EXTRACTOR)
		return "metal extractor";
	else if(def_id == POWER_PLANT)
		return "power plant";
	else if(def_id == STORAGE)
		return "storage";
	else if(def_id == METAL_MAKER)
		return "metal maker";
	else if(def_id == STATIONARY_CONSTRUCTOR)
		return "stationary constructor";
	else if(def_id == AIR_BASE)
		return "air base";
	else if(def_id == DEFLECTION_SHIELD)
		return "deflection shield";
	else if(def_id == STATIONARY_JAMMER)
		return "stationary jammer";
	else if(def_id == STATIONARY_RECON)
		return "stationary radar/sonar";
	else if(def_id == STATIONARY_LAUNCHER)
		return "stationary launcher";
	else if(def_id == MOBILE_JAMMER)
		return "mobile jammer";
	else if(def_id == MOBILE_LAUNCHER)
		return "mobile launcher";
	else if(def_id== COMMANDER)
		return "commander";

	return "unknown";
}

const char* AAIBuildTable::GetCategoryString2(UnitCategory category)
{
	if(category == UNKNOWN)
		return "unknown";
	else if(category == GROUND_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "light air assault";
		else
			return "ground assault";
	}
	else if(category == AIR_ASSAULT)
		return "air assault";
	else if(category == HOVER_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "heavy air assault";
		else
			return "hover assault";
	}
	else if(category == SEA_ASSAULT)
	{
		if(cfg->AIR_ONLY_MOD)
			return "super heavy air assault";
		else
			return "sea assault";
	}
	else if(category == SUBMARINE_ASSAULT)
		return "submarine assault";
	else if(category == MOBILE_CONSTRUCTOR)
		return "builder";
	else if(category == SCOUT)
		return "scout";
	else if(category == MOBILE_TRANSPORT)
		return "transport";
	else if(category == GROUND_ARTY)
	{
		if(cfg->AIR_ONLY_MOD)
			return "mobile artillery";
		else
			return "ground artillery";
	}
	else if(category == SEA_ARTY)
		return "naval artillery";
	else if(category == HOVER_ARTY)
		return "hover artillery";
	else if(category == STATIONARY_DEF)
		return "defence building";
	else if(category == STATIONARY_ARTY)
		return "stationary arty";
	else if(category == EXTRACTOR)
		return "metal extractor";
	else if(category == POWER_PLANT)
		return "power plant";
	else if(category == STORAGE)
		return "storage";
	else if(category == METAL_MAKER)
		return "metal maker";
	else if(category == STATIONARY_CONSTRUCTOR)
		return "stationary constructor";
	else if(category == AIR_BASE)
		return "air base";
	else if(category == DEFLECTION_SHIELD)
		return "deflection shield";
	else if(category == STATIONARY_JAMMER)
		return "stationary jammer";
	else if(category == STATIONARY_RECON)
		return "stationary radar/sonar";
	else if(category == STATIONARY_LAUNCHER)
		return "stationary launcher";
	else if(category == MOBILE_JAMMER)
		return "mobile jammer";
	else if(category == MOBILE_LAUNCHER)
		return "mobile launcher";
	else if(category == COMMANDER)
		return "commander";

	return "unknown";
}

bool AAIBuildTable::IsStartingUnit(int def_id)
{
	for(int i = 0; i < numOfSides; i++)
	{
		if(startUnits[i] == def_id)
			return true;
	}

	return false;
}

bool AAIBuildTable::IsBuilder(int def_id)
{
	if(units_static[def_id].unit_type & UNIT_TYPE_BUILDER)
		return true;
	else
		return false;
}

bool AAIBuildTable::IsFactory(int def_id)
{
	if(units_static[def_id].unit_type & UNIT_TYPE_FACTORY)
		return true;
	else
		return false;
}

int AAIBuildTable::GetIDOfAssaultCategory(UnitCategory category)
{
	if(category == GROUND_ASSAULT)
		return 0;
	else if(category == AIR_ASSAULT)
		return 1;
	else if(category == HOVER_ASSAULT)
		return 2;
	else if(category == SEA_ASSAULT)
		return 3;
	else if(category == SUBMARINE_ASSAULT)
		return 4;
	else if(category >= STATIONARY_DEF && category <= METAL_MAKER)
		return 5;
	else
		return -1;
}

UnitCategory AAIBuildTable::GetAssaultCategoryOfID(int id)
{
	if(id == 0)
		return GROUND_ASSAULT;
	else if(id == 1)
		return AIR_ASSAULT;
	else if(id == 2)
		return HOVER_ASSAULT;
	else if(id == 3)
		return SEA_ASSAULT;
	else if(id == 4)
		return SUBMARINE_ASSAULT;
	else if(id == 5)
		return STATIONARY_DEF;
	else
		return UNKNOWN;
}

unsigned int AAIBuildTable::GetAllowedMovementTypesForAssister(int building)
{
	// determine allowed movement types
	// alwas allowed: MOVE_TYPE_AIR, MOVE_TYPE_HOVER, MOVE_TYPE_AMPHIB
	unsigned int allowed_movement_types = 22;

	if(units_static[building].movement_type & MOVE_TYPE_STATIC_LAND)
		allowed_movement_types |= MOVE_TYPE_GROUND;
	else
		allowed_movement_types |= MOVE_TYPE_SEA;

	return allowed_movement_types;
}

int AAIBuildTable::DetermineBetterUnit(int unit1, int unit2, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float speed, float range, float cost)
{
	float rating1, rating2;

	rating1 = 0.1f + ground_eff * units_static[unit1].efficiency[0] +  air_eff * units_static[unit1].efficiency[1] +  hover_eff * units_static[unit1].efficiency[2] +  sea_eff * units_static[unit1].efficiency[3] + submarine_eff * units_static[unit1].efficiency[4];
	rating1 /= units_static[unit1].cost;

	rating2 = 0.1f + ground_eff * units_static[unit2].efficiency[0] +  air_eff * units_static[unit2].efficiency[1] +  hover_eff * units_static[unit2].efficiency[2] +  sea_eff * units_static[unit2].efficiency[3] + submarine_eff * units_static[unit2].efficiency[4];
	rating2 /= units_static[unit2].cost;

	if(cost * rating1/rating2 + range * units_static[unit1].range / units_static[unit2].range + speed * unitList[unit1-1]->speed / unitList[unit2-1]->speed  > 0)
		return unit1;
	else
		return unit2;
}
