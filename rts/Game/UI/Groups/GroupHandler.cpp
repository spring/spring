/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>

#include "GroupHandler.h"
#include "Group.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/CameraHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/creg/STL_Set.h"
#include "System/Input/KeyInput.h"
#include "System/Log/ILog.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"

std::vector<CGroupHandler*> grouphandlers;

CR_BIND(CGroupHandler, (0))
CR_REG_METADATA(CGroupHandler, (
	CR_MEMBER(groups),
	CR_MEMBER(team),
	CR_MEMBER(freeGroups),
	CR_MEMBER(firstUnusedGroup),
	CR_MEMBER(changedGroups)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler::CGroupHandler(int teamId)
: team(teamId)
, firstUnusedGroup(FIRST_SPECIAL_GROUP)
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
		for (CGroup* g: groups) {
			if (g != nullptr) {
				// Update may invoke RemoveGroup, but this will only NULL the element, so there will be no iterator invalidation here
				g->Update();
			}
		}
	}

	{
		if (changedGroups.empty())
			return;

		std::vector<int> grpChg;

		// swap containers to prevent recursion through lua
		changedGroups.swap(grpChg);

		for (int i: grpChg) {
			eventHandler.GroupChanged(i);
		}
	}
}

bool CGroupHandler::GroupCommand(int num)
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

	return GroupCommand(num, cmd);
}

bool CGroupHandler::GroupCommand(int num, const std::string& cmd)
{
	CGroup* group = groups[num];

	if ((cmd == "set") || (cmd == "add")) {
		if (cmd == "set")
			group->ClearUnits();

		for (const int unitID: selectedUnitsHandler.selectedUnits) {
			CUnit* u = unitHandler->GetUnit(unitID);

			if (u == nullptr) {
				assert(false);
				continue;
			}

			// change group, but do not call SUH::AddUnit while iterating
			u->SetGroup(group, false, false);
		}
	}
	else if (cmd == "selectadd")  {
		// do not select the group, just add its members to the current selection
		for (const int unitID: group->units) {
			selectedUnitsHandler.AddUnit(unitHandler->GetUnit(unitID));
		}
		return true;
	}
	else if (cmd == "selectclear")  {
		// do not select the group, just remove its members from the current selection
		for (const int unitID: group->units) {
			selectedUnitsHandler.RemoveUnit(unitHandler->GetUnit(unitID));
		}
		return true;
	}
	else if (cmd == "selecttoggle")  {
		// do not select the group, just toggle its members with the current selection
		const auto& selUnits = selectedUnitsHandler.selectedUnits;

		for (const int unitID: group->units) {
			CUnit* unit = unitHandler->GetUnit(unitID);

			if (selUnits.find(unitID) == selUnits.end()) {
				selectedUnitsHandler.AddUnit(unit);
			} else {
				selectedUnitsHandler.RemoveUnit(unit);
			}
		}
		return true;
	}

	if (group->units.empty())
		return false;

	if (selectedUnitsHandler.IsGroupSelected(num)) {
		const float3 groupCenter = group->CalculateCenter();
		camHandler->CameraTransition(0.5f);
		camHandler->GetCurrentController().SetPos(groupCenter);
	}

	selectedUnitsHandler.SelectGroup(num);
	return true;
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

	if (selectedUnitsHandler.IsGroupSelected(group->id))
		selectedUnitsHandler.ClearSelected();

	groups[group->id] = NULL;
	freeGroups.push_back(group->id);
	delete group;
}

void CGroupHandler::PushGroupChange(int id)
{
	spring::VectorInsertUnique(changedGroups, id, true);
}
