// GroupAICallback.cpp: implementation of the CGroupAICallback class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAiCallback.h"
#include "Net.h"
#include "ReadMap.h"
#include "LosHandler.h"
#include "InfoConsole.h"
#include "Group.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "Team.h"
//#include "multipath.h"
//#include "PathHandler.h"
#include "QuadField.h"
#include "SelectedUnits.h"
#include "GeometricObjects.h"
#include "CommandAI.h"
#include "GameHelper.h"
#include "UnitDefHandler.h"
#include "GuiHandler.h"		//todo: fix some switch for new gui
#include "NewGuiDefine.h"
#include "GUIcontroller.h"
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "PathManager.h"
#include "UnitDrawer.h"
#include "Player.h"
//#include "mmgr.h"

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

