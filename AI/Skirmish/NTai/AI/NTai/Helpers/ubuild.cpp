// Universal build .cpp
#include "../Core/include.h"

namespace ntai {
	CUBuild::CUBuild(){
	}

	void CUBuild::Init(Global* GL, CUnitTypeData* wu, int uid){
		G = GL;
		utd = wu;
		water = utd->GetUnitDef()->floater;
		this->uid = uid;
	}

	CUBuild::~CUBuild(){
	}

	bool CUBuild::OkBuildSelection(string name){

		CUnitTypeData* u =G->UnitDefLoader->GetUnitTypeDataByName(name);


		float emax=1000000000;
		string key = "Resource\\MaxEnergy\\";
		key += u->GetName();
		G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/
		if(G->Pl->GetEnergyIncome() > emax){
			//G->L.print("Factor::CBuild  emax " + name);
			return false;
		}
		//NLOG("CManufacturer::CBuild  Resource\\MinEnergy\\");
		float emin=1;
		key = "Resource\\MinEnergy\\";
		key += u->GetName();
		G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/
		if(G->Pl->GetEnergyIncome() < emin){
			//G->L.print("Factor::CBuild  emin " + name);
			return false;
		}
		// Now sort out stuff that can only be built one at a time

		if(u->GetSoloBuild()){
			if(u->GetSoloBuildActive()){
				// One is already being built! We're not supposed to build more than one at any one time.
				return false;
			}
			deque<CBPlan* >* b = G->Manufacturer->BPlans;
			if(b->empty() == false){
				//
				for(deque<CBPlan* >::iterator i = b->begin(); i != b->end(); ++i){
					string s = (*i)->utd->GetName();
					if(s == u->GetName()){
						return false;
					}
				}
			}
		}

		// Now sort out if it's one of those things that can only be built once
		if(u->GetSingleBuild()){

			if(u->GetSingleBuildActive()){
				return false;
			}

			deque<CBPlan* >* b = G->Manufacturer->BPlans;
			if(b->empty() == false){
				//
				for(deque<CBPlan* >::iterator i = b->begin(); i != b->end(); ++i){
					string s = (*i)->utd->GetName();
					if(s == name){
						return false;
					}
				}
			}
		}

		return true;
	}

	bool CUBuild::Useless(CUnitTypeData* u){
		NLOG("CUBuild::Useless");


		//TODO: Finish CUBuild::Useless()
		if(G->Pl->feasable(utd,u)==false){
			return false;
		}

		if(u != 0){
			if(!OkBuildSelection(u->GetName())) return false;
			if(water){
				if (u->GetUnitDef()->minWaterDepth <= 0) {
					//it is a ground unit or building
					if ((u->GetUnitDef()->floater==false)&&(u->GetUnitDef()->maxWaterDepth < 30)){
						//unit is ground only
						return true;
					}
				}
			}else{
				if (u->GetUnitDef()->minWaterDepth > 0) {
					//it is a water-only unit or building
					return true;
				}
			}
			// do stuff to determine if there is no use for this unit
			if(u->GetUnitDef()->extractRange > 0) return false;
			if(u->GetUnitDef()->radarRadius > 1) return false;
			if(u->GetUnitDef()->sonarJamRadius> 1) return false;
			
			if(u->GetUnitDef()->sonarRadius > 1){
				return false;
			}

			if(u->GetUnitDef()->builder == true) return false;
			if(u->GetUnitDef()->weapons.empty() == false) return false;
			if(u->GetUnitDef()->canmove == true) return false;
			if(u->GetUnitDef()->canfly == true) return false;
			if(u->GetUnitDef()->canhover == true) return false;
			if(u->GetUnitDef()->canCapture == true) return false;
			if(u->GetUnitDef()->canDGun == true) return false;
			if(u->GetUnitDef()->canKamikaze == true) return false;
			if(u->GetUnitDef()->canResurrect == true) return false;
			if(u->GetUnitDef()->energyMake > 10) return false;
			if(u->GetUnitDef()->type == string("MetalExtractor")) return false;
			if(u->GetUnitDef()->isMetalMaker == true) return false;
			//if(udt->isFeature == true) return false;
			if(u->GetUnitDef()->jammerRadius > 10) return false;
			if(u->GetUnitDef()->metalMake > 0) return false;
			if(u->GetUnitDef()->needGeo == true) return false;
			if(u->GetUnitDef()->tidalGenerator > 0) return false;
			if(u->GetUnitDef()->windGenerator > 0) return false;
			if(u->GetUnitDef()->transportCapacity > 0) return false;
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
		string s = "";
		if(utd->GetUnitDef()->buildOptions.empty()){
			s;
		}
		switch(build){
			case B_MEX :{
				s = GetMEX();
				break;
			}case B_POWER:{
				s = GetPOWER();
				break;
			}case B_RAND_ASSAULT:{
				s = GetRAND_ASSAULT();
				break;
			}case B_ASSAULT:{
				s = GetASSAULT();
				break;
			}case B_FACTORY:{
				s = GetFACTORY();
				break;
			}case B_BUILDER:{
				s = GetBUILDER();
				break;
			}case B_GEO:{
				s = GetGEO();
				break;
			}case B_SCOUT:{
				s = GetSCOUT();
				break;
			}case B_RANDOM:{
				s = GetRANDOM();
				break;
			}case B_DEFENCE:{
				s = GetDEFENCE();
				break;
			}case B_RADAR:{
				s = GetRADAR();
				break;
			}case B_ESTORE:{
				s = GetESTORE();
				break;
			}case B_MSTORE:{
				s = GetMSTORE();
				break;
			}case B_SILO:{
				s = GetSILO();
				break;
			}case B_JAMMER:{
				s = GetJAMMER();
				break;
			}case B_SONAR:{
				s = GetSONAR();
				break;
			}case B_ANTIMISSILE:{
				s = GetANTIMISSILE();
				break;
			}case B_ARTILLERY:{
				s = GetARTILLERY();
				break;
			}case B_FOCAL_MINE:{
				s = GetFOCAL_MINE();
				break;
			}case B_SUB : {
				s = GetSUB();
				break;
			}case B_AMPHIB: {
				s = GetAMPHIB();
				break;
			}case B_MINE:{
				s = GetMINE();
				break;
			}case B_CARRIER:{
				s = GetCARRIER();
				break;
			}case B_METAL_MAKER:{
				s = GetMETAL_MAKER();
				break;
			}case B_FORTIFICATION:{
				s = GetFORTIFICATION();
				break;
			}case B_BOMBER:{
				s = GetBOMBER();
				break;
			}case B_FIGHTER:{
				s = GetFIGHTER();
				break;
			}case B_GUNSHIP:{
				s = GetGUNSHIP();
				break;
			}case B_SHIELD:{
				s = GetSHIELD();
				break;
			}case B_MISSILE_UNIT:{
				s = GetMISSILE_UNIT();
				break;
			}case B_HUB:{
				s = GetHUB();
				break;
			}case B_AIRSUPPORT:{
				s = GetAIRSUPPORT();
				break;
			}/*default:{
				return string("");
				break;
			}*/
		}
		return s;
	}

	string CUBuild::GetMEX(){
		NLOG("CUBuild::GetMEX");
		// Find all metal extractors this unit can build and add them to a list.
		float highscore= 0;
		string highest="";
		for(map<int,string>::const_iterator is = utd->GetUnitDef()->buildOptions.begin(); is != utd->GetUnitDef()->buildOptions.end();++is){

			CUnitTypeData* p = G->UnitDefLoader->GetUnitTypeDataByName(is->second);

			if(p->IsMex()){
				if(Useless(p)){
					continue;
				}

				float score = (p->GetUnitDef()->extractsMetal+1)*1000;
				if(p->GetUnitDef()->weapons.empty()==false){
					score *= 1.2f;
				}

				if(p->GetUnitDef()->canCloak==true){
					score *= 1.2f;
				}

				score /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
				if(score > highscore){
					highscore = score;
					highest = p->GetUnitDef()->name;
				}

				continue;
			}

			if((p->GetUnitDef()->metalMake > 5)&&(p->GetUnitDef()->builder == false)&&(p->GetUnitDef()->movedata == 0 )&&(p->GetUnitDef()->canfly == false)){
				float score = p->GetUnitDef()->metalMake;
				if(p->GetUnitDef()->weapons.empty()==false){
					score *= 1.2f;
				}
				if(p->GetUnitDef()->canCloak==true){
					score *= 1.2f;
				}
				score /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
				if(score > highscore){
					highscore = score;
					highest = p->GetUnitDef()->name;
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
				CUnitTypeData* p = G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				if(p->IsEnergy()) possibles_u.push_back(p->GetUnitDef());
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
		//return string("");
	}

	string CUBuild::GetRAND_ASSAULT(){
		NLOG("CUBuild::GetRAND_ASSAULT");
		const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
		if(di == 0) return string("");
		list<string> possibles;
		int defnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
			if(is->id<0){

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false) continue;
				if(p->IsAttacker()){
					possibles.push_back(p->GetName());
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
				CUnitTypeData* p = G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false){
					continue;
				}
				if(p->IsAttacker()){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp -= (G->mrand()%max(int(temp/4),1));
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false) continue;
				if(p->IsBomber()){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp = temp - G->mrand()%max(int(temp/4),1);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false) continue;
				if(p->IsFighter()){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp = temp - G->mrand()%max(int(temp/4),1);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false) continue;
				if(p->IsGunship()){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp = temp - G->mrand()%max(int(temp/4),1);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(p->GetUnitDef()->metalCost+p->GetUnitDef()->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("2.1", "AI\\cheap_multiplier").c_str())) continue;
				bool good = true;
				if(p->GetUnitDef()->canfly == false) good = false;
				if(p->GetUnitDef()->weapons.empty() == true) good = false;
				if(p->GetUnitDef()->builder == true) good = false;
				if(p->GetUnitDef()->transportCapacity > 0) good = false;
				bool found = false;
				for(vector<UnitDef::UnitDefWeapon>::const_iterator i = p->GetUnitDef()->weapons.begin(); i != p->GetUnitDef()->weapons.end(); ++i){
					if(i->def->interceptor > 0){
						continue;
					}
					if(i->def->type == string("StarburstLauncher")){
						found = true;
						break;
					}
				}
				if(found == false) good = false;
				if(good == true){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp = temp - G->mrand()%min((int)(temp/3),1);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);


				if(G->Pl->feasable(p,utd)) continue;

				if(p->GetUnitDef()->metalCost+p->GetUnitDef()->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("2.1", "AI\\cheap_multiplier").c_str())) continue;

				bool found = false;
				if(p->GetUnitDef()->weapons.empty() == false){
					for(vector<UnitDef::UnitDefWeapon>::const_iterator i = p->GetUnitDef()->weapons.begin(); i != p->GetUnitDef()->weapons.end(); ++i){
						if(i->def->isShield){
							found = true;
							break;
						}
					}
				}
				if(found == true){
					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					temp = temp - G->mrand()%max(int(temp/4),1);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				CUnitTypeData* p = G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				
				if(p->IsFactory()){

					if(!G->Pl->feasable(p,utd)) continue;
					if(p->IsUWStructure()!=(water||G->info->spacemod)) continue;

					float temp = G->efficiency->GetEfficiency(p->GetName());
					temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
					int r = G->mrand()%max(int(temp/4),1)+1;
					temp += (float)abs(r);

					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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
				
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);


				if(p->GetUnitDef()->builder&&((p->GetUnitDef()->movedata !=0)||p->GetUnitDef()->canfly)){
					if(G->Pl->feasable(p,utd)==false) continue;
					if(p->GetUnitDef()->floater){
						if(G->info->spacemod||water){
							float temp = G->efficiency->GetEfficiency(p->GetUnitDef()->name);
							if(!p->GetUnitDef()->buildOptions.empty()){
								for(map<int,std::string>::const_iterator i = p->GetUnitDef()->buildOptions.begin(); i != p->GetUnitDef()->buildOptions.end(); ++i){
									temp += G->efficiency->GetEfficiency(i->second);
								}
							}
							temp*=p->GetUnitDef()->buildSpeed;
							temp /= (p->GetUnitDef()->energyCost+(p->GetUnitDef()->metalCost*45));
							temp = temp - G->mrand()%max(int(temp/4),1);
							if(temp > best_score){
								best_score = temp;
								best = p->GetName();
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

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				if(G->Pl->feasable(p,utd)==false) continue;
				if(p->GetUnitDef()->needGeo&&(p->GetUnitDef()->builder == false)) possibles.push_back(p->GetName());
			}
		}

		if(possibles.empty() == false){
			return possibles.front();
		}

		return string("");
	}

	string CUBuild::GetSCOUT(){
		NLOG("CUBuild::GetSCOUT");

		const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);

		if(di == 0) return string("");

		list<string> possibles;

		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
			if(is->id<0){

				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);


				//if(Useless(ud)) continue;
				if((p->GetUnitDef()->speed > G->info->scout_speed)&&(p->GetUnitDef()->movedata != 0)&&(p->GetUnitDef()->canfly == false)&&(p->GetUnitDef()->transportCapacity == 0)&&(p->GetUnitDef()->isCommander == false)&&(p->GetUnitDef()->builder == false))  possibles.push_back(p->GetName());
				if((p->GetUnitDef()->weapons.empty() == true)&&(p->GetUnitDef()->canfly == true)&&(p->GetUnitDef()->transportCapacity == 0)&&(p->GetUnitDef()->isCommander == false)&&(p->GetUnitDef()->builder == false))  possibles.push_back(p->GetName());
				if((p->GetUnitDef()->weapons.empty()==true)&&(p->GetUnitDef()->canResurrect == false)&&(p->GetUnitDef()->canCapture == false)&&(p->GetUnitDef()->builder == false)&& ((p->GetUnitDef()->canfly==true)||(p->GetUnitDef()->movedata != 0)) &&((p->GetUnitDef()->radarRadius > 10)||(p->GetUnitDef()->sonarRadius > 10)))  possibles.push_back(p->GetName());
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

		// for each possible build command the unit could do
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){

			if(is->id<0){

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				
				// no mine fields
				if(p->GetUnitDef()->canKamikaze &&((!p->GetUnitDef()->canfly)&&(p->GetUnitDef()->movedata == 0))){
					// IT'S AN EXPLOSIVE MINE!!!!
					// We dont want nonblocking mines built ontop of eachother next to the factory!!!!!!!!!!!
					continue;
				}
				
				// no radar towers!!!!
				if((p->GetUnitDef() ->radarRadius > 10)&&((!p->GetUnitDef()->builder)||p->GetUnitDef()->isAirBase)){
					continue;
				}
				
				// no resource storage!!!!!!!!
				if((p->GetUnitDef()->energyStorage > 100)||(p->GetUnitDef()->metalStorage > 100)){
					continue;
				}

				// no metal makers
				if(p->GetUnitDef()->isMetalMaker){
					continue;
				}
				
				// remove 'useless' or 'irrelevant' items
				if(Useless(p)){
					continue;
				}
				
				if((p->GetUnitDef()->metalCost+p->GetUnitDef()->energyCost < (G->cb->GetEnergy()+G->cb->GetMetal())*0.9)&&(((!p->GetUnitDef()->floater)&&(!water)&&(!G->info->spacemod))||(G->info->spacemod||water))){
					
					// check the antistall algorithm if it's ok
					if(!G->Pl->feasable(p,utd)){
						continue;
					}
					
					possibles.push_back(p->GetName());
					randnum++;
				}
			}
		}

		// if potential items where found
		if(!possibles.empty()){

			// randomly pick one of the
			randnum = G->mrand()%randnum;

			// remove entries from the beginning of the list of potential items
			// so that the randnum'th entry is at the begining of the list
			while(randnum > 0){
				possibles.pop_front();
			}
			
			// now that we've removed all those entries our random selection
			// is at the front of the list, so lets return it
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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(Useless(p)) continue;

				if((p->GetUnitDef()->weapons.empty() == false)&&(!p->GetUnitDef()->isFeature)&&(!p->IsMobile())){
					if((G->info->spacemod == true)||(water == true)){
						if(G->Pl->feasable(p,utd)==false) continue;
						possibles.push_back(p->GetName());
						defnum++;
					} else if (pd->floater == false){
						if(G->Pl->feasable(p,utd)==false) continue;
						possibles.push_back(p->GetName());
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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(pd == 0) continue;
				if(Useless(p)) continue;
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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(pd->metalStorage >100){

					if(G->Pl->feasable(p,utd)==false) continue;

					if(pd->floater){
						if(G->info->spacemod||water){
							possibles.push_back(pd->name);
							randnum++;
						}
					} else {
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
				
				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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
				
				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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
				
				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(pd->canKamikaze&&((pd->canfly == false)&&(pd->movedata == 0))){

					if(G->Pl->feasable(pd->name,uid)==false) continue;

					if(pd->floater){
						if(G->info->spacemod||water){
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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();


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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(G->DTHandler->IsDragonsTeeth(pd)){
					possibles.push_back(pd->name);
				}

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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);

				const UnitDef* pd = p->GetUnitDef();

				if(p->IsHub()==true){
					if(G->Pl->feasable(p,utd)==false) continue;
					if(p->IsUWStructure()!=(water||G->info->spacemod)) continue;
					float temp = G->efficiency->GetEfficiency(pd->name);
					temp /= (pd->energyCost+(pd->metalCost*45));
					int r = G->mrand()%max(int(temp/4),1)+1;
					temp += (float)abs(r);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
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

				// retrieve the unit type information
				CUnitTypeData* p =G->UnitDefLoader->GetUnitTypeDataByName(is->name);
				const UnitDef* pd = p->GetUnitDef();

				if(p->IsHub()){
					if(G->Pl->feasable(p,utd)==false) continue;
					if(p->IsUWStructure()!=(water||G->info->spacemod)) continue;
					float temp = G->efficiency->GetEfficiency(pd->name);
					temp /= (pd->energyCost+(pd->metalCost*45));
					int r = G->mrand()%max(int(temp/4),1)+1;
					temp += (float)abs(r);
					if(temp > best_score){
						best_score = temp;
						best = p->GetName();
					}
				}
			}
		}
		return best;
	}

}
