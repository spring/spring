// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIAttack.h"
#include "AAI.h"
#include "AAIAttackManager.h"
#include "AAIMap.h"
#include "AAIGroup.h"

#include "LegacyCpp/IAICallback.h"
using namespace springLegacyAI;

AAIAttack::AAIAttack(AAI *ai):
	dest(NULL),
	lastAttack(0),
	land(false),
	water(false)
{
	this->ai = ai;
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
	if(!combat_groups.empty())
	{
		// check if still enough power to attack target sector
		if(ai->Getam()->SufficientAttackPowerVS(dest, &combat_groups, 1.3f))
		{
			// check if sufficient power to combat enemy units
			float3 pos = (*combat_groups.begin())->GetGroupPos();
			AAISector *sector = ai->Getmap()->GetSectorOfPos(&pos);

			if(sector && ai->Getam()->SufficientCombatPowerAt(sector, &combat_groups, 2))
				return false;
		}
	}

	return true;
}

void AAIAttack::StopAttack()
{
	float3 pos;
//	AAISector *sector;

	for(set<AAIGroup*>::iterator group = combat_groups.begin(); group != combat_groups.end(); ++group)
	{
		// get rally point somewhere between current pos an base
		pos = (*group)->GetGroupPos();
//		sector = ai->Getmap()->GetSectorOfPos(&pos);

		(*group)->GetNewRallyPoint();

		/*if(sector)
		{
			int max_dist = sector->distance_to_base/2;
			int min_dist = max_dist - 1;

			if(min_dist < 1)
				min_dist = 1;


			pos = ai->execute->GetRallyPointCloseTo((*group)->category, (*group)->group_movement_type, ((*group)->GetGroupPos() + ai->brain->base_center) / 2.0f, min_dist, max_dist);

			if(pos.x > 0)
				(*group)->rally_point = pos;
		}*/

		(*group)->Retreat(&(*group)->rally_point);
		(*group)->attack = 0;
	}

	for(set<AAIGroup*>::iterator group = aa_groups.begin(); group != aa_groups.end(); ++group)
	{
		// get rally point somewhere between current pos an base
		pos = (*group)->GetGroupPos();
//		sector = ai->Getmap()->GetSectorOfPos(&pos);

		(*group)->GetNewRallyPoint();

		/*if(sector)
		{
			int max_dist = sector->distance_to_base/2;
			int min_dist = max_dist - 1;

			if(min_dist < 1)
				min_dist = 1;

			(*group)->GetNewRallyPoint();
		}*/

		(*group)->Retreat(&(*group)->rally_point);
		(*group)->attack = 0;
	}

	for(set<AAIGroup*>::iterator group = arty_groups.begin(); group != arty_groups.end(); ++group)
	{
		// todo
	}

	combat_groups.clear();
	aa_groups.clear();
	arty_groups.clear();
}

void AAIAttack::AttackSector(AAISector *sector)
{
	int unit;
	float importance = 110;

	dest = sector;

	lastAttack = ai->Getcb()->GetCurrentFrame();

	for(set<AAIGroup*>::iterator group = combat_groups.begin(); group != combat_groups.end(); ++group)
	{
		(*group)->AttackSector(dest, importance);
	}

	// order aa groups to guard combat units
	if(!combat_groups.empty())
	{
		for(set<AAIGroup*>::iterator group = aa_groups.begin(); group != aa_groups.end(); ++group)
		{
			unit = (*combat_groups.begin())->GetRandomUnit();

			if(unit >= 0)
			{
				Command c(CMD_GUARD);
				c.PushParam(unit);

				(*group)->GiveOrder(&c, 110, GUARDING, "Group::AttackSector");
			}
		}
	}

	for(set<AAIGroup*>::iterator group = arty_groups.begin(); group != arty_groups.end(); ++group)
	{
		(*group)->AttackSector(dest, importance);
	}
}

void AAIAttack::AddGroup(AAIGroup *group)
{
	if(group->group_unit_type == ASSAULT_UNIT)
	{
		combat_groups.insert(group);
		group->attack = this;
	}
	else if(group->group_unit_type == ANTI_AIR_UNIT)
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
	if(group->group_unit_type == ASSAULT_UNIT)
	{
		combat_groups.erase(group);
	}
	else if(group->group_unit_type == ANTI_AIR_UNIT)
	{
		aa_groups.erase(group);
	}
	else
	{
		arty_groups.erase(group);
	}

	ai->Getam()->CheckAttack(this);
}
