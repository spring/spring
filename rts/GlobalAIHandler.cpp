#include "StdAfx.h"
#include "GlobalAIHandler.h"
#include "GlobalAI.h"
#include "Unit.h"
#include "IGlobalAI.h"
#include "InfoConsole.h"
#include "GameHelper.h"
#include "TimeProfiler.h"
#include "ConfigHandler.h"
#include "errorhandler.h"
//#include "mmgr.h"

CGlobalAIHandler* globalAI=0;
static bool CatchException()
{
	static bool init=false;
	static bool Catch;
	if (!init) {
		Catch=configHandler.GetInt ("CatchAIExceptions", 1)!=0;
		init=true;
	}
	return Catch;
}


// to switch off the exception handling and have it catched by the debugger.
#define HANDLE_EXCEPTION if (CatchException ()) { AIException(); } else { throw; }

static void AIException()
{
	handleerror(0,"An unhandled exception occured in the global ai dll, please contact the author of the ai.","Error in global ai",0);
	exit(-1);
}

CGlobalAIHandler::CGlobalAIHandler(void)
{
	hasAI=false;

	for(int a=0;a<MAX_TEAMS;++a)
		ais[a]=0;
}

CGlobalAIHandler::~CGlobalAIHandler(void)
{
	for(int a=0;a<MAX_TEAMS;++a)
		delete ais[a];

	for(int team=0;team<MAX_TEAMS;++team){
		for(std::map<std::string,AIMemBuffer>::iterator mi=memBuffers[team].begin();mi!=memBuffers[team].end();++mi){
			delete mi->second.mem;
		}
	}
}

void CGlobalAIHandler::Update(void)
{
	START_TIME_PROFILE
		try {
			for(int a=0;a<gs->activeTeams;++a)
				if(ais[a])
					ais[a]->Update();
		} catch (...){
			HANDLE_EXCEPTION;
		}
		END_TIME_PROFILE("Global AI")
}

void CGlobalAIHandler::PreDestroy ()
{
	try {
		for(int a=0;a<gs->activeTeams;a++)
			if(ais[a])
				ais[a]->PreDestroy ();
	} catch (...){
		HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::UnitEnteredLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyEnterLOS(unit->id);
				} catch (...){
					HANDLE_EXCEPTION;
				}
		}
	}
}

void CGlobalAIHandler::UnitLeftLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyLeaveLOS(unit->id);
				} catch (...){
					HANDLE_EXCEPTION;
				}
		}
	}
}

void CGlobalAIHandler::UnitEnteredRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyEnterRadar(unit->id);
				} catch (...){
					HANDLE_EXCEPTION;
				}
		}
	}
}

void CGlobalAIHandler::UnitLeftRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyLeaveRadar(unit->id);
				} catch (...){
					HANDLE_EXCEPTION;
				}
		}
	}
}

void CGlobalAIHandler::UnitIdle(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitIdle(unit->id);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

void CGlobalAIHandler::UnitCreated(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitCreated(unit->id);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

void CGlobalAIHandler::UnitFinished(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitFinished(unit->id);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

void CGlobalAIHandler::UnitDestroyed(CUnit* unit)
{
	if(hasAI){
		try {
			for(int a=0;a<gs->activeTeams;++a){
 				if(ais[a] && !gs->Ally(gs->AllyTeam(a),unit->allyteam) && (unit->losStatus[a] & (LOS_INLOS | LOS_INRADAR)))
					ais[a]->ai->EnemyDestroyed(unit->id);
			}
			if(ais[unit->team])
				ais[unit->team]->ai->UnitDestroyed(unit->id);
		} catch (...){
			HANDLE_EXCEPTION;
		}
	}
}

bool CGlobalAIHandler::CreateGlobalAI(int team, const char* dll)
{
	try {
		if(team>=gs->activeTeams)
			return false;

		if(ais[team]){
			delete ais[team];
			ais[team]=0;
		}

		ais[team]=new CGlobalAI(team,dll);

		if(!ais[team]->ai){
			delete ais[team];
			ais[team]=0;
			return false;
		}
		hasAI=true;
		return true;
	} catch (...){
		HANDLE_EXCEPTION;
	}
}

void* CGlobalAIHandler::GetAIBuffer(int team, std::string name, int length)
{
	if(memBuffers[team].find(name)!=memBuffers[team].end()){
		memBuffers[team][name].usage++;
		return memBuffers[team][name].mem;
	}
	AIMemBuffer mb;
	mb.usage=1;
	mb.mem=new char[length];
	memset(mb.mem,0,length);

	memBuffers[team][name]=mb;
	return mb.mem;
}

void CGlobalAIHandler::ReleaseAIBuffer(int team, std::string name)
{
	if(memBuffers[team].find(name)!=memBuffers[team].end()){
		memBuffers[team][name].usage--;
		if(memBuffers[team][name].usage==0){
			delete memBuffers[team][name].mem;
			memBuffers[team].erase(name);
		}
	}
}

void CGlobalAIHandler::GotChatMsg(const char* msg, int player)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a])
				try {
					ais[a]->ai->GotChatMsg(msg,player);
				} catch (...){
					HANDLE_EXCEPTION;
				}
		}
	}
}

void CGlobalAIHandler::UnitDamaged(CUnit* attacked,CUnit* attacker,float damage)
{
	if(ais[attacked->team]){
		try {
			if(attacker){
				float3 dir=helper->GetUnitErrorPos(attacker,attacked->allyteam)-attacked->pos;
				dir.Normalize();
				ais[attacked->team]->ai->UnitDamaged(attacked->id,attacker->id,damage,dir);
			} else {
				ais[attacked->team]->ai->UnitDamaged(attacked->id,-1,damage,ZeroVector);
			}
		} catch (...){
			HANDLE_EXCEPTION;
		}
	}
}

void CGlobalAIHandler::UnitMoveFailed(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitMoveFailed (unit->id);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

void CGlobalAIHandler::UnitGiven(CUnit *unit,int oldteam)
{
	if(ais[unit->team])
		try {
			IGlobalAI::ChangeTeamEvent cte;
			cte.newteam = unit->team;
			cte.oldteam = oldteam;
			cte.unit = unit->id;
			ais[unit->team]->ai->HandleEvent (AI_EVENT_UNITGIVEN, &cte);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

void CGlobalAIHandler::UnitTaken (CUnit *unit,int newteam)
{
	if(ais[unit->team])
		try {
			IGlobalAI::ChangeTeamEvent cte;
			cte.newteam = newteam;
			cte.oldteam = unit->team;
			cte.unit = unit->id;
			ais[unit->team]->ai->HandleEvent (AI_EVENT_UNITCAPTURED, &cte);
		} catch (...){
			HANDLE_EXCEPTION;
		}
}

