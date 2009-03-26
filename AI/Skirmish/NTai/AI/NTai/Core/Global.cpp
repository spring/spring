//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

#include "include.h"

namespace ntai {

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	TdfParser* Global::Get_mod_tdf(){
		return info->mod_tdf;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

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
		if(id < 0) return false;
		if(id > MAX_UNITS+1) return false;
		return true;
	}

	Global::Global(IGlobalAICallback* callback){
		G = this;
		gcb = callback;
		cb = gcb->GetAICallback();
		if(cb == 0){
			throw string(" error cb ==0");
		}
		CLOG("Started Global::Global class constructor");

		unit_array = new CUnit*[MAX_UNITS];
		memset(unit_array,0,sizeof(CUnit*)*MAX_UNITS);

		CLOG("Starting CCached initialisation");
		Cached = new CCached(this);

		CLOG("Creating Config holder class");
		info = new CConfigData(G);

		CLOG("Setting the Logger class");
		L.Set(this);

		CLOG("Opening logfile in plaintext");
		L.Open(true);
		//L.Verbose();

		CLOG("Logging class Opened");
		L.print("logging started");
		
		CLOG("Retrieving cheat interface");
		chcb = callback->GetCheatInterface();
		CLOG("cheat interface retrieved");

		CLOG("Creating Actions class");
		Actions = new CActions(G);

		CLOG("Creating Map class");
		Map = new CMap(G);

		CLOG("Map class created");
		if(L.FirstInstance() == true){
			CLOG("First Instance == true");
		}

		CLOG("Creating MetalHandler class");
		M = new CMetalHandler(G);

		CLOG("Loading Metal cache");
		M->loadState();

		CLOG("View The NTai Log file from here on");

		//if(L.FirstInstance() == true){
		L << " :: Found " << M->m->NumSpotsFound << " Metal Spots" << endline;
		//}
		UnitDefLoader = new CUnitDefLoader(G);
		L.print("Unitdef loader constructed");

		efficiency = new CEfficiency(this);
		efficiency->Init();
		L.print("Efficiency initialized");

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

		Ch = new Chaser();
		L.print("Chaser constructed");
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	Global::~Global(){
		efficiency->SaveUnitData();
		L.Close();
		delete[] Cached->encache;
		delete[] unit_array;
		delete efficiency;
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

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::Update(){
		bool paused = false;

		NLOG("Global::Update()");
		cb->GetValue(AIVAL_GAME_PAUSED, &paused);
		if(paused){
			return;
		}

		START_EXCEPTION_HANDLING
		if(!msgqueue.empty()){
			bool loopfinished = true;
			int n = 0;
			if(!handlers.empty()){
			
				
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
					for(int i = 0; i < MAX_UNITS; i++){
						CUnit* u = unit_array[i];
						if(u){
							u->RecieveMessage(*mi);
						}
					}

					// We dont want to do everything at once if the queue is gigantic
					if(n >15){
						// so erase what we've already parsed and exit the loop, we
						// can do the rest of the queue in the next update call.
						msgqueue.erase(msgqueue.begin(),mi);
						loopfinished = false;
						break;
					}
				}	
			}
			if(loopfinished){
				msgqueue.erase(msgqueue.begin(),msgqueue.end());
			}
		}

		if(cb->GetCurrentFrame() == (1 SECOND)){
			NLOG("STARTUP BANNER IN Global::Update()");

			if(L.FirstInstance()){
				string s = string(":: ") + AI_NAME + string(" by AF");
				cb->SendTextMsg(s.c_str(), 0);
				cb->SendTextMsg(":: Copyright (C) 2006 AF", 0);
				string q = string(" :: ") + Get_mod_tdf()->SGetValueMSG("AI\\message");
				if(q != string("")){
					cb->SendTextMsg(q.c_str(), 0);
				}
				cb->SendTextMsg("Please check www.darkstars.co.uk for updates", 0);
			}

			int* ax = new int[MAX_UNITS];
			int anum =cb->GetFriendlyUnits(ax);
			
			ComName = string("");
	        
			if(anum !=0){
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
			delete[] ax;
		}
		END_EXCEPTION_HANDLING("Startup Banner and getting commander name")

		START_EXCEPTION_HANDLING
		OrderRouter->Update();
		END_EXCEPTION_HANDLING("OrderRouter->Update()")

		//if( EVERY_((2 SECONDS)) && (Cached->cheating == true) ){
		//	float a = cb->GetEnergyStorage() - cb->GetEnergy();
		//	if( a > 50) chcb->GiveMeEnergy(a);
		//	a = cb->GetMetalStorage() - cb->GetMetal();
		//	if(a > 50) chcb->GiveMeMetal(a);
		//}

		START_EXCEPTION_HANDLING
		if(EVERY_((2 MINUTES))){
			//L.print("saving UnitData");
			efficiency->SaveUnitData();
			/*if(!SaveUnitData()){
			 L.print("UnitData saved");
			 }else{
			 L.print("UnitData not saved");
			 }*/
		}
		END_EXCEPTION_HANDLING("SaveUnitData()")

		//EXCEPTION_HANDLER(Pl->Update(),"Pl->Update()",NA)

		START_EXCEPTION_HANDLING
		Actions->Update();
		END_EXCEPTION_HANDLING("Actions->Update()")

		START_EXCEPTION_HANDLING

		CMessage message("update");

		// a lifetime of 15 frames, so there should never be more than
		// a backlog of 15 valid update messages
		message.SetLifeTime(15);
		
		FireEvent(message);

		END_EXCEPTION_HANDLING("CMessage message(\"update\"); FireEvent(message);")

		//EXCEPTION_HANDLER(Manufacturer->Update(),"Manufacturer->Update()",NA)
		Ch->Update();

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

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

		if(!ValidUnitID(unit)){
			return;
		}
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
		CUnit* u = new CUnit(G, unit);
		unit_array[unit] = u;
		ITaskManager* taskManager = new CConfigTaskManager(G,unit);
		u->SetTaskManager(taskManager);
		u->Init();
		//RegisterMessageHandler(Unit);
		CMessage message("unitcreated");
		message.AddParameter(unit);
		FireEvent(message);
		END_EXCEPTION_HANDLING("CMessage message(\"unitcreated\"); FireEvent(message);")

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
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
				float e = efficiency->GetEfficiency(uda->name, uda->power);
				e =200/uda->metalCost;
				efficiency->SetEfficiency(uda->name, e);
			}

			uda = GetUnitDef(enemy);
			if(uda != 0){
				float e = efficiency->GetEfficiency(uda->name, uda->power);
				e -=200/uda->metalCost;
				efficiency->SetEfficiency(uda->name, e);
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

		//START_EXCEPTION_HANDLING
		CUnitTypeData* u = this->UnitDefLoader->GetUnitTypeDataByUnitId(unit);
		if(u ==0){
			return;
		}
		const UnitDef* ud = u->GetUnitDef();
		if(ud!=0){
			if(ud->isCommander){
				G->Cached->comID = unit;
			}
			max_energy_use += ud->energyUpkeep;

			// prepare unitname
			string t = u->GetName();

			// solo build cleanup

			// Regardless of wether the unit is subject to this behaviour the value of
			// solobuildactive will always be false, so why bother running a check?
			u->SetSoloBuildActive(false);

			if(ud->movedata == 0){
				if(!ud->canfly){
					if(!ud->builder){
						float3 upos = GetUnitPos(unit);
						if(upos != UpVector){
							DTHandler->AddRing(upos, 500.0f, float(PI_2) / 6.0f);
							DTHandler->AddRing(upos, 700.0f, float(-PI_2) / 6.0f);
						}
					}
				}
			}
		}
		//END_EXCEPTION_HANDLING("Sorting solobuild additions and DT Rings in Global::UnitFinished ")

		//START_EXCEPTION_HANDLING
		Manufacturer->UnitFinished(unit);
		//END_EXCEPTION_HANDLING("Manufacturer->UnitFinished")

		//START_EXCEPTION_HANDLING
		Ch->UnitFinished(unit);
		//END_EXCEPTION_HANDLING("Ch->UnitFinished")

		//START_EXCEPTION_HANDLING
		CMessage message("unitfinished");
		message.AddParameter(unit);
		FireEvent(message);
		//END_EXCEPTION_HANDLING("CMessage message(\"unitfinished\");")

	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitMoveFailed(int unit){

		/*

		START_EXCEPTION_HANDLING
		Manufacturer->UnitMoveFailed(unit);
		END_EXCEPTION_HANDLING("Manufacturer->UnitIdle in UnitMoveFailed")

		START_EXCEPTION_HANDLING
		Ch->UnitMoveFailed(unit);
		END_EXCEPTION_HANDLING("Ch->UnitIdle in UnitMoveFailed")*/

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

		//EXCEPTION_HANDLER(Manufacturer->UnitIdle(unit),"Manufacturer->UnitIdle",NA)

		//EXCEPTION_HANDLER(Actions->UnitIdle(unit),"Actions->UnitIdle",NA)

		START_EXCEPTION_HANDLING
		Ch->UnitIdle(unit);
		END_EXCEPTION_HANDLING("Ch->UnitIdle")
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	void Global::Crash(){
		NLOG(" Deliberate Global::Crash routine");
		// close the logfile
		efficiency->SaveUnitData();
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

		if(damage <= 0) return;
		if(!ValidUnitID(damaged)) return;
		if(!ValidUnitID(attacker)) return;

		//START_EXCEPTION_HANDLING
		if(cb->GetUnitAllyTeam(attacker) == cb->GetUnitAllyTeam(damaged)){
			return;
		}
		//END_EXCEPTION_HANDLING("Global::UnitDamaged, filtering out bad calls")

		//START_EXCEPTION_HANDLING
		const UnitDef* uda = GetUnitDef(attacker);

		if(uda != 0){
			float e = efficiency->GetEfficiency(uda->name, uda->power);
			e += 10000/uda->metalCost;
			efficiency->SetEfficiency(uda->name, e);
		}
		const UnitDef* udb = GetUnitDef(damaged);

		if(udb != 0){
			float e = efficiency->GetEfficiency(udb->name, udb->power);
			e -= 10000/uda->metalCost;
			efficiency->SetEfficiency(udb->name, e);
			/*if(udb->builder && UnitDefHelper->IsMobile(udb)&&udb->weapons.empty()){
				// if ti isnt currently building something then retreat
				const CCommandQueue* uc = cb->GetCurrentUnitCommands(damaged);
				if(uc != 0){
					//
					if(uc->front().id >= 0){
						G->Actions->Retreat(damaged);
					}
				}
			}*/
		}
		//END_EXCEPTION_HANDLING("Global::UnitDamaged, threat value handling")

		//START_EXCEPTION_HANDLING
		Actions->UnitDamaged(damaged, attacker, damage, dir);
		//END_EXCEPTION_HANDLING("Actions->UnitDamaged()")

		Ch->UnitDamaged(damaged, attacker, damage, dir);

		/*START_EXCEPTION_HANDLING*/
		CMessage message("unitdamaged");
		message.AddParameter(damaged);
		message.AddParameter(attacker);
		message.AddParameter(damage);
		message.AddParameter(dir);
		FireEvent(message);
		/*END_EXCEPTION_HANDLING("CMessage message(\"unitdamaged\"); FireEvent(message);")*/
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	void Global::GotChatMsg(const char* msg, int player){
		L.Message(msg, player);
		string tmsg = msg;
		if(tmsg == string(".verbose")){
			if(L.Verbose()){
				L.iprint("Verbose turned on");
			} else L.iprint("Verbose turned off");
		}else if(tmsg == string(".crash")){
			Crash();
		}else if(tmsg == string(".end")){
			exit(0);
		}else if(tmsg == string(".break")){
			if(L.FirstInstance() == true){
				L.print("The user initiated debugger break");
			}
		}else if(tmsg == string(".isfirst")){
			if(L.FirstInstance() == true) L.iprint(" info :: This is the first NTai instance");
		}else if(tmsg == string(".save")){
			if(L.FirstInstance() == true) efficiency->SaveUnitData();
		}else if(tmsg == string(".reload")){
			if(L.FirstInstance() == true) efficiency->LoadUnitData();
		}else if(tmsg == string(".flush")){
			L.Flush();
		}else if(tmsg == string(".threat")){
			Ch->MakeTGA();
		}/*else if(tmsg == string(".gridtest")){
		 if(Ch->gridmaintainer==false) return;
		 float cellvalue = 0;
		 Ch->Grid->SetValuebyIndex(10,299.0f);
		 cellvalue = Ch->Grid->GetValue(10);
		 if(cellvalue==299.0f){
		 G->L.iprint("Test 1 PASSED");
		 }else{
		 G->L.iprint("Test1 FAILED");
		 }
		 cellvalue=0;
		 float3 mpos = float3(2048,0,2048);
		 Ch->Grid->SetValuebyMap(mpos,999.0f);
		 cellvalue=Ch->Grid->GetValuebyMap(mpos);
		 if(cellvalue == 999.0f){
		 L.iprint("Test 2 PASSED");
		 }else{
		 G->L.iprint("Test 2 FAILED");
		 }
		 if(Ch->Grid->GridtoMap(Ch->Grid->MaptoGrid(mpos)) == mpos){
		 G->L.iprint("Test 3 PASSED");
		 }else{
		 G->L.iprint("Test 3 FAILED");
		 }
		 cellvalue=0;
		 float3 gpos = float3(4,0,4);
		 Ch->Grid->SetValuebyGrid(gpos,799.0f);
		 cellvalue=Ch->Grid->GetValuebyGrid(gpos);
		 if(cellvalue == 799.0f){
		 L.iprint("Test 4 PASSED");
		 }else{
		 G->L.iprint("Test 4 FAILED");
		 }
		 if(Ch->Grid->MaptoGrid(Ch->Grid->GridtoMap(gpos)) == gpos){
		 G->L.iprint("Test 5 PASSED");
		 }else{
		 G->L.iprint("Test 5 FAILED");
		 }
		 if(Ch->Grid->IndextoGrid(Ch->Grid->GetIndex(gpos)) == gpos){
		 G->L.iprint("Test 6 PASSED");
		 }else{
		 G->L.iprint("Test 6 FAILED");
		 }
		 Ch->MakeTGA();
		 }*/else if(tmsg == string(".aicheat")){
			 //chcb = G->gcb->GetCheatInterface();
			 if(Cached->cheating== false){
				 Cached->cheating = true;
				 chcb->SetMyHandicap(1000.0f);
				 if(L.FirstInstance() == true) L.iprint("Make sure you've typed .cheat for full cheating!");
				 // Spawn 4 commanders around the starting position
				 CUnitTypeData* ud = this->UnitDefLoader->GetUnitTypeDataByName(ComName);
				 if(ud != 0){
					 float3 pos = Map->basepos;
					 pos = cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000.0f, 0);
					 int ij = chcb->CreateUnit(ComName.c_str(), pos);
					 if(ij != 0) Actions->RandomSpiral(ij);
					 float3 epos = pos;
					 epos.z -= 1300.0f;
					 float angle = float(mrand()%320);
					 pos = G->Map->Rotate(epos, angle, pos);
					 pos =  cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000, 300, 1);
					 ///float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist, int facing);
					 ij = chcb->CreateUnit(ComName.c_str(), pos);
					 if(ij != 0) Actions->RandomSpiral(ij);
					 epos = pos;
					 epos.z -= 900.0f;

					 angle = float(mrand()%320);
					 pos = G->Map->Rotate(epos, angle, pos);
					 pos =  cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000, 300, 0);
					 ///
					 ij = chcb->CreateUnit(ComName.c_str(), pos);
					 if(ij != 0){
						 Actions->RandomSpiral(ij);
					 }
				 }
			 }else if(Cached->cheating == true){
				 Cached->cheating  = false;
				 if(L.FirstInstance() == true)L.iprint("cheating is now disabled therefore NTai will no longer cheat");
			 }
		 }
		//START_EXCEPTION_HANDLING
		CMessage message(string("##")+msg);
		message.AddParameter(player);
		FireEvent(message);
		//END_EXCEPTION_HANDLING("CMessage message(\"msg gotmsg\"); FireEvent(message);")
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::UnitDestroyed(int unit, int attacker){

		if(!ValidUnitID(unit)){//&&ValidUnitID(attacker)))
			return;
		}
		
		CUnit* un = unit_array[unit];
		unit_array[unit] = 0;

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
					

					if(efficiency->efficiency.find(uda->name) != efficiency->efficiency.end()){
						efficiency->SetEfficiency(uda->name, efficiency->GetEfficiency(uda->name,500)+(20000/udu->metalCost));
					}else{
						efficiency->SetEfficiency(uda->name,500);
					}
					if(efficiency->efficiency.find(udu->name) != efficiency->efficiency.end()){
						efficiency->SetEfficiency(uda->name, efficiency->GetEfficiency(udu->name,500)-(10000/uda->metalCost));
					}else{
						efficiency->SetEfficiency(udu->name,500);
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
		
		
		delete un;
		
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Global::InitAI(IAICallback* callback, int team){
		L.print("Initialisising");

		mrand.seed(uint(time(NULL)*team));
		string filename = info->datapath + "\\data\\" +  info->tdfpath + string(".tdf");
		string* buffer = new string();
		TdfParser* q = new TdfParser(this);

		int s =cb->GetFileSize(filename.c_str());
		if(s!=-1){

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
		//if(Cached->cheating == true){
		return chcb->GetEnemyUnits(units, pos, radius);
		/*}else{
		 return cb->GetEnemyUnits(units,pos,radius);
		 }*/
	}


	int Global::GetEnemyUnitsInRadarAndLos(int* units){
		NLOG("Global::GetEnemyUnitsinradarandLOS :: B");
		/*if(GetCurrentFrame() - Cached->lastcacheupdate>  30){
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
		 if(Cached->cheating == true){*/
		return chcb->GetEnemyUnits(units);
		/*	Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
		 for(uint h = 0; h < Cached->enemy_number; h++){
		 units[h] = Cached->encache[h];
		 }
		 ///*}else{
		 Cached->enemy_number = cb->GetEnemyUnitsInRadarAndLos(Cached->encache);
		 }*/
		//return Cached->enemy_number;*/
	}

	int Global::GetEnemyUnits(int* units){
		NLOG("Global::GetEnemyUnits :: B");
		if(GetCurrentFrame() - Cached->lastcacheupdate>  30){
			//if(Cached->cheating == true){
			Cached->enemy_number = chcb->GetEnemyUnits(Cached->encache);
			/*	}else{
			 Cached->enemy_number = cb->GetEnemyUnits(Cached->encache);
			 }
			 */	Cached->lastcacheupdate = cb->GetCurrentFrame();
		}
		for(uint h = 0; h < Cached->enemy_number; h++){
			units[h] = Cached->encache[h];
		}
		return Cached->enemy_number;
	}


	/*float Global::GetTargettingWeight(string unit, string target){
		float tempscore = 0;

		if(info->hardtarget == false){
			tempscore = GetEfficiency(target);
		}else{
			string fh = unit + "\\target_weights\\" + target;
			string fg = unit + "\\target_weights\\undefined";

			float fz = GetEfficiency(target);
			string tempdef = Get_mod_tdf()->SGetValueDef(to_string(fz), fg.c_str());
			tempscore = (float)atof(Get_mod_tdf()->SGetValueDef(tempdef, fh).c_str()); // load "unitname\\target_weights\\enemyunitname" to retirieve targetting weights
		}

		return tempscore;
	}*/


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
		assert(unit >= 0);
		assert(unit < MAX_UNITS);
		return unit_array[unit] != 0;
	}

	CUnit* Global::GetUnit(int unit){
		if(HasUnit(unit)==false){
			return 0;
		}

		return unit_array[unit];
	}

	void Global::RegisterMessageHandler(boost::shared_ptr<IModule> handler){
		handlers.insert(handler);
	}

	void Global::FireEvent(CMessage &message){
		if(message.GetType() == string("")){
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

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

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
