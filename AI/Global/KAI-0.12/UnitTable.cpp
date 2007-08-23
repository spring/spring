// Based on Submarine's BuildTable Class from AAI. Thanks sub!

#include "UnitTable.h"

CUnitTable::CUnitTable(AIClasses* ai) {
	this->ai = ai;

	numOfUnits = 0;
	unitList = 0;

	// get all sides and commanders
	// KLOOTNOTE: might not work universally
	string sideStr = "SIDE";
	string errorString = "-1";
	string valueString;
	char k[64];
	ai->parser->LoadVirtualFile("gamedata\\SIDEDATA.tdf");

	// look at SIDE0 through SIDE9
	// (should be enough for any mod)
	for (int i = 0; i < 10; i++) {
		sprintf(k, "%i", i);
		ai->parser->GetDef(valueString, errorString, string(sideStr + k + "\\commander"));

		const UnitDef* udef = ai->cb->GetUnitDef(valueString.c_str());

		if (udef) {
			startUnits.push_back(udef->id);
			ai->parser->GetDef(valueString, errorString, string(sideStr + k + "\\name"));
			sideNames.push_back(valueString);
			numOfSides = i + 1;
		}
	}

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
	nuke_silos = new vector<int>[numOfSides];

	all_lists.push_back(ground_factories);
	all_lists.push_back(ground_builders);
	all_lists.push_back(ground_attackers);
	all_lists.push_back(metal_extractors);
	all_lists.push_back(metal_makers);
	all_lists.push_back(ground_energy);
	all_lists.push_back(ground_defences);
	all_lists.push_back(metal_storages);
	all_lists.push_back(energy_storages);
	all_lists.push_back(nuke_silos);
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
	delete[] nuke_silos;
}



int CUnitTable::GetSide(int unit) {
	assert(ai->cb->GetUnitDef(unit) != NULL);
	return unittypearray[ai->cb->GetUnitDef(unit)->id].side;
}

int CUnitTable::GetCategory(const UnitDef* unitdef) {
	return unittypearray[unitdef->id].category;
}

int CUnitTable::GetCategory(int unit) {
	assert(ai->cb->GetUnitDef(unit) != NULL);

	return (unittypearray[ai->cb->GetUnitDef(unit)->id].category);
}



// used to update threat-map
float CUnitTable::GetDPS(const UnitDef* unit) {
	if (unit) {
		float totaldps = 0.0f;

		for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
			float dps = 0.0f;
			if (!i->def->paralyzer) {
				int numberofdamages;
				ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);
				float reloadtime = i->def->reload;

				for (int k = 0; k < numberofdamages; k++) {
					dps += i->def->damages[k];
				}

				dps = dps * i->def->salvosize / numberofdamages / reloadtime;
			}

			totaldps += dps;
		}

		return totaldps;
	}

	return 0.0f;
}



float CUnitTable::GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim) {
	if (unit->weapons.size()) {
		ai->math->TimerStart();

		float dps = 0.0f;
		bool canhit = false;
		string targetcat;
		int armortype = victim->armorType;
		int numberofdamages;
		ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);

		for (unsigned int i = 0; i != unit->weapons.size(); i++) {
			if (!unit->weapons[i].def->paralyzer) {
				targetcat = unittypearray[unit->id].TargetCategories[i];

				unsigned int a = victim->category;
				unsigned int b = unit->weapons[i].def->onlyTargetCategory;	// what the weapon can target
				unsigned int c = unit->weapons[i].onlyTargetCat;			// what the unit accepts as this weapons target
//				unsigned int d = unit->weapons[i].badTargetCat;				// what the unit thinks this weapon must be used for (hmm ?)
				bool canWeaponTarget = (a & b) > 0;
				bool canUnitTarget = (a & c) > 0;							// how is this used?
//				bool badUnitTarget = (a & d) > 0;							// probably means that it has low priority

				canhit = (canWeaponTarget && canUnitTarget);

				if (!unit->weapons[i].def->waterweapon && ai->cb->GetUnitDefHeight(victim->id) - victim->waterline < 0) {
					// weapon cannot hit this sub
					canhit = false;
				}

				if (unit->weapons[i].def->waterweapon && victim->minWaterDepth == 0) {
					// anti-sub weapon cannot kill this unit
					canhit = false;
				}

				// bombers are useless against air
				if (unit->weapons[i].def->dropped && victim->canfly && unit->canfly && unit->wantedHeight <= victim->wantedHeight) {
					canhit = false;
				}

				if (canhit) {
					float accuracy = unit->weapons[i].def->accuracy * 2.8;

					if (victim->speed != 0) {
						accuracy *= 1 - (unit->weapons[i].def->targetMoveError);
					}

					float basedamage = unit->weapons[i].def->damages[armortype] * unit->weapons[i].def->salvosize / unit->weapons[i].def->reload;
					float AOE = unit->weapons[i].def->areaOfEffect * 0.7;
					float tohitprobability;
					float impactarea;
					float targetarea;
					float distancetravelled = 0.7f * unit->weapons[i].def->range;
					float firingangle;
					float gravity = -(ai->cb->GetGravity() * 900);
					float timetoarrive;
					float u = unit->weapons[i].def->projectilespeed * 30;

					if (unit->weapons[i].def->type == string("Cannon")) {
						// L("u = " << u << " distancetravelled * gravity) / (u * u) = " << (distancetravelled * gravity) / (u * u));
						float sinoid = (distancetravelled * gravity) / (u * u);
						sinoid = min(sinoid, 1.0f);
						firingangle = asin(sinoid) / 2;

						if (unit->highTrajectoryType == 1) {
							firingangle = (PI / 2) - firingangle;
						}

						float heightreached = pow(u * sin(firingangle), 2) / (2 * gravity);
						float halfd = distancetravelled / 2;
						distancetravelled = 2 * sqrt(halfd * halfd + heightreached * heightreached) * 1.1;
					}

					if ((victim->canfly && unit->weapons[i].def->selfExplode) || !victim->canfly) {
						impactarea = pow((accuracy * distancetravelled) + AOE, 2);
						targetarea = ((victim->xsize * 16) + AOE) * ((victim->ysize * 16) + AOE);
					} else {
						impactarea = pow((accuracy) * (0.7f * distancetravelled), 2);
						targetarea = (victim->xsize * victim->ysize * 256);
					}

					if (impactarea > targetarea) {
						tohitprobability = targetarea / impactarea;
					} else {
						tohitprobability = 1;
					}

					if (!unit->weapons[i].def->guided && unit->weapons[i].def->projectilespeed != 0 && victim->speed != 0 && unit->weapons[i].def->beamtime == 1) {
						if (unit->weapons[i].def->type == string("Cannon")) {
							timetoarrive = (2 * u * sin(firingangle)) / gravity;
						} else {
							timetoarrive = distancetravelled / (unit->weapons[i].def->projectilespeed * 30);
						}

						float shotwindow = sqrt(targetarea) / victim->speed * 1.3;

						if (shotwindow < timetoarrive) {
							tohitprobability *= shotwindow / timetoarrive;
						}
					}

					dps += basedamage * tohitprobability;
				}
			}
		}

		return dps;
	}

	return 0.0f;
}



float CUnitTable::GetCurrentDamageScore(const UnitDef* unit) {
	int enemies[MAXUNITS];
	int numofenemies = ai->cheat->GetEnemyUnits(enemies);
	vector<int> enemyunitsoftype;
	float score = 0.01f;
	float totalcost=0.01f;
	enemyunitsoftype.resize(ai->cb->GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < numofenemies; i++) {
		enemyunitsoftype[ai->cheat->GetUnitDef(enemies[i])->id]++;
	}

	for (unsigned int i = 1; i != enemyunitsoftype.size(); i++) {
		bool b1 = unittypearray[i].def->builder;
		bool b2 = enemyunitsoftype[i] > 0;
		bool b3 = unittypearray[i].side != -1;
	//	bool b4 = (!unit->speed && !unittypearray[i].def->speed);

	//	if (!b1 && b2 && b3 && !b4) {
		if (!b1 && b2 && b3) {
			float currentscore = 0.0f;
			float costofenemiesofthistype = ((unittypearray[i].def->metalCost * METAL2ENERGY) + unittypearray[i].def->energyCost) * enemyunitsoftype[i];
			currentscore = unittypearray[unit->id].DPSvsUnit[i] * costofenemiesofthistype;
			totalcost += costofenemiesofthistype;

			/*
			if (unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype > 0) {
				// L("Score of them vs me: " << unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
				currentscore -= (unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
			}*/

			score += currentscore;
		}
	}

	if (totalcost <= 0)
		return 0.0f;

	return score / totalcost;
}




void CUnitTable::UpdateChokePointArray() {
	vector<float> EnemyCostsByMoveType;
	EnemyCostsByMoveType.resize(ai->pather->NumOfMoveTypes);
	vector<int> enemyunitsoftype;
	float totalcosts = 1;
	int enemies[MAXUNITS];
	int numofenemies = ai->cheat->GetEnemyUnits(enemies);
	enemyunitsoftype.resize(ai->cb->GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < ai->pather->totalcells; i++) {
		ai->dm->ChokePointArray[i] = 0;
	}
	for (int i = 0; i < ai->pather->NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] = 0;
	}
	for (int i = 0; i < numofenemies; i++) {
		enemyunitsoftype[ai->cheat->GetUnitDef(enemies[i])->id]++;
	}

	for (unsigned int i = 1; i != enemyunitsoftype.size(); i++) {
		if (unittypearray[i].side != -1 && !unittypearray[i].def->canfly && unittypearray[i].def->speed > 0) {
			float currentcosts = ((unittypearray[i].def->metalCost * METAL2ENERGY) + unittypearray[i].def->energyCost) * (enemyunitsoftype[i]);
			EnemyCostsByMoveType[unittypearray[i].def->moveType] += currentcosts;
			totalcosts += currentcosts;
		}
	}

	for (int i = 0; i < ai->pather->NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] /= totalcosts;

		for (int c = 0; c < ai->pather->totalcells; c++) {
			ai->dm->ChokePointArray[c] += ai->dm->ChokeMapsByMovetype[i][c] * EnemyCostsByMoveType[i];
		}
	}
}






float CUnitTable::GetScore(const UnitDef* udef) {
	float Cost = ((udef->metalCost * METAL2ENERGY) + udef->energyCost) + 0.1f;
	float CurrentIncome = INCOMEMULTIPLIER * (ai->cb->GetEnergyIncome() + (ai->cb->GetMetalIncome() * METAL2ENERGY)) + ai->cb->GetCurrentFrame() / 2;
	float Hitpoints = udef->health;
	float buildTime = udef->buildTime + 0.1f;
	float Benefit = 0.0f;
	float dps = 0.0f;
	int unitcounter = 0;
	bool candevelop = false;

	float RandNum = ai->math->RandNormal(4, 3, 1) + 1;
	float randMult = float((rand() % 2) + 1);
	int category = GetCategory(udef);

	switch (category) {
		case CAT_ENERGY: {
			// KLOOTNOTE: factor build-time into this as well
			// (so benefit values generally lie closer together)
			// Benefit = (udef->energyMake - udef->energyUpkeep);
			// Benefit = (udef->energyMake - udef->energyUpkeep) * randMult;
			Benefit = ((udef->energyMake - udef->energyUpkeep) / buildTime) * randMult;

			if (udef->windGenerator) {
				Benefit += ai->cb->GetMinWind();
			}
			if (udef->tidalGenerator) {
				Benefit += ai->cb->GetTidalStrength();
			}

			// filter geothermals
			if (udef->needGeo)
				Benefit = 0.0f;

			// KLOOTNOTE: dividing by cost here as well means
			// benefit is inversely proportional to square of
			// cost, so expensive generators are quadratically
			// less likely to be built if original calculation
			// of score is used
			// Benefit /= Cost;
		} break;

		case CAT_MEX: {
			Benefit = pow(udef->extractsMetal, 4.0f);
		} break;
		case CAT_MMAKER: {
			Benefit = (udef->metalMake - udef->metalUpkeep) / udef->energyUpkeep + 0.01;
		} break;

		case CAT_G_ATTACK: {
			dps = GetCurrentDamageScore(udef);

			if (udef->canfly && !udef->hoverAttack) {
				// TODO: improve to set reload-time to the bomber's
				// turnspeed vs. movespeed (eg. time taken for a run)
				dps /= 6;
			}

			Benefit = pow((udef->weapons.front().def->areaOfEffect + 80), 1.5f)
					* pow(GetMaxRange(udef) + 200, 1.5f)
					* pow(dps, 1.0f)
					* pow(udef->speed + 40, 1.0f)
					* pow(Hitpoints, 1.0f)
					* pow(RandNum, 2.5f)
					* pow(Cost, -0.5f);

			if (udef->canfly || udef->canhover) {
				// AA hack: slight reduction to the feasability of aircraft
				// general hack: should mostly prefer real L2 units to hovers
				Benefit *= 0.01;
			}
		} break;

		case CAT_DEFENCE: {
			Benefit = pow((udef->weapons.front().def->areaOfEffect + 80), 1.5f)
					* pow(GetMaxRange(udef), 2.0f)
					* pow(GetCurrentDamageScore(udef), 1.5f)
					* pow(Hitpoints, 0.5f)
					* pow(RandNum, 2.5f)
					* pow(Cost, -1.0f);
		} break;

		case CAT_BUILDER: {
			for (unsigned int i = 0; i != unittypearray[udef->id].canBuildList.size(); i++) {
				if (unittypearray[unittypearray[udef->id].canBuildList[i]].category == CAT_FACTORY) {
					candevelop = true;
				}
			}

			// builder units that cannot construct any
			// factories are worthless, prevent them
			// from being chosen via GetUnitByScore()
			// (they might have other uses though, eg.
			// nano-towers)
			if (!candevelop) {
				Benefit = 0.0f;
			} else {
				Benefit = pow(udef->buildSpeed, 1.0f)
						* pow(udef->speed, 0.5f)
						* pow(Hitpoints, 0.3f)
						* pow(RandNum, 0.4f);
			}
		} break;

		case CAT_FACTORY: {
			// benefit of a factory is dependant on the kind of
			// offensive units it can build, but EE-hubs are only
			// capable of building other buildings
			for (unsigned int i = 0; i != unittypearray[udef->id].canBuildList.size(); i++) {
				int buildOption = unittypearray[udef->id].canBuildList[i];
				int buildOptionCategory = unittypearray[buildOption].category;

				if (buildOptionCategory == CAT_G_ATTACK || buildOptionCategory == CAT_FACTORY) {
					Benefit += GetScore(unittypearray[buildOption].def);
					unitcounter++;
				}
			}
		
			if (unitcounter > 0) {
				Benefit /= (unitcounter * pow(float(ai->uh->AllUnitsByType[udef->id]->size() + 1), 3.0f));
			} else {
				Benefit = 0.0f;
			}
		} break;

		case CAT_MSTOR: {
			Benefit = pow((udef->metalStorage), 1.0f) * pow(Hitpoints, 1.0f);
		} break;
		case CAT_ESTOR: {
			Benefit = pow((udef->energyStorage), 1.0f) * pow(Hitpoints, 1.0f);
		} break;
		case CAT_NUKE: {
			// KLOOTNOTE: should factor damage into this as well
			float metalcost = udef->stockpileWeaponDef->metalcost;
			float energycost = udef->stockpileWeaponDef->energycost;
			float supplycost = udef->stockpileWeaponDef->supplycost;
			float denom = metalcost + energycost + supplycost + 1.0f;
			float range = udef->stockpileWeaponDef->range;
			Benefit = (udef->stockpileWeaponDef->areaOfEffect + range) / denom;
		} break;
		/*
		case CAT_ANTINUKE: {
			Benefit = udef->stockpileWeaponDef->coverageRange;
		} break;
		case CAT_SHIELD: {
			Benefit = udef->shieldWeaponDef->shieldRadius;
		} break;
		*/
		default:
			Benefit = 0.0f;
	}

	// return (Benefit / (CurrentIncome + Cost));
	// return ((Benefit / Cost) * CurrentIncome);
	return ((CurrentIncome / Cost) * Benefit);
}



// operates in terms of GetScore()
const UnitDef* CUnitTable::GetUnitByScore(int builderUnitID, int category) {
	vector<int>* templist;
	const UnitDef* builderDef = ai->cb->GetUnitDef(builderUnitID);
	const UnitDef* tempUnitDef;
	int side = GetSide(builderUnitID);
	float tempscore = 0.0f;
	float bestscore = 0.0f;

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
		case CAT_NUKE:
			templist = nuke_silos;
			break;
	}

	// iterate over all units for <side> in templist (eg. Core ground_defences)
	for (unsigned int i = 0; i != templist[side].size(); i++) {
		int tempUnitDefID = templist[side][i];

		// if our builder can build the i-th unit
		if (CanBuildUnit(builderDef->id, tempUnitDefID)) {
			// get the unit's heuristic score (based on current income)
			tempscore = GetScore(unittypearray[tempUnitDefID].def);

			if (tempscore > bestscore) {
				bestscore = tempscore;
				tempUnitDef = unittypearray[tempUnitDefID].def;
			}
		}
	}

	// if we didn't find a unit to build with score > 0 (ie.
	// if builder has no build-option matching this category)
	// then return NULL instead of first option on build menu
	// (to prevent radar farms and other bizarro side-effects)
	return ((bestscore > 0.0f)? tempUnitDef: NULL);
}






/*
 * find and return the unit that's best to make for this builder
 * (returns NULL if no buildings/units are better than minUsefulness
 * or buidler can't make any economy units)
 * TODO: make it, look at how to integrate into the main economy manager
 */
/*
const UnitDef* CUnitTable::GetBestEconomyBuilding(int builder, float minUsefulness) {
	builder = builder;
	minUsefulness = minUsefulness;

	return NULL;
}
*/




void CUnitTable::Init() {
	numOfUnits = ai->cb->GetNumUnitDefs();

	// one more than needed because 0 is dummy object (so
	// UnitDef->id can be used to adress that unit in array)
	unittypearray = new UnitType[numOfUnits + 1];
	unittypearray[0].side = -1;

	// get unit defs from game and stick them in the unittypearray[] array
	unitList = new const UnitDef*[numOfUnits];
	ai->cb->GetUnitDefList(unitList);

	for (int i = 1; i <= numOfUnits; i++) {
		unittypearray[i].def = unitList[i - 1];
	}

	// add units to UnitTable
	for (int i = 1; i <= numOfUnits; i++) {
		// side has not been assigned - will be done later
		unittypearray[i].side = -1;
		unittypearray[i].category = -1;

		// get build options
		for (map<int, string>::const_iterator j = unittypearray[i].def->buildOptions.begin(); j != unittypearray[i].def->buildOptions.end(); j++) {
			unittypearray[i].canBuildList.push_back(ai->cb->GetUnitDef(j->second.c_str())->id);
		}
	}


	// now set sides and create buildtree for each
	for (int s = 0; s < numOfSides; s++) {
		// set side of start unit (eg. commander) and continue recursively
		unittypearray[startUnits[s]].side = s;
		unittypearray[startUnits[s]].techLevel = 0;

		CalcBuildTree(startUnits[s]);
	}


	// add unit to different groups
	for (int i = 1; i <= numOfUnits; i++) {
		UnitType* me = &unittypearray[i];

		// filter out neutral units
		if (me->side != -1) {
			int UnitCost = int(me->def->metalCost * METAL2ENERGY + me->def->energyCost);

			CSunParser* attackerparser = new CSunParser(ai);
			char weaponnumber[10] = "";

			attackerparser->LoadVirtualFile(me->def->filename.c_str());
			me->TargetCategories.resize(me->def->weapons.size());

			for (unsigned int w = 0; w != me->def->weapons.size(); w++) {
				itoa(w, weaponnumber, 10);
				string ans;
				attackerparser->GetDef(me->TargetCategories[w], "-1", string("UNITINFO\\OnlyTargetCategory") + string(weaponnumber));
			}

			delete attackerparser;
			me->DPSvsUnit.resize(numOfUnits + 1);

			// calculate this unit type's DPS against all other unit types
			for (int v = 1; v <= numOfUnits; v++) {
				me->DPSvsUnit[v] = GetDPSvsUnit(me->def, unittypearray[v].def);
			}

			if (me->def->speed && me->def->minWaterDepth <= 0) {
				if (me->def->buildOptions.size()) {
					ground_builders[me->side].push_back(i);
					me->category = CAT_BUILDER;
				}
				else if (!me->def->weapons.empty() && !me->def->weapons.begin()->def->stockpile) {
					ground_attackers[me->side].push_back(i);
					me->category = CAT_G_ATTACK;
				}
			}



			else if (!me->def->canfly) {
				if (me->def->minWaterDepth <= 0) {
					if (me->def->buildOptions.size() >= 1 && me->def->builder) {
						if ((((me->def)->TEDClassString) == "PLANT") || (((me->def)->speed) > 0.0f)) {
							me->isHub = false;
						} else {
							me->isHub = true;
						}

						ground_factories[me->side].push_back(i);
						me->category = CAT_FACTORY;
					}
					else {
						const WeaponDef* weapon = (me->def->weapons.empty())? 0: me->def->weapons.begin()->def;

						if (weapon && !weapon->stockpile) {
							if (!weapon->waterweapon) {
								// filter out depth-charge launchers etc
								ground_defences[me->side].push_back(i);
								me->category = CAT_DEFENCE;
							}
						}

						if (me->def->stockpileWeaponDef) {
							if (me->def->stockpileWeaponDef->targetable) {
								// nuke
								nuke_silos[me->side].push_back(i);
								me->category = CAT_NUKE;
							}
							if (me->def->stockpileWeaponDef->interceptor) {
								// anti-nuke, not implemented yet
							}
						}

						if (me->def->shieldWeaponDef && me->def->shieldWeaponDef->isShield) {
							// shield, not implemented yet
							// me->category = CAT_SHIELD;
						}

						if (me->def->makesMetal) {
							metal_makers[me->side].push_back(i);
							me->category = CAT_MMAKER;
						}
						if (me->def->extractsMetal > 0.0f) {
							metal_extractors[me->side].push_back(i);
							me->category = CAT_MEX;
						}
						if ((me->def->energyMake - me->def->energyUpkeep) / UnitCost > 0.002 || me->def->tidalGenerator || me->def->windGenerator) {
							if (me->def->minWaterDepth <= 0 && !me->def->needGeo) {
								// filter tidals and geothermals
								ground_energy[me->side].push_back(i);
								me->category = CAT_ENERGY;
							}
						}
						if (me->def->energyStorage / UnitCost > 0.2) {
							energy_storages[me->side].push_back(i);
							me->category = CAT_ESTOR;
						}
						if (me->def->metalStorage / UnitCost > 0.1) {
							metal_storages[me->side].push_back(i);
							me->category = CAT_MSTOR;
						}
					}
				}
			}
		}
	}

	// KLOOTNOTE: dump generated unit table to file
	this->DebugPrint();
}





bool CUnitTable::CanBuildUnit(int id_builder, int id_unit) {
	// look in build options of builder for unit
	for (unsigned int i = 0; i != unittypearray[id_builder].canBuildList.size(); i++) {
		if (unittypearray[id_builder].canBuildList[i] == id_unit)
			return true;
	}

	// unit not found in builder's buildoptions
	return false;
}

// determines sides of unittypearray by recursion
void CUnitTable::CalcBuildTree(int unit) {
	UnitType* utype = &unittypearray[unit];

	// go through all possible build options and set side if necessary
	for (unsigned int i = 0; i != utype->canBuildList.size(); i++) {
		// add this unit to target's built-by list
		int buildOptionIndex = utype->canBuildList[i];
		UnitType* buildOptionType = &unittypearray[buildOptionIndex];

		// KLOOTNOTE: techLevel will not make much sense if
		// unit has multiple ancestors at different depths
		// in tree (eg. Adv. Vehicle Plants in XTA)
		buildOptionType->builtByList.push_back(unit);
		buildOptionType->techLevel = utype->techLevel + 1;

		if (buildOptionType->side == -1) {
			// unit has not been checked yet, set side
			// as side of its builder and continue
			buildOptionType->side = utype->side;
			CalcBuildTree(buildOptionIndex);
		}
	}
}



void CUnitTable::DebugPrint() {
	if (!unitList)
		return;

	// NOTE: same order as all_lists, not as CAT_* enum
	const char* listCategoryNames[12] = {
		"GROUND-FACTORY", "GROUND-BUILDER", "GROUND-ATTACKER", "METAL-EXTRACTOR",
		"METAL-MAKER", "GROUND-ENERGY", "GROUND-DEFENSE", "METAL-STORAGE",
		"ENERGY-STORAGE", "NUKE-SILO", "SHIELD-GENERATOR", "LAST-CATEGORY"
	};

	// for debugging
	char filename[1024] = ROOTFOLDER"CUnitTable.log";
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);
	FILE* file = fopen(filename, "w");

	for (int i = 1; i <= numOfUnits; i++) {
		if (unittypearray[i].side != -1) {
			fprintf(file, "UnitDef ID: %i\n", i);
			fprintf(file, "Name:       %s\n", unitList[i - 1]->humanName.c_str());
			fprintf(file, "Side:       %s\n", sideNames[unittypearray[i].side].c_str());
			fprintf(file, "Can Build:  ");

			for (unsigned int j = 0; j != unittypearray[i].canBuildList.size(); j++) {
				fprintf(file, "'%s' ", unittypearray[unittypearray[i].canBuildList[j]].def->humanName.c_str());
			}

			fprintf(file, "\n");
			fprintf(file, "Built by:   ");

			for (unsigned int k = 0; k != unittypearray[i].builtByList.size(); k++) {
				fprintf(file, "'%s' ", unittypearray[unittypearray[i].builtByList[k]].def->humanName.c_str());
			}

			fprintf(file, "\nTech-Level: %d", unittypearray[i].techLevel);
			fprintf(file, "\n\n");
		}
	}

	for (int s = 0; s < numOfSides; s++) {
		for (unsigned int l = 0; l != all_lists.size(); l++) {
			fprintf(file, "\n\n%s units of category %s:\n", sideNames[s].c_str(), listCategoryNames[l]);

			for (unsigned int i = 0; i != all_lists[l][s].size(); i++)
				fprintf(file, "\t%s\n", unittypearray[all_lists[l][s][i]].def->humanName.c_str());
		}
	}

	fclose(file);
}




float CUnitTable::GetMaxRange(const UnitDef* unit) {
	float max_range = 0.0f;

	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
		if ((i->def->range) > max_range)
			max_range = i->def->range;
	}

	return max_range;
}

float CUnitTable::GetMinRange(const UnitDef* unit) {
	float min_range = FLT_MAX;

	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
		if ((i->def->range) < min_range)
			min_range = i->def->range;
	}

	return min_range;
}
