#include "StdAfx.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "BoHandler.h"
#include <stdarg.h>

CBoHandler::CBoHandler(IAICallback* aicb,float mmkrME,float avgMetal,float maxPartitionRadius)
{
	this->aicb		= aicb;
	this->mmkrME	= mmkrME;
	this->avgMetal	= avgMetal;
	this->maxPartitionRadius = maxPartitionRadius;
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
	for (std::map<std::string,BOInfo*>::iterator boi=allBO.begin();boi!=allBO.end();++boi)
		delete boi->second;
	allBO.clear();
}

void CBoHandler::AddBuildOptions(const UnitDef* unitDef)
{
	if(unitDef->buildOptions.empty())
		return;
	for (std::map<int, std::string>::const_iterator boi=unitDef->buildOptions.begin();boi!=unitDef->buildOptions.end();++boi)
	{
		if(allBO.find(boi->second)!=allBO.end())
			continue;

		BOInfo* info = new BOInfo;
		const UnitDef* ud=aicb->GetUnitDef(boi->second.c_str());
		info->name			= boi->second;
		info->energyCost	= ud->energyCost;
		info->metalCost		= ud->metalCost;
		info->buildTime		= ud->buildTime; // this is always 1 or bigger
		info->totalCost		= std::max(1.0f,info->energyCost + (info->metalCost / mmkrME));
		info->isMex			= (ud->type=="MetalExtractor") ? true : false;
		info->isGeo			= (ud->needGeo) ? true : false;

		const WeaponDef* wd	= aicb->GetWeapon(ud->deathExplosion.c_str());
		if(wd!=0)
		{
			float maxLoss	= 0.67 * ud->health;
			float maxDamage	= wd->damages[ud->armorType];
			if(maxLoss >= maxDamage) // the deathexplosion does less damage than we're allowed to lose
			{
				info->spacing = 0;
			}
			else if(wd->edgeEffectiveness > 0.9) // the deathexplosion has almost 100% effect over its radius
			{
				info->spacing = wd->areaOfEffect;
			}
			else // calculate the allowed spacing
			{
				info->spacing = wd->areaOfEffect * (maxDamage - maxLoss) / (maxDamage - maxLoss * wd->edgeEffectiveness);
				info->spacing = std::max(info->spacing, 0.0f);
			}
			info->spacing = std::min(info->spacing, 0.5 * maxPartitionRadius);
		}
		else
		{
			info->spacing = 0;
		}

		info->mp = ud->extractsMetal*avgMetal + ud->metalMake + ud->makesMetal - ud->metalUpkeep;
		info->ep = ud->energyMake - ud->energyUpkeep + ud->tidalGenerator * tidalStrength + std::min(ud->windGenerator,avgWind);
		info->me = info->mp / std::max(ud->energyUpkeep, 1.0f);
		info->em = info->ep / std::max(ud->metalUpkeep, 1.0f);

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
		for (std::map<std::string,BOInfo*>::const_iterator boi=allBO.begin();boi!=allBO.end();++boi)
		{
			BOInfo* info = boi->second;
			if(info->mp > 0) bestMetal.push_back(info);
			if(info->ep > 0) bestEnergy.push_back(info);
		}
		sort(bestMetal.begin(),bestMetal.end(),compareMetal());
		sort(bestEnergy.begin(),bestEnergy.end(),compareEnergy());
	}
}
