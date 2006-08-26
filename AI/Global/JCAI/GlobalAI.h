//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_GLOBAL_AI_H
#define JC_GLOBAL_AI_H

#include "InfoMap.h"
#include "MetalSpotMap.h"
#include "BuildTable.h"
#include "BuildMap.h"
#include "TaskManager.h"

const char AI_NAME[]="JCAI";

class AIConfig;
class MainAI;
class DbgWindow;
struct UnitDef;

class ForceHandler;
class TaskManager;
class ResourceUnitHandler;
class SupportHandler;
class ReconHandler;
class BuildMap;

struct ReconConfig;
struct ForceConfig;

class CfgList;

class IAICheats;

class MainAI : public IGlobalAI  
{
public:
	MainAI(int aiID);
	virtual ~MainAI();

	void InitAI(IGlobalAICallback* callback, int team);

	void UnitCreated(int unit);									//called when a new unit is created on ai team
	void UnitFinished(int unit);								//called when an unit has finished building
	void UnitIdle(int unit);										//called when a unit go idle and is not assigned to any group
	void UnitDestroyed(int unit, int attacker);					//called when a unit is destroyed
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);					//called when one of your units are damaged
	void UnitMoveFailed(int unit);

	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);

	void EnemyEnterRadar(int enemy);						//called when an enemy enters radar coverage (not called if enemy go directly from not known -> los)
	void EnemyLeaveRadar(int enemy);						//called when an enemy leaves radar coverage (not called if enemy go directly from inlos -> now known)

	void GotChatMsg(const char* msg,int player);					//called when someone writes a chat msg

	void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);	//called when an enemy inside los or radar is damaged

	void EnemyDestroyed (int enemy, int attacker);			//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
	int HandleEvent (int msg, const void*data);

	// called every frame
	void Update();

	void InitUnit (aiUnit *u, int id);

	void AddHandler (aiHandler *handler);
	void RemoveHandler (aiHandler *handler);

	void Startup (int cmdr);

	void DumpBuildOrders ();
	float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,int minDist);
	void InitDebugWindow();

	static bool LoadCommonData(IGlobalAICallback *cb);
	static void FreeCommonData();

	float GetEnergyIncome () { return cb->GetEnergyIncome(); }
	float GetMetalIncome () { return cb->GetMetalIncome(); }
	
	int aiIndex;
	bool cfgLoaded, skip;
	IAICallback* cb;
	IGlobalAICallback* aicb;
	std::map <int, aiUnit*> units;
	DbgWindow *dbgWindow;

	typedef std::map <int, aiUnit*>::iterator UnitIterator;

	InfoMap map;
	MetalSpotMap metalmap;
	BuildMap buildmap;

	const UnitDef** unitDefs;

	typedef set <aiHandler*> HandlerList;
	CfgList *sidecfg; // side specific info
	CGlobals globals;
	ResourceManager *resourceManager;
};


#endif
