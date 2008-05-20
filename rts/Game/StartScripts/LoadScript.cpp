#include <iostream>
#include <vector>
#include "StdAfx.h"
#include "LoadScript.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/LoadSaveHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"

extern std::string stupidGlobalMapname;

CLoadScript::CLoadScript(std::string file)
	: CScript(std::string("Load ") + filesystem.GetFilename(file)),
	file(file)
{
}

CLoadScript::~CLoadScript(void)
{
}

void CLoadScript::Update(void)
{
	if(!started) {
/*
		if(gameSetup){
			TdfParser p("gamedata/SIDEDATA.TDF");
			for(int a=0;a<gs->activeTeams;++a){		
				CTeam* team = gs->Team(a);
				if (team->gaia) continue;
//				if(!gameSetup->aiDlls[a].empty()){
//					if (gu->myPlayerNum == gs->Team (a)->leader)
//						globalAI->CreateGlobalAI(a,gameSetup->aiDlls[a].c_str());
//				}

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
							break;
						}
					}
				}
//				gs->Team(a)->metalStorage=/ *gs->Team(a)->metalStorage/2+* /20;		//now remove the preexisting storage except for a small amount
//				gs->Team(a)->energyStorage=/ *gs->Team(a)->energyStorage/2+* /20;
			}
		}
*/
		loader.LoadGame();
		started=true;
	}
}

void CLoadScript::ScriptSelected()
{
	loader.LoadGameStartInfo(file);		//this is the first time we get called after getting choosen
	started = false;
	loadGame=true;
}

std::string CLoadScript::GetMapName(void)
{
	return loader.mapName;
}

std::string CLoadScript::GetModName(void)
{
	//this is the second time we get called after getting choosen
	return loader.modName;
}
