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
#include "SupportHandler.h"
#include "InfoMap.h"
#include "BuildTable.h"
#include "BuildMap.h"

//-------------------------------------------------------------------------
// Support Handler Config
//-------------------------------------------------------------------------

struct UnitDefProperty
{
	const char *name;
	float UnitDef::*fptr;
	int UnitDef::*iptr;
}
static UDefPropertyList [] =
{
	{ "radar", 0, &UnitDef::radarRadius },
	{ "los", &UnitDef::losRadius },
	{ "airlos", &UnitDef::airLosRadius },
	{ "control", &UnitDef::controlRadius },
	{ "jammer", 0, &UnitDef::jammerRadius },
	{ "sonar", 0, &UnitDef::sonarRadius },
	{ 0 }
};

void SupportConfig::MapUDefProperties (CfgBuildOptions *c, vector<UnitDefProperty>& props)
{
	props.resize (c->builds.size());

	for (int a=0;a<c->builds.size();a++)
	{
		CfgBuildOptions::BuildOpt *opt = c->builds [a];

		if (opt->info)
		{
			const char *coverprop = opt->info->GetLiteral ("coverproperty");
			if (coverprop)
			{
				for (int b=0;UDefPropertyList[b].name;a++) 
					if (!STRCASECMP (UDefPropertyList[b].name, coverprop))
					{
						props[a].isFloat = UDefPropertyList[b].fptr != 0;
						if (props[a].isFloat) props[a].fptr = UDefPropertyList[b].fptr;
						else props[a].iptr = UDefPropertyList[b].iptr;
						break;
					}
			}
		}
	}
}

bool SupportConfig::Load (CfgList *sidecfg)
{
	CfgList *scfg = dynamic_cast <CfgList*> (sidecfg->GetValue ("supportinfo"));

	if (scfg)
	{
		basecover = dynamic_cast<CfgBuildOptions*> (scfg->GetValue ("basecover"));
		mapcover = dynamic_cast<CfgBuildOptions*> (scfg->GetValue ("mapcover"));

		if (basecover) 
		{
			MapUDefProperties (basecover, basecoverProps);

			if (!basecover->InitIDs ())
				return false;
		}
		if (mapcover)
		{
			MapUDefProperties (mapcover, mapcoverProps);
			
			if (!mapcover->InitIDs ())
				return false;
		}

		CfgList *gl = dynamic_cast <CfgList*> (scfg->GetValue ("Groups"));
		if (gl)
		{
			for (list<CfgListElem>::iterator i=gl->childs.begin();i!=gl->childs.end();++i)
			{
				groups.push_back(Group());
				Group& gr(groups.back());
				gr.name = i->name;
				CfgList *groupInfo = dynamic_cast<CfgList*>(i->value);
				if (groupInfo) {
					gr.minEnergy = groupInfo->GetNumeric ("minenergy");
					gr.minMetal = groupInfo->GetNumeric ("minmetal");
					gr.units = dynamic_cast<CfgBuildOptions*>(groupInfo->GetValue("units"));
					if (gr.units) gr.units->InitIDs();
				}
			}
		}
	}

	return true;
}


//-------------------------------------------------------------------------
// Support Handler Unit Groups
//-------------------------------------------------------------------------

SupportHandler::UnitGroup::UnitGroup(SupportConfig::Group *Cfg) : cfg(Cfg)
{
	if (cfg->units)
		orderedUnits.resize(cfg->units->builds.size(), 0);
	sector.x=-1;
}

BuildTask* SupportHandler::UnitGroup::NewTask ()
{
	int counts[50];
	int a,total=0;

	if (!cfg || !cfg->units || cfg->units->builds.empty())
		return 0;

	for (a=0;a<cfg->units->builds.size ();a++)
	{
		int def = cfg->units->builds [a]->def;
		counts[a]=0;
		for (ptrvec<Unit>::iterator f=units.begin();f!=units.end();++f)
			if( f->def->id == def ) counts[a] ++;
		counts[a] += orderedUnits[a];
		total += counts[a];
	}

	int totalBuilds=cfg->units->TotalBuilds();
	for (a=0;a<cfg->units->builds.size();a++)
	{
		if (counts[a]*totalBuilds <= units.size())
			break;
	}

	// indecisive, so just pick the first
	if (a==cfg->units->builds.size())
		a=0;

	orderedUnits[a]++;
	Task *t = new Task ( buildTable.GetDef(cfg->units->builds[a]->def) );
	t->unitIndex = a;
	t->group = this;
	return t;
}

int SupportHandler::UnitGroup::CountOrderedUnits()
{
	int c=0;
	for (int b=0;b<cfg->units->builds.size();b++)
		c+=orderedUnits [b];
	return c;
}

//-------------------------------------------------------------------------
// Support Handler
//-------------------------------------------------------------------------

int SupportHandler::Task::GetBuildSpace(CGlobals *g)
{
	return BLD_TOTALBUILDSPACE;
}

bool SupportHandler::Task::InitBuildPos (CGlobals *g)
{
	if (group->sector.x < 0)
		group->sector = g->map->FindDefenseBuildingSector (float3(g->map->baseCenter.x,0.0f,g->map->baseCenter.y), 20);

	if (group->sector.x < 0)
		return false;

	// find a position for the unit
	float3 tpos = float3(group->sector.x+0.5f,0.0f,group->sector.y+0.5f)*g->map->mblocksize;
	if(!g->buildmap->FindBuildPosition (def, tpos, pos))
		return false;

	return true;
}

SupportHandler::SupportHandler (CGlobals *g) : TaskFactory (g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load support handler config";

	for (list<SupportConfig::Group>::iterator i=config.groups.begin();i!=config.groups.end();++i) {
		if(i->units->TotalBuilds())
			buildorder.push_back (&*i);
	}
}

void SupportHandler::Update ()
{}

void SupportHandler::UnitDestroyed (aiUnit *unit)
{
	UnitGroup::Unit* u = dynamic_cast<UnitGroup::Unit*>(unit);

	if (!u->group->CountOrderedUnits() && u->group->units.empty()) {
		groups.del(u->group);
		return;
	}

	u->group->units.del(u);
}

void SupportHandler::UnitFinished (aiUnit *unit)
{
	UnitGroup::Unit *u = dynamic_cast<UnitGroup::Unit*>(unit);
	assert (u);

	Task *t = u->task;

	t->group->units.add(u);
	t->group->orderedUnits[t->unitIndex]--;
	u->task=0;
}

aiUnit* SupportHandler::CreateUnit (int id, BuildTask *task)
{
	UnitGroup::Unit *u = new UnitGroup::Unit ();

	u->id = id;
	u->def = task->def;
	u->task = (Task*)task;
	u->group = u->task->group;

	return u;
}


BuildTask* SupportHandler::GetNewBuildTask ()
{
	// find an uncompleted group
	ResourceInfo &prod = globals->resourceManager->averageProd;

	for (int a=0;a<groups.size();a++)
	{
		SupportConfig::Group *grcfg = groups[a]->cfg;

		if (prod.energy >= grcfg->minEnergy && prod.metal >= grcfg->minMetal) {
			if (grcfg->units->TotalBuilds() > groups[a]->units.size() + groups[a]->CountOrderedUnits()) 
				return groups[a]->NewTask ();
		}
	}

	BuildTask *t = 0;
	if (!buildorder.empty()) {
		SupportConfig::Group *first = buildorder.front();
		do {
			if (prod.energy >= buildorder.front()->minEnergy && prod.metal >= buildorder.front()->minMetal)
				t=groups.add(new UnitGroup(buildorder.front()))->NewTask();
			buildorder.push_back (buildorder.front());
			buildorder.pop_front ();
		} while (!t && buildorder.front () != first);
	}

	return t;
}

