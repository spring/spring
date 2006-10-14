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

UnitType* CUnitTable::GetUnitType(const UnitDef* unitdef)
{
	return &unittypearray[unitdef->id];
}

UnitType* CUnitTable::GetUnitType(int unit)
{
	assert(ai->cb->GetUnitDef(unit) != NULL);
	return &unittypearray[ai->cb->GetUnitDef(unit)->id];
}

void CUnitTable::UpdateChokePointArray()
{
	ai->math->StartTimer(updateChokePointArrayTime);
	vector<float> EnemyCostsByMoveType;
	EnemyCostsByMoveType.resize(ai->pather->NumOfMoveTypes + 1);
	for(int i = 0; i < ai->pather->totalcells;i++){
		ai->dm->ChokePointArray[i] = 0.00001;
	}	
	for(int i = 0; i < ai->pather->NumOfMoveTypes + 1;i++){
		EnemyCostsByMoveType[i] = 0;
	}
	for(int i = 1; i <= ai->cb->GetNumUnitDefs(); i++){
		if(unittypearray[i].side != -1 && unittypearray[i].def->speed > 0){
			if(!unittypearray[i].def->canfly){
				EnemyCostsByMoveType[unittypearray[i].def->moveType] += ai->dc->TheirArmy[i].Cost;	
			}
			else{
				EnemyCostsByMoveType[ai->pather->NumOfMoveTypes] += ai->dc->TheirArmy[i].Cost;
			}
		}
	}	
	for(int i = 0; i < ai->pather->NumOfMoveTypes + 1;i++){
		EnemyCostsByMoveType[i] /= ai->dc->TheirTotalCost;
		if(EnemyCostsByMoveType[i] != 0)
			for(int c = 0; c < ai->pather->totalcells; c++){
				ai->dm->ChokePointArray[c] += ai->dm->ChokeMapsByMovetype[i][c] * EnemyCostsByMoveType[i];
			}		
	}
	ai->math->StopTimer(updateChokePointArrayTime);
	ai->debug->MakeBWTGA(ai->dm->ChokePointArray,ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight,"CHOKEMAP");
}


float CUnitTable::GetScore(const UnitDef* unit)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
{
	//L("Getting Score for: " << unit->humanName);
	float Cost = (unit->metalCost * METAL2ENERGY) + unit->energyCost;
	float CurrentIncome = INCOMEMULTIPLIER*(ai->cb->GetEnergyIncome() + (ai->cb->GetMetalIncome() * METAL2ENERGY)) + ai->cb->GetCurrentFrame()/2;
	float Hitpoints = unit->health;
	float Benefit = 0;
	float dps = 0;
	int unitcounter=0;
	bool candevelop = false;
	// Test: make it less random:
	float RandNum = ai->math->RandNormal(4,2.5,1) + 1;
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
		Benefit = (unit->metalMake + unit->makesMetal - unit->metalUpkeep) / (unit->energyUpkeep + 0.01) + 0.01;
		break;
	case CAT_G_ATTACK:
		dps = ai->dc->GetCurrentDamageScore(unit);
		if(unit->canfly && !unit->hoverAttack){
			dps /= 2; // Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
			if(unit->weapons.front().def->dropped){
				dps /= 3;
			}
		}
        Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit)+200,float(2))
				* pow(dps,float(1))
				* pow(unit->speed + 40,float(1))
				* pow(Hitpoints,float(1))
				* pow(RandNum,float(1))
				//* pow(buildTimeFix,float(-0.35))
				* pow(Cost,float(-0.5));		
		// AA hack (stop mass air)
		if(unit->canfly && ai->uh->AllUnitsByCat[CAT_FACTORY]->size() == 0)
			Benefit *= 0.3;//0.25; //slight reduction to the feasability of air
		break;
	case CAT_DEFENCE:
		//L("Getting DEFENCE SCORE FOR: " << unit->humanName);
		Benefit = pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit),float(2))
				* pow(ai->dc->GetCurrentDamageScore(unit),float(1.5))
				* pow(Hitpoints,float(0.5))
				* pow(RandNum,float(1.5))
				//* pow(buildTimeFix,float(-0.6));
				* pow(Cost,float(-1));
				//* pow(Cost,float(-(3-(5*(ai->em->Need4Def-0.5))))); // Math error
		//L("Done getting DEFENCE SCORE: " << Benefit << ", " << ai->dc->GetCurrentDamageScore(unit));
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
		//if(!candevelop)
			//Benefit = 0;
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
	
	//L("Unit: " << unit->humanName << " Has a score of : " << Benefit);
	float EScore = Benefit;//(Cost+CurrentIncome);
	//float EScore = Benefit * pow(buildTimeRatio,float(4));
	//L("EScore: " << EScore);
	return EScore;
}

/**  TODO!!!!  -  the global normalization data is not made yet
An exprimental function only, might be used for factories first. 
All the extra data will be ignored for now, but a unit will get score from all categories that is listed in needMask.
If needMask is 0 then the units own categoryMask is used.
(A score of 0 will mean that the unit is too costly or useless)
*/
float CUnitTable::GetFullScore(const UnitDef* unit, unsigned needMask, float buildSpeedEstimate, int maxTime, float budgetE, float budgetM)
{
	//L("Getting Score for: " << unit->humanName);
	UnitType* ut = GetUnitType(unit);
	if(needMask == 0)
	{
		needMask = ut->categoryMask;
	}
	
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
	if(needMask & CATM_ENERGY) {
		float Benefit2 = unit->energyMake - unit->energyUpkeep;
		if (unit->windGenerator){
			Benefit2 += ai->cb->GetMinWind();// + ((ai->cb->GetMaxWind() - ai->cb->GetMinWind()) / 5);
		}
		if (unit->tidalGenerator){
			Benefit2 += ai->cb->GetTidalStrength();
		}
		
		if(unit->needGeo)
			Benefit2 = 0;
		Benefit2 /= Cost;
		Benefit += Benefit2;
		//Benefit *= pow(buildTimeFix,float(-1.1));
	}
	if(needMask & CATM_MEX) {
		Benefit += pow(unit->extractsMetal,float(4));
	}
	if(needMask & CATM_MMAKER) {
		Benefit += (unit->metalMake + unit->makesMetal - unit->metalUpkeep) / (unit->energyUpkeep + 0.01) + 0.01;
	}
	if(needMask & CATM_ATTACK) {
		dps = ai->dc->GetCurrentDamageScore(unit);
		if(unit->canfly && !unit->hoverAttack){
			dps /= 6; // Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
			if(unit->weapons.front().def->dropped){
				dps /= 2;
			}
		}
        float Benefit2 = pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit)+200,float(2))
				* pow(dps,float(1))
				* pow(unit->speed + 40,float(1))
				* pow(Hitpoints,float(1))
				* pow(RandNum,float(1.5))
				//* pow(buildTimeFix,float(-0.35))
				* pow(Cost,float(-0.5));		
		// AA hack (stop mass air)
		if(unit->canfly)
			Benefit2 *= 0.5;//0.25; //slight reduction to the feasability of air
		Benefit += Benefit2;
	}
	if(needMask & CATM_DEFENCE) {
		//L("Getting DEFENCE SCORE FOR: " << unit->humanName);
		Benefit += pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit),float(2))
				* pow(ai->dc->GetCurrentDamageScore(unit),float(1.5))
				* pow(Hitpoints,float(0.5))
				* pow(RandNum,float(1.5))
				//* pow(buildTimeFix,float(-0.6));
				* pow(Cost,float(-1));
				//* pow(Cost,float(-(3-(5*(ai->em->Need4Def-0.5))))); // Math error
		//L("Done getting DEFENCE SCORE: " << Benefit << ", " << ai->dc->GetCurrentDamageScore(unit));
	}
	if(needMask & (CATM_BUILDER | CATM_CANMOVE)) {
		//L("Getting Score For Builder");
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			//L("category: " << unittypearray[unittypearray[unit->id].canBuildList[i]].category);
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_FACTORY){
				candevelop = true;
			}
		}
		 float Benefit2 = pow(unit->buildSpeed,float(1))
				* pow(unit->speed,float(0.5))
				* pow(Hitpoints,float(0.3))
				* pow(RandNum,float(0.4));
				//* pow(Cost,float(-1));
		if(!candevelop)
			Benefit2 = 0;
		Benefit += Benefit2;
	}
	if(needMask & (CATM_FACTORY | CATM_BUILDING)) {
		float Benefit2 = 0;
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_G_ATTACK){
				Benefit2 += GetScore(unittypearray[unittypearray[unit->id].canBuildList[i]].def);
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
			Benefit2 /= (unitcounter * pow(float(ai->uh->AllUnitsByType[unit->id]->size() + 1),float(3)));
		else
			Benefit2 = 0;
		Benefit += Benefit2;
		//L("Factory: " << unit->humanName << " Has a score of: " << Benefit);
	}
	if(needMask & CATM_MSTOR) {
		Benefit += pow((unit->metalStorage),float(1))
				* pow(Hitpoints,float(1));
				//* pow(Cost,float(-0.5));
	}
	if(needMask & CATM_ESTOR) {
		Benefit += pow((unit->energyStorage),float(1))
				* pow(Hitpoints,float(1));
				//* pow(Cost,float(-0.5));
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



/**  TODO!!!!  -  the global normalization data is not made yet
An exprimental function only.
Will make a score based on the abillities of the unit only.
The cost or current state is not taken into account.
Will be used to make the normalization data.
needMask must only have one category selected for each run.
A score of 0 will mean that something is wrong - the score function or categoryMask have a bug then.
Categories that have no real score like CATM_GEO must be filtered out.
*/
float CUnitTable::GetNonNormalizationGlobalScore(const UnitType* unitType, unsigned needMask)
{
	//L("Getting Score for: " << unit->humanName);
	const UnitDef* unit = unitType->def;
	// Ignore the cost now
	float Hitpoints = unit->health;
	float Benefit = 0;
	float dps = 0;
	int unitcounter=0;
	bool candevelop = false;
	// Test: make it less random:
	float RandNum = ai->math->RandNormal(4,3,1) + 1;
	int category = GetCategory(unit);
	if(needMask & CATM_ENERGY) {
		float Benefit2 = unit->energyMake - unit->energyUpkeep;
		if (unit->windGenerator){
			Benefit2 += ai->cb->GetMinWind();// + ((ai->cb->GetMaxWind() - ai->cb->GetMinWind()) / 5);
		}
		if (unit->tidalGenerator){
			Benefit2 += ai->cb->GetTidalStrength();
		}
		Benefit += Benefit2;
		//Benefit *= pow(buildTimeFix,float(-1.1));
	}
	if(needMask & CATM_MEX) {
		Benefit += unit->extractsMetal;
	}
	if(needMask & CATM_MMAKER) {
		Benefit += (unit->metalMake- unit->metalUpkeep) + unit->makesMetal;
	}
	if(needMask & CATM_ATTACK) {
		dps = unitType->AverageDPS;
		if(unit->canfly && !unit->hoverAttack){
			dps /= 6; // Improve to set reloadtime to the bombers turnspeed vs movespeed (eg time taken for a run)
			if(unit->weapons.front().def->dropped){
				dps /= 2;
			}
		}
        float Benefit2 = pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit)+200,float(2))
				* pow(dps,float(1))
				* pow(unit->speed + 40,float(1))
				* pow(Hitpoints,float(1));		
		// AA hack (stop mass air)
		if(unit->canfly)
			Benefit2 *= 0.5;//0.25; //slight reduction to the feasability of air
		Benefit += Benefit2;
	}
	if(needMask & CATM_DEFENCE) {
		//L("Getting DEFENCE SCORE FOR: " << unit->humanName);
		Benefit += pow((unit->weapons.front().def->areaOfEffect + 80),float(2))
				* pow(GetMaxRange(unit),float(2))
				* pow(unitType->AverageDPS,float(1.5))
				* pow(Hitpoints,float(0.5));
				//* pow(Cost,float(-(3-(5*(ai->em->Need4Def-0.5))))); // Math error
		//L("Done getting DEFENCE SCORE: " << Benefit << ", " << ai->dc->GetCurrentDamageScore(unit));
	}
	// TODO: make this smart:
	if(needMask & CATM_BUILDER && unitType->categoryMask & CATM_CANMOVE) {
		//L("Getting Score For Builder");
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			//L("category: " << unittypearray[unittypearray[unit->id].canBuildList[i]].category);
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_FACTORY){
				candevelop = true;
			}
		}
		float Benefit2 = pow(unit->buildSpeed,float(1))
				* pow(unit->speed,float(0.5))
				* pow(Hitpoints,float(0.3))
				* pow(unit->buildDistance,float(0.5));
				//* pow(Cost,float(-1));
		if(!candevelop)
			Benefit2 /= 10;
		Benefit += Benefit2;
	}
	// TODO: make this smart:
	if(needMask & CATM_FACTORY && unitType->categoryMask & CATM_BUILDING) {
		float Benefit2 = 0;
		// TODO: make this smart:
		for(int i = 0; i != unittypearray[unit->id].canBuildList.size(); i++){
			if(unittypearray[unittypearray[unit->id].canBuildList[i]].category == CAT_G_ATTACK){
				Benefit2 += GetScore(unittypearray[unittypearray[unit->id].canBuildList[i]].def);
				unitcounter++;
			}
		}
		//L("CAT_FACTORY: " << unit->humanName << " Has a sum score of : " << Benefit);
		
		if(unitcounter > 0)
			Benefit2 /= (unitcounter * pow(float(ai->uh->AllUnitsByType[unit->id]->size() + 1),float(3)));
		else
			Benefit2 = 0;
		Benefit += Benefit2;
		//L("Factory: " << unit->humanName << " Has a score of: " << Benefit);
	}
	if(needMask & CATM_MSTOR) {
		Benefit += unit->metalStorage;
	}
	if(needMask & CATM_ESTOR) {
		Benefit += unit->energyStorage;
	}
	if(needMask & CATM_RADAR) {
		Benefit += unit->radarRadius;
	}
	if(needMask & CATM_SONAR) {
		Benefit += unit->sonarRadius;
	}
	if(needMask & CATM_R_JAMMER) {
		Benefit += unit->jammerRadius;
	}
	if(needMask & CATM_S_JAMMER) {
		Benefit += unit->sonarJamRadius;
	}
	if(Benefit == 0)
	{
		assert(false);
	}
	//L("Benefit: " << Benefit);
	//L("Unit: " << unit->humanName << " Has a score of : " << Benefit);
	return Benefit;
}

const UnitDef* CUnitTable::GetUnitByScore(int builder, list<UnitType*> &unitList, unsigned needMask)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
{
	//ai->math->TimerStart();
	//L("Getting Score for category:" << category << " Builder: " << ai->cb->GetUnitDef(builder)->humanName);
	const UnitDef* tempunit;
	int side = GetSide(builder);
	float tempscore = 0;
	float bestscore = 0;

	for(list<UnitType*>::iterator i = unitList.begin(); i != unitList.end(); i++){
		if(CanBuildUnit(ai->cb->GetUnitDef(builder)->id, (*i)->def->id)){
			tempscore = GetFullScore((*i)->def, needMask);
			if (bestscore < tempscore){
				//L("Better Score found");
				bestscore = tempscore;
				tempunit = (*i)->def;
			}
		}
	}
	//L("Loop calculated");
	if(!bestscore){
		L("no scores found for unit!");
		return NULL; // State that this builder cant make any units of this type
	}
	//L("Time taken for getunitbyscore: " << ai->math->TimerSecs() << "s.");
	return tempunit;
}


const UnitDef* CUnitTable::GetUnitByScore(int builder, int category)//0 energy, 1 mex, 2mmaker, 3 ground attackers, 4 defenses, 5 builders
{
	//ai->math->TimerStart();
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
		return NULL; // State that this builder cant make any units of this type
		/*
		for(int i = 0; i != unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList.size(); i++){
				if(unittypearray[unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList[i]].category != -1){
					return unittypearray[unittypearray[ai->cb->GetUnitDef(builder)->id].canBuildList.front()].def;
				}
		}*/
	}
	//L("Time taken for getunitbyscore: " << ai->math->TimerSecs() << "s.");
	return tempunit;
}

inline bool CUnitTable::pairCMP(const std::pair<float,UnitType*> &a, const std::pair<float,UnitType*> &b)
{
	return a.first > b.first;
}

void CUnitTable::MakeGlobalNormalizationScore()
{
	assert(scoreLists.size() == 0);
	//vector< vector<pair<float,UnitType*> > > scoreLists;
	scoreLists.resize(32);
	for(int bitIndex = 0; bitIndex < 32; bitIndex++)
	{
		unsigned mask = 1 << bitIndex;
		if(mask & (CATM_ENERGY | CATM_ESTOR | CATM_MEX | CATM_MMAKER | CATM_MSTOR | CATM_RADAR | CATM_SONAR | CATM_R_JAMMER | CATM_S_JAMMER)) // CATM_BUILDER CATM_DEFENCE CATM_FACTORY CATM_KAMIKAZE CATM_LONGRANGE 
		{
			// For this mask its usefull to make a global score
			list<UnitType*> unitList = getGlobalUnitList(mask);
			if(unitList.size() > 0)
			{
				//vector<pair<float,UnitType*> > scoreList;
				for(list<UnitType*>::iterator i = unitList.begin(); i != unitList.end(); i++)
				{
					pair<float,UnitType*> data;
					data.second = *i;
					data.first = GetNonNormalizationGlobalScore(*i, mask);
					//scoreList.push_back(data);
					scoreLists[bitIndex].push_back(data);
				}
				sort(scoreLists[bitIndex].begin(), scoreLists[bitIndex].end(), &CUnitTable::pairCMP);
				L("The best unit in catm " << mask << " is " << scoreLists[bitIndex][0].second->def->humanName);
			}
		}
	}
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

list<UnitType*> CUnitTable::getGlobalUnitList(unsigned categoryMask, int side)
{
	list<UnitType*> foundList;
	for (int i = 1; i <= numOfUnits; i++){
		unsigned temp = unittypearray[i].categoryMask & categoryMask;
		// Test if some of the bits from categoryMask was lost in the 'and'
		if(temp == categoryMask)
		{
			// All the bits was preserved, so its an inclusive match
			if(side == -1 || unittypearray[i].side == side)
				foundList.push_back(&unittypearray[i]);
		}
	}
	return foundList;
}

void CUnitTable::Init()
{
	L("Initing Unittable");
	ai->math->TimerStart();
	
	// Do the units
	updateChokePointArrayTime = ai->math->GetNewTimerGroupNumber("UpdateChokePointArray()");
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
		unittypearray[i].categoryMask = 0;
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
	//CSunParser* weaponParser = new  CSunParser(ai);
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
		me->Cost = ai->math->GetUnitCost(me->def);
		if(me->side == -1){// filter out neutral units
		}	
		else{			
			int UnitCost = me->def->metalCost * METAL2ENERGY + me->def->energyCost;
			
			//L("name: " << me->def->name << ", : " << me->def->humanName);
			//CSunParser* attackerparser = new CSunParser(ai);
			//char weaponnumber[10] = "";
			//attackerparser->LoadVirtualFile(me->def->filename.c_str());
			//L(me->def->filename.c_str());
			//me->TargetCategories.resize(me->def->weapons.size());
			//me->airOnly = false; // HACK, needs to be done for each weapon...
			//for(int w = 0; w != me->def->weapons.size(); w++){
				//itoa(w,weaponnumber,10);
				//L("pre string ans;");
				//string ans;
				//ans = "";
				//L("pre me->def->weapons[i].name: ");
				//L("me->def->weapons[i].name: " << me->def->weapons[w].name);
				//L("me->def->weapons[w].def->name: " << me->def->weapons[w].def->name);
				//string sum = string("UNITINFO\\");// + string("\\toairweapon");
				//L("sum: " << sum);
				//weaponParser->GetDef(ans,"0", sum);
				//L("ans: " << ans);
				//L("onlyTargetCat: " << me->def->weapons[w].onlyTargetCat);
				//L("badTargetCat: " << me->def->weapons[w].badTargetCat);
				//L("onlyTargetCategory: " << me->def->weapons[w].def->onlyTargetCategory);			
				
				//attackerparser->GetDef(me->TargetCategories[w],"-1",string("UNITINFO\\OnlyTargetCategory") + string(weaponnumber));
			//}	
			//delete attackerparser;
			if(me->def->speed  && !me->def->canfly){ //  && ((me->def->minWaterDepth <= 0) ^ (me->def->movedata->depth ) 
				//L("Unit: " << me->def->humanName << " Slope: " << me->def->movedata->maxSlope << " depth :  " <<   me->def->movedata->depth << " depthMod :" <<   me->def->movedata->depthMod << " pathType: " <<  me->def->movedata->pathType << " slopeMod: " <<  me->def->movedata->slopeMod);
			}
			
			/*
			moveFamily: 0 = Vehicle
			moveFamily: 1 = kbot
			moveFamily: 2 = hover
			moveFamily: 3 = ship/sub
			
			moveType: 0 = Vehicle/kbot
			moveType: 1 = hover
			moveType: 2 = ship/sub
			
			slopeMod:  used for finding the move speed my spring (steepness)
			depthMod:  used for finding the move speed my spring (underwater depth)
			
			*/
			if(me->def->speed)
			{
				//L("adding mobile unit");
				if(me->def->movedata)
				{
					me->hackMoveType = 1 << me->def->movedata->pathType;
					me->moveType = 	me->def->movedata->moveType;
					// Temp only:  make the lists of move data (slow)
					if(me->def->movedata->moveType != me->def->movedata->Ship_Move)
					{
						ai->pather->slopeTypes.push_back(me->def->movedata->maxSlope); // Its a ground unit
					}
					ai->pather->crushStrengths.push_back(me->def->movedata->crushStrength);
					
					if(me->def->movedata->moveType == me->def->movedata->Ship_Move)
						ai->pather->waterUnitGroundLines.push_back(me->def->movedata->depth); // Its a ship/sub unit
					else if(me->def->movedata->moveType == me->def->movedata->Ground_Move)
						ai->pather->groundUnitWaterLines.push_back(me->def->movedata->depth); // Its a ground unit, but not a hover
					// else its a hover, and they move everywhere
						
				} else
				{
					me->hackMoveType = 0;
					me->moveType = 4; // Hack ()
				}
				KAIMoveType temp;
				temp.Assign(me->def);
				if(ai->pather->MoveTypes.size() == 0){
					ai->pather->MoveTypes.push_back(temp);
					//me->movetype = 0;
					L("Unit: " << me->def->humanName << " Has a movetype of number: " << 0 << " Slope: " << ai->pather->MoveTypes[0].MaxSlope << " Maxdepth " <<  ai->pather->MoveTypes[0].MaxDepth << " Min Depth:" <<  ai->pather->MoveTypes[0].MinDepth << " Air? " <<  ai->pather->MoveTypes[0].Air);
					//L("Movetype of number: " << me->movetype << " Slope: " << ai->pather->MoveTypes[me->movetype].MaxSlope << " Maxdepth " <<  ai->pather->MoveTypes[me->movetype].MaxDepth << " Min Depth:" <<  ai->pather->MoveTypes[me->movetype].MinDepth << " Air? " <<  ai->pather->MoveTypes[me->movetype].Air);
					//L("First movetype added");
				}
				else{
					bool foundtype = false;
					for(int i = 0; i != ai->pather->MoveTypes.size();i++){
						if(ai->pather->MoveTypes[i] == temp){
							//L("Found existing movetype");
							foundtype = true;
							//me->movetype = i;
							//L("Movetype Set");
						}
					}
					if(!foundtype){
						//L("adding new type");
						//me->movetype = ai->pather->MoveTypes.size();
						ai->pather->MoveTypes.push_back(temp);
						int moveType = ai->pather->MoveTypes.size() -1;
						L("Unit: " << me->def->humanName << " Has a movetype of number: " << moveType << " Slope: " << ai->pather->MoveTypes[moveType].MaxSlope << " Maxdepth " <<  ai->pather->MoveTypes[moveType].MaxDepth << " Min Depth:" <<  ai->pather->MoveTypes[moveType].MinDepth << " Air? " <<  ai->pather->MoveTypes[moveType].Air);
						if(!me->def->canfly) {
							//L("Unit: " << me->def->humanName << " Has a movetype of number: " << me->movetype << " Slope: " << me->def->movedata->maxSlope << " depth :  " <<   me->def->movedata->depth << " depthMod :" <<   me->def->movedata->depthMod << " moveFamily: " <<  me->def->movedata->moveFamily << " moveMath: " <<  me->def->movedata->moveMath << " moveType: " <<  me->def->movedata->moveType << " pathType: " <<  me->def->movedata->pathType << " slopeMod: " <<  me->def->movedata->slopeMod);
						}
						//L("Movetype of number: " << me->movetype << " Slope: " << ai->pather->MoveTypes[me->movetype].MaxSlope << " Maxdepth " <<  ai->pather->MoveTypes[me->movetype].MaxDepth << " Min Depth:" <<  ai->pather->MoveTypes[me->movetype].MinDepth << " Air? " <<  ai->pather->MoveTypes[me->movetype].Air);
					}
				}
			}
			
			if(me->def->speed && me->def->minWaterDepth <= 0){
				if(me->def->buildOptions.size()){						
					ground_builders[me->side].push_back(i);
					me->category = CAT_BUILDER;
				}
				else if(!me->def->weapons.empty() && !me->def->weapons.begin()->def->stockpile){
					ground_attackers[me->side].push_back(i);
					me->category = CAT_G_ATTACK;
				}
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
						if((((me->def->energyMake - me->def->energyUpkeep) / UnitCost) > 0.002) || me->def->tidalGenerator || me->def->windGenerator){
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
	
	// Now the unit move types are known, sort them out:
	ai->pather->slopeTypes.sort();
	ai->pather->slopeTypes.unique();
	ai->pather->groundUnitWaterLines.sort();
	ai->pather->groundUnitWaterLines.unique();
	ai->pather->waterUnitGroundLines.sort();
	ai->pather->waterUnitGroundLines.unique();
	ai->pather->crushStrengths.sort();
	ai->pather->crushStrengths.unique();
	// Remove from groundUnitWaterLines data that are useless
	for(list<float>::iterator i = ai->pather->groundUnitWaterLines.begin(); i != ai->pather->groundUnitWaterLines.end(); i++)
	{
		if(*i > 255)
		{
			ai->pather->groundUnitWaterLines.erase(i, ai->pather->groundUnitWaterLines.end());
			break;
		}
	}
	
	for(list<float>::iterator i = ai->pather->slopeTypes.begin(); i != ai->pather->slopeTypes.end(); i++)
	{
		L("slopeTypes: " << *i);
	}
	for(list<float>::iterator i = ai->pather->groundUnitWaterLines.begin(); i != ai->pather->groundUnitWaterLines.end(); i++)
	{
		L("groundUnitWaterLines: " << *i);
	}
	for(list<float>::iterator i = ai->pather->waterUnitGroundLines.begin(); i != ai->pather->waterUnitGroundLines.end(); i++)
	{
		L("waterUnitGroundLines: " << *i);
	}
	for(list<float>::iterator i = ai->pather->crushStrengths.begin(); i != ai->pather->crushStrengths.end(); i++)
	{
		L("crushStrengths: " << *i);
	}
	/* The indexes for the pather:
	slopeTypes: have index 0 - (slopeTypesSize -1)
	groundUnitWaterLines: have index slopeTypesSize - (slopeTypesSize + groundUnitWaterLinesSize -1)
	waterUnitGroundLines: same pattern
	crushStrengths: same pattern (but unused)
	*/
	int slopeTypesStart = 0;
	int groundUnitWaterLinesStart = ai->pather->slopeTypes.size();
	int waterUnitGroundLinesStart = groundUnitWaterLinesStart + ai->pather->groundUnitWaterLines.size();
	int crushStrengthsStart = waterUnitGroundLinesStart + ai->pather->crushStrengths.size();
	// Now update the units with global data
	for(int i = 1; i <= numOfUnits; i++){
		UnitType* me = &unittypearray[i];
		//me->Cost = ai->math->GetUnitCost(me->def);
		if(me->side == -1){// filter out neutral units
		
		}	
		else{
			me->moveSlopeType = 0;
			me->moveDepthType = 0;
			if(me->def->movedata)
			{
				int slopeType = slopeTypesStart;
				if(me->def->movedata->moveType != me->def->movedata->Ship_Move)
					for(list<float>::iterator i = ai->pather->slopeTypes.begin(); i != ai->pather->slopeTypes.end(); i++)
					{
						if(*i == me->def->movedata->maxSlope)
						{
							me->moveSlopeType = 1 << slopeType;
							break;
						}
						slopeType++;
					}
				int depthType = slopeTypesStart;
				if(me->def->movedata->moveType == me->def->movedata->Ship_Move)
				{
					// Its a ship/sub unit
					depthType = waterUnitGroundLinesStart;
					me->categoryMask |= CATM_WATER;
					for(list<float>::iterator i = ai->pather->waterUnitGroundLines.begin(); i != ai->pather->waterUnitGroundLines.end(); i++)
					{
						if(*i == me->def->movedata->depth)
						{
							me->moveDepthType = 1 << depthType;
							break;
						}
						depthType++;
					}
				}
				else if(me->def->movedata->moveType == me->def->movedata->Ground_Move)
				{
					 // Its a ground unit, but not a hover
					depthType = groundUnitWaterLinesStart;
					me->categoryMask |= CATM_GROUND;
					for(list<float>::iterator i = ai->pather->groundUnitWaterLines.begin(); i != ai->pather->groundUnitWaterLines.end(); i++)
					{
						if(*i == me->def->movedata->depth)
						{
							me->moveDepthType = 1 << depthType;
							break;
						}
						depthType++;
					}
				}
				else
				{
					// Its a hover
					me->categoryMask |= CATM_GROUND;
					me->categoryMask |= CATM_WATER;
				}
				// Crush strengths now:
				int crushStrength = crushStrengthsStart;
				for(list<float>::iterator i = ai->pather->crushStrengths.begin(); i != ai->pather->crushStrengths.end(); i++)
				{
					if(*i == me->def->movedata->crushStrength)
					{
						me->crushStrength = 1 << crushStrength;
						break;
					}
					crushStrength++;
				}
				L(me->def->humanName << ": " << me->def->movedata->maxSlope << " = " << slopeType << ", " <<  me->def->movedata->depth << " = " << depthType);
				L(me->def->humanName << ": " << me->def->movedata->maxSlope << " = " << me->moveSlopeType << ", " <<  me->def->movedata->depth << " = " << me->moveDepthType);
			}
			else if(me->def->canfly)
			{
				me->moveSlopeType = AIRPATH;
				me->categoryMask |= CATM_AIR;
			}
			
			
		}
	}
	//list<float> move
	//ai->pather->MoveTypes.size()
	
	//L("All units added!");
	DebugPrint();
	char k[200];
	sprintf(k,"UnitTable loaded in %fs",ai->math->TimerSecs());
	ai->cb->SendTextMsg(k,0);
	L(k);
	Init_nr2();
	MakeGlobalNormalizationScore();
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
	bool foundSomething = false;
	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++)
	{
		if (i->def->range > max_range){
			max_range = i->def->range;
			foundSomething = true;
		}
	}
	if (!foundSomething) max_range = 0.0f;
	return max_range;
}

float CUnitTable::GetMinRange(const UnitDef* unit)
{
	float min_range = FLT_MAX;
	bool foundSomething = false;
	for (vector<UnitDef::UnitDefWeapon>::const_iterator i = unit->weapons.begin(); i != unit->weapons.end(); i++)
	{
		if (i->def->range < min_range) {
			min_range = i->def->range;
			foundSomething = true;
		}
	}
	if (!foundSomething) min_range = 0.0f;
	return min_range;
}

/*
Temp/test for the new unit data analyser

*/
void CUnitTable::Init_nr2()
{
	//int timetaken = clock();
	L("Init_nr2 start");
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
				unittypearray[i].categoryMask |= CATM_ESTOR;
			}
			
			if(energyProduction != 0)
			{
				L("Total energy production: " << energyProduction);
				if(energyProduction > 0)
					unittypearray[i].categoryMask |= CATM_ENERGY;
			}
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
				unittypearray[i].categoryMask |= CATM_MEX;
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
				unittypearray[i].categoryMask |= CATM_MSTOR;
			}
			
			if(metalProduction != 0)
			{
				L("Total metal production: " << metalProduction);
				if(metalProduction > 0)
					unittypearray[i].categoryMask |= CATM_MMAKER;
			}
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
				unittypearray[i].categoryMask |= CATM_BUILDER;
				
				// See if its a factory like thing
				if(unittypearray[i].def->buildOptions.size())
				{
					// Its can build 
					L("It can build so many things: " << unittypearray[i].def->buildOptions.size());
					// See if it have construction powers
					assert(unittypearray[i].def->buildSpeed  > 0.0);
					// Nukes are not here
					unittypearray[i].categoryMask |= CATM_FACTORY;
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
				unittypearray[i].categoryMask |= CATM_REPAIRPAD;
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
			if(unittypearray[i].def->losRadius > 500) // TODO: fix this untested hack
				unittypearray[i].categoryMask |= CATM_SCOUT;
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
			unittypearray[i].categoryMask |= CATM_RADAR;
		}
		
		// Do it have a radar jammer ?
		if(unittypearray[i].def->jammerRadius > 0)
		{
			// It have a radar jammer
			L("It have a radar jammer, range: " << unittypearray[i].def->jammerRadius);
			unittypearray[i].categoryMask |= CATM_R_JAMMER;
		}
		
		// Do it have a sonar
		if(unittypearray[i].def->sonarRadius > 0)
		{
			// It have a sonar
			L("It have a sonar, range: " << unittypearray[i].def->sonarRadius);
			unittypearray[i].categoryMask |= CATM_SONAR;
		}
		
		// Do it have a sonar jammer ?
		if(unittypearray[i].def->sonarJamRadius > 0)
		{
			// It have a sonar jammer
			L("It have a sonar jammer, range: " << unittypearray[i].def->sonarJamRadius);
			unittypearray[i].categoryMask |= CATM_S_JAMMER;
		}
		
		// Is it stealthy (not seen on radar) ?
		if(unittypearray[i].def->stealth)
		{
			// Its stealthy
			L("Its stealthy");
			unittypearray[i].categoryMask |= CATM_STEALTH;
		}
		
		// Can it cloak ?
		if(unittypearray[i].def->canCloak)
		{
			// It can cloak
			L("It can cloak");
			// Do it want to start cloaked ?
			unittypearray[i].categoryMask |= CATM_CLOAK;
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
			// TODO: Add this to the category system ??
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
			//assert(unittypearray[i].def->armoredMultiple > 0);
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
			unittypearray[i].categoryMask |= CATM_COMM;
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
			unittypearray[i].categoryMask |= CATM_BUILDING;
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
			
			if(!unittypearray[i].def->canfly)
			{
				if(unittypearray[i].def->minWaterDepth > 0)
				{
					L("unittypearray[i].def->minWaterDepth: " << unittypearray[i].def->minWaterDepth);
					L("Its a water building only");
					unittypearray[i].categoryMask |= CATM_WATER;
				}
				else
				{
					unittypearray[i].categoryMask |= CATM_GROUND;
					L("Its a ground building");
				}
			}
			else
			{
				L("Its a flying building.");
				unittypearray[i].categoryMask |= CATM_AIR;
			}
			
			if(unittypearray[i].def->maxWaterDepth > 0)
			{
				L("unittypearray[i].def->maxWaterDepth: " << unittypearray[i].def->maxWaterDepth);
				if(unittypearray[i].def->maxWaterDepth >= 200) // TODO: Test this
				{
					unittypearray[i].categoryMask |= CATM_WATER;
					L("Its a water building");
				}
				
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
		if(unittypearray[i].def->canKamikaze)
		{
			L("Its a kamikaze unit");
			unittypearray[i].categoryMask |= CATM_KAMIKAZE;
		}
		// TODO: decode the self destruction data
		
		// **************** End of self destruction data mining ***************************
		
		
		// **************** Start of transport data mining ***************************
		if(unittypearray[i].def->transportCapacity > 0) // TODO: this is all untested!!!
		{
			L("Its an transport");
			unittypearray[i].categoryMask |= CATM_TRANSPORT;
		}
		
		
		// **************** End of transport data mining ***************************
		
		
		// **************** Start of movement data mining ***************************
		if(canMove)
		{
			L("It can move");
			
			unittypearray[i].categoryMask |= CATM_CANMOVE;
			assert(unittypearray[i].def->movedata || unittypearray[i].def->canfly);
			// TODO: move the movetype stuff here
			
		}
		// **************** End of movement data mining ***************************
		
		
		// **************** Start of weapon data mining ***************************
		
		if (!unittypearray[i].def->weapons.empty())
		{
			L("It have 'weapons'");
			
			// Look at all the weapons:
			for(int w = 0; w != unittypearray[i].def->weapons.size(); w++){
				
				if(unittypearray[i].def->weapons[w].def->targetable)
				{
					L("It can fire nukes");
					unittypearray[i].categoryMask |= CATM_NUKE;
				}
				if(unittypearray[i].def->weapons[w].def->interceptor)
				{
					L("It can fire anti nukes");
					unittypearray[i].categoryMask |= CATM_ANTINUKE;
				}
				
				if(unittypearray[i].def->weapons[w].def->stockpile)
				{
					// This weapon is stockpiled.
				}
				else
				{
					
					if(unittypearray[i].def->weapons[w].def->isShield)
					{
						// Its a shield system
						L("It have a shield system");
						// TODO: This (might) need to test the range first
						unittypearray[i].categoryMask |= CATM_SHIELD;
					}
					else
					{
						// It have normal weapons
						L("It have a 'normal' weapon");
						
						// TODO: This need to be sorterd:
						if(canMove)
							unittypearray[i].categoryMask |= CATM_ATTACK;
						else
							unittypearray[i].categoryMask |= CATM_DEFENCE;
					}
				}
			}
		}
		
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
			unittypearray[i].categoryMask |= CATM_GEO;
			
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

