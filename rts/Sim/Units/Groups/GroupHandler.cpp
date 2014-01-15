/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keycode.h>

#include "GroupHandler.h"
#include "Group.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_Set.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/FileSystem.h"
#include "System/EventHandler.h"

std::vector<CGroupHandler*> grouphandlers;

CR_BIND(CGroupHandler, (0))
CR_REG_METADATA(CGroupHandler, (
	CR_MEMBER(groups),
	CR_MEMBER(team),
	CR_MEMBER(freeGroups),
	CR_MEMBER(firstUnusedGroup),
	CR_MEMBER(changedGroups)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler::CGroupHandler(int teamId)
		: team(teamId),
		firstUnusedGroup(FIRST_SPECIAL_GROUP)
{
	for (int g = 0; g < FIRST_SPECIAL_GROUP; ++g) {
		groups.push_back(new CGroup(g, this));
	}
}

CGroupHandler::~CGroupHandler()
{
	for (int g = 0; g < firstUnusedGroup; ++g) {
		delete groups[g];
	}
}

void CGroupHandler::Update()
{
	{
		for (std::vector<CGroup*>::iterator gi = groups.begin(); gi != groups.end(); ++gi) {
			if ((*gi) != NULL) {
				// Update may invoke RemoveGroup, but this will only NULL the element, so there will be no iterator invalidation here
				(*gi)->Update();
			}
		}
	}

	std::set<int> grpChg;
	{
		if (changedGroups.empty())
			return;

		changedGroups.swap(grpChg);
	}
	// this batching mechanism is to prevent lua related readlocks for MT
	for (std::set<int>::iterator i = grpChg.begin(); i != grpChg.end(); ++i) {
		eventHandler.GroupChanged(*i);
	}
}

void CGroupHandler::GroupCommand(int num)
{
	std::string cmd = "";

	if (KeyInput::GetKeyModState(KMOD_CTRL)) {
		if (!KeyInput::GetKeyModState(KMOD_SHIFT)) {
			cmd = "set";
		} else {
			cmd = "add";
		}
	} else if (KeyInput::GetKeyModState(KMOD_SHIFT))  {
		cmd = "selectadd";
	} else if (KeyInput::GetKeyModState(KMOD_ALT)) {
		cmd = "selecttoggle";
	}

	GroupCommand(num, cmd);
}

void CGroupHandler::GroupCommand(int num, const std::string& cmd)
{
	CGroup* group = groups[num];

	if ((cmd == "set") || (cmd == "add")) {
		if (cmd == "set") {
			group->ClearUnits();
		}
		const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
		CUnitSet::const_iterator ui;
		for(ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
			(*ui)->SetGroup(group);
		}
	}
	else if (cmd == "selectadd")  {
		// do not select the group, just add its members to the current selection
		CUnitSet::const_iterator ui;
		for (ui = group->units.begin(); ui != group->units.end(); ++ui) {
			selectedUnitsHandler.AddUnit(*ui);
		}
		return;
	}
	else if (cmd == "selectclear")  {
		// do not select the group, just remove its members from the current selection
		CUnitSet::const_iterator ui;
		for (ui = group->units.begin(); ui != group->units.end(); ++ui) {
			selectedUnitsHandler.RemoveUnit(*ui);
		}
		return;
	}
	else if (cmd == "selecttoggle")  {
		// do not select the group, just toggle its members with the current selection
		const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
		CUnitSet::const_iterator ui;
		for (ui = group->units.begin(); ui != group->units.end(); ++ui) {
			if (selUnits.find(*ui) == selUnits.end()) {
				selectedUnitsHandler.AddUnit(*ui);
			} else {
				selectedUnitsHandler.RemoveUnit(*ui);
			}
		}
		return;
	}

	if ((selectedUnitsHandler.IsGroupSelected(num)) && !group->units.empty()) {
		const float3 groupCenter = group->CalculateCenter();
		camHandler->CameraTransition(0.5f);
		camHandler->GetCurrentController().SetPos(groupCenter);
	}

	selectedUnitsHandler.SelectGroup(num);
}

CGroup* CGroupHandler::CreateNewGroup()
{
	if (freeGroups.empty()) {
		CGroup* group = new CGroup(firstUnusedGroup++, this);
		groups.push_back(group);
		return group;
	} else {
		int id = freeGroups.back();
		freeGroups.pop_back();
		CGroup* group = new CGroup(id, this);
		groups[id] = group;
		return group;
	}
}

void CGroupHandler::RemoveGroup(CGroup* group)
{
	if (group->id < FIRST_SPECIAL_GROUP) {
		LOG_L(L_WARNING, "Trying to remove hot-key group %i", group->id);
		return;
	}
	if (selectedUnitsHandler.IsGroupSelected(group->id)) {
		selectedUnitsHandler.ClearSelected();
	}
	groups[group->id] = NULL;
	freeGroups.push_back(group->id);
	delete group;
}

void CGroupHandler::PushGroupChange(int id)
{
	changedGroups.insert(id);
}

