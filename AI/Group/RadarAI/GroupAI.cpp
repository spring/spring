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
	lastUpdate		= 0;
	lastEnterTime	= 0;
	numPrevEnemies	= 0;
	enemyIds 		= new int[MAX_UNITS];

	alertText 		= true;
	alertMinimap 	= true;

	#define CMD_TEXT_ALERT 		150
	#define CMD_MINIMAP_ALERT 	155
	#define CMD_DUMMY			156
}

CGroupAI::~CGroupAI()
{
}

bool CGroupAI::IsUnitSuited(const UnitDef* unitDef)
{
	if(unitDef->radarRadius==0 && unitDef->sonarRadius==0)
		return false;
	else
		return true;
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
}

bool CGroupAI::AddUnit(int unit)
{
	// check if it's a radar-capable unit
	const UnitDef* ud=aicb->GetUnitDef(unit);
	if(!IsUnitSuited(ud))
	{
		aicb->SendTextMsg("Cant use non-radar units",0);
		return false;
	}
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
}

void CGroupAI::GiveCommand(Command* c)
{
	switch(c->id)
	{
		case CMD_TEXT_ALERT:
			alertText = !alertText;
			break;
		case CMD_MINIMAP_ALERT:
			alertMinimap = !alertMinimap;
			break;
		case CMD_DUMMY:
			break;
		default:
			aicb->SendTextMsg("Unknown command to RadarAI",0);
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	commands.clear();
	CommandDescription cd;

	cd.id=CMD_TEXT_ALERT;
	cd.type=CMDTYPE_ICON_MODE;
	cd.hotkey="t";
	if(alertText)
	{
		cd.params.push_back("1");
	}
	else
	{
		cd.params.push_back("0");
	}
	cd.params.push_back("Text off");
	cd.params.push_back("Text on");
	cd.tooltip="Show a text message upon intruder alert";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_MINIMAP_ALERT;
	cd.type=CMDTYPE_ICON_MODE;
	cd.hotkey="m";
	if(alertMinimap)
	{
		cd.params.push_back("1");
	}
	else
	{
		cd.params.push_back("0");
	}
	cd.params.push_back("Minimap off");
	cd.params.push_back("Minimap on");
	cd.tooltip="Show a minimap alert upon intruder alert";
	commands.push_back(cd);

	return commands;
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_DUMMY;
}


void CGroupAI::Update()
{
	int frameNum=aicb->GetCurrentFrame();
	if(lastUpdate<=frameNum-32)
	{
		lastUpdate=frameNum;
		numEnemies = aicb->GetEnemyUnitsInRadarAndLos(enemyIds);
		int diffEnemies = numEnemies - numPrevEnemies;
		// do we have new enemy units in LOS / radar?
		if(diffEnemies <= 0)
		{
			numPrevEnemies += diffEnemies;
        }
        else if (lastEnterTime<=frameNum-320) // don't want to do this too often
		{
			// find one of the new enemy units for the msg position
			int newEnemyId = enemyIds[0]; // for if numPrevEnemies == 0
			for(int i=0; i < numEnemies; i++)
			{
				set<int>::const_iterator ei=prevEnemyIds.find(enemyIds[i]);
				if(ei==prevEnemyIds.end())
				{
					newEnemyId = enemyIds[i];
					break;
				}
			}
			// reset the enemy ids for the next Update()
			prevEnemyIds.clear();
			for(int i=0; i < numEnemies; i++)
			{
				prevEnemyIds.insert(enemyIds[i]);
			}

			// print the message to the console
			char c[200];
			if ( diffEnemies > 1)
			{
				  sprintf(c,"%i enemy units have entered LOS/radar",diffEnemies);
			}
			else
			{
				  sprintf(c,"1 enemy unit has entered LOS/radar");
			}
			float3 pos = aicb->GetUnitPos(newEnemyId);
			if(alertText)					aicb->SendTextMsg(c,0);
			if(alertMinimap)				aicb->AddNotification(pos,float3(0.3,1,0.3),0.5);
			if(alertText || alertMinimap)	aicb->SetLastMsgPos(pos);

			// update for the next call
			numPrevEnemies += diffEnemies;
			lastEnterTime=frameNum;
		}
	}
}
