#include "StdAfx.h"
#include "MoveTestScript.h"
#include "UnitLoader.h"
#include "SunParser.h"
#include "Team.h"

static CMoveTestScript ts;
//#include "mmgr.h"

extern std::string stupidGlobalMapname;

CMoveTestScript::CMoveTestScript(void)
: CScript(std::string("MoveTest"))
{
}

CMoveTestScript::~CMoveTestScript(void)
{
}

void CMoveTestScript::Update(void)
{
	switch(gs->frameNum){
	case 0:
		gs->teams[1]->energy=10000;
		gs->teams[1]->energyStorage=10000;
		gs->teams[1]->metal=10000;
		gs->teams[1]->metalStorage=10000;

		gs->teams[2]->energy=100000;
		gs->teams[2]->energyStorage=100000;
		gs->teams[2]->metal=100000;
		gs->teams[2]->metalStorage=100000;

		CSunParser p;
#ifdef _WIN32
                p.LoadFile("gamedata\\sidedata.tdf");
#else
                p.LoadFile("gamedata/SIDEDATA.TDF");
#endif
		string s0=p.SGetValueDef("armcom","side0\\commander");
		string s1=p.SGetValueDef("corcom","side1\\commander");

		CSunParser p2;
//#ifdef _WIN32
		p2.LoadFile(string("maps\\")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find('.'))+".smd");
/*#else
		p2.LoadFile(string("maps/")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find('.'))+".smd");
#endif*/

		float x0,x1,z0,z1;
		p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false);
		unitLoader.LoadUnit(s1,float3(x1,80,z1),1,false);

		//Arm
		unitLoader.LoadUnit("ARMMAV",float3(x1+150,80,z0+400),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+200,80,z0+350),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+250,80,z0+300),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+300,80,z0+250),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+250,80,z0+200),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+200,80,z0+150),1,false);
		unitLoader.LoadUnit("ARMMAV",float3(x1+150,80,z0+100),1,false);


		unitLoader.LoadUnit("ARMMEX",float3(x1+370,80,z0+480),1,false);
		unitLoader.LoadUnit("ARMMEX",float3(x1+200,80,z0-50),1,false);
		unitLoader.LoadUnit("ARMGEO",float3(x1+20,80,z0+90),1,false);

		unitLoader.LoadUnit("ARMALAB",float3(x1-150,80,z0+100),1,false);


		//Core
		unitLoader.LoadUnit("ARMARAD",float3(x1+1600,80,z0+1200),1,false);
		unitLoader.LoadUnit("ARMHLT",float3(x1+1600,80,z0+1000),1,false);
		unitLoader.LoadUnit("ARMHLT",float3(x1+1430,80,z0+980),1,false);
		unitLoader.LoadUnit("ARMRL",float3(x1+1700,80,z0+1000),1,false);
		unitLoader.LoadUnit("ARMRL",float3(x1+1650,80,z0+1000),1,false);


		break;
	}
}

