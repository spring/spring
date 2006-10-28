//Based on Submarine's BuildTable Class from AAI. Thanks sub!
#include "unittable.h"

CUnitTable::CUnitTable(AIClasses* ai)
{
	this->ai = ai;
	//L("Starting Unit Table");
	numOfUnits = 0;
	unitList = 0;
	
	// Get all sides and commanders
	string sidestr = "SIDE";
	string errorstring = "-1";
	string Valuestring;
	char k [50];
	//L("Loading sidedata");
	ai->parser->LoadVirtualFile("gamedata\\SIDEDATA.tdf");
	//L("Starting Loop");
	for(int i = 0; i < 10; i++){
		sprintf(k,"%i",i);
		ai->parser->GetDef(Valuestring,errorstring,string(sidestr + k + "\\commander"));
		if(ai->cb->GetUnitDef(Valuestring.c_str())){
			startUnits.push_back(ai->cb->GetUnitDef(Valuestring.c_str())->id);
			ai->parser->GetDef(Valuestring,errorstring,string(sidestr + k + "\\name"));
			sideNames.push_back(Valuestring);
			numOfSides = i+1;
		}
	}
	//L("Sides Set Up");

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
	//L("UnitTable Inited!");
}

CUnitTable::~CUnitTable()
{
	delete [] unittypearray;

	delete [] unitList;

	delete [] ground_factories;
	delete [] ground_builders;
	delete [] ground_attackers;
	delete [] metal_extractors;
	delete [] metal_makers;
	delete [] ground_energy;
	delete [] ground_defences;
	delete [] metal_storages;
	delete [] energy_storages;
}

int CUnitTable::GetSide(int unit)
{
	assert(ai->cb->GetUnitDef(unit) != NULL);
	return unittypearray[ai->cb->GetUnitDef(unit)->id].side;
}


int CUnitTable::GetCategory (const UnitDef* unitdef)
{
	return unittypearray[unitdef->id].category;
}

int CUnitTable::GetCategory (int unit)
{
	assert(ai->cb->GetUnitDef(unit) != NULL);
	return unittypearray[ai->cb->GetUnitDef(unit)->id].category;
}

float CUnitTable::GetDPS(const UnitDef* unit)
{
	//L("");
	//L("Getting DPS for Unit: " << unit->humanName << "category: " << unit->category << " catstring: " << unit->categoryString);
	if(unit){	
		//L("Units Only targetcat:" << defaultvalue);
		float totaldps = 0;
		for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++){
			float dps = 0;
			if (!i->def->paralyzer){
				//L("weapon: " << i->name);
				//L("Categories, ONLY:" <<  i->onlyTargetCat << "BAD: " << i->onlyTargetCat << " ONLY(def): " << i->def->onlyTargetCategory);
				int numberofdamages;
				ai->cb->GetValue(AIVAL_NUMDAMAGETYPES,&numberofdamages);
				float reloadtime = i->def->reload;
				for(int k = 0; k < numberofdamages; k++){
					//L("damage: " << i->def->damages.damages[k]);
					dps += i->def->damages.damages[k];
				}				
				dps = dps * i->def->salvosize / numberofdamages / reloadtime;
				
				//L("dps for this weapon: " << dps << " Salvo size: " << i->def->salvosize << " Reload: " << reloadtime << " number of damages: " << numberofdamages);
			}			
			totaldps += dps;
		}
		//L("DPS sucessful: " << totaldps);
		return totaldps;
	}
	return 0;
}



float CUnitTable::GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim)
{
	if(unit->weapons.size()){
		ai->math->TimerStart();
		//L("");
		//L("Getting DPS for Unit: " << unit->humanName << " Against " << victim->humanName);
		float dps = 0;
		bool canhit = false;
		string targetcat;	
		int armortype = victim->armorType;
		int numberofdamages;
		ai->cb->GetValue(AIVAL_NUMDAMAGETYPES,&numberofdamages);
		for(int i = 0; i != unit->weapons.size(); i++){
			if (!unit->weapons[i].def->paralyzer){	
				targetcat = unittypearray[unit->id].TargetCategories[i];
				//L(unit->weapons[i].name << " Targcat: " <<  targetcat);
				//L("unit->categoryString " <<  unit->categoryString);
				//L("victim->categoryString " <<  victim->categoryString);
				
				unsigned int a = victim->category;
				unsigned int b = unit->weapons[i].def->onlyTargetCategory; // This is what the weapon can target
				unsigned int c = unit->weapons[i].onlyTargetCat; // This is what the unit accepts as this weapons target
				unsigned int d = unit->weapons[i].badTargetCat; // This is what the unit thinks this weapon must be used for (hmm ?)
				bool canWeaponTarget = (a & b) > 0;
				bool canUnitTarget = (a & c) > 0; // How is this used in spring, needs testing...
				bool badUnitTarget = (a & d) > 0; // This probably means that it have low priority, ignore it for now

				canhit = (canWeaponTarget && canUnitTarget);
				
				if(!unit->weapons[i].def->waterweapon && ai->cb->GetUnitDefHeight(victim->id) - victim->waterline < 0){
					canhit = false;
					//L("This weapon cannot hit this sub");
				}
				if(unit->weapons[i].def->waterweapon && victim->minWaterDepth == 0){
					canhit = false;
					//L("this anti-sub weapon cannot kill this unit");
				}
				
				// Bombers are useless against air:
				if(unit->weapons[i].def->dropped && victim->canfly && unit->canfly && unit->wantedHeight <= victim->wantedHeight){
					canhit = false;
				}
				
				if(canhit){
					//L(unit->weapons[i].name << " a: " <<  a << ", b: " << b << ", c: " << c << ", d: " << d);
					//L("unit->categoryString " <<  unit->categoryString);
					//L("victim->categoryString " <<  victim->categoryString);
				
				}
				
				if(canhit){
					float accuracy = unit->weapons[i].def->accuracy *2.8;
					if(victim->speed != 0){
						accuracy *= 1-(unit->weapons[i].def->targetMoveError);
					}
					float basedamage = unit->weapons[i].def->damages.damages[armortype] * unit->weapons[i].def->salvosize / unit->weapons[i].def->reload;	
					float AOE = unit->weapons[i].def->areaOfEffect * 0.7;
					float tohitprobability;
					float impactarea;
					float targetarea;
					float distancetravelled = 0.7f*unit->weapons[i].def->range;
					//L("Initial DT: " << distancetravelled);
					float firingangle;
					float gravity = -(ai->cb->GetGravity()*900);
					float timetoarrive;
					float u = unit->weapons[i].def->projectilespeed * 30;
					//L("Type: " << unit->weapons[i].def->type);
					if(unit->weapons[i].def->type == string("Cannon")){
						//L("Is ballistic! Gravity: " << gravity);						
						//L("u = " << u << " distancetravelled*gravity)/(u*u) = " << (distancetravelled*gravity)/(u*u));
						float sinoid = (distancetravelled*gravity)/(u*u);
						sinoid = min(sinoid,1);
						firingangle = asin(sinoid)/2;
						if(unit->highTrajectoryType == 1){
							firingangle = (PI/2) - firingangle;
						}
						//L("Firing angle = " << firingangle*180/PI);
						float heightreached = pow(u*sin(firingangle),2)/(2*gravity);
						float halfd = distancetravelled/2;
						distancetravelled = 2*sqrt(halfd*halfd+heightreached*heightreached) * 1.1;
						//L("Height reached: " << heightreached << " distance travelled " << distancetravelled);
					}
					//L("Name: " << unit->weapons[i].name << " AOE: " << AOE << " Accuracy: " << unit->weapons[i].def->accuracy << " range: " << unit->weapons[i].def->range);
					if((victim->canfly && unit->weapons[i].def->selfExplode) || !victim->canfly){					
						impactarea = pow((accuracy*distancetravelled)+AOE,2);
						targetarea = ((victim->xsize * 16) + AOE) * ((victim->ysize * 16) + AOE);
					}
					else{
						impactarea = pow((accuracy)*(0.7f*distancetravelled),2);
						targetarea = (victim->xsize * victim->ysize * 256);
					}
					//L("Impact Area: " << impactarea << " Target Area: " << targetarea);
					if(impactarea > targetarea){
						tohitprobability = targetarea/impactarea;
					}
					else{
						tohitprobability = 1;
					}
					if(!unit->weapons[i].def->guided && unit->weapons[i].def->projectilespeed != 0 && victim->speed != 0 && unit->weapons[i].def->beamtime == 1){
						if(unit->weapons[i].def->type == string("Cannon")){
							timetoarrive = (2*u*sin(firingangle))/gravity;
						}
						else{
							timetoarrive = distancetravelled / (unit->weapons[i].def->projectilespeed * 30);
						}
						float shotwindow = sqrt(targetarea) / victim->speed * 1.3;
						if(shotwindow < timetoarrive){
							tohitprobability *= shotwindow / timetoarrive;
						}
						//L("Not guided. TTA: " <<  timetoarrive << " shotwindow " << shotwindow << " unit->weapons[i].def->targetmoveerror " << unit->weapons[i].def->targetMoveError);
					}
					//L("chance to hit: " << tohitprobability);
					//L("Dps for this weapon: " << basedamage * tohitprobability);
					dps += basedamage * tohitprobability;
				}
			}	
		}	
		//L("DPS: " << dps);
		//L("Time taken: " << ai->math->TimerSecs() << "s.");
		return dps;
	}
	return 0;

}

float CUnitTable::GetCurrentDamageScore(const UnitDef* unit)
{
	L("Getting CombatScore for : " << unit->humanName);
	int enemies[MAXUNITS];
	int numofenemies = ai->cheat->GetEnemyUnits(enemies);
	vector<int> enemyunitsoftype;
	float score = 0.01;
	float totalcost=0.01;
	enemyunitsoftype.resize(ai->cb->GetNumUnitDefs() + 1,0);
	for(int i = 0; i < numofenemies; i++){
		enemyunitsoftype[ai->cheat->GetUnitDef(enemies[i])->id]++;
		//assert(ai->cheat->GetUnitDef(enemies[i]) == unittypearray[ai->cheat->GetUnitDef(enemies[i])->id].def);
	}
	for(int i = 1; i != enemyunitsoftype.size(); i++){
		if(!unittypearray[i].def->builder && enemyunitsoftype[i] > 0 && unittypearray[i].side != -1){// && !(!unit->speed && !unittypearray[i].def->speed)){
			float currentscore = 0;
			float costofenemiesofthistype = ((unittypearray[i].def->metalCost * METAL2ENERGY) + unittypearray[i].def->energyCost) * enemyunitsoftype[i];
			currentscore = unittypearray[unit->id].DPSvsUnit[i] * costofenemiesofthistype;
			totalcost += costofenemiesofthistype;
		/*if(unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype > 0){
				L("Score of them vs me: " << unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
				currentscore -= (unittypearray[i].DPSvsUnit[unit->id] * costofenemiesofthistype);
			}*/ // UBER HEADACHE from this, ill have to work on it when im rested^^
		score += currentscore;
		L("MyScore Vs " << unittypearray[i].def->humanName << " is " << currentscore);
			//L("unittypearray.size()= " << unittypearray[i].DPSvsUnit.size() << " My ID " << unit->id << " their ID: " << i << " costofenemiesofthistype= " << costofenemiesofthistype);
	
		}
	}
	if(totalcost <= 0)
		return 0;
	L(unit->humanName << " has an average dps vs all enemies: " << score / totalcost);
	return score / totalcost;
}

void CUnitTable::UpdateChokePointArray()
{
	vector<float> EnemyCostsByMoveType;
	EnemyCostsByMoveType.resize(ai->pather->NumOfMoveTypes);
	vector<int> enemyunitsoftype;
	float totalcosts = 1;
	int enemies[MAXUNITS];
	int numofenemies = ai->cheat->GetEnemyUnits(enemies);	
	enemyunitsoftype.resize(ai->cb->GetNumUnitDefs() + 1,0);
	for(int i = 0; i < ai->pather->totalcells;i++){
		ai->dm->ChokePointArray[i] = 0;
	}	
	for(int i = 0; i < ai->pather->NumOfMoveTypes;i++){
		EnemyCostsByMoveType[i] = 0;
	}
	for(int i = 0; i < numofenemies; i++){
		enemyunitsoftype[ai->cheat->GetUnitDef(enemies[i])->id]++;
	}
	for(int i = 1; i != enemyunitsoftype.size(); i++){
		if(unittypearray[i].side != -1 && !unittypearray[i].def->canfly && unittypearray[i].def->speed > 0){
			float currentcosts = ((unittypearray[i].def->metalCost * METAL2ENERGY) + unittypearray[i].def->energyCost) * (enemyunitsoftype[i]);
			EnemyCostsByMoveType[unittypearray[i].def->moveType] += currentcosts;	
			totalcosts += currentcosts;
		}
	}	
	for(int i = 0; i < ai->pather->NumOfMoveTypes;i++){
		EnemyCostsByMoveType[i] /= totalcosts;
		for(int c = 0; c < ai->pather->totalcells; c++){
			ai->dm->ChokePointArray[c] += ai->dm->ChokeMapsByMovetype[i][c] * EnemyCostsByMoveType[i];
		}		
	}
}


float CUnitTable::GetScore(const UnitDef* unit)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
{
	//L("Getting Score for: " << unit->humanName);
	
	//float EProductionNeed = unit->energyCost - ai->cb->GetEnergy();//ai->cb->GetEnergy() - unit->energyCost;
	//float EProduction = ai->cb->GetEnergyIncome() - ai->cb->GetEnergyUsage();
	//if(EProduction < 1)
	//	EProduction = 1;
	//L("EProductionNeed: " << EProductionNeed);
	//L("EProduction: " << EProduction);
	//float MProductionNeed = unit->metalCost - ai->cb->GetMetal();//ai->cb->GetMetal() - unit->metalCost;
	//float MProduction = ai->cb->GetMetalIncome() - ai->cb->GetMetalUsage();
	//if(MProduction < 1)
	//	MProduction = 1;
	//L("MProductionNeed: " << MProductionNeed);
	//L("MProduction: " << MProduction);
	//float timeToBuild = max (EProductionNeed / EProduction, MProductionNeed / MProduction);
	//if(timeToBuild < 1)
	//	timeToBuild = 1; // It can be built right now...  Make something more costly instead?
	//L("timeToBuild: " << timeToBuild);
	//float METAL2ENERGY_current = ai->cb->GetEnergy() (ai->cb->GetEnergyIncome()*0.9 - ai->cb->GetEnergyUsage())
	//float buildtime = //unit->buildTime / builder->buildSpeed;
	// Take the buildtime, and use it smart.
	// Make a ratio of unit->buildTime / timeToBuild ???:
	// This realy needs to know its creators build speed.
	//float buildTimeRatio =  unit->buildTime / timeToBuild;
	//L("buildTimeRatio: " << buildTimeRatio);
	//float buildTimeFix = unit->buildTime / 100;

	// Ignore the cost now :P
	float Cost = (unit->metalCost * METAL2ENERGY) + unit->energyCost;
	float CurrentIncome = INCOMEMULTIPLIER*(ai->cb->GetEnergyIncome() + (ai->cb->GetMetalIncome() * METAL2ENERGY)) + ai->cb->GetCurrentFrame() /2;
	float Hitpoints = unit->health;
	float Benefit = 0;
	float dps = 0;
	int unitcounter=0;
	bool candevelop = false;
	// Test: make it less random:
	float RandNum = ai->math->RandNormal(4,3,1) + 1;
	int category = GetCategory(unit);
	switch (category){
	case CAT_ENERGY:
		Benefit = unit->energyMake - unit->energyUpkeep;
		if (unit->windGenerator){
			Benefit += ai->cb->GetMinWind();// + ((ai->cb->GetMaxWind() - ai->cb->GetMinWind()) / 5);
		}
		if (unit->tidalGenerator){
			Benefit += ai->cb->GetTidalStrength();
		}
		
		if(unit->needGeo)
			Benefit = 0;
		Benefit /= Cost;
		//Benefit *= pow(buildTimeFix,float(-1.1));
		break;
	case CAT_MEX:
		Benefit = pow(unit->extractsMetal,float(4));
		break;
	case CAT_MMAKER:
		Benefit = (unit->metalMake- unit->metalUpkeep) / unit->energyUpkeep + 0.01;
		break;
	case CAT_G_ATTACK:
		dps = GetCurrentDamageScore(unit);
		if(unit->canfly && !unit->hoverAttack){
			dps /= 6; // Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
		}
        Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(1.5))
				* pow(GetMaxRange(unit)+200,float(1.5))
				* pow(dps,float(1))
				* pow(unit->speed + 40,float(1))
				* pow(Hitpoints,float(1))
				* pow(RandNum,float(2.5))
				//* pow(buildTimeFix,float(-0.35))
				* pow(Cost,float(-0.5));
		
		// AA hack (stop mass air)
		if(unit->canfly)
			Benefit *= 0.25; //slight reduction to the feasability of air
		break;
	case CAT_DEFENCE:
		//L("Getting DEFENCE SCORE FOR: " << unit->humanName);
		Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(1.5))
				* pow(GetMaxRange(unit),float(2))
				* pow(GetCurrentDamageScore(unit),float(1.5))
				* pow(Hitpoints,float(0.5))
				* pow(RandNum,float(2.5))
				//* pow(buildTimeFix,float(-0.6));
				* pow(Cost,float(-1));
		break;
	case CAT_BUILDER:
		//L("Getting Score For Builder");
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			//L("category: " << unittypearray[unittypearray[unit->id].canBuildList[i]].category);
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_FACTORY){
				candevelop = true;
			}
		}
		Benefit = pow(unit->buildSpeed,float(1))
				* pow(unit->speed,float(0.5))
				* pow(Hitpoints,float(0.3))
				* pow(RandNum,float(0.4));
				//* pow(Cost,float(-1));
		if(!candevelop)
			Benefit = 0;
		break;
	case CAT_FACTORY:
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_G_ATTACK){
				Benefit += GetScore(unittypearray[unittypearray[unit->id].canBuildList[i]].def);
				unitcounter++;
			}
		}
		// If we dont have any factorys then a new one better not cost much
		//if(ai->uh->AllUnitsByCat[CAT_FACTORY] > 0)
		//	buildTimeRatio *= 5; // it can take some time to make the factory.
		//else
		//	buildTimeRatio *= 0.5;
		
		//L("CAT_FACTORY: " << unit->humanName << " Has a sum score of : " << Benefit);
		
		if(unitcounter > 0)
			Benefit /= (unitcounter * pow(float(ai->uh->AllUnitsByType[unit->id]->size() + 1),float(3)));
		else
			Benefit = 0;
		//L("Factory: " << unit->humanName << " Has a score of: " << Benefit);
		break;
	case CAT_MSTOR:
		Benefit = pow((unit->metalStorage),float(1))
				* pow(Hitpoints,float(1));
				//* pow(Cost,float(-0.5));
		break;
	case CAT_ESTOR:
		Benefit = pow((unit->energyStorage),float(1))
				* pow(Hitpoints,float(1));
				//* pow(Cost,float(-0.5));
		break;
	default:
		Benefit = 0;
	}
	
	// This buildTimeRatio must be balansed somehow.
	// If the number is big then it means that it will be fast to build.
	// If the number is less than 1 (or something like that) it will take too long to build.
	/*if(buildTimeRatio > 100)
		buildTimeRatio = 10;
	else if (buildTimeRatio > 0.5)
		buildTimeRatio = sqrt(buildTimeRatio);
	else
	{
		// This will be slow... Do a cut off.
		buildTimeRatio *= 0.001;
	}*/
	//L("buildTimeRatio: " << buildTimeRatio);
	
	
	//L("Benefit: " << Benefit);
	//L("Unit: " << unit->humanName << " Has a score of : " << Benefit);
	float EScore = Benefit/(Cost+CurrentIncome);
	//float EScore = Benefit * pow(buildTimeRatio,float(4));
	//L("EScore: " << EScore);
	return EScore;
}
const UnitDef* CUnitTable::GetUnitByScore(int builder, int category)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
{
	ai->math->TimerStart();
	//L("Getting Score for category:" << category << " Builder: " << ai->cb->GetUnitDef(builder)->humanName);
	vector<int>* templist;
	const UnitDef* tempunit;
	int side = GetSide(builder);
	float tempscore = 0;
	float bestscore = 0;
	//L("vars set");
	switch (category){
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
	//L("Switch done, side: " << side);
	for(int i = 0; i != templist[side].size(); i++){
		if(CanBuildUnit(ai->cb->GetUnitDef(builder)->id, templist[side][i])){
			tempscore = GetScore(unittypearray[templist[side][i]].def);
			if (bestscore < tempscore){
				//L("Better Score found");
				bestscore = tempscore;
				tempunit = unittypearray[templist[side][i]].def;
			}
		}
	}
	//L("Loop calculated");
	if(!bestscore){
		L("no scores found for unit!");
		for(int i = 0; i != unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList.size(); i++){
				if(unittypearray[unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList[i]].category != -1){
					return unittypearray[unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList.front()].def;
				}
		}
	}
	//L("Time taken for getunitbyscore: " << ai->math->TimerSecs() << "s.");
	return tempunit;
}



/*
Find and return the unit that its best to make for this builder.
If no buildings/units are better that minUsefullnes, or it cant make any economy units, NULL is returned.

TODO: make it, and look at how to integrate it into the main economy manager

NOTE: this function will most likely be removed/replased soon
*/
const UnitDef* CUnitTable::GetBestEconomyBuilding(int builder, float minUsefullnes)
{
	
	// for each unit that can be built by this builder, do:
	
	//float metalEfficiency = unit metalmakedelta / metalcost;
	//float EnergyEfficiency = unit energymakedelta / energycost;
	
	//int constructionTime = unit build time / builder speed (or sevral builders) + startup time + move time
	
	//float currentTotalEfficiency = (sum Efficiency and current/planed needs will make this one)
	
	//float productionPower = (unit metalmakedelta + energymakedelta based on current/planed needs makes this one)
	
	
	// This needs thinking - might be used as modifiers:
	//float baseAreaUsageEfficiency = productionPower / size   ( / estimate of available base area  ???)
	//float unitCountUsageEfficiency = productionPower / (max units - unit count)
	
	
	
	
	
	// If constructionTime > "too long to build"  then test how many builders that will be made available to 
	// economy usage. If it will take too long useing all builders, give it a big reduction (or ignore it).
	
	// This needs thinking:
	// If construction cost > "available resources + incomeForEconomyUsage*constructionTime"  or
	// If this construction will at any point make available resources hit 0
	// then give it a big reduction.
	
	
	
	// end for
	
	
	// return best unit


	
	return NULL;
}

void CUnitTable::Init()
{
	ai->math->TimerStart();
	numOfUnits = ai->cb->GetNumUnitDefs();
	// one more than needed because 0 is dummy object (so UnitDef->id can be used to adress that unit in the array) 
	unittypearray = new UnitType[numOfUnits+1];
	unittypearray[0].side = -1;
	// get unit defs from game and stick them in the unittypearray[] array
	unitList = new const UnitDef*[numOfUnits];
	ai->cb->GetUnitDefList(unitList);
	for (int i = 1; i <= numOfUnits; i++){
		unittypearray[i].def=unitList[i-1];
	}
	// add units to UnitTable
	for(int i = 1; i <= numOfUnits; i++){		
		// side has not been assigned - will be done later
		unittypearray[i].side = -1;	
		unittypearray[i].category = -1;
		// get build options
		for(map<int, string>::const_iterator j = unittypearray[i].def->buildOptions.begin(); j != unittypearray[i].def->buildOptions.end(); j++){
			unittypearray[i].canBuildList.push_back(ai->cb->GetUnitDef(j->second.c_str())->id);
		}
	}	
	// now set the sides and create buildtree
	for(int s = 0; s < numOfSides; s++){
		// set side of the start unit (eg commander) and continue recursively
		unittypearray[startUnits[s]].side = s;
		//L("Calcing buildtree of: " << startUnits[s]);
		CalcBuildTree(startUnits[s]);
	}
	
	// Make a weapon parser:
	CSunParser* weaponParser = new  CSunParser(ai);
	/*
	//std::vector<std::string> tafiles = CFileHandler::FindFiles("weapons/*.tdf");
	
	L("weaponParser->LoadVirtualFile(weapons.tdf);");
	//weaponParser->LoadVirtualFile("units/armanni.fbi");
	weaponParser->LoadVirtualFile("");
	L("weaponParser->LoadVirtualFile(weapons/ARM_TOTAL_ANNIHILATOR.tdf);");
	vector<string> list = weaponParser->GetSectionList("");
	L(list[0]);
	const map<string, string> theMap = weaponParser->GetAllValues("UNITINFO");
	for(map<string, string>::const_iterator i = theMap.begin(); i != theMap.end(); i++)
	{
		//string s = list[i];
		string s1 = (*i).first;
		string s2 = (*i).second;
		L("" << s1 << " : " << s2);
	}
	*/
	
	// add unit to different groups
	for(int i = 1; i <= numOfUnits; i++){
		UnitType* me = &unittypearray[i];			
		if(me->side == -1){// filter out neutral units
		}	
		else{			
			int UnitCost = me->def->metalCost * METAL2ENERGY + me->def->energyCost;
			
			L("name: " << me->def->name << ", : " << me->def->humanName);
			CSunParser* attackerparser = new  CSunParser(ai);
			char weaponnumber[10] = "";
			attackerparser->LoadVirtualFile(me->def->filename.c_str());
			L(me->def->filename.c_str());
			me->TargetCategories.resize(me->def->weapons.size());
			//me->airOnly = false; // HACK, needs to be done for each weapon...
			for(int w = 0; w != me->def->weapons.size(); w++){
				itoa(w,weaponnumber,10);
				//L("pre string ans;");
				string ans;
				//ans = "";
				//L("pre me->def->weapons[i].name: ");
				L("me->def->weapons[i].name: " << me->def->weapons[w].name);
				L("me->def->weapons[w].def->name: " << me->def->weapons[w].def->name);
				//string sum = string("UNITINFO\\");// + string("\\toairweapon");
				//L("sum: " << sum);
				//weaponParser->GetDef(ans,"0", sum);
				//L("ans: " << ans);
				L("onlyTargetCat: " << me->def->weapons[w].onlyTargetCat);
				L("badTargetCat: " << me->def->weapons[w].badTargetCat);
				L("onlyTargetCategory: " << me->def->weapons[w].def->onlyTargetCategory);			
				
				attackerparser->GetDef(me->TargetCategories[w],"-1",string("UNITINFO\\OnlyTargetCategory") + string(weaponnumber));
			}	
			delete attackerparser;

			me->DPSvsUnit.resize(numOfUnits+1);
			for(int v = 1; v <= numOfUnits; v++){
				me->DPSvsUnit[v] = GetDPSvsUnit(me->def,unittypearray[v].def);
				//L(me->def->humanName << " has a DPS of : " << me->DPSvsUnit[v] << " against: " << unittypearray[v].def->humanName);
			}		
			

			if(me->def->speed && me->def->minWaterDepth <= 0){
				//if(me->def->movedata->moveType == MoveData::Ground_Move || me->def->movedata->moveType == MoveData::Hover_Move){
					if(me->def->buildOptions.size()){						
						ground_builders[me->side].push_back(i);
						me->category = CAT_BUILDER;
					}
					else if(!me->def->weapons.empty() && !me->def->weapons.begin()->def->stockpile){
						ground_attackers[me->side].push_back(i);
						me->category = CAT_G_ATTACK;
					}
				//}
			}
			else if (!me->def->canfly){
				if(me->def->minWaterDepth <= 0){
					if(me->def->buildOptions.size()> 1){
						ground_factories[me->side].push_back(i);
						me->category = CAT_FACTORY;
					}
					else
					{
						if (!me->def->weapons.empty() && !me->def->weapons.begin()->def->stockpile){
							ground_defences[me->side].push_back(i);
							me->category = CAT_DEFENCE;
						}
						if(me->def->makesMetal){
							metal_makers[me->side].push_back(i);
							me->category = CAT_MMAKER;
						}
						if(me->def->extractsMetal){
							metal_extractors[me->side].push_back(i);
							me->category = CAT_MEX;
						}
						if((me->def->energyMake - me->def->energyUpkeep) / UnitCost > 0.002 || me->def->tidalGenerator || me->def->windGenerator){
							if(me->def->minWaterDepth <= 0 && !me->def->needGeo){
								ground_energy[me->side].push_back(i);
								me->category = CAT_ENERGY;
							}
						}
						if(me->def->energyStorage / UnitCost > 0.2){
							energy_storages[me->side].push_back(i);
							me->category = CAT_ESTOR;
						}
						if(me->def->metalStorage / UnitCost > 0.1){
							metal_storages[me->side].push_back(i);
							me->category = CAT_MSTOR;
						}

					}
				}
			}
		}
	}
	//L("All units added!");
	DebugPrint();
	char k[200];
	sprintf(k,"UnitTable loaded in %fs",ai->math->TimerSecs());
	ai->cb->SendTextMsg(k,0);
	L(k);
}


bool CUnitTable::CanBuildUnit(int id_builder, int id_unit)
{
	// look in build options of builder for unit
	for(int i = 0; i != unittypearray[id_builder].canBuildList.size(); i++){
		if(unittypearray[id_builder].canBuildList[i] == id_unit)
			return true;
	}
	// unit not found in builder´s buildoptions
	return false;
}

// dtermines sides of unittypearray[ by recursion
void CUnitTable::CalcBuildTree(int unit)
{
	// go through all possible build options and set side if necessary
	for(int i = 0; i != unittypearray[unit].canBuildList.size(); i++)
	{
		// add this unit to targets builtby-list
		unittypearray[unittypearray[unit].canBuildList[i]].builtByList.push_back(unit);

		if(unittypearray[unittypearray[unit].canBuildList[i]].side == -1)
		{
			// unit has not been checked yet, set side as side of its builder and continue 
			unittypearray[unittypearray[unit].canBuildList[i]].side = unittypearray[unit].side;			
			CalcBuildTree(unittypearray[unit].canBuildList[i]);
		}
		
		// if already checked end recursion	
	}
}

void CUnitTable::DebugPrint()
{
	if(!unitList)
		return;
	// for debugging
	char filename[80]=ROOTFOLDER"CUnitTable Debug.log";	
	FILE *file = fopen(filename, "w");
	for(int i = 1; i <= numOfUnits; i++)
	{
		if(unittypearray[i].side != -1){
			fprintf(file, "ID: %i\nName:         %s \nSide:         %s",  i,
				unitList[i-1]->humanName.c_str(), sideNames[unittypearray[i].side].c_str());
			
				fprintf(file, "\nCan Build:    ");
			for(int j = 0; j != unittypearray[i].canBuildList.size(); j++)
			{
				fprintf(file, "%s ", unittypearray[unittypearray[i].canBuildList[j]].def->humanName.c_str());
			}
			fprintf(file, "\nBuilt by:     ");

			for(int k = 0; k != unittypearray[i].builtByList.size(); k++)
			{
				fprintf(file, "%s ", unittypearray[unittypearray[i].builtByList[k]].def->humanName.c_str());
			}
			fprintf(file, "\n\n");
		}
	}
	for(int s = 0; s < numOfSides; s++)
	{
		for (int l = 0; l != all_lists.size(); l++)
		{
			fprintf(file, "\n\n%s:\n", sideNames[s].c_str());
			for(int i = 0; i != all_lists[l][s].size(); i++)
				fprintf(file, "%s\n", unittypearray[all_lists[l][s][i]].def->humanName.c_str());
		}		
	}
	fclose(file);
}

float CUnitTable::GetMaxRange(const UnitDef* unit)
{
	float max_range = 0;

	for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++)
	{
		if(i->def->range > max_range)
			max_range = i->def->range;
	}

	return max_range;
}

float CUnitTable::GetMinRange(const UnitDef* unit)
{
	float min_range = FLT_MAX;

	for(vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++)
	{
		if(i->def->range < min_range)
			min_range = i->def->range;
	}

	return min_range;
}

/*
Temp/test for the new unit data analyser

*/
void CUnitTable::Init_nr2()
{
	//int timetaken = clock();
	ai->math->TimerStart();
	/*
	// get number of units and alloc memory for unit list
	numOfUnits = ai->cb->GetNumUnitDefs();
	// one more than needed because 0 is dummy object (so UnitDef->id can be used to adress that unit in the array) 
	unittypearray = new UnitType[numOfUnits+1];
	unittypearray[0].side = -1;
	// get unit defs from game and stick them in the unittypearray[] array
	unitList = new const UnitDef*[numOfUnits];
	ai->cb->GetUnitDefList(unitList);
	for (int i = 1; i <= numOfUnits; i++){
		unittypearray[i].def=unitList[i-1];
	}
	// add units to UnitTable
	for(int i = 1; i <= numOfUnits; i++){		
		// side has not been assigned - will be done later
		unittypearray[i].side = -1;	
		unittypearray[i].category = -1;
		// get build options
		for(map<int, string>::const_iterator j = unittypearray[i].def->buildOptions.begin(); j != unittypearray[i].def->buildOptions.end(); j++){
			unittypearray[i].canBuildList.push_back(ai->cb->GetUnitDef(j->second.c_str())->id);
		}
	}	
	// now set the sides and create buildtree
	for(int s = 0; s < numOfSides; s++){
		// set side of the start unit (eg commander) and continue recursively
		unittypearray[startUnits[s]].side = s;
		//L("Calcing buildtree of: " << startUnits[s]);
		CalcBuildTree(startUnits[s]);
	}
	*/
	
	// Setup stuff:
	string mapfile = ai->cb->GetMapName();
	mapfile.resize(mapfile.size()-3);
	mapfile += string("smd");
	mapfile = "maps\\" + mapfile;
	string strsection;
	string posstring;
	string destination;
	
	
	
	//L("Buildtrees calculated");
	// add unit to different groups
	for(int i = 1; i <= numOfUnits; i++){
		//LNE("Unit to test: " << unittypearray[i].def->humanName << " ID " <<  unittypearray[i].def->id);
		//int UnitCost = unittypearray[i].def->metalCost * METAL2ENERGY + unittypearray[i].def->energyCost;
		// Lets make data available for easy use:
		//int unitMetalCost = unittypearray[i].def->metalCost;
		//int unitEnergyCost = unittypearray[i].def->energyCost;
		//bool canBuildStuff = unittypearray[i].def->buildOptions.size() > 0;
		bool canMove = unittypearray[i].def->speed > 0;
		//if(unittypearray[i].def->canmove != canMove)
		//	L("unittypearray[i].def->speed: " << unittypearray[i].def->speed << ", canMove: " << canMove << ", unittypearray[i].def->canmove: " << unittypearray[i].def->canmove);
		//assert(unittypearray[i].def->canmove == canMove);
		
		//bool canMoveOnLand =  unittypearray[i].def->movedata->moveType == MoveData::Ground_Move || unittypearray[i].def->movedata->moveType == MoveData::Hover_Move;
		//bool canMoveInWater = unittypearray[i].def->movedata->moveType == MoveData::Hover_Move || unittypearray[i].def->movedata->moveType == MoveData::Ship_Move;
		
		// Test if the unit is initialized
		if(unittypearray[i].side == -1)
		{
			L("Unit with no side filtered out:" << unittypearray[i].def->humanName << " ID " <<  unittypearray[i].def->id);
			continue;
		}
		L("********************************************************");
		L("Unit to test: " << unittypearray[i].def->humanName << ", "<< unittypearray[i].def->name);
		
		// **************** Start of energy data mining ***********************
		{
			float energyProduction = 0;
			// See if it cost energy
			if(unittypearray[i].def->energyCost > 0)
			{
				// Its not free to make
			}
			
			// Find out if it costs energy to have
			if(unittypearray[i].def->energyUpkeep != 0)
			{
				// It have an energy upkeep
				energyProduction -= unittypearray[i].def->energyUpkeep;
				//L("It have an energy upkeep: " << unittypearray[i].def->energyUpkeep);
			}
			
			// Find out if it makes energy
			if(unittypearray[i].def->energyMake != 0)
			{
				// It just makes energy
				energyProduction += unittypearray[i].def->energyMake;
				//L("It makes energy : " << unittypearray[i].def->energyMake);
				// What is the energy comming from ?
				// Make sure its not from windGenerator/tidalGenerator
				
				/*TODO: Solar Collector must be found*/
			}
			
			// Can it make more power from somthing ?
			// windGenerator ?
			if(unittypearray[i].def->windGenerator > 0) // Is this the power or a bool ?  and can it be negative ?
			{
				// It makes (some) power from wind
				//L("unittypearray[i].def->windGenerator: " << unittypearray[i].def->windGenerator);
				//L("ai->cb->GetMinWind(): " << ai->cb->GetMinWind() << ", ai->cb->GetMaxWind(): " << ai->cb->GetMaxWind());
				L("Its a wind generator");
				/* TODO: Refactor this */
				strsection = "MAP\\ATMOSPHERE";
				destination = strsection + string("\\MinWind");
				ai->parser->LoadVirtualFile(mapfile);
				ai->parser->GetDef(posstring,"-1",destination.c_str());
				int minWind = atoi(posstring.c_str());
				destination = strsection + string("\\MaxWind");
				ai->parser->GetDef(posstring,"-1",destination.c_str());
				int maxWind = atoi(posstring.c_str());
				//L("paresd MinWind: " << minWind << ", maxWind: " << maxWind);
				energyProduction += ((minWind + maxWind) / 2.0);
			}
			
			// tidalGenerator ?
			if(unittypearray[i].def->tidalGenerator > 0) // Is this the power or a bool ?  and can it be negative ?
			{
				// It makes (some) power from water
				//L("unittypearray[i].def->tidalGenerator: " << unittypearray[i].def->tidalGenerator);
				L("Its a tidal generator");
				energyProduction += ai->cb->GetTidalStrength();
			}
			
			// Can it store energy ?
			if(unittypearray[i].def->energyStorage > 0)
			{
				// It can store energy
				L("It can store energy: " << unittypearray[i].def->energyStorage);
			}
			
			if(energyProduction != 0)
				L("Total energy production: " << energyProduction);
			// Is there any more energy things to take into the calc ?
			/* TODO: weapons need some energy */
		}
		// **************** End of energy data mining ***********************
		
		
		
		
		// **************** Start of metal data mining ***********************
		{
			float metalProduction = 0;
			// See if it cost metal
			if(unittypearray[i].def->metalCost > 0)
			{
				// Its not free to make
			}
			
			// Find out if it costs metal to have
			if(unittypearray[i].def->metalUpkeep != 0)
			{
				// It have an metal upkeep
				metalProduction -= unittypearray[i].def->metalUpkeep;
			}
			
			// Find out if it makes metal
			if(unittypearray[i].def->metalMake != 0)
			{
				// It just makes metal
				metalProduction += unittypearray[i].def->metalMake;
				
			}
			
			// Is it a (energy draining) metalmaker ?
			if(unittypearray[i].def->makesMetal > 0)
			{
				// Its a metalmaker
				assert(unittypearray[i].def->isMetalMaker);
				metalProduction += unittypearray[i].def->makesMetal;
				
			}
			else
			{
				// Its not a metalmaker
				assert(!unittypearray[i].def->isMetalMaker);
			}
			
			// Is it a metal extractor ?
			if(unittypearray[i].def->extractsMetal > 0) // Is this the power or a bool ?  and can it be negative ?
			{
				// Its a metal extractor
				// Make sure it have a extractRange
				//L("unittypearray[i].def->extractsMetal: " << unittypearray[i].def->extractsMetal << ", unittypearray[i].def->extractRange: " << unittypearray[i].def->extractRange);
				L("Its a mex, power: " << unittypearray[i].def->extractsMetal);
				assert(unittypearray[i].def->extractRange > 0);
				
				/* Use the extractRange here */
			}
			else
			{
				// Make sure it dont have a extractRange
				//if(!(unittypearray[i].def->extractRange <= 0))
				//L("unittypearray[i].def->extractsMetal: " << unittypearray[i].def->extractsMetal << ", unittypearray[i].def->extractRange: " << unittypearray[i].def->extractRange);
				//assert(unittypearray[i].def->extractRange <= 0);
			}
			
			
			// Can it store metal ?
			if(unittypearray[i].def->metalStorage > 0)
			{
				// It can store metal
				L("It can store metal: " << unittypearray[i].def->metalStorage);
			}
			
			if(metalProduction != 0)
				L("Total metal production: " << metalProduction);
			// Are there any more metal things to take into the calc ?
			/* TODO: some weapons need metal */
		}
		// **************** End of metal data mining ***********************
		
		
		
		
		// **************** Start of construction data mining ***********************
		{
			// See if the unit can build something
			if(unittypearray[i].def->builder)
			{
				// Test if it can build something ... logic error (air repair pad, nanotowers)
				//assert(unittypearray[i].def->buildOptions.size());
				// Test if it can move ... logic error (factories can move in XTA++)
				//assert(canMove);
				
				// See if it have construction powers
				//L("unittypearray[i].def->builder: " << unittypearray[i].def->builder << ", unittypearray[i].def->buildSpeed: " << unittypearray[i].def->buildSpeed);
				assert(unittypearray[i].def->buildSpeed  > 0.0);
				// Ok, its a normal builder
				L("Its some type of builder");
				
				
				// See if its a factory like thing
				if(unittypearray[i].def->buildOptions.size())
				{
					// Its can build 
					L("It can build so many things: " << unittypearray[i].def->buildOptions.size());
					// See if it have construction powers
					assert(unittypearray[i].def->buildSpeed  > 0.0);
					// Nukes are not here
				}
				else
				{
					L("It can NOT build anything of its own, so its a helper/nanotower/repairpad/resurrecter/capturer");
					
				}
			
				// See if it can aid construction
				if(unittypearray[i].def->buildSpeed > 0)
				{
					// Ok, it can help build (nanotowers, builders, factories+++)
					L("Its build speed is: " << unittypearray[i].def->buildSpeed << ", with a range of " << unittypearray[i].def->buildDistance);
				}
			}
			else
			{
				//L("else unittypearray[i].def->builder: " << unittypearray[i].def->builder << ", unittypearray[i].def->buildSpeed: " << unittypearray[i].def->buildSpeed);
				assert(!(unittypearray[i].def->buildOptions.size()));
				// Only builders have buildSpeed ? ... logic error (FF: ARCLIGHT )
				//assert(unittypearray[i].def->buildSpeed  <= 0.0);
				if(unittypearray[i].def->buildSpeed > 0)
					L("It can help build,  speed: " << unittypearray[i].def->buildSpeed << ", with a range of " << unittypearray[i].def->buildDistance << ", but its NOT A BUILDER: ERROR ?");
			}
			
			// Test if its a carrier/repair pad
			if(unittypearray[i].def->buildSpeed > 0 && unittypearray[i].def->isAirBase)
			{
				L("It have a repair pad (for air units)");
			}
			
			
			// See if it can make stock pileable weapons
			/* TODO: canResurrect, canCapture, canReclame...*/
			
			// Can it build or help build still, and not been catched ???
		}
		// **************** End of construction data mining ***********************
		
		
		
		
		// **************** Start of vision and detection data mining ***************************
		
		// Can it see
		if(unittypearray[i].def->losRadius > 0)
		{
			// Not blind
			L("Its los radius is: " << unittypearray[i].def->losRadius);
			/* TODO: find out something about "losHeight"  */
			
		}
		else
		{
			// Hmmm
			/* TODO: test this */
			// Fail at once
			assert(unittypearray[i].def->losRadius > 0);
		}
		
		// Can it see up in the air ?
		if(unittypearray[i].def->airLosRadius > 0)
		{
			// Can see aircrafts
			// This is losRadius*1.5
			//L("Its air los radius is: " << unittypearray[i].def->airLosRadius);
		}
		else
		{
			// Hmmm
			/* TODO: test this */
			// Fail at once
			assert(unittypearray[i].def->airLosRadius > 0);
		}
		
		// Do it have a rader ?
		if(unittypearray[i].def->radarRadius > 0)
		{
			// It have a radar
			L("It have a radar, range: " << unittypearray[i].def->radarRadius);
		}
		
		// Do it have a radar jammer ?
		if(unittypearray[i].def->jammerRadius > 0)
		{
			// It have a radar jammer
			L("It have a radar jammer, range: " << unittypearray[i].def->jammerRadius);
		}
		
		// Do it have a sonar
		if(unittypearray[i].def->sonarRadius > 0)
		{
			// It have a sonar
			L("It have a sonar, range: " << unittypearray[i].def->sonarRadius);
		}
		
		// Do it have a sonar jammer ?
		if(unittypearray[i].def->sonarJamRadius > 0)
		{
			// It have a sonar jammer
			L("It have a sonar jammer, range: " << unittypearray[i].def->sonarJamRadius);
		}
		
		// Is it stealthy (not seen on radar) ?
		if(unittypearray[i].def->stealth)
		{
			// Its stealthy
			L("Its stealthy");
		}
		
		// Can it cloak ?
		if(unittypearray[i].def->canCloak)
		{
			// It can cloak
			L("It can cloak");
			// Do it want to start cloaked ?
			if(unittypearray[i].def->startCloaked)
			{
				// It starts cloaked
			}
			
			// What is the energy cost of staying cloaked ?
			if(unittypearray[i].def->cloakCost > 0)
			{
				// It costs something
			}
			
			// What is the energy cost of moving while cloaked ?
			if(unittypearray[i].def->cloakCostMoving > unittypearray[i].def->cloakCost)
			{
				// It costs more to move while cloaked
				// It better move too ... logic error: same as not moveing ?
				
				//L("canMove: " << canMove << ", unittypearray[i].def->cloakCost: " << unittypearray[i].def->cloakCost << ", unittypearray[i].def->cloakCostMoving: " << unittypearray[i].def->cloakCostMoving);
				assert(canMove);
			}
			
			// What is the decloak distance ?
			if(unittypearray[i].def->decloakDistance > 0)
			{
				// It can be spotted
			}
			else
			{
				// It can not be spotted ??
				/* TODO: test this */
				// FF: COMMAND CRUISER ends up here
				//assert(unittypearray[i].def->decloakDistance > 0);
			}
			
		}
		else
		{
			// It cant start cloaked
			/* TODO: test this */
			assert(!unittypearray[i].def->startCloaked);
			assert(unittypearray[i].def->cloakCost <= 0);
			assert(unittypearray[i].def->cloakCostMoving <= 0);
		}
		
		// Is it a targeting facility
		if(unittypearray[i].def->targfac)
		{
			//Its a targeting facility
			L("Its a targeting facility");
		}
		
		
		// Are there any more things to take into the calc ?
		/* TODO: take on/off ability into account ?*/
		
		// **************** End of vision and detection data mining ***************************
		
		
		
		
		// **************** Start of health and armor data mining ***************************
		
		// What health do it have ?
		if(unittypearray[i].def->health > 0)
		{
			// Yep
		}
		else
		{
			// Its undead
			assert(unittypearray[i].def->health > 0);
		}
		
		// What armor type do it have ?
		if(unittypearray[i].def->armorType >= 0)
		{
			/* TODO: find out what this means */
			L("Its armor type: " << unittypearray[i].def->armorType);
		}
		
		// What armored Multiple do it have ???
		if(unittypearray[i].def->armoredMultiple > 0)
		{
			/* TODO: find out what this means */
			L("Its armored Multiple: " << unittypearray[i].def->armoredMultiple);
		}
		else
		{
			// Hmm
			// Just fail ?
			assert(unittypearray[i].def->armoredMultiple > 0);
		}
		
		// Do it heal in combat ?
		if(unittypearray[i].def->autoHeal > 0)
		{
			// It heals all the time
			L("It heals all the time, speed: : " << unittypearray[i].def->autoHeal);
		}
		
		// Do it heal when idle ?
		if(unittypearray[i].def->idleAutoHeal > 0)
		{
			// It heals when idle
			// When do it start ideling ?
			if(unittypearray[i].def->idleTime >= 0)
			{
				// This is here only for reference
			}
			
		}
		
		// hideDamage
		if(unittypearray[i].def->hideDamage)
		{
			// Do this mean it hides its damage from the enemy ?
			/* TODO: find out what this means */
			L("It hides its damage ????");
		}
		
		// Are there any more things to take into the calc ?
		/* TODO: popup turrets (on/off), .... */

		// **************** End of health and armor data mining ***************************
		
		
		
		
		// **************** Start of commander data mining ***************************
		
		// Is it a commander ?
		if(unittypearray[i].def->isCommander)
		{
			// Its a commander
			L("Its a commander");
		}
		
		// Can it D-Gun  ?
		if(unittypearray[i].def->canDGun)
		{
			// It can D-Gun
			L("It can D-Gun");
		}
		
		// Are there any more things to add here ?

		// **************** End of commander data mining ***************************
		
		
		
		
		// **************** Start of building data mining ***************************
		if(!canMove)
		{
			L("Its a building");
			
			// TODO: find out if this means something:
			if(unittypearray[i].def->maxSlope > 0)
			{
				//L("unittypearray[i].def->maxSlope: " << unittypearray[i].def->maxSlope);
			}
			
			// Test the buildings maximum ground height difference (for building)
			if(unittypearray[i].def->maxHeightDif < FLT_MAX)
			{
				L("buildings maximum ground height difference: " << unittypearray[i].def->maxHeightDif);
			}
			
			if(unittypearray[i].def->minWaterDepth > 0)
			{
				L("unittypearray[i].def->minWaterDepth: " << unittypearray[i].def->minWaterDepth);
			}
			
			if(unittypearray[i].def->maxWaterDepth > 0)
			{
				L("unittypearray[i].def->maxWaterDepth: " << unittypearray[i].def->maxWaterDepth);
			}
			
			if(unittypearray[i].def->floater)
			{
				L("Its a floater");
			}
			
			if(unittypearray[i].def->xsize > 0)
			{
				L("Its x size: " << unittypearray[i].def->xsize);
			}
			
			if(unittypearray[i].def->ysize > 0)
			{
				L("Its y size: " << unittypearray[i].def->ysize);
			}
		
		
		
		}
		// **************** End of building data mining ***************************
		
		
		
		
		// **************** Start of self destruction data mining ***************************
		// **************** End of self destruction data mining ***************************
		
		
		// **************** Start of transport data mining ***************************
		// **************** End of transport data mining ***************************
		
		
		// **************** Start of movement data mining ***************************
		// **************** End of movement data mining ***************************
		
		
		// **************** Start of weapon data mining ***************************
		// **************** End of weapon data mining ***************************
		
		
		
		
		// **************** Start of temp stuff ***************************
		
		// buildTime
		if(unittypearray[i].def->buildTime > 0)
		{
			// Its not instant to build 
		}
		else
		{
			// Can this be ?
			/* TODO: test this */
			// Fail at once
			assert(unittypearray[i].def->buildTime > 0);
		}
		
		// geoThermal ?
		if(unittypearray[i].def->needGeo)
		{
			// It needs a geoThermal
			L("It needs a geoThermal");
		}
		
		// power ?
		if(unittypearray[i].def->power > 0)
		{
			/* TODO: find out what this is */
			L("Its power is: " << unittypearray[i].def->power);
		}
		
		// category ?
		if(unittypearray[i].def->category)
		{
			/* TODO: find out how this works */
		}
		
		// controlRadius ?
		if(unittypearray[i].def->controlRadius > 0)
		{
			/* TODO: find out how this works */
			L("Its controlRadius is: " << unittypearray[i].def->controlRadius);
		}
		
		// mass ?
		if(unittypearray[i].def->mass > 0)
		{
			/* TODO: find out how this works */
		}
		
		
		// **************** End of temp stuff ***************************
		
		
		
		/*
		// filter out neutral units
		if(unittypearray[i].side == -1){
			////L("Unit with no side filtered out:" << unittypearray[i].def->humanName << " ID " <<  unittypearray[i].def->id);
		}
		// check if builder or factory
		else{
			////L("Unit with side taken: " << unittypearray[i].def->humanName << " ID " <<  unittypearray[i].def->id << " Side: " << unittypearray[i].side);
			if(unittypearray[i].def->speed && unittypearray[i].def->minWaterDepth <= 0){
				//if(unittypearray[i].def->movedata->moveType == MoveData::Ground_Move || unittypearray[i].def->movedata->moveType == MoveData::Hover_Move){
					if(unittypearray[i].def->buildOptions.size()){						
						ground_builders[unittypearray[i].side].push_back(i);
						unittypearray[i].category = CAT_BUILDER;
					}
					else if(!unittypearray[i].def->weapons.empty() && !unittypearray[i].def->weapons.begin()->def->stockpile){
						string defaultvalue = "-1";
						CSunParser* attackerparser = new  CSunParser(ai);
						attackerparser->LoadVirtualFile(unittypearray[i].def->filename.c_str());
						attackerparser->GetDef(defaultvalue,"-1","UNITINFO\\OnlyTargetCategory1");
						if(defaultvalue == string("-1") || defaultvalue == string("GROUND")){
							ground_attackers[unittypearray[i].side].push_back(i);
							unittypearray[i].category = CAT_G_ATTACK;
						}
						delete attackerparser;
					}
				//}
			}
			else if (!unittypearray[i].def->canfly){
				if(unittypearray[i].def->minWaterDepth <= 0){
					if(unittypearray[i].def->buildOptions.size()){
						ground_factories[unittypearray[i].side].push_back(i);
						unittypearray[i].category = CAT_FACTORY;
					}
					else
					{
						if (!unittypearray[i].def->weapons.empty() && !unittypearray[i].def->weapons.begin()->def->stockpile){
							string defaultvalue = "-1";
							CSunParser* attackerparser = new  CSunParser(ai);
							attackerparser->LoadVirtualFile(unittypearray[i].def->filename.c_str());
							attackerparser->GetDef(defaultvalue,"-1","UNITINFO\\OnlyTargetCategory1");
							if(defaultvalue == string("-1") || defaultvalue == string("GROUND")){
								ground_defences[unittypearray[i].side].push_back(i);
								unittypearray[i].category = CAT_DEFENCE;
							}
							delete attackerparser;
						}
						if(unittypearray[i].def->makesMetal){
							metal_makers[unittypearray[i].side].push_back(i);
							unittypearray[i].category = CAT_MMAKER;
						}
						if(unittypearray[i].def->extractsMetal){
							metal_extractors[unittypearray[i].side].push_back(i);
							unittypearray[i].category = CAT_MEX;
						}
						if((unittypearray[i].def->energyMake - unittypearray[i].def->energyUpkeep) / UnitCost > 0.002 || unittypearray[i].def->tidalGenerator || unittypearray[i].def->windGenerator){
							if(unittypearray[i].def->minWaterDepth <= 0 && !unittypearray[i].def->needGeo){
								ground_energy[unittypearray[i].side].push_back(i);
								unittypearray[i].category = CAT_ENERGY;
							}
						}
						if(unittypearray[i].def->energyStorage / UnitCost > 0.2){
							energy_storages[unittypearray[i].side].push_back(i);
							unittypearray[i].category = CAT_ESTOR;
						}
						if(unittypearray[i].def->metalStorage / UnitCost > 0.1){
							metal_storages[unittypearray[i].side].push_back(i);
							unittypearray[i].category = CAT_MSTOR;
						}

					}
				}
			}
		}*/
	}
	
	//L("All units added!");
	//DebugPrint();
	float timetaken = ai->math->TimerSecs();
	char k[200];
	sprintf(k,"UnitTable 2 loaded in %f sec",timetaken);
	ai->cb->SendTextMsg(k,0);
	L(k);
}

