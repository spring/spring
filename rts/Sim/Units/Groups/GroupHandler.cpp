/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keysym.h>

#include "StdAfx.h"
#include "mmgr.h"

#include "GroupHandler.h"
#include "Group.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
#include "Sim/Units/Unit.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/FileSystem.h"

std::vector<CGroupHandler*> grouphandlers;

CR_BIND(CGroupHandler, (0))

CR_REG_METADATA(CGroupHandler, (
				CR_MEMBER(groups),
				CR_MEMBER(team),
				CR_MEMBER(freeGroups),
				CR_MEMBER(firstUnusedGroup)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler::CGroupHandler(int team)
		: team(team),
		firstUnusedGroup(10)
{
	for (int g = 0; g < 10; ++g) {
		groups.push_back(new CGroup(g, this));
	}
}

CGroupHandler::~CGroupHandler()
{
	for(int g = 0; g < firstUnusedGroup; ++g) {
		delete groups[g];
	}
}

void CGroupHandler::Update()
{
	for (std::vector<CGroup*>::iterator g = groups.begin(); g != groups.end(); ++g) {
		if ((*g) != NULL) {
			(*g)->Update();
		}
	}
}

void CGroupHandler::DrawCommands()
{
	for (std::vector<CGroup*>::iterator g = groups.begin(); g != groups.end(); ++g) {
		if ((*g) != NULL) {
			(*g)->DrawCommands();
		}
	}
}

void CGroupHandler::GroupCommand(int num)
{
	GML_RECMUTEX_LOCK(grpsel); // GroupCommand

	std::string cmd = "";

	if (keyInput->IsKeyPressed(SDLK_LCTRL)) {
		if (!keyInput->IsKeyPressed(SDLK_LSHIFT)) {
			cmd = "set";
		} else {
			cmd = "add";
		}
	} else if (keyInput->IsKeyPressed(SDLK_LSHIFT))  {
		cmd = "selectadd";
	} else if (keyInput->IsKeyPressed(SDLK_LALT)) {
		cmd = "selecttoggle";
	}

	GroupCommand(num, cmd);
}

void CGroupHandler::GroupCommand(int num, const std::string& cmd)
{
	GML_RECMUTEX_LOCK(grpsel); // GroupCommand

	if ((cmd == "set") || (cmd == "add")) {
		if (cmd == "set") {
			groups[num]->ClearUnits();
		}
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator ui;
		for(ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
			(*ui)->SetGroup(groups[num]);
		}
	}
	else if (cmd == "selectadd")  {
		// do not select the group, just add its members to the current selection
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			selectedUnits.AddUnit(*gi);
		}
		return;
	}
	else if (cmd == "selectclear")  {
		// do not select the group, just remove its members from the current selection
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			selectedUnits.RemoveUnit(*gi);
		}
		return;
	}
	else if (cmd == "selecttoggle")  {
		// do not select the group, just toggle its members with the current selection
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			if (selUnits.find(*gi) == selUnits.end()) {
				selectedUnits.AddUnit(*gi);
			} else {
				selectedUnits.RemoveUnit(*gi);
			}
		}
		return;
	}

	if ((selectedUnits.selectedGroup == num) && !groups[num]->units.empty()) {
		float3 p(0.0f, 0.0f, 0.0f);
		for (CUnitSet::iterator gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			p += (*gi)->pos;
		}
		p /= groups[num]->units.size();
		camHandler->GetCurrentController().SetPos(p);
	}

	selectedUnits.SelectGroup(num);
}

CGroup* CGroupHandler::CreateNewGroup()
{
	GML_RECMUTEX_LOCK(group); // GroupCommand

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
	GML_RECMUTEX_LOCK(grpsel); // RemoveGroup

	if (group->id < 10) {
		logOutput.Print("Warning trying to remove hotkey group %i", group->id);
		return;
	}
	if (selectedUnits.selectedGroup == group->id) {
		selectedUnits.ClearSelected();
	}
	groups[group->id] = NULL;
	freeGroups.push_back(group->id);
	delete group;
}
