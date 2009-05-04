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

	assert(setup->numTeams <= MAX_TEAMS);
	teams.resize(setup->numTeams);

	for (size_t i = 0; i < teams.size(); ++i) {
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
	assert(setup->numAllyTeams <= MAX_TEAMS);
	if (useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = static_cast<int>(teams.size());
		gaiaAllyTeamID = static_cast<int>(allyTeams.size());

		// Setup the gaia team
		CTeam team;
		team.color[0] = 255;
		team.color[1] = 255;
		team.color[2] = 255;
		team.color[3] = 255;
		team.gaia = true;
		team.teamNum = gaiaTeamID;
		team.StartposMessage(float3(0.0, 0.0, 0.0), true);
		team.teamAllyteam = gaiaAllyTeamID;
		teams.push_back(team);

		for (std::vector< ::AllyTeam >::iterator it = allyTeams.begin(); it != allyTeams.end(); ++it)
		{
			it->allies.push_back(false); // enemy to everyone
		}
		::AllyTeam allyteam;
		allyteam.allies.resize(allyTeams.size()+1,false); // everyones enemy
		allyteam.allies[gaiaTeamID] = true; // peace with itself
		allyTeams.push_back(allyteam);
	}
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
