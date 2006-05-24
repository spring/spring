// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupAI::CGroupAI()
{
	lastUpdate=0;
	lastEnterTime=0;
	numEnemies=0;
    enemies = new int[10000]; 
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
	myUnits.insert(unit);
	return true;		
}

void CGroupAI::RemoveUnit(int unit)
{
	myUnits.erase(unit);
}

void CGroupAI::GiveCommand(Command* c)
{
 	for(set<int>::iterator si=myUnits.begin();si!=myUnits.end();++si){
		aicb->GiveOrder(*si,c);
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	return commands;
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_MOVE;
}


void CGroupAI::Update()
{
	int frameNum=aicb->GetCurrentFrame();
	if(lastUpdate<=frameNum-32)
	{ 
		lastUpdate=frameNum;
		int diffNum = aicb->GetEnemyUnitsInRadarAndLos(enemies) - numEnemies;
		if (diffNum <= 0)
		{
			numEnemies += diffNum;
        }
        else if (diffNum > 0 && lastEnterTime<=frameNum-320)
		{
            char c[200];
            if ( diffNum > 1)
            {
                  sprintf(c,"Team %i: %i enemy units have entered LOS/radar",aicb->GetMyTeam(),diffNum);
            }
            else
            {
                  sprintf(c,"Team %i: An enemy unit has entered LOS/radar",aicb->GetMyTeam());
            }
            aicb->SendTextMsg(c,0);
            numEnemies += diffNum;
            lastEnterTime=frameNum;
        }
     }
}
