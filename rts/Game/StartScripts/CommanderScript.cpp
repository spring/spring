#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <map>
#include "CommanderScript.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/InfoConsole.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/SideParser.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "mmgr.h"


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
	if (gameSetup) {
		// setup the teams
		for (int a = 0; a < gs->activeTeams; ++a) {

			// don't spawn a commander for the gaia team
			if (gs->useLuaGaia && (a == gs->gaiaTeamID)) {
				continue;
			}

			CTeam* team = gs->Team(a);

			if (team->gaia) continue;

			// remove the pre-existing storage except for a small amount
			team->metalStorage = 20;
			team->energyStorage = 20;

			// create a GlobalAI if required
			if (!team->dllAI.empty() && (gu->myPlayerNum == team->leader)) {
				globalAI->CreateGlobalAI(a, team->dllAI.c_str());
			}

			// get the team startup info
			const std::string& side = team->side;
			const std::string& startUnit = sideParser.GetStartUnit(side);
			if (startUnit.empty()) {
				throw content_error( "Unable to load a commander for side: " + side);
			}

			CUnit* unit =
				unitLoader.LoadUnit(startUnit, team->startPos, a, false, 0, NULL);

			team->lineageRoot = unit->id;

			// FIXME this shouldn't be here, but no better place exists currently
			if (a == gu->myTeam) {
				minimap->AddNotification(team->startPos,
																 float3(1.0f, 1.0f, 1.0f), 1.0f);
				game->infoConsole->SetLastMsgPos(team->startPos);
			}
		}
	}
	else {
		const std::string startUnit0 = sideParser.GetStartUnit(0, "");
		const std::string startUnit1 = sideParser.GetStartUnit(1, startUnit0);

		if (startUnit0.length() == 0) {
			throw content_error("Unable to load a startUnit for the first side");
		}
		
		MapParser mapParser(stupidGlobalMapname);
		if (!mapParser.IsValid()) {
			throw content_error("MapParser: " + mapParser.GetErrorLog());
		}
		float3 startPos0(1000.0f, 80.0f, 1000.0f);
		float3 startPos1(1200.0f, 80.0f, 1200.0f);
		mapParser.GetStartPos(0, startPos0);
		mapParser.GetStartPos(1, startPos1);

		unitLoader.LoadUnit(startUnit0, startPos0, 0, false, 0, NULL);
		unitLoader.LoadUnit(startUnit1, startPos1, 1, false, 0, NULL);

		// FIXME this shouldn't be here, but no better place exists currently
		minimap->AddNotification(startPos0, float3(1.0f, 1.0f, 1.0f), 1.0f);
		game->infoConsole->SetLastMsgPos(startPos0);
	}
}

