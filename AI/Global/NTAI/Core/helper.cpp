#include "helper.h"
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

using namespace std;

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool send_to_web;
bool get_from_web;
bool update_NTAI;


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

TdfParser* Global::Get_mod_tdf(){
	return info->mod_tdf;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Global::Global(IGlobalAICallback* callback){
	G = this;
	gcb = callback;
	cb = gcb->GetAICallback();
	if(cb == 0){
		throw string(" error cb ==0");
	}
	CLOG("Started Global::Global class constructor");
	CLOG("Starting CCached initialisation");
	Cached = new CCached;
	Cached->comID = 0;
	Cached->randadd = 0;
	Cached->enemy_number = 0;
	Cached->lastcacheupdate = 0;
	Cached->team = 567;
	CLOG("gettign team value");
	Cached->team = cb->GetMyTeam();
	
	CLOG("Creating Info class");
	info = new CInfo(G);
	
	CLOG("Setting the Logger class");
	L.Set(this);
	CLOG("Loading AI.tdf with TdfParser");
	TdfParser cs(G);
	cs.LoadFile("AI.tdf");
	CLOG("Retrieving datapath value");
	info->datapath = cs.SGetValueDef(string("AI"),"AI\\data_path");
	CLOG("Opening logfile in plaintext");
	L.Open(true);
	//L.Verbose();
	CLOG("Logging class Opened");
	L.print("logging started");
	CLOG("Loading modinfo.tdf");
	TdfParser sf(G);
	sf.LoadFile("modinfo.tdf");
	CLOG("Getting tdfpath value");
	info->tdfpath =  sf.SGetValueDef(string(cb->GetModName()),"MOD\\NTAI\\tdfpath");
	CLOG("Retrieving cheat interface");
	chcb = callback->GetCheatInterface();
	CLOG("cheat interface retrieved");
	Cached->cheating = false;
	Cached->encache = new int[6001];
	CLOG("Getting LOS pointer");
	Cached->losmap = cb->GetLosMap();
	CLOG("initialising enemy cache elements to zero");
	for(int i = 0; i< 6000; i++){
		Cached->encache[i] = 0;
	}
	CLOG("Creating Actions class");
	Actions = new CActions(G);
	CLOG("Creating Map class");
	Map = new CMap(G);
	CLOG("Map class created");
	if(L.FirstInstance() == true){
		CLOG("First Instance == true");
        loaded = false;
		firstload = false;
		iterations = 0;
		saved = false;
		send_to_web = true;
		get_from_web = true;
		update_NTAI = true;
	}
	CLOG("Creating MetalHandler class");
	M = new CMetalHandler(G);
	CLOG("Loading Metal cache");
	M->loadState();
	CLOG("View The NTai Log file from here on");
	/*if(M->hotspot.empty() == false){
		for(vector<float3>::iterator hs = M->hotspot.begin(); hs != M->hotspot.end(); ++hs){
			float3 tpos = *hs;
			tpos.y = cb->GetElevation(tpos.x,tpos.z);
			ctri triangle = Tri(tpos);
			triangles.push_back(triangle);
		}
	}*/
	//if(L.FirstInstance() == true){
    L << " :: Found " << M->m->NumSpotsFound << " Metal Spots" << endline;
	//}
	UnitDefLoader = new CUnitDefLoader(G);
	OrderRouter = new COrderRouter(G);
	L.print("Order Router constructed");
	DTHandler = new CDTHandler(G);
	L.print("DTHandler constructed");
	RadarHandler = new CRadarHandler(G);
	L.print("RadarHandler constructed");
	Pl = new Planning(G);
	L.print("Planning constructed");
	As = new Assigner;
	L.print("Assigner constructed");
	Sc = new Scouter;
	L.print("Scouter constructed");
	Economy = new CEconomy(G);
	L.print("Economy constructed");
	//TaskFactory = new CTaskFactory(G);
	//L.print("TaskFactory constructed");
	Manufacturer = new CManufacturer(G);
	L.print("Manufacturer constructed");
	Ch = new Chaser;
	UnitDefHelper = new CUnitDefHelp(G);
	L.print("Chaser constructed");
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Global::~Global(){
	SaveUnitData();
	L.Close();
	delete [] Cached->encache;
	delete info;
	delete DTHandler;
	delete Economy;
	delete RadarHandler;
	delete Actions;
	delete Map;
	delete Manufacturer;
	delete Ch;
	delete Sc;
	delete As;
	delete Pl;
	delete M;
	delete info->mod_tdf;
	delete Cached;
	delete OrderRouter;
	delete UnitDefHelper;
	delete UnitDefLoader;
	//delete TaskFactory;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

map<int,const UnitDef*> endefs;
const UnitDef* Global::GetEnemyDef(int enemy){
	if(endefs.find(enemy) == endefs.end()){
		const UnitDef* ud = GetUnitDef(enemy);
		if(ud == 0){
			return 0;
		}else{
			endefs[enemy] = ud;
			return ud;
		}
	} else{
		return endefs[enemy];
	}
	return 0;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::Update(){
	EXCEPTION_HANDLER({
		if(cb->GetCurrentFrame() == (1 SECOND)){
			NLOG("STARTUP BANNER IN Global::Update()");
			if(L.FirstInstance() == true){
				cb->SendTextMsg(":: NTai XE9RC22 by AF",0);
				cb->SendTextMsg(":: Copyright (C) 2006 AF",0);
				string s = string(" :: ") + Get_mod_tdf()->SGetValueMSG("AI\\message");
				if(s != string("")){
					cb->SendTextMsg(s.c_str(),0);
				}
			}
			int* ax = new int[6000];
			int anum =cb->GetFriendlyUnits(ax);
			if(anum ==0){
				ComName = string("");
			}else{
				ComName = string("");
				for(int a = 0; a<anum; a++){
					if(cb->GetUnitTeam(ax[a]) == cb->GetMyTeam()){
						const UnitDef* ud = GetUnitDef(ax[a]);
						if(ud!=0){
							//
							ComName = ud->name;
							Cached->comID=ax[a];
						}
					}
				}
			}
		}
	},"Startup Banner and getting commander name",NA)
	EXCEPTION_HANDLER({
		if(EVERY_((15 FRAMES))){
			if((idlenextframe.empty()==false)&&(cb->GetCurrentFrame() > (1 SECOND))){
				char c[200];
				sprintf(c,"%i units scheduled for idle calls",idlenextframe.size());
				G->L.print(c);
				set<int> newer;
				newer.insert(idlenextframe.begin(),idlenextframe.end());
				idlenextframe.erase(idlenextframe.begin(),idlenextframe.end());
				idlenextframe.clear();
				for(set<int>::iterator i = newer.begin(); i != newer.end(); ++i){
					if(*i > 0){
						G->L.print("issuing scheduled unitidle");
						UnitIdle(*i);
					}
				}
				G->L.print("scheduled unitidle loop finished");
			}
		}
	},"Sorting scheduled untiIdles",NA)
	//if( EVERY_((2 SECONDS)) && (Cached->cheating == true) ){
	//	float a = cb->GetEnergyStorage() - cb->GetEnergy();
	//	if( a > 50) chcb->GiveMeEnergy(a);
	//	a = cb->GetMetalStorage() - cb->GetMetal();
	//	if(a > 50) chcb->GiveMeMetal(a);
	//}
	EXCEPTION_HANDLER({
			if(EVERY_((2 MINUTES))){
				L.print("saving UnitData");
				if(SaveUnitData()){
					L.print("UnitData saved");
				}else{
					L.print("UnitData not saved");
				}
			}
		},"SaveUnitData()",NA)
	EXCEPTION_HANDLER(OrderRouter->Update(),"OrderRouter->Update()",NA)
	//EXCEPTION_HANDLER(Pl->Update(),"Pl->Update()",NA)
	EXCEPTION_HANDLER(Actions->Update(),"Actions->Update()",NA)
	EXCEPTION_HANDLER(As->Update(),"As->Update()",NA)
	EXCEPTION_HANDLER(Manufacturer->Update(),"Manufacturer->Update()",NA)
	Ch->Update();
	
/*	if(EVERY_(1 SECOND)){
		if(triangles.empty() == false){
			NLOG("triangles.empty() == false");
			if((L.FirstInstance() == true)&&(EVERY_( 3 FRAMES))){
				int tricount = 0;
				for(vector<ctri>::iterator ti = triangles.begin(); ti != triangles.end(); ++ti){
					if(ti->bad == true){
						tricount++;
						continue;
					}
					Increment(ti, G->cb->GetCurrentFrame());
					if((ti->lifetime+ti->creation+1)>cb->GetCurrentFrame()){
						Draw(*ti);
					}else{
						ti->bad = true;
					}
				}
				for(int gk = 0; gk < tricount; gk++){
					for(vector<ctri>::iterator ti = triangles.begin(); ti != triangles.end(); ++ti){
						if(ti->bad == true){
							triangles.erase(ti);
							break;
						}
					}
				}
			}
		}
	}*/
	NLOG("Global::Update :: done");
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Global::SortSolobuilds(int unit){
	Cached->enemies.erase(unit);
	if(endefs.find(unit) != endefs.end()) endefs.erase(unit);
	if(positions.empty()==false){
		map<int,temp_pos>::iterator q = positions.find(unit);
		if(q != positions.end()){
			positions.erase(q);
		}
	}
	const UnitDef* ud = GetUnitDef(unit);
	if(ud != 0){
		bool found = false;
		for(vector<string>::iterator vi = Cached->solobuild.begin(); vi != Cached->solobuild.end(); ++vi){
			if(*vi == ud->name){
				found = true;
				break;
			}
		}
		if(Cached->singlebuilds.find(ud->name)!= Cached->singlebuilds.end()){
			Cached->singlebuilds[ud->name] = true;
		}
		if(found == true)	Cached->solobuilds[ud->name] = unit;
	}
}
void Global::UnitCreated(int unit){
	EXCEPTION_HANDLER(SortSolobuilds(unit),"Sorting solobuilds and singlebuilds in Global::UnitCreated",NA)
	//EXCEPTION_HANDLER(Ch->UnitCreated(unit),"Ch->UnitCreated",NA)
	EXCEPTION_HANDLER(Manufacturer->UnitCreated(unit),"Manufacturer->UnitCreated",NA)
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Global::EnemyDestroyed(int enemy,int attacker){
	if(positions.empty()==false){
		map<int,temp_pos>::iterator i = positions.find(enemy);
		if(i != positions.end()){
			positions.erase(i);
		}
	}
	Cached->enemies.erase(enemy);
	if(endefs.find(enemy) != endefs.end()) endefs.erase(enemy);
	if(attacker > 0){
		const UnitDef* uda = GetUnitDef(attacker);
		if(uda != 0){
			if(efficiency.find(uda->name) != efficiency.end()){
				efficiency[uda->name] += 200/uda->metalCost;
			}else{
				efficiency[uda->name] = 500;
			}
		}
		const UnitDef* ude = GetUnitDef(attacker);
		if(uda != 0){
			if(efficiency.find(ude->name) != efficiency.end()){
				efficiency[ude->name] += 100/ude->metalCost;
			}else{
				efficiency[ude->name] = 100/ude->metalCost;
			}
		}
	}
	//Actions->EnemyDestroyed(enemy,attacker);
	Ch->EnemyDestroyed(enemy,attacker);
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitFinished(int unit){
	// damned exception handler macro wont work when I try to shove this chunk in it, complains about too many parameters
	try{ 
		const UnitDef* ud = GetUnitDef(unit);
		if(ud!=0){
			max_energy_use += ud->energyUpkeep;
			map<string,int>::iterator k = Cached->solobuilds.find(ud->name);
			if(k != Cached->solobuilds.end()) Cached->solobuilds.erase(k);
			if(ud->movedata == 0){
				if(ud->canfly == false){
					if(ud->builder == true){
						float3 upos = GetUnitPos(unit);
						if(upos != UpVector){
							DTHandler->AddRing(upos, 500.0f, float(PI_2) / 6.0f);
							DTHandler->AddRing(upos, 700.0f, float(-PI_2) / 6.0f);
						}
					}
				}
			}
		}
	}catch(exception e){
		G->L.eprint(string("error in Sorting solobuild additions and DT Rings in Global::UnitFinished "));
		G->L.eprint(e.what());
	}catch(exception* e){
		G->L.eprint(string("error in Sorting solobuild additions and DT Rings in Global::UnitFinished "));
		G->L.eprint(e->what());
	}catch(string s){
		G->L.eprint(string("error in Sorting solobuild additions and DT Rings in Global::UnitFinished "));
		G->L.eprint(s);
	}catch(...){
		G->L.eprint(string("error in Sorting solobuild additions and DT Rings in Global::UnitFinished "));
	}
	EXCEPTION_HANDLER(As->UnitFinished(unit),"As->UnitFinished",NA)
	EXCEPTION_HANDLER(Manufacturer->UnitFinished(unit),"Manufacturer->UnitFinished",NA)
	EXCEPTION_HANDLER(Sc->UnitFinished(unit),"Sc->UnitFinished",NA)
	EXCEPTION_HANDLER(Ch->UnitFinished(unit),"Ch->UnitFinished",NA)
}
bool Global::CanDGun(int uid){
	const UnitDef* ud = GetUnitDef(uid);
	if(ud!=0){
		return ud->canDGun;
	}else{
		return false;
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitMoveFailed(int unit){
	EXCEPTION_HANDLER(Sc->UnitMoveFailed(unit),"Sc->UnitMoveFailed",NA)
	EXCEPTION_HANDLER(Manufacturer->UnitIdle(unit),"Manufacturer->UnitIdle in UnitMoveFailed",NA)
	EXCEPTION_HANDLER(Ch->UnitIdle(unit),"Ch->UnitIdle in UnitMoveFailed",NA)
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitIdle(int unit){
	if(unit<1){
		L.print("CManufacturer::UnitIdle negative uid, aborting");
		return;
	}
	EXCEPTION_HANDLER(Manufacturer->UnitIdle(unit),"Manufacturer->UnitIdle",NA)
	EXCEPTION_HANDLER(Sc->UnitIdle(unit),"Sc->UnitIdle",NA)
	//EXCEPTION_HANDLER(Actions->UnitIdle(unit),"Actions->UnitIdle",NA)
	EXCEPTION_HANDLER(Ch->UnitIdle(unit),"Ch->UnitIdle",NA)
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Global::Crash(){
	NLOG(" Deliberate Global::Crash routine");
	// close the logfile
	SaveUnitData();
	L.header("\n :: The user has initiated a crash, terminating NTai \n");
	L.Close();
#ifndef _DEBUG
	// Create an exception forcing spring to close
	vector<string> cv;
	string n = cv.back();
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	NLOG("Global::UnitDamaged");
	EXCEPTION_HANDLER({
		//if(damaged < 1) return;
		//if(attacker < 1) return;
		if(cb->GetUnitAllyTeam(attacker) == Cached->unitallyteam) return;
	},"Global::UnitDamaged, filtering out bad calls",NA)
	EXCEPTION_HANDLER({
		if(attacker > 0){
			const UnitDef* uda = GetUnitDef(attacker);
			if(uda != 0){
				if(efficiency.find(uda->name) != efficiency.end()){
					efficiency[uda->name] += 10000/uda->metalCost;
				}else{
					efficiency[uda->name] = 500;
				}
			}
		}
	},"Global::UnitDamaged, threat value handling",NA)
	//Manufacturer->UnitDamaged(damaged,attacker,damage,dir);
	EXCEPTION_HANDLER(Actions->UnitDamaged(damaged,attacker,damage,dir),"Actions->UnitDamaged()",NA)
	Ch->UnitDamaged(damaged,attacker,damage,dir);
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Global::GotChatMsg(const char* msg,int player){
	L.Message(msg,player);
	string tmsg = msg;
	if(tmsg == string(".verbose")){
		if(L.Verbose()){
			L.iprint("Verbose turned on");
		} else L.iprint("Verbose turned off");
	}else if(tmsg == string(".crash")){
		Crash();
	}else if(tmsg == string(".break")){
		if(L.FirstInstance() == true)L.print("The user initiated debugger break");
	}else if(tmsg == string(".isfirst")){
		if(L.FirstInstance() == true) L.iprint(" info :: This is the first NTai instance");
	}else if(tmsg == string(".save")){
		if(L.FirstInstance() == true) SaveUnitData();
	}else if(tmsg == string(".reload")){
		if(L.FirstInstance() == true) LoadUnitData();
	}else if(tmsg == string(".flush")){
		L.Flush();
	}else if(tmsg == string(".threat")){
		Ch->MakeTGA();
	}else if(tmsg == string(".aicheat")){
		//chcb = G->gcb->GetCheatInterface();
		if( (Cached->cheating== false) && (chcb != 0) ){
			Cached->cheating = true;
			chcb->SetMyHandicap(1000.0f);
            if(L.FirstInstance() == true) L.iprint("Make sure you've typed .cheat for full cheating!");
			// Spawn 4 commanders around the starting position
			const UnitDef* ud = GetUnitDef(ComName.c_str());
			if(ud != 0){
				float3 pos = Map->basepos;
				pos = cb->ClosestBuildSite(ud,pos,1000.0f,0);
				int ij = chcb->CreateUnit(ComName.c_str(),pos);
				if(ij != 0) Actions->RandomSpiral(ij);
				float3 epos = pos;
				epos.z -= 1300.0f;
				srand(uint(time(NULL)+Cached->randadd+Cached->team));
				Cached->randadd++;
				float angle = float(rand()%320);
				pos = G->Map->Rotate(epos,angle,pos);
				///
				ij = chcb->CreateUnit(ComName.c_str(),pos);
				if(ij != 0) Actions->RandomSpiral(ij);
				epos = pos;
				epos.z -= 900.0f;
				srand(uint(time(NULL)+Cached->randadd+Cached->team));
				Cached->randadd++;
				angle = float(rand()%320);
				pos = G->Map->Rotate(epos,angle,pos);
				///
				ij = chcb->CreateUnit(ComName.c_str(),pos);
				if(ij != 0){
					Actions->RandomSpiral(ij);
					int s= max(int(Sc->start_pos.size()),2);
					int i = G->GetCurrentFrame()%(s-1);
					float3 spos = Sc->GetStartPos(i);
					float3 newpos = pos + ((spos - pos)/2);
					Actions->Move(ij,newpos);
				}
			}
		}else if(Cached->cheating == true){
			Cached->cheating  = false;
			if(L.FirstInstance() == true)L.iprint("cheating is now disabled therefore NTai will no longer cheat");
		}
	}else if(tmsg == string(".threat")){
		Ch->MakeTGA();
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDestroyed(int unit, int attacker){
	if(positions.empty()==false){
		map<int,temp_pos>::iterator i = positions.find(unit);
		if(i != positions.end()){
			positions.erase(i);
		}
	}
	idlenextframe.erase(unit);
	const UnitDef* udu = GetUnitDef(unit);
	if(udu != 0){
		max_energy_use -= udu->energyUpkeep;
		if(Cached->singlebuilds.find(udu->name) != Cached->singlebuilds.end()){
			Cached->singlebuilds[udu->name] = false;
		}
	}
	if(attacker >0){
		const UnitDef* uda = GetUnitDef(attacker);
		if((uda != 0)&&(udu != 0)){
			map<string,int>::iterator k = Cached->solobuilds.find(udu->name);
			if(k != Cached->solobuilds.end()) Cached->solobuilds.erase(k);
			if(efficiency.find(uda->name) != efficiency.end()){
				efficiency[uda->name] += 20000/udu->metalCost;
			}else{
				efficiency[uda->name] = 500;
			}
			if(efficiency.find(udu->name) != efficiency.end()){
				efficiency[udu->name] -= 10000/uda->metalCost;
			}else{
				efficiency[udu->name] = 500;
			}
		}
	}
	//Actions->UnitDestroyed(unit);
	Cached->enemies.erase(unit);
	As->UnitDestroyed(unit);
    Manufacturer->UnitDestroyed(unit);
	Sc->UnitDestroyed(unit);
	Ch->UnitDestroyed(unit,attacker);
	OrderRouter->UnitDestroyed(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Global::DrawTGA(string filename,float3 position){
	STGA tga;
	if(LoadTGA(filename.c_str(),tga)==true){
		L.print(" :: the loading of logo.TGA worked!!!");
		for(int y= 0; y< tga.height; y++){
			for(int x = 0; x< tga.width; x++){
				float3 tpos = float3(x*1.0f,500,y*1.0f);
				tpos += position;
				float3 pos2 = tpos;
				pos2.y += 4;
				int group = cb->CreateLineFigure(tpos,pos2,2.0f,0,600,0);
				cb->SetFigureColor(group,tga.data[tga.width*3*y+x*3+2],tga.data[tga.width*3*y+x*3+1],tga.data[tga.width*3*y+x*3+3],1.0f);
			}
		}
		return true;
	}else{
		L.print(string(" :: ERROR!!!! the loading of ") + filename + string(" failed!!!"));
		return false;
	}
}
void Global::InitAI(IAICallback* callback, int team){
	L.print("Initialisising");
	string filename = info->datapath + slash + info->tdfpath + slash + string("mod.tdf");
	string* buffer = new string;
	if(cb->GetFileSize(filename.c_str())!=-1){
		Get_mod_tdf()->LoadFile(filename);
		L.print("Mod TDF loaded");
	} else {/////////////////
		info->abstract = true;
		L.header(" :: mod.tdf failed to load, assuming default values");
		L.header(endline);
	}
	delete buffer;
	//load all the mod.tdf settings!
	info->gaia = false;
	Get_mod_tdf()->GetDef(info->abstract, "1", "AI\\abstract");
	Get_mod_tdf()->GetDef(info->gaia, "0", "AI\\GAIA");
	Get_mod_tdf()->GetDef(info->spacemod, "0", "AI\\spacemod");
	Get_mod_tdf()->GetDef(info->mexfirst, "0", "AI\\first_attack_mexraid");
	Get_mod_tdf()->GetDef(info->hardtarget, "0", "AI\\hard_target");
	Get_mod_tdf()->GetDef(info->mexscouting, "1", "AI\\mexscouting");
	Get_mod_tdf()->GetDef(info->dynamic_selection, "1", "AI\\dynamic_selection");
	Get_mod_tdf()->GetDef(info->use_modabstracts, "0", "AI\\use_mod_default");
	Get_mod_tdf()->GetDef(info->absent_abstract, "1", "AI\\use_mod_default_if_absent");
	Get_mod_tdf()->GetDef(info->rule_extreme_interpolate, "1", "AI\\rule_extreme_interpolate");
	info->antistall = atoi(Get_mod_tdf()->SGetValueDef("0", "AI\\antistall").c_str());
	info->Max_Stall_TimeMobile = (float)atof(Get_mod_tdf()->SGetValueDef("0", "AI\\MaxStallTime").c_str());
	info->Max_Stall_TimeIMMobile = (float)atof(Get_mod_tdf()->SGetValueDef(Get_mod_tdf()->SGetValueDef("0", "AI\\MaxStallTime"), "AI\\MaxStallTimeimmobile").c_str());
	//Get_mod_tdf()->GetDef(send_to_web, "1", "AI\\web_contribute");
	//Get_mod_tdf()->GetDef(get_from_web, "1", "AI\\web_recieve");
	//Get_mod_tdf()->GetDef(update_NTAI, "1", "AI\\NTAI_update");
	info->fire_state_commanders = atoi(Get_mod_tdf()->SGetValueDef("0", "AI\\fire_state_commanders").c_str());
	info->move_state_commanders = atoi(Get_mod_tdf()->SGetValueDef("0", "AI\\move_state_commanders").c_str());
	info->scout_speed = (float)atof(Get_mod_tdf()->SGetValueDef("50", "AI\\scout_speed").c_str());
	if(info->abstract == true) info->dynamic_selection = true;
	if(info->use_modabstracts == true) info->abstract = false;
	if(info->abstract == true){
		L.print("abstract == true");
	}
	L.print("values filled");
	Cached->solobuild = bds::set_cont(Cached->solobuild,Get_mod_tdf()->SGetValueMSG("AI\\SoloBuild"));
	Pl->AlwaysAntiStall = bds::set_cont(Pl->AlwaysAntiStall,Get_mod_tdf()->SGetValueMSG("AI\\AlwaysAntiStall"));
	vector<string> singlebuild;
	singlebuild = bds::set_cont(singlebuild,Get_mod_tdf()->SGetValueMSG("AI\\SingleBuild"));
	if(singlebuild.empty() == false){
		for(vector<string>::iterator i= singlebuild.begin(); i != singlebuild.end(); ++i){
			Cached->singlebuilds[*i] = false;
		}
	}
	L.print("Arrays filled");
	if(info->abstract == true){
		L.header(" :: Using abstract buildtree");
		L.header(endline);
	}
	if(info->gaia == true){
		L.header(" :: GAIA AI On");
		L.header(endline);
	}
	L.header(endline);
	if(loaded == false){
		L.print("Loading unit data");
		LoadUnitData();
		L.print("Unit data loaded");
	}


	As->InitAI(this);
	L.print("Assigner Init'd");
	Pl->InitAI();
	L.print("Planner Init'd");
	Manufacturer->Init();
	L.print("Manufacturer Init'd");
	Sc->InitAI(this);
	L.print("Scouter Init'd");
	Ch->InitAI(this);
	L.print("Chaser Init'd");
}

int Global::GetCurrentFrame(){
	//
	int i = cb->GetCurrentFrame();
	if(i  < 30){
		return i;
	}else{
		return i +Cached->team;
	}
}
bool Global::InLOS(float3 pos){
	int lwidth = cb->GetMapWidth()/2;
	ushort v = Cached->losmap[lwidth*int(pos.z/16)+int(pos.x/16)];
	if(v == 0){
		return false;
	}else{
		return true;
	}
}
string Global::lowercase(string s){
	string ss = s;
	int (*pf)(int)=tolower;
	transform(ss.begin(), ss.end(), ss.begin(), pf);
	return ss;
}

bool Global::ReadFile(string filename, string* buffer){
	int ebsize= 0;
	ifstream fp;
	fp.open(filename.c_str(), ios::in);
	if(fp.is_open() == false){
		L.header(string(" :: error loading file :: ") + filename + endline);
		int size = G->cb->GetFileSize(filename.c_str());
		if(size >0){
			char* c = new char[size+1];
			bool fg = cb->ReadFile(filename.c_str(),c,size+1);
			if(fg==true){
				//
				*buffer = string(c);
				return true;
			}else{
				//
				return false;
			}
		}
		return false;
	}else{
		*buffer = "";
		char in_char;
		while(fp.get(in_char)){
			buffer->push_back(in_char);
			ebsize++;
		}
		if(ebsize == 0){
			L.header(string(" :: error loading contents of file :: ") + filename + endline);
			return false;
		}else{
            return true;
		}
	}
}

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::Draw(ctri triangle){
	int t1 = G->cb->CreateLineFigure(triangle.a,triangle.b,4,0,triangle.fade,0);
	int t2 = G->cb->CreateLineFigure(triangle.b,triangle.c,4,0,triangle.fade,0);
	int t3 = G->cb->CreateLineFigure(triangle.c,triangle.a,4,0,triangle.fade,0);
	G->cb->SetFigureColor(t1,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
	G->cb->SetFigureColor(t2,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
	G->cb->SetFigureColor(t3,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
}

ctri Global::Tri(float3 pos, int size, float speed, int lifetime, int fade, int creation){
	ctri Triangle;
	Triangle.d = size;
	Triangle.position = pos;
	Triangle.position.y = cb->GetElevation(pos.x,pos.y) + 30;
	Triangle.speed = speed;
	Triangle.flashy = false;
	Triangle.a = Triangle.position;
	Triangle.a.z = Triangle.a.z - size;
	Triangle.b = Triangle.c = Triangle.a;
	Triangle.b = Map->Rotate(Triangle.a,120 DEGREES,Triangle.position);
	Triangle.c = Map->Rotate(Triangle.b,120 DEGREES,Triangle.position);
	Triangle.lifetime = lifetime;
	Triangle.creation = creation;
	Triangle.fade = fade;
	Triangle.alpha = Triangle.colour.alpha = 0.6f;
	Triangle.colour.red = 0.7f;
	Triangle.colour.green = 0.9f;
	Triangle.colour.blue =  0.01f;
	Triangle.bad = false;
	return Triangle;
}

void Global::Increment(vector<ctri>::iterator triangle, int frame){
	triangle->a = Map->Rotate(triangle->a,triangle->speed DEGREES,triangle->position);
	triangle->b = Map->Rotate(triangle->b,triangle->speed DEGREES,triangle->position);
	triangle->c = Map->Rotate(triangle->c,triangle->speed DEGREES,triangle->position);
}


int Global::GetEnemyUnits(int* units, const float3 &pos, float radius){
	NLOG("Global::GetEnemyUnits :: A");
	if(Cached->cheating == true){
		return chcb->GetEnemyUnits(units,pos,radius);
	}else{
		return cb->GetEnemyUnits(units,pos,radius);
	}
}


int Global::GetEnemyUnitsInRadarAndLos(int* units){
	NLOG("Global::GetEnemyUnitsinradarandLOS :: B");
	if(GetCurrentFrame() - Cached->lastcacheupdate>  30){
		if(Cached->cheating == true){
			Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
			for(uint h = 0; h < Cached->enemy_number; h++){
				units[h] = Cached->encache[h];
			}
			Cached->lastcacheupdate = cb->GetCurrentFrame();
			return Cached->enemy_number;
		}else{
			return cb->GetEnemyUnitsInRadarAndLos(Cached->encache);
		}
	}
	if(Cached->cheating == true){
		Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
		for(uint h = 0; h < Cached->enemy_number; h++){
			units[h] = Cached->encache[h];
		}
	}else{
		Cached->enemy_number = cb->GetEnemyUnitsInRadarAndLos(Cached->encache);
	}
	return Cached->enemy_number;
}

int Global::GetEnemyUnits(int* units){
	NLOG("Global::GetEnemyUnits :: B");
	if(GetCurrentFrame() - Cached->lastcacheupdate>  30){
		if(Cached->cheating == true){
			Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
		}else{
			Cached->enemy_number = cb->GetEnemyUnits(Cached->encache);
		}
		Cached->lastcacheupdate = cb->GetCurrentFrame();
	}
	for(uint h = 0; h < Cached->enemy_number; h++){
		units[h] = Cached->encache[h];
	}
	return Cached->enemy_number;
}



float Global::GetEfficiency(string s){
	if(efficiency.find(s) != efficiency.end()){
		return efficiency[s];
	}else{
		L.iprint("error ::   " + s + " is missing from the efficiency array");
		return 500.0f;
	}
}



bool Global::LoadUnitData(){
	int unum = cb->GetNumUnitDefs();
	const UnitDef** ulist = new const UnitDef*[unum];
	cb->GetUnitDefList(ulist);
	for(int i = 0; i < unum; i++){
		float ts = 500;
		if(ulist[i]->weapons.empty()){
			ts += ulist[i]->health+ulist[i]->energyMake + ulist[i]->metalMake + ulist[i]->extractsMetal*50+ulist[i]->tidalGenerator*30 + ulist[i]->windGenerator*30;
			ts *= 300;
		}
		efficiency[ulist[i]->name] = ts;
		unit_names[ulist[i]->name] = ulist[i]->humanName;
		unit_descriptions[ulist[i]->name] = ulist[i]->tooltip;
	}
	string filename = info->datapath;
	filename += slash;
	filename += "learn";
	filename += slash;
	filename += info->tdfpath;
	filename += ".tdf";
	string* buffer = new string;
	if(ReadFile(filename,buffer) == true){
		TdfParser cq(this);
		cq.LoadBuffer(buffer->c_str(), buffer->size());
		iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());
		for(map<string,float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
			string s = "AI\\";
			s += i->first;
			float ank = (float)atof(cq.SGetValueDef("10", s.c_str()).c_str());
			if(ank > i->second) i->second = ank;
		}
		iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());
		iterations++;
		cq.GetDef(firstload, "1", "AI\\firstload");
		if(firstload == true){
			L.iprint(" This is the first time this mod has been loaded, up. Take this first game to train NTai up, and be careful f fthrowing the same units at it over and over again");
			firstload = false;
			for(map<string,float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				const UnitDef* uda = GetUnitDef(i->first);
				if(uda !=0){
					i->second += uda->health;
				}
			}
		}
		loaded = true;
		return true;
	} else{
		SaveUnitData();
		return false;
	}
	return false;
}

bool Global::SaveUnitData(){
	NLOG("Global::SaveUnitData()");
	if(L.FirstInstance() == true){
		if(this->iterations > 5){
			return true;
		}
		ofstream off;
		string filename = info->datapath;
		filename += slash;
		filename += "learn";
		filename += slash;
		filename += info->tdfpath;
		filename += ".tdf";
		off.open(filename.c_str());
		if(off.is_open() == true){
			off << "[AI]" << endl << "{" << endl << "    // NTai XE9RC22 AF :: unit efficiency cache file" << endl << endl;
			// put stuff in here;
			int first = firstload;
			off << "    version=XE9RC22;" << endl;
			off << "    firstload=" << first << ";" << endl;
			off << "    modname=" << G->cb->GetModName() << ";" << endl;
			off << "    iterations=" << iterations << ";" << endl;
			off << endl;
			for(map<string,float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				off << "    "<< i->first << "=" << i->second << ";    // " << unit_names[i->first] << " :: "<< unit_descriptions[i->first]<<endl;
			}
			off << "}" << endl;
			off.close();
			saved = true;
			return true;
		}else{
			off.close();
			return false;
		}
	}
	return false;
}

float Global::GetTargettingWeight(string unit, string target){
	float tempscore = 0;
	if(info->hardtarget == false){
		tempscore = GetEfficiency(target);
	}else{
		string fh = unit + "\\target_weights\\" + target;
		string fg = unit + "\\target_weights\\undefined";
		float fz = GetEfficiency(target);
		char c[16];
		sprintf(c,"%f",fz);
		string tempdef = Get_mod_tdf()->SGetValueDef(c, fg.c_str());
		tempscore = (float)atof(Get_mod_tdf()->SGetValueDef(tempdef.c_str(), fh.c_str()).c_str()); // load "unitname\\target_weights\\enemyunitname" to retirieve targetting weights
	}
	return tempscore;
}

bool Global::LoadTGA(const char *filename, STGA& tgaFile){
	FILE *file;
	unsigned char type[4];
	unsigned char Tinfo[6];

	file = fopen(filename, "rb");

	if (!file){
		L.print(":: TGA file couldn't be loaded");
		return false;
	}

	fread (&type, sizeof (char), 3, file);
	fseek (file, 12, SEEK_SET);
	fread (&Tinfo, sizeof (char), 6, file);

	//image type either 2 (color) or 3 (greyscale)
	if (type[1] != 0 || (type[2] != 2 && type[2] != 3)){
		L.print(":: TGA file couldn't be loaded(2)");
		fclose(file);
		return false;
	}

	tgaFile.width = Tinfo[0] + Tinfo[1] * 256;
	tgaFile.height = Tinfo[2] + Tinfo[3] * 256;
	tgaFile.byteCount = Tinfo[4] / 8;

	if (tgaFile.byteCount != 3 && tgaFile.byteCount != 4) {
		L.print(":: TGA file couldn't be loaded(3)");
		fclose(file);
		return false;
	}

	long imageSize = tgaFile.width * tgaFile.height
		* tgaFile.width * tgaFile.byteCount;

	//allocate memory for image data
	tgaFile.data = new unsigned char[imageSize];

	//read in image data
	fread(tgaFile.data, sizeof(unsigned char), imageSize, file);

	//close file
	fclose(file);

	return true;
}

void Global::EnemyEnterLOS(int enemy){ // an enemy has entered LOS
	Cached->enemies.insert(enemy);
}
void Global::EnemyLeaveLOS(int enemy){ // An enemy has left LOS
	Cached->enemies.erase(enemy);
}
void Global::EnemyEnterRadar(int enemy){ // an enemy has entered radar
	Cached->enemies.insert(enemy);
}
void Global::EnemyLeaveRadar(int enemy){ // an enemy has left radar
	Cached->enemies.erase(enemy);
}

float3 Global::GetUnitPos(int unitid,int enemy){ // do 10 frame delays between updates fo different units
	if(unitid < 1){
		return UpVector;
	}
	bool mine = true;
	if(enemy==1){
		mine=false;
	}else if (enemy==0){
		if(G->cb->GetUnitAllyTeam(unitid)!=G->cb->GetMyAllyTeam()){
			mine=false;
		}
	}
	
	if(positions.empty()==false){
		map<int,temp_pos>::iterator i = positions.find(unitid);
		if(i!=positions.end()){
			temp_pos k = i->second;
			if(GetCurrentFrame()-k.last_update > 10){
				float3 p = cb->GetUnitPos(unitid);
				if(Map->CheckFloat3(p)==false){
					if(Cached->cheating==false){
						return UpVector;
					}else{
						p = chcb->GetUnitPos(unitid);
						if(Map->CheckFloat3(p)==false){
							return UpVector;
						}else{
							k.pos = p;
							k.last_update=GetCurrentFrame();
							return p;
						}
					}
				}else{
					k.pos = p;
					k.last_update=GetCurrentFrame();
					return p;
				}
			}else{
				return k.pos;
			}
		}
	}
	float3 p = cb->GetUnitPos(unitid);
	if(Map->CheckFloat3(p)==false){
		if(Cached->cheating==false){
			return UpVector;
		}else{
			p = chcb->GetUnitPos(unitid);
			if(Map->CheckFloat3(p)==false){
				return UpVector;
			}else{
				temp_pos k;
				k.enemy=!mine;
				k.pos = p;
				k.last_update=GetCurrentFrame();
				positions[unitid]=k;
				return p;
			}
		}
	}else{
		return p;
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/** unitdef->type
	MetalExtractor
	Transport
	Builder
	Factory
	Bomber
	Fighter
	GroundUnit
	Building
**/

