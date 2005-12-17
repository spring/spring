#include "System/StdAfx.h"
#include "AICheats.h"
#include <vector>
#include "GlobalAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Game/Team.h"
//#include "System/mmgr.h"

using namespace std;

CAICheats::CAICheats(CGlobalAI* ai)
:	ai(ai)
{
}

CAICheats::~CAICheats(void)
{
}

void CAICheats::SetMyHandicap(float handicap)
{
	gs->Team(ai->team)->handicap=1+handicap/100;
}

void CAICheats::GiveMeMetal(float amount)
{
	gs->Team(ai->team)->metal+=amount;
}

void CAICheats::GiveMeEnergy(float amount)
{
	gs->Team(ai->team)->energy+=amount;
}

int CAICheats::CreateUnit(const char* name,float3 pos)
{
	CUnit* u=unitLoader.LoadUnit(name,pos,ai->team,false);
	if(u)
		return u->id;
	return 0;
}

const UnitDef* CAICheats::GetUnitDef(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->unitDef;
	}
	return 0;

}

float3 CAICheats::GetUnitPos(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->pos;
	}
	return ZeroVector;

}

int CAICheats::GetEnemyUnits(int *units)
{
	list<CUnit*>::iterator ui;
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(ai->team))){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CAICheats::GetEnemyUnits(int *units,const float3& pos,float radius)
{
	vector<CUnit*> unit=qf->GetUnitsExact(pos,radius);

	vector<CUnit*>::iterator ui;
	int a=0;

	for(ui=unit.begin();ui!=unit.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(ai->team))){
			units[a]=(*ui)->id;
			++a;
		}
	}
	return a;

}
