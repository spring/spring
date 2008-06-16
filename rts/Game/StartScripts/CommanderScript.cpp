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
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
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
		LuaParser luaParser("gamedata/sidedata.lua",
		                    SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
		if (!luaParser.Execute()) {
			logOutput.Print(luaParser.GetErrorLog());
		}

		const LuaTable sideData = luaParser.GetRoot();

		// make a map of all side names  (assumes contiguous sections)
		std::map<std::string, std::string> sideMap;
		for (int i = 1; true; i++) {
			const LuaTable side = sideData.SubTable(i);
			if (!side.IsValid()) {
				break;
			}
			const std::string sideName  = side.GetString("name", "unknown");
			const std::string startUnit = side.GetString("startUnit", "");
			sideMap[StringToLower(sideName)] = StringToLower(startUnit);
		}

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
			if (!gameSetup->aiDlls[a].empty() &&
			    (gu->myPlayerNum == team->leader)) {
				globalAI->CreateGlobalAI(a, gameSetup->aiDlls[a].c_str());
			}

			std::map<std::string, std::string>::const_iterator it =
				sideMap.find(team->side);

			if (it != sideMap.end()) {
				const std::string& sideName  = it->first;
				const std::string& startUnit = it->second;
				if (startUnit.length() == 0) {
					throw content_error(
						"Unable to load a commander for side: " + sideName
					);
				}
				CUnit* unit = unitLoader.LoadUnit(startUnit,
				                                  team->startPos, a, false, 0, NULL);

				team->lineageRoot = unit->id;

				// FIXME this shouldn't be here, but no better place exists currently
				if (a == gu->myTeam) {
					minimap->AddNotification(team->startPos,
					                         float3(1.0f, 1.0f, 1.0f), 1.0f);
					game->infoConsole->SetLastMsgPos(team->startPos);
				}
			}
		}
	}
	else {
		LuaParser luaParser("gamedata/sidedata.lua",
		                    SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
		if (!luaParser.Execute()) {
			logOutput.Print(luaParser.GetErrorLog());
		}
		const LuaTable sideData = luaParser.GetRoot();
		const LuaTable side1 = sideData.SubTable(1);
		const LuaTable side2 = sideData.SubTable(2);
		const std::string su1 = StringToLower(side1.GetString("startUnit", ""));
		const std::string su2 = StringToLower(side2.GetString("startUnit", su1));

		if (su1.length() == 0) {
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

		unitLoader.LoadUnit(su1, startPos0, 0, false, 0, NULL);
		unitLoader.LoadUnit(su2, startPos1, 1, false, 0, NULL);

		// FIXME this shouldn't be here, but no better place exists currently
		minimap->AddNotification(startPos0, float3(1.0f, 1.0f, 1.0f), 1.0f);
		game->infoConsole->SetLastMsgPos(startPos0);
	}
}

