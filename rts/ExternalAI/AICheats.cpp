#include "StdAfx.h"
#include "AICheats.h"
#include <vector>
#include "GlobalAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Net.h"
#include "Game/Team.h"
#include "mmgr.h"

using namespace std;

CAICheats::CAICheats(CGlobalAI* ai)
:	ai(ai)
{
}

CAICheats::~CAICheats(void)
{}

bool CAICheats::OnlyPassiveCheats ()
{
	return !net->onlyLocal;
}

void CAICheats::EnableCheatEvents(bool enable)
{
	ai->cheatevents = enable;
}

void CAICheats::SetMyHandicap(float handicap)
{
	if (net->onlyLocal) {
		gs->Team(ai->team)->handicap=1+handicap/100;
	}
}

void CAICheats::GiveMeMetal(float amount)
{
	if (net->onlyLocal)
		gs->Team(ai->team)->metal+=amount;
}

void CAICheats::GiveMeEnergy(float amount)
{
	if (net->onlyLocal)
		gs->Team(ai->team)->energy+=amount;
}

int CAICheats::CreateUnit(const char* name,float3 pos)
{
	if(net->onlyLocal) {
		CUnit* u=unitLoader.LoadUnit(name,pos,ai->team,false);
		if(u)
			return u->id;
	}
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

int CAICheats::GetUnitTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->team;
	}
	return 0;	
}

int CAICheats::GetUnitAllyTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->allyteam;
	}
	return 0;	
}

float CAICheats::GetUnitHealth(int unitid)			//the units current health
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->health;
	}
	return 0;
}

float CAICheats::GetUnitMaxHealth(int unitid)		//the units max health
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->maxHealth;
	}
	return 0;
}

float CAICheats::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->power;
	}
	return 0;
}

float CAICheats::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->experience;
	}
	return 0;
}

bool CAICheats::IsUnitActivated (int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->activated;
	}
	return false;
}

bool CAICheats::UnitBeingBuilt (int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->beingBuilt;
	}
	return false;
}

bool CAICheats::GetUnitResourceInfo (int unitid, UnitResourceInfo *i)
{
	CUnit* unit=uh->units[unitid];
	if(unit){
		i->energyMake = unit->energyMake;
		i->energyUse = unit->energyUse;
		i->metalMake = unit->metalMake;
		i->metalUse = unit->metalUse;
		return true;
	}
	return false;
}

const deque<Command>* CAICheats::GetCurrentUnitCommands(int unitid)
{
	CUnit *unit = uh->units[unitid];
	if (unit){
		return &unit->commandAI->commandQue;
	}
	return 0;
}

int CAICheats::GetBuildingFacing(int unitid) {
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->buildFacing;
	}
	return 0;
}

bool CAICheats::IsUnitCloaked(int unitid) {
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->isCloaked;
	}
	return false;
}

bool CAICheats::IsUnitParalyzed(int unitid){
	CUnit* unit=uh->units[unitid];
	if(unit){
		return unit->stunned;
	}
	return 0;
} 

bool CAICheats::GetProperty(int id, int property, void *data)
{
	switch (property) {
		case AIVAL_UNITDEF:{
			CUnit *unit = uh->units[id];
			if (unit) {
				(*(const UnitDef**)data) = unit->unitDef;
				return true;
			}
			break;
		}
	}
	return false;
}

bool CAICheats::GetValue(int id, void *data)
{
	switch (id) {
		default:
			return false;
	}
}

int CAICheats::HandleCommand (int commandId, void *data)
{
	switch (commandId)
	{
		case AIHCQuerySubVersionId:
			return 1; // current version of Handle Command interface
	}
	return 0;
}

IMPLEMENT_PURE_VIRTUAL(IAICheats::~IAICheats())
