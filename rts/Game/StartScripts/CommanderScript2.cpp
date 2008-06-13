#include "StdAfx.h"
#include "CommanderScript2.h"
#include "Game/Team.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;


CCommanderScript2::CCommanderScript2()
: CScript(std::string("Cmds 1000 res"))
{
}


CCommanderScript2::~CCommanderScript2()
{
}


void CCommanderScript2::GameStart()
{
	gs->Team(0)->energy=1000;
	gs->Team(0)->energyIncome=1000;	//for the endgame statistics
	gs->Team(0)->energyStorage=1000;
	gs->Team(0)->metal=1000;
	gs->Team(0)->metalIncome=1000;
	gs->Team(0)->metalStorage=1000;

	gs->Team(1)->energy=1000;
	gs->Team(1)->energyIncome=1000;
	gs->Team(1)->energyStorage=1000;
	gs->Team(1)->metal=1000;
	gs->Team(1)->metalIncome=1000;
	gs->Team(1)->metalStorage=1000;

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
		throw content_error ("Unable to load a startUnit for the first side");
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
}

