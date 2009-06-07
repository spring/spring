// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------


#pragma once

#include "aidef.h"
#include "AAIBrain.h"
#include "AAIExecute.h"
#include "AAISector.h"
#include "AAIBuildTable.h"
#include "AAIGroup.h"
#include "AAIBuildTask.h"
#include "AAIUnitTable.h"
#include "AAIMap.h"
#include "AAIAirForceManager.h"
#include "AAIAttackManager.h"
#include "AAIConstructor.h"
#include <math.h>

using namespace std;

class AAIExecute;

class AAI : public IGlobalAI
{
public:
	AAI();
	virtual ~AAI();

	void InitAI(IGlobalAICallback* callback, int team);

	void UnitCreated(int unit, int builder);								//called when a new unit is created on ai team
	void UnitFinished(int unit);											//called when an unit has finished building
	void UnitIdle(int unit);												//called when a unit go idle and is not assigned to any group
	void UnitDestroyed(int unit, int attacker);								//called when a unit is destroyed
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);		//called when one of your units are damaged
	void UnitMoveFailed(int unit);

	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);

	void EnemyEnterRadar(int enemy);				//called when an enemy enters radar coverage (not called if enemy go directly from not known -> los)
	void EnemyLeaveRadar(int enemy);				//called when an enemy leaves radar coverage (not called if enemy go directly from inlos -> now known)

	void GotChatMsg(const char* msg,int player);	//called when someone writes a chat msg

	void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);	//called when an enemy inside los or radar is damaged
	void EnemyDestroyed(int enemy, int attacker);

	int HandleEvent(int msg, const void *data);

	// called every frame
	void Update();

	// callbacks
	IAICallback* cb;
	IGlobalAICallback* aicb;

	// side 1= arm, 2 = core, 0 = neutral
	int side;

	// if there is more than one instance of AAI, make sure to allocate/free memory only once
	int aai_instance;

	// list of buildtasks
	list<AAIBuildTask*> build_tasks;

	AAIBrain *brain;			// makes decisions
	AAIExecute *execute;		// executes all kinds of tasks
	AAIUnitTable *ut;			// unit table
	AAIBuildTable *bt;			// buildtable for the different units
	AAIMap *map;				// keeps track of all constructed buildings and searches new constr. sites
	AAIAirForceManager *af;		// coordinates the airforce
	AAIAttackManager *am;		// coordinates combat forces

	vector<list<AAIGroup*> > group_list;  // unit groups

	bool initialized;

	FILE *file;
};
