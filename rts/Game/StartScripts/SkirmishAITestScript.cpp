
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
#include "mmgr.h"
#include "Exceptions.h"


extern std::string stupidGlobalMapname;


CSkirmishAITestScript::CSkirmishAITestScript(const SkirmishAIKey& key,
		const std::map<std::string, std::string>& options)
		: CScript(std::string("Skirmish AI test (")
			+ std::string(key.GetShortName()) + std::string(" v")
			+ std::string(key.GetVersion() + std::string(")"))),
		key(key), options(options)
{
	teamHandler->Team(skirmishAI_teamId)->isAI = true;
	teamHandler->Team(skirmishAI_teamId)->skirmishAIKey
			= SkirmishAIKey(
					"CSkirmishAITestScript:TEMP",
					"CSkirmishAITestScript:TEMP");
	teamHandler->Team(skirmishAI_teamId)->leader = 0;
}


CSkirmishAITestScript::~CSkirmishAITestScript(void) {}

void CSkirmishAITestScript::GameStart(void)
{
	// make sure CSelectedUnits::AiOrder()
	// still works without a setup script
	teamHandler->Team(skirmishAI_teamId)->isAI = true;
	teamHandler->Team(skirmishAI_teamId)->skirmishAIKey = key;
	teamHandler->Team(skirmishAI_teamId)->skirmishAIOptions = options;
	teamHandler->Team(skirmishAI_teamId)->leader = 0;

	teamHandler->Team(player_teamId)->energy        = 1000;
	teamHandler->Team(player_teamId)->energyStorage = 1000;
	teamHandler->Team(player_teamId)->metal         = 1000;
	teamHandler->Team(player_teamId)->metalStorage  = 1000;

	teamHandler->Team(skirmishAI_teamId)->energy        = 1000;
	teamHandler->Team(skirmishAI_teamId)->energyStorage = 1000;
	teamHandler->Team(skirmishAI_teamId)->metal         = 1000;
	teamHandler->Team(skirmishAI_teamId)->metalStorage  = 1000;

	eoh->CreateSkirmishAI(1, key);

	const std::string startUnit0 = sideParser.GetStartUnit(0, "");
	const std::string startUnit1 = sideParser.GetStartUnit(1, startUnit0);
	// default to side 1, in case mod has only 1 side

	if (startUnit0.length() == 0) {
		throw content_error ("Unable to load a commander for the first side");
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
}
