/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#include "StdAfx.h"
#include "TeamHandler.h"

#include <cstring>

#include "Game/GameSetup.h"
#include "Lua/LuaGaia.h"
#include "mmgr.h"
#include "Util.h"
#include "Platform/errorhandler.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/IAILibraryManager.h"

CR_BIND(CTeamHandler,);

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(activeTeams),
	CR_MEMBER(activeAllyTeams),
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

	for(int a = 0; a < MAX_TEAMS; ++a) {
		teams[a].teamNum = a;
		team2allyteam[a] = a;
	}

	for (int a = 0; a < MAX_TEAMS; ++a) {
		for (int b = 0; b < MAX_TEAMS; ++b) {
			allies[a][b] = false;
		}
		allies[a][a] = true;
	}

	activeTeams = 2;
	activeAllyTeams = 2;
}


CTeamHandler::~CTeamHandler()
{
}


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	const bool useLuaGaia  = CLuaGaia::SetConfigString(setup->luaGaiaStr);

	activeTeams = setup->numTeams;
	activeAllyTeams = setup->numAllyTeams;

	assert(activeTeams <= MAX_TEAMS);
	assert(activeAllyTeams <= MAX_TEAMS);

	for (int i = 0; i < activeTeams; ++i) {
		// TODO: this loop body could use some more refactoring
		CTeam* team = Team(i);
		const CGameSetup::TeamData& teamStartingData = setup->teamStartingData[i];

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

	for (int allyTeam1 = 0; allyTeam1 < activeAllyTeams; ++allyTeam1)
	{
		for (int allyTeam2 = 0; allyTeam2 < activeAllyTeams; ++allyTeam2)
			allies[allyTeam1][allyTeam2] = setup->allyStartingData[allyTeam1].allies[allyTeam2];
	}

	if (useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = activeTeams;
		gaiaAllyTeamID = activeAllyTeams;
		activeTeams++;
		activeAllyTeams++;

		// Setup the gaia team
		CTeam* team = Team(gaiaTeamID);
		team->color[0] = 255;
		team->color[1] = 255;
		team->color[2] = 255;
		team->color[3] = 255;
		team->gaia = true;
		team->StartposMessage(float3(0.0, 0.0, 0.0), true);
		//players[setup->numPlayers]->team = gaiaTeamID;
		SetAllyTeam(gaiaTeamID, gaiaAllyTeamID);
	}
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
