// GroupAI.cpp: implementation of the agent and CGroupAI classes.
// TAI Base template, Redstar & Co
//////////////////////////////////////////////////////////////////////
// Subject to GPL  liscence
//////////////////////////////////////////////////////////////////////

// This class is the root of NTAI. It is not holding any code, all it does
// is initialize NTAI, catch exceptions, and acts as a buffer between
// NTAI and the engine.

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Includes
#include "AICallback.h"
#include "IGlobalAICallback.h"
#include "CNTai.h"
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 using namespace std;
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// CNTai class

//// Constructor/Destructor/Initialisation

CNTai::CNTai(){
	GL = 0;
}

CNTai::~CNTai(){
	delete GL;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::InitAI(IGlobalAICallback* callback, int team){
	cg = callback;
	acallback = cg->GetAICallback();
	Good = true;
#ifdef EXCEPTION
	try{
		GL = new Global(callback);
		GL->Cached->team = team;
	}catch(exception e){
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
			GL->InitAI(acallback,tteam);
		}catch(exception e){
			Good = false;
			GL->L.eprint("error in Global InitAI, cannot continue");
			GL->L.eprint(e.what());
		}catch(exception* e){
			Good = false;
			GL->L.eprint("error in Global InitAI, cannot continue");
			GL->L.eprint(e->what());
		}catch(string s){
			Good = false;
			GL->L.eprint("error in Global InitAI, cannot continue");
			GL->L.eprint(s);
		}catch(...){
			Good = false;
			GL->L.eprint("error in Global InitAI, cannot continue");
		}
	}
#else
	GL = new Global(callback);
	GL->Cached->team = team;
	GL->InitAI(acallback,tteam);
#endif
}



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Engine Interface

void CNTai::Update(){
	if(Good == false){
		if(acallback->GetCurrentFrame() == (6 SECONDS)){
			//
			acallback->SendTextMsg("Error :: InitAI() in XE9RC18 failed, please notify AF at once",1);
			acallback->SendTextMsg("Error :: www.darkstars.co.uk",1);
		}
		return;
	}
	// No need for exception handling here as it's done internally by the Global object
	GL->Update();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitCreated(int unit){
	if(Good == false) return;
	GL->UnitCreated(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitFinished(int unit){
	if(Good == false) return;
	GL->UnitFinished(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitDestroyed(int unit, int attacker){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->UnitDestroyed(unit,attacker);
	}catch(exception e){
		GL->L.eprint("gl->unitdestroyed exception");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("gl->unitdestroyed exception");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("gl->unitdestroyed exception");
	}
#else
	GL->UnitDestroyed(unit,attacker);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::EnemyEnterLOS(int enemy){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->EnemyEnterLOS(enemy);
	}catch(exception e){
		GL->L.eprint("enemy enter LOS exception");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("enemy enter LOS exception");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("enemy enter LOS exception");
	}
#else
	GL->EnemyEnterLOS(enemy);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::EnemyLeaveLOS(int enemy){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->EnemyLeaveLOS(enemy);
	}catch(exception e){
		GL->L.eprint("eception in gl->enemyleaveLOS");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("eception in gl->enemyleaveLOS");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("eception in gl->enemyleaveLOS");
	}
#else
	GL->EnemyLeaveLOS(enemy);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::EnemyEnterRadar(int enemy){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->EnemyEnterRadar(enemy);
	}catch(exception e){
		GL->L.eprint("exception gl->enemyenterRadar");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("exception gl->enemyenterRadar");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("exception gl->enemyenterRadar");
	}
#else
	GL->EnemyEnterRadar(enemy);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::EnemyLeaveRadar(int enemy){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->EnemyLeaveRadar(enemy);
	}catch(exception e){
		GL->L.eprint("exception gl->enemyleaveradar");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("exception gl->enemyleaveradar");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("exception gl->enemyleaveradar");
	}
#else
	GL->EnemyLeaveRadar(enemy);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::EnemyDestroyed(int enemy,int attacker){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->EnemyDestroyed(enemy,attacker);
	}catch(exception e){
		GL->L.eprint("exception GL->EnemyDestroyed");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("exception GL->EnemyDestroyed");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("exception GL->EnemyDestroyed");
	}
#else
	GL->EnemyDestroyed(enemy,attacker);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitIdle(int unit){
	if(Good == false) return;
	GL->UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::GotChatMsg(const char* msg,int player){
	if(Good == false) return;
#ifdef EXCEPTION
	try{
		GL->GotChatMsg(msg,player);
	}catch(exception e){
		GL->L.eprint("exception GL->GotChatMsg");
		GL->L.eprint(e.what());
	}catch(exception* e){
		GL->L.eprint("exception GL->GotChatMsg");
		GL->L.eprint(e->what());
	} catch (...) {
		GL->L.print("exception GL->GotChatMsg");
	}
#else
	GL->GotChatMsg(msg,player);
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	if(Good == false) return;
	GL->UnitDamaged(damaged,attacker,damage,dir);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CNTai::UnitMoveFailed(int unit){
	if(Good == false) return;
	GL->UnitMoveFailed(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int CNTai::HandleEvent (int msg, const void *data){
	if(Good == false) return 0;
	switch (msg) {
		case AI_EVENT_UNITGIVEN: {
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
#ifdef EXCEPTION
			try{
				GL->UnitCreated(cte->unit);
			}catch(exception e){
				GL->L.eprint("exception GL->UnitCreated");
				GL->L.eprint(e.what());
			}catch(exception* e){
				GL->L.eprint("exception GL->UnitCreated");
				GL->L.eprint(e->what());
			} catch (...) {
				GL->L.print("exception GL->UnitCreated");
			}
#else
			GL->UnitCreated(cte->unit);
#endif
#ifdef EXCEPTION
			try{
				GL->UnitFinished(cte->unit);
			}catch(exception e){
				GL->L.eprint("exception GL->UnitFinished");
				GL->L.eprint(e.what());
			}catch(exception* e){
				GL->L.eprint("exception GL->UnitFinished");
				GL->L.eprint(e->what());
			} catch (...) {
				GL->L.print("exception GL->UnitFinished");
			}
#else
			GL->UnitFinished(cte->unit);
#endif
			break;
		}
		case AI_EVENT_UNITCAPTURED:{
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
#ifdef EXCEPTION
			try{
				GL->UnitDestroyed(cte->unit,0);
			}catch(exception e){
				GL->L.eprint("exception GL->UnitDestroyed");
				GL->L.eprint(e.what());
			}catch(exception* e){
				GL->L.eprint("exception GL->UnitDestroyed");
				GL->L.eprint(e->what());
			} catch (...) {
				GL->L.print("exception GL->UnitDestroyed");
			}
#else
			GL->UnitDestroyed(cte->unit,0);
#endif
			break;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//// END OF CODE
//////////////////////////////////////////////////////////////////////////////////////////
