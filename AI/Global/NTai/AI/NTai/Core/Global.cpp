//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

#include "include.h"

namespace ntai {

	int iterations=0;
	bool loaded=false;
	bool firstload=false;
	bool saved=false;
	map<string, float> efficiency;
	map<string, float> builderefficiency;
	map<string, int> lastbuilderefficiencyupdate;


	TdfParser* Global::Get_mod_tdf(){
		return info->mod_tdf;
	}


	void trim(string &str){
		string::size_type pos = str.find_last_not_of(' ');
		if(pos != string::npos) {
			str.erase(pos + 1);
			pos = str.find_first_not_of(' ');
			if(pos != string::npos) str.erase(0, pos);
		}
		else str.erase(str.begin(), str.end());
	}

	bool ValidUnitID(int id){
		if(id < 0){
			return false;
		}
		if(id > MAX_UNITS+1){
			return false;
		}
		return true;
	}

	Global::Global(IGlobalAICallback* callback){
		G = this;
		gcb = callback;
		cb = gcb->GetAICallback();
		if(cb == 0){
			throw string(" fatal error cb ==0");
		}

		CLOG("Started Global::Global class constructor");
		CLOG("Starting CCached initialisation");
		
		Cached = new CCached;
		Cached->comID = 0;
		Cached->randadd = 0;
		Cached->enemy_number = 0;
		Cached->lastcacheupdate = 0;
		Cached->team = 567;
		CLOG("getting team value");
		Cached->team = cb->GetMyTeam();

		CLOG("Creating Config holder class");
		info = new CConfigData(G);

		CLOG("Setting the Logger class");
		L.Set(this);

		CLOG("Loading AI.tdf with TdfParser");
		TdfParser cs(G);
		cs.LoadFile("AI/AI.tdf");

		CLOG("Retrieving datapath value");
		info->datapath = cs.SGetValueDef(string("AI/NTai"), "AI\\data_path");

		CLOG("Opening logfile in plaintext");
		L.Open(true);
		//L.Verbose();

		CLOG("Logging class Opened");
		L.print("logging started");

		CLOG("Loading modinfo.tdf");
		TdfParser sf(G);
		sf.LoadFile("modinfo.tdf");

		CLOG("Getting tdfpath value");
		info->tdfpath =  sf.SGetValueDef(string(cb->GetModName()), "MOD\\NTAI\\tdfpath");

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

		L << " :: Found " << M->m->NumSpotsFound << " Metal Spots" << endline;
		UnitDefLoader = new CUnitDefLoader(G);
		L.print("Unitdef loader constructed");

		OrderRouter = new COrderRouter(G);
		L.print("Order Router constructed");

		DTHandler = new CDTHandler(G);
		L.print("DTHandler constructed");

		RadarHandler = new CRadarHandler(G);
		L.print("RadarHandler constructed");

		Pl = new Planning(G);
		L.print("Planning constructed");


		Economy = new CEconomy(G);
		L.print("Economy constructed");

		Manufacturer = boost::shared_ptr<CManufacturer>(new CManufacturer(this));
		L.print("Manufacturer constructed");

		BuildingPlacer = boost::shared_ptr<CBuildingPlacer>(new CBuildingPlacer(this));
		L.print("BuildingPlacer constructed");

		Ch = new Chaser;
		L.print("Chaser constructed");
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	Global::~Global(){
		SaveUnitData();
		L.Close();
		delete[] Cached->encache;
		delete info;
		delete DTHandler;
		delete Economy;
		delete RadarHandler;
		delete Actions;
		delete Map;
		delete Ch;
		delete Pl;
		delete M;
		delete Cached;
		delete OrderRouter;
		delete UnitDefLoader;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	map<int, const UnitDef*> endefs;
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

	void Global::EnemyDamaged(int damaged, int attacker, float damage, float3 dir){
		/*Ch->EnemyDamaged(damaged, attacker, damage, dir);
		START_EXCEPTION_HANDLING
		CMessage message("enemydamaged");
		message.AddParameter(damaged);
		message.AddParameter(attacker);
		message.AddParameter(damage);
		message.AddParameter(dir);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"enemydamaged\"); FireEvent(message);")*/
	}

	void Global::Update(){
		bool paused = false;

		NLOG("Global::Update()");
		cb->GetValue(AIVAL_GAME_PAUSED, &paused);
		if(paused){
			return;
		}
		START_EXCEPTION_HANDLING

		if(!handlers.empty()){
			if(!msgqueue.empty()){
				int n = 0;
				for(vector<CMessage>::iterator mi = msgqueue.begin(); mi != msgqueue.end(); ++mi){
					if(mi->IsDead(GetCurrentFrame())){
						continue;
					}

					n++;

					for(set<boost::shared_ptr<IModule> >::iterator k = handlers.begin(); k != handlers.end(); ++k){
						if((*k)->IsValid()){
							(*k)->RecieveMessage(*mi);
						}else{
							RemoveHandler((*k));
						}
					}

					// We dont want to do everything at once if the queue is gigantic
					if(n >15){
						// so erase what we've already parsed and exit the loop, we
						// can do the rest of the queue in the next update call.
						msgqueue.erase(msgqueue.begin(),mi);
						break;
					}
				}

				msgqueue.erase(msgqueue.begin(),msgqueue.end());
			}

		}

		

		START_EXCEPTION_HANDLING

		CMessage message("update");

		// a lifetime of 15 frames, so there should never be more than
		// a backlog of 15 valid update messages
		message.SetLifeTime(15);
		
		FireEvent(message);

		END_EXCEPTION_HANDLING("CMessage message(\"update\"); FireEvent(message);")

		//EXCEPTION_HANDLER(Manufacturer->Update(),"Manufacturer->Update()",NA)
		

	}


	void Global::SortSolobuilds(int unit){
		Cached->enemies.erase(unit);
		if(endefs.find(unit) != endefs.end()) endefs.erase(unit);

		CUnitTypeData* u = UnitDefLoader->GetUnitTypeDataByUnitId(unit);

		const UnitDef* ud = u->GetUnitDef();
	    
		if(ud != 0){
			bool found = false;
			string s  = u->GetName();

			if(u->GetSingleBuild()){
				u->SetSingleBuildActive(true);
			}

			if(u->GetSoloBuild()){
				u->SetSoloBuildActive(true);
			}
		}
	}

	void Global::UnitCreated(int unit){

		if(!ValidUnitID(unit)) return;
		START_EXCEPTION_HANDLING
		SortSolobuilds(unit);
		END_EXCEPTION_HANDLING("Sorting solobuilds and singlebuilds in Global::UnitCreated")

		//EXCEPTION_HANDLER(Ch->UnitCreated(unit),"Ch->UnitCreated",NA)

		START_EXCEPTION_HANDLING
		Manufacturer->UnitCreated(unit);
		END_EXCEPTION_HANDLING("Manufacturer->UnitCreated")

		START_EXCEPTION_HANDLING
		const UnitDef* udf = GetUnitDef(unit);
		if(udf){
			CUnitTypeData* utd = UnitDefLoader->GetUnitTypeDataById(udf->id);
			if(!utd->IsMobile()){
				BuildingPlacer->Block(G->GetUnitPos(unit), utd);
			}
		}
		END_EXCEPTION_HANDLING("Global::UnitFinished blocking map for unit")

		START_EXCEPTION_HANDLING
		boost::shared_ptr<CUnit> Unit = boost::shared_ptr<CUnit>(new CUnit(G, unit));
		Unit->Init();
		units[unit] = Unit;
		RegisterMessageHandler(Unit);
		CMessage message("unitcreated");
		message.AddParameter(unit);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"unitcreated\"); FireEvent(message);")

	}

	void Global::EnemyDestroyed(int enemy, int attacker){

		if(ValidUnitID(attacker)){
			/*if(positions.empty()==false){
			 map<int,temp_pos>::iterator i = positions.find(enemy);
			 if(i != positions.end()){
			 positions.erase(i);
			 }
			 }*/
			Cached->enemies.erase(attacker);
			if(endefs.find(enemy) != endefs.end()) endefs.erase(enemy);
			const UnitDef* uda = GetUnitDef(enemy);
			if(uda != 0){
				float e = GetEfficiency(uda->name, uda->power);
				e =200/uda->metalCost;
				SetEfficiency(uda->name, e);
			}

			uda = GetUnitDef(enemy);
			if(uda != 0){
				float e = GetEfficiency(uda->name, uda->power);
				e -=200/uda->metalCost;
				SetEfficiency(uda->name, e);
			}

			Ch->EnemyDestroyed(enemy, attacker);
			START_EXCEPTION_HANDLING
			CMessage message("enemydestroyed");
			message.AddParameter(enemy);
			message.AddParameter(attacker);
			FireEvent(message);
			END_EXCEPTION_HANDLING("CMessage message(\"enemydestroyed\"); FireEvent(message);")
		}
		//Actions->EnemyDestroyed(enemy,attacker);

	}
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitFinished(int unit){

		CMessage message("unitfinished");
		message.AddParameter(unit);
		FireEvent(message);

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitMoveFailed(int unit){

		START_EXCEPTION_HANDLING
		CMessage message("unitmovefailed");
		message.AddParameter(unit);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"unitmovefailed\"); FireEvent(message);")

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitIdle(int unit){

		if(!ValidUnitID(unit)){
			L.print("CManufacturer::UnitIdle negative uid, aborting");
			return;
		}
		
		if(GetCurrentFrame() < 5 SECONDS){
			return;
		}

		START_EXCEPTION_HANDLING
		CMessage message("unitidle");
		message.AddParameter(unit);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"unitidle\"); FireEvent(message);")


		START_EXCEPTION_HANDLING
		Ch->UnitIdle(unit);
		END_EXCEPTION_HANDLING("Ch->UnitIdle")
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	void Global::Crash(){
		NLOG(" Deliberate Global::Crash routine");
		// close the logfile
		SaveUnitData();
		L.header("\n :: The user has initiated a crash, terminating NTai \n");
		L.Close();
		START_EXCEPTION_HANDLING
		CMessage message("crash");
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"crash\"); FireEvent(message);")
	#ifndef DEBUG
		// Create an exception forcing spring to close
		vector<string> cv;
		string n = cv.back();
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitDamaged(int damaged, int attacker, float damage, float3 dir){
		NLOG("Global::UnitDamaged");

		if(damage <= 0){
			return;
		}

		if(!ValidUnitID(damaged)){
			return;
		}

		if(!ValidUnitID(attacker)){
			return;
		}

		if(cb->GetUnitAllyTeam(attacker) == cb->GetUnitAllyTeam(damaged)){
			return;
		}

		CMessage message("unitdamaged");

		message.AddParameter(damaged);
		message.AddParameter(attacker);
		message.AddParameter(damage);
		message.AddParameter(dir);

		FireEvent(message);

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	void Global::GotChatMsg(const char* msg, int player){
		L.Message(msg, player);
		
		CMessage message("##"+string(msg));
		message.AddParameter(player);
		FireEvent(message);

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitDestroyed(int unit, int attacker){

		if(!(ValidUnitID(unit)&&ValidUnitID(attacker))){
			return;
		}
		units.erase(unit);

		idlenextframe.erase(unit);

		CUnitTypeData* u = G->UnitDefLoader->GetUnitTypeDataByUnitId(unit);

		const UnitDef* udu = u->GetUnitDef();
		if(udu != 0){
			max_energy_use -= udu->energyUpkeep;
			u->SetSingleBuildActive(false);
			u->SetSoloBuildActive(false);
		}

		if(ValidUnitID(attacker)){
			CUnitTypeData* atd = G->UnitDefLoader->GetUnitTypeDataByUnitId(attacker);
			if(atd != 0){
				const UnitDef* uda = atd->GetUnitDef();
				if((uda != 0)&&(udu != 0)){
					

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
		}

		START_EXCEPTION_HANDLING
		CMessage message("unitdestroyed");
		message.AddParameter(unit);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"unitdestroyed\"); FireEvent(message);");

		//Actions->UnitDestroyed(unit);
		Cached->enemies.erase(unit);
		Manufacturer->UnitDestroyed(unit);
		Ch->UnitDestroyed(unit, attacker);
		OrderRouter->UnitDestroyed(unit);
		units.erase(unit);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::InitAI(IAICallback* callback, int team){
		L.print("Initializing");

		mrand.seed(uint(time(NULL)*team));
		string filename = info->datapath + slash + string("NTai.tdf");

		filename = info->datapath + "/" +  info->tdfpath + string(".tdf");
		string* buffer = new string();
		TdfParser* q = new TdfParser(this);

		if(cb->GetFileSize(filename.c_str())!=-1){

			q->LoadFile(filename);
			L.print("Mod TDF loaded");
			filename = info->datapath + "/" + q->SGetValueDef("configs/default.tdf", "NTai\\modconfig");

		} else {/////////////////

			TdfParser* w = new TdfParser(this, "modinfo.tdf");
			info->_abstract = true;
			L.header(" :: mod.tdf failed to load, assuming default values");
			L.header(endline);
			// must write out a config and put in it the default stuff......
			ofstream off;
			//string filename = info->datapath + "/learn/" + info->tdfpath +".tdf";
			off.open(filename.c_str());
			if(off.is_open() == true){
				//off <<
				off << "[NTai]" << endl;
				off << "{"<<endl;
				off << "\tlearndata=" << "learn/" << info->tdfpath <<".tdf;"<<endl;
				off << "\tmodconfig=" << "configs/" << info->tdfpath << ".tdf;" << endl;
				off <<"\tmodname=" << w->SGetValueMSG("MOD\\Name") << ";" << endl;
				off << "}"<<endl;
				off.close();
				filename = info->datapath + slash + string("configs") + slash +  info->tdfpath + string(".tdf");
				off.open(filename.c_str());
				if(off.is_open() == true){
					//off <<
					filename = info->datapath + slash + string("configs") + slash + string("default.tdf");
					string* buffer2 = new string();
					ReadFile(filename, buffer2);
					off << *buffer2;
					off.close();
				}
			}
			delete w;
		}
		delete q;

		//
		if(cb->GetFileSize(filename.c_str())!=-1){
			Get_mod_tdf()->LoadFile(filename);
			L.print("Mod TDF loaded");

		} else {/////////////////

			info->_abstract = true;
			L.header(" :: mod.tdf failed to load, assuming default values");
			L.header(endline);
		}
		delete buffer;

		//load all the mod.tdf settings!
		info->Load();
		
		if(info->_abstract == true){
			L.print("abstract == true");
		}

		L.print("values filled");

		// initial handicap
		float x = 0;
		Get_mod_tdf()->GetDef(x, "0", "AI\\normal_handicap");
		chcb = G->gcb->GetCheatInterface();
		chcb->SetMyHandicap(x);

	    

		// solobuild
		set<std::string> solotemp;
		string sb = Get_mod_tdf()->SGetValueMSG("AI\\SoloBuild");
		CTokenizer<CIsComma>::Tokenize(solotemp, sb, CIsComma());


		if(!solotemp.empty()){
			for(set<string>::iterator i = solotemp.begin(); i != solotemp.end(); ++i){
				CUnitTypeData* u = this->UnitDefLoader->GetUnitTypeDataByName(*i);
				if(u){
					u->SetSoloBuild(true);
				}
			}
		}

		Cached->allyteam = cb->GetMyAllyTeam();

		CTokenizer<CIsComma>::Tokenize(Pl->AlwaysAntiStall, Get_mod_tdf()->SGetValueMSG("AI\\AlwaysAntiStall"), CIsComma());
		//Pl->AlwaysAntiStall = bds::set_cont(Pl->AlwaysAntiStall, Get_mod_tdf()->SGetValueMSG("AI\\AlwaysAntiStall"));

		vector<string> singlebuild;
		sb = Get_mod_tdf()->SGetValueMSG("AI\\SingleBuild");
		CTokenizer<CIsComma>::Tokenize(singlebuild, sb);

		if(singlebuild.empty() == false){
			for(vector<string>::iterator i= singlebuild.begin(); i != singlebuild.end(); ++i){
				string s = *i;
				trim(s);
				tolowercase(s);

				CUnitTypeData* u = UnitDefLoader->GetUnitTypeDataByName(s);

				u->SetSingleBuild(true);
			}
		}

		L.print("Arrays filled");
		if(info->_abstract == true){
			L.header(" :: Using abstract buildtree");
			L.header(endline);
		}

		if(info->gaia){
			L.header(" :: GAIA AI On");
			L.header(endline);
		}

		L.header(endline);
		if(loaded == false){
			L.print("Loading unit data");
			LoadUnitData();
			L.print("Unit data loaded");
		}

		L << " :: " << cb->GetMapName() << endline << " :: " << cb->GetModName() << endline << " :: map size " << cb->GetMapWidth()/64 << " x "  << cb->GetMapHeight()/64 << endline;

		Pl->InitAI();
		L.print("Planner Init'd");

		Manufacturer->Init();
		RegisterMessageHandler(Manufacturer);
		L.print("Manufacturer Init'd");

		BuildingPlacer->Init();
		RegisterMessageHandler(BuildingPlacer);
		L.print("BuildingPlacement Init'd");

		Ch->InitAI(this);
		L.print("Chaser Init'd");

		gproxy = boost::shared_ptr<IModule>(new CGlobalProxy(this));
		gproxy->Init();
		RegisterMessageHandler(gproxy);

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
		const unsigned short* lmap = G->cb->GetLosMap();
		ushort v = lmap[(int(pos.y/16))*G->cb->GetMapWidth()+(int(pos.x/16))];
		return (v >0);
	}

	void tolowercase(string &str){
		std::transform(str.begin(), str.end(), str.begin(), (int (*)(int))tolower);
	}

	bool Global::ReadFile(string filename, string* buffer){
		char buf[1000];
		int ebsize= 0;
		ifstream fp;

		strcpy(buf, filename.c_str());
		cb->GetValue(AIVAL_LOCATE_FILE_R, buf);

		fp.open(buf, ios::in);
		if(fp.is_open() == false){
			L.header(string(" :: error loading file :: ") + filename + endline);
			int size = G->cb->GetFileSize(filename.c_str());
			if(size >0){
				char* c = new char[size+1];
				bool fg = cb->ReadFile(filename.c_str(), c, size+1);
				if(fg==true){
					//
					*buffer = string(c);
					return true;
				}else{
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

	/*void Global::Draw(ctri triangle){
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
	 }*/


	int Global::GetEnemyUnits(int* units, const float3 &pos, float radius){
		NLOG("Global::GetEnemyUnits :: A");
		return chcb->GetEnemyUnits(units, pos, radius);
	}


	int Global::GetEnemyUnitsInRadarAndLos(int* units){
		NLOG("Global::GetEnemyUnitsinradarandLOS :: B");
		return chcb->GetEnemyUnits(units);

	}

	int Global::GetEnemyUnits(int* units){
		NLOG("Global::GetEnemyUnits :: B");
		if(GetCurrentFrame() - Cached->lastcacheupdate > 30){
			Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
			Cached->lastcacheupdate = cb->GetCurrentFrame();
		}
		for(uint h = 0; h < Cached->enemy_number; h++){
			units[h] = Cached->encache[h];
		}

		return Cached->enemy_number;
	}



	float Global::GetEfficiency(string s, float def_value){

		CUnitTypeData* ud = this->UnitDefLoader->GetUnitTypeDataByName(s);
		if(ud == 0){
			return def_value;
		}

		if(efficiency.find(ud->GetName()) != efficiency.end()){
			if(ud->CanConstruct()){

				if(builderefficiency.find(ud->GetName()) != builderefficiency.end()){
					int i = lastbuilderefficiencyupdate[ud->GetName()];
					if(GetCurrentFrame()-(5 MINUTES) < i){
						return builderefficiency[ud->GetName()];
					}
				}

				float e = efficiency[ud->GetName()];
				
				set<string> alreadydone;
				alreadydone.insert(ud->GetName());

				if(!ud->GetUnitDef()->buildOptions.empty()){
					for(map<int, string>::const_iterator i = ud->GetUnitDef()->buildOptions.begin();i != ud->GetUnitDef()->buildOptions.end(); ++i){
						alreadydone.insert(i->second);
						e += efficiency[s];
					}
				}

				lastbuilderefficiencyupdate[ud->GetName()] = GetCurrentFrame();
				builderefficiency[ud->GetName()] = e;

				return e;
			}else{
				return efficiency[ud->GetName()];
			}

		}else{
			L.print("error ::   " + ud->GetName() + " is missing from the efficiency array");
			return def_value;
		}

	}

	void Global::SetEfficiency(std::string s, float e){
		trim(s);
		tolowercase(s);
		efficiency[s] = e;
		//
	}

	float Global::GetEfficiency(string s, set<string>& doneconstructors, int techlevel){

		CUnitTypeData* ud = this->UnitDefLoader->GetUnitTypeDataByName(s);

		if(ud == 0){
			return 0;
		}

		if(efficiency.find(ud->GetName()) != efficiency.end()){
			if(ud->CanConstruct()&&(doneconstructors.find(ud->GetName())==doneconstructors.end())){

				if(builderefficiency.find(s) != builderefficiency.end()){
					int i = lastbuilderefficiencyupdate[ud->GetName()];
					if(GetCurrentFrame()-(5 MINUTES) < i){
						return builderefficiency[ud->GetName()];
					}
				}

				float e = efficiency[ud->GetName()];
				
				doneconstructors.insert(ud->GetName());

				for(map<int, string>::const_iterator i = ud->GetUnitDef()->buildOptions.begin();i != ud->GetUnitDef()->buildOptions.end(); ++i){
					CUnitTypeData* ud2 = UnitDefLoader->GetUnitTypeDataByName(i->second);

					if(doneconstructors.find(i->second)==doneconstructors.end()){
						doneconstructors.insert(i->second);

						if (ud2->GetUnitDef()->techLevel<techlevel){
							e+=1.0f;
						} else if (ud2->GetUnitDef()->techLevel != techlevel){
							e+= efficiency[i->second]*pow(0.6f, (ud2->GetUnitDef()->techLevel-techlevel));
						} else {
							e+= efficiency[i->second];
						}
					}else{

						if(ud2->GetUnitDef()->techLevel > ud->GetUnitDef()->techLevel){
							e+= 1.0f;
						}

						e += builderefficiency[i->second];
					}

				}

				lastbuilderefficiencyupdate[ud->GetName()] = GetCurrentFrame();
				builderefficiency[ud->GetName()] = e;

				return e;
			}else{
				if(ud->GetUnitDef()->techLevel < techlevel){
					return 1.0f;
				}else  if(ud->GetUnitDef()->techLevel > techlevel){
					return efficiency[ud->GetName()]*pow(0.6f, (ud->GetUnitDef()->techLevel-techlevel));
				}

				return efficiency[ud->GetName()];
			}
		}else{
			L.print("error ::   " + ud->GetName() + " is missing from the efficiency array");
			return 0.0f;
		}
	}

	bool Global::LoadUnitData(){

		if(G->L.FirstInstance()){

			int unum = cb->GetNumUnitDefs();

			const UnitDef** ulist = new const UnitDef*[unum];
			cb->GetUnitDefList(ulist);

			for(int i = 0; i < unum; i++){
				const UnitDef* pud = ulist[i];

				if(pud == 0){
					continue;
				}

				string eu = pud->name;

				tolowercase(eu);
				trim(eu);

				float ef = pud->energyMake + pud->metalMake;

				if(pud->energyCost < 0){
					ef += -pud->energyCost;
				}

				ef *= 2;

				if(pud->weapons.empty() == false){

					for(vector<UnitDef::UnitDefWeapon>::const_iterator k = pud->weapons.begin();k != pud->weapons.end();++k){

						float av=0;
						int numTypes;
						cb->GetValue(AIVAL_NUMDAMAGETYPES, &numTypes);
						for(int a=0;a<numTypes;++a){
							if(a == 0){
								av = k->def->damages[0];//damages
							}else{
								av = (av+k->def->damages[a])/2;
							}
						}
						ef += av;
					}
				}

				ef += pud->power;

				efficiency[eu] = ef;
				unit_names[eu] = pud->humanName;
				unit_descriptions[eu] = pud->tooltip;
			}

			string filename = info->datapath;
			filename += slash;
			filename += "learn";
			filename += slash;
			filename += info->tdfpath;
			filename += ".tdf";

			string* buffer = new string;

			if(ReadFile(filename, buffer)){

				TdfParser cq(this);

				cq.LoadBuffer(buffer->c_str(), buffer->size());
				iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());

				for(map<string, float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					string s = "AI\\";
					s += i->first;
					float ank = (float)atof(cq.SGetValueDef("14", s.c_str()).c_str());
					if(ank > i->second) i->second = ank;
				}

				iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());
				iterations++;

				cq.GetDef(firstload, "1", "AI\\firstload");

				if(firstload){
					L.iprint(" This is the first time this mod has been loaded, up. Take this first game to train NTai up, and be careful of throwing the same units at it over and over again");
					firstload = false;

					for(map<string, float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
						CUnitTypeData* uda = UnitDefLoader->GetUnitTypeDataByName(i->first);
						if(uda){
							i->second += uda->GetUnitDef()->health;
						}
					}

				}

				loaded = true;
				return true;

			} else{

				for(int i = 0; i < unum; i++){
					float ts = 500;
					if(ulist[i]->weapons.empty()){
						ts += ulist[i]->health+ulist[i]->energyMake + ulist[i]->metalMake + ulist[i]->extractsMetal*50+ulist[i]->tidalGenerator*30 + ulist[i]->windGenerator*30;
						ts *= 300;
					}else{
						ts += 20*ulist[i]->weapons.size();
					}

					string eu = ulist[i]->name;
					
					tolowercase(eu);
					trim(eu);
					
					efficiency[eu] = ts;
				}

				SaveUnitData();

				G->L.print("failed to load :" + filename);

				return false;
			}
		}
		return false;
	}

	bool Global::SaveUnitData(){
		NLOG("Global::SaveUnitData()");

		if(L.FirstInstance()){
			ofstream off;

			string filename = info->datapath;
			filename += slash;
			filename += "learn";
			filename += slash;
			filename += info->tdfpath;
			filename += ".tdf";

			off.open(filename.c_str());

			if(off.is_open() == true){

				off << "[AI]" << endl;
				off << "{" << endl;
				off << "    // " << AI_NAME << " AF :: unit efficiency cache file" << endl;
				off << endl;

				off << "    version="<< AI_NAME <<";" << endl;
				off << "    firstload=" << firstload << ";" << endl;
				off << "    modname=" << G->cb->GetModName() << ";" << endl;
				off << "    iterations=" << iterations << ";" << endl;
				off << endl;

				off << "    [VALUES]" << endl << "    {" << endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << i->second << ";";
					off << "// " << unit_names[i->first] << " :: "<< unit_descriptions[i->first]<<endl;
				}

				off << "    }" << endl;
				off << "    [NAMES]" << endl;
				off << "    {"<< endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << unit_names[i->first] << ";" <<endl;
				}

				off << "    }" << endl;

				off << "    [DESCRIPTIONS]" << endl;
				off << "    {"<< endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << unit_descriptions[i->first] << ";"<<endl;
				}

				off << "    }" << endl;
				off << "}" << endl;

				off.close();

				saved = true;
				return true;

			}else{
				G->L.print("failed to save :" + filename);
				off.close();

				return false;
			}
		}

		return false;
	}

	float Global::GetTargettingWeight(string unit, string target){

		float tempscore = 0;

		if(!info->hardtarget){

			tempscore = GetEfficiency(target);

		}else{

			string fh = unit + "\\target_weights\\" + target;
			string fg = unit + "\\target_weights\\undefined";

			float fz = GetEfficiency(target);
			string tempdef = Get_mod_tdf()->SGetValueDef(to_string(fz), fg.c_str());
			
			// load "unitname\\target_weights\\enemyunitname" to retirieve targetting weights
			tempscore = (float)atof(Get_mod_tdf()->SGetValueDef(tempdef, fh).c_str()); 
		}

		return tempscore;
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

	float3 Global::GetUnitPos(int unitid, int enemy){ // do 10 frame delays between updates fo different units
		if(!ValidUnitID(unitid)){
			return UpVector;
		}

		float3 p = chcb->GetUnitPos(unitid);

		if(Map->CheckFloat3(p)==false){
			return UpVector;
		}else{
			return p;
		}

	}

	bool Global::HasUnit(int unit){
		return (units.find(unit) != units.end());
	}

	boost::shared_ptr<IModule> Global::GetUnit(int unit){
		if(HasUnit(unit)==false){
			IModule* a = 0;
			boost::shared_ptr<IModule> t(a);
			return t;
		}

		return units[unit];
	}

	void Global::RegisterMessageHandler(boost::shared_ptr<IModule> handler){
		handlers.insert(handler);
	}

	void Global::FireEvent(CMessage &message){
		if(message.IsType("")){
			return;
		}

		message.SetFrame(GetCurrentFrame());

		msgqueue.push_back(message);
	}

	void Global::DestroyHandler(boost::shared_ptr<IModule> handler){
		handler->DestroyModule();

		if(!handlers.empty()){
			handlers.erase(handler);
		}

		return;
	}

	void Global::RemoveHandler(boost::shared_ptr<IModule> handler){
		dead_handlers.insert(handler);
	}

}

/** Random note
unitdef->type can be one of the following:
 MetalExtractor
 Transport
 Builder
 Factory
 Bomber
 Fighter
 GroundUnit
 Building
 **/
