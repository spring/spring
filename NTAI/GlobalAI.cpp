// GroupAI.cpp: implementation of the agent and CGroupAI classes.
// TAI Base template, Redstar & Co
//////////////////////////////////////////////////////////////////////
// Subject to GPL  liscence
//////////////////////////////////////////////////////////////////////

// This is a set of classes that should act as a base for any AI built
// ontop of them. Essentially a skirmish AI template structure with
// helper class and agent system. By creating a basic foundation which
// can be updated, it is allowing all that use it to have
// interchangeable code, and thus there should be more help and support,
// aswell as being able to collaborate on certain parts of any AI's
// across several projects.
// If you want to use this, go ahead, however any changes to the helper
// classes and Agent system we make must be included in your AI as we
// make them aswell as any changes you make. Other than giving credit
// where necessary. 

//////////////////////////////////////////////////////////////////////
///// Includes
//////////////////////////////////////////////////////////////////////
#include "IAICallback.h"
#include "IGlobalAICallback.h"
#include "GlobalAI.h"
#include "Agents.h"
//////////////////////////////////////////////////////////////////////
//// Global Objects
//////////////////////////////////////////////////////////////////////
 namespace std{	void _xlen(){};}

//// Interface classes

THelper* ghelper;
IGlobalAICallback* Gcallback;
IAICallback* acallback;
//D_Message err;/* This has been globalised firstly, since more than one error is unlikely to occur at a time, and this way I dont have to implement a last error function.*/

//// Agent Assignments
bool helped;
Assigner* As;
Factor* Fa;
Scouter* Sc;
Chaser* Ch;
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

///////////// CGlobalAI class

//// Constructor/Destructor/Initialisation

CGlobalAI::CGlobalAI(){}
CGlobalAI::~CGlobalAI(){}
int CGlobalAI::HandleEvent(int, const void*) {}

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team){
	::Gcallback = callback;
	acallback =callback->GetAICallback();
	ghelper = new THelper(acallback);
	As = new Assigner;
	this->AddTAgent(As);
	//Sc = new Scouter;
	//this->AddTAgent(Sc);
	Fa = new Factor;
	this->AddTAgent(Fa);
	Ch = new Chaser;
	this->AddTAgent(Ch);
	helped = false;
	ghelper->print("agents created");
}

//// Engine Interface
void CGlobalAI::Update(){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->Update();
	}*/
	As->Update();
	Fa->Update();
	Sc->Update();
	Ch->Update();
}
void CGlobalAI::UnitCreated(int unit){
	if(!helped){
		As->InitAI(acallback, ghelper);
		//Sc->InitAI(acallback,ghelper);
		Fa->InitAI(acallback,ghelper);
		Ch->InitAI(acallback,ghelper);
		ghelper->Init();
		helped = true;
	}
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->UnitCreated(unit);
	}*/
	As->UnitCreated(unit);
	Fa->UnitCreated(unit);
	//Sc->UnitCreated(unit);
	Ch->UnitCreated(unit);
}

void CGlobalAI::UnitFinished(int unit){
	//ghelper->AddUnit(unit);
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->UnitFinished(unit);
	}*/
	As->UnitFinished(unit);
	Fa->UnitFinished(unit);
	//Sc->UnitFinished(unit);
	Ch->UnitFinished(unit);
}

void CGlobalAI::UnitDestroyed(int unit, int attacker){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->UnitDestroyed(unit);
	}*/
	As->UnitDestroyed(unit);
	Fa->UnitDestroyed(unit);
	//Sc->UnitDestroyed(unit);
	Ch->UnitDestroyed(unit);
	//ghelper->RemoveUnit(unit);
}
void CGlobalAI::EnemyEnterLOS(int enemy){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->EnemyEnterLOS(enemy);
	}*/
	As->EnemyEnterLOS(enemy);
	Fa->EnemyEnterLOS(enemy);
	//Sc->EnemyEnterLOS(enemy);
	Ch->EnemyEnterLOS(enemy);
}

void CGlobalAI::EnemyLeaveLOS(int enemy){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->EnemyLeaveLOS(enemy);
	}*/
	As->EnemyLeaveLOS(enemy);
	Fa->EnemyLeaveLOS(enemy);
	//Sc->EnemyLeaveLOS(enemy);
	Ch->EnemyLeaveLOS(enemy);
}
void CGlobalAI::EnemyEnterRadar(int enemy){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->EnemyEnterRadar(enemy);
	}*/
	As->EnemyEnterRadar(enemy);
	Fa->EnemyEnterRadar(enemy);
	//Sc->EnemyEnterRadar(enemy);
	Ch->EnemyEnterRadar(enemy);
}

void CGlobalAI::EnemyLeaveRadar(int enemy){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->EnemyLeaveRadar(enemy);
	}*/
    As->EnemyLeaveRadar(enemy);
	Fa->EnemyLeaveRadar(enemy);
	//Sc->EnemyLeaveRadar(enemy);
	Ch->EnemyLeaveRadar(enemy);
}
void CGlobalAI::EnemyDestroyed(int enemy, int attacker){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->EnemyDestroyed(enemy);
	}*/
	As->EnemyDestroyed(enemy);
	Fa->EnemyDestroyed(enemy);
	//Sc->EnemyDestroyed(enemy);
	Ch->EnemyDestroyed(enemy);
}

void CGlobalAI::UnitIdle(int unit){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->UnitIdle(unit);
	}*/
	As->UnitIdle(unit);
	Fa->UnitIdle(unit);
	//Sc->UnitIdle(unit);
	Ch->UnitIdle(unit);
}

void CGlobalAI::GotChatMsg(const char* msg,int player){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->GotChatMsg(msg,player);
	}*/
	As->GotChatMsg(msg,player);
	Fa->GotChatMsg(msg,player);
	//Sc->GotChatMsg(msg,player);
	Ch->GotChatMsg(msg,player);
}

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	/*T_Agent* Tag;
	for(list<T_Agent*>::iterator Tit = TAgents.begin(); Tit != TAgents.end(); ++Tit){
		Tag = *Tit;
		Tag->UnitDamaged(damaged,attacker,damage,dir);
	}*/
	As->UnitDamaged(damaged,attacker,damage,dir);
	Fa->UnitDamaged(damaged,attacker,damage,dir);
	//Sc->UnitDamaged(damaged,attacker,damage,dir);
	Ch->UnitDamaged(damaged,attacker,damage,dir);
}



//// Agent Functions

void CGlobalAI::AddTAgent(T_Agent* a){
	TAgents.push_back(a);
}

void CGlobalAI::RemoveAgent(T_Agent* a){
	for(list<T_Agent*>::iterator tait = TAgents.begin(); tait != TAgents.end();++tait){
		if(a == *tait){
			TAgents.erase(tait);
		}
	}
}
void CGlobalAI::UnitMoveFailed(int unit){}

//////////////////////////////////////////////////////////////////////////////////////////
//// END OF CODE
//////////////////////////////////////////////////////////////////////////////////////////
