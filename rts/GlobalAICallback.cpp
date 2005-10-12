#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "Net.h"
#include "GlobalAI.h"
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
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "IGroupAI.h"
#include "PathManager.h"
#include "AICheats.h"
#include "GameSetup.h"
#include "SmfReadMap.h"
#include "Wind.h"
#include "UnitDrawer.h"
#include "Player.h"
//#include "mmgr.h"

CGlobalAICallback::CGlobalAICallback(CGlobalAI* ai)
: ai(ai),
	cheats(0), 
	scb(ai->team, ai->gh)
{
}

CGlobalAICallback::~CGlobalAICallback(void)
{
	delete cheats;
}

IAICheats* CGlobalAICallback::GetCheatInterface()
{
	if(cheats)
		return cheats;

	if(!gs->cheatEnabled)
		return 0;

	cheats=new CAICheats(ai);
	return cheats;
}

IAICallback *CGlobalAICallback::GetAICallback ()
{
	return &scb;
}

