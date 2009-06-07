//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

// This class is the intermediary of NTAI. It is not holding any code,
// all it does is initialize NTAI, catch exceptions, and acts as a buffer
// between NTAI and the engine.

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


#include "CNTai.h"

namespace ntai {
	 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// CNTai class

	//// Constructor/Destructor/Initialisation

	CNTai::CNTai(const SSkirmishAICallback* callback){
		G = 0;
	}

	CNTai::~CNTai(){
		delete G;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::InitAI(IGlobalAICallback* callback, int team){
		cg = callback;
		acallback = cg->GetAICallback();
		Good = true;
	#ifdef EXCEPTION
		try{
			G = new Global(callback);
			G->Cached->team = team;
		}catch(exception& e){
			Good = false;
			acallback->SendTextMsg("error in Global constructor, cannot continue",1);
			acallback->SendTextMsg(e.what(),1);
		}catch(exception* e){
			Good = false;
			acallback->SendTextMsg("error in Global constructor, cannot continue",1);
			acallback->SendTextMsg(e->what(),1);
		}catch(string s){
			Good = false;
			acallback->SendTextMsg("error in Global constructor, cannot continue",1);
			acallback->SendTextMsg(s.c_str(),1);
		}catch(...){
			Good = false;
			acallback->SendTextMsg("error in Global constructor, cannot continue",1);
		}
		if(Good == true){
			try{
				G->InitAI(acallback,team);
			}catch(exception& e){
				Good = false;
				G->L.eprint("error in Global InitAI, cannot continue");
				G->L.eprint(e.what());
			}catch(exception* e){
				Good = false;
				G->L.eprint("error in Global InitAI, cannot continue");
				G->L.eprint(e->what());
			}catch(string s){
				Good = false;
				G->L.eprint("error in Global InitAI, cannot continue");
				G->L.eprint(s);
			}catch(...){
				Good = false;
				G->L.eprint("error in Global InitAI, cannot continue");
			}
		}
	#else
		G = new Global(callback);
		G->Cached->team = team;
		G->InitAI(acallback,tteam);
	#endif
	}



	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Engine Interface

	void CNTai::Update(){
		if(Good == false){
			if(acallback->GetCurrentFrame() == (6 SECONDS)){
				//
				acallback->SendTextMsg("Error :: InitAI() in XE9.79 failed, please notify AF at once",1);
				acallback->SendTextMsg("Error :: www.darkstars.co.uk",1);
			}
			return;
		}
		// No need for exception handling here as it's done internally by the Global object
		G->Update();
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitCreated(int unit, int builder){
		if(Good == false) return;
		G->UnitCreated(unit);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitFinished(int unit){
		if(Good == false) return;
		G->UnitFinished(unit);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitDestroyed(int unit, int attacker){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->UnitDestroyed(unit,attacker);
		}catch(exception& e){
			G->L.eprint("G->unitdestroyed exception");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("G->unitdestroyed exception");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("G->unitdestroyed exception");
		}
	#else
		G->UnitDestroyed(unit,attacker);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyEnterLOS(int enemy){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->EnemyEnterLOS(enemy);
		}catch(exception& e){
			G->L.eprint("enemy enter LOS exception");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("enemy enter LOS exception");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("enemy enter LOS exception");
		}
	#else
		G->EnemyEnterLOS(enemy);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyLeaveLOS(int enemy){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->EnemyLeaveLOS(enemy);
		}catch(exception& e){
			G->L.eprint("eception in G->enemyleaveLOS");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("eception in G->enemyleaveLOS");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("eception in G->enemyleaveLOS");
		}
	#else
		G->EnemyLeaveLOS(enemy);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyEnterRadar(int enemy){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->EnemyEnterRadar(enemy);
		}catch(exception& e){
			G->L.eprint("exception G->enemyenterRadar");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("exception G->enemyenterRadar");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("exception G->enemyenterRadar");
		}
	#else
		G->EnemyEnterRadar(enemy);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyLeaveRadar(int enemy){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->EnemyLeaveRadar(enemy);
		}catch(exception& e){
			G->L.eprint("exception G->enemyleaveradar");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("exception G->enemyleaveradar");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("exception G->enemyleaveradar");
		}
	#else
		G->EnemyLeaveRadar(enemy);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyDamaged(int damaged,int attacker,float damage,float3 dir) {
		if(Good == false) return;
	//START_EXCEPTION_HANDLING
			G->EnemyDamaged(damaged,attacker,damage,dir);
	//END_EXCEPTION_HANDLING("G->enemyleaveradar")
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::EnemyDestroyed(int enemy,int attacker){
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->EnemyDestroyed(enemy,attacker);
		}catch(exception& e){
			G->L.eprint("exception G->EnemyDestroyed");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("exception G->EnemyDestroyed");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("exception G->EnemyDestroyed");
		}
	#else
		G->EnemyDestroyed(enemy,attacker);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitIdle(int unit){
		if(Good == false) return;
		G->UnitIdle(unit);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::GotChatMsg(const char* msg,int player){
		if(string(msg) == string(".freeze")){
			Good=false;
			return;
		}else if(string(msg)==string(".unfreeze")){
			Good=true;
		}
		if(Good == false) return;
	#ifdef EXCEPTION
		try{
			G->GotChatMsg(msg,player);
		}catch(exception& e){
			G->L.eprint("exception G->GotChatMsg");
			G->L.eprint(e.what());
		}catch(exception* e){
			G->L.eprint("exception G->GotChatMsg");
			G->L.eprint(e->what());
		} catch (...) {
			G->L.print("exception G->GotChatMsg");
		}
	#else
		G->GotChatMsg(msg,player);
	#endif
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
		if(Good == false) return;
		G->UnitDamaged(damaged,attacker,damage,dir);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CNTai::UnitMoveFailed(int unit){
		if(Good == false) return;
		G->UnitMoveFailed(unit);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	int CNTai::HandleEvent (int msg, const void *data){
		if(Good == false) return 0;
		switch (msg) {
			case AI_EVENT_UNITGIVEN: {
				ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
	#ifdef EXCEPTION
				try{
					G->UnitCreated(cte->unit);
				}catch(exception& e){
					G->L.eprint("exception G->UnitCreated");
					G->L.eprint(e.what());
				}catch(exception* e){
					G->L.eprint("exception G->UnitCreated");
					G->L.eprint(e->what());
				} catch (...) {
					G->L.print("exception G->UnitCreated");
				}
	#else
				G->UnitCreated(cte->unit);
	#endif
	#ifdef EXCEPTION
				try{
					G->UnitFinished(cte->unit);
				}catch(exception& e){
					G->L.eprint("exception G->UnitFinished");
					G->L.eprint(e.what());
				}catch(exception* e){
					G->L.eprint("exception G->UnitFinished");
					G->L.eprint(e->what());
				} catch (...) {
					G->L.print("exception G->UnitFinished");
				}
	#else
				G->UnitFinished(cte->unit);
	#endif
				break;
			}
			case AI_EVENT_UNITCAPTURED:{
				ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
	#ifdef EXCEPTION
				try{
					G->UnitDestroyed(cte->unit,0);
				}catch(exception& e){
					G->L.eprint("exception G->UnitDestroyed");
					G->L.eprint(e.what());
				}catch(exception* e){
					G->L.eprint("exception G->UnitDestroyed");
					G->L.eprint(e->what());
				} catch (...) {
					G->L.print("exception G->UnitDestroyed");
				}
	#else
				G->UnitDestroyed(cte->unit,0);
	#endif
				break;
			}
		}
		return 0;
	}
}

