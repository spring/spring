#include "StdAfx.h"
#include "GlobalAIHandler.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "LogOutput.h"
#include "TimeProfiler.h"
#include "NetProtocol.h"
#include "Platform/errorhandler.h"
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

void CGlobalAIHandler::PostLoad()
{
}

void CGlobalAIHandler::Load(std::istream *s)
{
	try {
		for(int a=0;a<teamHandler->ActiveTeams();++a)
			if(ais[a])
				ais[a]->Load(s);
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::Save(std::ostream *s)
{
	try {
		for(int a=0;a<teamHandler->ActiveTeams();++a)
			if(ais[a])
				ais[a]->Save(s);
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::Update(void)
{
	SCOPED_TIMER("Global AI")
	try {
		for(int a=0;a<teamHandler->ActiveTeams();++a)
			if(ais[a])
				ais[a]->Update();
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::PreDestroy ()
{
	try {
		for(int a=0;a<teamHandler->ActiveTeams();a++)
			if(ais[a])
				ais[a]->PreDestroy ();
	} HANDLE_EXCEPTION;
}

void CGlobalAIHandler::UnitEnteredLos(CUnit* unit, int allyteam)
{
	if (hasAI) {
		for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
			if (ais[a] && teamHandler->AllyTeam(a) == allyteam && !teamHandler->Ally(allyteam, unit->allyteam))
				try {
					ais[a]->ai->EnemyEnterLOS(unit->id);
				} HANDLE_EXCEPTION;
		}
	}
}

void CGlobalAIHandler::UnitLeftLos(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<teamHandler->ActiveTeams();++a){
			if(ais[a] && teamHandler->AllyTeam(a)==allyteam && !teamHandler->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyLeaveLOS(unit->id);
				} HANDLE_EXCEPTION;
		}
	}
}

void CGlobalAIHandler::UnitEnteredRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<teamHandler->ActiveTeams();++a){
			if(ais[a] && teamHandler->AllyTeam(a)==allyteam && !teamHandler->Ally(allyteam,unit->allyteam))
				try {
					ais[a]->ai->EnemyEnterRadar(unit->id);
				} HANDLE_EXCEPTION;
		}
	}
}

void CGlobalAIHandler::UnitLeftRadar(CUnit* unit,int allyteam)
{
	if(hasAI){
		for(int a=0;a<teamHandler->ActiveTeams();++a) {
			if(ais[a] && teamHandler->AllyTeam(a)==allyteam && !teamHandler->Ally(allyteam,unit->allyteam))
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
			for(int a=0;a<teamHandler->ActiveTeams();++a){
				if(ais[a] && !teamHandler->Ally(teamHandler->AllyTeam(a),unit->allyteam) && (ais[a]->cheatevents || (unit->losStatus[a] & (LOS_INLOS | LOS_INRADAR))))
					ais[a]->ai->EnemyDestroyed(unit->id,attacker?attacker->id:0);
			}
			if(ais[unit->team])
				ais[unit->team]->ai->UnitDestroyed(unit->id,attacker?attacker->id:0);
		} HANDLE_EXCEPTION;
	}
}

bool CGlobalAIHandler::CreateGlobalAI(int teamID, const char* dll)
{
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return false;
	}

	if (strncmp(dll, "LuaAI:", 6) == 0) {
		CTeam* team = teamHandler->Team(teamID);
		if (team != NULL) {
			team->luaAI = (dll + 6);
			return true;
		}
		return false;
	}

	try {
		if (ais[teamID]) {
			delete ais[teamID];
			ais[teamID] = 0;
		}

		ais[teamID] = SAFE_NEW CGlobalAI(teamID, dll);

		if (!ais[teamID]->ai) {
			delete ais[teamID];
			ais[teamID] = 0;
			return false;
		}

		hasAI = true;
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

void CGlobalAIHandler::GotChatMsg(const char* msg, int player)
{
	if(hasAI){
		for(int a=0;a<teamHandler->ActiveTeams();++a)
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
	if(hasAI){
		try {
			if(ais[attacked->team])
				if(attacker){
					float3 dir=helper->GetUnitErrorPos(attacker,attacked->allyteam)-attacked->pos;
					dir.ANormalize();
					ais[attacked->team]->ai->UnitDamaged(attacked->id,attacker->id,damage,dir);
				} else {
					ais[attacked->team]->ai->UnitDamaged(attacked->id,-1,damage,ZeroVector);
				}

			if(attacker) {
				int a = attacker->team;
				if(ais[attacker->team] && !teamHandler->Ally(teamHandler->AllyTeam(a),attacked->allyteam) && (ais[a]->cheatevents || (attacked->losStatus[a] & (LOS_INLOS | LOS_INRADAR)))) {
					float3 dir=attacker->pos-helper->GetUnitErrorPos(attacked,attacker->allyteam);
					dir.ANormalize();
					ais[a]->ai->EnemyDamaged(attacked->id,attacker->id,damage,dir);
				}
			}
		} HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::WeaponFired(CUnit* unit, const WeaponDef* def)
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
	if(ais[playerHandler->Player(player)->team]){
		try {
			IGlobalAI::PlayerCommandEvent pce;
			pce.units = selectedunits;
			pce.player = player;
			pce.command = c;
			ais[playerHandler->Player(player)->team]->ai->HandleEvent(AI_EVENT_PLAYER_COMMAND,&pce);
		}
		HANDLE_EXCEPTION;
	}
}

void CGlobalAIHandler::SeismicPing(int allyteam, CUnit *unit, const float3 &pos, float strength)
{
	if(hasAI){
		for(int a=0;a<teamHandler->ActiveTeams();++a){
			if(ais[a] && teamHandler->AllyTeam(a)==allyteam && !teamHandler->Ally(allyteam,unit->allyteam))
				try {
					IGlobalAI::SeismicPingEvent spe;
					spe.pos = pos;
					spe.strength = strength;
					ais[a]->ai->HandleEvent(AI_EVENT_SEISMIC_PING,&spe);
				} HANDLE_EXCEPTION;
		}
	}
}
