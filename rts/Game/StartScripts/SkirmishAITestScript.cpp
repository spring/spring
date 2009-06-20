
#include "SkirmishAITestScript.h"

#include <algorithm>
#include <cctype>
#include "StdAfx.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "LogOutput.h"
#include "FileSystem/FileSystem.h"
#include "Game/GameSetup.h"
#include "Game/PlayerHandler.h"
#include "mmgr.h"
#include "Exceptions.h"


CSkirmishAITestScript::CSkirmishAITestScript(const SkirmishAIKey& k, const std::map<std::string, std::string>& opts):
	CScript(
		std::string("Skirmish AI test: ")
		+ std::string(k.GetShortName()) + std::string(" ")
		+ std::string(k.GetVersion())
	),
	key(k),
	options(opts)
{
}


CSkirmishAITestScript::~CSkirmishAITestScript(void) {}

void CSkirmishAITestScript::GameStart(void)
{
	// make sure CSelectedUnits::AiOrder()
	// still works without a setup script
	teamHandler->Team(skirmishAI_teamId)->isAI = true;
	teamHandler->Team(skirmishAI_teamId)->skirmishAIKey = key;
	teamHandler->Team(skirmishAI_teamId)->skirmishAIOptions = options;
	teamHandler->Team(skirmishAI_teamId)->leader = player_teamId;

	playerHandler->Player(player_teamId)->SetControlledTeams();

	teamHandler->Team(player_teamId)->energy        = 1000;
	teamHandler->Team(player_teamId)->energyStorage = 1000;
	teamHandler->Team(player_teamId)->metal         = 1000;
	teamHandler->Team(player_teamId)->metalStorage  = 1000;

	teamHandler->Team(skirmishAI_teamId)->energy        = 1000;
	teamHandler->Team(skirmishAI_teamId)->energyStorage = 1000;
	teamHandler->Team(skirmishAI_teamId)->metal         = 1000;
	teamHandler->Team(skirmishAI_teamId)->metalStorage  = 1000;

	// do not instantiate an AI when watching a
	// demo recorded from a SkirmishAI test-script
	if (!gameSetup->hostDemo && !key.IsUnspecified()) {
		eoh->CreateSkirmishAI(skirmishAI_teamId, key);
	}

	const std::string startUnit0 = sideParser.GetStartUnit(0, "");
	const std::string startUnit1 = sideParser.GetStartUnit(1, startUnit0);
	// default to side 1, in case mod has only 1 side

	if (startUnit0.length() == 0) {
		throw content_error ("Unable to load a commander for the first side");
	}

	MapParser mapParser(gameSetup->mapName);
	if (!mapParser.IsValid()) {
		throw content_error("MapParser: " + mapParser.GetErrorLog());
	}
	float3 startPos0(1000.0f, 80.0f, 1000.0f);
	float3 startPos1(1200.0f, 80.0f, 1200.0f);
	mapParser.GetStartPos(0, startPos0);
	mapParser.GetStartPos(1, startPos1);

	unitLoader.LoadUnit(startUnit0, startPos0, 0, false, 0, NULL);
	unitLoader.LoadUnit(startUnit1, startPos1, 1, false, 0, NULL);
}
