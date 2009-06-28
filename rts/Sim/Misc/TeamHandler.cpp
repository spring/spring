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
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "Platform/errorhandler.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/IAILibraryManager.h"

CR_BIND(CTeamHandler, );

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(teams),
	//CR_MEMBER(allyTeams),
	CR_RESERVED(64)
));


CTeamHandler* teamHandler;


CTeamHandler::CTeamHandler():
	gaiaTeamID(-1),
	gaiaAllyTeamID(-1)
{
}


CTeamHandler::~CTeamHandler()
{
}


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	const bool useLuaGaia = CLuaGaia::SetConfigString(setup->luaGaiaStr);

	assert(setup->teamStartingData.size() <= MAX_TEAMS);
	teams.resize(setup->teamStartingData.size());

	for (size_t i = 0; i < teams.size(); ++i) {
		// TODO: this loop body could use some more refactoring
		CTeam* team = Team(i);
		*team = setup->teamStartingData[i];
		team->teamNum = i;
		team->metalIncome = team->metal; // for the endgame statistics

		team->energyIncome = setup->startEnergy;

		SetAllyTeam(i, team->teamAllyteam);

		const SkirmishAIData* skirmishAIData = setup->GetSkirmishAIDataForTeam(i);

		if (skirmishAIData != NULL) {
			bool isLuaAI = true;
			const IAILibraryManager::T_skirmishAIKeys& skirmishAIKeys = IAILibraryManager::GetInstance()->GetSkirmishAIKeys();
			IAILibraryManager::T_skirmishAIKeys::const_iterator skirmAiImpl;

			for (skirmAiImpl = skirmishAIKeys.begin();
				skirmAiImpl != skirmishAIKeys.end(); ++skirmAiImpl) {
				if (skirmishAIData->shortName == skirmAiImpl->GetShortName()) {
					isLuaAI = false;
					logOutput.Print("Skirmish AI (%s) for team %i is no Lua AI", skirmishAIData->shortName.c_str(), skirmishAIData->team);
					break;
				}
			}

			if (isLuaAI) {
				team->luaAI = skirmishAIData->shortName;
				team->isAI = true;
			} else {
				if (setup->hostDemo) {
					// CPreGame always adds the name of the demo
					// file to the internal setup script before
					// CGameSetup is inited, therefore hostDemo
					// tells us if we're watching a replay
					// if so, then we do NOT want to load any AI
					// libs, and therefore we must make sure each
					// team's skirmishAIKey is left "unspecified"
					//
					// however, flag this team as an AI anyway so
					// the original AI team's orders are not seen
					// as invalid and we don't desync the demo
					team->skirmishAIKey = SkirmishAIKey(); // unspecified AI Key
					team->isAI = true;
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
						// a missing AI lib is only a problem for
						// the player who is supposed to load it
						if (gu->myPlayerNum == skirmishAIData->hostPlayerNum) {
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
	}

	allyTeams = setup->allyStartingData;
	assert(setup->allyStartingData.size() <= MAX_TEAMS);
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
