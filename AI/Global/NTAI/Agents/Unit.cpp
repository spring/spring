#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL){
	G=GL;
	construction = false;
	executions = 0;
	valid = true;
	targ = _T("");
	help = false;
	type = B_NA;
}

Task::Task(Global* GL, Command _c){
	G=GL;
	type = B_NA;
	construction = false;
	executions = 0;
	valid = true;
	targ = _T("");
	help = false;
	c = _c;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL,string uname, int b_spacing){
	G = GL;
	type = B_NA;
	const UnitDef* u = G->cb->GetUnitDef(uname.c_str());
	if(u == 0){
		G->L.print(_T("unitdef failed to load Task::Task"));
		valid = false;
	}else{
		targ = uname;
		construction = true;
		spacing = b_spacing;
		help = false;
		executions = 0;
		valid = true;
	}
}
Task::Task(Global* GL,  int b_type, Tunit* tu){
	G=GL;
	construction = false;
	help = true;
	executions = 0;
	valid = true;
	targ = _T("");
	type = b_type;
	spacing = 9;
	t = tu;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Task::execute(int uid){
	if(this->valid == true){
		if(construction == true){
			const UnitDef* udi = G->cb->GetUnitDef(uid);
			if(udi == 0){
				valid = false;
				return false;
			}
			string typ = _T("Factory");
			if(udi->type == typ){
				if(G->Fa->FBuild(targ,uid,1)==true){
					executions++;
					return true;
				}else{
					valid = false;
					return false;
				} 
			} else{
				if(G->Fa->CBuild(targ,uid,spacing) == true){
					executions++;
					return true;
				}else{
					valid = false;
					return false;
				}
			}
		} else if(type  == B_REPAIR){
			const UnitDef* udi = G->cb->GetUnitDef(uid);
			if(udi == 0) return false;
			float3 pos = G->cb->GetUnitPos(uid);
			int* hn = new int[5000];
			int h = G->cb->GetFriendlyUnits(hn,pos,2000);
			if( h>0){
				for( int i = 0; i<h; i++){
					if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->team)){
						TCommand TCS(tc,_T("Task::execute :: repair cammand unfinished units"));
						tc.unit = uid;
						tc.clear = false;
						tc.Priority = tc_pseudoinstant;
						tc.c.id = CMD_REPAIR;
						tc.c.params.push_back((float)hn[i]);
						G->GiveOrder(tc);
						delete [] hn;
						return true;
					}
				}
			}else{
				h = G->cb->GetFriendlyUnits(hn);
				if(h <1) return false;
				for( int i = 0; i<h; i++){
					if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->team)){
						TCommand TCS(tc,_T("Task::execute :: repair cammand damaged units"));
						tc.unit = uid;
						tc.clear = false;
						tc.Priority = tc_pseudoinstant;
						tc.c.id = CMD_REPAIR;
						tc.c.params.push_back(hn[i]);
						G->GiveOrder(tc);
						delete [] hn;
						return true;
					}
				}
			}
			delete [] hn;
			valid = false;
			return false;
		}else if( (type != B_NA) && (type != B_REPAIR)){
			Tunit uu(G,uid);
			targ = uu.GetBuild(type,true);
			if(targ == string(_T(""))){
				return false;
			}else{
				switch (type) {
					case B_FACTORY: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("8"), _T("AI\\factory_spacing")).c_str());
						break;
					}
					case B_FACTORY_CHEAP:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("8"), _T("AI\\factory_spacing")).c_str());
						break;
					}
					case B_POWER: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("8"), _T("AI\\power_spacing")).c_str());
						break;
					 }
				}
				if(t->factory == true){
					return G->Fa->FBuild(targ,uid,1);
				}else{
                    return G->Fa->CBuild(targ,uid,spacing);
				}
			}
		}else{
			if(G->cb->GiveOrder(uid,&c) != -1){
				executions++;
				return true;
			}else{
				valid = false;
				return false;
			}
		}
	} else{
		valid = false;
		return false;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Tunit::Tunit(Global* GLI, int id){
	G = GLI;
	//p = new cpersona;
	uid = id;
	iscommander = false;
	if(uid < 1){
		bad = true;
		return;
	}
	ud = G->cb->GetUnitDef(id);
	reclaiming = false;
	if(ud == 0){
		G->L.eprint(_T("Tunit constructor error, loading unitdefinition"));
		bad = true;
		return;
	}else{
		bad = false;
	}
	idle = true;
	creation = G->cb->GetCurrentFrame();
	frame = 1;
	guarding = false;
	factory = false;
	waiting = false;
	can_dgun = cd_unsure;
	if(ud->weapons.empty() == false){
		for(vector<UnitDef::UnitDefWeapon>::const_iterator i = ud->weapons.begin(); i!= ud->weapons.end(); ++i){
			if(i->def->manualfire == true){
				can_dgun = cd_yes;
				break;
			}
		}
	} else {
		can_dgun = cd_no;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Tunit::~Tunit(){
	tasks.erase(tasks.begin(),tasks.end());
	tasks.clear();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Tunit::AddTask(string buildname, int spacing){
	const UnitDef* ua = G->cb->GetUnitDef(buildname.c_str());
	if(ua == 0){
		G->L.print(_T("unitdef loading failed Tunit::AddTask ")+ buildname);
		return;
	} else{
		if(spacing == 40){
			if(ua->movedata == 0){
				if(ua->canfly == false){ // it's a building!
					if(ua->builder){// it's a factory!
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("8"), _T("AI\\factory_spacing")).c_str());
					}else if(ua->weapons.empty() == false){ // It's probably a defencive structure, though a solar with a very weak gun that did no damage would be caught in the wrong category according to this
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("15"), _T("AI\\defence_spacing")).c_str());
					}else if((ua->energyMake > 10)||(ua->tidalGenerator)||(ua->windGenerator)||(ua->energyUpkeep < 0)){
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("8"), _T("AI\\power_spacing")).c_str());
					}else{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("9"), _T("AI\\default_spacing")).c_str());
					}
				}else{// It's an aircraft!!!!
					spacing = atoi(G->Get_mod_tdf()->SGetValueDef(_T("9"), _T("AI\\default_spacing")).c_str()); // never gonna happen! not without engine changes anyways
				}
			} else{
				spacing = 9;
			}
		}
		Task t(G,buildname,spacing);
		if(t.valid == false){
			return;
		} else {
            tasks.push_back(t);
		}
	}
}

void Tunit::AddTask(int btype){
		Task t (G, btype,this);
		tasks.push_back(t);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Tunit::execute(Task t){
	return t.execute(uid);
}
string Tunit::GetBuild(vector<string> v, bool efficiency){
	if(efficiency == true){
		float best_score = 0;
		string best = _T("");
		if(v.empty() == false){
			for(vector<string>::const_iterator is = v.begin(); is != v.end();++is){
				const UnitDef* pd = G->cb->GetUnitDef(is->c_str());
				if(pd == 0) continue;
				srand(time(NULL) + G->randadd + G->team);
				G->randadd++;
				float temp = G->GetEfficiency(pd->name);
				temp = temp - rand()%(int)(temp/3);
				if(temp > best_score){
					best_score = temp;
					best = pd->name;
				}
			}
		}
		return best;
	}else{
		if(v.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			int dnum = rand()%(v.size()-1);
			int j = 0;
			for(vector<string>::iterator k = v.begin(); k != v.end(); ++k){
				if(j == dnum){
					return *k;
				}else{
					j++;
				}
			}
			return v.front();
		}
	}
}
string Tunit::GetBuild(int btype, bool water=false){
	NLOG(_T("Unit::GetBuild"));
	if(bad == true) return string(_T(""));
	if(ud == 0) return string(_T(""));
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string(_T(""));
	if(ud->buildOptions.empty() == true) return string(_T(""));
	if(btype == B_MEX){////////////////////////////////////////////////////////////////////////////////
		// Find all metal extractors this unit can build and add them to a list.
		int defnum=0;
		for(map<int,string>::const_iterator is = ud->buildOptions.begin(); is != ud->buildOptions.end();++is){
			const UnitDef* pd = G->cb->GetUnitDef(is->second.c_str());
			if(pd == 0) continue;
			if(pd->extractRange >0){
				possibles.push_back(pd->name);
				defnum++;
				continue;
			}
			if((pd->metalMake > 1)&&(pd->builder == false)&&(pd->movedata == 0 )&&(pd->canfly == false)){
				possibles.push_back(pd->name);
				defnum++;
				continue;
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			defnum = rand()%defnum;
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
	}else if(btype == B_POWER){////////////////////////////////////////////////////////////////////////////////
		// Find all energy generators this unit can build and add them to a list.
		list<const UnitDef*> possibles_u;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(((pd->energyMake>10)||(pd->energyUpkeep<0)||(pd->tidalGenerator>0)||(pd->windGenerator>0))&&(pd->needGeo == false)) possibles_u.push_back(pd);
				if((pd->windGenerator>0)&&(pd->needGeo == false)) possibles_u.push_back(pd);
			}
		}/*string g = "";
		if(wind != g){
			if(G->cb->GetMinWind() >12){
				return wind;
			}else{
				if(possibles.empty() == true){
					return wind;
				}
			}
		}*/
		string best_energy = _T("");
		if(possibles_u.empty() == false){
			float bestpoint = 0;
			for(list<const UnitDef*>::iterator pd = possibles_u.begin();pd != possibles_u.end(); ++pd){
				float temp_energy = 0;
				if((*pd)->energyUpkeep <0) temp_energy += -(*pd)->energyUpkeep;
				if((*pd)->windGenerator>1) temp_energy += (G->cb->GetMaxWind() + G->cb->GetMinWind())/2;
				if((*pd)->energyMake > 1) temp_energy += (*pd)->energyMake;
				if((*pd)->tidalGenerator >0) temp_energy += G->cb->GetTidalStrength();
				if(temp_energy > bestpoint){
					bestpoint = temp_energy;
					best_energy = (*pd)->name;
				}
			}
		}
		return best_energy;
	}else if(btype == B_ASSAULT){////////////////////////////////////////////////////////////////////////////////
		float best_score = 0;
		string best = _T("");
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->metalCost+pd->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("1.3", "AI\\cheap_multiplier").c_str())) continue;
				if(((pd->weapons.empty() == false)&&((pd->movedata)||(pd->canfly == true)))&&((((pd->speed > 70)&&(pd->canfly == false))||((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->builder == false)&&(pd->isCommander == false))) == false)&&(pd->transportCapacity == 0)){
					srand(time(NULL) + G->randadd);
					G->randadd++;
					float temp = G->GetEfficiency(pd->name);
					temp = temp - rand()%(int)(temp/3);
					if(temp > best_score){
						best_score = temp;
						best = pd->name;
					}
				}
			}
		}
		if(best_score < 30){
			return GetBuild(B_RAND_ASSAULT,false);
		}
		return best;
	}else if(btype == B_GEO){////////////////////////////////////////////////////////////////////////////////
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->needGeo == true) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
	}else if(btype == B_RAND_ASSAULT){////////////////////////////////////////////////////////////////////////////////
		int defnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->metalCost+pd->energyCost > (G->cb->GetEnergyStorage()+G->cb->GetMetalStorage())*atof(G->Get_mod_tdf()->SGetValueDef("1.3", "AI\\cheap_multiplier").c_str())) continue;
				if(((pd->weapons.empty() == false)&&((pd->movedata)||(pd->canfly == true)))&&((((pd->speed > 70)&&(pd->canfly == false))||((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->builder == false)&&(pd->isCommander == false))) == false)&&(pd->transportCapacity == 0)){
					possibles.push_back(pd->name);
					defnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			defnum = rand()%defnum;
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
	}else if(btype == B_DEFENCE){////////////////////////////////////////////////////////////////////////////////
		int defnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->weapons.empty() == false)&&(!pd->movedata)){
					if((G->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						defnum++;
					} else if (pd->floater == false){
						possibles.push_back(pd->name);
						defnum++;
					}
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			defnum = rand()%defnum;
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
	}else if(btype == B_FACTORY){////////////////////////////////////////////////////////////////////////////////
		// Find all factories this unit can build and add them to a list.
		int facnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&(!pd->movedata)&&(pd->canfly == false)){
					if((G->spacemod == true)||(water == true)){
						possibles.push_back(pd->name);
						facnum++;
					} else if (pd->floater == false){
						possibles.push_back(pd->name);
						facnum++;
					}
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			facnum = rand()%facnum;
			int j = 0;
			for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
				if(j == facnum){
					return *k;
				}else{
					j++;
				}
			}
			return possibles.front();
		}
	}else if(btype == B_FACTORY_CHEAP){////////////////////////////////////////////////////////////////////////////////
		// Find all factories this unit can build and add them to a list.
		float multiplier = atof(G->Get_mod_tdf()->SGetValueMSG(_T("AI\\cheap_multiplier")).c_str());
		if(multiplier == 0) multiplier = 1.3f;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&(pd->movedata == 0)&&(pd->canfly == false)){
					if(G->spacemod == true){
						if(pd->metalCost+pd->energyCost < (G->cb->GetEnergyStorage()+1+G->cb->GetMetalStorage())*multiplier) possibles.push_back(pd->name);
					}else if(water == true){
						if(pd->metalCost+pd->energyCost < (G->cb->GetEnergyStorage()+1+G->cb->GetMetalStorage())*multiplier) possibles.push_back(pd->name);
					}else if(pd->floater == false){
						if(pd->metalCost+pd->energyCost < (G->cb->GetEnergyStorage()+1+G->cb->GetMetalStorage())*multiplier) possibles.push_back(pd->name);
					}
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			int facnum = rand()%possibles.size();
			int j = 0;
			for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
				if(j == facnum){
					return *k;
				}else{
					j++;
				}
			}
			return possibles.front();
		}
	}else if(btype == B_BUILDER){////////////////////////////////////////////////////////////////////////////////
		// Find all builders this unit can build and add them to a list.
		int buildnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&((pd->movedata !=0)||(pd->canfly == true))){
					possibles.push_back(pd->name);
					buildnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			buildnum = rand()%buildnum;
			int j = 0;
			for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
				if(j == buildnum){
					return *k;
				}else{
					j++;
				}
			}
			return possibles.front();
		}
	}else if(btype == B_SCOUT){////////////////////////////////////////////////////////////////////////////////
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->speed > 60)&&(pd->movedata != 0)&&(pd->canfly == false)&&(pd->transportCapacity == 0)&&(pd->isCommander == false)&&(pd->builder == false)) possibles.push_back(pd->name);
				if((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->transportCapacity == 0)&&(pd->isCommander == false)&&(pd->builder == false)) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
	}else if(btype == B_RANDOM){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->canKamikaze)&&((pd->canfly == false)&&(pd->movedata == 0))) continue; // IT'S A MINE!!!! We dont want nonblocking mines built ontop of eachother next to the factory!!!!!!!!!!!
				if((pd->radarRadius > 10)&&((pd->builder == false)||(pd->isAirBase == true))) continue; // no radar towers!!!!
				if((pd->energyStorage > 100)||(pd->metalStorage > 100)) continue; // no resource storage!!!!!!!!
				if(pd->isMetalMaker == true) continue; // no metal makers
				if((pd->metalCost+pd->energyCost < (G->cb->GetEnergy()+G->cb->GetMetal())*0.9)&&(((pd->floater == false)&&(water == false)&&(G->spacemod == false))||((G->spacemod == true)||(water == true)))){
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_FORTIFICATION){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->isFeature == true){
					if(pd->floater == true){
						if((G->spacemod == true)||(water == true)){
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
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_ESTORE){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->energyStorage >100){
					if(pd->floater == true){
						if((G->spacemod == true)||(water == true)){
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
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_MINE){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->canKamikaze)&&((pd->canfly == false)&&(pd->movedata == 0))){
					if(pd->floater == true){
						if((G->spacemod == true)||(water == true)){
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
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_MSTORE){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->metalStorage >100){
					if(pd->floater == true){
						if((G->spacemod == true)||(water == true)){
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
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_CARRIER){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->isAirBase == true){
					if(pd->floater == true){
						if((G->spacemod == true)||(water == true)){
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
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_METAL_MAKER){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->isMetalMaker == true)&&(pd->floater == false)){
					possibles.push_back(pd->name);
					randnum++;
				}
				if((pd->isMetalMaker == true)&&(pd->floater == true)&&((G->spacemod == true)||(water == true))){
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else if(btype == B_RADAR){////////////////////////////////////////////////////////////////////////////////
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->radarRadius >10)&&(pd->floater == false)&&(pd->builder == false)){
					possibles.push_back(pd->name);
					randnum++;
				}else if((pd->radarRadius >10)&&(pd->floater == false)&&(pd->builder == false)&&(pd->floater == true)&&((G->spacemod == true)||(water == true))){
					possibles.push_back(pd->name);
					randnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL) +G->team +  G->randadd);
			G->randadd++;
			randnum = rand()%randnum;
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
	}else{
		return string(_T(""));
	}
	return string(_T(""));
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

