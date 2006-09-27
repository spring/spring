#include "StdAfx.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "BoHandler.h"
#include <stdarg.h>

CBoHandler::CBoHandler(IAICallback* aicb,float mmkrME,float avgMetal)
{
	this->aicb		= aicb;
	this->mmkrME	= mmkrME;
	this->avgMetal	= avgMetal;
	tidalStrength	= aicb->GetTidalStrength();
	avgWind			= (aicb->GetMinWind() + aicb->GetMaxWind())/2;
	BOchanged		= false;
}

CBoHandler::~CBoHandler()
{
	ClearBuildOptions();
}

void CBoHandler::ClearBuildOptions()
{
	for(map<string,BOInfo*>::iterator boi=allBO.begin();boi!=allBO.end();++boi)
		delete boi->second;
	allBO.clear();
}

void CBoHandler::AddBuildOptions(const UnitDef* unitDef)
{
	if(unitDef->buildOptions.empty())
		return;
	for(map<int,string>::const_iterator boi=unitDef->buildOptions.begin();boi!=unitDef->buildOptions.end();++boi)
	{
		if(allBO.find(boi->second)!=allBO.end())
			continue;

		BOInfo* info = new BOInfo;
		const UnitDef* ud=aicb->GetUnitDef(boi->second.c_str());
		info->name			= boi->second;
		info->energyCost	= ud->energyCost;
		info->metalCost		= ud->metalCost;
		info->buildTime		= ud->buildTime; // this is always 1 or bigger
		info->totalCost		= max(1.0f,info->energyCost + (info->metalCost / mmkrME));
		info->isMex			= (ud->type=="MetalExtractor") ? true : false;
		info->isGeo			= (ud->needGeo) ? true : false;

		info->mp = ud->extractsMetal*avgMetal + ud->metalMake + ud->makesMetal - ud->metalUpkeep;
		info->ep = ud->energyMake - ud->energyUpkeep + ud->tidalGenerator*tidalStrength + min(ud->windGenerator,avgWind);
		info->me = info->mp / max(ud->energyUpkeep,1.0f);
		info->em = info->ep / max(ud->metalUpkeep,1.0f);

		allBO[info->name] = info;

		BOchanged = true;
	}
	return;
}

void CBoHandler::SortBuildOptions()
{
	if(BOchanged)
	{
		BOchanged = false;
		bestMetal.clear();
		bestEnergy.clear();
		for(map<string,BOInfo*>::const_iterator boi=allBO.begin();boi!=allBO.end();++boi)
		{
			BOInfo* info = boi->second;
			if(info->mp > 0) bestMetal.push_back(info);
			if(info->ep > 0) bestEnergy.push_back(info);
		}
		sort(bestMetal.begin(),bestMetal.end(),compareMetal());
		sort(bestEnergy.begin(),bestEnergy.end(),compareEnergy());
	}
}