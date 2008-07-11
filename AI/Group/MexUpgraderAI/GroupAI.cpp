// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <cassert>
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "creg/STL_Map.h"
#include "creg/STL_Set.h"
#include "creg/Serializer.h"
#include "creg/cregex.h"

#define CMD_CHANGE_MODE 	160
#define CMD_AREA_UPGRADE 	165
#define CMD_DUMMY			170

CR_BIND(CGroupAI, );

CR_REG_METADATA(CGroupAI,(
				CR_ENUM_MEMBER(mode),
				CR_MEMBER(myUnits),
				CR_MEMBER(lockedMexxes),
				CR_MEMBER(maxMetal),
				CR_MEMBER(mohoBuilderId),
				CR_MEMBER(unitsChanged),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(CGroupAI::UnitInfo, );

CR_REG_METADATA_SUB(CGroupAI, UnitInfo,(
					CR_MEMBER(maxExtractsMetal),
					CR_MEMBER(wantedMohoId),
					CR_MEMBER(wantedMohoName),
					CR_MEMBER(nearestMex),
					CR_MEMBER(wantedBuildSite),
					CR_ENUM_MEMBER(status)
					));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int CGroupAI::Instances=0;

CGroupAI::CGroupAI()
{
	if (!Instances++) creg::System::InitializeClasses();
	mode			= manual;
	maxMetal		= 0;
	mohoBuilderId	= -1;
	unitsChanged 	= false;
	friendlyUnits	= new int[MAX_UNITS];

	drawColorPath[0] = 1.0f; // R
	drawColorPath[1] = 1.0f; // G
	drawColorPath[2] = 0.1f; // B
	drawColorPath[3] = 0.9f; // A

	drawColorCircle[0] = 1.0f;
	drawColorCircle[1] = 1.0f;
	drawColorCircle[2] = 1.0f;
	drawColorCircle[3] = 0.7f;
}

CGroupAI::~CGroupAI()
{
	if (!--Instances) creg::System::FreeClasses();
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		delete ui->second;
	myUnits.clear();
	lockedMexxes.clear();
	commandQue.clear();
}

void CGroupAI::PostLoad()
{
	myTeam = aicb->GetMyTeam();
}

void CGroupAI::Reset()
{
	// clear all commands and stop all units
	lockedMexxes.clear();
	commandQue.clear();
	Command c;
	c.id = CMD_STOP;
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
	{
		ui->second->status = idle;
		aicb->GiveOrder(ui->first, &c);
	}
	unitsChanged = true; // this isn't the case, but it forces ManualFindMex to set all builders to guard the mohoBuilder.
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	myTeam = aicb->GetMyTeam();
}

bool CGroupAI::AddUnit(int unit)
{
	UnitInfo* info=new UnitInfo;
	info->maxExtractsMetal 	= 0;
	info->status			= idle;

	// determine what's the best mine this unit can build
	const vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for(vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi)		//check if this unit brings some new build options
	{
		if(cdi->id<0)			//id<0 = build option
		{
			const UnitDef* ud=aicb->GetUnitDef(cdi->name.c_str());
			if(ud->extractsMetal > info->maxExtractsMetal)
			{
				info->maxExtractsMetal	= ud->extractsMetal;
				info->wantedMohoId 		= cdi->id;
				info->wantedMohoName	= cdi->name;
			}
		}
	}
	// if it can't build a mine, we don't want it
	if(info->maxExtractsMetal == 0)
	{
		aicb->SendTextMsg("Must use units that can build mexes",0);
		delete info;
		return false;
	}
	// add it to the AI
	myUnits[unit]	= info;
	unitsChanged	= true;
	// set the unit to work.
	if(mode==automatic)
		AutoFindMex(unit);
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	// if it was reclaiming a mex, release that mex from lockedMexxes
	UnitInfo* info = myUnits[unit];
	if (info->status == reclaiming)
	{
		lockedMexxes.erase(info->nearestMex);
	}
	// do some cleaning-up
	delete myUnits[unit];
	myUnits.erase(unit);
	unitsChanged = true;
	if(mode == manual && unit == mohoBuilderId && !commandQue.empty() && !myUnits.empty())
		ManualFindMex();
}

void CGroupAI::GiveCommand(Command* c)
{
	switch(c->id)
	{
		case CMD_STOP:
			Reset();
			break;
		case CMD_CHANGE_MODE:
			if(c->params.empty())
				break;
			Reset();
			if(c->params[0]==0)
			{
				mode = automatic;
				for(map<int,UnitInfo*>::const_iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
				{
					AutoFindMex(ui->first);
				}
			}
			else if(c->params[0]==1)
			{
				mode = manual;
			}
			break;
		case CMD_AREA_UPGRADE:
			break;
		case CMD_DUMMY:
			break;
		default:
			aicb->SendTextMsg("Unknown cmd to mexUpgrader AI",0);
	}
	if(c->id==CMD_AREA_UPGRADE && c->params.size()==4)
	{
		if(!(c->options& SHIFT_KEY))
			commandQue.clear();

		Command c2;
		c2.id = CMD_RECLAIM;
		c2.params.push_back(c->params[0]);
		c2.params.push_back(c->params[1]);
		c2.params.push_back(c->params[2]);
		c2.params.push_back(c->params[3]);
		commandQue.push_back(c2);
		if(commandQue.size()==1)
			ManualFindMex();
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	commands.clear();
	CommandDescription cd;

	cd.id=CMD_CHANGE_MODE;
	cd.type=CMDTYPE_ICON_MODE;
	cd.action="onoff";
	if (mode == manual) {
		cd.params.push_back("1");
	} else { //automatic
		cd.params.push_back("0");
	}
	cd.params.push_back("Auto");
	cd.params.push_back("Manual");
	cd.tooltip="Mode: upgrade mexes manually or automatically";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_AREA_UPGRADE;
	cd.type=CMDTYPE_ICON_AREA;
	cd.name="Area upgrade";
	cd.action="repair";
	cd.tooltip="Area upgrade: drag out an area to upgrade all mexes there";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_STOP;
	cd.type=CMDTYPE_ICON;
	cd.name="Stop";
	cd.action="stop";
	commands.push_back(cd);

	return commands;
}

int CGroupAI::GetDefaultCmd(int unit)
{
	if(mode==manual)
	{
		return CMD_AREA_UPGRADE;
	}
	else
	{
		return CMD_DUMMY;
	}
}

void CGroupAI::CommandFinished(int unit,int type)
{
	map<int,UnitInfo*>::iterator ui=myUnits.find(unit);
	if(ui==myUnits.end())
	{
		//aicb->SendTextMsg("unitid not in group",0);
		return;
	}
	UnitInfo* info = ui->second;
	if(type==CMD_RECLAIM && info->status == reclaiming)
	{
		info->status = idle;
		// we are done reclaiming, so build a moho mine
		lockedMexxes.erase(info->nearestMex);
		// find the nearest buildsite
		const UnitDef* ud 	= aicb->GetUnitDef(info->wantedMohoName.c_str());
		float radius		= aicb->GetExtractorRadius();
		float3 pos			= aicb->ClosestBuildSite(ud,info->wantedBuildSite,radius,0);
		if(pos == float3(-1.0f,0.0f,0.0f))
		{
			aicb->SendTextMsg("Can't find a moho spot",0);
			aicb->SetLastMsgPos(aicb->GetUnitPos(unit));
			if(mode==manual)
			{
				ManualFindMex();
			}
			if(mode==automatic)
			{
				AutoFindMex(unit);
			}
			return;
		}
		// give the build order
		Command c;
		c.id = info->wantedMohoId;
		c.params.push_back(pos.x);
		c.params.push_back(pos.y);
		c.params.push_back(pos.z);

		aicb->GiveOrder(unit,&c);
		info->status = building;
	}
	if(type==info->wantedMohoId && info->status == building)
	{
		// we are done building a moho mine, so find a new order
		info->status = idle;
		if(mode==manual)
		{
			ManualFindMex();
		}
		if(mode==automatic)
		{
			AutoFindMex(unit);
		}
	}
}

void CGroupAI::AutoFindMex(int unit)
{
	if(myUnits[unit]->status == idle)
	{
		// we're doing nothing, so it's time to find a mex we can upgrade
		int	numFriendlyUnits = aicb->GetFriendlyUnits(friendlyUnits);
		int nearestMex = FindNearestMex(unit,friendlyUnits,numFriendlyUnits);
		if(nearestMex != -1)
		{
			// we have found an upgradable mex, so let's reclaim it first
			ReclaimMex(unit, nearestMex);
		}
		else
		{
			// no more reclaimable mexes
			aicb->SendTextMsg("There are no mexes to upgrade",0);
			aicb->SetLastMsgPos(aicb->GetUnitPos(unit));
		}
	}
}

void CGroupAI::ManualFindMex()
{
	if(unitsChanged)
	{
		// the units in the AI have changed. we might have lost our mohobuilder, so lets find a new one
		maxMetal = 0;
		for(map<int,UnitInfo*>::const_iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			float extractsMetal = ui->second->maxExtractsMetal;
			if(extractsMetal > maxMetal)
			{
				maxMetal 		= extractsMetal;
				mohoBuilderId 	= ui->first;
			}
		}
		unitsChanged = false;
		// set the moho builder to idle, and the others to guard it
		Command c1;
		c1.id = CMD_GUARD;
		c1.params.push_back(mohoBuilderId);
		for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			if(ui->first != mohoBuilderId)
			{
				aicb->GiveOrder(ui->first, &c1);
				ui->second->status = guarding;
			}
			else
			{
				ui->second->status = idle;
			}
		}
	}
	// check if there are any commands left in the queue
	if(commandQue.empty())
	{
		aicb->SendTextMsg("There are no mexes to upgrade",0);
		aicb->SetLastMsgPos(aicb->GetUnitPos(mohoBuilderId));
		return;
	}

	// we still have an area-upgrade command left, so let's find an upgradable mex
	Command& c2=commandQue.front();
	float3 pos(c2.params[0],c2.params[1],c2.params[2]);
	if(myUnits[mohoBuilderId]->status == idle)
	{
		// we're doing nothing, so it's time to find a mex we can upgrade
		int	numFriendlyUnits = aicb->GetFriendlyUnits(friendlyUnits,pos,c2.params[3]);
		int nearestMex = FindNearestMex(mohoBuilderId,friendlyUnits,numFriendlyUnits);
		if(nearestMex != -1)
		{
			ReclaimMex(mohoBuilderId, nearestMex);
		}
		else
		{
			// there aren't any upgradable mexes left in this area, so let's move on the next one
			commandQue.pop_front();
			ManualFindMex();
		}
	}
}

int CGroupAI::FindNearestMex(int unit, int* possibleMexes, int size)
{
	// we're doing nothing, so it's time to find a mex we can upgrade
	UnitInfo* info				= myUnits[unit];
	float 	minSqDist 			= 0;
	int 	nearestMex			= -1;
	for (int i=0; i < size; i++)
	{
		// check if it's a unit of our own (and not one of our allies).
		int mex = possibleMexes[i];
		if(myTeam != aicb->GetUnitTeam(mex))
			continue;

		//check if it's a mex we can upgrade
		const UnitDef* ud = aicb->GetUnitDef(mex);
		if(ud == 0) // for some reason aicb->GetUnitDef(mex) returns 0 from time to time
			continue;
		if(ud->extractsMetal > 0 && ud->extractsMetal < info->maxExtractsMetal)
		{
			// check if it's already being reclaimed by another unit
			set<int>::const_iterator mi=lockedMexxes.find(mex);
			if(mi==lockedMexxes.end())
			{
				float sqDist = (aicb->GetUnitPos(unit) - aicb->GetUnitPos(mex)).SqLength();
				// find the nearest mex we can upgrade
				if(sqDist < minSqDist || minSqDist == 0)
				{
					minSqDist = sqDist;
					nearestMex= mex;
				}
			}
		} // if (upgradable)
	} // for (size)
	return nearestMex;
}

void CGroupAI::ReclaimMex(int unit, int mex)
{
	UnitInfo* info = myUnits[unit];
	info->nearestMex 		= mex;
	info->status 			= reclaiming;
	info->wantedBuildSite 	= aicb->GetUnitPos(info->nearestMex);
	lockedMexxes.insert(info->nearestMex);
	// reclaim it
	Command c;
	c.id=CMD_RECLAIM;
	c.options=0;
	c.params.push_back(info->nearestMex);
	aicb->GiveOrder(unit,&c);
}

void CGroupAI::DrawCommands()
{
	// draw the queued-up commands if the group is selected
	if(mode==manual && callback->IsSelected())
	{
		float3 pos1, pos2;
		CCommandQueue::const_iterator ci;

		aicb->LineDrawerStartPath(aicb->GetUnitPos(mohoBuilderId), drawColorPath);
		for(ci=commandQue.begin();ci!=commandQue.end();++ci)
			aicb->LineDrawerDrawLine(float3(ci->params[0],ci->params[1],ci->params[2]),drawColorPath);
		aicb->LineDrawerFinishPath();

		for(ci=commandQue.begin();ci!=commandQue.end();++ci)
		{
			float radius	= ci->params[3];
			float3 pos1		= float3(ci->params[0],ci->params[1],ci->params[2]);
			for(int a=0;a<=20;++a)
			{
				pos2	= float3(pos1.x+sin(a*PI*2/20)*radius,0,pos1.z+cos(a*PI*2/20)*radius);
				pos2.y	= aicb->GetElevation(pos2.x,pos2.z)+5;
				if(a==0)
					aicb->LineDrawerStartPath(pos2,drawColorCircle);
				else
					aicb->LineDrawerDrawLine(pos2,drawColorCircle);
			}
			aicb->LineDrawerFinishPath();
		}
	}
}

CREX_REG_STATE_COLLECTOR(MexUpgraderAI,CGroupAI);

void CGroupAI::Load(IGroupAICallback* callback,std::istream *ifs)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	CREX_SC_LOAD(MexUpgraderAI,ifs);
}

void CGroupAI::Save(std::ostream *ofs)
{
	CREX_SC_SAVE(MexUpgraderAI,ofs);
}
