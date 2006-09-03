#include "AAIBuildTable.h"

#include "AAI.h"

// all the static vars
const UnitDef** AAIBuildTable::unitList = 0;
list<int>* AAIBuildTable::units_of_category[SEA_BUILDER+1];
int AAIBuildTable::aai_instances = 0; 
float* AAIBuildTable::avg_cost[SEA_BUILDER+1]; 
float* AAIBuildTable::avg_buildtime[SEA_BUILDER+1];
float* AAIBuildTable::avg_value[SEA_BUILDER+1];
float* AAIBuildTable::max_cost[SEA_BUILDER+1]; 
float* AAIBuildTable::max_buildtime[SEA_BUILDER+1];
float* AAIBuildTable::max_value[SEA_BUILDER+1];
float* AAIBuildTable::min_cost[SEA_BUILDER+1]; 
float* AAIBuildTable::min_buildtime[SEA_BUILDER+1];
float* AAIBuildTable::min_value[SEA_BUILDER+1];
float**	AAIBuildTable::avg_speed;
float**	AAIBuildTable::min_speed;
float**	AAIBuildTable::max_speed;
float**	AAIBuildTable::group_speed;
float*** AAIBuildTable::mod_usefulness;
UnitTypeStatic* AAIBuildTable::units_static = 0;

AAIBuildTable::AAIBuildTable(IAICallback *cb, AAI* ai)
{
	this->cb = cb;
	this->ai = ai;

	initialized = false;
	numOfUnits = 0;
	units_dynamic = 0;

	numOfSides = cfg->SIDES;
	startUnits = new int[numOfSides];
	sideNames = new string[numOfSides+1];
	sideNames[0] = "Neutral";
	
	const UnitDef *temp;

	for(int i = 0; i < numOfSides; i++)
	{
		temp = cb->GetUnitDef(cfg->START_UNITS[i]);

		if(temp)
			startUnits[i] = temp->id;
		else
		{
			startUnits[i] = -1;

			char c[120];
			sprintf(c, "Error: starting unit %s not found\n", cfg->START_UNITS[i]);
			cb->SendTextMsg(c,0);
			fprintf(ai->file, c);
		}

		sideNames[i+1].assign(cfg->SIDE_NAMES[i]);
	}	

	// add assault categories
	assault_categories.push_back(GROUND_ASSAULT);
	assault_categories.push_back(AIR_ASSAULT);
	assault_categories.push_back(HOVER_ASSAULT);
	assault_categories.push_back(SEA_ASSAULT);
	assault_categories.push_back(SUBMARINE_ASSAULT);

	combat_categories = (int)assault_categories.size() + 1;

	// one more instance
	++aai_instances;

	// only set up static things if first aai intsance is iniatialized
	if(aai_instances == 1)
	{
		// set up the unit lists
		for(int i = 0; i <= SEA_BUILDER; ++i)
		{
			units_of_category[i] = new list<int>[numOfSides];

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

		// set up speed
		avg_speed = new float*[combat_categories];
		max_speed = new float*[combat_categories];
		min_speed = new float*[combat_categories];
		group_speed = new float*[combat_categories];

		for(int i = 0; i < combat_categories; ++i)
		{
			avg_speed[i] = new float[combat_categories];
			max_speed[i] = new float[combat_categories];
			min_speed[i] = new float[combat_categories];
			group_speed[i] = new float[combat_categories];
		}

		// set up mod_usefulness
		mod_usefulness = new float**[assault_categories.size()];

		for(int i = 0; i < assault_categories.size(); ++i)
		{
			mod_usefulness[i] = new float*[cfg->SIDES];

			for(int j = 0; j < cfg->SIDES; ++j)
				mod_usefulness[i][j] = new float[WATER_MAP+1];
		}
	}

	// init eff stats
	avg_eff = new float*[combat_categories];
	max_eff = new float*[combat_categories];
	min_eff = new float*[combat_categories];
	total_eff = new float*[combat_categories];

	for(int i = 0; i < combat_categories; ++i)
	{
		avg_eff[i] = new float[combat_categories];
		max_eff[i] = new float[combat_categories];
		min_eff[i] = new float[combat_categories];
		total_eff[i] = new float[combat_categories];
	}
}

AAIBuildTable::~AAIBuildTable(void)
{
	// one instance less
	--aai_instances;

	if(units_dynamic)
	{
		delete [] units_dynamic;
	}

	// delete common data only if last aai instace has gone
	if(aai_instances == 0)
	{	
		if(units_static)
		{
			// delete eff. 
			for(int i = 1; i <= numOfUnits; ++i)
				delete [] units_static[i].efficiency;

			delete [] units_static;
		}

		delete [] unitList;

		for(int i = 0; i <= SEA_BUILDER; ++i)
			delete [] units_of_category[i];

		for(int i = 0; i <= METAL_MAKER; ++i)
		{
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

		// clean up mod usefulness	
		for(int i = 0; i < assault_categories.size(); ++i)
		{
			for(int j = 0; j < cfg->SIDES; ++j)
				delete [] mod_usefulness[i][j];
		
			delete [] mod_usefulness[i];
		}

		delete [] mod_usefulness;
	}

	delete [] sideNames;
	delete [] startUnits;

	// delete efficiency stats
	for(int i = 0; i < combat_categories; ++i)
	{
		delete [] avg_eff[i];
		delete [] max_eff[i];
		delete [] min_eff[i];
		delete [] total_eff[i];
	}

	delete [] avg_eff;
	delete [] max_eff;
	delete [] min_eff;
	delete [] total_eff;
}

void AAIBuildTable::Init()
{
	float max_cost = 0, min_cost = 1000000, eff;

	// initialize random numbers generator
	srand ( time(NULL) );

	// get number of units and alloc memory for unit list
	numOfUnits = cb->GetNumUnitDefs();

	// one more than needed because 0 is dummy object (so UnitDef->id can be used to adress that unit in the array) 
	units_dynamic = new UnitTypeDynamic[numOfUnits+1];

	for(int i = 0; i <= numOfUnits; i++)
	{
		units_dynamic[i].active = 0;
		units_dynamic[i].requested = 0;
		units_dynamic[i].builderAvailable = false;
		units_dynamic[i].builderRequested = false;
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
		units_static = new UnitTypeStatic[numOfUnits+1];

		// temmporary to sort air uni9t in air only mods
		list<int> *temp_list;
		temp_list = new list<int>[numOfSides];

		units_static[0].id = 0;
		units_static[0].side = 0;

		// add units to buildtable
		for(int i = 1; i <= numOfUnits; i++)
		{
			// get id 
			units_static[i].id = unitList[i-1]->id;
			units_static[i].cost = (unitList[i-1]->metalCost + (unitList[i-1]->energyCost / 75)) / 10;

			if(units_static[i].cost > max_cost)
				max_cost = units_static[i].cost;

			if(units_static[i].cost < min_cost)
				min_cost = units_static[i].cost;
			
			units_static[i].builder_cost = 0; // will be added later when claculating the buildtree
			units_static[i].builder_energy_cost = 0;
			units_static[i].builder_metal_cost = 0;
	
			// side has not been assigned - will be done later
			units_static[i].side = 0;
			units_static[i].range = 0;
			
			units_static[i].category = UNKNOWN;
		
			// get build options
			for(map<int, string>::const_iterator j = unitList[i-1]->buildOptions.begin(); j != unitList[i-1]->buildOptions.end(); j++)
			{
				units_static[i].canBuildList.push_back(cb->GetUnitDef(j->second.c_str())->id);
			}
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
				units_static[i].efficiency = new float[combat_categories];

				eff = 5 + 20 * (units_static[i].cost - min_cost)/(max_cost - min_cost);
				//eff = 10;
				
				for(int k = 0; k < combat_categories; ++k)
					units_static[i].efficiency[k] = eff;
			}
			else
			{
				units_static[i].range = 0;
				
				// get memory for eff
				units_static[i].efficiency = new float[combat_categories];
				
				for(int k = 0; k < combat_categories; ++k)
					units_static[i].efficiency[k] = -1;
			}
		}

		// add unit to different groups
		for(int i = 1; i <= numOfUnits; i++)
		{
			// first we can calculate builder_cost
			units_static[i].builder_cost = (units_static[i].builder_metal_cost + (units_static[i].builder_energy_cost / 75)) / 10;

			// filter out neutral units
			if(!units_static[i].side)
			{
			}
			// get scouts
			else if(IsScout(i))
			{
				if(unitList[i-1]->movedata)
				{
					if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move)
					{
						units_of_category[GROUND_SCOUT][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = GROUND_SCOUT;
					}
					else if(unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
					{
						units_of_category[HOVER_SCOUT][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = HOVER_SCOUT;
					}
					else if(unitList[i-1]->movedata->moveType == MoveData::Ship_Move)
					{	
						units_of_category[SEA_SCOUT][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = SEA_SCOUT;
					}
				}
				else if(unitList[i-1]->canfly)
				{
					units_of_category[AIR_SCOUT][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = AIR_SCOUT;
				}
			}
			// check if builder or factory
			else if(unitList[i-1]->buildOptions.size() > 0)
			{
				// check if ground or sea unit
				if(unitList[i-1]->movedata)
				{
					if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move)
					{
						units_of_category[GROUND_BUILDER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = GROUND_BUILDER;
					}
					if(unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
					{
						units_of_category[HOVER_BUILDER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = HOVER_BUILDER;
					}
					if(unitList[i-1]->movedata->moveType == MoveData::Ship_Move || unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
					{
						units_of_category[SEA_BUILDER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = SEA_BUILDER;
					}
				}
				else // must be factory or aircraft
				{ 
					// check if aircraft
					if(unitList[i-1]->canfly)
					{
						units_of_category[AIR_BUILDER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = AIR_BUILDER;
					}
					else
					{
						// ground factory or sea factory
						if(unitList[i-1]->minWaterDepth <= 0)
						{
							units_of_category[GROUND_FACTORY][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = GROUND_FACTORY;
						}
						else
						{
							units_of_category[SEA_FACTORY][units_static[i].side-1].push_back(unitList[i-1]->id);
							units_static[i].category = SEA_FACTORY;
						}
					}
				}
			}
			// no builder or factory
			// check if other building
			else if(!unitList[i-1]->movedata && !unitList[i-1]->canfly)    
			{
				// check if repair pad
				if(unitList[i-1]->isAirBase)
				{
					units_of_category[AIR_BASE][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = AIR_BASE;
				}
				// check if extractor
				else if(unitList[i-1]->extractsMetal)
				{
					units_of_category[EXTRACTOR][units_static[i].side-1].push_back(unitList[i-1]->id);
					units_static[i].category = EXTRACTOR;
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
				else if(!unitList[i-1]->weapons.empty() && unitList[i-1]->extractsMetal == 0)
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
					if(unitList[i-1]->weapons.empty())
					{
						units_of_category[STATIONARY_RECON][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = STATIONARY_RECON;
					}
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
					if(unitList[i-1]->weapons.empty())
					{
						units_of_category[STORAGE][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = STORAGE;
					}
				}
				else if(unitList[i-1]->metalStorage > cfg->MIN_METAL_STORAGE && !unitList[i-1]->extractsMetal)
				{
					if(unitList[i-1]->weapons.empty())
					{
						units_of_category[STORAGE][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = STORAGE;
					}
				}
				else if(unitList[i-1]->makesMetal > 0)
				{
					if(unitList[i-1]->weapons.empty())
					{
						units_of_category[METAL_MAKER][units_static[i].side-1].push_back(unitList[i-1]->id);
						units_static[i].category = METAL_MAKER;
					}
				}
			}
			// units that are not builders
			else if(unitList[i-1]->movedata)
			{
				// ground units
				if(unitList[i-1]->movedata->moveType == MoveData::Ground_Move || unitList[i-1]->movedata->moveType == MoveData::Hover_Move)
				{
					// units with weapons
					if(!unitList[i-1]->weapons.empty())
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
								units_of_category[MOBILE_ARTY][units_static[i].side-1].push_back(unitList[i-1]->id);
								units_static[i].category = MOBILE_ARTY;
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
						else if(GetMaxDamage(unitList[i-1]->id) > 1)
						{
							if(unitList[i-1]->categoryString.find("UNDERWATER") != string::npos)
							{
								units_of_category[SUBMARINE_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
								units_static[i].category = SUBMARINE_ASSAULT;
							}
							else
							{
								units_of_category[SEA_ASSAULT][units_static[i].side-1].push_back(unitList[i-1]->id);
								units_static[i].category = SEA_ASSAULT;
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
				if(!unitList[i-1]->weapons.empty() && GetMaxDamage(unitList[i-1]->id) > 1)
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

		// set special eff for air units
		if(cfg->AIR_ONLY_MOD)
		{
		}
		else
		{
			UnitCategory cat;
			float eff;

			for(int i = 0; i <= numOfUnits; ++i)
			{
				cat = units_static[i].category;
				eff = 1.5 + 6 * (units_static[i].cost - min_cost)/(max_cost - min_cost);

				if(cat == AIR_ASSAULT)
				{
					for(int k = 0; k < combat_categories; ++k)
						units_static[i].efficiency[k] = eff;		
				}
				else if(cat == GROUND_ASSAULT || cat == SEA_ASSAULT || cat == HOVER_ASSAULT || cat == MOBILE_ARTY || cat == STATIONARY_DEF)
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

		// set default map_usefulness of units
		for(int i = 0; i < cfg->SIDES; ++i)
		{
			if(cfg->AIR_ONLY_MOD)
			{
				
				if(units_of_category[GROUND_ASSAULT][i].size() > 0)
				{
					mod_usefulness[0][i][0] = units_of_category[GROUND_ASSAULT][i].size() * 100.0 
						/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[AIR_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size());
				}
				else 
					mod_usefulness[0][i][0] = 0;

				if(units_of_category[AIR_ASSAULT][i].size() > 0)
				{
					mod_usefulness[1][i][0] = units_of_category[HOVER_ASSAULT][i].size() * 100.0 
						/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[AIR_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size());
				}
				else 
					mod_usefulness[1][i][0] = 0;

				if(units_of_category[HOVER_ASSAULT][i].size() > 0)
				{
					mod_usefulness[2][i][0] = units_of_category[HOVER_ASSAULT][i].size() * 100.0 
						/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[AIR_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size());
				}
				else 
					mod_usefulness[2][i][0] = 0;

				if(units_of_category[SEA_ASSAULT][i].size() > 0)
				{
					mod_usefulness[3][i][0] = units_of_category[SEA_ASSAULT][i].size() * 100.0 
						/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[AIR_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size());
				}
				else 
					mod_usefulness[3][i][0] = 0;

				mod_usefulness[4][i][0] = 0;

				if(mod_usefulness[0][i][0] > 90)
					mod_usefulness[0][i][0] = 90;
				else if(mod_usefulness[0][i][0] < 10 && mod_usefulness[0][i][0] > 0)
					mod_usefulness[0][i][0] = 10;

				if(mod_usefulness[1][i][0] > 90)
					mod_usefulness[1][i][0] = 90;
				else if(mod_usefulness[1][i][0] < 10 && mod_usefulness[1][i][0] > 0)
					mod_usefulness[1][i][0] = 10;
			
				if(mod_usefulness[2][i][0] > 90)
					mod_usefulness[2][i][0] = 90;
				else if(mod_usefulness[2][i][0] < 10 && mod_usefulness[2][i][0] > 0)
					mod_usefulness[2][i][0] = 10;
				
				if(mod_usefulness[3][i][0] > 90)
					mod_usefulness[3][i][0] = 90;
				else if(mod_usefulness[3][i][0] < 10 && mod_usefulness[3][i][0] > 0)
					mod_usefulness[3][i][0] = 10;
				
				for(int map = 1; map <= WATER_MAP; ++map)
				{
					mod_usefulness[0][i][map] = mod_usefulness[0][i][0];
					mod_usefulness[1][i][map] = mod_usefulness[1][i][0];
					mod_usefulness[2][i][map] = mod_usefulness[2][i][0];
					mod_usefulness[3][i][map] = mod_usefulness[3][i][0];
					mod_usefulness[4][i][map] = mod_usefulness[4][i][0];
				}
			}
			else
			{
			for(int map = 0; map < WATER_MAP +1; ++map)
			{
				// ground units
				if(units_of_category[GROUND_ASSAULT][i].size() > 0)
				{
					if(map == LAND_MAP)
						mod_usefulness[0][i][map] = units_of_category[GROUND_ASSAULT][i].size() * 100.0
							/ ( units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size());
					else if(map == LAND_WATER_MAP)
						mod_usefulness[0][i][map] = units_of_category[GROUND_ASSAULT][i].size() * 100.0 
							/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else if(map == WATER_MAP)
						mod_usefulness[0][i][map] = 0;
					else
						mod_usefulness[0][i][map] = 0;
				}	
				else
					mod_usefulness[0][i][map] = 0;
			
				// air units
				mod_usefulness[1][i][map] = 100.0 / (float)cfg->AIRCRAFT_RATE; // not used yet
				
				// hover
				if(units_of_category[HOVER_ASSAULT][i].size() > 0)
				{
					if(map == LAND_MAP)
						mod_usefulness[2][i][map] = units_of_category[HOVER_ASSAULT][i].size() * 100.0
							/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size());
					else if(map == LAND_WATER_MAP)
						mod_usefulness[2][i][map] = units_of_category[HOVER_ASSAULT][i].size() * 100.0 
							/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else if(map == WATER_MAP)
						mod_usefulness[2][i][map] = units_of_category[HOVER_ASSAULT][i].size() * 100.0
							/(units_of_category[SEA_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else
						mod_usefulness[2][i][map] = 0;
				}
				else
					mod_usefulness[2][i][map] = 0;
				
				// sea
				if(units_of_category[SEA_ASSAULT][i].size() > 0)
				{
					if(map == LAND_WATER_MAP)
						mod_usefulness[3][i][map] = units_of_category[SEA_ASSAULT][i].size() * 100.0 
							/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else if(map == WATER_MAP)
						mod_usefulness[3][i][map] = units_of_category[SEA_ASSAULT][i].size() * 100.0
							/(units_of_category[SEA_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else
						mod_usefulness[3][i][map] = 0;
				}
				else 
					mod_usefulness[3][i][map] = 0;

				// these categories are not used, only to avoid crashes when units killed buildings etc.
				if(units_of_category[SUBMARINE_ASSAULT][i].size() > 0)
				{
					if(map == LAND_WATER_MAP)
						mod_usefulness[4][i][map] = units_of_category[SUBMARINE_ASSAULT][i].size() * 100.0 
							/(units_of_category[GROUND_ASSAULT][i].size() + units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else if(map == WATER_MAP)
						mod_usefulness[4][i][map] = units_of_category[SUBMARINE_ASSAULT][i].size() * 100.0
							/( units_of_category[HOVER_ASSAULT][i].size() + units_of_category[SEA_ASSAULT][i].size() + units_of_category[SUBMARINE_ASSAULT][i].size());
					else
						mod_usefulness[4][i][map] = 0;
				}
				else 
					mod_usefulness[4][i][map] = 0;

				// check for too high/low values
				if(map == LAND_MAP)
				{
					if(mod_usefulness[0][i][map] > 90)
						mod_usefulness[0][i][map] = 90;
					else if(mod_usefulness[0][i][map] < 10)
						mod_usefulness[0][i][map] = 10;
			
					if(mod_usefulness[2][i][map] > 90)
						mod_usefulness[2][i][map] = 90;
					else if(mod_usefulness[2][i][map] < 10)
						mod_usefulness[2][i][map] = 10;
				}
				else if(map == LAND_WATER_MAP)
				{
					if(mod_usefulness[0][i][map] > 90)
						mod_usefulness[0][i][map] = 90;
					else if(mod_usefulness[0][i][map] < 10)
						mod_usefulness[0][i][map] = 10;
			
					if(mod_usefulness[2][i][map] > 90)
						mod_usefulness[2][i][map] = 90;
					else if(mod_usefulness[2][i][map] < 10)
						mod_usefulness[2][i][map] = 10;
				
					if(mod_usefulness[3][i][map] > 90)
						mod_usefulness[3][i][map] = 90;
					else if(mod_usefulness[3][i][map] < 10)
						mod_usefulness[3][i][map] = 10;
			
					if(mod_usefulness[4][i][map] > 90)
						mod_usefulness[4][i][map] = 90;
					else if(mod_usefulness[4][i][map] < 10)
						mod_usefulness[4][i][map] = 10;
				}
				else if(map == WATER_MAP)
				{
					if(mod_usefulness[2][i][map] > 90)
						mod_usefulness[2][i][map] = 90;
					else if(mod_usefulness[2][i][map] < 10)
						mod_usefulness[2][i][map] = 10;
				
					if(mod_usefulness[3][i][map] > 90)
						mod_usefulness[3][i][map] = 90;
					else if(mod_usefulness[3][i][map] < 10)
						mod_usefulness[3][i][map] = 10;
			
					if(mod_usefulness[4][i][map] > 90)
						mod_usefulness[4][i][map] = 90;
					else if(mod_usefulness[4][i][map] < 10)
						mod_usefulness[4][i][map] = 10;
				}
			}
			}
		}
		
		// save to cache file
		SaveBuildTable();

		cb->SendTextMsg("New BuildTable has been created",0);
	}

	// only once
	if(aai_instances == 1)
	{
		for(int s = 0; s < numOfSides; ++s)
			UpdateMinMaxAvgEfficiency(s);

		DebugPrint();
	}

	// buildtable is initialized
	initialized = true;
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
		for(list<int>::iterator i = units_of_category[METAL_MAKER][s].begin(); i != units_of_category[METAL_MAKER][s].end(); i++)
			units_static[*i].efficiency[0] = unitList[*i-1]->makesMetal/(unitList[*i-1]->energyUpkeep+1);

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

		float average_metal, average_energy;
		// precache average metal and energy consumption of factories
		for(list<int>::iterator i = units_of_category[GROUND_FACTORY][s].begin(); i != units_of_category[GROUND_FACTORY][s].end(); i++)
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
		
		for(list<int>::iterator i = units_of_category[SEA_FACTORY][s].begin(); i != units_of_category[SEA_FACTORY][s].end(); i++)
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
		for(list<int>::iterator i = units_of_category[STATIONARY_ARTY][s].begin(); i != units_of_category[STATIONARY_ARTY][s].end(); i++)
		{
			units_static[*i].efficiency[1] = GetMaxRange(*i);
			units_static[*i].efficiency[0] = 1 + units_static[*i].cost/100.0;
		}

		// precache costs and buildtime
		float buildtime;

		for(int i = 1; i <= SEA_BUILDER; ++i)
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
				buildtime = unitList[*unit-1]->buildTime/256.0;

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
		min_value[STATIONARY_ARTY][s] = 10000;
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

		// precache stationary defences weapon range
		min_value[STATIONARY_DEF][s] = 10000;
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

		// precache unit speed and weapons range
		if(cfg->AIR_ONLY_MOD)
		{
			min_value[AIR_ASSAULT][s] = 10000;
			max_value[AIR_ASSAULT][s] = 0;

			for(list<int>::iterator unit = units_of_category[AIR_ASSAULT][s].begin(); unit != units_of_category[AIR_ASSAULT][s].end(); ++unit)
			{
				if(unitList[*unit-1]->speed < min_value[AIR_ASSAULT][s])
					min_value[AIR_ASSAULT][s] = unitList[*unit-1]->speed;
	
				if(unitList[*unit-1]->speed > max_value[AIR_ASSAULT][s])
					max_value[AIR_ASSAULT][s] = unitList[*unit-1]->speed;
			}

			// now calculate average speed-range and safe minum speed in max_value
			avg_value[AIR_ASSAULT][s] = (1 + max_value[AIR_ASSAULT][s] - min_value[AIR_ASSAULT][s]) / ((float)cfg->UNIT_SPEED_SUBGROUPS);
		}
		else
		{
			min_value[GROUND_ASSAULT][s] = 10000;
			max_value[GROUND_ASSAULT][s] = 0;	
			min_value[SEA_ASSAULT][s] = 10000;
			max_value[SEA_ASSAULT][s] =	0;

			for(list<int>::iterator unit = units_of_category[GROUND_ASSAULT][s].begin(); unit != units_of_category[GROUND_ASSAULT][s].end(); ++unit)
			{
				if(unitList[*unit-1]->speed < min_value[GROUND_ASSAULT][s])
					min_value[GROUND_ASSAULT][s] = unitList[*unit-1]->speed;
	
				if(unitList[*unit-1]->speed > max_value[GROUND_ASSAULT][s])
					max_value[GROUND_ASSAULT][s] = unitList[*unit-1]->speed;
			}

			for(list<int>::iterator unit = units_of_category[SEA_ASSAULT][s].begin(); unit != units_of_category[SEA_ASSAULT][s].end(); ++unit)
			{
				if(unitList[*unit-1]->speed < min_value[SEA_ASSAULT][s] )
					min_value[SEA_ASSAULT][s] = unitList[*unit-1]->speed;

				if(unitList[*unit-1]->speed > max_value[SEA_ASSAULT][s])
					max_value[SEA_ASSAULT][s] = unitList[*unit-1]->speed;
			}

			// now calculate average speed-range and safe minum speed in max_value
			avg_value[GROUND_ASSAULT][s] = (1 + max_value[GROUND_ASSAULT][s] - min_value[GROUND_ASSAULT][s]) / ((float)cfg->UNIT_SPEED_SUBGROUPS);
			avg_value[SEA_ASSAULT][s] = (1 + max_value[SEA_ASSAULT][s] - min_value[SEA_ASSAULT][s]) / ((float)cfg->UNIT_SPEED_SUBGROUPS);
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
		UnitCategory cat = units_static[def_id].category;
		
		if(cat == GROUND_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[0][1] > 4 * (units_static[def_id].efficiency[0] / max_eff[0][0]))
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == AIR_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[1][1] > 4 * (units_static[def_id].efficiency[5] / max_eff[1][5]) )
				return ANTI_AIR_UNIT;
			else
				return BOMBER_UNIT;
		}
		else if(cat == HOVER_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[2][1] > 4 * (units_static[def_id].efficiency[0] / max_eff[2][0]))
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == SEA_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[3][1] > 4 * (units_static[def_id].efficiency[3] / max_eff[3][3]) )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == SUBMARINE_ASSAULT)
		{
			if( units_static[def_id].efficiency[1] / max_eff[4][1] > 4 * (units_static[def_id].efficiency[3] / max_eff[4][3]) )
				return ANTI_AIR_UNIT;
			else
				return ASSAULT_UNIT;
		}
		else if(cat == MOBILE_ARTY)
		{
			return ARTY_UNIT;
		}
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

int AAIBuildTable::GetPowerPlant(int side, float cost, float urgency, float power, float current_energy, float comparison, bool water, bool geo, bool canBuild)
{
	int best_unit = 0;  
	float buildtime;
	float best_ranking = -10000, my_ranking;

	float max_cost = this->max_cost[POWER_PLANT][side-1];

	float max_buildtime = this->max_buildtime[POWER_PLANT][side-1];

	float max_power = this->max_value[POWER_PLANT][side-1];

	current_energy /= 1.7;

	//debug
	//fprintf(ai->file, "Selecting power plant:     power %f    cost %f    urgency %f   energy %f \n", power, cost, urgency, current_energy);

	// TODO: improve ranking algorithm, use fuzzy logic?!?
	for(list<int>::iterator i = units_of_category[POWER_PLANT][side-1].begin(); i != units_of_category[POWER_PLANT][side-1].end(); ++i)
	{
		// get buildtime
		buildtime = unitList[*i-1]->buildTime/256;

		if(canBuild && !units_dynamic[*i].builderAvailable)
			my_ranking = -10000;
		else if(!geo && unitList[*i-1]->needGeo)
			my_ranking = -10000;
		else if(!water && unitList[*i-1]->minWaterDepth <= 0)
		{
			my_ranking = (power * max_cost * units_static[*i].efficiency[0]) / (max_power * cost * units_static[*i].cost);
			my_ranking *= (1 + (comparison * exp(- pow((units_static[*i].efficiency[0] - current_energy)/current_energy, 2.0f))));
			my_ranking -= urgency * buildtime / max_buildtime;
		
			//fprintf(ai->file, "%-20s: %f\n", unitList[*i-1]->humanName.c_str(), my_ranking);
		}
		// water building needed
		else if(water && unitList[*i-1]->minWaterDepth > 0)
		{
			// calculate current ranking
			if(unitList[*i-1]->tidalGenerator)
			{
				my_ranking = (power * max_cost * cb->GetTidalStrength()) / (max_power * cost * units_static[*i].cost);
				my_ranking *= (1 + (comparison * exp(- pow((units_static[*i].efficiency[0] - current_energy)/current_energy, 2.0f))));
				my_ranking -= urgency * buildtime / max_buildtime;
			}
			else
			{
				my_ranking = (power * max_cost * units_static[*i].efficiency[0]) / (max_power * cost * units_static[*i].cost);
				my_ranking *= (1 + (comparison * exp(- pow((units_static[*i].efficiency[0] - current_energy)/current_energy, 2.0f))));
				my_ranking -= urgency * buildtime / max_buildtime;
			}
		}
		else
			my_ranking = 0;

		if(my_ranking)
		{	
			//fprintf(ai->file, "%-20s %f\n", unitList[*i-1]->humanName.c_str(), my_ranking); 

			if(my_ranking > best_ranking)
			{
				best_ranking = my_ranking;
				best_unit = *i;
			}
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

	// TODO: improve ranking algorithm, use fuzzy logic?!?
	for(list<int>::iterator i = units_of_category[EXTRACTOR][side].begin(); i != units_of_category[EXTRACTOR][side].end(); i++)
	{
		if(canBuild && !units_dynamic[*i].builderAvailable)
			my_ranking = -10000;
		// check if under water or ground
		else if((!water) && unitList[*i-1]->minWaterDepth <= 0)
		{
			my_ranking = effiency * (unitList[*i-1]->extractsMetal - avg_value[EXTRACTOR][side]) / max_value[EXTRACTOR][side] 
						+ cost * (avg_cost[EXTRACTOR][side] - units_static[*i].cost)/max_cost[EXTRACTOR][side];

			if(armed && !unitList[*i-1]->weapons.empty())
				my_ranking += 1;
		}
		// water = true and building under water
		else if(water && unitList[*i-1]->minWaterDepth > 0)
		{
			my_ranking = effiency * (unitList[*i-1]->extractsMetal - avg_value[EXTRACTOR][side]) / max_value[EXTRACTOR][side] 
						+ cost * (avg_cost[EXTRACTOR][side] - units_static[*i].cost)/max_cost[EXTRACTOR][side];

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
			if(unitList[*mex-1]->xsize * unitList[*mex-1]->ysize > biggest_yard_map)
			{
				biggest_yard_map = unitList[*mex-1]->xsize * unitList[*mex-1]->ysize;
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
		if(canBuild && !units_dynamic[*storage].builderAvailable)
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
		if(canBuild && !units_dynamic[*maker].builderAvailable)
			my_rating = 0;
		else if(!water && unitList[*maker-1]->minWaterDepth <= 0)
		{
			my_rating = (pow(efficiency * units_static[*maker].efficiency[0], 1.4f) + pow(metal * unitList[*maker-1]->makesMetal, 1.6f))
				/(pow(cost * units_static[*maker].cost, 1.4f) + pow(urgency * unitList[*maker-1]->buildTime, 1.4f));
		}
		else if(water && unitList[*maker-1]->minWaterDepth > 0)
		{
			my_rating = (pow(efficiency * units_static[*maker].efficiency[0], 1.4f) + pow(metal * unitList[*maker-1]->makesMetal, 1.6f))
				/(pow(cost * units_static[*maker].cost, 1.4f) + pow(urgency * unitList[*maker-1]->buildTime, 1.4f));
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

int AAIBuildTable::GetDefenceBuilding(int side, float efficiency, float cost, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float urgency, float range, int randomness, bool water, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_defence = 0;
	float defences = 0;
	
	float max_cost = this->max_cost[STATIONARY_DEF][side-1];
	
	float max_buildtime = this->max_buildtime[STATIONARY_DEF][side-1];

	float max_range = max_value[STATIONARY_DEF][side-1];

	float eff;

	float max_eff = ground_eff * this->max_eff[5][0] + air_eff * this->max_eff[5][1] + hover_eff * this->max_eff[5][2] 
					+ sea_eff * this->max_eff[5][3] + submarine_eff * this->max_eff[5][4];	

	if(max_eff <= 0)
		max_eff = 1;

	UnitTypeStatic *unit;

	// calculate rating
	for(list<int>::iterator defence = units_of_category[STATIONARY_DEF][side-1].begin(); defence != units_of_category[STATIONARY_DEF][side-1].end(); defence++)
	{
		// check if water 
		if(canBuild && !units_dynamic[*defence].builderAvailable)
			my_ranking = 0;
		else if(!water && unitList[*defence-1]->minWaterDepth <= 0)
		{
			unit = &units_static[*defence];

			eff = ground_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]  
					+ sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4];

			my_ranking = pow(eff, efficiency) * max_cost / (max_eff * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range  + urgency * unitList[*defence-1]->buildTime / max_buildtime);

			my_ranking *= (1 + 0.05 * ((float)(rand()%randomness)));
		}
		else if(water && unitList[*defence-1]->minWaterDepth > 0)
		{
			unit = &units_static[*defence];

			eff = ground_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]  
					+ sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4];

			my_ranking = pow(eff, efficiency) * max_cost / (max_eff * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range  + urgency * unitList[*defence-1]->buildTime / max_buildtime);

			my_ranking *= (1 + 0.05 * ((float)(rand()%randomness)));
		}
		else
			my_ranking = 0;	

		if(my_ranking > best_ranking)
		{
			best_ranking = my_ranking;
			best_defence = *defence;
		}
	}

	
	// debug
	/*if(best_defence)
		fprintf(ai->file, "Selected: %s \n", unitList[best_defence-1]->humanName.c_str());
	else
		fprintf(ai->file, "Selected: none\n");*/

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

	for(list<int>::iterator airbase = units_of_category[STATIONARY_ARTY][side-1].begin(); airbase != units_of_category[STATIONARY_ARTY][side-1].end(); ++airbase)
	{
		// check if water 
		if(canBuild && !units_dynamic[*airbase].builderAvailable)
			my_ranking = 0;
		else if(!water && unitList[*airbase-1]->minWaterDepth <= 0)
		{
			my_ranking =  100 / (cost * units_static[*airbase].cost); 
		}
		else if(water && unitList[*airbase-1]->minWaterDepth > 0)
		{
			my_ranking =  100 / (cost * units_static[*airbase].cost);
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
		if(canBuild && !units_dynamic[*arty].builderAvailable)
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
			if(canBuild && !units_dynamic[*i].builderAvailable)
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
		if(canBuild && !units_dynamic[*i].builderAvailable)
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


int AAIBuildTable::GetScout(int side, float speed, float los, float cost, UnitCategory category, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_scout = 0;

	for(list<int>::iterator i = units_of_category[category][side-1].begin(); i != units_of_category[category][side-1].end(); i++)
	{
		if(!canBuild || (canBuild && units_dynamic[*i].builderAvailable))
		{
			my_ranking = (speed * unitList[*i-1]->speed  + los * unitList[*i-1]->losRadius) / (cost * units_static[*i].cost);
			my_ranking /= (float)(1 + cfg->MAX_SCOUTS + units_dynamic[*i].active + units_dynamic[*i].requested);
			my_ranking *= (1 + 0.05 * ((float)(rand()%randomness)));

			if(my_ranking > best_ranking)
			{
				best_ranking = my_ranking;
				best_scout = *i;
			}
		}
	}

	return best_scout;
}

int AAIBuildTable::GetScout(int side, float speed, float los, float cost, UnitCategory category1, UnitCategory category2, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_scout = 0;

	for(list<int>::iterator i = units_of_category[category1][side-1].begin(); i != units_of_category[category1][side-1].end(); i++)
	{
		if(!canBuild || (canBuild && units_dynamic[*i].builderAvailable))
		{
			my_ranking = (speed * unitList[*i-1]->speed  + los * unitList[*i-1]->losRadius) / (cost * units_static[*i].cost);
			my_ranking /= (float)(1 + cfg->MAX_SCOUTS + units_dynamic[*i].active + units_dynamic[*i].requested);
			my_ranking *= (1 + 0.05 * ((float)(rand()%randomness)));

			if(my_ranking > best_ranking)
			{
				best_ranking = my_ranking;
				best_scout = *i;
			}
		}
	}
	for(list<int>::iterator i = units_of_category[category2][side-1].begin(); i != units_of_category[category2][side-1].end(); i++)
	{	
		if(!canBuild || (canBuild && units_dynamic[*i].builderAvailable))
		{
			my_ranking = (((float)(rand()%randomness)+1) * 10.0) / (cost * (units_static[*i].cost + units_static[*i].builder_cost));

			if(my_ranking > best_ranking)
			{
				best_ranking = my_ranking;
				best_scout = *i;
			}
		}
	}

	return best_scout;
}


int AAIBuildTable::GetBuilder(int unit_id)
{
	return *(units_static[unit_id].builtByList.begin());
}

int AAIBuildTable::GetBuilder(int unit_id, UnitMoveType moveType)
{
	if(moveType == GROUND)
	{
		for(list<int>::iterator builder = units_static[unit_id].builtByList.begin(); builder != units_static[unit_id].builtByList.end(); builder++)
		{
			if(unitList[*builder-1]->movedata->moveType == MoveData::Ground_Move)
					return *builder;
		}
	}
	else if(moveType == AIR)
	{
		for(list<int>::iterator builder = units_static[unit_id].builtByList.begin(); builder != units_static[unit_id].builtByList.end(); builder++)
		{
			if(unitList[*builder-1]->canfly)
				return *builder;
		}
	}
	else if(moveType == SEA)
	{
		for(list<int>::iterator builder = units_static[unit_id].builtByList.begin(); builder != units_static[unit_id].builtByList.end(); builder++)
		{
			if(unitList[*builder-1]->movedata->moveType == MoveData::Ship_Move)
				return *builder;
		}
	}
	else if(moveType = HOVER)
	{
		for(list<int>::iterator builder = units_static[unit_id].builtByList.begin(); builder != units_static[unit_id].builtByList.end(); builder++)
		{
			if(unitList[*builder-1]->movedata->moveType == MoveData::Hover_Move)
				return *builder;
		}
	}
	else
		return 0;

	// no builder of that type found
	return 0;
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

int AAIBuildTable::GetGroundAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	int best_unit = 0;
	
	float max_cost = this->max_cost[GROUND_ASSAULT][side-1];
	float max_range = max_value[GROUND_ASSAULT][side-1];
	float max_speed = this->max_speed[0][side-1];

	double total_efficiency = gr_eff * max_eff[0][0] + air_eff * max_eff[0][1] + hover_eff * max_eff[0][2] 
							+ sea_eff * max_eff[0][3] + stat_eff * max_eff[0][5];

	if(total_efficiency <= 0)
		total_efficiency = 1;
	
	UnitTypeStatic *unit;

	// debug
	//fprintf(ai->file, "\nSelection: (ground %f)  (air %f) \n", gr_eff, air_eff);

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[GROUND_ASSAULT][side-1].begin(); i != units_of_category[GROUND_ASSAULT][side-1].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].builderAvailable)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else if(!canBuild)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else 
		{
			my_ranking = 0;
		}

		//debug
		//char c[90];
		//sprintf(c, "%s %f\n", unitList[*i-1]->humanName.c_str(), my_ranking);
		//fprintf(ai->file, c);

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

	// debug
	/*if(best_unit)
		fprintf(ai->file, "Selected: %s \n", unitList[best_unit-1]->humanName.c_str());
	else
		fprintf(ai->file, "Selected: none\n");*/

	return best_unit;
}

int AAIBuildTable::GetHoverAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	float best_unit = 0;
	
	float max_cost = this->max_cost[HOVER_ASSAULT][side-1];
	float max_range = max_value[HOVER_ASSAULT][side-1];
	float max_speed = this->max_speed[2][side-1];

	double total_efficiency = gr_eff * max_eff[2][0] + air_eff * max_eff[2][1] + hover_eff * max_eff[2][2] 
							+ sea_eff * max_eff[2][3] + stat_eff * max_eff[2][5];

	if(total_efficiency <= 0)
		total_efficiency = 1;
	
	UnitTypeStatic *unit;

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[HOVER_ASSAULT][side-1].begin(); i != units_of_category[HOVER_ASSAULT][side-1].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].builderAvailable)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else if(!canBuild)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
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


int AAIBuildTable::GetAirAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	float best_unit = 0;
	
	float max_cost = this->max_cost[AIR_ASSAULT][side-1];
	float max_range = max_value[AIR_ASSAULT][side-1];
	float max_speed = this->max_speed[1][side-1];

	double total_efficiency = gr_eff * max_eff[1][0] + air_eff * max_eff[1][1] + hover_eff * max_eff[1][2] 
							+ sea_eff * max_eff[1][3] + stat_eff * max_eff[1][5];
	
	UnitTypeStatic *unit;

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[AIR_ASSAULT][side-1].begin(); i != units_of_category[AIR_ASSAULT][side-1].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].builderAvailable)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else if(!canBuild)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
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

int AAIBuildTable::GetSeaAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	float best_unit = 0;
	
	float max_cost = this->max_cost[SEA_ASSAULT][side-1];
	float max_range = max_value[SEA_ASSAULT][side-1];
	float max_speed = this->max_speed[3][side-1];

	double total_efficiency = gr_eff * max_eff[3][0] + air_eff * max_eff[3][1] + hover_eff * max_eff[3][2] 
							+ sea_eff * max_eff[3][3] +  + submarine_eff * max_eff[3][4] + stat_eff * max_eff[3][5];

	if(total_efficiency <= 0)
		total_efficiency = 1;

	UnitTypeStatic *unit;

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[SEA_ASSAULT][side-1].begin(); i != units_of_category[SEA_ASSAULT][side-1].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].builderAvailable)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else if(!canBuild)
		{
			my_ranking = gr_eff * unit->efficiency[0] + air_eff * unit->efficiency[1] + hover_eff * unit->efficiency[2]
						+ sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
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

int AAIBuildTable::GetSubmarineAssault(int side, float sea_eff, float submarine_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild)
{
	float best_ranking = 0, my_ranking;
	float best_unit = 0;
	
	float max_cost = this->max_cost[SUBMARINE_ASSAULT][side-1];
	float max_range = max_value[SUBMARINE_ASSAULT][side-1];
	float max_speed = this->max_speed[4][side-1];

	double total_efficiency = sea_eff * max_eff[4][3] + submarine_eff * max_eff[4][4] + stat_eff * max_eff[4][5];

	if(total_efficiency <= 0)
		total_efficiency = 1;

	UnitTypeStatic *unit;

	// TODO: improve algorithm
	for(list<int>::iterator i = units_of_category[SEA_ASSAULT][side-1].begin(); i != units_of_category[SEA_ASSAULT][side-1].end(); ++i)
	{
		unit = &units_static[*i];

		if(canBuild && units_dynamic[*i].builderAvailable)
		{
			my_ranking = sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
		}
		else if(!canBuild)
		{
			my_ranking = sea_eff * unit->efficiency[3] + submarine_eff * unit->efficiency[4] + stat_eff * unit->efficiency[5];

			my_ranking *= max_cost / (total_efficiency * pow(unit->cost, cost));

			my_ranking *= (range * unit->range / max_range + speed * unitList[*i-1]->speed / max_speed);

			my_ranking *= (1 + 0.1 * ((float)(rand()%randomness)));
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
			if(units_static[def_killer->id].efficiency[killed] < 15)
				units_static[def_killer->id].efficiency[killed] += 0.05;
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

void AAIBuildTable::UpdateMinMaxAvgEfficiency(int side)
{
	float group_units;

	float min_eff, max_eff, sum_eff;
	UnitCategory killer, killed;
	
	for(int i = 0; i < combat_categories; ++i)
	{
		for(int j = 0; j < combat_categories; ++j)
		{
			// update max and avg efficiency of i versus j
			max_eff = 0; 
			min_eff = 10000;
			sum_eff = 0;

			killer = GetAssaultCategoryOfID(i);
			killed = GetAssaultCategoryOfID(j);

			if(killer != UNKNOWN && killed != UNKNOWN)
			{
				group_units = units_of_category[killer][side].size();

				if(group_units > 0)
				{
					for(list<int>::iterator unit = units_of_category[killer][side].begin(); unit != units_of_category[killer][side].end(); ++unit)
					{
						sum_eff += units_static[*unit].efficiency[j];

						if(units_static[*unit].efficiency[j] > max_eff)
							max_eff = units_static[*unit].efficiency[j];

						if(units_static[*unit].efficiency[j] < min_eff)
							min_eff = units_static[*unit].efficiency[j];
					}

					this->avg_eff[i][j] = sum_eff/group_units;
					this->max_eff[i][j] = max_eff;
					this->min_eff[i][j] = min_eff;
					
					if(max_eff - min_eff > 0)
						total_eff[i][j] = max_eff - min_eff;
					else
						total_eff[i][j] = 1;
				}
				else
				{
					this->max_eff[i][j] = 0;
					this->min_eff[i][j] = 0;
					this->avg_eff[i][j] = 0;
					total_eff[i][j] = 1;
				}
			}
			else
			{
				this->max_eff[i][j] = 0;
				this->min_eff[i][j] = 0;
				this->avg_eff[i][j] = 0;
				total_eff[i][j] = 1;
			}
		}
	}
}

// returns true if unitdef->id can build unit with unitdef->id
bool AAIBuildTable::CanBuildUnit(int id_builder, int id_unit)
{
	// look in build options of builder for unit
	for(list<int>::iterator i = units_static[id_builder].canBuildList.begin(); i != units_static[id_builder].canBuildList.end(); i++)
	{
		if(*i == id_unit)
			return true;
	}

	// unit not found in builders buildoptions
	return false;
}

// dtermines sides of units by recursion
void AAIBuildTable::CalcBuildTree(int unit)
{
	// go through all possible build options and set side if necessary
	for(list<int>::iterator i = units_static[unit].canBuildList.begin(); i != units_static[unit].canBuildList.end(); i++)
	{
		// add this unit to targets builtby-list
		// filter out cons. units that can build other units
		if(unitList[*i-1]->canfly || unitList[*i-1]->movedata) // buildoption is a unit 
		{	
			if(!unitList[unit-1]->canfly && !unitList[unit-1]->movedata) // builder is a building
				units_static[*i].builtByList.push_back(unit);
		}
		else
		{
			// buildoption is a building
			units_static[*i].builtByList.push_back(unit);
		}

		// calculate builder_costs
		if(units_static[*i].builder_metal_cost)
		{
			if(unitList[unit-1]->metalCost < units_static[*i].builder_metal_cost)
				units_static[*i].builder_metal_cost = unitList[unit-1]->metalCost;
		}
		else
			units_static[*i].builder_metal_cost = unitList[unit-1]->metalCost;
		
		if(units_static[*i].builder_energy_cost)
		{
			if(unitList[unit-1]->energyCost < units_static[*i].builder_energy_cost)
				units_static[*i].builder_energy_cost = unitList[unit-1]->energyCost;
		}
		else
			units_static[*i].builder_energy_cost = unitList[unit-1]->energyCost;

		// continue with all builldoptions (if they have not been visited yet)
		if(!units_static[*i].side)
		{
			// unit has not been checked yet, set side as side of its builder and continue 
			units_static[*i].side = units_static[unit].side;
			
			CalcBuildTree(*i);
		}
		
		// if already checked end recursion	
	}
}


// returns true if cache found
bool AAIBuildTable::LoadBuildTable()
{
	// stop further loading if alredy done
	if(units_static)
	{
		units_dynamic = new UnitTypeDynamic[numOfUnits+1];

		for(int i = 0; i <= numOfUnits; ++i)
		{
			units_dynamic[i].active = 0;
			units_dynamic[i].requested = 0;
			units_dynamic[i].builderAvailable = false;
			units_dynamic[i].builderRequested = false;
		}

		return true;
	}
	else	// load data
	{
		// get filename
		char filename[1000];
		char buffer[120];
		strcpy(buffer, AI_PATH);
		strcat(buffer, MOD_LEARN_PATH);
		strcat(buffer, cb->GetModName());
		ReplaceExtension (buffer, filename, sizeof(filename), ".dat");
		cb->GetValue(AIVAL_LOCATE_FILE_R, filename);

		FILE *load_file;

		int tmp = 0, bo = 0, bb = 0, cat = 0;
	
		// load units if file exists 
		if(load_file = fopen(filename, "r"))
		{
			// check if correct version
			fscanf(load_file, "%s", buffer);

			if(strcmp(buffer, TABLE_FILE_VERSION))
			{
				cb->SendTextMsg("Buildtable version out of date - creating new one",0);
				return false;
			}	

			// load mod usefulness first
			for(int i = 0; i < numOfSides; ++i)
			{
				for(int k = 0; k <= WATER_MAP; ++k)
				{
					for(int j = 0; j < assault_categories.size(); ++j)		
						fscanf(load_file, "%f ", &mod_usefulness[j][i][k]);
				}
			}

			units_static = new UnitTypeStatic[numOfUnits+1];
			units_dynamic = new UnitTypeDynamic[numOfUnits+1];

			units_static[0].id = 0;
			units_static[0].side = 0;

			for(int i = 1; i <= numOfUnits; ++i)
			{
				fscanf(load_file, "%i %i %f %f %f %f %f %i %i %i ",&units_static[i].id, &units_static[i].side, 
									&units_static[i].range, &units_static[i].cost, &units_static[i].builder_cost, 
									&units_static[i].builder_metal_cost, &units_static[i].builder_energy_cost, &cat, &bo, &bb);

				// get memory for eff
				units_static[i].efficiency = new float[combat_categories];

				// load eff
				for(int k = 0; k < combat_categories; ++k)
					fscanf(load_file, "%f ", &units_static[i].efficiency[k]);
			
				units_static[i].category = (UnitCategory) cat;

				units_dynamic[i].active = 0;
				units_dynamic[i].requested = 0;
				units_dynamic[i].builderAvailable = false;
				units_dynamic[i].builderRequested = false;
			
				// load buildoptions
				for(int j = 0; j < bo; j++)
				{
					fscanf(load_file, "%i ", &tmp);
					units_static[i].canBuildList.push_back(tmp);
				}

				// load builtby-list
				for(int k = 0; k < bb; k++)
				{
					fscanf(load_file, "%i ", &tmp);
					units_static[i].builtByList.push_back(tmp);
				}
			}

			// now load unit lists 
			for(int s = 0; s < numOfSides; s++)
			{
				for(int cat = 0; cat <= SEA_BUILDER; ++cat)
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
			}

			
		
			fclose(load_file);
			return true;
		}
	}

	return false;
}

void AAIBuildTable::SaveBuildTable()
{
	// reset factory ratings
	for(int s = 0; s < cfg->SIDES; ++s)
	{
		for(list<int>::iterator fac = units_of_category[GROUND_FACTORY][s].begin(); fac != units_of_category[GROUND_FACTORY][s].end(); ++fac)
		{
			units_static[*fac].efficiency[5] = -1;
			units_static[*fac].efficiency[4] = 0;
		}
		
		for(list<int>::iterator fac = units_of_category[SEA_FACTORY][s].begin(); fac != units_of_category[SEA_FACTORY][s].end(); ++fac)
		{
			units_static[*fac].efficiency[5] = -1;
			units_static[*fac].efficiency[4] = 0;
		}
	}
	// reset builder ratings
	for(int s = 0; s < cfg->SIDES; ++s)
	{
		for(list<int>::iterator builder = units_of_category[GROUND_BUILDER][s].begin(); builder != units_of_category[GROUND_BUILDER][s].end(); ++builder)
			units_static[*builder].efficiency[5] = -1;

		for(list<int>::iterator builder = units_of_category[AIR_BUILDER][s].begin(); builder != units_of_category[AIR_BUILDER][s].end(); ++builder)
			units_static[*builder].efficiency[5] = -1;
		
		for(list<int>::iterator builder = units_of_category[SEA_BUILDER][s].begin(); builder != units_of_category[SEA_BUILDER][s].end(); ++builder)
			units_static[*builder].efficiency[5] = -1;

		for(list<int>::iterator builder = units_of_category[HOVER_BUILDER][s].begin(); builder != units_of_category[HOVER_BUILDER][s].end(); ++builder)
			units_static[*builder].efficiency[5] = -1;
	}

	// get filename
	char filename[1000];
	char buffer[120];
	strcpy(buffer, AI_PATH);
	strcat(buffer, MOD_LEARN_PATH);
	strcat(buffer, cb->GetModName());
	ReplaceExtension (buffer, filename, sizeof(filename), ".dat");
	cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	FILE *save_file = fopen(filename, "w+");

	// file version
	fprintf(save_file, "%s \n", TABLE_FILE_VERSION);
	
	MapType mapType;
	
	if(ai->map)
		mapType = ai->map->mapType;
	else
		mapType = LAND_MAP;

	float sum = 0;

	// save category usefulness
	for(int i = 0; i < cfg->SIDES; ++i)
	{	
		// rebalance mod_usefulness
		if(cfg->AIR_ONLY_MOD)
		{
			sum = mod_usefulness[0][i][mapType] + mod_usefulness[2][i][mapType] + mod_usefulness[3][i][mapType] + mod_usefulness[4][i][mapType];
			mod_usefulness[0][i][mapType] *= (100.0/sum);
			mod_usefulness[2][i][mapType] *= (100.0/sum);
			mod_usefulness[3][i][mapType] *= (100.0/sum);
			mod_usefulness[4][i][mapType] *= (100.0/sum);
		}
		else if(mapType == LAND_MAP)
		{
			sum = mod_usefulness[0][i][LAND_MAP] + mod_usefulness[2][i][LAND_MAP];
			mod_usefulness[0][i][LAND_MAP] *= (100.0/sum);
			mod_usefulness[2][i][LAND_MAP] *= (100.0/sum);
		}
		else if(mapType == LAND_WATER_MAP)
		{
			sum = mod_usefulness[0][i][LAND_WATER_MAP] + mod_usefulness[2][i][LAND_WATER_MAP] + mod_usefulness[3][i][LAND_WATER_MAP] + mod_usefulness[4][i][LAND_WATER_MAP];
			mod_usefulness[0][i][LAND_WATER_MAP] *= (100.0/sum);
			mod_usefulness[2][i][LAND_WATER_MAP] *= (100.0/sum);
			mod_usefulness[3][i][LAND_WATER_MAP] *= (100.0/sum);
			mod_usefulness[4][i][LAND_WATER_MAP] *= (100.0/sum);
		}
		else if(mapType == WATER_MAP)
		{
			sum = mod_usefulness[2][i][WATER_MAP] + mod_usefulness[3][i][WATER_MAP] + mod_usefulness[4][i][WATER_MAP];
			mod_usefulness[2][i][WATER_MAP] *= (100.0/sum);
			mod_usefulness[3][i][WATER_MAP] *= (100.0/sum);
			mod_usefulness[4][i][WATER_MAP] *= (100.0/sum);
		}

		// save
		for(int k = 0; k <= WATER_MAP; ++k)
		{
			for(int j = 0; j < assault_categories.size(); ++j)		
				fprintf(save_file, "%f ", mod_usefulness[j][i][k]);
			
			fprintf(save_file, "\n");
		}
	}

	int tmp;

	for(int i = 1; i <= numOfUnits; ++i)
	{
		tmp = units_static[i].canBuildList.size();

		fprintf(save_file, "%i %i %f %f %f %f %f %i %i %i ", units_static[i].id, units_static[i].side, units_static[i].range, 
								units_static[i].cost, units_static[i].builder_cost, units_static[i].builder_metal_cost, units_static[i].builder_energy_cost, 
								(int) units_static[i].category, units_static[i].canBuildList.size(), units_static[i].builtByList.size());

		// save combat eff
		for(int k = 0; k < combat_categories; ++k)
			fprintf(save_file, "%f ", units_static[i].efficiency[k]);

		// save buildoptions
		for(list<int>::iterator j = units_static[i].canBuildList.begin(); j != units_static[i].canBuildList.end(); j++)
		{
			fprintf(save_file, "%i ", *j);
		}

		// save builtby-list
		for(list<int>::iterator k = units_static[i].builtByList.begin(); k != units_static[i].builtByList.end(); k++)
		{
			fprintf(save_file, "%i ", *k);
		}

		fprintf(save_file, "\n");
	}

	for(int s = 0; s < numOfSides; ++s)
	{
		// now save unit lists
		for(int cat = 0; cat <= SEA_BUILDER; ++cat)
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
	char filename[1000];
	char buffer[120];
	strcpy(buffer, AILOG_PATH);
	strcat(buffer, "BuildTable_");
	strcat(buffer, cb->GetModName());
	ReplaceExtension (buffer, filename, sizeof(filename), ".txt");
	cb->GetValue(AIVAL_LOCATE_FILE_W, filename);

	FILE *file = fopen(filename, "w");

	if(file)
	{

	for(int i = 1; i <= numOfUnits; i++)
	{
		// unit type
		unitType = GetUnitType(i);

		if(units_static[i].category == GROUND_ASSAULT)
		{
			if(unitType == ANTI_AIR_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s anti air unit\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
			else if(unitType == ASSAULT_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s assault unit\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
		}
		else if(units_static[i].category == HOVER_ASSAULT)
		{
			if(unitType == ANTI_AIR_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s anti air unit\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
			else
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s assault unit\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
		}
		else if(units_static[i].category == SEA_ASSAULT)
		{
			if(unitType == ANTI_AIR_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s anti air uni \n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
			else
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s naval assault unit\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
		}
		else if(units_static[i].category == AIR_ASSAULT)
		{
			if(unitType == ANTI_AIR_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s fighter\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
			else if(unitType == ASSAULT_UNIT)
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s gunship\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
			else
				fprintf(file, "ID: %-3i %-16s %-40s %-25s %-8s bomber\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());
		}
		else
			fprintf(file, "ID: %-3i %-16s %-40s %-25s %s\n", i, unitList[i-1]->name.c_str(), unitList[i-1]->humanName.c_str(), GetCategoryString(i), sideNames[units_static[i].side].c_str());

		//fprintf(file, "Max damage: %f\n", GetMaxDamage(i));
		/*fprintf(file, "Can Build:\n");
		for(list<int>::iterator j = units[i].canBuildList.begin(); j != units[i].canBuildList.end(); j++)
		{
			fprintf(file, "%s ", unitList[*j-1]->humanName.c_str());
		}
		fprintf(file, "\n Built by: ");

		for(list<int>::iterator k = units[i].builtByList.begin(); k != units[i].builtByList.end(); k++)
		{
			fprintf(file, "%s ", unitList[*k-1]->humanName.c_str());
		}
		fprintf(file, "\n \n");*/
	}

	for(int s = 1; s <= numOfSides; s++)
	{	
		// print unit lists
		for(int cat = 1; cat <= SEA_BUILDER; ++cat)
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

	for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unitList[unit_id -1]->weapons.begin(); i != unitList[unit_id -1]->weapons.end(); i++)
	{
		for(int k = 0; k < armor_types; ++k)
		{
			if((*i).def->damages.damages[k] > max_damage)
				max_damage = (*i).def->damages.damages[k];
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
	if (a>s-sizeof(AI_PATH)) a=s-sizeof("");
	memcpy (&dst [sizeof ("")-1], n, a);
	dst[a+sizeof("")]=0;

	strncat (dst, ext, s);
}

float AAIBuildTable::GetFactoryRating(int id)
{
	// check if value already chached
	if(units_static[id].efficiency[5] != -1)
		return units_static[id].efficiency[5];

	// calculate rating and cache result
	bool builder = false;
	float rating = 1;
	float combat_units = 0;
	float ground = (ai->map->map_usefulness[0][ai->side-1] + mod_usefulness[0][ai->side-1][ai->map->mapType]) / 20.0;
	float hover = (ai->map->map_usefulness[2][ai->side-1] + mod_usefulness[2][ai->side-1][ai->map->mapType]) / 10.0;
	float air = 10/((float)(cfg->AIRCRAFT_RATE));
	float sea = (ai->map->map_usefulness[3][ai->side-1] + mod_usefulness[3][ai->side-1][ai->map->mapType]) / 10.0;
	float submarine = (ai->map->map_usefulness[4][ai->side-1] + mod_usefulness[4][ai->side-1][ai->map->mapType]) /10.0;

	if(cfg->AIR_ONLY_MOD || ai->map->mapType == LAND_MAP)
	{
		for(list<int>::iterator unit = units_static[id].canBuildList.begin(); unit != units_static[id].canBuildList.end(); unit++)	
		{
			if(units_static[*unit].category == GROUND_ASSAULT) 
			{
				rating += ground * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == HOVER_ASSAULT) 
			{
				rating += hover * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT) 
			{
				rating += air * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2]);
				combat_units += 1;
			}
			else if(IsBuilder(units_static[*unit].category) && units_static[*unit].category != SEA_BUILDER)
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
		}	
	}
	else if(ai->map->mapType == AIR_MAP)
	{
		for(list<int>::iterator unit = units_static[id].canBuildList.begin(); unit != units_static[id].canBuildList.end(); unit++)	
		{
			if(units_static[*unit].category == AIR_ASSAULT)
			{
				rating += units_static[*unit].efficiency[1];
				combat_units += 1;
			}
			else if(IsBuilder(units_static[*unit].category))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
		}	
	}
	else if(ai->map->mapType == LAND_WATER_MAP)
	{
		for(list<int>::iterator unit = units_static[id].canBuildList.begin(); unit != units_static[id].canBuildList.end(); unit++)	
		{
			if(units_static[*unit].category == GROUND_ASSAULT) 
			{
				rating += ground * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == HOVER_ASSAULT) 
			{
				rating += hover * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT) 
			{
				rating += air * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == SEA_ASSAULT) 
			{
				rating += sea * (ground * units_static[*unit].efficiency[0] + air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3] + + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == SUBMARINE_ASSAULT) 
			{
				rating += submarine * (sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(IsBuilder(units_static[*unit].category))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
		}
	}
	else if(ai->map->mapType == WATER_MAP)
	{
		for(list<int>::iterator unit = units_static[id].canBuildList.begin(); unit != units_static[id].canBuildList.end(); unit++)	
		{
			if(units_static[*unit].category == HOVER_ASSAULT) 
			{
				rating += hover * (air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == AIR_ASSAULT) 
			{
				rating += air * (air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == SEA_ASSAULT) 
			{
				rating += sea * (air * units_static[*unit].efficiency[1] + hover * units_static[*unit].efficiency[2] + sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(units_static[*unit].category == SUBMARINE_ASSAULT) 
			{
				rating += submarine * (sea * units_static[*unit].efficiency[3] + submarine * units_static[*unit].efficiency[4]);
				combat_units += 1;
			}
			else if(IsBuilder(units_static[*unit].category))
			{
				rating += 256 * GetBuilderRating(*unit);
				builder = true;
			}
		}
	}
	
	if(combat_units)
	{
		rating /= (combat_units * pow(units_static[id].cost, 2));
		rating *= sqrt(8 + combat_units) * 10;
		units_static[id].efficiency[5] = rating;
		return rating;
	}
	else if(builder)
	{
		units_static[id].efficiency[5] = 0.25;
		return 0.25;
	}
	else
	{
		units_static[id].efficiency[5] = 0;
		return 0;
	}
}

float AAIBuildTable::GetBuilderRating(int id)
{
	// calculate rating and cache result
	float rating = 1;

	// check if value already chached
	if(units_static[id].efficiency[5] != -1)
		rating = units_static[id].efficiency[5];
	else
	{
		/*for(list<int>::iterator building = units_static[id].canBuildList.begin(); building != units_static[id].canBuildList.end(); ++building)
		{
			if(units_static[*building].category == GROUND_FACTORY || units_static[*building].category == SEA_FACTORY)
				rating += GetFactoryRating(*building);
		}*/

		rating *= (pow(unitList[id-1]->buildSpeed, 1.3f) * sqrt((float) 4 + units_static[id].canBuildList.size()));
		units_static[id].efficiency[5] = rating;
	}

	// add costs of factory if no factory to build that cons. unit has been built yet
	if(units_dynamic[id].builderAvailable)
		rating /= pow(units_static[id].cost, 1.5f); 
	else
		rating /= pow(units_static[id].builder_cost + units_static[id].cost, 1.5f);

	rating /= (5 + units_dynamic[id].requested + units_dynamic[id].active);

	return rating;
}

void AAIBuildTable::BuildFactoryFor(int unit_id)
{
	bool suitable;

	if(unit_id > numOfUnits)
	{
		fprintf(ai->file, "ERROR: BuildFactoryFor(), index %i out of range", unit_id);
		return;
	}

	if(!units_dynamic[unit_id].builderRequested)
	{
		fprintf(ai->file, "Looking for factory to build %s\n", unitList[unit_id-1]->humanName.c_str());
		
		for(list<int>::iterator i = units_static[unit_id].builtByList.begin();  i != units_static[unit_id].builtByList.end(); i++)
		{	
			if(!units_dynamic[*i].requested)
			{
				if(ai->map->mapType == LAND_MAP && units_static[*i].category == SEA_FACTORY)
					suitable = false;
				else 
					suitable = true;
			}
			else
				suitable = false;

			if(suitable)
			{
				++units_dynamic[*i].requested;

				for(list<int>::iterator j = units_static[*i].canBuildList.begin(); j != units_static[*i].canBuildList.end(); j++)
				{
					// only set to true, if the factory is not built by that unit itself
					if(!MemberOf(*j, units_static[*i].builtByList))
					{
						units_dynamic[*j].builderRequested = true;
						//fprintf(ai->file, "%s can now be built\n", unitList[*j-1]->humanName.c_str());
					}
				}

				// debug
				fprintf(ai->file, "Added %s to buildque\n", unitList[*i-1]->humanName.c_str()); 

				if(!units_dynamic[*i].builderRequested)
					BuildBuilderFor(*i);

				return;
			}
		}
	}
}

void AAIBuildTable::BuildBuilderFor(int building_id)
{
	if(building_id > numOfUnits)
	{
		fprintf(ai->file, "ERROR: BuildBuilderFor(), index %i out of range", building_id);
		return;
	}

	if(!units_dynamic[building_id].builderRequested)
	{
		fprintf(ai->file, "Looking for builder to build %s\n", unitList[building_id-1]->humanName.c_str());

		int builder = 0;
		float best_rating = 0, my_rating; 

		// look for best builder to do the job
		for(list<int>::iterator i = units_static[building_id].builtByList.begin();  i != units_static[building_id].builtByList.end(); i++)
		{
			my_rating = GetBuilderRating(*i);

			if(IsStartingUnit(*i))
				my_rating = 0;
			else if(units_static[*i].category == SEA_BUILDER && ai->map->mapType == LAND_MAP)
				my_rating = 0;

			if(my_rating > best_rating)
			{	
				best_rating = my_rating;
				builder = *i;
			}
		}

		if(builder)
		{
			++units_dynamic[builder].requested;

			// set all its buildoptions buildable
			for(list<int>::iterator j = units_static[builder].canBuildList.begin(); j != units_static[builder].canBuildList.end(); j++)
				units_dynamic[*j].builderRequested = true;
			
			// debug
			fprintf(ai->file, "Added %s to buildque\n", unitList[builder-1]->humanName.c_str()); 

			if(!units_dynamic[builder].builderRequested)
				BuildFactoryFor(builder);

			ai->execute->AddUnitToBuildque(builder);

			//char c[120];
			fprintf(ai->file, "Requested: %s %i\n", unitList[builder-1]->humanName.c_str(), units_dynamic[builder].requested);
			//cb->SendTextMsg(c,0);	
		}
	}
}

void AAIBuildTable::CheckAddBuilder(int building_id)
{
	int builder = 0;
	float best_rating = 0, my_rating; 

	// look for best builder to do the job
	for(list<int>::iterator unit = units_static[building_id].builtByList.begin();  unit != units_static[building_id].builtByList.end(); ++unit)
	{
		if(units_dynamic[*unit].requested + units_dynamic[*unit].active >= cfg->MAX_BUILDERS_PER_TYPE || IsStartingUnit(*unit) || !units_dynamic[*unit].builderAvailable) 
			my_rating = 0;
		else
			my_rating = GetBuilderRating(*unit);
			
		if(my_rating > best_rating)
		{	
			best_rating = my_rating;
			builder = *unit;
		}
	}

	if(builder && units_dynamic[builder].requested < 2)
	{
		++units_dynamic[builder].requested;

		// set all its buildoptions buildable
		if(units_dynamic[builder].active  < 1)
		{
			for(list<int>::iterator j = units_static[builder].canBuildList.begin(); j != units_static[builder].canBuildList.end(); j++)
				units_dynamic[*j].builderRequested = true;
		}

		// build factory if necessary
		if(!units_dynamic[builder].builderRequested)
			BuildFactoryFor(builder);

		// add to buildque (suitable factory should be requested now
		ai->execute->AddUnitToBuildque(builder);

		//char c[120];
		//fprintf(ai->file, "Requested: %s \n", unitList[builder-1]->humanName.c_str(), units_dynamic[builder].requested);
		//cb->SendTextMsg(c,0);	
	}
}

void AAIBuildTable::AddAssitantBuilder(bool water, bool floater, bool canBuild)
{
	int builder = 0;
	float best_rating = 0, my_rating; 

	int side = ai->side-1;

	// look for best builder to do the job
	// ground builders only allowed for land based tasks
	if(!water)
	{
		for(list<int>::iterator unit = units_of_category[GROUND_BUILDER][side].begin();  unit != units_of_category[GROUND_BUILDER][side].end(); ++unit)
		{
			//char c[120]; 
			//sprintf(c, "%s %f %f", unitList[*unit-1]->humanName.c_str() ,unitList[*unit-1]->buildSpeed, (float)cfg->MIN_ASSISTANCE_BUILDSPEED);
			//cb->SendTextMsg(c, 0);
			// filter out builders that already reached builder per type limit or cannot be built atm 
			if(!(canBuild && !units_dynamic[*unit].builderAvailable) && unitList[*unit-1]->buildSpeed >= (float)cfg->MIN_ASSISTANCE_BUILDTIME 
				&& units_dynamic[*unit].requested + units_dynamic[*unit].active < cfg->MAX_BUILDERS_PER_TYPE)
			{
				my_rating = (10 + unitList[*unit-1]->buildSpeed) / (10 + units_static[*unit].cost); 
					
				if(my_rating > best_rating)
				{	
					best_rating = my_rating;
					builder = *unit;
				}
			}
		}
	}

	// ships allowed for all sea based tasks
	if(water)
	{
		for(list<int>::iterator unit = units_of_category[SEA_BUILDER][side].begin();  unit != units_of_category[SEA_BUILDER][side].end(); ++unit)
		{
			// filter out builders that already reached builder per type limit or cannot be built atm 
			if(!(canBuild && !units_dynamic[*unit].builderAvailable) && units_dynamic[*unit].requested + units_dynamic[*unit].active < cfg->MAX_BUILDERS_PER_TYPE)
			{
				my_rating = (10 + unitList[*unit-1]->buildSpeed) / (10 + units_static[*unit].cost); 
					
				if(my_rating > best_rating)
				{	
					best_rating = my_rating;
					builder = *unit;
				}
			}
		}
	}

	// air and hover assisters only not allowed for underwater tasks
	if(floater)
	{
		for(list<int>::iterator unit = units_of_category[AIR_BUILDER][side].begin();  unit != units_of_category[AIR_BUILDER][side].end(); ++unit)
		{
			// filter out builders that already reached builder per type limit or cannot be built atm 
			if(!(canBuild && !units_dynamic[*unit].builderAvailable) && units_dynamic[*unit].requested + units_dynamic[*unit].active < cfg->MAX_BUILDERS_PER_TYPE)
			{
				my_rating = (10 + unitList[*unit-1]->buildSpeed) / (10 + units_static[*unit].cost); 
					
				if(my_rating > best_rating)
				{	
					best_rating = my_rating;
					builder = *unit;
				}
			}
		}

		for(list<int>::iterator unit = units_of_category[HOVER_BUILDER][side].begin();  unit != units_of_category[HOVER_BUILDER][side].end(); ++unit)
		{
			// filter out builders that already reached builder per type limit or cannot be built atm 
			if(!(canBuild && !units_dynamic[*unit].builderAvailable) && units_dynamic[*unit].requested + units_dynamic[*unit].active < cfg->MAX_BUILDERS_PER_TYPE)
			{
				my_rating = (10 + unitList[*unit-1]->buildSpeed) / (10 + units_static[*unit].cost); 
					
				if(my_rating > best_rating)
				{	
					best_rating = my_rating;
					builder = *unit;
				}
			}
		}
	}

	if(builder && units_dynamic[builder].requested < 2)
	{
		++units_dynamic[builder].requested;

		// set all its buildoptions buildable
		if(units_dynamic[builder].active  < 1)
		{
			for(list<int>::iterator j = units_static[builder].canBuildList.begin(); j != units_static[builder].canBuildList.end(); j++)
				units_dynamic[*j].builderRequested = true;
		}

		// build factory if necessary
		if(!units_dynamic[builder].builderRequested)
			BuildFactoryFor(builder);

		// add to buildque (suitable factory should be requested now
		ai->execute->AddUnitToBuildque(builder);

		//char c[120];
		//fprintf(ai->file, "Requested: %s \n", unitList[builder-1]->humanName.c_str(), units_dynamic[builder].requested);
		//cb->SendTextMsg(c,0);	
	}
}

	


bool AAIBuildTable::IsArty(int id)
{
	if(!unitList[id-1]->weapons.empty())
	{
		float max_range = 0;
		WeaponDef *longest = 0;

		for(vector<UnitDef::UnitDefWeapon>::const_iterator weapon = unitList[id-1]->weapons.begin(); weapon != unitList[id-1]->weapons.end(); weapon++)
		{
			if(weapon->def->range > max_range)
			{
				max_range = weapon->def->range;
				longest = weapon->def;
			}
		}

		if(max_range > cfg->MOBILE_ARTY_RANGE)
			return true;

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
		for(list<int>::iterator i = cfg->SCOUTS.begin(); i != cfg->SCOUTS.end(); i++)
		{
			if(*i == id)
				return true;
		}
	}

	return false;
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

	if(unitList[def_id-1]->weapons.size() > 1)
	{ 
		return false;
	}
	else
	{
		for(vector<UnitDef::UnitDefWeapon>::const_iterator weapon = unitList[def_id-1]->weapons.begin(); weapon != unitList[def_id-1]->weapons.end(); ++weapon)
		{
			if(weapon->def->isShield)
				return true;
		}
	}

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
	else if(category == MOBILE_ARTY)
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
	else if(def_id == GROUND_BUILDER)
		return "ground builder";
	else if(def_id == AIR_BUILDER)
		return "air builder";
	else if(def_id == HOVER_BUILDER)
		return "hover builder";
	else if(def_id == SEA_BUILDER)
		return "sea builder";
	else if(def_id== GROUND_SCOUT)
		return "ground scout";
	else if(def_id == AIR_SCOUT)
		return "air scout";
	else if(def_id == HOVER_SCOUT)
		return "hover scout";
	else if(def_id == SEA_SCOUT)
		return "sea scout";
	else if(def_id == MOBILE_ARTY)
		return "mobile artillery";
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
	else if(def_id == GROUND_FACTORY)
		return "ground factory";
	else if(def_id == SEA_FACTORY)
		return "water factory";
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
	else if(def_id == BARRICADE)
		return "barricade";

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
	else if(category == GROUND_BUILDER) 
		return "ground builder";
	else if(category == AIR_BUILDER) 
		return "air builder";
	else if(category == HOVER_BUILDER) 
		return "hover builder";
	else if(category == SEA_BUILDER) 
		return "sea builder";
	else if(category == GROUND_SCOUT)
		return "ground scout";
	else if(category == AIR_SCOUT)
		return "air scout";
	else if(category == HOVER_SCOUT)
		return "hover scout";
	else if(category == SEA_SCOUT)
		return "sea scout";
	else if(category == MOBILE_ARTY)
		return "mobile artillery";
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
	else if(category == GROUND_FACTORY)
		return "ground factory";
	else if(category == SEA_FACTORY)
		return "water factory";
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
	else if(category == BARRICADE)
		return "barricade";

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

bool AAIBuildTable::IsBuilder(UnitCategory category)
{
	if(category >= GROUND_BUILDER && category <= SEA_BUILDER)
		return true;
	else 
		return false;
}

bool AAIBuildTable::IsScout(UnitCategory category)
{
	if(category >= GROUND_SCOUT && category <= SEA_SCOUT)
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
	//else if(category == STATIONARY_DEF)
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
