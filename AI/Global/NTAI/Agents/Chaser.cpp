#include "../helper.h"


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


void Chaser::InitAI(Global* GLI){
	//NLOG("Chaser::InitAI");
	G = GLI;

	// calculate number of sectors
	xSectors = int(floor(0.5 + ( G->cb->GetMapWidth())/20));
	ySectors = int(floor(0.5 + ( G->cb->GetMapHeight())/20));

	// calculate effective sector size
	xSizeMap = int(G->cb->GetMapWidth() / xSectors);
	ySizeMap = int(G->cb->GetMapHeight() /ySectors); 
	xSize = 8 * xSizeMap;
	ySize = 8 * ySizeMap;
	//if(sec_set == false){
	// create field of sectors
	for(int i = 0; i < xSectors; i++){

		for(int j = 0; j < ySectors; j++){
			// set coordinates of the sectors
			sector[i][j].centre = float3(((xSize*i)+(xSize*(i+1)))/2,0, ((ySize * j)+(ySize * (j+1)))/2 );
			sector[i][j].location = float3(i,0,j);
			sector[i][j].ground_threat = 1;
			sector[i][j].ebuildings = 1;
		}
	}
	/*for(vector<float3>::iterator ij = G->Sc->start_pos.begin(); ij != G->Sc->start_pos.end(); ++ij){
		int x = int(ij->x/xSize);
		int y = int(ij->z/ySize);
		sector[x][y].ebuildings += 100;
	}*/
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Add(int unit){
	G->L.print("Chaser::Add()");
	Attackers.insert(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDestroyed(int unit){
	NLOG("Chaser::UnitDestroyed");
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
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	NLOG("Chaser::UnitDamaged");
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
	if(attacker <0) return;
	if(G->cb->GetUnitAllyTeam(attacker) == G->unitallyteam) return;
	float3 dpos = G->cb->GetUnitPos(damaged);
	TCommand tc;
	tc.unit = damaged;
	tc.clear = false;
	tc.Priority = tc_instant;
	if(ud->weapons.empty() == false){
		for(vector<UnitDef::UnitDefWeapon>::const_iterator wu = ud->weapons.begin();wu !=ud->weapons.end();++wu){
			if(wu->def->manualfire == true){
			//	if(dpos.distance(dir) < wu->def->range*2){
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
				//}
			}
		}
	}
	if(((dir == UpVector)||(dir == ZeroVector))||((dpos == UpVector)||(dpos == ZeroVector))){
		return;
	}else{
		int x = int(dir.x/xSize);
		int y = int(dir.z/ySize);
		if(adg == 0){
			sector[x][y].ebuildings += (damage*dpos.distance(dir))/10;
		}else{
			if((adg->movedata == 0)&&(adg->canfly == false)){
				sector[x][y].ebuildings += (damage*dpos.distance(dir))/2;
			}else{
				sector[x][y].ground_threat += (damage*dpos.distance(dir))/2;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void AckInsert(Ins* ik){
	/*while(ik->G->Ch->lock == true){
		// keep looping till the lock == false which means no other threads are acessing it.
		// note: this is just untill I figure out how to do proper threadsafe stuff
	}*/
	ik->G->Ch->lock = true;
	ik->G->Ch->groups.push_back(ik->i);
	ik->G->Ch->lock = false;
	ik->G->Ch->FindTarget(&ik->G->Ch->groups.back());
	delete ik;
}
void Chaser::UnitFinished(int unit){
	NLOG("Chaser::UnitFinished");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		G->L.print("UnitDef call filed, was this unit blown up as soon as it was created?");
		return;
	}
	TCommand tc;
	tc.unit = unit;
	tc.clear = false;
	tc.Priority = tc_low;
	tc.c.timeOut = G->cb->GetCurrentFrame() + 20 MINUTES;
	if(ud->weapons.empty() == false){
        if(ud->weapons.front().def->stockpile == true){
    		tc.c.id = CMD_STOCKPILE;
			for(int i = 0;i<110;i++){
				G->GiveOrder(tc);
			}
			sweap.insert(unit);
		}
	}
	
	// load attackers.txt so that we can get the list of units the modder has defined as atatckers
	bool atk = false;
	// load scouters.txt so that we can get the list of units the modder has defined as scouters
	if(G->abstract == false){
		string filename;
		filename += "NTAI/";
		filename += G->cb->GetModName();
		filename += "/mod.tdf";
		ifstream fp;
		fp.open(filename.c_str(), ios::in);
		char buffer[5000];
		if(fp.is_open() == false){
			G->L.print(" error loading mod.tdf");
		}else{
			char in_char;
			int bsize = 0;
			while(fp.get(in_char)){
				buffer[bsize] = in_char;
				bsize++;
			}
			if(bsize > 0){
				CSunParser md(G);
				md.LoadBuffer(buffer,bsize);
				vector<string> v;
				v = bds::set_cont(v,md.SGetValueMSG("AI\\Attackers"));
				if(v.empty() == false){
					for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
						if(*vi == ud->name){
							atk = true;
							break;
						}
					}
				}
			}
		}
	}else{
		// determine dynamically if this is an attacker
		if(((ud->weapons.empty() == false)&&((ud->movedata)||(ud->canfly == true)))&&((((ud->speed > 70)&&(ud->canfly == false))||((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->builder == false)&&(ud->isCommander == false))) == false)&&(ud->transportCapacity == 0)) atk = true;
	}
	
	if(atk == true){
		/*if(ud->highTrajectoryType == 2){
			TCommand tc;
			tc.c.params.push_back(3);
			tc.clear = false;
			tc.Priority = tc_assignment;
			tc.unit = unit;
            tc.c.id = CMD_TRAJECTORY;
			G->GiveOrder(tc);
		}*/
		Add(unit);
		G->L.print("new attacker added :" + ud->name);
		if(this->Attackers.size() >threshold){
			/*bool idled = false;
			if((groups.empty() == false)&&(groups.back().gid != groups.front().gid)){
				for(vector<ackforce>::iterator ad = groups.begin();ad != groups.end();++ad){
					if(ad->idle){
						ad->acknum += acknum;
						for(set<int>::iterator acf = Attackers.begin();acf != Attackers.end();++acf){
							ad->units.insert(*acf);
						}
						idled = true;
						Attackers.clear();
						acknum = 0;
						FindTarget(&*ad);
						break;
					}
				}
			}
			if(idled == false){*/
                ackforce ac(G);
				ac.marching = false;
				ac.idle = true;
				if(groups.empty() == true) ac.type = a_mexraid;
				ac.i = 1;
				forces++;
				ac.gid = forces;
				ac.units = Attackers;
				G->Ch->lock = true;
				G->Ch->groups.push_back(ac);
				G->Ch->lock = false;
				G->Ch->FindTarget(&G->Ch->groups.back());
				//_beginthread((void (__cdecl *)(void *))AckInsert, 0, (void*)ik);
				Attackers.erase(Attackers.begin(),Attackers.end());
				Attackers.clear();
			//}
		} else {
			Attackers.insert(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::~Chaser(){
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyDestroyed(int enemy){
	NLOG("Chaser::EnemyDestroyed");
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
		} else{
			if((def->movedata == 0)&&(def->canfly == false)){
				if(sector[x][y].ebuildings <5) return;
				sector[x][y].ebuildings -= ((def->metalCost+def->energyCost+0.1f)/2)+def->energyMake+((def->extractsMetal+0.1f)*30)+def->buildSpeed;
				if(sector[x][y].ebuildings <1) sector[x][y].ebuildings = 1;
			}else{
				if(sector[x][y].ground_threat <10) return;
				sector[x][y].ground_threat -= def->power*4;
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
		T->G->Ch->lock = false;
		delete T;
		return;
	}
	map< int, map<int,agrid> > sec = T->G->Ch->sector;
	float3 final_dest = ZeroVector;
	if(T->i->type == a_mexraid){
		final_dest = T->G->Sc->GetMexScout(T->i->i);
		T->i->i++;
		if((final_dest == UpVector)||(final_dest == ZeroVector)){
			T->G->Ch->lock = false;
			delete T;
			return;
			//_endthread();
		}
	}else{
		float best_rating , my_rating, best_ground, my_ground;
		best_rating = my_rating = best_ground = my_ground = 0;
		float3 dest = ZeroVector;
		float3 ground_dest = ZeroVector;
		float sx1,sy1 = 0;
		float sx3,sy3 = 0;

		// TODO: improve destination sector selection
		for(int x = 0; x < T->G->Ch->xSectors; x++){
			for(int y = 0; y < T->G->Ch->ySectors; y++){
				my_rating =  sec[x][y].ebuildings / sec[x][y].centre.distance(T->G->nbasepos(sec[x][y].centre)) ;
				if(my_rating > best_rating){
					if(T->i->starg != sec[x][y].centre){
						dest = sec[x][y].centre;
						best_rating = my_rating;
						sx1 = x;
						sy1 = y;
					}
				}
				my_ground =  sec[x][y].ground_threat /sec[x][y].centre.distance(T->G->nbasepos(sec[x][y].centre));
				if(my_ground > best_ground){
					if(T->i->starg != sec[x][y].centre){
						ground_dest= sec[x][y].centre;
						best_ground = my_ground;
						sx3 = x;
						sy3 = y;
					}
				}
			}
		}

		////
		if((best_rating<20)&&(best_ground<15)){
			T->G->Ch->lock = false;
			delete T;
			return;
			//_endthread();
		}
		
		
		if(best_rating>best_ground/1.5){
			final_dest = dest;
			if(final_dest == ZeroVector){
				T->G->Ch->lock = false;
				delete T;
				return;
				//_endthread();
			}
			T->i->starg.z = sy1;
		}else{
			final_dest = ground_dest;
			if(final_dest == ZeroVector){
				T->G->Ch->lock = false;
				delete T;
				return;
				//_endthread();
			}
			T->i->starg.x = sx3;
			T->i->starg.z = sy3;
		}
	}
	if(final_dest != ZeroVector){
		TCommand tc;
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		if(T->i->type == a_mexraid) tc.c.id = CMD_AREA_ATTACK; else  tc.c.id = CMD_MOVE;
		tc.c.params.push_back(final_dest.x);
		tc.c.params.push_back(T->G->cb->GetElevation(final_dest.x,final_dest.z));
		tc.c.params.push_back(final_dest.z);
		tc.created = T->G->cb->GetCurrentFrame();
		tc.c.timeOut = tc.created + 10 MINUTES;
		if(T->i->type == a_mexraid) tc.c.params.push_back(300);
		/*float3 zpositions = final_dest;
		zpositions.y += 400;
		G->Pl->DrawLine(zpositions,final_dest,60,10,true);*/
		if(T->i->units.empty() == false){
			for(set<int>::const_iterator aik = T->i->units.begin();aik != T->i->units.end();++aik){
				if(T->G->cb->GetUnitHealth(*aik) >1){
					tc.unit = *aik;
					T->G->GiveOrder(tc);
				}
			}
		}
		if(T->upthresh == true)	T->G->Ch->threshold++;
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
	NLOG("Chaser::FindTarget");
	T_Targetting* T = new T_Targetting;
	T->G = G;
	T->i = i;
	T->upthresh = upthresh;
	T_FindTarget(T);
	//_beginthread((void (__cdecl *)(void *))T_FindTarget, 0, (void*)T);
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitIdle(int unit){
	NLOG("Chaser::UnitIdle");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0) return;
	int* en = new int[5000];
	int e = G->GetEnemyUnits(en,G->cb->GetUnitPos(unit),G->cb->GetUnitMaxRange(unit)*2.6f);
	if(e>0){
		TCommand tc;
		tc.c.timeOut = 400+(G->cb->GetCurrentFrame()%600) + G->cb->GetCurrentFrame();
		tc.clear = false;
		tc.Priority = tc_low;
		tc.unit = unit;
		tc.c.id = CMD_TRAJECTORY;
		tc.c.params.push_back(2.0f);
		TCommand tc1;
		tc1.unit = unit;
		tc1.clear = false;
		tc1.Priority = tc_pseudoinstant;
		tc1.c.id = CMD_ATTACK;
		
		float best_score = 0;
		int best_target = en[0];
		float tempscore = 0;
		bool mobile = true;
		for(int i = 0; i < e; i++){
			const UnitDef* endi = G->cb->GetUnitDef(en[i]);
			if(endi == 0){
				continue;
			}else{
				bool tmobile = false;
				tempscore = (endi->energyMake*4)+(endi->metalMake*4)+(endi->metalStorage*2+endi->energyStorage*2)+((endi->radarRadius+endi->sonarRadius+endi->sonarJamRadius+endi->jammerRadius)*5)-(endi->isMetalMaker*200)-(endi->weapons.size()*100)+(ud->builder*400)-(endi->targfac*60);
				if((endi->movedata != 0)||(endi->canfly)){
					tempscore = tempscore/4;
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
		const UnitDef* guy = G->cb->GetUnitDef(unit);
		if(guy != 0 ){
			if(guy->highTrajectoryType == 2){
				if(mobile == false){
					tc.c.params.push_back(1.0f);
					G->GiveOrder(tc);
				} else{
					tc.c.params.push_back(0.0f);
					G->GiveOrder(tc);
				}
			}
		}
		tc1.c.params.push_back(best_target);
		tc1.c.timeOut = 10 SECONDS + G->cb->GetCurrentFrame();
		G->GiveOrder(tc1);
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
	if(f>0){
		TCommand tc;
		tc.unit = unit;
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		tc.c.id = CMD_ATTACK;
		tc.c.params.push_back(ten[0]);
		tc.c.timeOut = 600 + G->cb->GetCurrentFrame();
		G->GiveOrder(tc);
		delete [] ten;
		return;
	}else{
		delete [] ten;
		return;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Fire units in the sweap array, such as nuke silos etc.

void Chaser::FireSilos(){
	NLOG("Chaser::FireSilos");
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
		TCommand tc;
		tc.clear = false;
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
	NLOG("ThreatTGA");
	// open file
	char c [4];
	sprintf(c,"%i",G->team);
	string filename = string("threatmatrix") + c + string(".tga");
	FILE *fp=fopen(filename.c_str(), "wb");
	#ifdef _MSC_VER
	if(!fp) _endthread();
	#else
	if(!fp) return;
	#endif
	map< int, map<int,agrid> > sec = G->Ch->sector;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));
	float biggestebuild = 1;
	float biggestheight = 1;
	float biggestground = 1;
	for(int xa = 0; xa < G->Ch->xSectors; xa++){
		for(int ya = 0; ya < G->Ch->ySectors; ya++){
			if(sec[xa][ya].ebuildings > biggestebuild)	biggestebuild = sec[xa][ya].ebuildings;
			if(sec[xa][ya].ground_threat > biggestground) biggestground = sec[xa][ya].ground_threat;
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
			out[0]=(uchar)min(255, (int)max(1.0f,G->cb->GetElevation(sec[x1][z1].centre.x, sec[x1][z1].centre.z)/heightp));//Blue Heightmap
			out[1]=(uchar)min(255,(int)max(1.0f,(sec[x1][z1].ground_threat/groundp) / div));//Green Ground threat
			out[2]=(uchar)min(255,(int)max(1.0f,(sec[x1][z1].ebuildings/ebuildp)/div));//Red Building threat

			fwrite (out, 3, 1, fp);
		}
	}
	fclose(fp);
	#ifdef _MSC_VER
	system(filename.c_str());
	_endthread();
	#endif
}
void Chaser::MakeTGA(){
	NLOG("Chaser::MakeTGA");
	#ifdef _MSC_VER
	_beginthread((void (__cdecl *)(void *))ThreatTGA, 0, (void*)G);
	#else
	ThreatTGA(G);
	#endif
}

void Chaser::Update(){
	NLOG("Chaser::Update");
	// decrease threat values of all sectors..
	if(EVERY_(45 FRAMES)){
		NLOG("Chaser::Update :: threat matrix depreciation");
        /*for(int xa = 0; xa < xSectors; xa++){
			for(int ya = 0; ya < ySectors; ya++){
				//if(sector[xa][ya].ebuildings != 0)	sector[xa][ya].ebuildings *= 0.9999999999999999999f;
				if(sector[xa][ya].ground_threat != 0)	sector[xa][ya].ground_threat *= 0.999999999999999f;
			}
		}*/
		sector[0][0].ground_threat *= 0.999f;
	
		NLOG("Chaser::Update :: threat matrix update friendly units");
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
						int x = (int)fusec.x;
						int z = (int)fusec.z;
						sector[max(0,x-1)][max(0,z-1)].ground_threat = sector[max(0,x-1)][max(z-1,0)].ground_threat-urd->power;
						if(sector[max(x-1,0)][max(0,z-1)].ground_threat <0) sector[max(x-1,0)][max(z-1,0)].ground_threat = 1;
						sector[max(0,x-1)][max(0,z-1)].ebuildings = sector[max(x-1,0)][max(0,z-1)].ebuildings-(urd->power+urd->weapons.size()*8);
						if(sector[max(0,x-1)][max(z-1,0)].ebuildings < 0) sector[max(x-1,0)][max(0,z-1)].ebuildings = 1;
					}
				}
			}
			lock = false;
		}
		delete [] funits;
	}
	// Run the code that handles stockpiled weapons.
	if(sweap.empty() == false){
		if(EVERY_(10 SECONDS)){
			NLOG("Chaser::Update :: firing silos");
			FireSilos();
		}
	}
	if(EVERY_(5 FRAMES)){
		NLOG("Chaser::Update :: update threat matrix enemy targets");
		// calculate new threads
		float3 pos = ZeroVector;
		int x, y = 0; 

		// get all enemies in los
		int* en = new int[5000];
		for(int i = 0; i<4999; i++){
			en[i] = 0;
		}
		int unum = G->GetEnemyUnits(en);

		// go through the list 
		if(unum > 0){
			for(int i = 0; i < 4999; i++){
				// if 0 end of array reached
				if(en[i] == 0){
					continue;
				}else{
					const UnitDef* def = G->cb->GetUnitDef(en[i]);
					if(def == 0){
						pos = G->cb->GetUnitPos(en[i]);
						if((pos == UpVector)||(pos == ZeroVector)){
							continue;
						}else{
							x = int(pos.x/xSize);
							y = int(pos.z/ySize);
							sector[min(x,xSectors-1)][min(ySectors-1,y)].ebuildings += 120;
							continue;
						}
					}
					pos = G->cb->GetUnitPos(en[i]);
					// if pos is one of the below then it will cause a divide by zero error and throw an exception, so we must prevent this.
					if((pos == UpVector)||(pos == ZeroVector)){
						continue;
					}else{

						// calculate in which sector unit is located
						x = int(pos.x/xSize);
						y = int(pos.z/ySize);
						x = max(0,x);
						y = max(0,x);

						// check if ground unit or ship
						if(def != 0){
							if(G->cb->UnitBeingBuilt(en[i]) == true){ // this unti is being built, which signifies the presence of a construction unit or factory, which means a base.
								sector[min(xSectors-1,x)][min(ySectors-1,y)].ebuildings += ((def->metalCost+def->energyCost+0.1f)/2)+(def->energyMake*5)+(def->metalMake*10)+((def->extractsMetal+0.1f)*30)+(def->radarRadius+def->sonarRadius+def->sonarJamRadius*2 + def->jammerRadius)+(def->buildSpeed*4);
							}else if((def->movedata != 0)||(def->canfly == true)){
								if(def->builder != 0 ){
									sector[min(xSectors-1,x)][min(ySectors-1,y)].ebuildings += ((def->metalCost+def->energyCost+0.1f)/2)+(def->energyMake*5)+(def->metalMake*10)+((def->extractsMetal+0.1f)*30)+def->buildSpeed*4+def->power;
									sector[min(xSectors-1,x)][min(ySectors-1,y)].ground_threat += ((def->metalCost+def->energyCost+0.1f)/2)+(def->energyMake*5)+((def->metalMake+0.1f)*10);
								}else{
									sector[min(xSectors-1,x)][min(ySectors-1,y)].ground_threat += ((def->metalCost+def->energyCost+0.1f)/2)+(def->energyMake*5)+((0.1f+def->metalMake)*10) + def->power;
									sector[min(xSectors-1,x)][min(ySectors-1,y)].ground_threat += def->energyMake;
									if(def->stealth == true) sector[min(xSectors-1,x)][min(ySectors-1,y)].ground_threat += def->power/4;
								}
							}else if(def->canfly == false){ // its a building
								sector[min(xSectors-1,x)][min(ySectors-1,y)].ebuildings += ((def->metalCost+def->energyCost+0.1f)/2)+def->energyMake+((def->extractsMetal+0.1f)*30)+def->buildSpeed;
							}
						}
						en[i] = 0;
					}
				}
			}
		}
		delete [] en;
	}

	
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// re-evaluate the target of an attack force
	if(EVERY_(16 SECONDS)){
		NLOG("Chaser::Update :: re-evaluate attack targets");
		if(groups.empty() == false){
			for(vector<ackforce>::iterator af = groups.begin();af != groups.end();++af){
				if(af->units.empty() == false) FindTarget(&*af,false);
			}
		}
		NLOG("Chaser::Update :: re-evaluate attack targets done");
	}
	if(EVERY_(45 FRAMES)){
		NLOG("Chaser::Update :: make attackers attack nearby targets");
		if(this->Attackers.empty() == false){
			for(set<int>::iterator aa = Attackers.begin(); aa != Attackers.end();++aa){
				int* en = new int[5000];
				int e = G->GetEnemyUnits(en,G->cb->GetUnitPos(*aa),G->cb->GetUnitMaxRange(*aa)*2.6f);
				if(e>0){
					TCommand tc;
					tc.c.id = CMD_ATTACK;
					tc.c.params.push_back(en[0]);
					tc.c.timeOut = 10 SECONDS+(G->cb->GetCurrentFrame()%300) + G->cb->GetCurrentFrame();
					tc.clear = false;
					tc.Priority = tc_instant;
					tc.unit = *aa;
					G->GiveOrder(tc);
					tc.c.params.erase(tc.c.params.begin(),tc.c.params.end());
					tc.c.params.clear();
					tc.c.id = CMD_TRAJECTORY;
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
									G->GiveOrder(tc);
								}
							}
						}
					}
				}
				delete [] en;
			}
		}
	}
	NLOG("Chaser::Update :: DONE");
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

