#include "../helper.h"



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::Chaser(){
	G = 0;
	lock = false;
}

void Chaser::InitAI(Global* GLI){
	NLOG("Chaser::InitAI");
	forces=0;
	enemynum=0;
	G = GLI;
	threshold = atoi(G->mod_tdf->SGetValueDef("4", "AI\\initial_threat_value").c_str());
	string contents = G->Get_mod_tdf()->SGetValueMSG("AI\\kamikaze");
	if(contents.empty() == false){
		vector<string> v = bds::set_cont(v,contents.c_str());
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				*vi = G->lowercase(*vi);
				sd_proxim[*vi] = true;
			}
		}
	}
	

	// calculate number of sectors
	xSectors = int(floor(0.5 + ( G->cb->GetMapWidth())/40));
	ySectors = int(floor(0.5 + ( G->cb->GetMapHeight())/40));

	// calculate effective sector size
	xSizeMap = int(G->cb->GetMapWidth() / xSectors);
	ySizeMap = int(G->cb->GetMapHeight() /ySectors); 
	xSize = SQUARE_SIZE * xSizeMap;
	ySize = SQUARE_SIZE * ySizeMap;
	//if(sec_set == false){
	// create field of sectors
	float3 ik(xSize/2,0, ySize/2 );
	for(int i = 0; i < xSectors; i++){
		for(int j = 0; j < ySectors; j++){
			// set coordinates of the sectors
			sector[i][j].location = float3(i,0,j);
			sector[i][j].centre = float3(i*xSize,0,j*ySize);
			sector[i][j].centre += ik;
			sector[i][j].ground_threat = 10;
			sector[i][j].ebuildings = 10;
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Add(int unit, bool aircraft){
	G->L.print("Chaser::Add()");
	NO_GAIA
	Attackers.insert(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDestroyed(int unit, int attacker){
	NLOG("Chaser::UnitDestroyed");
	NO_GAIA
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0) return;
	if(groups.empty() == false){
		for(vector<ackforce>::iterator at = groups.begin();at !=groups.end();++at){
			for(set<int>::iterator aa = at->units.begin(); aa != at->units.end();++aa){
				if(*aa == unit){
					at->units.erase(aa);
					at->acknum -= 1;
					if(at->units.empty()) groups.erase(at);
					return;
				}
			}
		}
	}
	if(Attackers.empty() == false){
		for(set<int>::iterator aiy = Attackers.begin(); aiy != Attackers.end();++aiy){
			if(*aiy == unit){
				Attackers.erase(aiy);
				break;
			}
		}
	}
	if(sweap.empty() == false){
		if(sweap.find(unit) != sweap.end()){
			sweap.erase(unit);
		}
	}
	if(kamikaze_units.empty() == false){
		if(kamikaze_units.find(unit) != kamikaze_units.end()){
			kamikaze_units.erase(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	NLOG("Chaser::UnitDamaged");
	NO_GAIA
	const UnitDef* ud = G->cb->GetUnitDef(damaged);
	const UnitDef* adg;
	if(attacker <0 ){
		adg = 0;
	}else{
		adg = G->cb->GetUnitDef(attacker);
	}
	if(adg !=0){
		if(adg->isCommander == true){
			if(ud->isCommander == true){
				return;
			}
		}
	} else {
		return;
	}
	if(ud == 0) return;
	if(attacker <1) return;
	if(G->cb->GetUnitAllyTeam(attacker) == G->unitallyteam) return;
	float3 dpos = G->cb->GetUnitPos(damaged);
	if(dpos == UpVector){
		return;
	}else if(dpos == ZeroVector){
		return;
	}
	TCommand TCS(tc,_T("chaser::unitdamaged dgun"));
	tc.unit = damaged;
	tc.Priority = tc_instant;
	if(this->can_dgun.find(ud->name) != can_dgun.end()){
		if(can_dgun[ud->name] == true){
			tc.c.id = CMD_DGUN;
			tc.c.timeOut = 4 SECONDS + G->cb->GetCurrentFrame();
			tc.c.params.push_back(attacker);
			G->GiveOrder(tc);
		}
	}else if(ud->weapons.empty() == false){
		for(vector<UnitDef::UnitDefWeapon>::const_iterator wu = ud->weapons.begin();wu !=ud->weapons.end();++wu){
			if(wu->def->manualfire == true){
				can_dgun[ud->name] = true;
				if(adg != 0){
					if(adg->canfly == true){
						break;
					}
					if(adg->isCommander == true) break;
				}
				tc.c.id = CMD_DGUN;
				tc.c.timeOut = 4 SECONDS + G->cb->GetCurrentFrame();
				tc.c.params.push_back(attacker);
				G->GiveOrder(tc);
				break;
			}
		}
	}
	if(((dir == UpVector)||(dir == ZeroVector))||((dpos == UpVector)||(dpos == ZeroVector))){
		return;
	}else{
		int x = int(dir.x/xSize);
		int y = int(dir.z/ySize);
		if(adg == 0){
			sector[x][y].ebuildings += damage*dpos.distance2D(dir);
		}else{
			if((adg->movedata == 0)&&(adg->canfly == false)){
				sector[x][y].ebuildings += damage*dpos.distance2D(dir);
			}else{
				sector[x][y].ground_threat += damage*dpos.distance2D(dir);
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*void AckInsert(Ins* ik){
	/*while(ik->G->Ch->lock == true){
		// keep looping till the lock == false which means no other threads are acessing it.
		// note: this is just untill I figure out how to do proper threadsafe stuff
	}* /
	ik->G->Ch->lock = true;
	ik->G->Ch->groups.push_back(ik->i);
	ik->G->Ch->lock = false;
	ik->G->Ch->FindTarget(&ik->G->Ch->groups.back());
	delete ik;
}*/

void Chaser::UnitFinished(int unit){
	NLOG(_T("Chaser::UnitFinished"));
	NO_GAIA
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		G->L.print(_T("UnitDef call filled, was this unit blown up as soon as it was created?"));
		return;
	}
	
	bool atk = false;
	if(G->dynamic_selection == false){
		vector<string> v;
		v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG(_T("AI\\Attackers")));
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(*vi == ud->name){
					atk = true;
					break;
				}
			}
		}
	}else{
		// determine dynamically if this is an attacker
		if(((ud->weapons.empty() == false)&&((ud->movedata)||(ud->canfly == true)))&&((((ud->speed > G->scout_speed)&&(ud->canfly == false))||((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->builder == false)&&(ud->isCommander == false))) == false)&&(ud->transportCapacity == 0)) atk = true;
	}
	if(atk == true){
		//G->L.iprint("atk true");
		Add(unit);
		G->L.print(_T("new attacker added :: ") + ud->name);
		if(Attackers.size()>threshold){
			ackforce ac(G);
			ac.marching = false;
			ac.idle = true;
			if((G->mexfirst == true)&&(groups.empty() == true)) ac.type = a_mexraid;
			ac.i = 1;
			forces++;
			ac.gid = forces;
			ac.units = Attackers;
			groups.push_back(ac);
			FindTarget(&groups.back(),false);
			//_beginthread((void (__cdecl *)(void *))AckInsert, 0, (void*)ik);
			Attackers.erase(Attackers.begin(),Attackers.end());
			Attackers.clear();
		} else {
			Attackers.insert(unit);
			//if(G->base_positions.size()>1){
			//	TCommand TCS(tc,_T("chaser::unitfinished cmd_patrol"));
			//	tc.c.id = CMD_PATROL;
			//	tc.c.timeOut = G->cb->GetCurrentFrame() + 5 MINUTES;
			//	float3 npos;
			//
			//	srand(time(NULL)+G->team+G->randadd);
			//	G->randadd++;
			//	int j = rand()%(G->base_positions.size()-1);
			//	npos = G->base_positions.at(j);
			//	tc.c.params.push_back(npos.x);
			//	tc.c.params.push_back(npos.y);
			//	tc.c.params.push_back(npos.z);
			//	tc.unit = unit;
			//	tc.Priority = tc_low;
			//	tc.created = G->cb->GetCurrentFrame();
			//	G->GiveOrder(tc,false);
			//}
		}
	}
	NLOG(_T("kamikaze"));
	if(sd_proxim.empty() == false){
		string sh = G->lowercase(ud->name);
		if(sd_proxim.find(sh) != sd_proxim.end()){
			kamikaze_units.insert(unit);
		}
	}
	NLOG(_T("stockpile/dgun"));
	if(ud->weapons.empty() == false){
		bool dguner = false;
		if(can_dgun.find(ud->name) != can_dgun.end()) dguner = true;
		bool stockpiler = false;
		for(vector<UnitDef::UnitDefWeapon>::const_iterator i = ud->weapons.begin(); i!= ud->weapons.end(); ++i){
			if((dguner == false)&&(i->def->manualfire == true)){
				can_dgun[ud->name] = true;
				dguner = true;
			}
			if((stockpiler== false)&&(i->def->stockpile == true)){
				TCommand TCS(tc,_T("chaser::unitfinished stockpile"));
				tc.unit = unit;
				tc.clear = false;
				tc.Priority = tc_low;
				tc.c.timeOut = G->cb->GetCurrentFrame() + 20 MINUTES;
    			tc.c.id = CMD_STOCKPILE;
				for(int i = 0;i<110;i++){
					G->GiveOrder(tc);
				}
				sweap.insert(unit);
				stockpiler = true;
			}
			if((stockpiler == true)&&(dguner == true)){
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::~Chaser(){
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyDestroyed(int enemy){
	NLOG(_T("Chaser::EnemyDestroyed"));
	NO_GAIA
	if(enemy < 1) return;
	float3 dir = G->cb->GetUnitPos(enemy);
	if((dir == ZeroVector)||(dir == UpVector)){
		return;
	}else{
		int x = int(dir.x/xSize);
		int y = int(dir.z/ySize);
		const UnitDef* def = G->cb->GetUnitDef(enemy);
		if(def == 0){
			if(sector[x][y].ebuildings <10) return;
			sector[x][y].ebuildings -= 120;
			if(sector[x][y].ebuildings <1) sector[x][y].ebuildings = 1;
		}else{
			if(def->extractsMetal > 0) G->M->EnemyExtractorDead(dir,enemy);
			if((def->movedata == 0)&&(def->canfly == false)){
				sector[x][y].ebuildings -= G->GetEfficiency(def->name);
				if(sector[x][y].ebuildings <1) sector[x][y].ebuildings = 1;
			}else{
				sector[x][y].ground_threat -= G->GetEfficiency(def->name);
				if(sector[x][y].ground_threat <1) sector[x][y].ground_threat = 1;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void T_FindTarget(T_Targetting* T){
	//NLOG("Chaser::T_FindTarget");
	T->G->Ch->lock = true;
	if(T->i->units.empty() == true){
		T->i->idle =  true;
		//T->G->Ch->lock = false;
		delete T;
		return;
	}
	//map< int, map<int,agrid> > sec = T->G->Ch->sector;
	float3 final_dest = UpVector;
	if(T->i->type == a_mexraid){
		final_dest = T->G->Sc->GetMexScout(T->i->i);
		T->i->i++;
		if((final_dest == UpVector)||(final_dest == ZeroVector)){
			//T->G->Ch->lock = false;
			delete T;
			return;
			//_endthread();
		}
	}else{
		float best_rating , my_rating, best_ground, my_ground;
		best_rating = my_rating = best_ground = my_ground = 0;
		float3 dest = UpVector;
		float3 ground_dest = UpVector;
		float sx1 = 0,sy1 = 0;
		float sx3 = 0,sy3 = 0;

		// TODO: improve destination sector selection
		for(int x = 0; x < T->G->Ch->xSectors; x++){
			for(int y = 0; y < T->G->Ch->ySectors; y++){
				my_rating =  T->G->Ch->sector[x][y].ebuildings / (T->G->Ch->sector[x][y].centre.distance(T->G->nbasepos(T->G->Ch->sector[x][y].centre))/10) ;
				if(my_rating > best_rating){
					if(T->i->starg != T->G->Ch->sector[x][y].centre){
						dest = T->G->Ch->sector[x][y].centre;
						best_rating = my_rating;
						sx1 = x;
						sy1 = y;
					}
				}
				my_ground =  T->G->Ch->sector[x][y].ground_threat / (T->G->Ch->sector[x][y].centre.distance(T->G->nbasepos(T->G->Ch->sector[x][y].centre))/10);
				if(my_ground > best_ground){
					if(T->i->starg != T->G->Ch->sector[x][y].centre){
						ground_dest= T->G->Ch->sector[x][y].centre;
						best_ground = my_ground;
						sx3 = x;
						sy3 = y;
					}
				}
			}
		}

		////
		if((best_ground <10)&& ( 10 >best_rating)){
			delete T;
			return;
		}
		
		if(best_rating>best_ground/1.5f){
			final_dest = dest;
			if(final_dest == UpVector){
				T->G->Ch->lock = false;
				T->G->L.print(_T("find target exiting on upvector target"));
				delete T;
				return;
				//_endthread();
			}
			T->i->starg.z = sy1;
		}else{
			final_dest = ground_dest;
			if(final_dest == UpVector){
				T->G->Ch->lock = false;
				T->G->L.print(_T("find target exiting on up vector target"));
				delete T;
				return;
				//_endthread();
			}
			//T->i->starg.x = sx3;
			//T->i->starg.z = sy3;
		}
	}
	if(final_dest != UpVector){
		TCommand TCS(tc,_T("chaser::_TFindTarget attacking move.area_attack"));
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		if(T->i->type == a_mexraid){
			tc.c.id = CMD_AREA_ATTACK;
		}else {
			tc.c.id = CMD_MOVE;
		}
		tc.c.params.push_back(final_dest.x);
		tc.c.params.push_back(T->G->cb->GetElevation(final_dest.x,final_dest.z));
		tc.c.params.push_back(final_dest.z);
		tc.created = T->G->cb->GetCurrentFrame();
		tc.c.timeOut = tc.created + 10 MINUTES;
		if(T->i->type == a_mexraid) tc.c.params.push_back(300);
		if(T->i->units.empty() == false){
			for(set<int>::const_iterator aik = T->i->units.begin();aik != T->i->units.end();++aik){
				if(T->G->cb->GetUnitHealth(*aik) >1){
					tc.unit = *aik;
					T->G->GiveOrder(tc);
				}
			}
		}
		if(T->upthresh == true){
			T->G->Ch->threshold++;
			char c[10];
			sprintf(c,"%i",T->G->Ch->threshold);
			T->G->L.print(string(_T("new threshold :: ")) + c);
		}
		T->i->marching = true;
		T->i->idle = false;
	}
	T->G->Ch->lock = false;
	delete T;
	return;
	//_endthread();
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Chaser::FindTarget(ackforce* i, bool upthresh){
	NLOG(_T("Chaser::FindTarget"));
	NO_GAIA
	T_Targetting* T = new T_Targetting;
	T->G = G;
	T->i = i;
	T->upthresh = upthresh;
	T_FindTarget(T);
	//_beginthread((void (__cdecl *)(void *))T_FindTarget, 0, (void*)T);
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitIdle(int unit){
	NLOG(_T("Chaser::UnitIdle"));
	NO_GAIA
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0) return;
	if(ud->isCommander == true) return;
	if(ud->builder == true) return;
	if(ud->weapons.empty() == true) return;
	if(G->enemies.empty() == true) return;
	int* en = new int[5000];
	int e = G->GetEnemyUnits(en,G->cb->GetUnitPos(unit),G->cb->GetUnitMaxRange(unit)*2.6f);
	if(e>0){
		float best_score = 0;
		int best_target = 0;
		float tempscore = 0;
		bool mobile = true;
		for(int i = 0; i < e; i++){
			if(en[i] < 1) continue;
			const UnitDef* endi = G->cb->GetUnitDef(en[i]);
			if(endi == 0){
				continue;
			}else{
				bool tmobile = false;
				if(G->hardtarget == false){
					tempscore = G->GetEfficiency(endi->name);
				}else{
					string fh = ud->name + _T("\\target_weights\\") + endi->name;
					string fg = ud->name + _T("\\target_weights\\undefined");
					string tempdef = G->mod_tdf->SGetValueDef("20", fg.c_str());
					tempscore = atof(G->mod_tdf->SGetValueDef(tempdef.c_str(), fh.c_str()).c_str()); // load "unitname\\target_weights\\enemyunitname" to retirieve targetting weights
				}
				if((endi->movedata != 0)||(endi->canfly)){
					if(G->hardtarget == false) tempscore = tempscore/4;
					tmobile = true;
				}
				if(tempscore > best_score){
					best_score = tempscore;
					best_target = en[i];
					mobile = tmobile;
				}
				tempscore = 0;
			}
		}
		if(ud->highTrajectoryType == 2){
			TCommand TCS(tc,_T("Trajectory chaser::unitidle best target traj"));
			tc.c.timeOut = 400+(G->cb->GetCurrentFrame()%600) + G->cb->GetCurrentFrame();
			tc.Priority = tc_low;
			tc.unit = unit;
			tc.c.id = CMD_TRAJECTORY;
			if(mobile == false){
				tc.c.params.push_back(1.0f);
			} else{
				tc.c.params.push_back(0.0f);
			}
			G->GiveOrder(tc);
		}
		if(best_target > 0){
			TCommand TCS(tc1,_T("chaser::unitidle :: best target ATTACK"));
			tc1.unit = unit;
			tc1.Priority = tc_assignment;
			tc1.c.id = CMD_ATTACK;
			tc1.c.params.push_back(best_target);
			if(tc1.c.params.empty() == true){
				G->L.print(_T("empty parameters for command that should have 1 item inside it"));
			}else{
				tc1.created = G->cb->GetCurrentFrame();
				tc1.c.timeOut = 10 SECONDS + G->cb->GetCurrentFrame();
				G->GiveOrder(tc1,false);
			}
		}
		delete [] en;
		return;
	}else{
        delete [] en;
		/* find attack group*/
		if(this->groups.empty() == false){
			bool found = false;
			for(vector<ackforce>::iterator si = groups.begin(); si != groups.end(); ++si){
				if((si->marching == true)&&(si->units.empty() == false)){
					for(set<int>::iterator sd = si->units.begin(); sd != si->units.end(); ++sd){
						if(*sd == unit){
							FindTarget(&*si,false);
							found = true;
							break;
						}
					}
				}
				if(found == true) break;
			}
		}
	}
	/*if(groups.empty() == false){
        for(vector<ackforce>::iterator at = groups.begin();at !=groups.end();++at){
			for(set<int>::iterator aa = at->units.begin(); aa != at->units.end();++aa){
				if(*aa == unit){
					if(at->idle == false){
						G->L.print("findtarget UnitIdle");
						FindTarget(&*at);
					}
					return;
				}
			}
		}
	}*/
	int* ten = new int[5000];
	int f = G->GetEnemyUnits(en,G->cb->GetUnitPos(unit),G->cb->GetUnitMaxRange(unit)*4);
	if((f>0)&&(ten[0] > 0)){
		float best_score = 0;
		int best_target = 0;
		float tempscore = 0;
		bool mobile = true;
		for(int i = 0; i < e; i++){
			if(en[i] < 1) continue;
			const UnitDef* endi = G->cb->GetUnitDef(en[i]);
			if(endi == 0){
				continue;
			}else{
				bool tmobile = false;
				if(G->hardtarget == false){
					tempscore = G->GetEfficiency(endi->name);
				}else{
					string fh = ud->name + _T("\\TARGET_WEIGHTS\\") + endi->name;
					string fg = ud->name + _T("\\TARGET_WEIGHTS\\undefined");
					string tempdef = G->mod_tdf->SGetValueDef("20", fg.c_str());
					tempscore = atof(G->mod_tdf->SGetValueDef(tempdef.c_str(), fh.c_str()).c_str()); // load "unitname\\target_weights\\enemyunitname" to retirieve targetting weights
				}
				if((endi->movedata != 0)||(endi->canfly)){
					if(G->hardtarget == false) tempscore = tempscore/4;
					tmobile = true;
				}
				if(tempscore > best_score){
					best_score = tempscore;
					best_target = en[i];
					mobile = tmobile;
				}
				tempscore = 0;
			}
		}
		TCommand TCS(tc,_T("chaser::unitidle :: idle ATTACK"));
		tc.unit = unit;
		tc.Priority = tc_assignment;
		tc.c.id = CMD_ATTACK;
		tc.c.params.push_back(best_target);
		if(tc.c.params.empty() == true){
			G->L.print(_T("empty parameters for command that should have 1 item inside it"));
		}else{
			tc.c.timeOut = 600 + G->cb->GetCurrentFrame();
			tc.created =G->cb->GetCurrentFrame();
			G->GiveOrder(tc);
		}
	}
	delete [] ten;
	return;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Fire units in the sweap array, such as nuke silos etc.

void Chaser::FireSilos(){
	NLOG(_T("Chaser::FireSilos"));
	NO_GAIA

	int fnum = G->cb->GetCurrentFrame(); // Get frame
	if((fnum%200 == 0)&&(sweap.empty() == false)){ // Every 30 seconds
		float best_rating = 0;
		float my_rating = 0; // Search for the area with the highest threat level.
		for(int x = 0; x < xSectors; x++){
			for(int y = 0; y < ySectors; y++){
				my_rating = sector[x][y].ebuildings;
				if(my_rating > best_rating){
					swtarget= sector[x][y].centre;
					best_rating = my_rating;
				}
			}
		}
		TCommand TCS(tc,_T("chaser::firesilos attack"));
		tc.Priority = tc_pseudoinstant;
		tc.c.id = CMD_ATTACK; // Attack chosen area.
		tc.c.params.push_back(swtarget.x);
		tc.c.params.push_back(swtarget.y);
		tc.c.params.push_back(swtarget.z);
		tc.c.params.push_back(max(xSize,ySize));
		for(set<int>::iterator si = sweap.begin(); si != sweap.end();++si){
			tc.unit = *si;
			G->GiveOrder(tc);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#define TGA_RES 8

void ThreatTGA(Global* G){
	NLOG(_T("ThreatTGA"));
	// open file
	char c [4];
	sprintf(c,"%i",G->team);
	string filename = string(_T("threatmatrix")) + c + string(_T(".tga"));
	FILE *fp=fopen(filename.c_str(), "wb");
	//#ifdef _MSC_VER
 	//	if(!fp)	_endthread();
	//#else
		if(!fp) return;
	//#endif
	map< int, map<int,agrid> > sec = G->Ch->sector;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));
	float biggestebuild = 1;
	float biggestheight = 1;
	float biggestground = 1;
	for(int xa = 0; xa < G->Ch->xSectors; xa++){
		for(int ya = 0; ya < G->Ch->ySectors; ya++){
			if(sec[xa][ya].ebuildings / (sec[xa][ya].centre.distance(G->nbasepos(sec[xa][ya].centre))/4) > biggestebuild)	biggestebuild = sec[xa][ya].ebuildings / (sec[xa][ya].centre.distance(G->nbasepos(sec[xa][ya].centre))/4);
			if(sec[xa][ya].ground_threat / (sec[xa][ya].centre.distance(G->nbasepos(sec[xa][ya].centre))/4) > biggestground) biggestground = sec[xa][ya].ground_threat / (sec[xa][ya].centre.distance(G->nbasepos(sec[xa][ya].centre))/4);
			float g = G->cb->GetElevation(sec[xa][ya].centre.x,sec[xa][ya].centre.z);
			if(g > biggestheight) biggestheight = g;
		}
	}

	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
	targaheader[12] = (char) (G->Ch->xSectors*TGA_RES & 0xFF);
	targaheader[13] = (char) (G->Ch->xSectors*TGA_RES >> 8);
	targaheader[14] = (char) (G->Ch->ySectors*TGA_RES & 0xFF);
	targaheader[15] = (char) (G->Ch->ySectors*TGA_RES >> 8);
	targaheader[16] = 24; /* 24 bit color */
	targaheader[17] = 0x20;	/* Top-down, non-interlaced */

	fwrite(targaheader, 18, 1, fp);

	// write file
	uchar out[3];
	float heightp = (biggestheight)/320;
	float ebuildp = (biggestebuild)/320;
	float groundp = (biggestground)/320;
	if(groundp <= 0.0f) groundp = 1;
	if(ebuildp <= 0.0f) ebuildp = 1;
	if(heightp <= 0.0f) heightp = 1;
	for (int z=0;z<G->Ch->ySectors*TGA_RES;z++){
		for (int x=0;x<G->Ch->xSectors*TGA_RES;x++){
			const int div=4;
			int x1 = x;
			int z1 = z;
			if(z%TGA_RES != 0) z1 = z-(z%TGA_RES); z1 = z1/TGA_RES;
			if(x%TGA_RES != 0) x1 = x-(x%TGA_RES); x1 = x1/TGA_RES;
			out[0]=(uchar)min(255, (int)max(1.0f,sec[x1][z1].centre.y/heightp));//Blue Heightmap
			out[1]=(uchar)min(255,(int)max(1.0f,(sec[x1][z1].ground_threat / (sec[x1][z1].centre.distance(G->nbasepos(sec[x1][z1].centre))/4)/groundp) / div));//Green Ground threat
			out[2]=(uchar)min(255,(int)max(1.0f,(sec[x1][z1].ebuildings / (sec[x1][z1].centre.distance(G->nbasepos(sec[x1][z1].centre))/4)/ebuildp)/div));//Red Building threat

			fwrite (out, 3, 1, fp);
		}
	}
	fclose(fp);
	#ifdef _MSC_VER
	system(filename.c_str());
	//_endthread();
	#endif
	return;
}
void Chaser::MakeTGA(){
	NLOG(_T("Chaser::MakeTGA"));
	//#ifdef _MSC_VER
 	//_beginthread((void (__cdecl *)(void *))ThreatTGA, 0, (void*)G);
	//#else
	ThreatTGA(G);
	//#endif
 }

void Chaser::Update(){
	NLOG(_T("Chaser::Update"));
	NO_GAIA
	// decrease threat values of all sectors..
	sector[0][0].ground_threat = sector[0][0].ground_threat  * 0.99f;
	if(EVERY_((3 SECONDS))){
		NLOG(_T("Chaser::Update :: threat matrix update friendly units"));
		int* funits = new int[5000];
		int fu = G->cb->GetFriendlyUnits(funits);
		if(fu > 0){
			lock = true;
			for(int f = 0; f <fu; f++){
				const UnitDef* urd = G->cb->GetUnitDef(funits[f]);
				if(urd != 0){
					float3 fupos = G->cb->GetUnitPos(funits[f]);
					if((fupos == UpVector)||(fupos == ZeroVector)){
						continue;
					}else{
						float3 fusec = G->WhichSector(fupos);
						if(fusec == UpVector) continue;
						int z = min(ySectors,(int)fusec.z-1);
						int x = min(xSectors,(int)fusec.x-1);
						float gt = sector[x][z].ground_threat - G->GetEfficiency(urd->name);
						sector[x][z].ground_threat = max(10.0f,gt);
						gt = sector[x][z].ebuildings - (G->GetEfficiency(urd->name)/2);
						sector[x][z].ebuildings = max(10.0f,gt);
					}
				}
			}
			lock = false;
		}
		delete [] funits;
	}
	// Run the code that handles stockpiled weapons.
	if(EVERY_((10 SECONDS))){
		if((sweap.empty() == false)&&(G->enemies.empty() == false)){
			NLOG(_T("Chaser::Update :: firing silos"));
			FireSilos();
		}
	}
	if(EVERY_((3 SECONDS))){
		if((kamikaze_units.empty() == false)&&(G->enemies.empty() == false)){
			for(set<int>::iterator i = kamikaze_units.begin(); i != kamikaze_units.end(); ++i){
				int* funits = new int [5000];
				int fu = G->GetEnemyUnits(funits,G->cb->GetUnitPos(*i),G->cb->GetUnitMaxRange(*i));
				if(fu > 0 ){
					TCommand TCS(tc,_T("chaser::update selfd"));
					tc.unit = *i;
					tc.Priority = tc_instant;
					tc.c.id = CMD_SELFD;
					tc.created = G->cb->GetCurrentFrame();
					tc.delay= 0;
					G->GiveOrder(tc);
				}
				delete [] funits;
			}
		}
	}
	if((EVERY_(4 FRAMES))&&(G->enemies.empty() == false)){
		NLOG(_T("Chaser::Update :: update threat matrix enemy targets"));
		// calculate new threads
		float3 pos = UpVector;
		int x=0;
		int y = 0; 

		// get all enemies in los
		int* en = new int[5000];
		//for(int i = 0; i<4999; i++){
		//	en[i] = 0;
		//}
		int unum = G->GetEnemyUnits(en);

		// go through the list 
		if(unum > 0){
			for(int i = 0; i < unum; i++){
				if(en[i] < 1){
					continue;
				}else{
					pos = G->cb->GetUnitPos(en[i]);
					if(pos == UpVector) continue;
					if(pos == ZeroVector)continue;
					if(pos.x <0) continue;
					if(pos.z < 0)continue;
					x = min(int(pos.x/xSize),xSectors-1);
					y = min(ySectors-1,int(pos.z/ySize));
					const UnitDef* def = G->cb->GetUnitDef(en[i]);
					if(def == 0){
						sector[x][y].ebuildings = sector[x][y].ebuildings + 600;
						continue;
					}else{
						if(def->extractsMetal > 0.0f) G->M->EnemyExtractor(pos,en[i]);
						float ef = G->GetEfficiency(def->name);
						if((def->movedata == 0)&&(def->canfly == false)){ // its a building
							sector[x][y].ebuildings = sector[x][y].ebuildings + ef;
						}else{
							sector[x][y].ground_threat = sector[x][y].ground_threat + ef;
						}
					}
					x=0;
					y=0;
				}
			}
		}
		delete [] en;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// re-evaluate the target of an attack force
	if(EVERY_(519/*17 SECONDS*/)){
		NLOG(_T("Chaser::Update :: re-evaluate attack targets"));
		if(groups.empty() == false){
			for(vector<ackforce>::iterator af = groups.begin();af != groups.end();++af){
				if(af->units.empty() == false) FindTarget(&*af,false);
			}
		}
		NLOG(_T("Chaser::Update :: re-evaluate attack targets done"));
	}
	if(EVERY_((4 SECONDS))&&(G->enemies.empty() == false)){
		NLOG(_T("Chaser::Update :: make attackers attack nearby targets"));
		if(this->Attackers.empty() == false){
			for(set<int>::iterator aa = Attackers.begin(); aa != Attackers.end();++aa){
				int* en = new int[5000];
				int e = G->GetEnemyUnits(en,G->cb->GetUnitPos(*aa),G->cb->GetUnitMaxRange(*aa)*2.6f);
				if(e>0){
					TCommand TCS(tc,_T("cmd_attack chaser::update every 4 secs"));
					tc.c.id = CMD_ATTACK;
					tc.c.params.push_back(en[0]);
					if (tc.c.params.empty() == true){
						delete [] en;
						continue;
					}
					tc.c.timeOut = 10 SECONDS+(G->cb->GetCurrentFrame()%300) + G->cb->GetCurrentFrame();
					tc.clear = false;
					tc.Priority = tc_instant;
					tc.unit = *aa;
					G->GiveOrder(tc);
					tc.c.params.erase(tc.c.params.begin(),tc.c.params.end());
					tc.c.params.clear();
					tc.c.id = CMD_TRAJECTORY;
					#ifdef TC_SOURCE
						tc.source = _T("cmd_traj chaser::update every  secs");
					#endif
					tc.c.params.push_back(2);
					const UnitDef* guy = G->cb->GetUnitDef(*aa);
					if(guy != 0 ){
						if(guy->highTrajectoryType == 2){
							const UnitDef* enmy = G->cb->GetUnitDef(en[0]);
							if(enmy != 0){
								if((enmy->canfly == false)&&(enmy->movedata == 0)){ // it must therefore be a building
									tc.c.params.push_back(1);
									G->GiveOrder(tc);
								} else{
									tc.c.params.push_back(0);
									G->GiveOrder(tc,false);
								}
							}
						}
					}
				}
				delete [] en;
			}
		}
	}
	NLOG(_T("Chaser::Update :: DONE"));
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

