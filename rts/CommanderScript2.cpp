#include "StdAfx.h"
#include "CommanderScript2.h"
#include "UnitLoader.h"
#include "SunParser.h"
#include "Team.h"

static CCommanderScript2 ts;
//#include "mmgr.h"

extern std::string stupidGlobalMapname;

CCommanderScript2::CCommanderScript2(void)
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
		gs->teams[0]->energy=1000;
		gs->teams[0]->energyStorage=1000;
		gs->teams[0]->metal=1000;
		gs->teams[0]->metalStorage=1000;

		gs->teams[1]->energy=1000;
		gs->teams[1]->energyStorage=1000;
		gs->teams[1]->metal=1000;
		gs->teams[1]->metalStorage=1000;

		CSunParser p;
		p.LoadFile("gamedata/SIDEDATA.TDF");
		string s0=p.SGetValueDef("armcom","side0\\commander");
		string s1=p.SGetValueDef("corcom","side1\\commander");

		CSunParser p2;
		p2.LoadFile(string("maps/")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find_last_of('.'))+".smd");

		float x0,x1,z0,z1;
		p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false);
		unitLoader.LoadUnit(s1,float3(x1,80,z1),1,false);

//		unitLoader.LoadUnit("armsam",float3(2650,10,2600),0,false);
//		unitLoader.LoadUnit("armflash",float3(2450,10,2600),1,false);

		break;
	}
}

