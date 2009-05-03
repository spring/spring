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
	//CR_MEMBER(allyTeams),
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
		*team = setup->teamStartingData[i];
		team->teamNum = i;
		team->metal = team->startMetal < 0 ? setup->startMetal : team->startMetal;
		team->metalIncome = team->metal; // for the endgame statistics

		team->energy = team->startEnergy < 0 ? setup->startEnergy : team->startEnergy;
		team->energyIncome = setup->startEnergy;

		SetAllyTeam(i, team->teamAllyteam);

		const SkirmishAIData* skirmishAIData =
				setup->GetSkirmishAIDataForTeam(i);

		if (skirmishAIData != NULL) {
			if (skirmishAIData->isLuaAI) {
				team->luaAI = skirmishAIData->shortName;
				team->isAI = true;
			} else {
				if (setup->hostDemo) {
					team->skirmishAIKey = SkirmishAIKey(); // unspecified AI Key
				} else {
					const char* sn = skirmishAIData->shortName.c_str();
					const char* v = skirmishAIData->version.c_str();
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
								"Specified Skirmish AI could not be found: %s (version: %s)",
								spec.GetShortName().c_str(), spec.GetVersion() != "" ? spec.GetVersion().c_str() : "<not specified>");
						handleerror(NULL, s_msg, "Team Handler Error", MBF_OK | MBF_EXCL);
					}
				}
			}
		}
	}

	allyTeams = setup->allyStartingData;

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
		::AllyTeam allyteam;
		allyteam.allies.resize(activeAllyTeams+1,false); // everyones enemy
		allyteam.allies[gaiaTeamID] = true; // peace with itself
		allyTeams.push_back(allyteam);
		for (size_t allyTeam1 = 0; allyTeam1 < activeAllyTeams; ++allyTeam1)
		{
			allyTeams[allyTeam1].allies.push_back(false); // enemy to everyone
		}
	}
	assert(team2allyteam.size() == teams.size());
	assert(teams.size() <= MAX_TEAMS);
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
