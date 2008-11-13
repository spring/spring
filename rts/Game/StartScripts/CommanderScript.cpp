#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <map>

#include "mmgr.h"

#include "CommanderScript.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
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
		if (!SSAIKey_Comparator::IsEmpty(team->skirmishAISpecifier) // is an AI specifyed?
				&& (gu->myPlayerNum == team->leader)) {
			eoh->CreateSkirmishAI(a, team->skirmishAISpecifier, team->skirmishAIOptions);
		}

		// get the team startup info
		// NOTE: team->side isn't set when playing GameSetup-type demos
		// (CGlobalStuff::LoadFromSetup() doesn't get called for them),
		// this needs to be sorted out properly
		const std::string& tside = team->side;
		const std::string& gside = gameSetup->teamStartingData[a].side;
		const std::string& rside = (tside == gside)? tside: gside;
		const std::string& startUnit = sideParser.GetStartUnit(rside);

		if (startUnit.empty()) {
			throw content_error( "Unable to load a commander for side: " + rside);
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

