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
	prevEnemies=0;
	prevEnemyIds = new int[MAX_UNITS];
	prevEnemyIdsSize=0;
	enemyIds = new int[MAX_UNITS];
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
		enemies = aicb->GetEnemyUnitsInRadarAndLos(enemyIds);
		int diffEnemies = enemies - prevEnemies;
		// do we have new enemy units in LOS / radar?
		if(diffEnemies <= 0)
		{
			prevEnemies += diffEnemies;
        }
        else if (lastEnterTime<=frameNum-320) // don't want to do this too often
		{
			// find one of the new enemy units for the msg position
			int newEnemyId = enemyIds[0]; // for if prevEnemyIdsSize == 0
			for(int i=0; i < enemies; i++)
			{
				for(int j=0; j < prevEnemyIdsSize; j++)
				{
					if(enemyIds[i] != prevEnemyIds[j])
					{
						newEnemyId = enemyIds[i];
						break;
					}
				}
			}
			// reset the enemy ids for the next Update()
			prevEnemyIdsSize = enemies;
			for(int k=0; k < enemies; k++)
			{
				prevEnemyIds[k] = enemyIds[k];
			}

			// print the message to the console
			char c[200];
			if ( diffEnemies > 1)
			{
				  sprintf(c,"Team %i: %i enemy units have entered LOS/radar",aicb->GetMyTeam(),diffEnemies);
			}
			else
			{
				  sprintf(c,"Team %i: An enemy unit has entered LOS/radar",aicb->GetMyTeam());
			}
			float3 pos = aicb->GetUnitPos(newEnemyId);
			aicb->SendTextMsg(c,0);
			aicb->SetLastMsgPos(pos);
			aicb->AddNotification(pos,float3(0.3,1,0.3),0.5);

			// update for the next call
			prevEnemies += diffEnemies;
			lastEnterTime=frameNum;
		}
	}
}
