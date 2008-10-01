#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "SkirmishAIWrapper.h"
#include "LogOutput.h"
#include "GroupHandler.h"
#include "AICheats.h"
#include "mmgr.h"

CGlobalAICallback::CGlobalAICallback(CSkirmishAIWrapper* ai):
	ai(ai),
	cheatCallback(0),
	callback(ai->GetTeamId(), grouphandlers[ai->GetTeamId()])
{
}

CGlobalAICallback::~CGlobalAICallback()
{
	delete cheatCallback;
}

IAICheats* CGlobalAICallback::GetCheatInterface()
{
	if (cheatCallback)
		return cheatCallback;

	logOutput.Print("SkirmishAI (with team ID = %i): Cheating enabled!", ai->GetTeamId());
	cheatCallback = SAFE_NEW CAICheats(ai);
	return cheatCallback;
}

IAICallback* CGlobalAICallback::GetAICallback()
{
	return &callback;
}

IMPLEMENT_PURE_VIRTUAL(IGlobalAICallback::~IGlobalAICallback())
