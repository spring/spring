// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Helper class implementation.

#define PI 3.141592654f
#define SQUARE_SIZE 8
#define GAME_SPEED 30
#define RANDINT_MAX 0x7fff
#define MAX_VIEW_RANGE 8000
#define MAX_PLAYERS 32
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
#include "GlobalStuff.h"
#include "IAICallback.h"
#include "IGlobalAICallback.h"
#include "IAICheats.h"
#include "MoveInfo.h"
#include "WeaponDefHandler.h"
#include "UnitDef.h"
#include "FeatureHandler.h"
//#include <stdarg.h>
//#include <assert.h> 
//#include <algorithm>
//#include <hash_map>
//#include <boost/bind.hpp>
//#include <stdio.h>
#include "MetalHandler.h"
#include "./Helpers/Log.h"
//#include "./Helpers/GridManager.h"
#include <map>
#include <set>
#include <vector>
#include <deque>
#include "TAgents.h"
#include "UGroup.h"
//#include "./Helpers/arg.h"
#include "./Helpers/InitUtil.h"
//#include "./Helpers/build.h"
#include "./Agents/Register.h"
#include "./Helpers/gui.h"
#include "./Agents/ctaunt.h"
#include "./Agents/Planning.h"
#include "Agents.h"
#include "./Agents/Assigner.h"
#include "./Agents/Chaser.h"
#include "./Agents/Scouter.h"

#define exprint(a) G->L->eprint(a)

#define AI_OTAI 2
#define AI_AAI 3
#define AI_JCAI 4
#define AI_ZCAIN 5
#define AI_NTAI 6
#define AI_KAI 7
#define AI_TAI 8
#define AI_HUMAN 9

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
using namespace std;

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

class Global : base{ // derives from base, thus this is an agent not a global structure.
public:
	virtual ~Global();
	Global(IGlobalAICallback* callback);
	CMetalHandler* M;
	Log* L;
	//GridManager* GM;
	//BuildPlacer* B;
	int team;
	IAICallback* cb;
	IGlobalAICallback* gcb;
	IAICheats* ccb;
	Assigner* As;
	Factor* Fa;
	Scouter* Sc;
	Chaser* Ch;
	Planning* Pl;
	ctaunt* Ct;
	Register* R;
	GUIController* MGUI;
	float3 basepos;
	int comID;
	map<int,int> players; // player nubmer maps to an itneger signifying what this player is, be it an AI or human.
	void InitAI(IAICallback* callback, int team){
		L->iprint("InitAI");
		//B->Init();
		try{
			As->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Assigner::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Pl->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Planner::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Fa->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Factor::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Sc->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Scouter::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Ch->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Chaser::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Ct->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, ctaunt::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			R->InitAI(this);
		} catch(exception e){
			L->eprint("Exception, Register::InitAI");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}
		L->iprint("InitAI finished");
	}
	void UnitCreated(int unit){
		try{
			As->UnitCreated(unit);
		} catch(exception e){
			L->eprint("Exception, Assigner::UnitCreated");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Fa->UnitCreated(unit);
		} catch(exception e){
			L->eprint("Exception, Factor::UnitCreated");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitCreated(unit);
		} catch(exception e){
			L->eprint("Exception, Scouter::UnitCreated");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha);
		}try{
			Ch->UnitCreated(unit);
		} catch(exception e){
			L->eprint("Exception, Chaser::UnitCreated");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			R->UnitCreated(unit);
		} catch(exception e){
			L->eprint("Exception, Register::UnitCreated");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void UnitFinished(int unit){
		//const UnitDef* ud = cb->GetUnitDef(unit);
		//float3 pos = cb->GetUnitPos(unit);
		//if(ud->speed == 0) B->Mark(ud,pos,true);
		try{
			As->UnitFinished(unit);
		} catch(exception e){
			L->eprint("Exception, Assigner::UnitFinished");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->UnitFinished(unit);
		} catch(exception e){
			L->eprint("Exception, Factor::UnitFinished");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitFinished(unit);
		} catch(exception e){
			L->eprint("Exception, Scouter::UnitFinished");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->UnitFinished(unit);
		} catch(exception exc){
			L->eprint("Exception, Chaser::UnitFinished");
			string wha = "Exception!";
			wha += exc.what();
			L->eprint(wha.c_str());
		}
	}
	void UnitDestroyed(int unit){
		//if(ud->speed == 0) B->Mark(ud,pos,false);
		try{
			As->UnitDestroyed(unit);
		}catch(exception e){
			L->eprint("Exception, Assigner::UnitDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
            Fa->UnitDestroyed(unit);
		} catch(exception* e){
			L->eprint("Exception, Factor::UnitDestroyed");
			string wha = "Exception!";
			wha += e->what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitDestroyed(unit);
		} catch(exception* e){
			L->eprint("Exception, Scouter::UnitFinished");
			string wha = "Exception!";
			wha += e->what();
			L->eprint(wha.c_str());
		}try{
			Ch->UnitDestroyed(unit);
		} catch(exception e){
			L->eprint("Exception, Chaser::UnitDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			R->UnitDestroyed(unit);
		} catch(exception e){
			L->eprint("Exception, Register::UnitDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void EnemyEnterLOS(int enemy){
		//const UnitDef* ud = cb->GetUnitDef(enemy);
		//float3 pos = cb->GetUnitPos(enemy);
		//if(ud->speed == 0) B->Mark(ud,pos,true);
		try{
			As->EnemyEnterLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Assigner::EnemyEnterLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->EnemyEnterLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Factor::EnemyEnterLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->EnemyEnterLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Scouter::EnemyEnterLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->EnemyEnterLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Chaser::EnemyEnterLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void EnemyLeaveLOS(int enemy){
		try{
			As->EnemyLeaveLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Assigner::EnemyLeaveLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->EnemyLeaveLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Factor::EnemyLeaveLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->EnemyLeaveLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Scouter::EnemyLeaveLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->EnemyLeaveLOS(enemy);
		} catch(exception e){
			L->eprint("Exception, Chaser::EnemyLeaveLOS");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void EnemyEnterRadar(int enemy){
		try{
			As->EnemyEnterRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Assigner::EnemyEnterRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->EnemyEnterRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Factor::EnemyEnterRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->EnemyEnterRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Scouter::EnemyEnterRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->EnemyEnterRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Chaser::EnemyEnterRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void EnemyLeaveRadar(int enemy){
		try{
			As->EnemyLeaveRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Assigner::EnemyLeaveRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->EnemyLeaveRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Factor::EnemyLeaveRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->EnemyLeaveRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Scouter::EnemyLeaveRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->EnemyLeaveRadar(enemy);
		} catch(exception e){
			L->eprint("Exception, Chaser::EnemyLeaveRadar");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void EnemyDestroyed(int enemy){
		//const UnitDef* ud = cb->GetUnitDef(enemy);
		//float3 pos = cb->GetUnitPos(enemy);
		//if(ud->speed == 0) B->Mark(ud,pos,false);
		try{
			As->EnemyDestroyed(enemy);
		} catch(exception e){
			L->eprint("Exception, Assigner::EnemyDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->EnemyDestroyed(enemy);
		} catch(exception e){
			L->eprint("Exception, Factor::EnemyDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->EnemyDestroyed(enemy);
		} catch(exception e){
			L->eprint("Exception, Scouter::EnemyDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->EnemyDestroyed(enemy);
		} catch(exception e){
			L->eprint("Exception, Chaser::EnemyDestroyed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void UnitIdle(int unit){
		try{
			As->UnitIdle(unit);
		} catch(exception e){
			L->eprint("Exception, Assigner::UnitIdle");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->UnitIdle(unit);
		} catch(exception e){
			L->eprint("Exception, Factor::UnitIdle");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitIdle(unit);
		} catch(exception exc){
			L->eprint("Exception, Scouter::UnitIdle");
			string wha = "Exception!";
			wha += exc.what();
			L->eprint(wha.c_str());
		}try{
			Ch->UnitIdle(unit);
		} catch(exception e){
			L->eprint("Exception, Chaser::UnitIdle");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void GotChatMsg(const char* msg,int player){
		try{
			Ct->GotChatMsg(msg,player);
		} catch(exception e){
			L->eprint("Exception, CTaunt::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			As->GotChatMsg(msg,player);
		} catch(exception e){
			L->eprint("Exception, Assigner::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->GotChatMsg(msg,player);
		} catch(exception e){
			L->eprint("Exception, Factor::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->GotChatMsg(msg,player);
		} catch(exception e){
			L->eprint("Exception, Scouter::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->GotChatMsg(msg,player);
		} catch(exception e){
			L->eprint("Exception, Chaser::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
		try{
			L->Message(msg,player);
			string tmsg = msg;
			string verb = ".verbose";
			if(tmsg == verb){
				if(L->Verbose()){
					L->iprint("Verbose turned on");
				} else L->iprint("Verbose turned off");
			}
			verb = ".crash";
			if(tmsg == verb){
				Crash();
			}
			verb = ".cheat";
			if(tmsg == verb){
				ccb = gcb->GetCheatInterface();
			}
			verb = ".mouse";
			if(tmsg == verb){
				if(R->lines == true){
					R->lines = false;
				}else{
					R->lines = true;
				}
			}
		}catch(exception e){
			L->eprint("Exception, Global::GotChatMsg");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
		//cb->SendTextMsg("medium!",2);
		//cb->SendTextMsg("low",3);
	}
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir){
		try{
			As->UnitDamaged(damaged,attacker,damage,dir);
		} catch(exception e){
			L->eprint("Exception, Assigner::UnitDamaged");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->UnitDamaged(damaged,attacker,damage,dir);
		} catch(exception e){
			L->eprint("Exception, Factor::UnitDamaged");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitDamaged(damaged,attacker,damage,dir);
		} catch(exception e){
			L->eprint("Exception, Scouter::UnitDamaged");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->UnitDamaged(damaged,attacker,damage,dir);
		} catch(exception e){
			L->eprint("Exception, Chaser::UnitDamaged");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void UnitMoveFailed(int unit){
		try{
			As->UnitMoveFailed(unit);
		}catch(exception e){
			L->eprint("Exception, Assigner::UnitMoveFailed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->UnitMoveFailed(unit);
		}catch(exception e){
			L->eprint("Exception, Factor::UnitMoveFailed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->UnitMoveFailed(unit);
		}catch(exception e){
			L->eprint("Exception, Scouter::UnitMoveFailed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->UnitMoveFailed(unit);
		}catch(exception e){
			L->eprint("Exception, Chaser::UnitMoveFailed");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void Update(){
		try{
			As->Update();
		} catch(exception e){
			L->eprint("Exception, Assigner::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			MGUI->Update();
		} catch(exception e){
			L->eprint("Exception, GUIController::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Fa->Update();
		} catch(exception e){
			L->eprint("Exception, Factor::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Sc->Update();
		} catch(exception e){
			L->eprint("Exception, Scouter::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			Ch->Update();
		} catch(exception e){
			L->eprint("Exception, Chaser::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}try{
			R->Update();
		} catch(exception e){
			L->eprint("Exception, Register::Update");
			string wha = "Exception!";
			wha += e.what();
			L->eprint(wha.c_str());
		}
	}
	void Crash(){
		L->eprint("The user has initiated a crash, terminating NTAI");
		L->Close();
		vector<string> cv;
		string n = cv.back();
	}
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
