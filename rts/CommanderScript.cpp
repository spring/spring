#include "StdAfx.h"
#include "CommanderScript.h"
#include "UnitLoader.h"
#include "SunParser.h"
static CCommanderScript ts;
#include <algorithm>
#include <cctype>
#include "Team.h"
#include "GameSetup.h"
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
#ifdef _WIN32
			p.LoadFile("gamedata\\sidedata.tdf");
#else
                        p.LoadFile("gamedata/SIDEDATA.TDF");
#endif
			for(int a=0;a<gs->activeTeams;++a){		
				for(int b=0;b<8;++b){					//loop over all sides
					char sideText[50];
					sprintf(sideText,"side%i",b);
					if(p.SectionExist(sideText)){
						string sideName=p.SGetValueDef("arm",string(sideText)+"\\name");
						std::transform(sideName.begin(), sideName.end(), sideName.begin(), (int (*)(int))std::tolower);
						if(sideName==gs->teams[a]->side){		//ok found the right side
							string cmdType=p.SGetValueDef("armcom",string(sideText)+"\\commander");
							
							unitLoader.LoadUnit(cmdType,gs->teams[a]->startPos,a,false);
							break;
						}
					}
				}
			}
		} else {
			CSunParser p;
#ifdef _WIN32
			p.LoadFile("gamedata\\sidedata.tdf");
#else
                        p.LoadFile("gamedata/SIDEDATA.TDF");
#endif
			string s0=p.SGetValueDef("armcom","side0\\commander");
			string s1=p.SGetValueDef("corcom","side1\\commander");

			CSunParser p2;
			p2.LoadFile(string("maps\\")+stupidGlobalMapname.substr(0,stupidGlobalMapname.find('.'))+".smd");

			float x0,x1,z0,z1;
			p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
			p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");
			p2.GetDef(x1,"1200","MAP\\TEAM1\\StartPosX");
			p2.GetDef(z1,"1200","MAP\\TEAM1\\StartPosZ");

			unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false);
			unitLoader.LoadUnit(s1,float3(x1,80,z1),1,false);
		}
		break;
	}
}

