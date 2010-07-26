/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Group.h"
#include "GroupHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/Unit.h"
#include "GlobalUnsynced.h"
#include "EventHandler.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "mmgr.h"
#include "creg/STL_List.h"
#include "creg/STL_Set.h"

CR_BIND(CGroup, (0,NULL))

CR_REG_METADATA(CGroup, (
				CR_MEMBER(id),
				CR_MEMBER(units),
				CR_MEMBER(myCommands),
				CR_MEMBER(lastCommandPage),
				CR_MEMBER(handler),
				CR_SERIALIZER(Serialize),
				CR_POSTLOAD(PostLoad)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroup::CGroup(int id, CGroupHandler* grouphandler)
		: id(id),
		lastCommandPage(0),
		handler(grouphandler)
{
}

CGroup::~CGroup()
{
	// should not have any units left but just to be sure
	ClearUnits();
}

void CGroup::Serialize(creg::ISerializer *s)
{
}

void CGroup::PostLoad()
{
	CUnitSet unitBackup = units;

	for(CUnitSet::iterator ui=unitBackup.begin();ui!=unitBackup.end();++ui)
	{
		units.erase(*ui);
		(*ui)->group=0;
	}
}

bool CGroup::AddUnit(CUnit *unit)
{
	GML_RECMUTEX_LOCK(group); // AddUnit

	eventHandler.GroupChanged(id);

	units.insert(unit);
	return true;
}

void CGroup::RemoveUnit(CUnit *unit)
{
	GML_RECMUTEX_LOCK(group); // RemoveUnit

	eventHandler.GroupChanged(id);
	units.erase(unit);
}

void CGroup::Update()
{
	// last check is a hack so Global AI groups dont get erased
	if (units.empty() && (id >= 10) && /*(handler == grouphandler) && */(handler->team == gu->myTeam)) {
		handler->RemoveGroup(this);
		return;
	}
}

void CGroup::DrawCommands()
{
	// last check is a hack so Global AI groups dont get erased
	if (units.empty() && (id >= 10) && /*(handler == grouphandler) && */(handler->team == gu->myTeam)) {
		handler->RemoveGroup(this);
		return;
	}
}

const vector<CommandDescription>& CGroup::GetPossibleCommands()
{
	CommandDescription c;

	myCommands.clear();

	// here, Group AI commands were added, when they still existed

	return myCommands;
}

int CGroup::GetDefaultCmd(const CUnit *unit, const CFeature* feature) const
{
	return CMD_STOP;
}

void CGroup::GiveCommand(Command c)
{
	// There are no commands that a group could receive
	// TODO: possible FIXME: give command to all units in the group that can handle it
}

void CGroup::CommandFinished(int unitId, int commandTopicId)
{
}

void CGroup::ClearUnits(void)
{
	GML_RECMUTEX_LOCK(group); // ClearUnits

	eventHandler.GroupChanged(id);
	while(!units.empty()){
		(*units.begin())->SetGroup(0);
	}
}
