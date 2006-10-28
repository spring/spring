#include "EconomyManager.h"

CEconomyManager::CEconomyManager(AIClasses* ai)
{
	this->ai=ai;
}

CEconomyManager::~CEconomyManager()
{
}


/*float CEconomyManager::GetScore(const UnitDef* unit)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
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
	float RandNum = (RANDINT%20 + 10)/10;
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
			dps /= 5; // Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
		}
        Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(1))
				* pow(GetMaxRange(unit)+200,float(1.2))
				* pow(dps,float(1))
				* pow(unit->speed + 40,float(0.5))
				* pow(Hitpoints,float(1))
				* pow(RandNum,float(4))
				//* pow(buildTimeFix,float(-0.35))
				* pow(Cost,float(-0.5));
		
		// AA hack (stop mass air)
		if(unit->canfly)
			Benefit *= 0.7; //slight reduction to the feasability of air
		break;
	case CAT_DEFENCE:
		//L("Getting DEFENCE SCORE FOR: " << unit->humanName);
		Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(1))
				* pow(GetMaxRange(unit),float(2))
				* pow(GetCurrentDamageScore(unit),float(1.5))
				* pow(Hitpoints,float(0.5))
				* pow(RandNum,float(4))
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
	if(buildTimeRatio > 100)
		buildTimeRatio = 10;
	else if (buildTimeRatio > 0.5)
		buildTimeRatio = sqrt(buildTimeRatio);
	else
	{
		// This will be slow... Do a cut off.
		buildTimeRatio *= 0.001;
	}
	//L("buildTimeRatio: " << buildTimeRatio);
	
	
	//L("Benefit: " << Benefit);
	//L("Unit: " << unit->humanName << " Has a score of : " << Benefit);
	float EScore = Benefit/(Cost+CurrentIncome);
	//float EScore = Benefit * pow(buildTimeRatio,float(4));
	//L("EScore: " << EScore);
	return EScore;
}
const UnitDef* CEconomyManager::GetUnitByScore(int builder, int category)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
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
}*/


