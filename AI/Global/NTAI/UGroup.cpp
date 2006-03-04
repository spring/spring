#include "helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL){
	G=GL;
	construction = false;
	executions = 0;
	valid = true;
	targ = "";
	help = false;
}

Task::Task(Global* GL, Command _c){
	G=GL;
	construction = false;
	executions = 0;
	valid = true;
	targ = "";
	help = false;
	c = _c;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL,string uname, int b_spacing){
	G = GL;
	const UnitDef* u = G->cb->GetUnitDef(uname.c_str());
	if(u == 0){
		G->L.print("unitdef failed to load Task::Task");
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
Task::Task(Global* GL,  bool repair, bool re){
	G=GL;
	construction = false;
	help = true;
	executions = 0;
	valid = true;
	targ = "";
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
			string typ = "Factory";
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
		} else if(help == true){
			const UnitDef* udi = G->cb->GetUnitDef(uid);
			if(udi == 0) return false;
			float3 pos = G->cb->GetUnitPos(uid);
			int* hn = new int[5000];
			int h = G->cb->GetFriendlyUnits(hn,pos,500);
			if( h>0){
				for( int i = 0; i<h; i++){
					if(G->cb->UnitBeingBuilt(hn[i]) == true){
						TCommand tc;
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
					if(G->cb->UnitBeingBuilt(hn[i]) == true){
						TCommand tc;
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
	ud = G->cb->GetUnitDef(id);
	reclaiming = false;
	if(ud == 0){
		G->L.eprint("Tunit constructor error, loading unitdefinition");
		bad = true;
	}else{
		bad = false;
	}
	idle = true;
	creation = G->cb->GetCurrentFrame();
	frame = 1;
	guarding = false;
	factory = false;
	waiting = false;
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
		G->L.print("unitdef loading failed Tunit::AddTask "+ buildname);
		return;
	} else{
		if(spacing == 40){
			if(ua->movedata == 0){
				if(ua->canfly == false){ // it's a building!
					if(ua->builder){// it's a factory!
						spacing = S_FACTORY;
					}else if(ua->weapons.empty() == false){ // It's probably a defencive structure, though a solar with a very weak gun that did no damage would be caught in the wrong category according to this
						spacing = S_DEFENCE;
					}else if((ua->energyMake > 10)||(ua->tidalGenerator)||(ua->windGenerator)||(ua->energyUpkeep < 0)){
						spacing = S_POWER;
					}else{
						spacing = 9;
					}
				}else{// It's an aircraft!!!!
					spacing = 9;// ignore
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

void Tunit::AddTask(bool repair,bool rep,bool re){
		Task t (G,true,true);
		tasks.push_back(t);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Tunit::execute(Task t){
	return t.execute(uid);
}

string Tunit::GetBuild(int btype, bool water=false){
	NLOG("Unit::GetBuild");
	if(bad == true) return string("nought");
	if(ud == 0) return string("nought");
	list<string> possibles;
	const vector<CommandDescription>* di = G->cb->GetUnitCommands(uid);
	if(di == 0) return string("nought");
	if(di->empty() == true) return string("nought");
	if(btype == B_MEX){
		// Find all metal extractors this unit can build and add them to a list.
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
			if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->extractsMetal>0) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
		G->L.print("no mexes found");
	}else if(btype == B_POWER){
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
		string best_energy = "";
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
		string g = "";
		if(best_energy != g){
			return best_energy;
		}
		G->L.print("no energy found");
	}else if(btype == B_ASSAULT){
		float best_score = 0;
		string best = "";
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(((pd->weapons.empty() == false)&&((pd->movedata)||(pd->canfly == true)))&&((((pd->speed > 70)&&(pd->canfly == false))||((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->builder == false)&&(pd->isCommander == false))) == false)&&(pd->transportCapacity == 0)){
					float temp = G->GetEfficiency(pd->name);
					if(temp > best_score){
						best_score = temp;
						best = pd->name;
					}
				}
			}
		}
		if(best_score < 10){
			return GetBuild(B_RAND_ASSAULT,false);
		}else if(best != string("")){
			return best;
		}
		G->L.print("no assault found");
	}else if(btype == B_GEO){
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->needGeo == true) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
		G->L.print("no geothermals found");
	}else if(btype == B_RAND_ASSAULT){
		int defnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(((pd->weapons.empty() == false)&&((pd->movedata)||(pd->canfly == true)))&&((((pd->speed > 70)&&(pd->canfly == false))||((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->builder == false)&&(pd->isCommander == false))) == false)&&(pd->transportCapacity == 0)){
					possibles.push_back(pd->name);
					defnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL));
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
		G->L.print("no assault found");
	}else if(btype == B_GEO){
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if(pd->needGeo == true) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
		G->L.print("no geothermals found");
	}else if(btype == B_DEFENCE){
		int defnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->weapons.empty() == false)&&(!pd->movedata)){
					possibles.push_back(pd->name);
					defnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL));
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
		G->L.print("no defence found");
	}else if(btype == B_FACTORY){
		// Find all factories this unit can build and add them to a list.
		int facnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&(!pd->movedata)){
					if(pd->floater == false){
						possibles.push_back(pd->name);
						facnum++;
					}
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL));
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
		G->L.print("no factories found");
	}else if(btype == B_FACTORY_CHEAP){
		// Find all factories this unit can build and add them to a list.
		float largest_metal = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
				const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&(!pd->movedata)){
					if(pd->floater == false){
						possibles.push_back(pd->name);
						if(pd->metalCost > largest_metal) largest_metal = pd->metalCost;
					}
				}
			}
		}
		if(possibles.empty() == false){
			int facnum = 0;
			list<string> poss2;
			largest_metal *= 0.65f;
			for(list<string>::iterator k = possibles.begin(); k != possibles.end(); ++k){
				const UnitDef* uds = G->cb->GetUnitDef(k->c_str());
				if(uds != 0){
					if(uds->metalCost < largest_metal){
						facnum++;
						poss2.push_back(*k);
					}
				}
			}
			if(poss2.empty() == false){
				srand(time(NULL));
				facnum = rand()%facnum;
				int j = 0;
				for(list<string>::iterator k = poss2.begin(); k != poss2.end(); ++k){
					if(j == facnum){
						return *k;
					}else{
						j++;
					}
				}
				return poss2.front();
			}else{
				return possibles.front();
			}
		}
		G->L.print("no factories found");
	}else if(btype == B_BUILDER){
		// Find all builders this unit can build and add them to a list.
		int buildnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->builder == true)&&(pd->movedata)){
					possibles.push_back(pd->name);
					buildnum++;
				}
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL));
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
		G->L.print("no builders found");
	}else if(btype == B_SCOUT){
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				if((pd->speed > 65)&&(pd->movedata != 0)&&(pd->canfly == false)&&(pd->transportCapacity == 0)&&(pd->isCommander == false)&&(pd->builder == false)) possibles.push_back(pd->name);
				if((pd->weapons.empty() == true)&&(pd->canfly == true)&&(pd->transportCapacity == 0)&&(pd->isCommander == false)&&(pd->builder == false)) possibles.push_back(pd->name);
			}
		}
		if(possibles.empty() == false) return possibles.front();
		G->L.print("no scouts found");
	}else if(btype == B_RANDOM){
		int randnum = 0;
		for(vector<CommandDescription>::const_iterator is = di->begin(); is != di->end();++is){
            if(is->id<0){
                const UnitDef* pd = G->cb->GetUnitDef(is->name.c_str());
				if(pd == 0) continue;
				possibles.push_back(pd->name);
				randnum++;
			}
		}
		if(possibles.empty() == false){
			srand(time(NULL));
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
		G->L.print("no nothing  found");
	}else{
		return string("nought");
	}
	return string("nought");
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

