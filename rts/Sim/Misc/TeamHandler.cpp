/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#include "StdAfx.h"
#include "TeamHandler.h"

#include <cstring>

#include "Game/GameSetup.h"
#include "Lua/LuaGaia.h"
#include "Sim/Misc/GlobalConstants.h"
#include "mmgr.h"
#include "Util.h"
#include "Platform/errorhandler.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/IAILibraryManager.h"

CR_BIND(CTeamHandler,);

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(allies),
	CR_MEMBER(team2allyteam),
	CR_MEMBER(teams),
	CR_RESERVED(64)
));


CTeamHandler* teamHandler;


CTeamHandler::CTeamHandler()
{
	gaiaTeamID = -1;
	gaiaAllyTeamID = -1;
}


CTeamHandler::~CTeamHandler()
{
}


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	const bool useLuaGaia = CLuaGaia::SetConfigString(setup->luaGaiaStr);

	const size_t activeTeams = setup->numTeams;
	assert(activeTeams <= MAX_TEAMS);
	teams.resize(activeTeams);
	team2allyteam.resize(activeTeams);

	const size_t activeAllyTeams = setup->numAllyTeams;
	assert(activeAllyTeams <= MAX_TEAMS);

	for (size_t i = 0; i < activeTeams; ++i) {
		// TODO: this loop body could use some more refactoring
		CTeam* team = Team(i);
		const CGameSetup::TeamData& teamStartingData = setup->teamStartingData[i];
		team->teamNum = i;
		team->metal = setup->startMetal;
		team->metalIncome = setup->startMetal; // for the endgame statistics

		team->energy = setup->startEnergy;
		team->energyIncome = setup->startEnergy;

		float3 start(teamStartingData.startPos.x, teamStartingData.startPos.y, teamStartingData.startPos.z);
		team->StartposMessage(start, (setup->startPosType != CGameSetup::StartPos_ChooseInGame));
		std::memcpy(team->color, teamStartingData.color, 4);
		team->handicap = teamStartingData.handicap;
		team->leader = teamStartingData.leader;
		team->side = teamStartingData.side;
		SetAllyTeam(i, teamStartingData.teamAllyteam);

		const SkirmishAIData* skirmishAIData =
				setup->GetSkirmishAIDataForTeam(i);

		if (skirmishAIData != NULL) {
			if (skirmishAIData->isLuaAI) {
				team->luaAI = skirmishAIData->shortName;
				team->isAI = true;
			} else {
				if (setup->hostDemo) {
					team->skirmishAIKey = SkirmishAIKey(); // unspecifyed AI Key
				} else {
					const char* sn = skirmishAIData->shortName.c_str();
					const char* v = skirmishAIData->version.empty()
							? NULL : skirmishAIData->version.c_str();
					SkirmishAIKey spec = SkirmishAIKey(sn, v);
					SkirmishAIKey fittingKey =
							IAILibraryManager::GetInstance()->ResolveSkirmishAIKey(spec);
					if (!fittingKey.IsUnspecified()) {
						team->skirmishAIKey = fittingKey;
						team->skirmishAIOptions = skirmishAIData->options;
						team->isAI = true;
					} else {
						const int MAX_MSG_LENGTH = 511;
						char s_msg[MAX_MSG_LENGTH + 1];
						SNPRINTF(s_msg, MAX_MSG_LENGTH,
								"Specifyed Skirmish AI could not be found: %s (version: %s)",
								spec.GetShortName().c_str(), spec.GetVersion() != "" ? spec.GetVersion().c_str() : "<not specifyed>");
						handleerror(NULL, s_msg, "Team Handler Error", MBF_OK | MBF_EXCL);
					}
				}
			}
		}
	}

	for (size_t allyTeam1 = 0; allyTeam1 < activeAllyTeams; ++allyTeam1)
	{
		allies.push_back(setup->allyStartingData[allyTeam1].allies);
	}

	if (useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = static_cast<int>(activeTeams);
		gaiaAllyTeamID = static_cast<int>(activeAllyTeams);

		// Setup the gaia team
		CTeam team;
		team.color[0] = 255;
		team.color[1] = 255;
		team.color[2] = 255;
		team.color[3] = 255;
		team.gaia = true;
		team.teamNum = gaiaTeamID;
		team.StartposMessage(float3(0.0, 0.0, 0.0), true);
		teams.push_back(team);
		team2allyteam.push_back(gaiaAllyTeamID);
		for (size_t allyTeam1 = 0; allyTeam1 < activeAllyTeams; ++allyTeam1)
		{
			allies[allyTeam1].push_back(false); // enemy to everyone
		}
		allies.push_back(std::vector<bool>(activeAllyTeams+1,false)); // everyones enemy
		allies[activeAllyTeams][activeAllyTeams] = true; // peace with itself
	}
	assert(team2allyteam.size() == teams.size());
	assert(teams.size() <= MAX_TEAMS);
}

CTeam* CTeamHandler::Team(int i) {

	assert(i >= 0 && (size_t)i < teams.size());
	return &teams[i];
}
bool CTeamHandler::Ally(int a, int b) {
	return allies[a][b];
}
int CTeamHandler::AllyTeam(int team) {
	return team2allyteam[team];
}
bool CTeamHandler::AlliedTeams(int a, int b) {
	return allies[team2allyteam[a]][team2allyteam[b]];
}
void CTeamHandler::SetAllyTeam(int team, int allyteam) {
	team2allyteam[team] = allyteam;
}
void CTeamHandler::SetAlly(int allyteamA, int allyteamB, bool allied) {
	allies[allyteamA][allyteamB] = allied;
}
int CTeamHandler::GaiaTeamID() const {
	return gaiaTeamID;
}
int CTeamHandler::GaiaAllyTeamID() const {
	return gaiaAllyTeamID;
}
int CTeamHandler::ActiveTeams() const {
	return teams.size();
}
int CTeamHandler::ActiveAllyTeams() const {
	return allies.size();
}

void CTeamHandler::GameFrame(int frameNum)
{
	if (!(frameNum & 31)) {
		for (int a = 0; a < ActiveTeams(); ++a) {
			Team(a)->ResetFrameVariables();
		}
		for (int a = 0; a < ActiveTeams(); ++a) {
			Team(a)->SlowUpdate();
		}
	}
}
