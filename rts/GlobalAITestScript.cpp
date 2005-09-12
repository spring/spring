#include "StdAfx.h"
#include "GlobalAITestScript.h"
#include "UnitLoader.h"
#include "SunParser.h"
#include <algorithm>
#include <cctype>
#include "Team.h"
#include "UnitDefHandler.h"
#include "GlobalAIHandler.h"
//#include "mmgr.h"

static CGlobalAITestScript ts;
extern std::string stupidGlobalMapname;

CGlobalAITestScript::CGlobalAITestScript(void)
: CScript(std::string("GlobalAI test"))
{
}

CGlobalAITestScript::~CGlobalAITestScript(void)
{
}

void CGlobalAITestScript::Update(void)
{
	switch(gs->frameNum){
	case 0:{
#ifdef _WIN32
		globalAI->CreateGlobalAI(1,"./aidll/globalai/test.dll");
#else
		globalAI->CreateGlobalAI(1,"./aidll/globalai/libtest.so");
#endif

		gs->teams[0]->energy=1000;
		gs->teams[0]->energyStorage=1000;
		gs->teams[0]->metal=1000;
		gs->teams[0]->metalStorage=1000;

		gs->teams[1]->energy=5000;
		gs->teams[1]->energyStorage=5000;
		gs->teams[1]->metal=5000;
		gs->teams[1]->metalStorage=5000;

		CSunParser p;
		p.LoadFile("gamedata/sidedata.tdf");
		string s0=p.SGetValueDef("armcom","side0\\commander");
		string s1=p.SGetValueDef("corcom","side1\\commander");

		CSunParser p2;
		p2.LoadFile(string("maps/")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find('.'))+".smd");

		float x0,x1,z0,z1;
		p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
		p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
		p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

		unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false);
		unitLoader.LoadUnit(s1,float3(x1,80,z1),1,false);
		break;}
	default:
		break;
	}
}
