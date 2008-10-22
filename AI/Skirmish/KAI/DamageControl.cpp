#include "DamageControl.h"

#include "Game/GlobalConstants.h"

CDamageControl::CDamageControl(AIClasses* ai)
{
	this->ai=ai;
	NumOfUnitTypes = ai->cb->GetNumUnitDefs();
	MyUnitsAll.resize(NumOfUnitTypes+1);
	MyDefences.resize(NumOfUnitTypes+1);
	MyArmy.resize(NumOfUnitTypes+1);
	TheirUnitsAll.resize(NumOfUnitTypes+1);
	TheirDefences.resize(NumOfUnitTypes+1);
	TheirArmy.resize(NumOfUnitTypes+1);
	TheirTotalCost = 1;
}

CDamageControl::~CDamageControl()
{

}

void CDamageControl::GenerateDPSTables()
{	
	L("Generating DPS tables....");
	//L("");
	for(int i= 1; i <= NumOfUnitTypes; i++){
		//L("*****************************UNIT: " << ai->ut->unittypearray[i].def->humanName);
		ai->ut->unittypearray[i].DPSvsUnit.resize(NumOfUnitTypes+1);
		ai->ut->unittypearray[i].AverageDPS = 0;
		for(int v = 1; v <= NumOfUnitTypes; v++){			
			float dps = GetDPSvsUnit(ai->ut->unittypearray[i].def,ai->ut->unittypearray[v].def);
			ai->ut->unittypearray[i].DPSvsUnit[v] = dps;
			ai->ut->unittypearray[i].AverageDPS += dps;
			//L("DPS vs " << ai->ut->unittypearray[v].def->humanName << ": " << dps);
		}
		ai->ut->unittypearray[i].AverageDPS /= NumOfUnitTypes;
		//L("AverageDPS: " << ai->ut->unittypearray[i].AverageDPS);
	}
}

void CDamageControl::UpdateTheirDistribution()
{
	//L("Updating Their Distribution");
	TheirTotalCost = 0.0001; // Safety if they have 0 units.
	TheirTotalArmyCost = 0;
	//ai->math->TimerStart();
	const int* enemiesCount = ai->sh->GetNumOfEnemiesUnitsOfType();
	for(int i= 1; i <= NumOfUnitTypes; i++){
		TheirUnitsAll[i].Clear();
		TheirDefences[i].Clear();
		TheirArmy[i].Clear();
	}
	UnitType* unittypearray = ai->ut->unittypearray;
	for(int i = 1; i <= NumOfUnitTypes; i++){
		if(enemiesCount[i] > 0)
		{
			//int id = ai->cheat->GetUnitDef(enemies[i])->id;
			UnitType* unitType = &unittypearray[i];			
			float enemycost = unitType->Cost * enemiesCount[i];
			TheirTotalCost += enemycost;
			if(unitType->def->isCommander)
				enemycost = 100; // Ignore the commander (hack mostly)
			TheirUnitsAll[i].Cost = enemycost;			
			TheirUnitsAll[i].Hitpoints = unitType->def->health * enemiesCount[i];
			if(ai->ut->GetCategory(unitType->def) == CAT_DEFENCE){
				TheirDefences[i].Cost = enemycost;
				TheirDefences[i].Hitpoints = unitType->def->health;
				TheirTotalArmyCost += enemycost;

			}
			else if(ai->ut->GetCategory(unitType->def) == CAT_ATTACK ||
				ai->ut->GetCategory(unitType->def) == CAT_ARTILLERY ||
				ai->ut->GetCategory(unitType->def) == CAT_ASSAULT) {
				TheirArmy[i].Cost = enemycost;
				TheirArmy[i].Hitpoints = unitType->def->health;
				TheirTotalArmyCost += enemycost;
			}

			if(unitType->AverageDPS > 0 && !unittypearray[i].def->isCommander)
			{
				for(int e = 1; e <= NumOfUnitTypes; e++){
					float totaldps = unitType->DPSvsUnit[e] * ai->uh->AllUnitsByType[i].size();
					TheirUnitsAll[e].Damage += totaldps;
					if(ai->ut->GetCategory(unitType->def) == CAT_DEFENCE){
						TheirDefences[e].Damage += totaldps;
					}
					else if(ai->ut->GetCategory(unitType->def) == CAT_ATTACK ||
						ai->ut->GetCategory(unitType->def) == CAT_ARTILLERY ||
						ai->ut->GetCategory(unitType->def) == CAT_ASSAULT){
						TheirArmy[e].Damage += totaldps;
					}
				}
			}
		}
	}
	//L("Time of UpdateTheirDistribution: " << ai->math->TimerSecs());
}
void CDamageControl::UpdateMyDistribution()
{
	//L("UpdateMyDistribution");
	MyTotalCost = 0.0001; // Safety if I have 0 units.	
	//ai->math->TimerStart();
	for(int i= 1; i <= NumOfUnitTypes; i++){
		MyUnitsAll[i].Clear();
		MyDefences[i].Clear();
		MyArmy[i].Clear();
	}
	UnitType* unittypearray = ai->ut->unittypearray;
	//L("Starting first loop");
	for(int i = 1; i <= NumOfUnitTypes; i++){
		if(ai->uh->AllUnitsByType[i].size())
		{
			//L("Unit ID: " << i);
			//int id = ai->cheat->GetUnitDef(enemies[i])->id;
			UnitType * unitType = &unittypearray[i];
			float myunitcost = unitType->Cost *  ai->uh->AllUnitsByType[i].size();
			MyUnitsAll[i].Hitpoints = unitType->def->health * ai->uh->AllUnitsByType[i].size();
			MyUnitsAll[i].Cost = myunitcost;
			MyTotalCost += myunitcost;
			//L("Total Costs defined");
			if(ai->ut->GetCategory(unitType->def) == CAT_DEFENCE){
				MyDefences[i].Hitpoints = unitType->def->health;
				MyDefences[i].Cost = myunitcost;
			}
			else if(ai->ut->GetCategory(unitType->def) == CAT_ATTACK ||
				ai->ut->GetCategory(unitType->def) == CAT_ARTILLERY ||
				ai->ut->GetCategory(unitType->def) == CAT_ASSAULT){
				MyArmy[i].Hitpoints = unitType->def->health;
				MyArmy[i].Cost = myunitcost;
			}
			//L("Starting Damage procedure");
			if(unitType->AverageDPS > 0 && !unittypearray[i].def->isCommander)
			{
				for(int e = 1; e <= NumOfUnitTypes; e++){
					float totaldps = unitType->DPSvsUnit[e] * ai->uh->AllUnitsByType[i].size();
					MyUnitsAll[e].Damage += totaldps;
					if(ai->ut->GetCategory(unitType->def) == CAT_DEFENCE){
						MyDefences[e].Damage += totaldps;
					}
					else if(ai->ut->GetCategory(unitType->def) == CAT_ATTACK ||
						ai->ut->GetCategory(unitType->def) == CAT_ARTILLERY ||
						ai->ut->GetCategory(unitType->def) == CAT_ASSAULT){
						MyArmy[e].Damage += totaldps;
					}
				}
			}
		}
	}
	//L("My cost: " << MyTotalCost);
	//L("Time of UpdateMyDistribution: " << ai->math->TimerSecs());
}

float CDamageControl::GetCurrentDamageScore(const UnitDef* unit)
{
	//L("Getting CombatScore for : " << unit->humanName << " number of units: " << NumOfUnitTypes);
	float score = 0;
	int category = GCAT(unit); 
	bool foundaunit = false;
	if(category == CAT_ATTACK || category == CAT_ARTILLERY || category == CAT_ASSAULT){
		for(int i = 1; i <= NumOfUnitTypes; i++){
			if(TheirUnitsAll[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0 && ai->ut->unittypearray[i].AverageDPS > 0){
				//L("unit->id: " << unit->id << " I: " << i);
				//L("DPSvsUnit[i] " << ai->ut->unittypearray[unit->id].DPSvsUnit[i] << " TheirUnitsAll[i].Cost " << TheirUnitsAll[i].Cost);
				score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirUnitsAll[i].Cost / TheirUnitsAll[i].Hitpoints / (MyArmy[i].Damage + 1); // kill the stuff with the best value
				//L("Score:" << score);
				//score+= 0; // This was for making a debug brakepoint
				foundaunit = true;
			}
		}
		if(!foundaunit){ // if nothing found try to kill his movable units (comm and builders)
			for(int i = 1; i <= NumOfUnitTypes; i++){
				if(TheirUnitsAll[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0 && ai->ut->unittypearray[i].def->speed > 0){
					score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirUnitsAll[i].Cost / TheirUnitsAll[i].Hitpoints / (MyArmy[i].Damage + 1); // kill the stuff with the best value
					foundaunit = true;
				}
			}
		}
		if(!foundaunit){ // if all fails just get whatever hes got and try to hit it
			for(int i = 1; i <= NumOfUnitTypes; i++){
				if(TheirUnitsAll[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0){
					score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirUnitsAll[i].Cost / TheirUnitsAll[i].Hitpoints / (MyArmy[i].Damage + 1); // kill the stuff with the best value
				}
			}
		}
	}
	else if(category == CAT_DEFENCE){
		for(int i = 1; i <= NumOfUnitTypes; i++){
			if(TheirArmy[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0 && (ai->ut->unittypearray[i].category == CAT_ATTACK || ai->ut->unittypearray[i].category == CAT_ARTILLERY || ai->ut->unittypearray[i].category == CAT_ASSAULT)){
				//L("unit->id: " << unit->id << " I: " << i);
				//L("DPSvsUnit[i] " << ai->ut->unittypearray[unit->id].DPSvsUnit[i] << " TheirUnitsAll[i].Cost " << TheirUnitsAll[i].Cost);
				score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirArmy[i].Cost / TheirArmy[i].Hitpoints / (MyDefences[i].Damage + 1); // kill the stuff with the best value
				//L("Score:" << score);
				//score+= 0; // This was for making a debug brakepoint
				foundaunit = true;				
			}
		}
		if(!foundaunit){ // if nothing found try to kill his movable units (comm and builders)
			for(int i = 1; i <= NumOfUnitTypes; i++){
				if(TheirUnitsAll[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0 && ai->ut->unittypearray[i].def->speed > 0){
					score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirUnitsAll[i].Cost / TheirUnitsAll[i].Hitpoints / (MyArmy[i].Damage + 1); // kill the stuff with the best value
					foundaunit = true;
				}
			}
		}
		if(!foundaunit){ // if all fails just get whatever hes got and try to hit it
			for(int i = 1; i <= NumOfUnitTypes; i++){
				if(TheirUnitsAll[i].Cost > 0 && ai->ut->unittypearray[unit->id].DPSvsUnit[i] > 0){
					score+= ai->ut->unittypearray[unit->id].DPSvsUnit[i] * TheirUnitsAll[i].Cost / TheirUnitsAll[i].Hitpoints / (MyArmy[i].Damage + 1); // kill the stuff with the best value
				}
			}
		}
	}

	//L(" score " << score << " TheirTotalCost " << TheirTotalCost);
	if (TheirTotalArmyCost <= 0)
		return 1;
	if (category == CAT_ATTACK || category == CAT_ARTILLERY || category == CAT_ASSAULT) {
		return score / TheirTotalArmyCost / (TheirUnitsAll[unit->id].Damage + 1);
	}

	else if (category == CAT_DEFENCE) {
		return score / TheirTotalArmyCost / (TheirArmy[unit->id].Damage + 1);
	}
	L("***** Getdamagescore Error! *****");
	return 1;
}

float CDamageControl::GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim)
{
	if(unit->weapons.size() && ai->ut->unittypearray[unit->id].side != -1){
		ai->math->TimerStart();
		//L("");
		//if(unit->weapons.size() > 0)
			//L("Getting DPS for Unit: " << unit->humanName << " Against " << victim->humanName);
		float dps = 0;
		bool canhit = false;
		bool underwater = false;
		//string targetcat;	
		int armortype = victim->armorType;
		int numberofdamages;
		ai->cb->GetValue(AIVAL_NUMDAMAGETYPES,&numberofdamages);
		if (ai->cb->GetUnitDefHeight(victim->id) - victim->waterline < 0) {
			underwater = true;
		}

		//L("Starting weapon loop");
		for (unsigned int i = 0; i != unit->weapons.size(); i++) {
			if (!unit->weapons[i].def->paralyzer) {
				//L(ai->ut->unittypearray[unit->id].TargetCategories[i]);
				//targetcat = ai->ut->unittypearray[unit->id].TargetCategories[i];
				//L(unit->weapons[i].name << " Targcat: " <<  targetcat);
				//L("unit->categoryString " <<  unit->categoryString);
				//L("victim->categoryString " <<  victim->categoryString);
				
				unsigned int a = victim->category;
				unsigned int b = unit->weapons[i].def->onlyTargetCategory; // This is what the weapon can target
				unsigned int c = unit->weapons[i].onlyTargetCat; // This is what the unit accepts as this weapons target
//				unsigned int d = unit->weapons[i].badTargetCat; // This is what the unit thinks this weapon must be used for (hmm ?)
				bool canWeaponTarget = (a & b) > 0;
				bool canUnitTarget = (a & c) > 0; // How is this used in spring, needs testing...
//				bool badUnitTarget = (a & d) > 0; // This probably means that it have low priority, ignore it for now

				canhit = (canWeaponTarget && canUnitTarget);

				if (!unit->weapons[i].def->waterweapon && underwater) {
					canhit = false;
					//L("This weapon cannot hit this sub");
				}
				if (unit->weapons[i].def->waterweapon && victim->minWaterDepth <= 0) {
						canhit = false;
				}


				// Bombers are useless against more stuff:
				if(unit->weapons[i].def->dropped){
					if(victim->canfly && unit->wantedHeight <= victim->wantedHeight)
						canhit = false; // If the target can fly and it flyes below the victim its useless
				}
				
				if(canhit){
					//L(unit->weapons[i].name << " a: " <<  a << ", b: " << b << ", c: " << c << ", d: " << d);
					//L("unit->categoryString " <<  unit->categoryString);
					//L("victim->categoryString " <<  victim->categoryString);
				
				}
				//unit->weapons[i].def->turnrate
				if(canhit){
					float accuracy = unit->weapons[i].def->accuracy *2.8;
					if(victim->speed > 0){ // Better not use !=  as floats can have many forms of 0
						accuracy *= 1-(unit->weapons[i].def->targetMoveError);
					}
					float basedamage = unit->weapons[i].def->damages[armortype] * unit->weapons[i].def->salvosize * unit->weapons[i].def->projectilespershot / unit->weapons[i].def->reload;
					float AOE = unit->weapons[i].def->areaOfEffect * 0.7;
					float tohitprobability;
					float impactarea;
					float targetarea;
					float distancetravelled = unit->weapons[i].def->range;
					//L("Initial DT: " << distancetravelled);
					float firingangle;
					float gravity = -(ai->cb->GetGravity()*900);
					float timetoarrive;
					float u = unit->weapons[i].def->projectilespeed * 30;
					if(u == 0) // needed... for ARMGATE
						u = 0.00000001;
					//L("Type: " << unit->weapons[i].def->type);
					if(unit->weapons[i].def->type == string("Cannon")){ // Do ballistic calculations to account for increased flight time
						//L("Is ballistic! Gravity: " << gravity);
						//L("u = " << u << " distancetravelled*gravity)/(u*u) = " << (distancetravelled*gravity)/(u*u));
						float sinoid = (distancetravelled*gravity)/(u*u);
						sinoid = min(sinoid,1.0f);
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
					//L("Guidance: " << unit->weapons[i].def->guided << " Turning: " << unit->weapons[i].def->turnrate);
				//FIXME turnrate can be set for weapons that don't support it (only StarburstLauncher and MissileLauncher support it)!
					if((unit->weapons[i].def->turnrate == 0.0f  ||  unit->weapons[i].def->type == string("StarburstLauncher")) && (unit->weapons[i].def->projectilespeed > 0 || unit->weapons[i].def->dropped) && victim->speed != 0 && unit->weapons[i].def->beamtime == 1){
						if(unit->weapons[i].def->type == string("Cannon")){
							timetoarrive = (2*u*sin(firingangle))/gravity;
						}
						else{
							if(!unit->weapons[i].def->dropped){
								timetoarrive = distancetravelled / (unit->weapons[i].def->projectilespeed * 30);
								if(unit->weapons[i].def->type == string("StarburstLauncher")){
									timetoarrive += unit->weapons[i].def->uptime;
								}
							}
							else{
								timetoarrive = distancetravelled / (unit->speed);
							}
						}
						float shotwindow = sqrt(targetarea) / victim->speed;
						if(shotwindow < timetoarrive){
							tohitprobability *= pow(shotwindow / timetoarrive,1.5f);
						}
						//L("Not guided. TTA: " <<  timetoarrive << " shotwindow " << shotwindow << " unit->weapons[i].def->targetmoveerror " << unit->weapons[i].def->targetMoveError);
					}
					//L("chance to hit: " << tohitprobability);
					//L("Dps for this weapon: " << basedamage * tohitprobability);
					assert(tohitprobability <= 1);
					assert(tohitprobability >= 0);
					dps += basedamage * tohitprobability;
				}
			}	
		}
		//if(unit->weapons.size() > 0)
		//L("DPS: " << dps);
		//L("Time taken: " << ai->math->TimerSecs() << "s.");
		return dps;
	}
	return 0;

}
