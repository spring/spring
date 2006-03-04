#include "StdAfx.h"
#include "GlobalAITestScript.h"
#include "Sim/Units/UnitLoader.h"
#include "TdfParser.h"
#include <algorithm>
#include <cctype>
#include "Game/Team.h"
#include "Sim/Units/UnitDefHandler.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "FileSystem/FileHandler.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CGlobalAITestScript::CGlobalAITestScript(std::string dll, std::string base)
: CScript(std::string("GlobalAI test (") + dll + std::string(")")),
	dllName(dll), baseDir(base)
{
}

CGlobalAITestScript::~CGlobalAITestScript(void)
{
}

void CGlobalAITestScript::Update(void)
{
	switch(gs->frameNum){
	case 0:{
		globalAI->CreateGlobalAI(1, string(baseDir + dllName).c_str());

		gs->Team(0)->energy=1000;
		gs->Team(0)->energyStorage=1000;
		gs->Team(0)->metal=1000;
		gs->Team(0)->metalStorage=1000;

		gs->Team(1)->energy=1000;
		gs->Team(1)->energyStorage=1000;
		gs->Team(1)->metal=1000;
		gs->Team(1)->metalStorage=1000;

		TdfParser p("gamedata/sidedata.tdf");
		string s0=p.SGetValueDef("armcom","side0\\commander");
		string s1=p.SGetValueDef("corcom","side1\\commander");

		TdfParser p2(string("maps/")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find('.'))+".smd");

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
