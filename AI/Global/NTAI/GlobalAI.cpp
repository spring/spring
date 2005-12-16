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
 namespace std{	void _xlen(){};}
 // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// CGlobalAI class

//// Constructor/Destructor/Initialisation

CGlobalAI::CGlobalAI(int aindex){
	/*ai_index = aindex;
	if(teamx == 34567){
		alone = true;
	}else{
		teamx = 34567;
	}*/
}

CGlobalAI::~CGlobalAI(){}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team){
	cg = callback;
	//callback->GetAICallback()->SendTextMsg("initialising",1);
	try{
        GL = new Global(callback);
	} catch(...){
		//GL->L->eprint("Exception, Init Global");
		callback->GetAICallback()->SendTextMsg("error initializing globals",1);
	}
	//callback->GetAICallback()->SendTextMsg("initialized",1);
	GL->team = team;
	//tteam = team;
	//if(alone == true) callback->GetAICallback()->SendTextMsg("NTAI HAS ONE INSTANCE!",1000);
	helped = false;
}



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Engine Interface

void CGlobalAI::Update(){
	//cg->GetAICallback()->SendTextMsg("update",1);
	//if(badmap == true)	return;
	try{
		GL->Update();
	} catch(...){
			GL->L->eprint("Exception, Update");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitCreated(int unit){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unit created",1);
	if(helped == false){
		GL->InitAI(acallback,tteam);
		helped = true;
	}
	try{
		if(helped == true) GL->UnitCreated(unit);
	} catch(...){
			GL->L->eprint("Exception, UnitCreated");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitFinished(int unit){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unit finished",1);
	try{
		if(helped == true) GL->UnitFinished(unit);
	} catch(...){
			GL->L->eprint("Exception, UnitFinished");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDestroyed(int unit, int attacker){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unit destroyed",1);
	try{
		if(helped == true) GL->UnitDestroyed(unit);
	} catch(...){
			GL->L->eprint("Exception, UnitDestroyed");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterLOS(int enemy){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("enemy enter los",1);
	try{
		if(helped == true)  GL->EnemyEnterLOS(enemy);
	} catch(...){
			GL->L->eprint("Exception, EnemyEnterLOS");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveLOS(int enemy){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("enemy leave LOS",1);
	try{
		if(helped == true)  GL->EnemyLeaveLOS(enemy);
	} catch(...){
			GL->L->eprint("Exception, EnemyLeaveLOS");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyEnterRadar(int enemy){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("enemyenter radar",1);
	try{
		if(helped == true) GL->EnemyEnterRadar(enemy);
	} catch(...){
			GL->L->eprint("Exception, EnemyEnterRadar");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyLeaveRadar(int enemy){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("enemy leave radar",1);
	try{
		if(helped == true)  GL->EnemyLeaveRadar(enemy);
	} catch(...){
			GL->L->eprint("Exception, EnemyLeaveRadar");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::EnemyDestroyed(int enemy,int attacker){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("enemy destroyed",1);
	try{
		if(helped == true) GL->EnemyDestroyed(enemy);
	} catch(...){
			GL->L->eprint("Exception, EnemyDestroyed");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitIdle(int unit){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unit idle",1);
	try{
		if(helped == true)  GL->UnitIdle(unit);
	} catch(...){
		GL->L->eprint("Exception, UnitIdle");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::GotChatMsg(const char* msg,int player){
	// if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("gotchatmsg",1);
	try{
		if(helped == true)  GL->GotChatMsg(msg,player);
	} catch(...){
		GL->L->eprint("Exception, GotChatMsg");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unit damaged",1);
	try{
		if(helped == true)  GL->UnitDamaged(damaged,attacker,damage,dir);
	} catch(...){
			GL->L->eprint("Exception, UnitDamaged");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CGlobalAI::UnitMoveFailed(int unit){
	//if(badmap == true)	return;
	//cg->GetAICallback()->SendTextMsg("unitmovefailed",1);
	try{
		if(helped == true)  GL->UnitMoveFailed(unit);
	} catch(...){
			GL->L->eprint("Exception, UnitMoveFailed");
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int CGlobalAI::HandleEvent (int msg, const void *data){
	//if(badmap == true)	return 0;
	//cg->GetAICallback()->SendTextMsg("handle event",1);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//// END OF CODE
//////////////////////////////////////////////////////////////////////////////////////////
