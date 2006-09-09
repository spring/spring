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
	lastAlertFrame	= 0;
	lastEnterFrame	= 0;

	enemyIds 		= new int[MAX_UNITS];
	enemyIdsLos		= new int[MAX_UNITS];

	alertText 		= true;
	alertMinimap 	= true;
	showGhosts		= true;

	#define CMD_TEXT_ALERT 		150
	#define CMD_MINIMAP_ALERT 	155
	#define CMD_SHOW_GHOSTS		160
	#define CMD_DUMMY			165
}

CGroupAI::~CGroupAI()
{
	for(map<int,UnitInfo*>::iterator ui=enemyUnits.begin();ui!=enemyUnits.end();++ui)
	{
		delete ui->second;
	}
	enemyUnits.clear();
	newEnemyIds.clear();
	oldEnemyIds.clear();
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
	switch(c->id)
	{
		case CMD_TEXT_ALERT:
			alertText = !alertText;
			break;
		case CMD_MINIMAP_ALERT:
			alertMinimap = !alertMinimap;
			break;
		case CMD_SHOW_GHOSTS:
			showGhosts = !showGhosts;
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

	cd.params.clear();
	cd.id=CMD_SHOW_GHOSTS;
	cd.type=CMDTYPE_ICON_MODE;
	cd.hotkey="g";
	if(showGhosts)
	{
		cd.params.push_back("1");
	}
	else
	{
		cd.params.push_back("0");
	}
	cd.params.push_back("Ghosts off");
	cd.params.push_back("Ghosts on");
	cd.tooltip="Show ghosts of enemy units that have been in LOS";
	commands.push_back(cd);

	return commands;
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_DUMMY;
}



void CGroupAI::InsertNewEnemy(int id)
{
	float3 pos = aicb->GetUnitPos(id);
	if(aicb->PosInCamera(pos, 60))
	{
		return;
	}

	for(map<int,UnitInfo*>::const_iterator ei=enemyUnits.begin(); ei!=enemyUnits.end(); ++ei)
	{
		if(ei->first!=id)
		{
			if(currentFrame > ei->second->firstEnterFrame + 300)
			{
				if((ei->second->pos-pos).SqLength2D() < 40000.0f)
				{
					return;
				}
			}
		}
	}

	newEnemyIds.insert(id);
	if(newEnemyIds.size() == 1)
	{
		lastEnterFrame = currentFrame;
	}
}

void CGroupAI::Update()
{
	currentFrame = aicb->GetCurrentFrame();

	oldEnemyIdsBackup = oldEnemyIds;
	oldEnemyIds.clear();

	// update the info for units that are within LOS and radar
	numEnemies = aicb->GetEnemyUnitsInRadarAndLos(enemyIds);
	for(int i=0; i < numEnemies; i++)
	{
		int id = enemyIds[i];
		oldEnemyIds.insert(id);

		map<int,UnitInfo*>::iterator ei=enemyUnits.find(id);
		if(ei==enemyUnits.end())
		{
			// we haven't seen this unit before, so insert it.
			UnitInfo* info = new UnitInfo;
			info->prevLos			= false;
			info->inLos				= false;
			info->inRadar			= true;
			info->isBuilding		= false;
			info->firstEnterFrame	= currentFrame;
			info->pos				= aicb->GetUnitPos(id);
			enemyUnits[id] = info;

			// notify user of it
			InsertNewEnemy(id);
		}
		else // we have seen it before
		{
			float3 newPos = aicb->GetUnitPos(id);
			// has this unit entered LOS / radar after the last user alert?
			if(oldEnemyIdsBackup.find(id)==oldEnemyIdsBackup.end())
			{
				// only notify the user if it has moved more than a certain amount since the last time we saw it
				if(!ei->second->isBuilding)
				{
					if((ei->second->pos-newPos).SqLength2D() > 40000.0f)
					{
						InsertNewEnemy(id);
					}
				}
			}

			// update the position and the fact that's in radar range
			ei->second->pos		= newPos;
			ei->second->inRadar = true;
		}
	}

	// update the info for units that are within LOS only
	numEnemiesLos = aicb->GetEnemyUnits(enemyIdsLos);
	for(int i=0; i < numEnemiesLos; i++)
	{
		int id = enemyIdsLos[i];
		UnitInfo* info = enemyUnits[id];
		if(!info->prevLos)
		{
			// we haven't seen this one within LOS before, so store it's information
			const UnitDef* ud = aicb->GetUnitDef(id);
			info->prevLos		= true;
			info->team			= aicb->GetUnitTeam(id);
			info->name			= ud->name.c_str();
			info->isBuilding	= (ud->speed > 0) ? false : true;
		}
		info->inLos = true;
	}

	// show the ghosts of units that are within radar but outside LOS
	if(showGhosts)
	{
		map<int,UnitInfo*>::iterator ei;
		for(ei=enemyUnits.begin(); ei!=enemyUnits.end(); ++ei)
		{
			UnitInfo* info = ei->second;
			if(info->prevLos && info->inRadar && !info->inLos && !info->isBuilding)
			{
				aicb->DrawUnit(info->name,info->pos,0,1,info->team,true,false);
			}
			info->inLos		= false;
			info->inRadar	= false;
		}
	}

	// text and minimap alerts
	if(currentFrame > (lastAlertFrame + 300) && currentFrame > (lastEnterFrame + 60) && !newEnemyIds.empty())
	{
		lastAlertFrame = currentFrame;

		// text alert
		if(alertText)
		{
			int newEnemyNum = newEnemyIds.size();
			char msg[200];
			if (newEnemyIds.size() > 1)
			{
				sprintf(msg,"%i enemy units have entered LOS/radar",newEnemyNum);
			}
			else
			{
				sprintf(msg,"1 enemy unit has entered LOS/radar");
			}
			aicb->SendTextMsg(msg,0);
		}

		// minimap alert
		if(alertMinimap)
		{
			set<int>::const_iterator it;
			for(it=newEnemyIds.begin(); it!=newEnemyIds.end();++it)
			{
				aicb->AddNotification(enemyUnits[*it]->pos,float3(0.3,1,0.3),0.7f);
			}
		}
		if(alertMinimap || alertText)
		{
			aicb->SetLastMsgPos(enemyUnits[*(newEnemyIds.begin())]->pos);
		}

		// reset the data on units that are new in LOS / radar
		newEnemyIds.clear();
	}
}
