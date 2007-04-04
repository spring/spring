#include "StdAfx.h"
#include "CommanderScript.h"
#include "Sim/Units/UnitLoader.h"
#include "TdfParser.h"
#include <algorithm>
#include <cctype>
#include "Game/Team.h"
#include "Game/GameSetup.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Map/ReadMap.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CCommanderScript::CCommanderScript()
: CScript(std::string("Commanders"))
{
}

CCommanderScript::~CCommanderScript(void)
{
}

void CCommanderScript::Update(void)
{
	switch(gs->frameNum){
	case 0:
		if(gameSetup){
			TdfParser p("gamedata/SIDEDATA.TDF");
			for(int a=0;a<gs->activeTeams;++a){
				if(!gameSetup->aiDlls[a].empty()){
					if (gu->myPlayerNum == gs->Team (a)->leader)
						globalAI->CreateGlobalAI(a,gameSetup->aiDlls[a].c_str());
				}

				for(int b=0;b<8;++b){					//loop over all sides
					char sideText[50];
					sprintf(sideText,"side%i",b);
					if(p.SectionExist(sideText)){
						string sideName = StringToLower(p.SGetValueDef("arm",string(sideText)+"\\name"));
						if(sideName==gs->Team(a)->side){		//ok found the right side
							string cmdType=p.SGetValueDef("armcom",string(sideText)+"\\commander");

							UnitDef* ud= unitDefHandler->GetUnitByName(cmdType);
							ud->metalStorage=gs->Team(a)->metalStorage;			//make sure the cmd has the right amount of storage
							ud->energyStorage=gs->Team(a)->energyStorage;

							CUnit* u = unitLoader.LoadUnit(cmdType,gs->Team(a)->startPos,a,false,0,NULL);
							gs->Team(a)->lineageRoot = u->id;
							break;
						}
					}
				}
				gs->Team(a)->metalStorage=gs->Team(a)->metalStorage/2+20;		//now remove the preexisting storage except for a small amount
				gs->Team(a)->energyStorage=gs->Team(a)->energyStorage/2+20;
			}
		} else {
			TdfParser p("gamedata/SIDEDATA.TDF");
			string s0=p.SGetValueDef("armcom","side0\\commander");
			string s1=p.SGetValueDef("corcom","side1\\commander");

			TdfParser p2;
			CReadMap::OpenTDF (stupidGlobalMapname, p2);

			float x0,x1,z0,z1;
			p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
			p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
			p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
			p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

			unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false,0,NULL);
			unitLoader.LoadUnit(s1,float3(x1,80,z1),1,false,0,NULL);
//			unitLoader.LoadUnit("armsam",float3(x0,80,z0)+float3(100,0,0),0,false,0,NULL);
		}
		break;
	}
}

