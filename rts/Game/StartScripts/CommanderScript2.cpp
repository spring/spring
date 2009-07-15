#include "StdAfx.h"
#include "mmgr.h"

#include "CommanderScript2.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/TeamHandler.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Units/UnitLoader.h"
#include "LogOutput.h"
#include "Exceptions.h"


CCommanderScript2::CCommanderScript2()
: CScript(std::string("Cmds 1000 res"))
{
}


CCommanderScript2::~CCommanderScript2()
{
}


void CCommanderScript2::GameStart()
{
	teamHandler->Team(0)->energy=1000;
	teamHandler->Team(0)->energyIncome=1000;	//for the endgame statistics
	teamHandler->Team(0)->energyStorage=1000;
	teamHandler->Team(0)->metal=1000;
	teamHandler->Team(0)->metalIncome=1000;
	teamHandler->Team(0)->metalStorage=1000;

	teamHandler->Team(1)->energy=1000;
	teamHandler->Team(1)->energyIncome=1000;
	teamHandler->Team(1)->energyStorage=1000;
	teamHandler->Team(1)->metal=1000;
	teamHandler->Team(1)->metalIncome=1000;
	teamHandler->Team(1)->metalStorage=1000;

	const std::string startUnit0 = sideParser.GetStartUnit(0, "");
	const std::string startUnit1 = sideParser.GetStartUnit(1, startUnit0);

	if (startUnit0.length() == 0) {
		throw content_error ("Unable to load a startUnit for the first side");
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

