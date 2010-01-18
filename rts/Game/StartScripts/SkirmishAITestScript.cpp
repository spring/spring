
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
	// AI uses the second side, if available, otherwise the first one
	const size_t       skirmishAI_side_index = sideParser.ValidSide(1) ? 1 : 0;
	const std::string& skirmishAI_side_name  = sideParser.GetSideName(skirmishAI_side_index);
	// Human player always uses the first side
	const size_t       player_side_index = 0;
	const std::string& player_side_name  = sideParser.GetSideName(player_side_index);

	teamHandler->Team(player_teamId)->leader = player_Id;
	teamHandler->Team(player_teamId)->side   = player_side_name;
	gameServer->teams[player_teamId].leader  = player_Id;
	gameServer->teams[player_teamId].side    = player_side_name;
	gameServer->teams[player_teamId].active  = true;

	// make sure CSelectedUnits::AiOrder()
	// still works without a setup script
	teamHandler->Team(skirmishAI_teamId)->leader = player_Id;
	teamHandler->Team(skirmishAI_teamId)->side   = skirmishAI_side_name;
	gameServer->teams[skirmishAI_teamId].leader  = player_Id;
	gameServer->teams[skirmishAI_teamId].side    = skirmishAI_side_name;
	gameServer->teams[skirmishAI_teamId].active  = true;

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

	const std::string player_startUnit     = sideParser.GetStartUnit(player_side_index, "");
	const std::string skirmishAI_startUnit = sideParser.GetStartUnit(skirmishAI_side_index, player_startUnit);

	if (player_startUnit.length() == 0) {
		throw content_error ("Unable to load a commander for side " + player_side_name);
	}

	MapParser mapParser(gameSetup->mapName);
	if (!mapParser.IsValid()) {
		throw content_error("MapParser: " + mapParser.GetErrorLog());
	}
	float3 player_startPos(1000.0f, 80.0f, 1000.0f);
	float3 skirmishAI_startPos(1200.0f, 80.0f, 1200.0f);
	mapParser.GetStartPos(player_teamId, player_startPos);
	mapParser.GetStartPos(skirmishAI_teamId, skirmishAI_startPos);

	unitLoader.LoadUnit(player_startUnit, player_startPos, player_teamId, false, 0, NULL);
	unitLoader.LoadUnit(skirmishAI_startUnit, skirmishAI_startPos, skirmishAI_teamId, false, 0, NULL);
}
