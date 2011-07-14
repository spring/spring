/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include "Group.h"
#include "GroupHandler.h"
#include "Game/GlobalUnsynced.h"
// #include "Game/SelectedUnits.h"
#include "System/EventHandler.h"
#include "System/LogOutput.h"
#include "System/Platform/errorhandler.h"
#include "System/mmgr.h"
#include "System/creg/STL_Set.h"
#include "System/float3.h"

CR_BIND(CGroup, (0, NULL))

CR_REG_METADATA(CGroup, (
				CR_MEMBER(id),
				CR_MEMBER(units),
				CR_MEMBER(handler),
				CR_POSTLOAD(PostLoad)
				));

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
	CUnitSet unitBackup = units;

	for (CUnitSet::const_iterator ui = unitBackup.begin(); ui != unitBackup.end(); ++ui) {
		units.erase(*ui);
		(*ui)->group = NULL;
	}
}

bool CGroup::AddUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(group); // AddUnit

	eventHandler.GroupChanged(id);

	units.insert(unit);
	return true;
}

void CGroup::RemoveUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(group); // RemoveUnit

	eventHandler.GroupChanged(id);
	units.erase(unit);
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
	GML_RECMUTEX_LOCK(group); // ClearUnits

	eventHandler.GroupChanged(id);
	while (!units.empty()) {
		(*units.begin())->SetGroup(0);
	}
}

float3 CGroup::CalculateCenter() const
{
	float3 center = ZeroVector;

	if (!units.empty()) {
		CUnitSet::const_iterator ui;
		for (ui = units.begin(); ui != units.end(); ++ui) {
			center += (*ui)->pos;
		}
		center /= units.size();
	}

	return center;
}
