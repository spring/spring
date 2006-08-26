// GroupAICallback.cpp: implementation of the CGroupAICallback class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAiCallback.h"
#include "Group.h"
#include "IGroupAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/UI/GuiHandler.h"		//todo: fix some switch for new gui
#include "Game/UI/NewGuiDefine.h"
#include "Game/UI/GUI/GUIcontroller.h"
#include "GroupHandler.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;

CGroupAICallback::CGroupAICallback(CGroup* group)
: group(group), aicb(group->handler->team, group->handler)
{
	aicb.group = group;
}

CGroupAICallback::~CGroupAICallback()
{}

IAICallback* CGroupAICallback::GetAICallback ()
{
	return &aicb;
}

void CGroupAICallback::UpdateIcons()
{
	selectedUnits.PossibleCommandChange(0);
}

const Command* CGroupAICallback::GetOrderPreview()
{
	static Command tempcmd;
	//todo: need to add support for new gui
#ifdef NEW_GUI
	tempcmd=guicontroller->GetOrderPreview();
#else
	tempcmd=guihandler->GetOrderPreview();
#endif
	return &tempcmd;
}

bool CGroupAICallback::IsSelected()
{
	return selectedUnits.selectedGroup==group->id;
}

int CGroupAICallback::GetUnitLastUserOrder(int unitid)
{
	CUnit *unit = uh->units[unitid];
	if (unit->group == group)
		return uh->units[unitid]->commandAI->lastUserCommand;
	return 0;
}

IMPLEMENT_PURE_VIRTUAL(IGroupAICallback::~IGroupAICallback())
IMPLEMENT_PURE_VIRTUAL(IGroupAI::~IGroupAI())
