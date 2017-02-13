/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Group.h"
#include "GroupHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Units/UnitHandler.h"
#include "System/EventHandler.h"
#include "System/creg/STL_Set.h"
#include "System/float3.h"

CR_BIND(CGroup, (0, NULL))
CR_REG_METADATA(CGroup, (
	CR_MEMBER(id),
	CR_MEMBER(units),
	CR_MEMBER(handler),
	CR_POSTLOAD(PostLoad)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroup::CGroup(int id, CGroupHandler* groupHandler)
		: id(id)
		, handler(groupHandler)
{
}

CGroup::~CGroup()
{
	// should not have any units left, but just to be sure
	ClearUnits();
}



void CGroup::PostLoad()
{
	assert(unitHandler != nullptr);

	auto unitBackup = units;

	for (const int unitID: unitBackup) {
		CUnit* unit = unitHandler->GetUnit(unitID);
		units.erase(unit->id);
		unit->group = nullptr;
	}
}

bool CGroup::AddUnit(CUnit* unit)
{
	units.insert(unit->id);
	handler->PushGroupChange(id);
	return true;
}

void CGroup::RemoveUnit(CUnit* unit)
{
	units.erase(unit->id);
	handler->PushGroupChange(id);
}

void CGroup::RemoveIfEmptySpecialGroup()
{
	if (units.empty()
			&& (id >= CGroupHandler::FIRST_SPECIAL_GROUP)
			/*&& (handler == grouphandler)*/
			&& (handler->team == gu->myTeam)) // HACK so Global AI groups do not get erased DEPRECATED
	{
		handler->RemoveGroup(this);
	}
}

void CGroup::Update()
{
	RemoveIfEmptySpecialGroup();
}

void CGroup::ClearUnits()
{
	while (!units.empty()) {
		CUnit* unit = unitHandler->GetUnit(*units.begin());
		unit->SetGroup(0);
	}
	handler->PushGroupChange(id);
}

float3 CGroup::CalculateCenter() const
{
	float3 center = ZeroVector;

	if (!units.empty()) {
		for (const int unitID: units) {
			center += (unitHandler->GetUnit(unitID))->pos;
		}
		center /= units.size();
	}

	return center;
}
