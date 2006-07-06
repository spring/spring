#include "StdAfx.h"
#include "GlobalAIHandler.h"
#include "GlobalAI.h"
#include "Sim/Units/Unit.h"
#include "IGlobalAI.h"
#include "Game/UI/InfoConsole.h"
#include "Game/GameHelper.h"
#include "TimeProfiler.h"
#include "Platform/errorhandler.h"
#include "Game/Player.h"
#include "mmgr.h"

CGlobalAIHandler* globalAI=0;

bool CGlobalAIHandler::CatchException()
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
#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {	\
		if (CatchException ())			\
			AIException(e.what());		\
		else throw;						\
	}									\
	catch (...) {						\
		if (CatchException ())			\
			AIException(0);				\
		else throw;						\
	}

static void AIException(const char *what)
{
	static char msg[512];
	if(what) {
		SNPRINTF(msg, sizeof(msg), "An exception occured in the global ai dll: \'%s\',\n please contact the author of the AI.",	what);
		handleerror(0,msg, "Exception in global AI",0);
	} else
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
		} HANDLE_EXCEPTION;
		END_TIME_PROFILE("Global AI")
}

void CGlobalAIHandler::PreDestroy ()
{
	try {
		for(int a=0;a<gs->activeTeams;a++)
			if(ais[a])
				ais[a]->PreDestroy ();
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitEnteredLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyEnterLOS(unit->id);
				} HANDLE_EXCEPTION;
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
				} HANDLE_EXCEPTION;
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
				} HANDLE_EXCEPTION;
		}
	}
}

void CGlobalAIHandler::UnitLeftRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a) {
			if(ais[a] && gs->AllyTeam(a)==allyteam && !gs->Ally(allyteam,unit->allyteam))
			{
				try {
					ais[a]->ai->EnemyLeaveRadar(unit->id);
				} HANDLE_EXCEPTION;
			}
		}
	}
}

void CGlobalAIHandler::UnitIdle(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitIdle(unit->id);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitCreated(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitCreated(unit->id);
		} 	HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitFinished(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitFinished(unit->id);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitDestroyed(CUnit* unit,CUnit* attacker)
{
	if(hasAI){
		try {
			for(int a=0;a<gs->activeTeams;++a){
				if(ais[a] && !gs->Ally(gs->AllyTeam(a),unit->allyteam) && (unit->losStatus[a] & (LOS_INLOS | LOS_INRADAR)))
					ais[a]->ai->EnemyDestroyed(unit->id,attacker?attacker->id:0);
			}
			if(ais[unit->team])
				ais[unit->team]->ai->UnitDestroyed(unit->id,attacker?attacker->id:0);
		} HANDLE_EXCEPTION;
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
	} HANDLE_EXCEPTION;
	return false;
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
		for(int a=0;a<gs->activeTeams;++a)
		{
			if(ais[a]) 
			{
				try {
					ais[a]->ai->GotChatMsg(msg,player);
				} HANDLE_EXCEPTION
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
		} 
		HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::WeaponFired(CUnit* unit,WeaponDef* def)
{
	if(ais[unit->team]){
		try {
			IGlobalAI::WeaponFireEvent wfe;
			wfe.unit = unit->id;
			wfe.def = def;
			ais[unit->team]->ai->HandleEvent(AI_EVENT_WEAPON_FIRED,&wfe);
		} 
		HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::UnitMoveFailed(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->ai->UnitMoveFailed (unit->id);
		} HANDLE_EXCEPTION;
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
		} HANDLE_EXCEPTION;
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
		} 
		HANDLE_EXCEPTION;
}

void CGlobalAIHandler::PlayerCommandGiven(std::vector<int>& selectedunits,Command& c,int player)
{
	if(ais[gs->players[player]->team]){
		try {
			IGlobalAI::PlayerCommandEvent pce;
			pce.units = selectedunits;
			pce.player = player;
			pce.command = c;
			ais[gs->players[player]->team]->ai->HandleEvent(AI_EVENT_PLAYER_COMMAND,&pce);
			//shouldn't "delete pce" be here??
		} 
		HANDLE_EXCEPTION;
	}
}
