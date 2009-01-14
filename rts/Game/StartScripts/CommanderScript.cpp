#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <map>

#include "mmgr.h"

#include "CommanderScript.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "Game/Game.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/InfoConsole.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "LogOutput.h"
#include "Exceptions.h"



extern std::string stupidGlobalMapname;


CCommanderScript::CCommanderScript()
: CScript(std::string("Commanders"))
{
}


CCommanderScript::~CCommanderScript(void)
{
}


void CCommanderScript::GameStart()
{
	// setup the teams
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {

		// don't spawn a commander for the gaia team
		if (gs->useLuaGaia && (a == teamHandler->GaiaTeamID())) {
			continue;
		}

		CTeam* team = teamHandler->Team(a);

		if (team->gaia) continue;

		// remove the pre-existing storage except for a small amount
		team->metalStorage = 20;
		team->energyStorage = 20;

		// create a Skirmish AI if required
		if (!team->skirmishAIKey.IsUnspecified() // is an AI specifyed?
				&& (gu->myPlayerNum == team->leader)) {
			eoh->CreateSkirmishAI(a, team->skirmishAIKey);
		}

		if (team->side.empty()) {
			// not a GameSetup-type game, or the script
			// didn't specify a side for this team (bad)
			// NOTE: the gameSetup ptr now always exists
			// even for local games, so we can't rely on
			// its being NULL to identify ones without a
			// script (which have only two teams)
			team->side = sideParser.GetSideName((a % 2), "arm");
		}

		// get the team startup info
		const std::string& startUnit = sideParser.GetStartUnit(team->side);

		if (startUnit.empty()) {
			throw content_error( "Unable to load a commander for side: " + team->side);
		}

		CUnit* unit =
			unitLoader.LoadUnit(startUnit, team->startPos, a, false, 0, NULL);

		team->lineageRoot = unit->id;

		// FIXME this shouldn't be here, but no better place exists currently
		if (a == gu->myTeam) {
			minimap->AddNotification(team->startPos, float3(1.0f, 1.0f, 1.0f), 1.0f);
			game->infoConsole->SetLastMsgPos(team->startPos);
		}
	}
}

