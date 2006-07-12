#include "../Core/helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Scouter::InitAI(Global* GLI){
	G = GLI;
	for(vector<float3>::iterator kj = G->M->hotspot.begin(); kj != G->M->hotspot.end(); ++kj){
		mexes.push_back(*kj);
	}
	TdfParser cq(G);
	cq.LoadFile("script.txt");
	int nd=0;
	cq.GetDef(nd,"0","GAME\\NumTeams");
	TdfParser MP(G);
	MP.LoadFile(string("maps\\")+string(G->cb->GetMapName()).substr(0,string(G->cb->GetMapName()).find('.'))+".smd");
	vector<string> sections = MP.GetSectionList("MAP\\");
	for(int n=0; n<nd; n++){
    	char c[8];
    	//itoa(n,c,10);// something to do with ANSI standards and 64 bit compatability
		sprintf(c,"%i",n);
		float3 pos= UpVector;
    	MP.GetDef(pos.x, "-1", string("MAP\\TEAM") + string(c) + "\\StartPosX");
    	MP.GetDef(pos.z, "-1", string("MAP\\TEAM") + string(c) + "\\StartPosZ");
		if(G->Map->CheckFloat3(pos)==true){
			start_pos.push_back(pos);
			G->Actions->AddPoint(pos);
		}
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitFinished(int unit){
	NLOG("Scouter::UnitFinished");
	const UnitDef* ud = G->GetUnitDef(unit);
	if( ud == 0) return;
	bool naset = false;
	vector<string> v;
	v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\Scouters"));
	if(v.empty() == false){
		for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
			if(*vi == ud->name){
				naset = true;
				break;
			}
		}
	}
	if((G->info->dynamic_selection == false)&&(naset==false)){
		// determine if this is a scouter or not dynamically
		if((ud->speed > G->info->scout_speed)&&(ud->movedata != 0)&&(ud->canfly == false)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty()==true)&&(ud->canResurrect == false)&&(ud->canCapture == false)&&(ud->builder == false)&& ((ud->canfly==true)||(ud->movedata != 0)) &&((ud->radarRadius > 10)||(ud->sonarRadius > 10))) naset = true;
	}
	float3 dpos = G->GetUnitPos(unit);
	if((ud->extractsMetal >0)&&(cp.empty() == false)){
		for(map<int, list<float3> >::iterator i = cp.begin(); i != cp.end(); ++i){
			if(i->second.empty() == false){
				for(list<float3>::iterator j = i->second.begin(); j != i->second.end(); ++j){
					if(*j == dpos){
						i->second.erase(j);
						break;
					}
				}
			}
		}
		if(mexes.empty() == false){
			for(list<float3>::iterator i = mexes.begin(); i != mexes.end(); ++i){
				if( *i == dpos){
					mexes.erase(i);
					break;
				}
			}
		}
	}
	if(naset){
		if((G->cb->GetCurrentFrame()%4 == 2)||(cp.empty() == true)){
            cp[unit] = start_pos;
		}else if((G->cb->GetCurrentFrame()%4 == 1)||(G->cb->GetCurrentFrame()%4 == 3)){
			if(this->sectors.empty() == true){
				list<float3> cf;
				for(int xa = 0; xa < G->Ch->xSectors; xa++){
					for(int ya = 0; ya < G->Ch->ySectors; ya++){
						if(xa > ya){
							sectors.push_back(G->Ch->sector[xa][ya].centre);
						}else{
							sectors.insert(cf.begin(),G->Ch->sector[xa][ya].centre);
						}
					}
				}
				cp[unit] = cf;
			}else{
				cp[unit] = sectors;
			}
		}else if(G->cb->GetCurrentFrame()%4 == 0){
			if(G->info->mexscouting == true){
				cp[unit] = mexes;
			}else{
				if(this->sectors.empty() == true){
					list<float3> cf;
					for(int xa = 0; xa < G->Ch->xSectors; xa++){
						for(int ya = 0; ya < G->Ch->ySectors; ya++){
							if(xa > ya){
								sectors.push_back(G->Ch->sector[xa][ya].centre);
							}else{
								sectors.insert(cf.begin(),G->Ch->sector[xa][ya].centre);
							}
						}
					}
					cp[unit] = cf;
				}else{
					cp[unit] = sectors;
				}
			}
		}
		G->L.print("scouter found");
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitMoveFailed(int unit){
	NLOG("Scouter::UnitMoveFailed");
	UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitIdle(int unit){
	NLOG("Scouter::UnitIdle");
	
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0) return;
	bool naset = false;
	vector<string> v;
	v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\Scouters"));
	if(v.empty() == false){
		for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
			if(*vi == ud->name){
				naset = true;
				break;
			}
		}
	}
	if((G->info->dynamic_selection == true)&&(naset==false)){
		// determine if this is a scouter or not dynamically
		if((ud->speed > G->info->scout_speed)&&(ud->movedata != 0)&&(ud->canfly == false)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty()==true)&&(ud->canResurrect == false)&&(ud->canCapture == false)&&(ud->builder == false)&& ((ud->canfly==true)||(ud->movedata != 0)) &&((ud->radarRadius > 10)||(ud->sonarRadius > 10))) naset = true;
	}
	
	if(naset){
		srand(uint(time(NULL)) + uint(G->Cached->randadd));
		G->Cached->randadd++;
		int j = rand()%66;
		for(int i = 0; i < j; i++){
			cp[unit].push_back(cp[unit].front());
			cp[unit].pop_front();
		}
		float3 upos = G->GetUnitPos(unit);
		float3 pos=cp[unit].front();
		if(G->Map->CheckFloat3(upos)== true){
			pos = G->Map->distfrom(upos,pos,200);
		}
		if(G->Map->CheckFloat3(pos) == false){
			if(G->Actions->SeekOutInterest(unit)==false){
				if(G->Actions->RandomSpiral(unit)==false){
					G->Actions->ScheduleIdle(unit);
					return;
				}
			}
		}
		if(G->InLOS(pos)==true){
			if(G->Actions->SeekOutInterest(unit)==false){
				if(G->Actions->RandomSpiral(unit)==false){
					G->Actions->ScheduleIdle(unit);
					return;
				}
			}
		}
		pos.y = G->cb->GetElevation(pos.x,pos.z);
		cp[unit].push_back(cp[unit].front());
		cp[unit].erase(cp[unit].begin());
		if(G->Actions->Move(unit,pos) == false){
			G->Actions->ScheduleIdle(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitDestroyed(int unit){
	NLOG("Scouter::UnitDestroyed");
	if(cp.empty() == false){
		for(map< int, list<float3> >::iterator q = cp.begin(); q != cp.end();++q){
			if(q->first == unit){
				cp.erase(q);
				break;
			}
		}
		const UnitDef* ud = G->GetUnitDef(unit);
		if(ud == 0){
			return;
		}else{
			float3 dpos = G->GetUnitPos(unit);
			if((ud->extractsMetal >0)&&(cp.empty() == false)){
				for(map<int, list<float3> >::iterator i = cp.begin(); i != cp.end(); ++i){
					i->second.push_back(dpos);
				}
				mexes.push_back(dpos);
			}
		}
	}
}

float3 Scouter::GetMexScout(int i){
	NLOG("Scouter::GetMexScout");
	int j=0;
	int h = (int)mexes.size();
	if(i > h-2){
		j = (h-2)%i ;
		if(j==0) j++;
	}else{
		j = i;
		if(j==0) j++;
	}
	list<float3>::iterator ij= mexes.begin();
	for(int hj = 0; hj != j; ++hj){
		++ij;
	}
	return *ij;
}

float3 Scouter::GetStartPos(int i){
	NLOG("Scouter::GetStartPos");
	int j;
	if(i > (int)start_pos.size()){
		j = (int)start_pos.size()%i;
	}else{
		j = i;
	}
	list<float3>::iterator ij= start_pos.begin();
	for(int hj = 0; hj != j; ++hj){
		++ij;
	}
	return *ij;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

