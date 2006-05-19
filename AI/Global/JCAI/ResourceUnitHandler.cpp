//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"

#include "BaseAIObjects.h"
#include "CfgParser.h"
#include "TaskManager.h"
#include "ResourceUnitHandler.h"
#include "AI_Config.h"
#include "BuildTable.h"
#include "InfoMap.h"
#include "MetalSpotMap.h"
#include "BuildMap.h"

void RUHandlerConfig::EnergyBuildHeuristicConfig::Load (CfgList *c)
{
	BuildTime = c->GetNumeric ("BuildTime");
	EnergyCost = c->GetNumeric ("EnergyCost");
	MetalCost = c->GetNumeric ("MetalCost");
	MaxUpscale = c->GetNumeric ("MaxUpscale", 1.8f);
}

void RUHandlerConfig::MetalBuildHeuristicConfig::Load (CfgList *c)
{
	PaybackTimeFactor = c->GetNumeric ("PaybackTimeFactor");
	EnergyUsageFactor = c->GetNumeric ("EnergyUsageFactor");
	ThreatConversionFactor = c->GetNumeric ("ThreatConversionFactor");
	PrefUpscale = c->GetNumeric ("PrefUpscale", 2);
	UpscaleOvershootFactor = c->GetNumeric ("UpscaleOvershootFactor", -50);
}

static bool ParseDefList (CfgValue *val, vector<UnitDefID>& v, BuildTable* tbl)
{
	CfgList *l = dynamic_cast <CfgList *> (val);

	if (l) {
		for (list<CfgListElem>::iterator i=l->childs.begin();i!=l->childs.end();++i)
		{
			UnitDefID id = buildTable.GetDefID (i->name.c_str());
			if (id) v.push_back (id);
			else return false;
		}
	} else {
		CfgLiteral *s = dynamic_cast <CfgLiteral *> (val);

		if (s) {
			UnitDefID id = buildTable.GetDefID (s->value.c_str());
			if (id) v.push_back (id);
			else return false;
		}
	}

	return true;
}

bool RUHandlerConfig::Load (CfgList *sidecfg)
{
	CfgList *c = dynamic_cast<CfgList*>(sidecfg->GetValue ("ResourceInfo"));
	BuildTable *tbl = &buildTable;

	if (c)
	{
		EnergyBuildRatio = c->GetNumeric ("EnergyBuildRatio", 1.4);
		MetalBuildRatio = c->GetNumeric ("MetalBuildRatio", 0.8);

		CfgList *ebh = dynamic_cast<CfgList*>(c->GetValue ("EnergyBuildHeuristic"));
		if (ebh) EnergyHeuristic.Load (ebh);

		CfgList *mbh = dynamic_cast<CfgList*>(c->GetValue ("MetalBuildHeuristic"));
		if (mbh) MetalHeuristic.Load (mbh);

		ParseDefList (c->GetValue ("EnergyMakers"), EnergyMakers, tbl);
		ParseDefList (c->GetValue ("MetalMakers"), MetalMakers, tbl);
		ParseDefList (c->GetValue ("MetalExtracters"), MetalExtracters, tbl);

		// Read storage config
		CfgList *st = dynamic_cast<CfgList*>(c->GetValue("Storage"));
		if (st)
		{
			ParseDefList (st->GetValue ("MetalStorage"), StorageConfig.MetalStorage, tbl);
				
			if (StorageConfig.MetalStorage.empty ()) {
				logPrintf ("Error: No metal storage unit type given");
				return false;
			}
			
			ParseDefList (st->GetValue ("EnergyStorage"), StorageConfig.EnergyStorage, tbl);

			if (StorageConfig.EnergyStorage.empty ()) {
				logPrintf ("Error: No energy storage unit type given");
				return false;
			}
		
			StorageConfig.MaxRatio = st->GetNumeric ("MaxRatio", 0.9);
			StorageConfig.MinEnergyIncome = st->GetNumeric ("MinEnergyIncome", 60);
			StorageConfig.MinMetalIncome = st->GetNumeric ("MinMetalIncome", 5);
			StorageConfig.MaxEnergyFactor = st->GetNumeric ("MaxEnergyStorageFactor", 20);
			StorageConfig.MaxMetalFactor = st->GetNumeric ("MaxMetalStorageFactor", 20);
		}

		CfgList *ep = dynamic_cast<CfgList*>(c->GetValue("EnablePolicy"));
		if (ep)
		{
			EnablePolicy.MaxEnergy = ep->GetNumeric ("MaxEnergy");
			EnablePolicy.MinEnergy = ep->GetNumeric ("MinEnergy");
			EnablePolicy.MinUnitEnergyUsage = ep->GetNumeric ("MinUnitEnergyUsage");
		}
	}
	else {
		logPrintf ("Error: No list node named \'ResourceInfo\' was found.\n");
		return false;
	}

	return true;
}

bool ResourceUnitHandler::Task::InitBuildPos (CGlobals *g)
{
	if (def->extractsMetal > 0) {
		while(1) {
			// Find an empty metal spot
			float3 st (g->map->baseCenter.x, 0.0f, g->map->baseCenter.y);
			int id = g->metalmap->FindSpot (st, g->map, def->extractsMetal);
			if(id < 0) {
				// no spots left
				return false;
			}

			MetalSpot *spot = g->metalmap->GetSpot (id);
			if (spot->unit) {
				spotID = id;
				logPrintf ("FindTaskBuildPos: Spot %d has to be cleared first.\n", id);
				return false;
			}

			float3 tpos = float3(spot->pos.x, 0.0f, spot->pos.y) * SQUARE_SIZE;
			if (g->buildmap->FindBuildPosition (def, tpos, pos))
			{
				g->metalmap->SetSpotExtractDepth (id, def->extractsMetal);
				spotID = id;
				logPrintf ("Estimated metal production of extractor: %f\n", def->extractsMetal * spot->metalProduction);
				break;
			}
			logPrintf ("No build space on mex pos %d,%d\n", spot->pos.x,spot->pos.y);
			g->metalmap->SetSpotExtractDepth (id, 100000);// spill the metalspot as it seems it can't be used anyway
			pos.x = -1.0f;
		}

		g->buildmap->Mark (def, pos, true);
		return true;
	}
	else
		return BuildTask::InitBuildPos (g);
}


ResourceUnitHandler::ResourceUnitHandler(CGlobals *g) : TaskFactory (g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load resource config";

	lastEnergy = 0.0f;
	lastUpdate = 0;
}

BuildTask* ResourceUnitHandler::CreateMetalExtractorTask (const UnitDef* def)
{
	return new BuildTask (def);
}

BuildTask* ResourceUnitHandler::CreateStorageTask (UnitDefID id)
{
	const UnitDef* def = buildTable.GetDef (id);

	// See if this storage task is already being made
	for (int a=0;a<activeTasks.size();a++)
	{
		if (dynamic_cast<BuildTask*>(activeTasks[a]) && ((BuildTask*)activeTasks[a])->def == def)
			return 0;
	}

	return new BuildTask (def);
}

BuildTask *ResourceUnitHandler::GetNewBuildTask ()
{
	// task for metal or energy?
	ResourceManager *rm = globals->resourceManager;
	IAICallback *cb = globals->cb;
	InfoMap *imap = globals->map;

	// check if storage has to be build
	if (rm->averageProd.metal > config.StorageConfig.MinMetalIncome && cb->GetMetal () > cb->GetMetalStorage () * config.StorageConfig.MaxRatio &&
		cb->GetMetalStorage() <= rm->averageProd.metal * config.StorageConfig.MaxMetalFactor)
	{
		BuildTask *t = CreateStorageTask (config.StorageConfig.MetalStorage.front());
		if (t) return t;
	}

	if (rm->averageProd.energy > config.StorageConfig.MinEnergyIncome && cb->GetEnergy () > cb->GetEnergyStorage () * config.StorageConfig.MaxRatio &&
		cb->GetEnergyStorage() <= rm->averageProd.energy * config.StorageConfig.MaxEnergyFactor)
	{
		BuildTask *t = CreateStorageTask (config.StorageConfig.EnergyStorage.front());
		if (t) return t;
	}

	if (rm->buildMultiplier.energy * config.MetalBuildRatio < rm->buildMultiplier.metal * config.EnergyBuildRatio) {
		// pick an energy producing unit to build

        // find the unit type that has the highest energyMake/ResourceValue(buildcost) ratio and
		// does not  with MaxResourceUpscale

		int best = -1;
		float bestRatio;
		float minWind = globals->cb->GetMinWind ();
		float maxWind = globals->cb->GetMaxWind ();

		for (int a=0;a<config.EnergyMakers.size();a++)
		{
			BuildTable::UDef*d = buildTable.GetCachedDef (config.EnergyMakers[a]);

			if (rm->averageProd.energy + d->make.energy > config.EnergyHeuristic.MaxUpscale * rm->averageProd.energy)
				continue;

			// calculate costs based on the heuristic parameters
			float cost = 
				d->buildTime * config.EnergyHeuristic.BuildTime +
				d->cost.metal * config.EnergyHeuristic.MetalCost +
				d->cost.energy * config.EnergyHeuristic.EnergyCost;

			float energyProduction = d->make.energy;

			if (d->flags & CUD_WindGen)
				energyProduction += 0.5f * (minWind + maxWind);

			float ratio = energyProduction / cost;

			if (best < 0 || ratio > bestRatio)
			{
				best = config.EnergyMakers[a];
				bestRatio = ratio;
			}
		}

		if (best >= 0)
			return new BuildTask (buildTable.GetDef (best));
	}
	else {
		// pick a metal producing unit to build
		float3 st (globals->map->baseCenter.x, 0.0f, globals->map->baseCenter.y);

		int best=0;
		float bestScore;
		MetalSpot* bestSpot=0;

		// sector has been found, now calculate the best suitable metal extractor
		for (int a=0;a<config.MetalExtracters.size();a++)
		{
			BuildTable::UDef *d = buildTable.GetCachedDef (config.MetalExtracters[a]);
			const UnitDef* def= buildTable.GetDef (config.MetalExtracters[a]);
			MetalSpotID spotID = globals->metalmap->FindSpot(st, imap, def->extractsMetal);
			if (spotID<0) break;

			MetalSpot* spot = globals->metalmap->GetSpot (spotID);
			if (!spot->metalProduction)
			{
				logPrintf ("Metalmap error: No metal production on spot.\n");
				spot->extractDepth=100;
				break;
			}

			// get threat info
			GameInfo *gi = imap->GetGameInfoFromMapSquare (spot->pos.x,spot->pos.y);
			float metalMake = spot->metalProduction * d->metalExtractDepth;
			float PaybackTime = d->cost.metal + gi->threat * config.MetalHeuristic.ThreatConversionFactor;
			PaybackTime /= metalMake;

			float score = config.MetalHeuristic.PaybackTimeFactor * PaybackTime + 
				d->energyUse * config.MetalHeuristic.EnergyUsageFactor;

			float upscale = 1.0f + metalMake / (1.0f + rm->averageProd.metal);
			if (upscale > config.MetalHeuristic.PrefUpscale)
				score += config.MetalHeuristic.UpscaleOvershootFactor * (upscale - config.MetalHeuristic.PrefUpscale);

			logPrintf ("ResourceUnitHandler: %s has score %f with threat %f\n",def->name.c_str(),score,gi->threat);
			if (!best || bestScore < score)
			{
				bestScore = score;
				best = config.MetalExtracters[a];
				bestSpot=spot;
			}
		}

		// compare the best extractor with the metal makers
		for (int a=0;a<config.MetalMakers.size();a++)
		{
			BuildTable::UDef *d = buildTable.GetCachedDef (config.MetalMakers[a]);

			const UnitDef* def = buildTable.GetDef (config.MetalMakers[a]);

			float PaybackTime = d->cost.metal / d->make.metal;
			float score = config.MetalHeuristic.PaybackTimeFactor * PaybackTime + 
				d->energyUse * config.MetalHeuristic.EnergyUsageFactor;

			// calculate upscale and use it in the score
			if (rm->averageProd.metal>0.0f)
			{
				float upscale = 1.0f + d->make.metal / rm->averageProd.metal;

				if (upscale > config.MetalHeuristic.PrefUpscale)
					score += config.MetalHeuristic.UpscaleOvershootFactor * (upscale - config.MetalHeuristic.PrefUpscale);
			}

			logPrintf ("ResourceUnitHandler: %s has score %f\n",def->name.c_str(),score);
			if (!best || bestScore < score)
			{
				bestScore = score;
				best = config.MetalMakers[a];
			}
		}

		if (best)
		{
			const UnitDef *def = buildTable.GetDef (best);
			Task *task = new Task (def);
			return task;
		}
	}

	return 0;
}


aiUnit* ResourceUnitHandler::CreateUnit (int id, BuildTask *task)
{
	IAICallback *cb=globals->cb;
	const UnitDef *d = cb->GetUnitDef (id);
	ResourceUnit *u;

	if (d->extractsMetal > 0.0f) {
		ExtractorUnit *ex = new ExtractorUnit;
		float3 pos = cb->GetUnitPos (id);

		ex->spot = task->spotID;
		globals->metalmap->GetSpot(task->spotID)->unit = ex;
		u=ex;
	}else
		u = new ResourceUnit;

	u->id = id;
	u->def = d;

	return u;
}


void ResourceUnitHandler::UnitFinished (aiUnit *unit)
{
	ResourceUnit* ru = dynamic_cast<ResourceUnit*>(unit);

	assert (ru);
	units.add (ru);
}

void ResourceUnitHandler::UnitDestroyed (aiUnit *unit)
{
	ResourceUnit* ru = dynamic_cast<ResourceUnit*>(unit);
	assert(ru);

	ExtractorUnit *ex = dynamic_cast<ExtractorUnit *>(ru);
	if (ex) globals->metalmap->SetSpotExtractDepth (ex->spot, 0.0f);

	units.del(ru);
}

void ResourceUnitHandler::UnfinishedUnitDestroyed (aiUnit *unit) 
{
	ExtractorUnit *ex = dynamic_cast<ExtractorUnit *>(unit);

	if (ex)
		globals->metalmap->SetSpotExtractDepth (ex->spot, 0.0f);
}

const char* ResourceUnitHandler::GetName ()
{
	return "Resource";
}

void ResourceUnitHandler::Update()
{
	IAICallback *cb=globals->cb;
	int frameNum=cb->GetCurrentFrame();

	if (lastUpdate <= frameNum - 32)
	{
		lastUpdate=frameNum;

		Command c;
		c.params.resize(1);
		c.id=CMD_ONOFF;

		float energy=cb->GetEnergy();
		float estore=cb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;

		//how much energy we need to save to turn positive
		if(energy<estore*config.EnablePolicy.MinEnergy)
		{
			float needed=-dif+5;
			for (int a=0;a<units.size();a++) {
				if(needed<0)
					break;

				ResourceUnit *u = units[a];
				if (u->def->isMetalMaker || u->def->extractsMetal)
				{
					if(u->def->energyUpkeep < config.EnablePolicy.MinUnitEnergyUsage)
						continue;

					if(u->turnedOn)
					{
						needed-=u->def->energyUpkeep;
						c.id=CMD_ONOFF;
						c.params[0] = 0;
						cb->GiveOrder(u->id,&c);
						u->turnedOn=false;
					}
				}
			}
		} 
		else if(energy>estore*config.EnablePolicy.MaxEnergy)
		{
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(int a=0;a<units.size();a++) {
				if(needed<0)
					break;
				ResourceUnit *u=units[a];
				if (u->def->isMetalMaker || u->def->extractsMetal)
				{
					if(u->def->energyUpkeep < config.EnablePolicy.MinUnitEnergyUsage)
						continue;
					if (!u->turnedOn)
					{
						needed-=u->def->energyUpkeep;
						c.params[0]=1;
						cb->GiveOrder(u->id,&c);
						u->turnedOn=true;
					}
				}
			}
		}
	}
}
