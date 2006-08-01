#pragma once

#include "aidef.h"
#include "AAIBrain.h"
#include "AAIExecute.h"
#include "AAISector.h"
#include "AAIBuildTable.h"
#include "AAIFactory.h"
#include "AAIGroup.h"
#include "AAIBuilder.h"
#include "AAIBuildTask.h"
#include "AAIUnitTable.h"
#include "AAIMap.h"
#include "AAIAirForceManager.h"
#include <math.h>

using namespace std;

class AAIExecute;

class AAI : public IGlobalAI 
{
public:
	AAI();
	virtual ~AAI();
	
	void InitAI(IGlobalAICallback* callback, int team);

	void UnitCreated(int unit);												//called when a new unit is created on ai team
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

	void EnemyDestroyed (int enemy, int attacker);

	int HandleEvent (int msg, const void *data);

	// called every frame
	void Update();
	
	// callbacks
	IAICallback* cb;
	IGlobalAICallback* aicb;

	// side 1= arm, 2 = core, 0 = neutral
	int side;

	// units, buildings etc.
	list<int> scouts;
	
	// wishlists for the different categories
	list<int> unit_wishlist;
	list<int> scout_wishlist;
	list<int> air_wishlist;

	// number of active/under construction units of all different types
	int activeUnits[(int)SEA_BUILDER+1];
	int futureUnits[(int)SEA_BUILDER+1];

	int activeScouts, futureScouts;

	// list of buildtasks
	list<AAIBuildTask*> build_tasks;

	AAIBrain *brain;			// makes decisions
	AAIExecute *execute;		// executes all kinds of tasks
	AAIUnitTable *ut;			// unit table
	AAIBuildTable *bt;			// buildtable for the different units
	AAIMap *map;				// keeps track of all constructed buildings and searches new constr. sites
	AAIAirForceManager *af;		// coordinates the airforce

	list<AAIGroup*> *group_list;  // unit groups

	bool initialized;

	FILE *file;
};

