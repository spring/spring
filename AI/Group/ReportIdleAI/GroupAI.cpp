// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"

#define CMD_DUMMY			170

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupAI::CGroupAI()
{
}

CGroupAI::~CGroupAI()
{
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
}

bool CGroupAI::AddUnit(int unit)
{
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
}

void CGroupAI::GiveCommand(Command* c)
{
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	return commands;
}

int CGroupAI::GetDefaultCmd(int unit)
{
	return CMD_DUMMY;
}

void CGroupAI::CommandFinished(int unit,int type)
{
	if(type != CMD_STOP && type != CMD_MOVE)
	{
		const deque<Command>* commandQue = aicb->GetCurrentUnitCommands(unit);
		if (commandQue->empty() ||
		    ((commandQue->size() == 1) &&
		     (commandQue->front().id == CMD_SET_WANTED_MAX_SPEED))) {
			aicb->SendTextMsg("Builder idle",0);
			aicb->SetLastMsgPos(aicb->GetUnitPos(unit));
		}
	}
}
void CGroupAI::Update()
{
}
