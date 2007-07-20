// Universal build .cpp
#include "../Core/helper.h"
CUBuild::CUBuild(){
}

void CUBuild::Init(Global* GL, const UnitDef* u, int unit){
	if(u == 0){
		//
		throw string("INVALID UnitDef");
	}
	G = GL;
	ud = u;
	uid = unit;
	water = false;
	if(u->floater == true){
		water=true;
	}
}

CUBuild::~CUBuild(){
}

bool CUBuild::OkBuildSelection(string name){
	//NLOG("CManufacturer::CBuild  Resource\\MaxEnergy\\");
	float emax=1000000000;
	string key = "Resource\\MaxEnergy\\";
	key += name;
	G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/
	if(G->Pl->GetEnergyIncome() > emax){
		//G->L.print("Factor::CBuild  emax " + name);
		return false;
	}
	//NLOG("CManufacturer::CBuild  Resource\\MinEnergy\\");
	float emin=1;
	key = "Resource\\MinEnergy\\";
	key += name;
	G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/
	if(G->Pl->GetEnergyIncome() < emin){
		//G->L.print("Factor::CBuild  emin " + name);
		return false;
	}
	// Now sort out stuff that can only be built one at a time

	if(G->Cached->solobuild.find(name)!= G->Cached->solobuild.end()){
		if(G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()){
			//NLOG("CManufacturer::CBuild  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
			//G->L.print("Factor::CBuild  solobuild " + name);
			return false;// One is already being built, change to a repair order to go help it!
		}
		deque<CBPlan>* b = G->Manufacturer->BPlans;
		if(b->empty() == false){
			//
			for(deque<CBPlan>::iterator i = b->begin(); i != b->end(); ++i){
				string s = i->ud->name;
				tolowercase(s);
				if(s == name){
					return false;
				}
			}
		}
	}
	// Now sort out if it's one of those things that can only be built once
	if(G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()){
		//NLOG("CManufacturer::CBuild  G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()");
		if(G->Cached->singlebuilds[name] == true){
			//G->L.print("Factor::CBuild  singlebuild " + name);
			return false;
		}
		deque<CBPlan>* b = G->Manufacturer->BPlans;
		if(b->empty() == false){
			//
			for(deque<CBPlan>::iterator i = b->begin(); i != b->end(); ++i){
				string s = i->ud->name;
				tolowercase(s);
				if(s == name){
					return false;
				}
			}
		}
	}

	return true;
}

bool CUBuild::Useless(const UnitDef* udt){
	NLOG("CUBuild::Useless");
	//TODO: Finish CUBuild::Useless()
	if(G->Pl->feasable(udt,ud)==false){
		return false;
	}

	if(udt != 0){
		if(!OkBuildSelection(udt->name)) return false;
		if(water){
			if (udt->minWaterDepth <= 0) {
				//it is a ground unit or building
				if ((udt->floater==false)&&(udt->maxWaterDepth < 30)){
					//unit is ground only
					return true;
				}
			}
		}else{
			if (udt->minWaterDepth > 0) {
				//it is a water-only unit or building
				return true;
			}
		}
		// do stuff to determine if there is no use for this unit
		if(udt->extractRange > 0) return false;
		if(udt->radarRadius > 1) return false;
		if(udt->sonarJamRadius> 1) return false;
		if(udt->sonarRadius > 1) return false;
		if(udt->builder == true) return false;
		if(udt->weapons.empty() == false) return false;
		if(udt->canmove == true) return false;
		if(udt->canfly == true) return false;
		if(udt->canhover == true) return false;
		if(udt->canCapture == true) return false;
		if(udt->canDGun == true) return false;
		if(udt->canKamikaze == true) return false;
		if(udt->canResurrect == true) return false;
		if(udt->energyMake > 10) return false;
		if(udt->type == string("MetalExtractor")) return false;
		if(udt->isMetalMaker == true) return false;
		//if(udt->isFeature == true) return false;
		if(udt->jammerRadius > 10) return false;
		if(udt->metalMake > 0) return false;
		if(udt->needGeo == true) return false;
		if(udt->tidalGenerator > 0) return false;
		if(udt->windGenerator > 0) return false;
		if(udt->transportCapacity > 0) return false;
		return true;
	}
	return false;
}

void CUBuild::SetWater(bool w){
	NLOG("CUBuild::SetWater");
	water =w;
}

string CUBuild::operator() (btype build,float3 pos){// CBuild c; string s = c(btype build);
	NLOG("CUBuild::operator()");
	if(ud->buildOptions.empty() == true) return string("");
	switch(build){
		case B_MEX :{
			return GetMEX();
			break;
		}case B_POWER:{
			return GetPOWER();
			break;
		}case B_RAND_ASSAULT:{
			return GetRAND_ASSAULT();
			break;
		}case B_ASSAULT:{
			return GetASSAULT();
			break;
		}case B_FACTORY:{
			return GetFACTORY();
			break;
		}case B_BUILDER:{
			return GetBUILDER();
			break;
		}case B_GEO:{
			return GetGEO();
			break;
		}case B_SCOUT:{
			return GetSCOUT();
			break;
		}case B_RANDOM:{
			return GetRANDOM();
			break;
		}case B_DEFENCE:{
			return GetDEFENCE();
            break;
		}case B_RADAR:{
			return GetRADAR();
			break;
		}case B_ESTORE:{
			return GetESTORE();
			break;
		}case B_MSTORE:{
			return GetMSTORE();
			break;
		}case B_SILO:{
			return GetSILO();
			break;
		}case B_JAMMER:{
			return GetJAMMER();
			break;
		}case B_SONAR:{
			return GetSONAR();
			break;
		}case B_ANTIMISSILE:{
			return GetANTIMISSILE();
			break;
		}case B_ARTILLERY:{
			return GetARTILLERY();
			break;
		}case B_FOCAL_MINE:{
			return GetFOCAL_MINE();
			break;
		}case B_SUB : {
			return GetSUB();
			break;
		}case B_AMPHIB: {
			return GetAMPHIB();
			break;
		}case B_MINE:{
			return GetMINE();
			break;
		}case B_CARRIER:{
			return GetCARRIER();
			break;
		}case B_METAL_MAKER:{
			return GetMETAL_MAKER();
			break;
		}case B_FORTIFICATION:{
			return GetFORTIFICATION();
			break;
		}case B_BOMBER:{
			return GetBOMBER();
			break;
		}case B_FIGHTER:{
			return GetFIGHTER();
			break;
		}case B_GUNSHIP:{
			return GetGUNSHIP();
			break;
		}case B_SHIELD:{
			return GetSHIELD();
			break;
		}case B_MISSILE_UNIT:{
			return GetMISSILE_UNIT();
			break;
		}case B_HUB:{
			return GetHUB();
			break;
		}case B_AIRSUPPORT:{
			return GetAIRSUPPORT();
			break;
		}default:{
			return string("nought");
			break;
		}
	}
	return string("nought");
}

string CUBuild::GetMEX(){
	NLOG("CUBuild::GetMEX");
	// Find all metal extractors this unit can build and add them to a list.
	float highscore= 0;
	string highest="";
	for(map<int,string>::const_iterator is = ud->buildOptions.begin(); is != ud->buildOptions.end();++is){
		const UnitDef* pd = G->GetUnitDef(is->second);
		if(pd == 0) continue;
		if(G->UnitDefHelper->IsMex(pd)){
			if(Useless(pd)==true) continue;
			float score = (pd->extractsMetal+1)*1000;
			if(pd->weapons.empty()==false){
				score *= 1.2f;
			}
			if(pd->canCloak==true){
				score *= 1.2f;
			}
			score /= (pd->energyCost+(pd->metalCost*45));
			if(score > highscore){
				highscore = score;
				highest = pd->name;
			}
			continue;
		}
		if((pd->metalMake > 5)&&(pd->builder == false)&&(pd->movedata == 0 )&&(pd->canfly == false)){
			float score = pd->metalMake;
			if(pd->weapons.empty()==false){
				score *= 1.2f;
			}
			if(pd->canCloak==true){
				score *= 1.2f;
			}
			score /= (pd->energyCost+(pd->metalCost*45));
			if(score > highscore){
				highscore = score;
				highest = pd->name;
			}
			continue;
		}
	}
	return highest;
}

string CUBuild::GetPOWER(){
	NLOG("CUBuild::GetPOWER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	// Find all energy generators this unit can build and add them to a list.
	list<const UnitDef*> possibles_u;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->UnitDefHelper->IsEnergy(pd)) possibles_u.push_back(pd);
		}
	}
	string best_energy = "";
	if(possibles_u.empty() == false){
		float bestpoint = 0;
		for(list<const UnitDef*>::iterator pd = possibles_u.begin();pd != possibles_u.end(); ++pd){
			float temp_energy = 0;
			if((*pd)->energyUpkeep <0) temp_energy += -(*pd)->energyUpkeep;
			if((*pd)->windGenerator>1) temp_energy += (G->cb->GetMaxWind()+G->cb->GetMinWind())/2;
			if((*pd)->energyMake > 1) temp_energy += (*pd)->energyMake;
			if((*pd)->tidalGenerator >0) temp_energy += G->cb->GetTidalStrength();
			temp_energy /= ((*pd)->energyCost+((*pd)->metalCost*45));
			if(temp_energy/(((*pd)->metalCost+(*pd)->energyCost)/2) > bestpoint){
				bestpoint = temp_energy;
				best_energy = (*pd)->name;
			}
		}
	}
	return best_energy;
	return string("");
}

string CUBuild::GetRAND_ASSAULT(){
	NLOG("CUBuild::GetRAND_ASSAULT");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int defnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd,ud)==false) continue;
			if(G->UnitDefHelper->IsAttacker(pd)){
				possibles.push_back(pd->name);
				defnum++;
			}
		}
	}
	if(possibles.empty() == false){
		defnum = G->mrand()%defnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == defnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}else{
		return string("");
	}
}

string CUBuild::GetASSAULT(){
	NLOG("CUBuild::GetASSAULT");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd,ud)==false) continue;
			if(G->UnitDefHelper->IsAttacker(pd)){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp -= (G->mrand()%max(int(temp/4),1));
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	if(best_score < 30){
		return GetRAND_ASSAULT();
	}
	return best;
}


string CUBuild::GetBOMBER(){
	NLOG("CUBuild::GetBOMBER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd,ud)==false) continue;
			if(G->UnitDefHelper->IsBomber(pd)){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp = temp - G->mrand()%max(int(temp/4),1);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetFIGHTER(){
	NLOG("CUBuild::GetFIGHTER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd,ud)==false) continue;
			if(G->UnitDefHelper->IsFighter(pd)){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp = temp - G->mrand()%max(int(temp/4),1);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetGUNSHIP(){
	NLOG("CUBuild::GetGUNSHIP");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd,ud)==false) continue;
			if(G->UnitDefHelper->IsGunship(pd)){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp = temp - G->mrand()%max(int(temp/4),1);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetMISSILE_UNIT(){
	NLOG("CUBuild::GetMISSILE_UNIT");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->metalCost+pd->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("2.1", "AI\\cheap_multiplier").c_str())) continue;
			bool good = true;
			if(pd->canfly ==false) good = false;
			if(pd->weapons.empty() == true) good = false;
			if(pd->builder == true) good = false;
			if(pd->transportCapacity > 0) good = false;
			bool found = false;
			for(vector<UnitDef::UnitDefWeapon>::const_iterator i = pd->weapons.begin(); i != pd->weapons.end(); ++i){
				if(i->def->interceptor > 0){
					continue;
				}
				if(i->def->vlaunch == true){
					found = true;
					break;
				}
			}
			if(found == false) good = false;
			if(good == true){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp = temp - G->mrand()%min((int)(temp/3),1);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetSHIELD(){
	NLOG("CUBuild::GetSHIELD");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((G->Pl->feasable(pd->name,uid)==true)&&(G->info->antistall>0)) continue;
			if(pd->metalCost+pd->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("2.1", "AI\\cheap_multiplier").c_str())) continue;

			bool found = false;
			if(pd->weapons.empty() == false){
				for(vector<UnitDef::UnitDefWeapon>::const_iterator i = pd->weapons.begin(); i != pd->weapons.end(); ++i){
					if(i->def->isShield == true){
						found = true;
						break;
					}
				}
			}
			if(found == true){
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				temp = temp - G->mrand()%max(int(temp/4),1);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetFACTORY(){
	NLOG("CUBuild::GetFACTORY");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->UnitDefHelper->IsFactory(pd)==true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(G->UnitDefHelper->IsUWStructure(pd)!=(water||G->info->spacemod)) continue;
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				int r = G->mrand()%max(int(temp/4),1)+1;
				temp += (float)abs(r);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetBUILDER(){
	NLOG("CUBuild::GetBUILDER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->builder == true)&&((pd->movedata !=0)||(pd->canfly == true))){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						float temp = G->GetEfficiency(pd->name);
						if(!pd->buildOptions.empty()){
							for(map<int,std::string>::const_iterator i = pd->buildOptions.begin(); i != pd->buildOptions.end(); ++i){
								temp += G->GetEfficiency(i->second);
							}
						}
						temp*=pd->buildSpeed;
						temp /= (pd->energyCost+(pd->metalCost*45));
						temp = temp - G->mrand()%max(int(temp/4),1);
						if(temp > best_score){
							best_score = temp;
							best = pd->name;
						}
					}
				}
			}
		}
	}
	return best;
}

string CUBuild::GetGEO(){
	NLOG("CUBuild::GetGEO");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->Pl->feasable(pd->name,uid)==false) continue;
			if((pd->needGeo == true)&&(pd->builder == false)) possibles.push_back(pd->name);
		}
	}
	if(possibles.empty() == false) return possibles.front();
	return string("");
}

string CUBuild::GetSCOUT(){
	NLOG("CUBuild::GetSCOUT");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* ud = G->GetUnitDef(is->name);
			if(ud == 0) continue;
			//if(Useless(ud)) continue;
			if((ud->speed > G->info->scout_speed)&&(ud->movedata != 0)&&(ud->canfly == false)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false))  possibles.push_back(ud->name);
			if((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false))  possibles.push_back(ud->name);
			if((ud->weapons.empty()==true)&&(ud->canResurrect == false)&&(ud->canCapture == false)&&(ud->builder == false)&& ((ud->canfly==true)||(ud->movedata != 0)) &&((ud->radarRadius > 10)||(ud->sonarRadius > 10)))  possibles.push_back(ud->name);
		}
	}
	if(possibles.empty() == false) return possibles.front();
	return string("");
}

string CUBuild::GetRANDOM(){
	NLOG("CUBuild::GetRANDOM");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->canKamikaze)&&((pd->canfly == false)&&(pd->movedata == 0))) continue; // IT'S A MINE!!!! We dont want nonblocking mines built ontop of eachother next to the factory!!!!!!!!!!!
			if((pd->radarRadius > 10)&&((pd->builder == false)||(pd->isAirBase == true))) continue; // no radar towers!!!!
			if((pd->energyStorage > 100)||(pd->metalStorage > 100)) continue; // no resource storage!!!!!!!!
			if(pd->isMetalMaker == true) continue; // no metal makers
			if(Useless(pd)==true) continue;
			if((pd->metalCost+pd->energyCost < (G->cb->GetEnergy()+G->cb->GetMetal())*0.9)&&(((pd->floater == false)&&(water == false)&&(G->info->spacemod == false))||((G->info->spacemod == true)||(water == true)))){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				possibles.push_back(pd->name);
				randnum++;
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetDEFENCE(){
	NLOG("CUBuild::GetDEFENCE");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int defnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(Useless(pd)) continue;
			if((pd->weapons.empty() == false)&&(!pd->isFeature)&&(!G->UnitDefHelper->IsMobile(pd))){
				if((G->info->spacemod == true)||(water == true)){
					if(G->Pl->feasable(pd->name,uid)==false) continue;
					possibles.push_back(pd->name);
					defnum++;
				} else if (pd->floater == false){
					if(G->Pl->feasable(pd->name,uid)==false) continue;
					possibles.push_back(pd->name);
					defnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		defnum = G->mrand()%defnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == defnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetRADAR(){
	NLOG("CUBuild::GetRADAR");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(Useless(pd)) continue;
			if((pd->radarRadius >10)&&(pd->floater == false)&&(pd->builder == false)){
				possibles.push_back(pd->name);
				randnum++;
			}else if((pd->radarRadius >10)&&(pd->floater == false)&&(pd->builder == false)&&(pd->floater == true)&&((G->info->spacemod == true)||(water == true))){
				possibles.push_back(pd->name);
				randnum++;
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetESTORE(){
	NLOG("CUBuild::GetESTORE");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->energyStorage >100){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetMSTORE(){
	NLOG("CUBuild::GetMSTORE");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");

	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->metalStorage >100){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}
string CUBuild::GetSILO(){
	NLOG("CUBuild::GetSILO");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->weapons.empty() != true){
				for(vector<UnitDef::UnitDefWeapon>::const_iterator i = pd->weapons.begin(); i != pd->weapons.end(); ++i){
					if((i->def->stockpile == true)&&(i->def->targetable > 0)){
						if(G->Pl->feasable(pd->name,uid)==false) continue;
						possibles.push_back(pd->name);
						randnum++;
						break;
					}
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetJAMMER(){
	NLOG("CUBuild::GetJAMMER");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");

	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->jammerRadius >10)&&(pd->floater == false)&&(pd->builder == false)){
				possibles.push_back(pd->name);
				randnum++;
			}else if((pd->jammerRadius >10)&&(pd->floater == false)&&(pd->builder == false)&&(pd->floater == true)&&((G->info->spacemod == true)||(water == true))){
				possibles.push_back(pd->name);
				randnum++;
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetSONAR(){
	NLOG("CUBuild::GetSONAR");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");

	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->sonarRadius >10)&&(pd->floater == false)&&(pd->builder == false)){
				possibles.push_back(pd->name);
				randnum++;
			}else if((pd->sonarRadius >10)&&(pd->floater == false)&&(pd->builder == false)&&(pd->floater == true)&&((G->info->spacemod == true)||(water == true))){
				possibles.push_back(pd->name);
				randnum++;
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetANTIMISSILE(){
	NLOG("CUBuild::GetANTIMISSILE");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->weapons.empty() != true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				for(vector<UnitDef::UnitDefWeapon>::const_iterator i = pd->weapons.begin(); i != pd->weapons.end(); ++i){
					//
					if(i->def->interceptor == 1){
						possibles.push_back(pd->name);
						randnum++;
						break;
					}
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetARTILLERY(){
	NLOG("CUBuild::GetARTILLERY");
	return string("");
	//list<string> possibles;
	//const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	//if(di == 0) return string("");
}

string CUBuild::GetTARG(){
	NLOG("CUBuild::GetTARG");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->targfac == true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}
string CUBuild::GetFOCAL_MINE(){
	NLOG("CUBuild::GetFOCAL_MINE");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->canKamikaze==true)&&((pd->canfly == false)&&(pd->movedata == 0))){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetSUB(){
	NLOG("CUBuild::GetSUB");
	return string("");
// 	water=true;
// 	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
// 	if(di == 0) return string("");
// 	list<string> possibles;

}

string CUBuild::GetAMPHIB(){
	NLOG("CUBuild::GetAMPHIB");
	return string("");
// 	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
// 	if(di == 0) return string("");
// 	list<string> possibles;

}

string CUBuild::GetMINE(){
	NLOG("CUBuild::GetMINE");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->canKamikaze)&&((pd->canfly == false)&&(pd->movedata == 0))){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetCARRIER(){
	NLOG("CUBuild::GetCARRIER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	int randnum = 0;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(pd->isAirBase == true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(pd->floater == true){
					if((G->info->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						randnum++;
					}
				} else{
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
	}
	if(possibles.empty() == false){
		randnum = G->mrand()%randnum;
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetMETAL_MAKER(){
	NLOG("CUBuild::GetMETAL_MAKER");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if((pd->isMetalMaker == true)&&(pd->floater == false)){
				possibles.push_back(pd->name);
			}
			if((pd->isMetalMaker == true)&&(pd->floater == true)&&((G->info->spacemod == true)||(water == true))){
				possibles.push_back(pd->name);
			}
		}
	}
	if(possibles.empty() == false){
		int randnum = G->mrand()%possibles.size();
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(0 == randnum){
				return *k;
			}else{
				randnum--;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetFORTIFICATION(){
	NLOG("CUBuild::GetFORTIFICATION");
	if(G->DTHandler->DTNeeded() == false) return string("");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->DTHandler->IsDragonsTeeth(pd)) possibles.push_back(pd->name);
		}
	}
	if(possibles.empty() == false){
		int randnum = G->mrand()%possibles.size();
		int j = 0;
		for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
			if(j == randnum){
				return *k;
			}else{
				j++;
			}
		}
		return possibles.front();
	}
	return string("");
}

string CUBuild::GetHUB(){
	NLOG("CUBuild::GetHUB");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->UnitDefHelper->IsHub(pd)==true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(G->UnitDefHelper->IsUWStructure(pd)!=(water||G->info->spacemod)) continue;
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				int r = G->mrand()%max(int(temp/4),1)+1;
				temp += (float)abs(r);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

string CUBuild::GetAIRSUPPORT(){
	NLOG("CUBuild::GetAIRSUPPORT");
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("");
	list<string> possibles;
	float best_score = 0;
	string best = "";
	for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
		if(is->id<0){
			const UnitDef* pd = G->GetUnitDef(is->name);
			if(pd == 0) continue;
			if(G->UnitDefHelper->IsHub(pd)==true){
				if(G->Pl->feasable(pd->name,uid)==false) continue;
				if(G->UnitDefHelper->IsUWStructure(pd)!=(water||G->info->spacemod)) continue;
				float temp = G->GetEfficiency(pd->name);
				temp /= (pd->energyCost+(pd->metalCost*45));
				int r = G->mrand()%max(int(temp/4),1)+1;
				temp += (float)abs(r);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
	}
	return best;
}

