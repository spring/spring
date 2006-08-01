#include "../Core/helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL){
	G=GL;
	construction = false;
	valid = true;
	targ = "";
	type = B_NA;
}

Task::Task(Global* GL, Command _c){
	G=GL;
	type = B_NA;
	construction = false;
	valid = true;
	targ = "";
	c = _c;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL, bool meta,string uname, int b_spacing){
	G = GL;
	metatag = false;
	type = B_NA;
	if(meta == true){
		metatag = true;
		construction = true;
		spacing = b_spacing;
		valid = true;
	}else{
		const UnitDef* u = G->cb->GetUnitDef(uname.c_str());
		if(u == 0){
			G->L.print(string("unitdef failed to load Task::Task :: ") + uname);
			valid = false;
		}else{
			targ = uname;
			construction = true;
			spacing = b_spacing;
			valid = true;
		}
	}
}
Task::Task(Global* GL,  btype b_type){
	G=GL;
	construction = false;
	valid = true;
	targ = "";
	type = b_type;
	spacing = 9;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Task::execute(int uid){
	if(valid == true){
		const UnitDef* udi = G->cb->GetUnitDef(uid);
		if(udi == 0){
			G->L.print("fbuild returns false on :: ");
			return false;
		}
		if(metatag == true){
			targ = G->Manufacturer->GetBuild(uid,targ,true);
			construction = true;
		}
		if(construction == true){
			valid = G->Manufacturer->CBuild(targ,uid,spacing);
			if(valid == false){
				G->L.print(string("cbuild returns false on :: ")+targ);
			} 
			return valid;
		} else if(type == B_RANDMOVE){
			if(G->Actions->RandomSpiral(uid)==false){
				valid = false;
				return false;
			}else{
				return true;
			}
		} else if(type == B_RETREAT){
			return G->Actions->Retreat(uid);
		} else if(type == B_GUARDIAN){
			return G->Actions->RepairNearby(uid,1200);
		} else if(type == B_RESURECT){
			valid = G->Actions->RessurectNearby(uid);
			return valid;
		} else if(type == B_RULE){//////////////////////////////
			CUBuild b;
			b.Init(G,udi,uid);
			btype bt = G->Economy->Get(false);
			if(bt == B_NA){
				valid = false;
				return false;
			}
			if(G->UnitDefHelper->IsUWCapable(udi)){
				b.SetWater(true);
			}
			//G->L.print("checks out, getting targ value");
			targ = b(bt);
			//G->L.print("gotten targ value of " + targ);
			if (targ != string("")){
				switch (bt) {
					case B_FACTORY: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("14", "AI\\factory_spacing").c_str());
						break;
									}
					case B_DEFENCE:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("4", "AI\\defence_spacing").c_str());
						break;
								   }
					case B_POWER: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("3", "AI\\power_spacing").c_str());
						break;
								  }
					default:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("1", "AI\\default_spacing").c_str());
						break;
					}
				}
				return G->Manufacturer->CBuild(targ,uid,spacing);
			}else{
				return false;
			}
		} else if((type == B_RULE_EXTREME)||(type == B_RULE_EXTREME_NOFACT)||(type == B_RULE_EXTREME_CARRY)){//////////////////////////////
			CUBuild b;
			b.Init(G,udi,uid);
			btype bt;
			if(B_RULE_EXTREME_NOFACT == type){
				bt= G->Economy->Get(true,false);
			}else{
				bt= G->Economy->Get(true,true);
			}
			if(bt == B_NA){
				valid = false;
				return false;
			}
			targ = b(bt);
			G->L.print("gotten targ value of " + targ + " for RULE");
			if (targ != string("")){
				switch (type) {
					case B_FACTORY: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("14", "AI\\factory_spacing").c_str());
						break;
									}
					case B_DEFENCE:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("4", "AI\\defence_spacing").c_str());
						break;
								   }
					case B_POWER: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("3", "AI\\power_spacing").c_str());
						break;
								  }
					default:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("1", "AI\\default_spacing").c_str());
						break;
							}
				}
				return G->Manufacturer->CBuild(targ,uid,spacing);
			}else{
				G->L.print("B_RULE_EXTREME skipped bad return");
				return false;
			}
		}else if(type  == B_REPAIR){
			if(G->Actions->RepairNearby(uid,500)==false){
				if(G->Actions->RepairNearby(uid,1000)==false){
					if(G->Actions->RepairNearby(uid,2600)==false){
						return false;
					}else{
						return true;
					}
				}else{
					return true;
				}
			}else{
				return true;
			}
			return valid;
		}else if(type  == B_RECLAIM){
			valid = G->Actions->ReclaimNearby(uid);
			return valid;
		}else if(type  == B_GUARD_FACTORY){
			float3 upos = G->GetUnitPos(uid);
			if(G->Map->CheckFloat3(upos)==false){
				return false;
			}
			int* funits = new int[6000];
			int fnum = G->cb->GetFriendlyUnits(funits,upos,2000);
			if(fnum > 0){
				int closest=0;
				float d = 2500;
				for(int  i = 0; i < fnum; i++){
					if(funits[i] == uid) continue;
					float3 rpos = G->GetUnitPos(funits[i]);
					if(G->Map->CheckFloat3(rpos)==false){
						continue;
					}else{
						float q = upos.distance2D(rpos);
						if(q < d){
							const UnitDef* rd = G->GetUnitDef(funits[i]);
							if(rd == 0){
								continue;
							}else{
								if(rd->builder && (rd->movedata == 0) && (rd->buildOptions.empty()==false)){
									d = q;
									closest = funits[i];
									continue;
								}else{
									continue;
								}
							}
						}
					}
				}
				if(closest > 0){
					delete [] funits;
					return G->Actions->Guard(uid,closest);
				}else{
					delete [] funits;
					return false;
				}
			}
			delete [] funits;
			return false;
		}else if(type  == B_GUARD_LIKE_CON){
			float3 upos = G->GetUnitPos(uid);
			if(G->Map->CheckFloat3(upos)==false){
				return false;
			}
			int* funits = new int[5005];
			int fnum = G->cb->GetFriendlyUnits(funits,upos,2000);
			if(fnum > 0){
				int closest=0;
				float d = 2500;
				for(int  i = 0; i < fnum; i++){
					if(funits[i] == uid) continue;
					float3 rpos = G->GetUnitPos(funits[i]);
					if(G->Map->CheckFloat3(rpos)==false){
						continue;
					}else{
						float q = upos.distance2D(rpos);
						if(q < d){
							const UnitDef* rd = G->GetUnitDef(funits[i]);
							if(rd == udi){
								d = q;
								closest = funits[i];
								continue;
							}else{
								continue;
							}
						}
					}
				}
				if(closest > 0){
					delete [] funits;
					return G->Actions->Guard(uid,closest);
				}else{
					delete [] funits;
					return false;
				}
			}
			delete [] funits;
			return false;
		}else if( (type != B_NA) &&(type != B_RULE)&&(type != B_GUARDIAN)&&(type != B_RECLAIM)&&(type != B_RETREAT)&&(type != B_RULE_EXTREME)&&(type!= B_RESURECT)&&(type != B_RANDMOVE)&& (type != B_REPAIR)){
			CUBuild b;
			b.Init(G,G->GetUnitDef(uid),uid);
			b.SetWater(G->info->spacemod);
			//if(b.ud->floater==true){
			//	b.SetWater(true);
			//}
			targ = b(type);
			if(targ == string("")){
				G->L << "if(targ == string(\"\")) for "<< G->Manufacturer->GetTaskName(type)<< endline;
				return false;
			}else{
				switch (type) {
					case B_FACTORY: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("14", "AI\\factory_spacing").c_str());
						break;
					}
					case B_DEFENCE:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("4", "AI\\defence_spacing").c_str());
						break;
					}
					case B_POWER: {
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("3", "AI\\power_spacing").c_str());
						break;
					 }
					default:{
						spacing = atoi(G->Get_mod_tdf()->SGetValueDef("2", "AI\\default_spacing").c_str());
						break;
					}
				}
				valid =  G->Manufacturer->CBuild(targ,uid,spacing);
				return valid;
			}
		}else{
			if(G->cb->GiveOrder(uid,&c) != -1){
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
	valid = false;
	return false;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
/*
Tunit::Tunit(Global* GLI, int unit){
	G = GLI;
	dgunning = false;
	scavenging = false;
	reclaiming = false;
	waiting = false;
	uid = unit;
	if(uid < 1){
		int k = 0;
	}
	iscommander = false;
	if(uid < 1){
		bad = true;
		return;
	}
	ud = G->GetUnitDef(unit);
	if(ud == 0){
		G->L.eprint(_T("Tunit constructor error, loading unitdefinition"));
		bad = true;
		return;
	}else{
		bad = false;
	}
	creation = G->cb->GetCurrentFrame();
	if(creation < 1){
		bad = true;
		return;
	}
	role = R_OTHER;
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
	ubuild.Init(G,ud,unit);
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
		//Task t (G, btype,this);
		//tasks.push_back(t);
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
				srand(uint(time(NULL) + G->Cached->randadd + G->Cached->team));
				G->Cached->randadd++;
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
			srand(uint(time(NULL) +G->Cached->team +  G->Cached->randadd));
			G->Cached->randadd++;
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
	return string("");
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

