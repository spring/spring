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
#include "ForceHandler.h"

#ifdef _MSC_VER
#pragma warning(disable: 4018) // signed/unsigned conflicts blahblah......
#endif

void ForceConfig::Precalc ()
{
	for (GroupIterator i=groups.begin();i!=groups.end();++i)
		catmap [i->category].push_back (&*i);
}


ForceUnit::~ForceUnit()
{}

// ----------------------------------------------------------------------------------------
// Unit Batch
// ----------------------------------------------------------------------------------------

UnitGroup::UnitGroup (CGlobals *g) : aiHandler(g)
{
	goal.x=0;
	state=ugroup_Building;
	current.x=-1;
	curTarget=-1;
	group=0;
	orderedUnits=0;
}

UnitGroup::~UnitGroup()
{
	// TODO: Unhackify this ;)
	units.vec.clear();

	if (orderedUnits)
		delete[] orderedUnits;
}

void UnitGroup::UnitIdle (aiUnit *unit)
{
	unit->flags |= UNIT_IDLE;
}

const char *UnitGroup::GetName ()
{
	return group->name.c_str();
}

void UnitGroup::DependentDied (aiObject *obj)
{
	ForceUnit *fu = (ForceUnit *)obj;

	logPrintf ("UnitGroup: DependentDied(): FU ID=%d, index=d\n", fu->id, fu->index);

	/*assert (units[fu->index] == fu); */
	units.erase (fu);
}


aiUnit* UnitGroup::CreateUnit (int id,BuildTask *task)
{
	ForceUnit* fu = new ForceUnit;
	logPrintf ("ForceUnit created: ID=%d\n", id);
	return fu;
}


void UnitGroup::UnitFinished (aiUnit *unit)
{
	ForceUnit *fu = dynamic_cast<ForceUnit*>(unit);
	assert (fu);

	AddDeathDependence (fu);
	units.add (fu);

	CfgBuildOptions *bopt = group->units;
	int a = 0;
	for (;a<bopt->builds.size();a++)
		if (unit->def->id == bopt->builds [a]->def) {
			orderedUnits [a] --;
			break;
		}

	assert (a < bopt->builds.size());

	logPrintf ("UnitGroup: Added %s - batch has %d units.\n", unit->def->name.c_str(),units.size());
}

void UnitGroup::UnitDestroyed (aiUnit *unit)
{
	ForceUnit *fu = dynamic_cast<ForceUnit*>(unit);
	assert (fu);

	DeleteDeathDependence (fu);
	units.erase (fu);
    delete fu;
}

void UnitGroup::SetMoving ()
{
	state = ugroup_Moving;

	if (goal.x >= 0)
	{
		// go to next sector
		Command c;
		c.id = CMD_MOVE;
		c.params.push_back (globals->map->gblocksize * (goal.x + 0.5f));
		c.params.push_back (0.0f);
		c.params.push_back (globals->map->gblocksize * (goal.y + 0.5f));

		GiveOrder (&c);
	}
	else
	{
		Command c;
		c.id = CMD_STOP;
		GiveOrder (&c);
	}
}

void UnitGroup::SetPruning ()
{
	GameInfo *gi = globals->map->GetGameInfo (current);
    curTarget = SelectTarget (gi->enemies);

	if (curTarget >= 0)
	{
		// attack 
		Command c;

		c.id = CMD_ATTACK;
		c.params.push_back (curTarget);

		GiveOrder (&c);
	}

	state = ugroup_Pruning;
}

int UnitGroup::SelectTarget (const vector <int>& enemies)
{
	// select the most powerful unit
	int best=-1;
	float bestscore=0.0f;

	for (vector<int>::const_iterator i=enemies.begin();i!=enemies.end();++i)
	{
		float s = globals->cb->GetUnitPower (*i);

		if (best < 0 || s > bestscore)
		{
			bestscore = s;
			best = *i;
		}
	}
	
	return best;
}

void UnitGroup::SetGrouping ()
{
	state=ugroup_Grouping;

	Command c;
	c.id = CMD_MOVE;
	c.params.push_back (mid.x);
	c.params.push_back (0.0f);
	c.params.push_back (mid.y);

	GiveOrder (&c);
}

void UnitGroup::SetWaitingForGoal ()
{
	state = ugroup_WaitingForGoal;

	Command c;
	c.id = CMD_STOP;
	GiveOrder (&c);
}

void UnitGroup::Init ()
{
	state = ugroup_Moving;
	goal.x = -1;
}

bool UnitGroup::IsGoalReached(const float2& mid, float spread)
{
	int2 s = globals->map->GetGameInfoCoords (float3(mid.x,0.0f,mid.y));

	return s.x == goal.x && s.y == goal.y;
}

void UnitGroup::Update ()
{
	if (state == ugroup_Building || units.empty ())
		return;

	int2 sector, dif;
	float2 pmin, pmax;
	if (!CalcPositioning (pmin,pmax,mid,sector))
		return;

	if (current.x < 0)
		current = globals->map->GetGameInfoCoords (float3 (mid.x,0.0f,mid.y));

	// change current?
	if (sector.x >= 0) {
		dif.x=sector.x-current.x; dif.y=sector.y-current.y;
		if (dif.x*dif.x+dif.y*dif.y > 2)
			current = sector;
	}

	GameInfo *b = globals->map->GetGameInfo (goal);

	// calc spreading
	float difx=pmax.x-pmin.x, dify=pmax.y-pmin.y;
	float spread = sqrtf (difx*difx+dify*dify);

	GameInfo *cur = 0;
	cur = globals->map->GetGameInfo (current);

	switch (state)
	{
	case ugroup_Grouping:
		if (spread < group->groupdist)
			SetMoving ();
		break;

	case ugroup_Pruning: {
		// is current target still in sector?
		if (find (cur->enemies.begin(),cur->enemies.end(), curTarget) == cur->enemies.end ())
		{
			// no, so find a new target
			if (cur->enemies.empty ()) // move on
			{
				SetNewGoal();
				SetMoving ();
			}
			else if (lastCmdFrame < globals->cb->GetCurrentFrame () - 30)
				SetPruning ();
		}
		break;}

	case ugroup_Moving:
		if (!cur->enemies.empty ())
			SetPruning ();
		else
		{
			if (goal.x < 0 || IsGoalReached (mid,spread))
				SetNewGoal ();

			if (spread > group->maxspread)
				SetGrouping ();
		}
		break;

	case ugroup_WaitingForGoal:
		SetNewGoal ();
		break;
	}
}

void UnitGroup::GiveOrder (Command *c)
{
/*	for (int a=0;a<units.size();a++) {
		units [a]->flags &= ~UNIT_IDLE;
		globals->cb->GiveOrder (units[a]->id, c);
	}*/

	for (ptrvec<ForceUnit>::iterator i = units.begin();i != units.end(); ++i)
		globals->cb->GiveOrder ( i->id, c );

	lastCmdFrame = globals->cb->GetCurrentFrame();
}

void UnitGroup::SetNewGoal ()
{
	if (group->task == ft_Raid || group->task == ft_AttackDefend)
	{
		goal = globals->map->FindAttackDefendSector (globals->cb,globals->map->GetGameInfoCoords (float3(mid.x,0.0f,mid.y)));
	}

	if (goal.x < 0)
	{
		if (state != ugroup_WaitingForGoal)
			SetWaitingForGoal ();
	}
	else {
		//logPrintf ("New goal set: %d,%d\n", goal.x, goal.y);
		SetMoving ();
	}
}

bool UnitGroup::CalcPositioning (float2& pmin, float2& pmax, float2& mid, int2& cs)
{
	const float l=10000000;
	float2 sum= {0.0f,0.0f};
	pmin.x=pmin.y=l;pmax.x=pmax.y=-l;

	int bestdis=10000000;
	cs.x=-1;

	//for (int a=0;a<units.size();a++)
	for (ptrvec<ForceUnit>::iterator i=units.begin();i!=units.end();++i)
	{
		float3 p = globals->cb->GetUnitPos ( i->id );
		if (p.x < pmin.x) pmin.x = p.x;
		if (p.z < pmin.y) pmin.y = p.z;
		if (p.x > pmax.x) pmax.x = p.x;
		if (p.z > pmax.y) pmax.y = p.z;
		sum.x += p.x;
		sum.y += p.z;

		if (goal.x < 0)
			continue;

		// select the sector closest to the goal
		int2 cur = globals->map->GetGameInfoCoords(p), dif;
		dif.x = goal.x-cur.x; dif.y = goal.x-cur.y;
		// squared distance
		int dis = dif.x*dif.x+dif.y*dif.y;
		if (cs.x < 0 || bestdis > dis)
		{
			bestdis=dis;
			cs=cur;
		}
	}

	if (!units.empty()) {
		mid.x = sum.x/units.size();
		mid.y = sum.y/units.size();
		return true;
	}
	return false;
}

int UnitGroup::FindPreferredUnit ()
{
	static int counts[50];
	int a,total=0;

	CfgBuildOptions *unitl = group->units;

	if (aiConfig.debug)
		logPrintf ("FindPreferredUnit on group %s\n", group->name.c_str());

	assert (unitl->builds.size()<=50);

	for (a=0;a<unitl->builds.size ();a++)
	{
		const UnitDef *def = buildTable.GetDef (unitl->builds [a]->def);
		counts[a]=0;
		for (ptrvec<ForceUnit>::iterator f=units.begin();f!=units.end();++f)
			if( f->def == def ) counts[a] ++;
		counts[a] += orderedUnits [a];
		total += counts[a];
	}

	int totalBuilds=unitl->TotalBuilds();
	int best = -1, bestScore = 0;

	for (a=0;a<unitl->builds.size();a++)
	{
		int score = units.size()-counts[a]*totalBuilds;

		if (best < 0 || bestScore < score)
		{
			best = a;
			bestScore = score;
		}
	}

	return best;
}

int UnitGroup::CountCurrentOrders ()
{
	int c = 0;

	if (!orderedUnits)
		return 0;

	for (int a=0;a<group->units->builds.size();a++)
		c += orderedUnits [a];
	
	return c;
}

// ----------------------------------------------------------------------------------------
// Force co�dinator
// ----------------------------------------------------------------------------------------

ForceHandler::ForceHandler (CGlobals *g) : TaskFactory(g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load force info";

	config.Precalc ();

	build.resize (config.catmap.size());

	int a = 0;
	for (ForceConfig::CatMapIterator i = config.catmap.begin(); i != config.catmap.end(); ++i)
	{
		build [a].batch = 0;
		build [a].cat = i->first;
		a ++;
	}
}


ForceHandler::~ForceHandler()
{
	batches.vec.clear(); // UnitGroups are handlers, so they get deleted by the MainAI class
}

typedef map<int, vector <ForceConfig::Group*> > CategoryMap;


void ForceHandler::UpdateNextCategoryTask (int c)
{
	BuildState& state = build [c];

	if (!state.batch)
		return;

	ForceConfig::Group *gtype = state.batch->group;
	
	state.newUnitTypeIndex = state.batch->FindPreferredUnit ();
	if (state.newUnitTypeIndex >= 0) {
		state.newDef = buildTable.GetDef (gtype->units->builds [state.newUnitTypeIndex]->def);
		state.waitFrame = globals->cb->GetCurrentFrame ();
	}
}

BuildTask* ForceHandler::GetNewBuildTask ()
{
	int best=-1;
	int bestwait;

	for (int a=0;a<build.size();a++)
	{
		if (!build[a].batch || !build[a].newDef)
			continue;

		int d = globals->cb->GetCurrentFrame () - build[a].waitFrame;

		if (best < 0 || d > bestwait)
		{
			best=a;
			bestwait=d;
		}
	}

	if (best >= 0)
	{
		Task*nt = new Task(build[best].newDef, build[best].batch);
		build[best].newDef=0;
		build[best].batch->orderedUnits [build[best].newUnitTypeIndex] ++;
		return nt;
	}

	return false;
}

void ForceHandler::Update ()
{
	float metal = globals->cb->GetMetalIncome ();
	float energy = globals->cb->GetEnergyIncome ();

	for (int a=0;a<build.size();a++)
	{
		BuildState *s = &build [a];

		if (!s->batch)
		{
			// select a new group to build
			CategoryMap::iterator i = config.catmap.find (s->cat);
			vector <ForceConfig::Group *>& fgl = i->second;

			int bestlevel, best=-1;
			for (int a=0;a<fgl.size();a++) 
			{
				ForceConfig::Group *gr = fgl [a];

				if (gr->minmetal <= metal && gr->minenergy <= energy)
					if (best < 0 || bestlevel < gr->level) 
					{
						best = a;
						bestlevel = gr->level;
					}
			}

			if (best >= 0)
			{
				UnitGroup *gr = s->batch = new UnitGroup (globals);
				gr->group = fgl [best];
				gr->orderedUnits = new int [gr->group->units->builds.size()];
				fill (gr->orderedUnits, gr->orderedUnits + gr->group->units->builds.size(), 0);

				if (aiConfig.debug)
					logPrintf ("Building new batch: %s\n", gr->group->name.c_str());

				globals->handlers.insert (gr);

				s->waitFrame = globals->cb->GetCurrentFrame ();
				s->newDef = 0;
			}
		}
		else
		{
			if (s->batch->units.size() >= s->batch->group->batchsize)
			{
				if (aiConfig.debug)
					logPrintf ("ForceHandler: Batch build completed: (%s)\n", s->batch->GetName ());
				s->batch->SetNewGoal ();

				delete[] s->batch->orderedUnits;
				s->batch->orderedUnits = 0;

				batches.add (s->batch);
				s->batch = 0;
			}

			else if (!s->newDef && (s->batch->CountCurrentOrders () + s->batch->units.size() < s->batch->group->batchsize))
				UpdateNextCategoryTask (a);
		}
	}

	ClearEmptyBatches ();
}


void ForceHandler::ClearEmptyBatches()
{
	int i = 0;
	while (i < batches.size())
	{
		if (batches[i]->units.empty ())
		{
			globals->handlers.erase(batches[i]);
			batches.del (batches[i]);
			continue;
		}
		i++;
	}
}

const char *groupstatestr[] =
{
	"Building", // group is incomplete - building
	"Moving",   // moving to goal
	"Pruning",  // clear units in current sector
	"Grouping",
	"WaitingForGoal"
};


void ForceHandler::ShowGroupStates ()
{
	for (int a=0;a<batches.size();a++)  {
		UnitGroup *gr = batches[a];
		ChatMsgPrintf (globals->cb, "Group %s is in state: %s on(%d,%d)", gr->GetName(), groupstatestr[(int)gr->state], gr->current.x,gr->current.y);
	}
}


// ----------------------------------------------------------------------------------------
// Force system config
// ----------------------------------------------------------------------------------------

ForceConfig::ForceConfig ()
{}

struct ForceTaskName
{
	const char *name;
	ForceGroupTask task;
}
static forcetasktbl [] =
{ 
	{ "raid",ft_Raid},
	//{ "attackreturn", ft_AttackReturn},
	{ "attackdefend", ft_AttackDefend},
	{ "defend", ft_Defend},
	{ 0 }
};


bool ForceConfig::ParseForceGroup (CfgList *cfg, const string& name)
{	
	groups.push_front (Group());
	Group& g = groups.front ();
	g.name = name;

	CfgBuildOptions *units = dynamic_cast <CfgBuildOptions*> (cfg->GetValue ("units"));

	if (units) {
		if (!units->InitIDs ()) {
			groups.pop_front();
			return false;
		}
	}
	else
	{
		logPrintf ("Error: Force group %s has no units\n", name.c_str());
		groups.pop_front();
		return false;
	}

	g.units = units;
	g.name = name;
	g.category = cfg->GetInt ("category", 0);
	g.level = cfg->GetInt ("level", 0);
	g.batchsize = cfg->GetInt ("batchsize", 1);
	g.minenergy = cfg->GetNumeric ("minenergy", -1.0f);
	g.minmetal = cfg->GetNumeric ("minmetal",-1.0f);
	g.maxspread = cfg->GetNumeric ("maxspread",defaultSpread);
	g.groupdist = cfg->GetNumeric ("groupdist",g.maxspread * 0.5f);

	if (g.minenergy == -1.0f && g.minmetal == -1.0f)
		logPrintf ("Warning: minmetal or minenergy not set for force group %s\n", name.c_str());

	if (g.minenergy < 0.0f) g.minenergy = 0.0f;
	if (g.minmetal < 0.0f) g.minmetal = 0.0f;

	const char *str = cfg->GetLiteral ("task", "attackdefend");
	for (int a=0;forcetasktbl [a].name; a++)
		if (!STRCASECMP (forcetasktbl [a].name, str)) 
		{
			g.task=forcetasktbl[a].task;
			break; 
		}

	logPrintf ("Parsed force group: %s\n", name.c_str());
	return true;
}

bool ForceConfig::Load (CfgList *cfg)
{
	CfgList *forceinfo = dynamic_cast<CfgList*> (cfg->GetValue ("forceinfo"));

	if (forceinfo)
	{
		defaultSpread = forceinfo->GetNumeric ("defaultspread", 500);

		for (list<CfgListElem>::iterator gr = forceinfo->childs.begin(); gr != forceinfo->childs.end(); ++gr)
		{
			if (dynamic_cast <CfgList*> (gr->value))
				ParseForceGroup ( (CfgList *)gr->value, gr->name );
		}
		return true;
	}
	
	logPrintf ("Error: No force info in config.\n");
	return false;
}

