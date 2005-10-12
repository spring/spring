#include "StdAfx.h"
#include "GlobalAIHandler.h"
#include "GlobalAI.h"
#include "Unit.h"
#include "IGlobalAI.h"
#include "InfoConsole.h"
#include "GameHelper.h"
#include "TimeProfiler.h"
//#include "mmgr.h"

CGlobalAIHandler* globalAI=0;

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

	for(int a=0;a<gs->activeTeams;++a)
		if(ais[a])
			ais[a]->Update();

	END_TIME_PROFILE("Global AI")
}

void CGlobalAIHandler::PreDestroy ()
{
	for(int a=0;a<gs->activeTeams;a++)
		if(ais[a])
			ais[a]->PreDestroy ();
}

void CGlobalAIHandler::UnitEnteredLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->team2allyteam[a]==allyteam && !gs->allies[allyteam][unit->allyteam])
				ais[a]->ai->EnemyEnterLOS(unit->id);
		}
	}
}

void CGlobalAIHandler::UnitLeftLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->team2allyteam[a]==allyteam && !gs->allies[allyteam][unit->allyteam])
				ais[a]->ai->EnemyLeaveLOS(unit->id);
		}
	}
}

void CGlobalAIHandler::UnitEnteredRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->team2allyteam[a]==allyteam && !gs->allies[allyteam][unit->allyteam])
				ais[a]->ai->EnemyEnterRadar(unit->id);
		}
	}
}

void CGlobalAIHandler::UnitLeftRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->team2allyteam[a]==allyteam && !gs->allies[allyteam][unit->allyteam])
				ais[a]->ai->EnemyLeaveRadar(unit->id);
		}
	}
}

void CGlobalAIHandler::UnitIdle(CUnit* unit)
{
	if(ais[unit->team])
		ais[unit->team]->ai->UnitIdle(unit->id);
}

void CGlobalAIHandler::UnitCreated(CUnit* unit)
{
	if(ais[unit->team])
		ais[unit->team]->ai->UnitCreated(unit->id);
}

void CGlobalAIHandler::UnitFinished(CUnit* unit)
{
	if(ais[unit->team])
		ais[unit->team]->ai->UnitFinished(unit->id);
}

void CGlobalAIHandler::UnitDestroyed(CUnit* unit)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && !gs->allies[gs->team2allyteam[a]][unit->allyteam] && unit->losStatus[a] & (LOS_INLOS | LOS_INRADAR))
				ais[a]->ai->EnemyDestroyed(unit->id);
		}
		if(ais[unit->team])
			ais[unit->team]->ai->UnitDestroyed(unit->id);
	}
}

bool CGlobalAIHandler::CreateGlobalAI(int team, const char* dll)
{
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
				ais[a]->ai->GotChatMsg(msg,player);
		}
	}
}

void CGlobalAIHandler::UnitDamaged(CUnit* attacked,CUnit* attacker,float damage)
{
	if(ais[attacked->team]){
		if(attacker){
			float3 dir=helper->GetUnitErrorPos(attacker,attacked->allyteam)-attacked->pos;
			dir.Normalize();
			ais[attacked->team]->ai->UnitDamaged(attacked->id,attacker->id,damage,dir);
		} else {
			ais[attacked->team]->ai->UnitDamaged(attacked->id,-1,damage,ZeroVector);
		}
	}
}
