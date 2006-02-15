#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "Net.h"
#include "GlobalAI.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Group.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Game/Team.h"
//#include "multipath.h"
//#include "PathHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/GameHelper.h"
#include "Sim/Units/UnitDefHandler.h"
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "IGroupAI.h"
#include "Sim/Path/PathManager.h"
#include "AICheats.h"
#include "Game/GameSetup.h"
#include "Sim/Map/SmfReadMap.h"
#include "Sim/Misc/Wind.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Game/Player.h"
#include "Game/UI/InfoConsole.h"
#include "mmgr.h"

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

	// Cheating does not generate network commands, so it would desync a multiplayer game
	if(!net->onlyLocal) {
		info->AddLine (0,"AI cheating is only possible in singleplayer games");
		return 0;
	}

	info->AddLine ("AI has enabled cheating.");
	cheats=new CAICheats(ai);
	return cheats;
}

IAICallback *CGlobalAICallback::GetAICallback ()
{
	return &scb;
}

