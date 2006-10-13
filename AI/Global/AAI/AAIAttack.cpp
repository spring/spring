//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#include "AAIAttack.h"
#include "AAI.h"
#include "AAISector.h"
#include "AAIGroup.h"

AAIAttack::AAIAttack(AAI *ai)
{
	this->ai = ai;

	lastAttack = 0;

	dest = 0;
}

AAIAttack::~AAIAttack(void)
{
	for(set<AAIGroup*>::iterator group = combat_groups.begin(); group != combat_groups.end(); ++group)
		(*group)->attack = 0;
	
	for(set<AAIGroup*>::iterator group = aa_groups.begin(); group != aa_groups.end(); ++group)
		(*group)->attack = 0;

	for(set<AAIGroup*>::iterator group = arty_groups.begin(); group != arty_groups.end(); ++group)
		(*group)->attack = 0;
}

bool AAIAttack::Failed()
{
	if(combat_groups.empty()) 
		return true;
	else
		return false;
}

void AAIAttack::StopAttack()
{
	float3 pos;

	// retreat groups
	for(set<AAIGroup*>::iterator group = combat_groups.begin(); group != combat_groups.end(); ++group)
	{
		pos = (*group)->rally_point;
			
		(*group)->Retreat(pos);
	
		(*group)->attack = 0;
	}

	for(set<AAIGroup*>::iterator group = aa_groups.begin(); group != aa_groups.end(); ++group)
	{
		pos = (*group)->rally_point;

		(*group)->Retreat(pos);
	
		(*group)->attack = 0;
	}

	for(set<AAIGroup*>::iterator group = arty_groups.begin(); group != arty_groups.end(); ++group)
	{
		if((*group)->move_type == GROUND)
			pos = ai->execute->GetSafePos(true, false);
		else if((*group)->move_type == SEA)
			pos = ai->execute->GetSafePos(false, true);
		else if((*group)->move_type == HOVER)
			pos = ai->execute->GetSafePos(true, true);
		else
			pos = ai->execute->GetSafePos(true, false);

		if(pos.x > 0) 
			(*group)->Retreat(pos);
		else
			(*group)->task = GROUP_IDLE;
		
		(*group)->attack = 0;
	}

	combat_groups.clear();
	aa_groups.clear();
	arty_groups.clear();
}

void AAIAttack::AttackSector(AAISector *sector, AttackType type)
{
	int unit;

	dest = sector;
	this->type = type;
	float importance = 110;

	lastAttack = ai->cb->GetCurrentFrame();


	for(set<AAIGroup*>::iterator group = combat_groups.begin(); group != combat_groups.end(); ++group)
	{
		(*group)->AttackSector(dest, importance);	
	}

	// order aa groups to guard combat units
	for(set<AAIGroup*>::iterator group = aa_groups.begin(); group != aa_groups.end(); ++group)
	{
		unit = -1;

		if(!combat_groups.empty())
			unit = (*combat_groups.begin())->GetRandomUnit();

		if(unit >= 0)
		{
			Command c;
			c.id = CMD_GUARD;

			c.params.push_back(unit);

			(*group)->GiveOrder(&c, 110, GUARDING);
		}
	}

	for(set<AAIGroup*>::iterator group = arty_groups.begin(); group != arty_groups.end(); ++group)
	{
		(*group)->AttackSector(dest, importance);
	}
}

void AAIAttack::AddGroup(AAIGroup *group)
{
	if(group->group_type == ASSAULT_UNIT)
	{
		combat_groups.insert(group);
		group->attack = this;
	}
	else if(group->group_type == ANTI_AIR_UNIT)
	{
		aa_groups.insert(group);
		group->attack = this;
	}
	else
	{
		arty_groups.insert(group);
		group->attack = this;
	} 

}

void AAIAttack::RemoveGroup(AAIGroup *group)
{
	if(group->group_type == ASSAULT_UNIT)
	{
		group->attack = 0;
		combat_groups.erase(group);
	}
	else if(group->group_type == ANTI_AIR_UNIT)	
	{
		group->attack = 0;
		aa_groups.erase(group);		
	}
	else
	{
		group->attack = 0;
		arty_groups.erase(group);
	}
}
