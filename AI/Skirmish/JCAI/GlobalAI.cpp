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
#include "GlobalAI.h"
#include "ExternalAI/IAICheats.h"

#include "AI_Config.h"
#include "TaskManager.h"
// TaskHandlers!
#include "ForceHandler.h"
#include "ResourceUnitHandler.h"
#include "SupportHandler.h"
#include "ReconHandler.h"

//#include "Sim/Units/UnitHandler.h" // for MAX_UNITS

#include "DebugWindow.h"

#include <stdarg.h>
#include <time.h> 
#ifdef _MSC_VER
#include <windows.h> // for OutputDebugString
#endif

CfgList* rootcfg = 0;

// ID buffer to use for all callback calls that require a buffer
int idBuffer [IDBUF_SIZE];

// ----------------------------------------------------------------------------------------
// Config loading through the spring FS
// ----------------------------------------------------------------------------------------

CfgList *LoadConfigFromFS (IAICallback *cb, const char *file)
{
	CfgBuffer buf;

	buf.len = cb->GetFileSize (file);
	if (buf.len < 0) {
		ChatMsgPrintf (cb, "Failed to load \"%s\"", file);
		return 0;
	}

	buf.data = new char [buf.len];
	cb->ReadFile (file, buf.data, buf.len);

	buf.filename = file;

	CfgList *list = new CfgList;
	if (!list->Parse (buf,true))
	{
		delete list;
		delete[] buf.data;
		return 0;
	}
	delete[] buf.data;

	return list;
}

// ----------------------------------------------------------------------------------------
// Logfile output
// ----------------------------------------------------------------------------------------

const char *LogFileName = AI_PATH "testai.log";

void logFileOpen ()
{
	remove (LogFileName);

	time_t tval;
	char buf[128];

	/* Get current date and time */
	tval = time(NULL);
	tm* now = localtime(&tval);
	if (now) {
		strftime(buf,sizeof(buf),"Log started on: %A, %B %d, day %j of %Y.\nThe time is %I:%M %p.\n",now);

		logPrintf (buf);
	}
}

void logFileClose ()
{}

void logPrintf (const char *fmt, ...)
{
	static char buf[256];
	va_list vl;
	va_start (vl, fmt);
	VSNPRINTF (buf, sizeof(buf), fmt, vl);

	FILE *file = fopen (LogFileName, "a");
	if (file)
	{
		fputs (buf, file);
		fclose (file);
	}

#ifdef _MSC_VER
	OutputDebugString (buf);
#endif
	va_end (vl);
}



void DebugPrintf (IAICallback *cb, const char *fmt, ...)
{
	static char buf[256];
	
	if (!aiConfig.debug)
		return;

	va_list vl;
	va_start (vl, fmt);
	VSNPRINTF (buf, sizeof(buf), fmt, vl);
	va_end (vl);

	logPrintf ("%s\n", buf);
}


void ChatDebugPrintf (IAICallback *cb, const char *fmt, ...)
{
	static char buf[256];

	va_list vl;
	va_start (vl, fmt);
	VSNPRINTF (buf, sizeof(buf), fmt, vl);
	va_end (vl);

	logPrintf ("%s\n", buf);

	if (aiConfig.debug)
		cb->SendTextMsg (buf,  0);
}



void ChatMsgPrintf (IAICallback *cb, const char *fmt, ...)
{
	static char buf[256];
	va_list vl;
	va_start (vl, fmt);
	VSNPRINTF (buf, sizeof(buf), fmt, vl);
	va_end (vl);

	logPrintf ("%s\n", buf);

	cb->SendTextMsg (buf,  0);
}

// ----------------------------------------------------------------------------------------
// aiUnit
// ----------------------------------------------------------------------------------------

void aiUnit::DependentDied (aiObject *) {}


// ----------------------------------------------------------------------------------------
// MainAI - Static functions
// ----------------------------------------------------------------------------------------


static bool isCommonDataLoaded = false;

// Loads AI data not depending on side or units
bool MainAI::LoadCommonData(IGlobalAICallback *cb)
{
	if (isCommonDataLoaded)
		return true;

	if (!aiConfig.Load (AI_PATH "settings.cfg",cb))
		return false;

	IAICallback *aicb = cb->GetAICallback();
	ChatMsgPrintf (aicb, "Running mod %s on map %s%s",  aicb->GetModName (),aicb->GetMapName (), 
#ifdef _DEBUG
		" - debug build"
#else
		""
#endif
		);

	buildTable.Init (aicb, aiConfig.cacheBuildTable);

	if (!modConfig.Load (cb))
		return false;

	GetSearchOffsetTable();
	isCommonDataLoaded = true;
	return true;
}

void MainAI::FreeCommonData()
{
	FreeSearchOffsetTable();
}

// ----------------------------------------------------------------------------------------
// MainAI
// ----------------------------------------------------------------------------------------

MainAI::MainAI(int i) 
{
	cb = 0;
	sidecfg = 0;

	cfgLoaded=false;
	skip=false;
	aiIndex=i;
	dbgWindow=0;
	unitDefs=0;
	resourceManager = 0;

	for (int a=0;a<NUM_TASK_TYPES;a++) 
		globals.taskFactories[a]=0;
	
	logFileOpen ();
}

MainAI::~MainAI()
{
	logPrintf ("Closing AI...\n");

	for (HandlerList::iterator a=globals.handlers.begin ();a!=globals.handlers.end();++a)
		delete *a;

	if (dbgWindow) {
		DbgDestroyWindow (dbgWindow);
		dbgWindow=0;
	}

	logFileClose ();

	if (unitDefs) {
		delete[] unitDefs;
		unitDefs=0;
	}
}


void MainAI::GotChatMsg (const char *msg, int player)
{
	if (msg[0] != '.')
		return;

	if (!STRCASECMP(".skipai", msg)) {
		skip=true;
		cb->SendTextMsg ("Skip enabled",0);
	} else if (!STRCASECMP(".unskipai", msg)) {
		skip=false;
		cb->SendTextMsg ("Skip disabled",0);
	} else if(!STRCASECMP(".inactives", msg)) {
		TaskManager *tm = globals.taskManager;
		for (int a=0;a<tm->builders.size();a++)  {
			BuildUnit *b = tm->builders [a];
			if (b->tasks.empty())
				ChatMsgPrintf (cb, "Inactive: %s", b->def->name.c_str());
		}
	}
	else if(!STRCASECMP(".writethreatmap", msg))
		map.WriteThreatMap ("threatmap.tga");
	else if(!STRCASECMP(".writebuildmap", msg))
		globals.buildmap->WriteTGA ("buildmap.tga");
	else if (!STRCASECMP(".debugwindow", msg))
	{
		if (dbgWindow)
			DbgDestroyWindow(dbgWindow);
		InitDebugWindow();
	}
	else if (!STRCASECMP(".debugai", msg))
		aiConfig.debug = !aiConfig.debug;

	for (HandlerList::iterator i=globals.handlers.begin();i!=globals.handlers.end();++i)
		(*i)->ChatMsg (msg,player);
}

void MainAI::InitDebugWindow()
{
	char name[64];
	sprintf (name, "JCAI Debug Output window - %d", aiIndex);

	dbgWindow = DbgCreateWindow (name, 400, 400);
}

void MainAI::InitAI(IGlobalAICallback* callback, int team)
{
	aicb = callback;
	cb = callback->GetAICallback ();

	if (!LoadCommonData(callback))
		return;

	cfgLoaded=true;

	map.CalculateInfoMap (cb, aiConfig.infoblocksize);

	metalmap.debugShowSpots = aiConfig.showMetalSpots;
	metalmap.Initialize (cb);

	buildmap.Init (cb, &metalmap);
	
	if (aiConfig.showDebugWindow)
		InitDebugWindow();

	int MAX_UNITS = -1;
	bool valueFetchOk = cb->GetValue(AIVAL_UNIT_LIMIT, &MAX_UNITS);
	if (!valueFetchOk) {
		ChatMsgPrintf(cb, "Failed to fetch unit limit (AIVAL_UNIT_LIMIT)");
		return;
	}
	
	unitDefs = new const UnitDef*[MAX_UNITS];
	for (int a=0;a<MAX_UNITS;a++) unitDefs[a]=0;
}

void MainAI::UnitMoveFailed(int unit)
{
	UnitIterator u = units.find(unit);

	if (u != units.end()) {
		aiUnit *p = u->second;

		if (p->owner) 
			p->owner->UnitMoveFailed (p);
	}
}

void MainAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
	map.UpdateDamageMeasuring (damage, damaged,cb);
}

int MainAI::HandleEvent (int msg, const void *data)
{
	switch (msg) {
		case AI_EVENT_UNITGIVEN: {
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			aiUnit *u=units[cte->unit]=new aiUnit;
			u->id=cte->unit;
			u->flags|=UNIT_FINISHED;// assumption here!
			u->def=cb->GetUnitDef(cte->unit);
			break;}
		case AI_EVENT_UNITCAPTURED:{
			ChangeTeamEvent* cte=(ChangeTeamEvent *)data;
			UnitIterator u = units.find(cte->unit);
			if (u != units.end()) {
				aiUnit *p = u->second;
				// let the handler of the unit destroy it
				if (p->owner)
					p->owner->UnitDestroyed (p);
				else {
					ChatMsgPrintf (cb, "Error: %s didn't have handler\n", p->def->name.c_str());
					delete p;
				}
				units.erase (u);
			}
			break;}
	}
	return 0;
}

void MainAI::Startup (int cmdrid)
{
	if (!cfgLoaded)
		return;

	const UnitDef* cdef = cb->GetUnitDef(cmdrid);
    
	sidecfg = dynamic_cast<CfgList*>(modConfig.root->GetValue (cdef->name.c_str()));
	if (!sidecfg)
	{
		ChatMsgPrintf (cb, "No info for side of %s\n", cdef->name.c_str());
		cfgLoaded=false;
		skip=true;
		return;
	}

	globals.buildmap = &buildmap;
	globals.cb = cb;
	globals.map = &map;
	globals.sidecfg = sidecfg;
	globals.metalmap = &metalmap;

	resourceManager = new ResourceManager (&globals);
	resourceManager->DistributeResources();
	globals.resourceManager = resourceManager;

	// Create task-creating handlers:
	try { 
		TaskManager *taskman = new TaskManager (&globals);
		aiUnit *com = taskman->Init (cmdrid);
		globals.handlers.insert (taskman);
		globals.taskManager = taskman;
		units[cmdrid] = com;

		TaskFactory *forceHandler = new ForceHandler (&globals);
		globals.handlers.insert (forceHandler);

		TaskFactory* resUnitHandler = new ResourceUnitHandler (&globals);
		globals.handlers.insert (resUnitHandler);

		TaskFactory* defenseHandler = new SupportHandler (&globals);
		globals.handlers.insert (defenseHandler);

		TaskFactory* reconHandler = new ReconHandler (&globals);
		globals.handlers.insert (reconHandler);

		TaskFactory **tf = globals.taskFactories;
		tf[0] = resUnitHandler;
		tf[1] = forceHandler;
		tf[2] = defenseHandler;
		tf[3] = reconHandler;
	}
	catch (const char *s)
	{
		ChatMsgPrintf (cb, "Error: %s",s);
		cfgLoaded=false;
	}

	// do a single update of the infomap, so there is correct info even the first frames
	map.UpdateBaseCenter (cb);
	map.UpdateThreatInfo (cb);
}

void MainAI::UnitCreated(int unit)
{
	if (!cfgLoaded)
		return;

	const UnitDef *def = cb->GetUnitDef (unit);

	if (def->isCommander)
	{
		Startup (unit);
		return;
	}

	aiUnit *u = globals.taskManager->UnitCreated (unit);
	u->id = unit;
	u->def = def;
	units [unit] = u;
}

void MainAI::UnitDestroyed(int unit, int attacker)
{
	if (!cfgLoaded)
		return;

	UnitIterator u = units.find (unit);

	assert (u != units.end());

	aiUnit *p = u->second;
	//ChatDebugPrintf (cb,"%s lost,agent: %s",p->def->name.c_str(), p->owner ? p->owner->GetName() : "none");

	// the build handler should know it can build on this spot again
	if (p->owner != globals.taskManager)
	{
		BuildTable::UDef* ud = buildTable.GetCachedDef (p->def->id);
		if (ud->IsBuilding ())
			globals.taskManager->RemoveUnitBlocking (p->def, cb->GetUnitPos (p->id));
	}

	// let the handler of the unit destroy it
	if (p->owner)
		p->owner->UnitDestroyed (p);
	else {
		ChatMsgPrintf (cb, "Error: %s didn't have handler\n", p->def->name.c_str());
		delete p;
	}
	units.erase (u);
}

void MainAI::UnitFinished (int unit)
{
	if (!cfgLoaded)
		return;

	UnitIterator u = units.find (unit);
	assert (u != units.end());

	aiUnit *p = u->second;
	p->flags |= UNIT_FINISHED;

	if (p->def->isCommander)
		return;

	p->UnitFinished ();
	if (p->owner)
		p->owner->UnitFinished (p);
}

void MainAI::UnitIdle (int unit) {}

void MainAI::EnemyEnterLOS(int enemy) {}
void MainAI::EnemyLeaveLOS(int enemy) {}
void MainAI::EnemyEnterRadar(int enemy) {}
void MainAI::EnemyLeaveRadar(int enemy) {}
void MainAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir) {}
void MainAI::EnemyDestroyed (int enemy, int attacker) {}

void MainAI::Update()
{
	if (!cfgLoaded)
		return;

	int f=cb->GetCurrentFrame ();

	if (skip)
		return;

	if (f == 70)
	{
		const char *sl = modConfig.root->GetLiteral ("StartLine");
		if (sl) cb->SendTextMsg (sl,0);
	}

	resourceManager->Update();

	if(f%MAP_UPDATE_INTERVAL==0)
		map.UpdateThreatInfo (cb);
	if(f%(MAP_UPDATE_INTERVAL*2)==4)
		map.UpdateBaseCenter(cb);

	for (HandlerList::iterator i=globals.handlers.begin();i!=globals.handlers.end();++i) 
	{
		const char *n = (*i)->GetName ();
		(*i)->Update ();
	}

	if (dbgWindow)
	{
		if (DbgWindowIsClosed(dbgWindow))
		{
			DbgDestroyWindow (dbgWindow);
			dbgWindow = 0;
		}
		else
		{
			DbgClearWindow(dbgWindow);

			DbgWndPrintf (dbgWindow, 0, 0, "C(%d,%d),D(%d,%d)", 
				(int)map.baseCenter.x, (int)map.baseCenter.y,(int)map.baseDirection.x,(int)map.baseDirection.y);
			map.DrawThreatMap (dbgWindow,0,200,128,128);
			map.DrawDefenseMap (dbgWindow,130,200,128,128);

			TaskManager::Stat *s=&globals.taskManager->stat;
			DbgWndPrintf (dbgWindow, 0, 180, "MinBuildspeed=%f, Cur=%f", s->minBuildSpeed,s->totalBuildSpeed);

			globals.resourceManager->ShowDebugInfo (dbgWindow);
			
			DbgWindowUpdate (dbgWindow);
		}
	}
}

float3 MainAI::ClosestBuildSite(const UnitDef* unitdef,float3 pos,int minDist)
{
	return cb->ClosestBuildSite (unitdef, pos,map.mblocksize, minDist);
}

