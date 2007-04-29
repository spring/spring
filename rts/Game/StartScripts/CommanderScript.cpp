#include "StdAfx.h"
#include "CommanderScript.h"
#include "Sim/Units/UnitLoader.h"
#include "TdfParser.h"
#include <algorithm>
#include <cctype>
#include <map>
#include "Game/Team.h"
#include "Game/GameSetup.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Map/ReadMap.h"
#include "mmgr.h"


extern std::string stupidGlobalMapname;


CCommanderScript::CCommanderScript()
: CScript(std::string("Commanders"))
{
}


CCommanderScript::~CCommanderScript(void)
{
}


void CCommanderScript::Update(void)
{
	if (gs->frameNum != 0) {
		return;
	}

	if (gameSetup) {
		TdfParser p("gamedata/SIDEDATA.TDF");

		// make a map of all side names  (assumes contiguous sections)
		std::map<string, string> sideMap;
		char sideText[64];
		for (int side = 0;
				 SNPRINTF(sideText, sizeof(sideText), "side%i", side),
				 p.SectionExist(sideText); // the test
				 side++) {
			const string sideName =
				StringToLower(p.SGetValueDef("arm", string(sideText) + "\\name"));
			sideMap[sideName] = sideText;
		}

		// setup the teams
		for (int a = 0; a < gs->activeTeams; ++a) {
			CTeam* team = gs->Team(a);

			// create a GlobalAI if required
			if (!gameSetup->aiDlls[a].empty() &&
			    (gu->myPlayerNum == team->leader)) {
				globalAI->CreateGlobalAI(a, gameSetup->aiDlls[a].c_str());
			}
			
			std::map<string, string>::const_iterator it = sideMap.find(team->side);

			if (it != sideMap.end()) {
				const string& sideSection = it->second;
				const string cmdrType =
					p.SGetValueDef("armcom", sideSection + "\\commander");

				// make sure the commander has the right amount of storage
				UnitDef* ud = unitDefHandler->GetUnitByName(cmdrType);
				ud->metalStorage  = team->metalStorage;
				ud->energyStorage = team->energyStorage;

				CUnit* unit =
					unitLoader.LoadUnit(cmdrType, team->startPos, a, false, 0, NULL);

				team->lineageRoot = unit->id;
			}

			// now remove the pre-existing storage except for a small amount
			team->metalStorage  = (team->metalStorage  / 2) + 20;
			team->energyStorage = (team->energyStorage / 2) + 20;
		}
	}
	else {
		TdfParser p("gamedata/SIDEDATA.TDF");
		const string s0 = p.SGetValueDef("armcom", "side0\\commander");
		const string s1 = p.SGetValueDef("corcom", "side1\\commander");

		TdfParser p2;
		CReadMap::OpenTDF(stupidGlobalMapname, p2);

		float x0, x1, z0, z1;
		p2.GetDef(x0, "1000", "MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0, "1000", "MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1, "1200", "MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1, "1200", "MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0, float3(x0, 80.0f, z0), 0, false, 0, NULL);
		unitLoader.LoadUnit(s1, float3(x1, 80.0f, z1), 1, false, 0, NULL);
	}
}

