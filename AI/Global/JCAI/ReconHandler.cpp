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
#include "AI_Config.h"
#include "TaskManager.h"
#include "InfoMap.h"
#include "BuildTable.h"
#include "ReconHandler.h"

using namespace boost;


// ----------------------------------------------------------------------------------------
// Recon config
// ----------------------------------------------------------------------------------------

bool ReconConfig::Load (CfgList *rootcfg)
{
	CfgList *rinfo = dynamic_cast<CfgList*> (rootcfg->GetValue ("reconinfo"));

	if (rinfo)
	{
		scouts = dynamic_cast<CfgBuildOptions*> (rinfo->GetValue ("scouts"));
		if (scouts)
		{
			scouts->InitIDs ();
		}

		updateInterval = rinfo->GetInt ("updateinterval", 4);
		maxForce = rinfo->GetInt ("maxforce", 5);
		minMetal = rinfo->GetNumeric ("minmetal");
		minEnergy = rinfo->GetNumeric ("minenergy");

		CfgList* hcfg = dynamic_cast<CfgList*> (rinfo->GetValue ("SearchHeuristic"));
		if (hcfg)
		{
			searchHeuristic.DistanceFactor = hcfg->GetNumeric ("DistanceFactor");
			searchHeuristic.ThreatFactor = hcfg->GetNumeric ("ThreatFactor");
			searchHeuristic.TimeFactor = hcfg->GetNumeric ("TimeFactor");
			searchHeuristic.SpreadFactor = hcfg->GetNumeric ("SpreadFactor");
		}
		else {
			logPrintf ("Error: No search heuristic info in recon config node.\n");
			return false;
		}

		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------------
// Recon handling
// ----------------------------------------------------------------------------------------

ReconHandler::ReconHandler (CGlobals *g) : TaskFactory (g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load recon info block";
}

ReconHandler::~ReconHandler()
{
	units.vec.clear();
}

ReconHandler::Unit::~Unit()
{
}

float inline SectorScore (int x,int y, const GameInfo& i,const int2& sp,ReconHandler* h, int frame, ReconHandler::Unit* unit)
{
	ScoutSectorHeuristic& cfg = h->config.searchHeuristic;

	float score = (frame - i.lastLosFrame) * cfg.TimeFactor; // older is better 
	score += i.losThreat * cfg.ThreatFactor; // higher threat is worse
	InfoMap *imap = h->globals->map;

	if (cfg.SpreadFactor!=0.0f)
	{
		int dis2 = 1<<30;
		for (int a=0;a<h->units.size();a++)
		{
			ReconHandler::Unit *u = h->units[a];
			if(u==unit || u->goal.x<0) continue;
			int dx = int(u->goal.x - x*imap->gblocksize) / 128;
			int dy = int(u->goal.y - y*imap->gblocksize) / 128;
			dx = dx*dx+dy*dy;
			if (dx < dis2) dis2 = dx;
		}
		dis2+=1;
		score -= 1.0f / sqrtf (dis2) * cfg.SpreadFactor;
	}

	int dx=x-sp.x,dy=y-sp.y;
	dx=dx*dx+dy*dy;
	return score + sqrtf (dx)*cfg.DistanceFactor; // longer distance is worse
}

void ReconHandler::Update ()
{
	if(globals->cb->GetCurrentFrame() % config.updateInterval != 1)
		return;

	// give the new order
	Command c;
	c.id = CMD_MOVE;

	c.params.push_back (0.0f);
	c.params.push_back (0.0f);
	c.params.push_back (0.0f);

	for (ptrvec<Unit>::iterator i=units.begin();i!=units.end();++i) {
		int2 pgoal=i->goal;

		// use the forces to scout sectors which have not been 
		float3 pos = globals->cb->GetUnitPos (i->id);
		// convert SectorScore(x,y,i,sp,frame) to HF(x,y,i) and use it in SelectGameInfoSector
		int2 ngoal = globals->map->SelectGameInfoSector (bind (SectorScore, _1,_2,_3,
				globals->map->GetGameInfoCoords(pos), this, globals->cb->GetCurrentFrame(),&*i));
		if (ngoal.x >= 0)
		{
			i->goal.x = (int) (ngoal.x + 0.5f) * globals->map->gblocksize;
			i->goal.y = (int) (ngoal.y + 0.5f) * globals->map->gblocksize;
		}

		if (pgoal.x!=i->goal.x || pgoal.y!=i->goal.y)
		{
			c.params [0] = i->goal.x;
			c.params [2] = i->goal.y;

			globals->cb->GiveOrder (i->id, &c);
		}
	}
}

void ReconHandler::UnitDestroyed (aiUnit *unit)
{
	Unit* u = dynamic_cast <Unit*> (unit);
	assert(u);

	units.del (u);
}

void ReconHandler::UnitFinished (aiUnit *unit)
{
	Unit* u = dynamic_cast <Unit*> (unit);
	assert(u);

	units.add (u);
}

aiUnit* ReconHandler::CreateUnit (int id,BuildTask *task)
{
	aiUnit *u = new Unit;
	u->owner = this;
	return u;
}

BuildTask* ReconHandler::GetNewBuildTask()
{
	int counts[50];
	int a,total=0;

	if (!config.scouts || config.scouts->builds.empty () || units.size () >= config.maxForce ||
		config.minEnergy > globals->resourceManager->averageProd.energy || config.minMetal > globals->resourceManager->averageProd.metal)
		return 0;

	for (a=0;a<config.scouts->builds.size ();a++)
	{
		int def = config.scouts->builds [a]->def;
		counts[a]=0;
		for (ptrvec<Unit>::iterator f=units.begin();f!=units.end();++f)
			if( f->def->id == def ) counts[a] ++;
		total += counts[a];
	}

	int totalBuilds=config.scouts->TotalBuilds();
	for (a=0;a<config.scouts->builds.size();a++)
	{
		if (counts[a]*totalBuilds <= units.size())
			break;
	}

	// indecisive, so just pick the first
	if (a==config.scouts->builds.size())
		a=0;

	return new BuildTask (buildTable.GetDef (config.scouts->builds [a]->def));
}
