// Based on Submarine's BuildTable Class from AAI. Thanks sub!

#include "UnitTable.h"

#include "System/GlobalStuff.h"
#include "System/Util.h"

/// CR_BIND(CUnitTable, );
/// CR_REG_METADATA(CUnitTable, (
/// 	CR_MEMBER(all_lists),
/// 	CR_MEMBER(ground_factories),
/// 	CR_MEMBER(ground_builders),
/// 	CR_MEMBER(ground_attackers),
/// 	CR_MEMBER(metal_extractors),
/// 	CR_MEMBER(metal_makers),
/// 	CR_MEMBER(ground_energy),
/// 	CR_MEMBER(ground_defences),
/// 	CR_MEMBER(metal_storages),
/// 	CR_MEMBER(energy_storages),
/// 	CR_MEMBER(nuke_silos),
/// 
/// 	CR_MEMBER(numOfSides),
/// 	CR_MEMBER(sideNames),
/// 	CR_MEMBER(modSideMap),
/// 	CR_MEMBER(teamSides),
/// 
/// 	CR_MEMBER(unitTypes)
/// ));


CUnitTable::CUnitTable(AIClasses* ai) {
	this->ai = ai;

	numOfUnits = 0;
	unitList = 0;

	BuildModSideMap();
	ReadTeamSides();

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

	all_lists.push_back(ground_factories);	// CAT_FACTORY  (idx: 0, cat enum:  7)
	all_lists.push_back(ground_builders);	// CAT_BUILDER  (idx: 1, cat enum:  4)
	all_lists.push_back(ground_attackers);	// CAT_G_ATTACK (idx: 2, cat enum:  9)
	all_lists.push_back(metal_extractors);	// CAT_MEX      (idx: 3, cat enum:  2)
	all_lists.push_back(metal_makers);		// CAT_MMAKER   (idx: 4, cat enum:  3)
	all_lists.push_back(ground_energy);		// CAT_ENERGY   (idx: 5, cat enum:  1)
	all_lists.push_back(ground_defences);	// CAT_DEFENCE  (idx: 6, cat enum:  8)
	all_lists.push_back(metal_storages);	// CAT_MSTOR    (idx: 7, cat enum:  6)
	all_lists.push_back(energy_storages);	// CAT_ESTOR    (idx: 8, cat enum:  5)
	all_lists.push_back(nuke_silos);		// CAT_NUKE     (idx: 9, cat enum: 10)
}

CUnitTable::~CUnitTable() {
	delete[] unitTypes;
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


int CUnitTable::BuildModSideMap() {
	// get all sides and commanders
	std::string commKey;	// eg. "SIDE4\\commander"
	std::string commName;	// eg. "arm_commander"
	std::string sideKey;	// eg. "SIDE4\\name"
	std::string sideName;	// eg. "Arm"
	char sideNum[64] = {0};

	ai->parser->LoadVirtualFile("gamedata\\SIDEDATA.tdf");

	// look at SIDE0 through SIDE9
	// (should be enough for any mod)
	for (int i = 0; i < 10; i++) {
		sprintf(sideNum, "%i", i);

		commKey = "SIDE" + std::string(sideNum) + "\\commander";
		sideKey = "SIDE" + std::string(sideNum) + "\\name";

		ai->parser->GetDef(commName, "-1", commKey);
		const UnitDef* udef = ai->cb->GetUnitDef(commName.c_str());

		if (udef) {
			// if this unit exists, the side is valid too
			startUnits.push_back(udef->id);
			ai->parser->GetDef(sideName, "-1", sideKey);

			// transform the side string to lower-case
			StringToLowerInPlace(sideName);

			sideNames.push_back(sideName);
			modSideMap[sideName] = i;
			numOfSides = i + 1;
		}
	}

	return numOfSides;
}

int CUnitTable::ReadTeamSides() {
	teamSides.resize(MAX_TEAMS, 0);
	teamSides[0] = 0;	// team 0 defaults to side 0 (in GlobalAI startscript)
	teamSides[1] = 1;	// team 1 defaults to side 1 (in GlobalAI startscript)

	for (int i = 0; i < MAX_TEAMS; i++) {
		const char* sideKey = ai->cb->GetTeamSide(i);

		if (sideKey) {
			// FIXME: Gaia-team side?
			// team index was valid (and we are in a GameSetup-type
			// game), override the default side index for this team
			teamSides[i] = modSideMap[sideKey];
		}
	}

	return teamSides[ai->cb->GetMyTeam()];
}

// called at the end of Init()
void CUnitTable::ReadModConfig() {
	const char* modName = ai->cb->GetModName();
	char configFileName[1024] = {0};
	char logMsg[2048] = {0};
	snprintf(configFileName, 1023, "%s%s.cfg", CFGFOLDER, modName);
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, configFileName);

	FILE* f = fopen(configFileName, "r");

	if (f) {
		// read the mod's .cfg file
		char str[1024];
		char name[512];
		float costMult = 1.0f;
		int techLvl = -1;
		int category = -1;

		while (fgets(str, 1024, f) != 0x0) {
			if (str[0] == '/' && str[1] == '/') {
				continue;
			}

			int i = sscanf(str, "%s %f %d %d", name, &costMult, &techLvl, &category);
			const UnitDef* udef = ai->cb->GetUnitDef(name);

			if ((i == 4) && udef) {
				UnitType* utype = &unitTypes[udef->id];
				utype->costMultiplier = costMult;
				utype->techLevel = techLvl;

				// TODO: look for any possible side-effects that might arise
				// from overriding categories like this, then enable overrides
				// other than builder --> attacker
				// FIXME: SEGV when unarmed CAT_BUILDER units masquerading as
				// CAT_G_ATTACK'ers want to or are attacked
				if (category >= 0 && category < LASTCATEGORY) {
					if (category == CAT_G_ATTACK && utype->category == CAT_BUILDER) {
						// maps unit categories to indices into all_lists
						// FIXME: hackish, poorly maintainable, bad style
						int catLstIdx[11] = {0, 5, 3, 4, 1, 8, 7, 0, 6, 2, 9};

						// index of sublist (eg. ground_builders) that ::Init() thinks it belongs to
						int idx1 = catLstIdx[utype->category];
						// index of sublist (eg. ground_attackers) that mod .cfg says it belongs to
						int idx2 = catLstIdx[category];

						if (idx1 != idx2) {
							std::vector<int>* oldLst = all_lists[idx1];	// old category list
							std::vector<int>* newLst = all_lists[idx2];	// new category list
							std::set<int>::iterator sit;
							std::vector<int>::iterator vit;

							for (sit = utype->sides.begin(); sit != utype->sides.end(); sit++) {
								int side = *sit;

								for (vit = oldLst[side].begin(); vit != oldLst[side].end(); vit++) {
									int udefID = *vit;

									if (udefID == udef->id) {
										oldLst[side].erase(vit);
										newLst[side].push_back(udef->id);
										vit--;
									}
								}
							}

							utype->category = category;
						}
					}
				}
			}
		}

		sprintf(logMsg, "read mod configuration file %s", configFileName);
	} else {
		// write a new .cfg file with default values
		f = fopen(configFileName, "w");
		fprintf(f, "// unitName costMultiplier techLevel category\n");

		for (int i = 1; i <= numOfUnits; i++) {
			UnitType* utype = &unitTypes[i];
			// assign and write default values for costMultiplier
			// and techLevel, category is already set in ::Init()
			utype->costMultiplier = 1.0f;
			utype->techLevel = -1;
			fprintf(f, "%s %.2f %d %d\n", utype->def->name.c_str(), utype->costMultiplier, utype->techLevel, utype->category);
		}

		sprintf(logMsg, "wrote mod configuration file %s", configFileName);
	}

	ai->cb->SendTextMsg(logMsg, 0);
	fclose(f);
}



int CUnitTable::GetSide(int unitID) {
	int team = ai->cb->GetUnitTeam(unitID);
	int side = teamSides[team];

	return side;
}

int CUnitTable::GetCategory(const UnitDef* unitdef) {
	return unitTypes[unitdef->id].category;
}

int CUnitTable::GetCategory(int unit) {
	assert(ai->cb->GetUnitDef(unit) != NULL);

	return (unitTypes[ai->cb->GetUnitDef(unit)->id].category);
}



// used to update threat-map, should probably
// use cost multipliers too (but in that case
// non-squad units like Flashes could become
// artifically overrated by a massive amount)
float CUnitTable::GetDPS(const UnitDef* unit) {
	if (unit) {
		float totaldps = 0.0f;

		for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
			float dps = 0.0f;

			if (!i->def->paralyzer) {
				float reloadtime = i->def->reload;
				int numberofdamages;
				ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);

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
		int armortype = victim->armorType;
		int numberofdamages = 0;
		ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numberofdamages);

		for (unsigned int i = 0; i != unit->weapons.size(); i++) {
			if (!unit->weapons[i].def->paralyzer) {
				unsigned int a = victim->category;
				unsigned int b = unit->weapons[i].def->onlyTargetCategory;	// what the weapon can target
				unsigned int c = unit->weapons[i].onlyTargetCat;			// what the unit accepts as this weapons target
//				unsigned int d = unit->weapons[i].badTargetCat;				// what the unit thinks this weapon must be used for (?)
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
					float tohitprobability = 0.0f;
					float impactarea = 0.0f;
					float targetarea = 0.0f;
					float distancetravelled = 0.7f * unit->weapons[i].def->range;
					float firingangle = 0.0f;
					float gravity = -(ai->cb->GetGravity() * 900);
					float timetoarrive = 0.0f;
					float u = unit->weapons[i].def->projectilespeed * 30;

					if (unit->weapons[i].def->type == string("Cannon")) {
						float sinoid = (distancetravelled * gravity) / (u * u);
						sinoid = std::min(sinoid, 1.0f);
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

					if (unit->weapons[i].def->turnrate == 0.0f && unit->weapons[i].def->projectilespeed != 0 && victim->speed != 0 && unit->weapons[i].def->beamtime == 1) {
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
	int enemies[MAX_UNITS];
	int numEnemies = ai->cheat->GetEnemyUnits(enemies);
	vector<int> enemiesOfType;
	float score = 0.01f;
	float totalCost = 0.01f;
	enemiesOfType.resize(ai->cb->GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < numEnemies; i++) {
		const UnitDef* udef = ai->cheat->GetUnitDef(enemies[i]);

		if (udef) {
			enemiesOfType[udef->id]++;
		}
	}

	for (unsigned int i = 1; i < enemiesOfType.size(); i++) {
		bool b1 = unitTypes[i].def->builder;
		bool b2 = (enemiesOfType[i] > 0);
		bool b3 = (unitTypes[i].sides.size() > 0);
		// bool b4 = (!unit->speed && !unitTypes[i].def->speed);

		if (!b1 && b2 && b3 /* && !b4 */) {
			float currentScore = 0.0f;
			float costOfEnemiesOfThisType = ((unitTypes[i].def->metalCost * METAL2ENERGY) + unitTypes[i].def->energyCost) * enemiesOfType[i];
			currentScore = unitTypes[unit->id].DPSvsUnit[i] * costOfEnemiesOfThisType;
			totalCost += costOfEnemiesOfThisType;

			/*
			if (unitTypes[i].DPSvsUnit[unit->id] * costofenemiesofthistype > 0) {
				currentscore -= (unitTypes[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
			}
			*/

			score += currentScore;
		}
	}

	if (totalCost <= 0)
		return 0.0f;

	return (score / totalCost);
}




void CUnitTable::UpdateChokePointArray() {
	vector<float> EnemyCostsByMoveType;
	EnemyCostsByMoveType.resize(ai->pather->NumOfMoveTypes);
	vector<int> enemiesOfType;
	float totalCost = 1.0f;
	int enemies[MAX_UNITS];
	int numEnemies = ai->cheat->GetEnemyUnits(enemies);
	enemiesOfType.resize(ai->cb->GetNumUnitDefs() + 1, 0);

	for (int i = 0; i < ai->pather->totalcells; i++) {
		ai->dm->ChokePointArray[i] = 0;
	}
	for (int i = 0; i < ai->pather->NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] = 0;
	}
	for (int i = 0; i < numEnemies; i++) {
		enemiesOfType[ai->cheat->GetUnitDef(enemies[i])->id]++;
	}

	for (unsigned int i = 1; i < enemiesOfType.size(); i++) {
		if (unitTypes[i].sides.size() > 0 && !unitTypes[i].def->canfly && unitTypes[i].def->speed > 0) {
			float currentcosts = ((unitTypes[i].def->metalCost * METAL2ENERGY) + unitTypes[i].def->energyCost) * (enemiesOfType[i]);
			EnemyCostsByMoveType[unitTypes[i].def->moveType] += currentcosts;
			totalCost += currentcosts;
		}
	}

	for (int i = 0; i < ai->pather->NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] /= totalCost;

		for (int c = 0; c < ai->pather->totalcells; c++) {
			ai->dm->ChokePointArray[c] += ai->dm->ChokeMapsByMovetype[i][c] * EnemyCostsByMoveType[i];
		}
	}
}






float CUnitTable::GetScore(const UnitDef* udef, int category) {
	int m = (ai->uh->AllUnitsByType[udef->id]).size();
	int n = udef->maxThisUnit;

	if (m >= n) {
		// if we've hit the build-limit for this
		// type of unit, make sure GetUnitByScore()
		// won't pick it for construction anyway
		return 0.0f;
	}

	if (udef->minWaterDepth > 0) {
		// we can't swim yet
		return 0.0f;
	}

	int frame = ai->cb->GetCurrentFrame();
	float Cost = ((udef->metalCost * METAL2ENERGY) + udef->energyCost) + 0.1f;
	float CurrentIncome = INCOMEMULTIPLIER * (ai->cb->GetEnergyIncome() + (ai->cb->GetMetalIncome() * METAL2ENERGY)) + frame / 2;
	float Hitpoints = udef->health;
	float buildTime = udef->buildTime + 0.1f;
	float benefit = 0.0f;
	float aoe = 0.0f;
	float dps = 0.0f;
	int unitcounter = 0;
	bool candevelop = false;

	float RandNum = ai->math->RandNormal(4, 3, 1) + 1;
	float randMult = float((rand() % 2) + 1);

	switch (category) {
		case CAT_ENERGY: {
			// KLOOTNOTE: factor build-time into this as well
			// (so benefit values generally lie closer together)
			// benefit = (udef->energyMake - udef->energyUpkeep);
			// benefit = (udef->energyMake - udef->energyUpkeep) * randMult;
			benefit = ((udef->energyMake - udef->energyUpkeep) / buildTime) * randMult;

			if (udef->windGenerator) {
				const float minWind = ai->cb->GetMinWind();
				const float maxWind = ai->cb->GetMaxWind();
				const float avgWind = (minWind + maxWind) * 0.5f;
				if (minWind >= 8.0f || (minWind >= 4.0f && avgWind >= 8.0f)) {
					benefit += avgWind;
				}
			}
			if (udef->tidalGenerator) {
				benefit += ai->cb->GetTidalStrength();
			}

			// filter geothermals
			if (udef->needGeo) {
				benefit = 0.0f;
			}

			// KLOOTNOTE: dividing by cost here as well means
			// benefit is inversely proportional to square of
			// cost, so expensive generators are quadratically
			// less likely to be built if original calculation
			// of score is used
			// benefit /= Cost;
		} break;

		case CAT_MEX: {
			benefit = pow(udef->extractsMetal, 4.0f);
		} break;
		case CAT_MMAKER: {
			// benefit = ((udef->metalMake - udef->metalUpkeep) / udef->energyUpkeep) + 0.01f;
			benefit = (udef->metalMake - udef->metalUpkeep) / (udef->energyUpkeep + 0.01f);
		} break;

		case CAT_G_ATTACK: {
			dps = GetCurrentDamageScore(udef);
			aoe = ((udef->weapons.size())? ((udef->weapons.front()).def)->areaOfEffect: 0.0f);

			if (udef->canfly && !udef->hoverAttack) {
				// TODO: improve to set reload-time to the bomber's
				// turnspeed vs. movespeed (eg. time taken for a run)
				dps /= 6;
			}

			benefit = pow((aoe + 80), 1.5f)
					* pow(GetMaxRange(udef) + 200, 1.5f)
					* pow(dps, 1.0f)
					* pow(udef->speed + 40, 1.0f)
					* pow(Hitpoints, 1.0f)
					* pow(RandNum, 2.5f)
					* pow(Cost, -0.5f);

			if (udef->canfly || udef->canhover) {
				// general hack: reduce feasibility of aircraft for 20 mins
				// and that of hovercraft permanently, should mostly prefer
				// real L2 units to hovers
				benefit = (udef->canfly && frame >= (30 * 60 * 20))? benefit: benefit * 0.01f;
			}
		} break;

		case CAT_DEFENCE: {
			aoe = ((udef->weapons.size())? ((udef->weapons.front()).def)->areaOfEffect: 0.0f);
			benefit = pow((aoe + 80), 1.5f)
					* pow(GetMaxRange(udef), 2.0f)
					* pow(GetCurrentDamageScore(udef), 1.5f)
					* pow(Hitpoints, 0.5f)
					* pow(RandNum, 2.5f)
					* pow(Cost, -1.0f);
		} break;

		case CAT_BUILDER: {
			for (unsigned int i = 0; i != unitTypes[udef->id].canBuildList.size(); i++) {
				if (unitTypes[unitTypes[udef->id].canBuildList[i]].category == CAT_FACTORY) {
					candevelop = true;
				}
			}

			// builder units that cannot construct any
			// factories are worthless, prevent them
			// from being chosen via GetUnitByScore()
			// (they might have other uses though, eg.
			// nano-towers)
			if (!candevelop) {
				benefit = 0.0f;
			} else {
				benefit = pow(udef->buildSpeed, 1.0f)
						* pow(udef->speed, 0.5f)
						* pow(Hitpoints, 0.3f)
						* pow(RandNum, 0.4f);
			}
		} break;

		case CAT_FACTORY: {
			// benefit of a factory is dependant on the kind of
			// offensive units it can build, but EE-hubs are only
			// capable of building other buildings
			for (unsigned int i = 0; i != unitTypes[udef->id].canBuildList.size(); i++) {
				int buildOption = unitTypes[udef->id].canBuildList[i];
				int buildOptionCategory = unitTypes[buildOption].category;

				if (buildOptionCategory == CAT_G_ATTACK || buildOptionCategory == CAT_FACTORY) {
					if (unitTypes[buildOption].def != udef) {
						// KLOOTNOTE: guard against infinite recursion (BuildTowers in
						// PURE trigger this since they are able to build themselves)
						benefit += GetScore(unitTypes[buildOption].def, buildOptionCategory);
						unitcounter++;
					}
				}
			}

			if (unitcounter > 0) {
				benefit /= (unitcounter * pow(float(ai->uh->AllUnitsByType[udef->id].size() + 1), 3.0f));
			} else {
				benefit = 0.0f;
			}
		} break;

		case CAT_MSTOR: {
			benefit = pow((udef->metalStorage), 1.0f) * pow(Hitpoints, 1.0f);
		} break;
		case CAT_ESTOR: {
			benefit = pow((udef->energyStorage), 1.0f) * pow(Hitpoints, 1.0f);
		} break;
		case CAT_NUKE: {
			// KLOOTNOTE: should factor damage into this as well
			float metalcost = udef->stockpileWeaponDef->metalcost;
			float energycost = udef->stockpileWeaponDef->energycost;
			float supplycost = udef->stockpileWeaponDef->supplycost;
			float denom = metalcost + energycost + supplycost + 1.0f;
			float range = udef->stockpileWeaponDef->range;
			benefit = (udef->stockpileWeaponDef->areaOfEffect + range) / denom;
		} break;
		/*
		case CAT_ANTINUKE: {
			benefit = udef->stockpileWeaponDef->coverageRange;
		} break;
		case CAT_SHIELD: {
			benefit = udef->shieldWeaponDef->shieldRadius;
		} break;
		*/
		default:
			benefit = 0.0f;
	}

	benefit *= unitTypes[udef->id].costMultiplier;
	// return (benefit / (CurrentIncome + Cost));
	// return ((benefit / Cost) * CurrentIncome);
	return ((CurrentIncome / Cost) * benefit);
}



// operates in terms of GetScore() (which is recursive for factories)
const UnitDef* CUnitTable::GetUnitByScore(int builderUnitID, int category) {
	if (category == LASTCATEGORY)
		return 0x0;

	vector<int>* tempList = 0;
	const UnitDef* builderDef = ai->cb->GetUnitDef(builderUnitID);
	const UnitDef* tempUnitDef = 0;
	int side = GetSide(builderUnitID);
	float tempScore = 0.0f;
	float bestScore = 0.0f;

	switch (category) {
		case CAT_ENERGY:
			tempList = ground_energy;
			break;
		case CAT_MEX:
			tempList = metal_extractors;
			break;
		case CAT_MMAKER:
			tempList = metal_makers;
			break;
		case CAT_G_ATTACK:
			tempList = ground_attackers;
			break;
		case CAT_DEFENCE:
			tempList = ground_defences;
			break;
		case CAT_BUILDER:
			tempList = ground_builders;
			break;
		case CAT_FACTORY:
			tempList = ground_factories;
			break;
		case CAT_MSTOR:
			tempList = metal_storages;
			break;
		case CAT_ESTOR:
			tempList = energy_storages;
			break;
		case CAT_NUKE:
			tempList = nuke_silos;
			break;
	}

	// if we are a builder on side i, then templist must have
	// at least i + 1 elements (templist[0], ..., templist[i])
	// but if a mod is not symmetric (eg. no builders for side
	// 1) this assumption fails; enabling this breaks PURE 0.6
	// however
	//
	// if (tempList->size() >= side + 1) {
		// iterate over all units for <side> in tempList (eg. Core ground_defences)
		for (unsigned int i = 0; i != tempList[side].size(); i++) {
			int tempUnitDefID = tempList[side][i];

			// if our builder can build the i-th unit
			if (CanBuildUnit(builderDef->id, tempUnitDefID)) {
				// get the unit's heuristic score (based on current income)
				tempScore = GetScore(unitTypes[tempUnitDefID].def, category);

				if (tempScore > bestScore) {
					bestScore = tempScore;
					tempUnitDef = unitTypes[tempUnitDefID].def;
				}
			}
		}
	// }

	// if we didn't find a unit to build with score > 0 (ie.
	// if builder has no build-option matching this category)
	// then return NULL instead of first option on build menu
	// (to prevent radar farms and other bizarro side-effects)
	return ((bestScore > 0.0f)? tempUnitDef: NULL);
}






/*
 * find and return the unit that's best to make for this builder
 * (returns NULL if no buildings/units are better than minUsefulness
 * or buidler can't make any economy units)
 * TODO: make it, look at how to integrate into the main economy manager
 */
/*
const UnitDef* CUnitTable::GetBestEconomyBuilding(int builder, float minUsefulness) {
	return 0;
}
*/




void CUnitTable::Init() {
	// get unit defs from game and stick them in the unitTypes[] array
	numOfUnits = ai->cb->GetNumUnitDefs();
	unitList = new const UnitDef*[numOfUnits];
	ai->cb->GetUnitDefList(unitList);

	// one more than needed because 0 is dummy object (so
	// UnitDef->id can be used to adress that unit in array)
	unitTypes = new UnitType[numOfUnits + 1];

	// add units to UnitTable
	for (int i = 1; i <= numOfUnits; i++) {
		unitTypes[i].def = unitList[i - 1];
		// side has not been assigned - will be done later
		unitTypes[i].category = -1;

		// get build options
		for (map<int, string>::const_iterator j = unitTypes[i].def->buildOptions.begin(); j != unitTypes[i].def->buildOptions.end(); j++) {
			unitTypes[i].canBuildList.push_back(ai->cb->GetUnitDef(j->second.c_str())->id);
		}
	}


	// now set sides and create buildtree for each
	// note: this skips Lua commanders completely!
	for (int s = 0; s < numOfSides; s++) {
		// set side of start unit (eg. commander) and continue recursively
		int unitDefID = startUnits[s];
		unitTypes[unitDefID].sides.insert(s);

		CalcBuildTree(unitDefID, s);
	}

	// add unit to different groups
	for (int i = 1; i <= numOfUnits; i++) {
		UnitType* me = &unitTypes[i];

		// KLOOTNOTE: this is a hack to make KAIK recognize Lua
		// commanders ((which are unreachable from the starting
		// units in the mod hierarchy and so will be skipped by
		// CalcBuildTree(), meaning me->sides stays empty)) as
		// builders, but the ground_builders[side] list for this
		// unit might not exist (and will never actually contain
		// this unitDef ID)
		if (/* me->def->isCommander && */ me->def->buildOptions.size() > 0) {
			me->category = CAT_BUILDER;
		}

		for (std::set<int>::iterator it = me->sides.begin(); it != me->sides.end(); it++) {
			int mySide = *it;
			int UnitCost = int(me->def->metalCost * METAL2ENERGY + me->def->energyCost);
			me->TargetCategories.resize(me->def->weapons.size());

			if (me->def->filename.find(".lua") != std::string::npos) {
				// can't parse these without a Lua parser
				for (unsigned int w = 0; w != me->def->weapons.size(); w++) {
					me->TargetCategories[w] = "";
				}
			} else {
				CSunParser attackerParser(ai);
				attackerParser.LoadVirtualFile(me->def->filename.c_str());

				for (unsigned int w = 0; w != me->def->weapons.size(); w++) {
					char weaponnumber[10] = "";
					itoa(w, weaponnumber, 10);
					attackerParser.GetDef(me->TargetCategories[w], "-1", string("UNITINFO\\OnlyTargetCategory") + string(weaponnumber));
				}
			}


			me->DPSvsUnit.resize(numOfUnits + 1);

			// calculate this unit type's DPS against all other unit types
			for (int v = 1; v <= numOfUnits; v++) {
				me->DPSvsUnit[v] = GetDPSvsUnit(me->def, unitTypes[v].def);
			}

			// speed > 0 means we are mobile, minWaterDepth <= 0 means we
			// are allergic to water and cannot be in it (positive values
			// are inverted internally)
			if (me->def->speed > 0.0f /* && me->def->minWaterDepth <= 0 */) {
				if (me->def->buildOptions.size() > 0) {
					ground_builders[mySide].push_back(i);
					me->category = CAT_BUILDER;
				}
				else if (!me->def->weapons.empty() && !me->def->weapons.begin()->def->stockpile) {
					ground_attackers[mySide].push_back(i);
					me->category = CAT_G_ATTACK;
				}
			}



			else if (!me->def->canfly) {
				if (true /* me->def->minWaterDepth <= 0 */) {
					if (me->def->buildOptions.size() >= 1 && me->def->builder) {
						if ((((me->def)->TEDClassString) == "PLANT") || (((me->def)->speed) > 0.0f)) {
							me->isHub = false;
						} else {
							me->isHub = true;
						}

						ground_factories[mySide].push_back(i);
						me->category = CAT_FACTORY;
					}
					else {
						const WeaponDef* weapon = (me->def->weapons.empty())? 0: me->def->weapons.begin()->def;

						if (weapon && !weapon->stockpile && me->def->extractsMetal == 0.0f) {
							// we don't want armed extractors to be seen as general-purpose defense
							if (!weapon->waterweapon) {
								// filter out depth-charge launchers etc
								ground_defences[mySide].push_back(i);
								me->category = CAT_DEFENCE;
							}
						}

						if (me->def->stockpileWeaponDef) {
							if (me->def->stockpileWeaponDef->targetable) {
								// nuke
								nuke_silos[mySide].push_back(i);
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
							metal_makers[mySide].push_back(i);
							me->category = CAT_MMAKER;
						}
						if (me->def->extractsMetal > 0.0f) {
							metal_extractors[mySide].push_back(i);
							me->category = CAT_MEX;
						}
						if (((me->def->energyMake - me->def->energyUpkeep) / UnitCost) > 0.002 || me->def->tidalGenerator || me->def->windGenerator) {
							if (/* me->def->minWaterDepth <= 0 && */ !me->def->needGeo) {
								// filter tidals and geothermals
								ground_energy[mySide].push_back(i);
								me->category = CAT_ENERGY;
							}
						}
						if (me->def->energyStorage / UnitCost > 0.2) {
							energy_storages[mySide].push_back(i);
							me->category = CAT_ESTOR;
						}
						if (me->def->metalStorage / UnitCost > 0.1) {
							metal_storages[mySide].push_back(i);
							me->category = CAT_MSTOR;
						}
					}
				}
			}
		}
	}

	ReadModConfig();
	// dump generated unit table to file
	DebugPrint();
}





bool CUnitTable::CanBuildUnit(int id_builder, int id_unit) {
	// look in build options of builder for unit
	for (unsigned int i = 0; i != unitTypes[id_builder].canBuildList.size(); i++) {
		if (unitTypes[id_builder].canBuildList[i] == id_unit) {
			return true;
		}
	}

	// unit not found in builder's buildoptions
	return false;
}

// determines sides of unitTypes by recursion
void CUnitTable::CalcBuildTree(int unitDefID, int rootSide) {
	UnitType* utype = &unitTypes[unitDefID];

	// go through all possible build options
	for (unsigned int i = 0; i != utype->canBuildList.size(); i++) {
		// add this unit to target's built-by list
		int buildOptionIndex = utype->canBuildList[i];
		UnitType* buildOptionType = &unitTypes[buildOptionIndex];

		// KLOOTNOTE: techLevel will not make much sense if
		// unit has multiple ancestors at different depths
		// in tree (eg. Adv. Vehicle Plants in XTA)
		//
		// buildOptionType->techLevel = utype->techLevel;
		// buildOptionType->techLevel = utype->def->techLevel;
		// FIXME: causes duplicated entries in PURE
		// buildOptionType->builtByList.push_back(unitDefID);

		if (buildOptionType->sides.find(rootSide) == buildOptionType->sides.end()) {
			// unit has not been checked yet, set side
			// as side of its builder and continue
			buildOptionType->sides.insert(rootSide);
			CalcBuildTree(buildOptionIndex, rootSide);
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

	char filename[1024] = ROOTFOLDER"CUnitTable.log";
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, filename);
	FILE* file = fopen(filename, "w");

	for (int i = 1; i <= numOfUnits; i++) {
		UnitType* utype = &unitTypes[i];

		fprintf(file, "UnitDef ID: %i\n", i);
		fprintf(file, "Name:       %s\n", unitList[i - 1]->humanName.c_str());
		fprintf(file, "Sides:      ");

		for (std::set<int>::iterator it = utype->sides.begin(); it != utype->sides.end(); it++) {
			fprintf(file, "%d (%s) ", *it, sideNames[*it].c_str());
		}

		fprintf(file, "\n");
		fprintf(file, "Can Build:  ");

		for (unsigned int j = 0; j != utype->canBuildList.size(); j++) {
			UnitType* buildOption = &unitTypes[utype->canBuildList[j]];

			for (std::set<int>::iterator it = buildOption->sides.begin(); it != buildOption->sides.end(); it++) {
				const char* sideName = sideNames[*it].c_str();
				const char* buildOptionName = buildOption->def->humanName.c_str();
				fprintf(file, "'(%s) %s' ", sideName, buildOptionName);
			}
		}

		fprintf(file, "\n");
		fprintf(file, "Built by:   ");

		for (unsigned int k = 0; k != utype->builtByList.size(); k++) {
			UnitType* parent = &unitTypes[utype->builtByList[k]];

			for (std::set<int>::iterator it = parent->sides.begin(); it != parent->sides.end(); it++) {
				const char* sideName = sideNames[*it].c_str();
				const char* parentName = parent->def->humanName.c_str();
				fprintf(file, "'(%s) %s' ", sideName, parentName);
			}
		}

		fprintf(file, "\n\n");
	}

	for (int s = 0; s < numOfSides; s++) {
		for (unsigned int l = 0; l != all_lists.size(); l++) {
			fprintf(file, "\n\n%s (side %d) units of category %s:\n", sideNames[s].c_str(), s, listCategoryNames[l]);

			for (unsigned int i = 0; i != all_lists[l][s].size(); i++)
				fprintf(file, "\t%s\n", unitTypes[all_lists[l][s][i]].def->humanName.c_str());
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
