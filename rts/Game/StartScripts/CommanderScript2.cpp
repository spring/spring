#include "StdAfx.h"
#include "CommanderScript2.h"
#include "Game/Team.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h"
#include "System/FileSystem/FileHandler.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CCommanderScript2::CCommanderScript2()
: CScript(std::string("Cmds 1000 res"))
{
}

CCommanderScript2::~CCommanderScript2(void)
{
}

void CCommanderScript2::Update(void)
{
	switch(gs->frameNum){
	case 0:
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

		LuaParser p("gamedata/sidedata.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
		if (!p.Execute() || !p.IsValid())
			logOutput.Print(p.GetErrorLog());
		
		const LuaTable sideTable = p.GetRoot();
		const std::string s0 = StringToLower(sideTable.SubTable("side0").GetString("commander", ""));
		const std::string s1 = StringToLower(sideTable.SubTable("side1").GetString("commander", s0)); // default to side 0, in case mod has only 1 side

		if (s0.length() == 0)
			throw content_error ("Unable to load a commander for SIDE0");

		TdfParser p2;
		CMapInfo::OpenTDF (stupidGlobalMapname, p2);

		float x0,x1,z0,z1;
		p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0, float3(x0, 80, z0), 0, false, 0, NULL);
		unitLoader.LoadUnit(s1, float3(x1, 80, z1), 1, false, 0, NULL);

//		unitLoader.LoadUnit("armsam",float3(2650,10,2600),0,false);
//		unitLoader.LoadUnit("armflash",float3(2450,10,2600),1,false);

		break;
	}
}

