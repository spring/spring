// Based on Submarine's BuildTable Class from AAI. Thanks sub!

#include "UnitTable.h"

CUnitTable::CUnitTable(AIClasses* ai) {
	this -> ai = ai;
	// L("Starting Unit Table");
	numOfUnits = 0;
	unitList = 0;

	// Get all sides and commanders
	string sidestr = "SIDE";
	string errorstring = "-1";
	string Valuestring;
	char k[50];
	// L("Loading sidedata");
	ai -> parser -> LoadVirtualFile("gamedata\\SIDEDATA.tdf");

	// L("Starting Loop");
	for (int i = 0; i < 10; i++) {
		sprintf(k,"%i",i);
		ai -> parser -> GetDef(Valuestring, errorstring, string(sidestr + k + "\\commander"));

		if (ai -> cb -> GetUnitDef(Valuestring.c_str())) {
			startUnits.push_back(ai -> cb -> GetUnitDef(Valuestring.c_str()) -> id);
			ai -> parser -> GetDef(Valuestring, errorstring, string(sidestr + k + "\\name"));
			sideNames.push_back(Valuestring);
			numOfSides = i + 1;
		}
	}

	// L("Sides Set Up");

	// now set up the unit lists
	ground_factories = new vector<int>[numOfSides];
	ground_builders = new vector<int>[numOfSides];
	ground_attackers = new vector<int>[numOfSides];
	metal_extractors = new vector<int>[numOfSides];
	metal_makers = new vector<int>[numOfSides];
	ground_energy = new vector<int>[numOfSides];
	ground_defences = new vector<int>[numOfSides];
	metal_storages = new vector<int>[numOfSides];
	energy_storages = new vector<int>[numOfSides];

	all_lists.push_back(ground_factories);
	all_lists.push_back(ground_builders);
	all_lists.push_back(ground_attackers);
	all_lists.push_back(metal_extractors);
	all_lists.push_back(metal_makers);
	all_lists.push_back(ground_energy);
	all_lists.push_back(ground_defences);
	all_lists.push_back(metal_storages);
	all_lists.push_back(energy_storages);

	// L("UnitTable Inited!");
}

CUnitTable::~CUnitTable() {
	delete[] unittypearray;
	delete[] unitList;

	delete[] ground_factories;
	delete[] ground_builders;
	delete[] ground_attackers;
	delete[] metal_extractors;
	delete[] metal_makers;
	delete[] ground_energy;
	delete[] ground_defences;
	delete[] metal_storages;
	delete[] energy_storages;
}



int CUnitTable::GetSide(int unit) {
	assert(ai -> cb -> GetUnitDef(unit) != NULL);
	return unittypearray[ai -> cb -> GetUnitDef(unit) -> id].side;
}

int CUnitTable::GetCategory(const UnitDef* unitdef) {
	return unittypearray[unitdef -> id].category;
}

int CUnitTable::GetCategory(int unit) {
	assert(ai -> cb -> GetUnitDef(unit) != NULL);

	return (unittypearray[ai -> cb -> GetUnitDef(unit) -> id].category);
}




float CUnitTable::GetDPS(const UnitDef* unit) {
	// L("");
	// L("Getting DPS for Unit: " << unit->humanName << "category: " << unit->category << " catstring: " << unit->categoryString);
	if (unit) {
		// L("Units Only targetcat:" << defaultvalue);
		float totaldps = 0;

		for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
			float dps = 0;
			if (!i -> def -> paralyzer) {
				// L("weapon: " << i->name);
				// L("Categories, ONLY:" <<  i->onlyTargetCat << "BAD: " << i->onlyTargetCat << " ONLY(def): " << i->def->onlyTargetCategory);
				int numberofdamages;
				ai -> cb -> GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);
				float reloadtime = i -> def -> reload;

				for (int k = 0; k < numberofdamages; k++) {
					// L("damage: " << i->def->damages.damages[k]);
					dps += i -> def -> damages.damages[k];
				}

				dps = dps * i -> def -> salvosize / numberofdamages / reloadtime;
				
				// L("dps for this weapon: " << dps << " Salvo size: " << i->def->salvosize << " Reload: " << reloadtime << " number of damages: " << numberofdamages);
			}

			totaldps += dps;
		}

		// L("DPS sucessful: " << totaldps);
		return totaldps;
	}

	return 0;
}



float CUnitTable::GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim) {
	if (unit->weapons.size()) {
		ai -> math -> TimerStart();
		// L("");
		// L("Getting DPS for Unit: " << unit->humanName << " Against " << victim -> humanName);
		float dps = 0;
		bool canhit = false;
		string targetcat;
		int armortype = victim -> armorType;
		int numberofdamages;
		ai -> cb -> GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);

		for (unsigned int i = 0; i != unit -> weapons.size(); i++) {
			if (!unit -> weapons[i].def -> paralyzer) {
				targetcat = unittypearray[unit->id].TargetCategories[i];
				// L(unit->weapons[i].name << " Targcat: " <<  targetcat);
				// L("unit->categoryString " <<  unit->categoryString);
				// L("victim->categoryString " <<  victim->categoryString);
				
				unsigned int a = victim -> category;
				unsigned int b = unit -> weapons[i].def -> onlyTargetCategory;	// This is what the weapon can target
				unsigned int c = unit -> weapons[i].onlyTargetCat;				// This is what the unit accepts as this weapons target
//				unsigned int d = unit -> weapons[i].badTargetCat;				// This is what the unit thinks this weapon must be used for (hmm ?)
				bool canWeaponTarget = (a & b) > 0;
				bool canUnitTarget = (a & c) > 0;								// How is this used in spring, needs testing...
//				bool badUnitTarget = (a & d) > 0;								// This probably means that it have low priority, ignore it for now

				canhit = (canWeaponTarget && canUnitTarget);

				if (!unit -> weapons[i].def -> waterweapon && ai -> cb -> GetUnitDefHeight(victim -> id) - victim -> waterline < 0) {
					canhit = false;
					// L("This weapon cannot hit this sub");
				}

				if (unit -> weapons[i].def -> waterweapon && victim -> minWaterDepth == 0) {
					canhit = false;
					// L("this anti-sub weapon cannot kill this unit");
				}

				// Bombers are useless against air:
				if (unit -> weapons[i].def -> dropped && victim -> canfly && unit -> canfly && unit -> wantedHeight <= victim -> wantedHeight) {
					canhit = false;
				}

				if (canhit) {
					// L(unit->weapons[i].name << " a: " <<  a << ", b: " << b << ", c: " << c << ", d: " << d);
					// L("unit->categoryString " <<  unit->categoryString);
					// L("victim->categoryString " <<  victim->categoryString);
					float accuracy = unit->weapons[i].def->accuracy * 2.8;

					if (victim -> speed != 0) {
						accuracy *= 1 - (unit -> weapons[i].def -> targetMoveError);
					}

					float basedamage = unit->weapons[i].def->damages.damages[armortype] * unit->weapons[i].def->salvosize / unit->weapons[i].def->reload;
					float AOE = unit -> weapons[i].def -> areaOfEffect * 0.7;
					float tohitprobability;
					float impactarea;
					float targetarea;
					float distancetravelled = 0.7f * unit->weapons[i].def -> range;
					// L("Initial DT: " << distancetravelled);
					float firingangle;
					float gravity = -(ai -> cb -> GetGravity() * 900);
					float timetoarrive;
					float u = unit -> weapons[i].def -> projectilespeed * 30;
					// L("Type: " << unit->weapons[i].def -> type);

					if (unit->weapons[i].def -> type == string("Cannon")) {
						// L("Is ballistic! Gravity: " << gravity);
						// L("u = " << u << " distancetravelled * gravity) / (u * u) = " << (distancetravelled * gravity) / (u * u));
						float sinoid = (distancetravelled * gravity) / (u * u);
						sinoid = min(sinoid, 1.0f);
						firingangle = asin(sinoid) / 2;

						if (unit -> highTrajectoryType == 1) {
							firingangle = (PI / 2) - firingangle;
						}

						// L("Firing angle = " << firingangle * 180 / PI);
						float heightreached = pow(u * sin(firingangle), 2) / (2 * gravity);
						float halfd = distancetravelled / 2;
						distancetravelled = 2 * sqrt(halfd * halfd + heightreached * heightreached) * 1.1;
						// L("Height reached: " << heightreached << " distance travelled " << distancetravelled);
					}

					// L("Name: " << unit->weapons[i].name << " AOE: " << AOE << " Accuracy: " << unit -> weapons[i].def -> accuracy << " range: " << unit->weapons[i].def -> range);
					if ((victim -> canfly && unit -> weapons[i].def -> selfExplode) || !victim -> canfly) {
						impactarea = pow((accuracy * distancetravelled) + AOE, 2);
						targetarea = ((victim -> xsize * 16) + AOE) * ((victim -> ysize * 16) + AOE);
					}
					else {
						impactarea = pow((accuracy) * (0.7f * distancetravelled), 2);
						targetarea = (victim -> xsize * victim -> ysize * 256);
					}

					// L("Impact Area: " << impactarea << " Target Area: " << targetarea);
					if (impactarea > targetarea) {
						tohitprobability = targetarea / impactarea;
					}
					else {
						tohitprobability = 1;
					}

					if (!unit->weapons[i].def -> guided && unit -> weapons[i].def -> projectilespeed != 0 && victim -> speed != 0 && unit -> weapons[i].def -> beamtime == 1) {
						if (unit->weapons[i].def -> type == string("Cannon")) {
							timetoarrive = (2 * u * sin(firingangle)) / gravity;
						}
						else {
							timetoarrive = distancetravelled / (unit->weapons[i].def -> projectilespeed * 30);
						}

						float shotwindow = sqrt(targetarea) / victim -> speed * 1.3;
						if (shotwindow < timetoarrive) {
							tohitprobability *= shotwindow / timetoarrive;
						}
						// L("Not guided. TTA: " <<  timetoarrive << " shotwindow " << shotwindow << " unit->weapons[i].def -> targetmoveerror " << unit -> weapons[i].def -> targetMoveError);
					}

					//L("chance to hit: " << tohitprobability);
					//L("Dps for this weapon: " << basedamage * tohitprobability);
					dps += basedamage * tohitprobability;
				}
			}
		}

		// L("DPS: " << dps);
		// L("Time taken: " << ai -> math -> TimerSecs() << "s.");
		return dps;
	}

	return 0;
}

float CUnitTable::GetCurrentDamageScore(const UnitDef* unit) {
	// L("Getting CombatScore for : " << unit->humanName);
	int enemies[MAXUNITS];
	int numofenemies = ai -> cheat -> GetEnemyUnits(enemies);
	vector<int> enemyunitsoftype;
	float score = 0.01;
	float totalcost=0.01;
	enemyunitsoftype.resize(ai -> cb -> GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < numofenemies; i++) {
		enemyunitsoftype[ai -> cheat -> GetUnitDef(enemies[i]) -> id]++;
		// assert(ai -> cheat -> GetUnitDef(enemies[i]) == unittypearray[ai -> cheat -> GetUnitDef(enemies[i]) -> id].def);
	}

	for (unsigned int i = 1; i != enemyunitsoftype.size(); i++) {
		bool b1 = unittypearray[i].def -> builder;
		bool b2 = enemyunitsoftype[i] > 0;
		bool b3 = unittypearray[i].side != -1;
	//	bool b4 = (!unit -> speed && !unittypearray[i].def -> speed);

	//	if (!b1 && b2 && b3 && !b4) {
		if (!b1 && b2 && b3) {
			float currentscore = 0;
			float costofenemiesofthistype = ((unittypearray[i].def -> metalCost * METAL2ENERGY) + unittypearray[i].def -> energyCost) * enemyunitsoftype[i];
			currentscore = unittypearray[unit -> id].DPSvsUnit[i] * costofenemiesofthistype;
			totalcost += costofenemiesofthistype;

			/*
			if (unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype > 0) {
				// L("Score of them vs me: " << unittypearray[i].DPSvsUnit[unit -> id] * costofenemiesofthistype);
				currentscore -= (unittypearray[i].DPSvsUnit[unit -> id] * costofenemiesofthistype);
			}*/

		score += currentscore;
		// L("MyScore Vs " << unittypearray[i].def -> humanName << " is " << currentscore);
		// L("unittypearray.size()= " << unittypearray[i].DPSvsUnit.size() << " My ID " << unit -> id << " their ID: " << i << " costofenemiesofthistype= " << costofenemiesofthistype);
		}
	}

	if (totalcost <= 0)
		return 0;

	// L(unit->humanName << " has an average dps vs all enemies: " << score / totalcost);
	return score / totalcost;
}

void CUnitTable::UpdateChokePointArray() {
	vector<float> EnemyCostsByMoveType;
	EnemyCostsByMoveType.resize(ai -> pather -> NumOfMoveTypes);
	vector<int> enemyunitsoftype;
	float totalcosts = 1;
	int enemies[MAXUNITS];
	int numofenemies = ai -> cheat -> GetEnemyUnits(enemies);
	enemyunitsoftype.resize(ai -> cb -> GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < ai -> pather -> totalcells; i++) {
		ai -> dm -> ChokePointArray[i] = 0;
	}
	for (int i = 0; i < ai -> pather -> NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] = 0;
	}
	for (int i = 0; i < numofenemies; i++) {
		enemyunitsoftype[ai -> cheat -> GetUnitDef(enemies[i]) -> id]++;
	}

	for (unsigned int i = 1; i != enemyunitsoftype.size(); i++) {
		if (unittypearray[i].side != -1 && !unittypearray[i].def -> canfly && unittypearray[i].def -> speed > 0) {
			float currentcosts = ((unittypearray[i].def -> metalCost * METAL2ENERGY) + unittypearray[i].def -> energyCost) * (enemyunitsoftype[i]);
			EnemyCostsByMoveType[unittypearray[i].def -> moveType] += currentcosts;
			totalcosts += currentcosts;
		}
	}

	for (int i = 0; i < ai -> pather -> NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] /= totalcosts;

		for (int c = 0; c < ai -> pather -> totalcells; c++) {
			ai -> dm -> ChokePointArray[c] += ai -> dm -> ChokeMapsByMovetype[i][c] * EnemyCostsByMoveType[i];
		}
	}
}


// 0: energy, 1: mex, 2: mmaker, 3: ground attackers, 4: defenses, 5: builders
float CUnitTable::GetScore(const UnitDef* unit) {
	// L("Getting Score for: " << unit -> humanName);

	// Ignore the cost now
	float Cost = (unit -> metalCost * METAL2ENERGY) + unit -> energyCost;
	float CurrentIncome = INCOMEMULTIPLIER * (ai -> cb -> GetEnergyIncome() + (ai -> cb -> GetMetalIncome() * METAL2ENERGY)) + ai -> cb -> GetCurrentFrame() / 2;
	float Hitpoints = unit->health;
	float Benefit = 0;
	float dps = 0;
	int unitcounter = 0;
	bool candevelop = false;

	float RandNum = ai -> math -> RandNormal(4, 3, 1) + 1;
	int category = GetCategory(unit);

	switch (category) {
		case CAT_ENERGY:
			Benefit = unit -> energyMake - unit -> energyUpkeep;
			if (unit -> windGenerator) {
				Benefit += ai->cb->GetMinWind();
			}
			if (unit -> tidalGenerator) {
				Benefit += ai -> cb -> GetTidalStrength();
			}

			if (unit -> needGeo)
				Benefit = 0;

			Benefit /= Cost;
			break;
		case CAT_MEX:
			Benefit = pow(unit -> extractsMetal, float(4));
			break;
		case CAT_MMAKER:
			Benefit = (unit -> metalMake - unit -> metalUpkeep) / unit -> energyUpkeep + 0.01;
			break;
		case CAT_G_ATTACK:
			dps = GetCurrentDamageScore(unit);

			if (unit -> canfly && !unit -> hoverAttack) {
				// Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
				dps /= 6;
			}

			Benefit = pow((unit -> weapons.front().def -> areaOfEffect + 80), float(1.5))
					* pow(GetMaxRange(unit) + 200, float(1.5))
					* pow(dps, float(1))
					* pow(unit -> speed + 40, float(1))
					* pow(Hitpoints, float(1))
					* pow(RandNum, float(2.5))
					* pow(Cost, float(-0.5));
			
			if (unit -> canfly) {
				 // AA hack: slight reduction to the feasability of air
				Benefit *= 0.25;
			}
			break;
		case CAT_DEFENCE:
			// L("Getting DEFENCE SCORE FOR: " << unit -> humanName);
			Benefit = pow((unit -> weapons.front().def -> areaOfEffect + 80), float(1.5))
					* pow(GetMaxRange(unit), float(2))
					* pow(GetCurrentDamageScore(unit), float(1.5))
					* pow(Hitpoints, float(0.5))
					* pow(RandNum, float(2.5))
					* pow(Cost, float(-1));
			break;
		case CAT_BUILDER:
			// L("Getting Score For Builder");
			for (unsigned int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++) {
				// L("category: " << unittypearray[unittypearray[unit -> id].canBuildList[i]].category);
				if (unittypearray[unittypearray[unit -> id].canBuildList[i]].category == CAT_FACTORY) {
					candevelop = true;
				}
			}

			Benefit = pow(unit -> buildSpeed, float(1))
					* pow(unit -> speed, float(0.5))
					* pow(Hitpoints, float(0.3))
					* pow(RandNum, float(0.4));

			if (!candevelop)
				Benefit = 0;
			break;
		case CAT_FACTORY:
			for (unsigned int i = 0; i != unittypearray[unit -> id].canBuildList.size(); i++) {
				if (unittypearray[unittypearray[unit -> id].canBuildList[i]].category == CAT_G_ATTACK) {
					Benefit += GetScore(unittypearray[unittypearray[unit -> id].canBuildList[i]].def);
					unitcounter++;
				}
			}
		
			if (unitcounter > 0)
				Benefit /= (unitcounter * pow(float(ai -> uh -> AllUnitsByType[unit -> id] -> size() + 1), float(3)));
			else
				Benefit = 0;
			// L("Factory: " << unit->humanName << " Has a score of: " << Benefit);
			break;
		case CAT_MSTOR:
			Benefit = pow((unit -> metalStorage), float(1)) * pow(Hitpoints, float(1));
			break;
		case CAT_ESTOR:
			Benefit = pow((unit -> energyStorage), float(1)) * pow(Hitpoints, float(1));
			break;
		default:
			Benefit = 0;
	}
	
	// L("Unit: " << unit -> humanName << " Has a score of : " << Benefit);
	float EScore = Benefit / (Cost + CurrentIncome);
	// L("EScore: " << EScore);
	return EScore;
}

// 0: energy, 1: mex, 2: mmaker, 3: ground attackers, 4: defenses, 5: builders
const UnitDef* CUnitTable::GetUnitByScore(int builder, int category) {
	ai -> math -> TimerStart();
	// L("Getting Score for category:" << category << " Builder: " << ai -> cb -> GetUnitDef(builder) -> humanName);
	vector<int>* templist;
	const UnitDef* tempunit;
	int side = GetSide(builder);
	float tempscore = 0;
	float bestscore = 0;

	// L("vars set");
	switch (category) {
		case CAT_ENERGY:
			templist = ground_energy;
			break;
		case CAT_MEX:
			templist = metal_extractors;
			break;
		case CAT_MMAKER:
			templist = metal_makers;
			break;
		case CAT_G_ATTACK:
			templist = ground_attackers;
			break;
		case CAT_DEFENCE:
			templist = ground_defences;
			break;
		case CAT_BUILDER:
			templist = ground_builders;
			break;
		case CAT_FACTORY:
			templist = ground_factories;
			break;
		case CAT_MSTOR:
			templist = metal_storages;
			break;
		case CAT_ESTOR:
			templist = energy_storages;
			break;
	}
	// L("Switch done, side: " << side);
	for (unsigned int i = 0; i != templist[side].size(); i++) {
		if (CanBuildUnit(ai -> cb -> GetUnitDef(builder) -> id, templist[side][i])) {
			tempscore = GetScore(unittypearray[templist[side][i]].def);
			if (bestscore < tempscore) {
				// L("Better Score found");
				bestscore = tempscore;
				tempunit = unittypearray[templist[side][i]].def;
			}
		}
	}

	// L("Loop calculated");
	if (!bestscore){
		// L("no scores found for unit!");
		for (unsigned int i = 0; i != unittypearray[ai -> cb -> GetUnitDef(builder) -> id].canBuildList.size(); i++) {
			if (unittypearray[unittypearray[ai -> cb -> GetUnitDef(builder) -> id].canBuildList[i]].category != -1) {
				return unittypearray[unittypearray[ai -> cb -> GetUnitDef(builder) -> id].canBuildList.front()].def;
			}
		}
	}

	// L("Time taken for getunitbyscore: " << ai -> math -> TimerSecs() << "s.");
	return tempunit;
}



/*
 * Find and return the unit that its best to make for this builder.
 * If no buildings/units are better that minUsefullnes, or it cant make any economy units, NULL is returned.
 * TODO: make it, and look at how to integrate it into the main economy manager
 * NOTE: this function will most likely be removed/replaced soon
 */
const UnitDef* CUnitTable::GetBestEconomyBuilding(int builder, float minUsefulness) {
	builder = builder;
	minUsefulness = minUsefulness;

	return NULL;
}




void CUnitTable::Init() {
	ai -> math -> TimerStart();
	numOfUnits = ai -> cb -> GetNumUnitDefs();

	// one more than needed because 0 is dummy object (so UnitDef -> id can be used to adress that unit in the array)
	unittypearray = new UnitType[numOfUnits + 1];
	unittypearray[0].side = -1;

	// get unit defs from game and stick them in the unittypearray[] array
	unitList = new const UnitDef*[numOfUnits];
	ai -> cb -> GetUnitDefList(unitList);

	for (int i = 1; i <= numOfUnits; i++) {
		unittypearray[i].def=unitList[i - 1];
	}

	// add units to UnitTable
	for (int i = 1; i <= numOfUnits; i++) {
		// side has not been assigned - will be done later
		unittypearray[i].side = -1;
		unittypearray[i].category = -1;

		// get build options
		for (map<int, string>::const_iterator j = unittypearray[i].def -> buildOptions.begin(); j != unittypearray[i].def -> buildOptions.end(); j++) {
			unittypearray[i].canBuildList.push_back(ai -> cb -> GetUnitDef(j -> second.c_str()) -> id);
		}
	}

	// now set the sides and create buildtree
	for (int s = 0; s < numOfSides; s++) {
		// set side of the start unit (eg commander) and continue recursively
		unittypearray[startUnits[s]].side = s;
		// L("Calcing buildtree of: " << startUnits[s]);
		CalcBuildTree(startUnits[s]);
	}

	// add unit to different groups
	for (int i = 1; i <= numOfUnits; i++) {
		UnitType* me = &unittypearray[i];
		// filter out neutral units
		if (me -> side == -1) {
		}
		else {
			int UnitCost = int(me -> def -> metalCost * METAL2ENERGY + me -> def -> energyCost);

			// L("name: " << me -> def -> name << ", : " << me -> def -> humanName);
			CSunParser* attackerparser = new CSunParser(ai);
			char weaponnumber[10] = "";

			attackerparser -> LoadVirtualFile(me -> def -> filename.c_str());
			// L(me -> def -> filename.c_str());
			me -> TargetCategories.resize(me -> def -> weapons.size());

			for (unsigned int w = 0; w != me -> def -> weapons.size(); w++) {
				itoa(w, weaponnumber, 10);
				// L("pre string ans;");
				string ans;
				
				attackerparser -> GetDef(me -> TargetCategories[w], "-1", string("UNITINFO\\OnlyTargetCategory") + string(weaponnumber));
			}

			delete attackerparser;

			me -> DPSvsUnit.resize(numOfUnits + 1);

			for (int v = 1; v <= numOfUnits; v++) {
				me -> DPSvsUnit[v] = GetDPSvsUnit(me -> def, unittypearray[v].def);
				// L(me -> def -> humanName << " has a DPS of : " << me -> DPSvsUnit[v] << " against: " << unittypearray[v].def -> humanName);
			}

			if (me -> def -> speed && me -> def -> minWaterDepth <= 0) {
				if (me -> def -> buildOptions.size()) {
					ground_builders[me -> side].push_back(i);
					me -> category = CAT_BUILDER;
				}
				else if (!me -> def -> weapons.empty() && !me -> def -> weapons.begin() -> def -> stockpile) {
					ground_attackers[me -> side].push_back(i);
					me -> category = CAT_G_ATTACK;
				}
			}



			else if (!me -> def -> canfly) {
				if (me -> def -> minWaterDepth <= 0) {
					if (me -> def -> buildOptions.size() > 1) {
						if ((((me -> def) -> TEDClassString) == "PLANT") || (((me -> def) -> speed) > 0.0f))
							(ai -> MyUnits[i]) -> isHub = false;
						else
							(ai -> MyUnits[i]) -> isHub = true;

						ground_factories[me -> side].push_back(i);
						me -> category = CAT_FACTORY;
					}
					else {
						if (!me -> def -> weapons.empty() && !me -> def -> weapons.begin() -> def -> stockpile) {
							ground_defences[me -> side].push_back(i);
							me -> category = CAT_DEFENCE;
						}
						if (me -> def -> makesMetal){
							metal_makers[me -> side].push_back(i);
							me -> category = CAT_MMAKER;
						}
						if (me -> def -> extractsMetal){
							metal_extractors[me -> side].push_back(i);
							me -> category = CAT_MEX;
						}
						if ((me -> def -> energyMake - me -> def -> energyUpkeep) / UnitCost > 0.002 || me -> def -> tidalGenerator || me -> def -> windGenerator) {
							if (me -> def -> minWaterDepth <= 0 && !me -> def -> needGeo) {
								ground_energy[me -> side].push_back(i);
								me -> category = CAT_ENERGY;
							}
						}
						if (me -> def -> energyStorage / UnitCost > 0.2) {
							energy_storages[me -> side].push_back(i);
							me -> category = CAT_ESTOR;
						}
						if (me -> def -> metalStorage / UnitCost > 0.1) {
							metal_storages[me -> side].push_back(i);
							me -> category = CAT_MSTOR;
						}

					}
				}
			}
		}
	}

	// L("All units added!");
	char k[256];
	sprintf(k, "UnitTable loaded in %fs", ai -> math -> TimerSecs());
	ai -> cb -> SendTextMsg(k, 0);
	// KLOOTNOTE: dump generated unit table to file
	this -> DebugPrint();
}





bool CUnitTable::CanBuildUnit(int id_builder, int id_unit) {
	// look in build options of builder for unit
	for (unsigned int i = 0; i != unittypearray[id_builder].canBuildList.size(); i++) {
		if (unittypearray[id_builder].canBuildList[i] == id_unit)
			return true;
	}

	// unit not found in builders buildoptions
	return false;
}

// determines sides of unittypearray by recursion
void CUnitTable::CalcBuildTree(int unit) {
	// go through all possible build options and set side if necessary
	for (unsigned int i = 0; i != unittypearray[unit].canBuildList.size(); i++) {
		// add this unit to targets builtby-list
		unittypearray[unittypearray[unit].canBuildList[i]].builtByList.push_back(unit);

		if (unittypearray[unittypearray[unit].canBuildList[i]].side == -1) {
			// unit has not been checked yet, set side as side of its builder and continue 
			unittypearray[unittypearray[unit].canBuildList[i]].side = unittypearray[unit].side;
			CalcBuildTree(unittypearray[unit].canBuildList[i]);
		}

		// if already checked end recursion
	}
}



void CUnitTable::DebugPrint() {
	if (!unitList)
		return;

	// for debugging
	char filename[1024] = ROOTFOLDER"CUnitTable Debug.log";
	ai -> cb -> GetValue(AIVAL_LOCATE_FILE_W, filename);
	FILE* file = fopen(filename, "w");

	for (int i = 1; i <= numOfUnits; i++) {
		if (unittypearray[i].side != -1) {
			fprintf(file, "ID: %i\nName:         %s \nSide:         %s",
				i, unitList[i - 1] -> humanName.c_str(), sideNames[unittypearray[i].side].c_str());

			fprintf(file, "\nCan Build:    ");

			for (unsigned int j = 0; j != unittypearray[i].canBuildList.size(); j++) {
				fprintf(file, "%s ", unittypearray[unittypearray[i].canBuildList[j]].def -> humanName.c_str());
			}

			fprintf(file, "\nBuilt by:     ");

			for (unsigned int k = 0; k != unittypearray[i].builtByList.size(); k++) {
				fprintf(file, "%s ", unittypearray[unittypearray[i].builtByList[k]].def -> humanName.c_str());
			}

			fprintf(file, "\n\n");
		}
	}

	for (int s = 0; s < numOfSides; s++) {
		for (unsigned int l = 0; l != all_lists.size(); l++) {
			fprintf(file, "\n\n%s:\n", sideNames[s].c_str());

			for (unsigned int i = 0; i != all_lists[l][s].size(); i++)
				fprintf(file, "%s\n", unittypearray[all_lists[l][s][i]].def -> humanName.c_str());
		}
	}

	fclose(file);
}


float CUnitTable::GetMaxRange(const UnitDef* unit) {
	float max_range = 0;

	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit -> weapons.begin(); i != unit -> weapons.end(); i++) {
		if ((i -> def -> range) > max_range)
			max_range = i -> def -> range;
	}

	return max_range;
}

float CUnitTable::GetMinRange(const UnitDef* unit) {
	float min_range = FLT_MAX;

	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit -> weapons.begin(); i != unit -> weapons.end(); i++) {
		if ((i -> def -> range) < min_range)
			min_range = i -> def -> range;
	}

	return min_range;
}
