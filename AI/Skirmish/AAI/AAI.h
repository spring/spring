// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_H
#define AAI_H

#include "LegacyCpp/IGlobalAI.h"
#include <list>
#include <vector>

namespace springLegacyAI {
	class IAICallback;
}

using namespace springLegacyAI;
using namespace std;

class AAIExecute;
class Profiler;
class AAIBrain;
class AAIBuildTask;
class AAIAirForceManager;
class AAIAttackManager;
class AAIBuildTable;
class AAIUnitTable;
class AAIMap;
class AAIGroup;

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

	void RecvChatMessage(const char* /*msg*/,int /*player*/) {}	//called when someone writes a chat msg
	void RecvLuaMessage(const char* /*inData*/, const char** /*outData*/) {}

	void EnemyDamaged(int /*damaged*/,int /*attacker*/,float /*damage*/,float3 /*dir*/) {}	//called when an enemy inside los or radar is damaged
	void EnemyDestroyed(int enemy, int attacker);
	void Log(const char* format, ...);
	void LogConsole(const char* format, ...);

	int HandleEvent(int msg, const void *data);
	//return count of aai instances
	int GetInstances() { return aai_instance; }

	// called every frame
	void Update();

	IAICallback* Getcb() { return cb; }
	IGlobalAICallback* Getaicb() { return aicb; }
	int Getside()
	{
		assert(side>=0);
		assert(side<=2);
		return side;
	}
	list<AAIBuildTask*>& Getbuild_tasks() { return build_tasks; }
	AAIBrain* Getbrain() { return brain; }
	AAIExecute* Getexecute() { return execute; }
	AAIUnitTable* Getut() { return ut; }
	AAIMap* Getmap() { return map; }
	AAIAirForceManager* Getaf() { return af; }
	AAIAttackManager* Getam() { return am; }
	AAIBuildTable* Getbt() { return bt; }
	vector<list<AAIGroup*> >& Getgroup_list() { return group_list; }

private:
	Profiler* GetProfiler(){ return profiler; }

	// callbacks
	IAICallback* cb;
	IGlobalAICallback* aicb;

	// side 1= arm, 2 = core, 0 = neutral
	int side;

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

	Profiler* profiler;
	FILE *file;
	bool initialized;

	// if there is more than one instance of AAI, make sure to allocate/free memory only once
	static int aai_instance;

};

#endif

