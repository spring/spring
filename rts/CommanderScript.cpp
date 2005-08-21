#include "StdAfx.h"
#include "CommanderScript.h"
#include "UnitLoader.h"
#include "SunParser.h"
static CCommanderScript ts;
#include <algorithm>
#include <cctype>
#include "Team.h"
#include "GameSetup.h"
#include "UnitDefHandler.h"
#include "GlobalAIHandler.h"
//#include "mmgr.h"

extern std::string stupidGlobalMapname;

CCommanderScript::CCommanderScript(void)
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
			CSunParser p;
			p.LoadFile("gamedata/SIDEDATA.TDF");
			for(int a=0;a<gs->activeTeams;++a){		
				if(!gameSetup->aiDlls[a].empty() && gs->teams[a]->leader==gu->myTeam){
					globalAI->CreateGlobalAI(a,gameSetup->aiDlls[a].c_str());
				}

				for(int b=0;b<8;++b){					//loop over all sides
					char sideText[50];
					sprintf(sideText,"side%i",b);
					if(p.SectionExist(sideText)){
						string sideName=p.SGetValueDef("arm",string(sideText)+"\\name");
						std::transform(sideName.begin(), sideName.end(), sideName.begin(), (int (*)(int))std::tolower);
						if(sideName==gs->teams[a]->side){		//ok found the right side
							string cmdType=p.SGetValueDef("armcom",string(sideText)+"\\commander");
							
							UnitDef* ud= unitDefHandler->GetUnitByName(cmdType);
							ud->metalStorage=gs->teams[a]->metalStorage;			//make sure the cmd has the right amount of storage
							ud->energyStorage=gs->teams[a]->energyStorage;

							unitLoader.LoadUnit(cmdType,gs->teams[a]->startPos,a,false);
							break;
						}
					}
				}
				gs->teams[a]->metalStorage=gs->teams[a]->metalStorage/2+20;		//now remove the preexisting storage except for a small amount
				gs->teams[a]->energyStorage=gs->teams[a]->energyStorage/2+20;
			}
		} else {
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
//			unitLoader.LoadUnit("armsam",float3(x0,80,z0)+float3(100,0,0),0,false);
		}
		break;
	}
}

