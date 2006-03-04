// GroupAI.cpp: implementation of the agent and CGroupAI classes.
// TAI Base template, Redstar & Co
//////////////////////////////////////////////////////////////////////
// Subject to GPL  liscence
//////////////////////////////////////////////////////////////////////

// This class is the root of NTAI. It is not holding any code, all it does
// is initialise NTAI, catch exceptions, and acts as a buffer between
// NTAI and the engine.

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Includes
//Global* GLI=0;
#include "IAICallback.h"
#include "IGlobalAICallback.h"
#include "GlobalAI.h"
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 namespace std{	void _xlen(){};}
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// CGlobalAI class

//// Constructor/Destructor/Initialisation

CGlobalAI::CGlobalAI(int aindex){
}

CGlobalAI::~CGlobalAI(){}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team){
	cg = callback;
	GL = new Global(callback);
	GL->team = team;
	helped = false;
}



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Engine Interface

void CGlobalAI::Update(){
	GL->Update();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitCreated(int unit){
	if(helped == false){
		GL->InitAI(acallback,tteam);
		helped = true;
	}
	if(helped == true) GL->UnitCreated(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitFinished(int unit){
		if(helped == true) GL->UnitFinished(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDestroyed(int unit, int attacker){
		if(helped == true) GL->UnitDestroyed(unit,attacker);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterLOS(int enemy){
		if(helped == true)  GL->EnemyEnterLOS(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveLOS(int enemy){
		if(helped == true)  GL->EnemyLeaveLOS(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterRadar(int enemy){
		if(helped == true) GL->EnemyEnterRadar(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveRadar(int enemy){
		if(helped == true)  GL->EnemyLeaveRadar(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyDestroyed(int enemy,int attacker){
		if(helped == true) GL->EnemyDestroyed(enemy,attacker);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitIdle(int unit){
		if(helped == true)  GL->UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::GotChatMsg(const char* msg,int player){
		if(helped == true)  GL->GotChatMsg(msg,player);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
		if(helped == true)  GL->UnitDamaged(damaged,attacker,damage,dir);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitMoveFailed(int unit){
		if(helped == true)  GL->UnitMoveFailed(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int CGlobalAI::HandleEvent (int msg, const void *data){
	switch (msg) {
		case AI_EVENT_UNITGIVEN: {
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			if(helped == true) GL->UnitCreated(cte->unit);
			if(helped == true) GL->UnitFinished(cte->unit);
			break;
		}
		case AI_EVENT_UNITCAPTURED:{
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			if(helped == true) GL->UnitDestroyed(cte->unit,0);
			break;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//// END OF CODE
//////////////////////////////////////////////////////////////////////////////////////////
