#include "StdAfx.h"
#include "SpawnScript.h"
#include <fstream>
#include "TdfParser.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Game/Team.h"
#include <set>
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CSpawnScript::CSpawnScript(bool _autonomous) :
		CScript(std::string(_autonomous ? "Random enemies 2" : "Random enemies")),
		autonomous(_autonomous),
		frameOffset(0)
{
}

CSpawnScript::~CSpawnScript()
{
}

void CSpawnScript::Update()
{
	switch(gs->frameNum){
	case 0:
		LoadSpawns();

		TdfParser p("gamedata/sidedata.tdf");
		string s0=p.SGetValueDef("armcom","side0\\commander");
		string s1=p.SGetValueDef("corcom","side1\\commander");

		TdfParser p2;
		CMapInfo::OpenTDF (stupidGlobalMapname, p2);

		float x0,z0;
		p2.GetDef(x0,"1000","MAP\\TEAM0\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM0\\StartPosZ");

		// Set the TEAM0 startpos as spawnpos if we're supposed to be
		// autonomous, load the commander for the player if not.
		if (autonomous) spawnPos.push_back(float3(x0,80,z0));
		else unitLoader.LoadUnit(s0,float3(x0,80,z0),0,false,0,NULL);

		p2.GetDef(x0,"1000","MAP\\TEAM1\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM1\\StartPosZ");
		spawnPos.push_back(float3(x0,80,z0));

		p2.GetDef(x0,"1000","MAP\\TEAM2\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM2\\StartPosZ");
		spawnPos.push_back(float3(x0,80,z0));

		p2.GetDef(x0,"1000","MAP\\TEAM3\\StartPosX");
		p2.GetDef(z0,"1000","MAP\\TEAM3\\StartPosZ");
		spawnPos.push_back(float3(x0,80,z0));
	}

	if(!spawns.empty()){
		while(curSpawn->frame+frameOffset<gs->frameNum){
			int num = gs->randInt() % spawnPos.size();
			int team = autonomous ? (num & 1) : 1;
			float3 pos;
			float dist=200;
			CFeature* feature;
			do {
				pos=spawnPos[num]+gs->randVector()*dist;
				dist*=1.05f;
			} while (dist < 500 && uh->TestUnitBuildSquare(BuildInfo(curSpawn->name,pos,0),feature,team) != 2);

			// Ignore unit if it really doesn't fit.
			// (within 18 tries, 200*1.05f^18 < 500 < 200*1.05f^19)
			if (dist < 500) {
				CUnit* u = unitLoader.LoadUnit(curSpawn->name, pos, team, false, 0, NULL);

				Unit unit;
				unit.id=u->id;
				unit.target=-1;
				unit.team=team;
				myUnits.push_back(unit);
				if(myUnits.size()==1)
					curUnit=myUnits.begin();
			}

			++curSpawn;
			if(curSpawn==spawns.end()){
				curSpawn=spawns.begin();
				frameOffset+=spawns.back().frame;
			}
		}
	}

	if(!myUnits.empty() && !gs->Team(1 - curUnit->team)->units.empty()) {
		if(uh->units[curUnit->id]){
			if(curUnit->target<0 || uh->units[curUnit->target]==0){
				// We can't rely on the ordering of units in a std::set<CUnit*>,
				// because they're sorted on memory address. Hence we must first
				// build a set of IDs and then pick an unit from that.
				// This guarantees the script doesn't desync in multiplayer games.
				int num = gs->randInt() % gs->Team(1 - curUnit->team)->units.size();
				std::set<int> unitids;
				CUnitSet* tu = &gs->Team(1 - curUnit->team)->units;
				for (CUnitSet::iterator u = tu->begin(); u != tu->end(); ++u)
					unitids.insert((*u)->id);
				std::set<int>::iterator ui = unitids.begin();
				for(int a=0;a<num;++a)
					++ui;
				curUnit->target=(*ui);
				curUnit->lastTargetPos.x=-500;
			}
			float3 pos=uh->units[curUnit->target]->pos;
			if(pos.distance2D(curUnit->lastTargetPos)>100){
				curUnit->lastTargetPos=pos;
				Command c;
				c.id=CMD_PATROL;
				c.options=0;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				uh->units[curUnit->id]->commandAI->GiveCommand(c);
			}
			curUnit++;
		} else {
			curUnit=myUnits.erase(curUnit);
		}
		if(curUnit==myUnits.end())
			curUnit=myUnits.begin();
	}
}

void CSpawnScript::LoadSpawns()
{
	CFileHandler file("spawn.txt");

	while(file.FileExists() && !file.Eof()){

		Spawn s;
		s.frame=atoi(LoadToken(file).c_str());
		s.name=LoadToken(file).c_str();

		if(s.name.empty())
			break;

		spawns.push_back(s);
	}
	curSpawn=spawns.begin();
}

std::string CSpawnScript::LoadToken(CFileHandler& file)
{
	string s;
	char c;

	while (!file.Eof()) {
		file.Read(&c,1);
		if(c>='0' && c<='z')
			break;
	}
	s += c;
	while (!file.Eof()) {
		file.Read(&c,1);
		if(c<'0' || c>'z')
			return s;
		s+=c;
	}
	return s;
}
