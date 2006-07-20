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
	lastUpdate			= 0;
	friendlyUnits 		= new int[MAX_UNITS];
}

CGroupAI::~CGroupAI()
{
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		delete ui->second;
	myUnits.clear();
	lockedMexxes.clear();
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	myTeam = aicb->GetMyTeam();
}

bool CGroupAI::AddUnit(int unit)
{
	const UnitDef* ud=aicb->GetUnitDef(unit);
	if(ud->buildSpeed==0){								//can only use builder units
		aicb->SendTextMsg("Cant use non builders",0);
		return false;
	}

	UnitInfo* info=new UnitInfo;
	info->maxExtractsMetal 	= 0;
	info->reclaiming		= false;
	info->building			= false;
	info->hasReported		= false;

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
	if(info->maxExtractsMetal == 0)
	{
		aicb->SendTextMsg("Must use units that can build mexes",0);
		delete info;
		return false;
	}

	myUnits[unit]=info;
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	delete myUnits[unit];
	myUnits.erase(unit);
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
	return CMD_MOVE;
}

void CGroupAI::CommandFinished(int unit,int type)
{
	UnitInfo* info=myUnits[unit];
	if(type==CMD_RECLAIM && info->reclaiming)
	{
		info->reclaiming = false;
		lockedMexxes.erase(info->nearestMex);
		// find the nearest buildsite
		const UnitDef* ud 	= aicb->GetUnitDef(info->wantedMohoName.c_str());
		float radius		= aicb->GetExtractorRadius();
		float3 pos			= aicb->ClosestBuildSite(ud,info->wantedBuildSite,radius,0);
		if(pos == float3(-1.0f,0.0f,0.0f))
		{
			aicb->SendTextMsg("Can't find a moho spot",0);
			aicb->SetLastMsgPos(aicb->GetUnitPos(unit));
			return;
		}
		// give the build order
		Command c;
		c.id = info->wantedMohoId;
		c.params.push_back(pos.x);
		c.params.push_back(pos.y);
		c.params.push_back(pos.z);

		aicb->GiveOrder(unit,&c);
		info->building = true;
	}
	if(type==info->wantedMohoId && info->building)
	{
		info->building = false;
	}
}

void CGroupAI::Update()
{
	int frameNum=aicb->GetCurrentFrame();
	if(lastUpdate<=frameNum-32)
	{
		lastUpdate = frameNum;

		for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			UnitInfo* info 	= ui->second;
			int unit		= ui->first;
			if(!info->reclaiming && !info->building)
			{
				// we're doing nothing, so it's time to find a mex we can upgrade
				minSqDist = 0;
				numUpgradableMexxes = 0;
				numFriendlyUnits = aicb->GetFriendlyUnits(friendlyUnits);
				for (int i=0; i < numFriendlyUnits; i++)
				{
					// check if it's a unit of our own (and not one of our allies).
					int mex = friendlyUnits[i];
					if(myTeam == aicb->GetUnitTeam(mex))
					{
						//check if it's a mex we can upgrade
						const UnitDef* ud = aicb->GetUnitDef(mex);
						if(ud->extractsMetal > 0 && ud->extractsMetal < info->maxExtractsMetal)
						{
							// check if it's already being reclaimed by another unit
							set<int>::iterator mi=lockedMexxes.find(mex);
							if(mi==lockedMexxes.end())
							{
								numUpgradableMexxes++;
								float sqDist = (aicb->GetUnitPos(unit) - aicb->GetUnitPos(mex)).SqLength();
								// find the nearest mex we can upgrade
								if(sqDist < minSqDist || minSqDist == 0)
								{
									minSqDist = sqDist;
									info->nearestMex = mex;
								}
							}
						} // if (upgradable)
					} // if (myTeam)
				} // for (numFriendlyUnits)

				if(numUpgradableMexxes > 0)
				{
					info->reclaiming 	= true;
					info->hasReported 	= false;
					// we have a nearest mex we can upgade.
					lockedMexxes.insert(info->nearestMex);
					// use its position for the buildsite of a moho mine
					info->wantedBuildSite = aicb->GetUnitPos(info->nearestMex);
					// reclaim it
					Command c;
					c.id=CMD_RECLAIM;
					c.options=0;
					c.params.push_back(info->nearestMex);
					aicb->GiveOrder(unit,&c);
				}
				else
				{
					if(!info->hasReported)
					{
						aicb->SendTextMsg("There are no mexes to upgrade",0);
						aicb->SetLastMsgPos(aicb->GetUnitPos(unit));
						info->hasReported = true;
					}
				}
			} // if (!reclaiming && !building)
		} // for (units)
	} // if (update)
}
