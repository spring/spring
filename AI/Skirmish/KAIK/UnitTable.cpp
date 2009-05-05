#include <sstream>

#include "Sim/Misc/GlobalConstants.h"
#include "System/Util.h"

#include "IncCREG.h"
#include "IncEngine.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

#include "KAIK.h"
extern CKAIK* KAIKStateExt;

CR_BIND(CUnitTable, );
CR_REG_METADATA(CUnitTable, (
	CR_MEMBER(categoryData),

	CR_MEMBER(numDefs),
	CR_MEMBER(unitTypes),

	CR_POSTLOAD(PostLoad)
));

CR_BIND(CategoryData, );
CR_REG_METADATA(CategoryData, (
	CR_MEMBER(groundFactories),
	CR_MEMBER(groundBuilders),
	CR_MEMBER(groundAttackers),
	CR_MEMBER(metalExtractors),
	CR_MEMBER(metalMakers),
	CR_MEMBER(groundEnergy),
	CR_MEMBER(groundDefenses),
	CR_MEMBER(metalStorages),
	CR_MEMBER(energyStorages),
	CR_MEMBER(nukeSilos)
));



CUnitTable::CUnitTable(AIClasses* aic) {
	ai = aic;
}

CUnitTable::~CUnitTable() {
}

void CUnitTable::PostLoad() {
	// NOTE: can we actually restore the UT
	// state sensibly wrt. team sides etc.?
	ai = KAIKStateExt->GetAi();

	Init();
}



void CUnitTable::ReadModConfig() {
	L(ai, "[CUnitTable::ReadModConfig()]");

	std::stringstream msg;
	std::string cfgFileName = GetModCfgName();

	FILE* f = fopen(cfgFileName.c_str(), "r");

	if (f != NULL) {
		msg << "\tparsing existing mod configuration file ";
		msg << cfgFileName;
		L(ai, msg.str());

		// read the mod's .cfg file
		char str[1024] = {0};

		char         unitName[512] = {0};
		float        unitCostMult  =  1.0f;
		int          unitTechLvl   = -1;
		UnitCategory defUnitCat    = CAT_LAST;
		UnitCategory cfgUnitCat    = CAT_LAST;

		while (fgets(str, 1024, f) != NULL) {
			if (str[0] == '/' && str[1] == '/') {
				continue;
			}

			const int i = sscanf(str, "%s %f %d %d", unitName, &unitCostMult, &unitTechLvl, (int*) &cfgUnitCat);
			const UnitDef* udef = ai->cb->GetUnitDef(unitName);

			if ((i == 4) && udef != NULL) {
				UnitType* utype = &unitTypes[udef->id];

				utype->costMultiplier = unitCostMult;
				utype->techLevel      = unitTechLvl;

				defUnitCat = utype->category;

				msg.str("");
				msg << "\t\tudef->id: " << udef->id << ", udef->name: " << udef->name;
				msg << ", default cat.: " << defUnitCat << ", .cfg cat.: " << cfgUnitCat;
				L(ai, msg.str());

				// TODO: look for any possible side-effects that might arise
				// from overriding categories like this, then enable overrides
				// other than builder --> attacker?
				//
				// FIXME: SEGV when unarmed CAT_BUILDER units masquerading as
				// CAT_G_ATTACK'ers want to or are attacked (NULL weapondefs)
				//
				if (cfgUnitCat >= 0 && cfgUnitCat < CAT_LAST) {
					if (cfgUnitCat == CAT_G_ATTACK && defUnitCat == CAT_BUILDER) {
						msg.str("");
						msg << "\t\t\t.cfg unit category (CAT_G_ATTACK) overrides utype->category (CAT_BUILDER)";
						L(ai, msg.str());

						std::vector<int>::iterator vit;

						std::vector<int>& oldDefs = categoryData.GetDefsForUnitCat(defUnitCat);
						std::vector<int>& newDefs = categoryData.GetDefsForUnitCat(cfgUnitCat);

						for (vit = oldDefs.begin(); vit != oldDefs.end(); vit++) {
							const int udefID = *vit;

							if (udefID == udef->id) {
								oldDefs.erase(vit);
								newDefs.push_back(udef->id);
								vit--;
							}
						}

						utype->category = cfgUnitCat;
					}
				}
			}
		}

		msg.str("");
		msg << "read mod configuration file ";
		msg << cfgFileName;
		L(ai, msg.str());
	} else {
		msg.str("");
		msg << "\tcreating new mod configuration file ";
		msg << cfgFileName;
		L(ai, msg.str());

		// write a new .cfg file with default values
		f = fopen(cfgFileName.c_str(), "w");
		fprintf(f, "// unitName costMultiplier techLevel category\n");

		for (int i = 1; i <= numDefs; i++) {
			UnitType* utype = &unitTypes[i];
			// assign and write default values for costMultiplier
			// and techLevel, category is already set in ::Init()
			utype->costMultiplier =  1.0f;
			utype->techLevel      = -1;

			fprintf(
				f,
				"%s %.2f %d %d\n",
				utype->def->name.c_str(), utype->costMultiplier,
				utype->techLevel, utype->category
			);

			msg.str("");
			msg << "\t\tname: " << (utype->def->name);
			msg << ", .cfg category: " << (utype->category);
			L(ai, msg.str());
		}

		msg.str("");
		msg << "wrote mod configuration file ";
		msg << cfgFileName;
		L(ai, msg.str());
	}

	fclose(f);
}



/*
// returns the side of the AI's team
int CUnitTable::GetSide(void) const {
	const int team = ai->cb->GetMyTeam();
	const int side = teamSides[team];

	return side;
}
// returns the side of the team that is
// currently controlling this unit (not
// always the same as the unit's native
// side defined by the mod's build-tree)
int CUnitTable::GetSide(int unitID) const {
	const int team = ai->cb->GetUnitTeam(unitID);
	const int side = teamSides[team];

	return side;
}
// returns the side (build-tree) that a
// given UnitDef statically belongs to
int CUnitTable::GetSide(const UnitDef* udef) const {
	const UnitType&      utype  = unitTypes[udef->id];
	const std::set<int>& sides  = utype.sides;
	const int            mySide = GetSide();

	if (!sides.empty()) {
		if (sides.find(mySide) != sides.end()) {
			// our team's side can build this
			return mySide;
		} else {
			// our team's side cannot build this,
			// just return the first side that it
			// _is_ part of
			return *(sides.begin());
		}
	}

	// this unitdef lives outside of _all_ of the mod's
	// build-trees (ie. is not reachable from any side's
	// starting unit) but we are in control of it anyway
	return mySide;
}
*/

UnitCategory CUnitTable::GetCategory(const UnitDef* unitdef) const {
	return (unitTypes[unitdef->id].category);
}
UnitCategory CUnitTable::GetCategory(int unitID) const {
	const UnitDef* udef = ai->cb->GetUnitDef(unitID);

	if (udef != NULL) {
		const UnitType* utype = &unitTypes[udef->id];
		return (utype->category);
	} else {
		return CAT_LAST;
	}
}



// used to update threat-map, should probably
// use cost multipliers too (but in that case
// non-squad units like Flashes could become
// artifically overrated by a massive amount)
float CUnitTable::GetDPS(const UnitDef* unit) {
	if (unit != NULL) {
		float totaldps = 0.0f;

		for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
			float dps = 0.0f;

			if (!i->def->paralyzer) {
				float reloadtime = i->def->reload;
				int numDamages = 0;
				ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numDamages);

				for (int k = 0; k < numDamages; k++) {
					dps += i->def->damages[k];
				}

				dps = dps * i->def->salvosize / numDamages / reloadtime;
			}

			totaldps += dps;
		}

		return totaldps;
	}

	return 0.0f;
}



float CUnitTable::GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim) {
	if (unit->weapons.size() > 0) {
		ai->math->TimerStart();

		float dps = 0.0f;
		bool canhit = false;
		int armortype = victim->armorType;
		int numDamages = 0;
		ai->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numDamages);

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

					if (unit->weapons[i].def->type == std::string("Cannon")) {
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
						targetarea = ((victim->xsize * 16) + AOE) * ((victim->zsize * 16) + AOE);
					} else {
						impactarea = pow((accuracy) * (0.7f * distancetravelled), 2);
						targetarea = (victim->xsize * victim->zsize * 256);
					}

					if (impactarea > targetarea) {
						tohitprobability = targetarea / impactarea;
					} else {
						tohitprobability = 1;
					}

					if (unit->weapons[i].def->turnrate == 0.0f && unit->weapons[i].def->projectilespeed != 0 && victim->speed != 0 && unit->weapons[i].def->beamtime == 1) {
						if (unit->weapons[i].def->type == std::string("Cannon")) {
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
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);
	std::vector<int> enemiesOfType(ai->cb->GetNumUnitDefs() + 1, 0);

	float score = 0.01f;
	float totalCost = 0.01f;

	for (int i = 0; i < numEnemies; i++) {
		const UnitDef* udef = ai->ccb->GetUnitDef(ai->unitIDs[i]);

		if (udef != NULL) {
			enemiesOfType[udef->id]++;
		}
	}

	for (unsigned int i = 1; i < enemiesOfType.size(); i++) {
		bool b1 = unitTypes[i].def->builder;
		bool b2 = (enemiesOfType[i] > 0);
		// bool b3 = (!unit->speed && !unitTypes[i].def->speed);

		if (!b1 && b2 /* && !b3 */) {
			float costOfEnemiesOfThisType = ((unitTypes[i].def->metalCost * METAL2ENERGY) + unitTypes[i].def->energyCost) * enemiesOfType[i];
			float currentScore = unitTypes[unit->id].DPSvsUnit[i] * costOfEnemiesOfThisType;

			/*
			if (unitTypes[i].DPSvsUnit[unit->id] * costofenemiesofthistype > 0) {
				currentscore -= (unitTypes[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
			}
			*/

			totalCost += costOfEnemiesOfThisType;
			score += currentScore;
		}
	}

	if (totalCost <= 0)
		return 0.0f;

	return (score / totalCost);
}




void CUnitTable::UpdateChokePointArray() {
	std::vector<float> EnemyCostsByMoveType(ai->pather->NumOfMoveTypes);
	std::vector<int> enemiesOfType(ai->cb->GetNumUnitDefs() + 1, 0);

	float totalCost = 1.0f;
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);

	for (int i = 0; i < ai->pather->totalcells; i++) {
		ai->dm->ChokePointArray[i] = 0;
	}
	for (int i = 0; i < ai->pather->NumOfMoveTypes; i++) {
		EnemyCostsByMoveType[i] = 0;
	}
	for (int i = 0; i < numEnemies; i++) {
		enemiesOfType[ai->ccb->GetUnitDef(ai->unitIDs[i])->id]++;
	}

	for (unsigned int i = 1; i < enemiesOfType.size(); i++) {
		if (!unitTypes[i].def->canfly && unitTypes[i].def->speed > 0) {
			float currentcosts =
				((unitTypes[i].def->metalCost * METAL2ENERGY) +
				unitTypes[i].def->energyCost) * (enemiesOfType[i]);
			// non-zero speed implies non-NULL movedata
			EnemyCostsByMoveType[(unitTypes[i].def)->movedata->pathType] += currentcosts;
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



float CUnitTable::GetMaxRange(const UnitDef* unit) {
	float max_range = 0.0f;

	for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
		if ((i->def->range) > max_range) {
			max_range = i->def->range;
		}
	}

	return max_range;
}

float CUnitTable::GetMinRange(const UnitDef* unit) {
	float min_range = MY_FLT_MAX;

	for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++) {
		if ((i->def->range) < min_range) {
			min_range = i->def->range;
		}
	}

	return min_range;
}






float CUnitTable::GetScore(const UnitDef* udef, UnitCategory cat) {
	const int m = (ai->uh->AllUnitsByType[udef->id]).size();
	const int n = udef->maxThisUnit;

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

	const int frame = ai->cb->GetCurrentFrame();
	const float cost =
		((udef->metalCost * METAL2ENERGY) +
		udef->energyCost) + 0.1f;
	const float currentIncome =
		INCOMEMULTIPLIER *
		(ai->cb->GetEnergyIncome() + (ai->cb->GetMetalIncome() * METAL2ENERGY)) +
		frame / 2;
	const float Hitpoints = udef->health;
	const float buildTime = udef->buildTime + 0.1f;
	const float RandNum = ai->math->RandNormal(4, 3, 1) + 1;

	float benefit = 0.0f;
	float aoe = 0.0f;
	float dps = 0.0f;
	int unitcounter = 0;
	bool candevelop = false;

	switch (cat) {
		case CAT_ENERGY: {
			// KLOOTNOTE: factor build-time into this as well?
			// (so benefit values generally lie closer together)
			float baseBenefit = udef->energyMake - udef->energyUpkeep;

			if (udef->windGenerator > 0.0f) {
				const float minWind = ai->cb->GetMinWind();
				const float maxWind = ai->cb->GetMaxWind();
				const float avgWind = (minWind + maxWind) * 0.5f;
				const float avgEffi = std::min(avgWind / udef->windGenerator, 1.0f);

				if (avgEffi >= 0.4f) {
					baseBenefit += avgWind;
				}
			}
			if (udef->tidalGenerator > 0.0f) {
				baseBenefit += ai->cb->GetTidalStrength();
			}

			// filter geothermals
			if (udef->needGeo) {
				baseBenefit = 0.0f;
			}

			// KLOOTNOTE: dividing by cost here as well means
			// benefit is inversely proportional to square of
			// cost, so expensive generators are quadratically
			// less likely to be built if original calculation
			// of score is used
			// benefit /= cost;
			benefit = (baseBenefit / buildTime) * float((rand() % 2) + 1);
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

			benefit =
				pow((aoe + 80), 1.5f) *
				pow(GetMaxRange(udef) + 200, 1.5f) *
				pow(dps, 1.0f) *
				pow(udef->speed + 40, 1.0f) *
				pow(Hitpoints, 1.0f) *
				pow(RandNum, 2.5f) *
				pow(cost, -0.5f);

			if (udef->canfly || udef->canhover) {
				// general hack: reduce feasibility of aircraft for 20 mins
				// and that of hovercraft permanently, should mostly prefer
				// real L2 units to hovers
				benefit = (udef->canfly && frame >= (30 * 60 * 20))? benefit: benefit * 0.01f;
			}
		} break;

		case CAT_DEFENCE: {
			aoe = ((udef->weapons.size())? ((udef->weapons.front()).def)->areaOfEffect: 0.0f);
			benefit =
				pow((aoe + 80), 1.5f) *
				pow(GetMaxRange(udef), 2.0f) *
				pow(GetCurrentDamageScore(udef), 1.5f) *
				pow(Hitpoints, 0.5f) *
				pow(RandNum, 2.5f) *
				pow(cost, -1.0f);
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
				benefit =
					pow(udef->buildSpeed, 1.0f) *
					pow(udef->speed, 0.5f) *
					pow(Hitpoints, 0.3f) *
					pow(RandNum, 0.4f);
			}
		} break;

		case CAT_FACTORY: {
			// benefit of a factory is dependant on the kind of
			// offensive units it can build, but EE-hubs are only
			// capable of building other buildings
			for (unsigned int i = 0; i != unitTypes[udef->id].canBuildList.size(); i++) {
				const int          buildOpt    = unitTypes[udef->id].canBuildList[i];
				const UnitCategory buildOptCat = unitTypes[buildOpt].category;

				if (buildOptCat == CAT_G_ATTACK || buildOptCat == CAT_FACTORY) {
					if (unitTypes[buildOpt].def != udef) {
						// KLOOTNOTE: guard against infinite recursion (BuildTowers in
						// PURE trigger this since they are able to build themselves)
						benefit += GetScore(unitTypes[buildOpt].def, buildOptCat);
						unitcounter++;
					}
				}
			}

			if (unitcounter > 0) {
				benefit /= (unitcounter * pow(float(ai->uh->AllUnitsByType[udef->id].size() + 1), 3.0f));
				benefit /= ((m > 0)? (m * 2.0f): 1.0f);
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
			float metalCost = udef->stockpileWeaponDef->metalcost;
			float energyCost = udef->stockpileWeaponDef->energycost;
			float supplyCost = udef->stockpileWeaponDef->supplycost;
			float denom = metalCost + energyCost + supplyCost + 1.0f;
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
	// return (benefit / (currentIncome + cost));
	// return ((benefit / cost) * currentIncome);
	return ((currentIncome / cost) * benefit);
}



// operates in terms of GetScore() (which is recursive for factories)
const UnitDef* CUnitTable::GetUnitByScore(int builderID, UnitCategory cat) {
	if (cat == CAT_LAST) {
		return NULL;
	}

	const UnitDef* builderDef  = ai->cb->GetUnitDef(builderID);
	const UnitDef* tempUnitDef = NULL;

	const std::vector<int>& defs = categoryData.GetDefsForUnitCat(cat);
	float tempScore = 0.0f;
	float bestScore = 0.0f;

	// if we are a builder on side i, then templist must have
	// at least i + 1 elements (templist[0], ..., templist[i])
	// but if a mod is not symmetric (eg. no builders for side
	// 1) this assumption fails; enabling this breaks PURE 0.6
	// however

	// iterate over all types in defs (eg. Core groundDefenses)
	for (unsigned int i = 0; i != defs.size(); i++) {
		int tempUnitDefID = defs[i];

		// if our builder can build the i-th unit
		if (CanBuildUnit(builderDef->id, tempUnitDefID)) {
			// get the unit's heuristic score (based on current income)
			tempScore = GetScore(unitTypes[tempUnitDefID].def, cat);

			if (tempScore > bestScore) {
				bestScore = tempScore;
				tempUnitDef = unitTypes[tempUnitDefID].def;
			}
		}
	}


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
	numDefs = ai->cb->GetNumUnitDefs();

	// one more than needed because [0] is a dummy object (so
	// UnitDef->id can be used to adress that unit in array)
	unitTypes.resize(numDefs + 1);
	unitDefs.resize(numDefs, NULL);

	ai->cb->GetUnitDefList(&unitDefs[0]);

	// add units to UnitTable
	for (int i = 1; i <= numDefs; i++) {
		unitTypes[i].def      = unitDefs[i - 1];
		unitTypes[i].category = CAT_LAST;

		// GetUnitDefList() filled our unitDefs
		// partially with null UnitDef*'s (bad,
		// nothing much to do if this happens)
		assert(unitTypes[i].def != 0x0);

		if ((unitTypes[i].def)->movedata != NULL) {
			moveDefs[(unitTypes[i].def)->movedata->pathType] = (unitTypes[i].def)->movedata;
		}

		std::map<int, std::string>::const_iterator j;

		// get build options
		for (j = unitTypes[i].def->buildOptions.begin(); j != unitTypes[i].def->buildOptions.end(); j++) {
			const char*    buildOptName = j->second.c_str();
			const UnitDef* buildOptDef  = ai->cb->GetUnitDef(buildOptName);

			unitTypes[i].canBuildList.push_back(buildOptDef->id);
		}
	}

	// add unit to different groups
	for (int i = 1; i <= numDefs; i++) {
		UnitType* uType = &unitTypes[i];
		const int UnitCost = int(uType->def->metalCost * METAL2ENERGY + uType->def->energyCost);

		if (/* uType->def->isCommander && */ uType->def->buildOptions.size() > 0) {
			uType->category = CAT_BUILDER;
		}

		uType->DPSvsUnit.resize(numDefs + 1);

		// calculate this unit type's DPS against all other unit types
		for (int v = 1; v <= numDefs; v++) {
			uType->DPSvsUnit[v] = GetDPSvsUnit(uType->def, unitTypes[v].def);
		}

		// speed > 0 means we are mobile, minWaterDepth <= 0 means we
		// are allergic to water and cannot be in it (positive values
		// are inverted internally)
		if (uType->def->speed > 0.0f /* && uType->def->minWaterDepth <= 0 */) {
			if (uType->def->buildOptions.size() > 0) {
				categoryData.groundBuilders.push_back(i);
				uType->category = CAT_BUILDER;
			}
			else if (!uType->def->weapons.empty() && !uType->def->weapons.begin()->def->stockpile) {
				categoryData.groundAttackers.push_back(i);
				uType->category = CAT_G_ATTACK;
			}
		}



		else if (!uType->def->canfly) {
			if (true /* uType->def->minWaterDepth <= 0 */) {
				if (uType->def->buildOptions.size() >= 1 && uType->def->builder) {
					if ((((uType->def)->TEDClassString) == "PLANT") || (((uType->def)->speed) > 0.0f)) {
						uType->isHub = false;
					} else {
						uType->isHub = true;
					}

					categoryData.groundFactories.push_back(i);
					uType->category = CAT_FACTORY;
				}
				else {
					const WeaponDef* weapon = (uType->def->weapons.empty())? 0: uType->def->weapons.begin()->def;

					if (weapon && !weapon->stockpile && uType->def->extractsMetal == 0.0f) {
						// we don't want armed extractors to be seen as general-purpose defense
						if (!weapon->waterweapon) {
							// filter out depth-charge launchers etc
							categoryData.groundDefenses.push_back(i);
							uType->category = CAT_DEFENCE;
						}
					}

					if (uType->def->stockpileWeaponDef) {
						if (uType->def->stockpileWeaponDef->targetable) {
							// nuke
							categoryData.nukeSilos.push_back(i);
							uType->category = CAT_NUKE;
						}
						if (uType->def->stockpileWeaponDef->interceptor) {
							// anti-nuke, not implemented yet
						}
					}

					if (uType->def->shieldWeaponDef && uType->def->shieldWeaponDef->isShield) {
						// shield, not implemented yet
						// uType->category = CAT_SHIELD;
					}

					if (uType->def->makesMetal) {
						categoryData.metalMakers.push_back(i);
						uType->category = CAT_MMAKER;
					}
					if (uType->def->extractsMetal > 0.0f) {
						categoryData.metalExtractors.push_back(i);
						uType->category = CAT_MEX;
					}
					if (((uType->def->energyMake - uType->def->energyUpkeep) / UnitCost) > 0.002 || uType->def->tidalGenerator || uType->def->windGenerator) {
						if (/* uType->def->minWaterDepth <= 0 && */ !uType->def->needGeo) {
							// filter tidals and geothermals
							categoryData.groundEnergy.push_back(i);
							uType->category = CAT_ENERGY;
						}
					}
					if (uType->def->energyStorage / UnitCost > 0.2) {
						categoryData.energyStorages.push_back(i);
						uType->category = CAT_ESTOR;
					}
					if (uType->def->metalStorage / UnitCost > 0.1) {
						categoryData.metalStorages.push_back(i);
						uType->category = CAT_MSTOR;
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



void CUnitTable::DebugPrint() {
	const char* listCategoryNames[12] = {
		"GROUND-FACTORY", "GROUND-BUILDER", "GROUND-ATTACKER", "METAL-EXTRACTOR",
		"METAL-MAKER", "METAL-STORAGE", "ENERGY-STORAGE", "GROUND-ENERGY", "GROUND-DEFENSE",
		"NUKE-SILO", "SHIELD-GENERATOR", "LAST-CATEGORY"
	};

	std::stringstream msg;
	std::string logFileName = GetDbgLogName();

	FILE* f = fopen(logFileName.c_str(), "w");

	if (f == NULL) {
		msg << "[CUnitTable::DebugPrint()] could not open ";
		msg << "debug log " << logFileName << " for writing";
		L(ai, msg.str());
		return;
	}

	for (int i = 1; i <= numDefs; i++) {
		const UnitType* utype = &unitTypes[i];
		const UnitDef*  udef  = unitDefs[i - 1];

		msg << "UnitDef ID: " << i << "\n";
		msg << "\tName: " << udef->name;
		msg << " (\"" << udef->humanName << "\")\n";
		msg << "\tCan Build:\n";

		for (unsigned int j = 0; j != utype->canBuildList.size(); j++) {
			const UnitType* buildOption = &unitTypes[utype->canBuildList[j]];
			const char*    buildOptName = buildOption->def->humanName.c_str();

			msg << "\t\t\"" << buildOptName << "\"\n";
		}

		/*
		msg << "\tBuilt By:\n";

		for (unsigned int k = 0; k != utype->builtByList.size(); k++) {
			UnitType* parent = &unitTypes[utype->builtByList[k]];

			for (std::set<int>::iterator it = parent->sides.begin(); it != parent->sides.end(); it++) {
				const char* sideName   = sideNames[*it].c_str();
				const char* parentName = parent->def->humanName.c_str();

				msg << "\t\t(\"" << sideName << "\") \"" << parentName << "\"\n";
			}
		}
		*/

		msg << "\n\n";
	}

	for (int defCatIdx = int(CAT_GROUND_FACTORY); defCatIdx <= int(CAT_NUKE_SILO); defCatIdx++) {
		msg << "units grouped under category \"";
		msg << listCategoryNames[defCatIdx];
		msg << "\":\n";

		const UnitDefCategory c = UnitDefCategory(defCatIdx);
		const std::vector<int>& defs = categoryData.GetDefsForUnitDefCat(c);

		for (unsigned int i = 0; i != defs.size(); i++) {
			const UnitDef* udef = unitTypes[defs[i]].def;

			msg << "\t" << udef->name << " (\"";
			msg << udef->humanName << "\")\n";
		}

		msg << "\n";
	}

	msg << "\n\n";


	fprintf(f, "%s", msg.str().c_str());
	fclose(f);
}



std::string CUnitTable::GetDbgLogName() const {
	std::string relFile =
		std::string(LOGFOLDER) +
		"CUnitTable.log";
	std::string absFile = AIUtil::GetAbsFileName(ai->cb, relFile);

	return absFile;
}

std::string CUnitTable::GetModCfgName() const {
	std::string relFile =
		std::string(CFGFOLDER) +
		(ai->cb->GetModName()) +
		".cfg";
	std::string absFile = AIUtil::GetAbsFileName(ai->cb, relFile);

	return absFile;
}
