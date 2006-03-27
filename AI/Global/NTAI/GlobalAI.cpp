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
#include "IAICallback.h"
#include "IGlobalAICallback.h"
#include "GlobalAI.h"
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 using namespace std;
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// CGlobalAI class

//// Constructor/Destructor/Initialisation

CGlobalAI::CGlobalAI(){
	GL = 0;
}

CGlobalAI::~CGlobalAI(){
	delete GL;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team){
	cg = callback;
	GL = new Global(callback);
	acallback = cg->GetAICallback();
	GL->team = team;
	GL->InitAI(acallback,tteam);
}



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Engine Interface

void CGlobalAI::Update(){
	GL->Update();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitCreated(int unit){
	//if(GL->team == 567){
	//	GL->InitAI(acallback,tteam);
	//}else{
		GL->UnitCreated(unit);
	//}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitFinished(int unit){
	GL->UnitFinished(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDestroyed(int unit, int attacker){
	GL->UnitDestroyed(unit,attacker);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterLOS(int enemy){
	GL->EnemyEnterLOS(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveLOS(int enemy){
	GL->EnemyLeaveLOS(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterRadar(int enemy){
	GL->EnemyEnterRadar(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveRadar(int enemy){
	GL->EnemyLeaveRadar(enemy);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyDestroyed(int enemy,int attacker){
	GL->EnemyDestroyed(enemy,attacker);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitIdle(int unit){
	GL->UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::GotChatMsg(const char* msg,int player){
	GL->GotChatMsg(msg,player);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	GL->UnitDamaged(damaged,attacker,damage,dir);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitMoveFailed(int unit){
	GL->UnitMoveFailed(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int CGlobalAI::HandleEvent (int msg, const void *data){
	switch (msg) {
		case AI_EVENT_UNITGIVEN: {
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			GL->UnitCreated(cte->unit);
			GL->UnitFinished(cte->unit);
			break;
		}
		case AI_EVENT_UNITCAPTURED:{
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			GL->UnitDestroyed(cte->unit,0);
			break;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//// END OF CODE
//////////////////////////////////////////////////////////////////////////////////////////
