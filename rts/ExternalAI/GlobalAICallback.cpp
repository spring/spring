#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "GlobalAI.h"
#include "LogOutput.h"
#include "GroupHandler.h"
#include "AICheats.h"
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

	logOutput.Print ("AI has enabled cheating.");
	cheats=new CAICheats(ai);
	return cheats;
}

IAICallback *CGlobalAICallback::GetAICallback ()
{
	return &scb;
}

IMPLEMENT_PURE_VIRTUAL(IGlobalAICallback::~IGlobalAICallback())
