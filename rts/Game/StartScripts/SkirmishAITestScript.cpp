
#include "SkirmishAITestScript.h"

#include <algorithm>
#include <cctype>
#include "StdAfx.h"
#include "ExternalAI/SkirmishAIHandler.h"
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
#include "Game/GameServer.h"
#include "Game/PlayerHandler.h"
#include "Game/Server/GameSkirmishAI.h"
#include "NetProtocol.h"
#include "mmgr.h"
#include "Exceptions.h"

const std::string CSkirmishAITestScript::SCRIPT_NAME_PRELUDE = "Skirmish AI test: ";

CSkirmishAITestScript::CSkirmishAITestScript(const SkirmishAIData& aiData):
	CScript(SCRIPT_NAME_PRELUDE + aiData.shortName + std::string(" ") + aiData.version)
	, aiData(aiData)
{
}


CSkirmishAITestScript::~CSkirmishAITestScript() {}

void CSkirmishAITestScript::GameStart()
{
	// make sure CSelectedUnits::AiOrder()
	// still works without a setup script
	teamHandler->Team(skirmishAI_teamId)->leader = player_Id;
	gameServer->teams[skirmishAI_teamId].leader  = player_Id;
	gameServer->teams[skirmishAI_teamId].active  = true;
	gameServer->teams[player_teamId].leader  = player_Id;
	gameServer->teams[player_teamId].active  = true;

	teamHandler->Team(player_teamId)->energy        = 1000;
	teamHandler->Team(player_teamId)->energyStorage = 1000;
	teamHandler->Team(player_teamId)->metal         = 1000;
	teamHandler->Team(player_teamId)->metalStorage  = 1000;

	teamHandler->Team(skirmishAI_teamId)->energy        = 1000;
	teamHandler->Team(skirmishAI_teamId)->energyStorage = 1000;
	teamHandler->Team(skirmishAI_teamId)->metal         = 1000;
	teamHandler->Team(skirmishAI_teamId)->metalStorage  = 1000;

	aiData.name       = aiData.shortName + "_" + aiData.version;
	aiData.team       = skirmishAI_teamId;
	aiData.hostPlayer = player_Id;

	const size_t skirmishAIId = gameServer->ReserveNextAvailableSkirmishAIId();
	gameServer->ais[skirmishAIId] = aiData;
	skirmishAIHandler.AddSkirmishAI(aiData, skirmishAIId);

	playerHandler->Player(player_Id)->SetControlledTeams();

	// do not instantiate an AI when watching a
	// demo recorded from a SkirmishAI test-script
	if (!gameSetup->hostDemo) {
		skirmishAIHandler.CreateLocalSkirmishAI(skirmishAIId);
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
