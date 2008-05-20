#include <algorithm>
#include <cctype>
#include <map>
#include "StdAfx.h"
#include "CommanderScript.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/InfoConsole.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h" // still required for parsing map SMD for start positions
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
		LuaParser p("gamedata/sidedata.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
		if (!p.Execute() || !p.IsValid())
			logOutput.Print(p.GetErrorLog());
			
		const LuaTable sideTable = p.GetRoot();

		// make a map of all side names  (assumes contiguous sections)
		std::map<std::string, std::string> sideMap;
		char sideText[64];
		for (int side = 0;
				 SNPRINTF(sideText, sizeof(sideText), "side%i", side),
				 sideTable.KeyExists(sideText); // the test
				 side++) {
			const std::string sideName =
				StringToLower(sideTable.SubTable(std::string(sideText)).GetString("name", "No Name"));
			sideMap[sideName] = sideText;
		}

		// setup the teams
		for (int a = 0; a < gs->activeTeams; ++a) {

			// don't spawn a commander for the gaia team
			if (gs->useLuaGaia && a == gs->gaiaTeamID)
				continue;

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

			std::map<std::string, std::string>::const_iterator it = sideMap.find(team->side);

			if (it != sideMap.end()) {
				const std::string& sideSection = it->second;
				const std::string cmdrType =
					StringToLower(sideTable.SubTable(sideSection).GetString("commander",""));
				if (cmdrType.length() == 0)
					throw content_error ("Unable to load a commander for SIDE" + sideSection);
				CUnit* unit = unitLoader.LoadUnit(cmdrType, team->startPos, a, false, 0, NULL);

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
		LuaParser p("gamedata/sidedata.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
		if (!p.Execute() || !p.IsValid())
			logOutput.Print(p.GetErrorLog());
		
		const LuaTable sideTable = p.GetRoot();
		const std::string s0 = StringToLower(sideTable.SubTable("side0").GetString("commander", ""));
		const std::string s1 = StringToLower(sideTable.SubTable("side1").GetString("commander", s0)); // default to side 0, in case mod has only 1 side

		if (s0.length() == 0)
			throw content_error ("Unable to load a commander for SIDE0");
		
		TdfParser p2;
		CMapInfo::OpenTDF(stupidGlobalMapname, p2);

		float x0, x1, z0, z1;
		p2.GetDef(x0, "1000", "MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0, "1000", "MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1, "1200", "MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1, "1200", "MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0, float3(x0, 80.0f, z0), 0, false, 0, NULL);
		unitLoader.LoadUnit(s1, float3(x1, 80.0f, z1), 1, false, 0, NULL);

		// FIXME this shouldn't be here, but no better place exists currently
		minimap->AddNotification(float3(x0, 80.0f, z0),
				float3(1.0f, 1.0f, 1.0f), 1.0f);
		game->infoConsole->SetLastMsgPos(float3(x0, 80.0f, z0));
	}
}

