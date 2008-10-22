#include <iostream>

#include "StdAfx.h"
#include "GlobalAIHandler.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "AILibraryManager.h"
#include "SkirmishAI.h"
#include "SkirmishAIWrapper.h"
#include "IGlobalAI.h"
#include "Game/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/Team.h"
#include "Sim/Units/Unit.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"
#include "System/NetProtocol.h"
#include "System/Platform/errorhandler.h"
#include "mmgr.h"

CGlobalAIHandler* globalAI=0;

CR_BIND_DERIVED(CGlobalAIHandler,CObject,)

CR_REG_METADATA(CGlobalAIHandler, (
				CR_MEMBER(ais),
				CR_MEMBER(hasAI)
//				CR_MEMBER(memBuffers)
				));

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
		if (CatchException ()) {		\
			AIException(e.what());		\
			throw;						\
		} else throw;					\
	}									\
	catch (const char *s) {	\
		if (CatchException ()) {		\
			AIException(s);				\
			throw;						\
		} else throw;					\
	}									\
	catch (...) {						\
		if (CatchException ()) {		\
			AIException(0);				\
			throw;						\
		}else throw;					\
	}

void AIException(const char *what)
{
//	static char msg[512];
	if(what) {
//		SNPRINTF(msg, sizeof(msg), "An exception occured in the global ai dll: \'%s\',\n please contact the author of the AI.",	what);
//		handleerror(0,msg, "Exception in global AI",0);
		logOutput.Print("Global ai exception:\'%s\'",what);
	} else
//		handleerror(0,"An unhandled exception occured in the global ai dll, please contact the author of the ai.","Error in global ai",0);
		logOutput.Print("Global ai exception");
//	exit(-1);
}

CGlobalAIHandler::CGlobalAIHandler()
{
	hasAI=false;

	for(int a=0;a<MAX_TEAMS;++a)
		ais[a]=0;
}

CGlobalAIHandler::~CGlobalAIHandler()
{
	for(int a=0;a<MAX_TEAMS;++a)
		delete ais[a];
}

void CGlobalAIHandler::PostLoad()
{
}

void CGlobalAIHandler::Load(std::istream *s)
{
	try {
		for(int a=0;a<gs->activeTeams;++a)
			if(ais[a])
				ais[a]->Load(s);
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::Save(std::ostream *s)
{
	try {
		for(int a=0;a<gs->activeTeams;++a)
			if(ais[a])
				ais[a]->Save(s);
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::Update()
{
	SCOPED_TIMER("Skimrish AI")
	try {
		int frame = gs->frameNum;
		for(int a=0;a<gs->activeTeams;++a)
			if(ais[a])
				ais[a]->Update(frame);
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::PreDestroy ()
{
	try {
		for(int a=0;a<gs->activeTeams;a++)
			if(ais[a])
				ais[a]->PreDestroy();
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitEnteredLos(CUnit* unit, int allyteam)
{
	if (hasAI) {
		for (int a = 0; a < gs->activeTeams; ++a) {
			if (ais[a] && gs->AllyTeam(a) == allyteam && !gs->Ally(allyteam, unit->allyteam))
				try {
					ais[a]->EnemyEnterLOS(unit->id);
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
					ais[a]->EnemyLeaveLOS(unit->id);
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
					ais[a]->EnemyEnterRadar(unit->id);
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
					ais[a]->EnemyLeaveRadar(unit->id);
				} HANDLE_EXCEPTION;
			}
		}
	}
}

void CGlobalAIHandler::UnitIdle(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitIdle(unit->id);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitCreated(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitCreated(unit->id);
		} 	HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitFinished(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitFinished(unit->id);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitDestroyed(CUnit* unit,CUnit* attacker)
{
	if(hasAI){
		try {
			for(int a=0;a<gs->activeTeams;++a){
				if(ais[a]
						&& !gs->Ally(gs->AllyTeam(a),unit->allyteam)
						&& (ais[a]->IsCheatEventsEnabled() || (unit->losStatus[a] & (LOS_INLOS | LOS_INRADAR))))
					ais[a]->EnemyDestroyed(unit->id, attacker ? attacker->id : 0);
			}
			if(ais[unit->team])
				ais[unit->team]->UnitDestroyed(unit->id,attacker?attacker->id:0);
		} HANDLE_EXCEPTION;
	}
}

/*
void* CGlobalAIHandler::GetAIBuffer(int team, std::string name, int length)
{
	if(memBuffers[team].find(name)!=memBuffers[team].end()){
		memBuffers[team][name].usage++;
		return memBuffers[team][name].mem;
	}
	AIMemBuffer mb;
	mb.usage=1;
	mb.mem=SAFE_NEW char[length];
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
*/

void CGlobalAIHandler::GotChatMsg(const char* msg, int fromPlayerId)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a)
		{
			if(ais[a])
			{
				try {
					ais[a]->GotChatMsg(msg, fromPlayerId);
				} HANDLE_EXCEPTION
			}
		}
	}
}

void CGlobalAIHandler::UnitDamaged(CUnit* attacked, CUnit* attacker, float damage)
{
	if(hasAI){
		try {
			if(ais[attacked->team])
				if(attacker){
					float3 dir=helper->GetUnitErrorPos(attacker,attacked->allyteam)-attacked->pos;
					dir.ANormalize();
					ais[attacked->team]->UnitDamaged(attacked->id,attacker->id,damage,dir);
				} else {
					ais[attacked->team]->UnitDamaged(attacked->id,-1,damage,ZeroVector);
				}

			if(attacker) {
				int a = attacker->team;
				if(ais[attacker->team] && !gs->Ally(gs->AllyTeam(a),attacked->allyteam) && (ais[a]->IsCheatEventsEnabled() || (attacked->losStatus[a] & (LOS_INLOS | LOS_INRADAR)))) {
					float3 dir=attacker->pos-helper->GetUnitErrorPos(attacked,attacker->allyteam);
					dir.ANormalize();
					ais[a]->EnemyDamaged(attacked->id,attacker->id,damage,dir);
				}
			}
		} HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::WeaponFired(CUnit* unit, const WeaponDef* def)
{
	if(ais[unit->team]){
		try {
			ais[unit->team]->WeaponFired(unit->id, def->id);
		} HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::UnitMoveFailed(CUnit* unit)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitMoveFailed(unit->id);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitGiven(CUnit *unit, int oldTeam)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitGiven(unit->id, oldTeam, unit->team);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitTaken(CUnit *unit, int newTeam)
{
	if(ais[unit->team])
		try {
			ais[unit->team]->UnitCaptured(unit->id, unit->team, newTeam);
		} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::PlayerCommandGiven(std::vector<int>& selectedunits, Command& c, int playerId)
{
	if(ais[gs->players[playerId]->team]){
		try {
			ais[gs->players[playerId]->team]->PlayerCommandGiven(selectedunits, c, playerId);
		}
		HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::SeismicPing(int allyTeam, CUnit* unit, const float3& pos, float strength)
{
	if(hasAI){
		for(int a=0;a<gs->activeTeams;++a){
			if(ais[a] && gs->AllyTeam(a)==allyTeam && !gs->Ally(allyTeam, unit->allyteam))
				try {
					ais[a]->SeismicPing(allyTeam, unit->id, pos, strength);
				} HANDLE_EXCEPTION;
		}
	}
}





bool CGlobalAIHandler::CreateSkirmishAI(int teamId, const SSAIKey& skirmishAIKey)
{
	if ((teamId < 0) || (teamId >= gs->activeTeams)) {
		return false;
	}

	if (net->localDemoPlayback) {
		return false;
	}

	//TODO: make this work again
/*
	if (strncmp(dll, "LuaAI:", 6) == 0) {
		CTeam* team = gs->Team(teamId);
		if (team != NULL) {
			team->luaAI = (dll + 6);
			return true;
		}
		return false;
	}
*/

	try {
		if (ais[teamId]) {
			delete ais[teamId];
			ais[teamId] = 0;
		}

		//TODO : This is the crossover to the new AI
		//Eventually we should rewrite a less hacky AILibraryHandler
		//and change all calls to the global ai to be appropriate.
		//ais[teamId] = SAFE_NEW CGlobalAI(teamId, dll);
		//ais[teamId] = SAFE_NEW CAILibraryGlobalAI(dll, teamId);
/*
		const ISkirmishAILibrary* skirmishAILibrary = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(skirmishAIKey);
		skirmishAIs[teamId] = SAFE_NEW CSkirmishAI(teamId, skirmishAILibrary);
*/
		ais[teamId] = SAFE_NEW CSkirmishAIWrapper(teamId, skirmishAIKey);
		
/*
		if (!ais[teamId]->ai) {
			delete ais[teamId];
			ais[teamId] = 0;
			return false;
		}
*/

		hasAI = true;
		return true;
	} HANDLE_EXCEPTION;
	return false;
}

bool CGlobalAIHandler::IsSkirmishAI(int teamId) {
	return ais[teamId] != 0;
}

void CGlobalAIHandler::DestroySkirmishAI(int teamId) {
	
	try {
		delete ais[teamId];
		ais[teamId] = 0;
	} HANDLE_EXCEPTION;
}

const CSkirmishAIWrapper* CGlobalAIHandler::GetSkirmishAI(int teamId) {
	return ais[teamId];
}

