#ifndef UNITHANDLER_H
#define UNITHANDLER_H


#include "GlobalAI.h"
#include "MetalMaker.h"

class CUnitHandler
{
public:
	CR_DECLARE(CUnitHandler);
	CUnitHandler(AIClasses* ai);
	virtual ~CUnitHandler();
	void UnitCreated(int unit);
	void UnitDestroyed(int unit);

	void IdleUnitUpdate();
	void IdleUnitAdd(int unit);
	void IdleUnitRemove(int unit);
	int GetIU(int category);
	int NumIdleUnits(int category);
	void UnitMoveFailed(int unit); // Will be used to track stuck workers


	void MMakerAdd(int unit);
	void MMakerRemove(int unit);
	void MMakerUpdate();

	void CloakUpdate();

	void StockpileUpdate();

	void BuildTaskCreate(int id);
	void BuildTaskRemove(int id);
	void BuildTaskRemoved(BuilderTracker* builderTracker);
	bool BuildTaskAddBuilder (int builder, int category);
	void BuildTaskAddBuilder(BuildTask* buildTask, BuilderTracker* builderTracker);
	BuildTask* GetBuildTask(int buildTaskId);
	BuildTask* BuildTaskExist(float3 pos,const UnitDef* builtdef);

	void TaskPlanCreate(int builder, float3 pos, const UnitDef* builtdef);
	//void TaskPlanRemove (int builder);
	void TaskPlanRemoved (BuilderTracker* builderTracker);
	bool TaskPlanExist(float3 pos,const UnitDef* builtdef);
	void TaskPlanAdd (TaskPlan* taskPlan, BuilderTracker* builderTracker);
	TaskPlan* GetTaskPlan(int taskPlanId);
	
	BuilderTracker* GetBuilderTracker(int builder);

	void FactoryAdd (int id);
	void FactoryLost (int id);
	bool FactoryBuilderAdd (int builder);
	bool FactoryBuilderAdd (BuilderTracker* builderTracker);
	void FactoryBuilderRemoved (BuilderTracker* builderTracker);
	float Distance2DToNearestFactory (float x,float z);

	void BuilderReclaimOrder(int builderId, int target); // Use this to tell the tracker that the builder is on a reclaim job.


	bool VerifyOrder(BuilderTracker* builderTracker);
	void ClearOrder(BuilderTracker* builderTracker, bool reportError);
	void DecodeOrder(BuilderTracker* builderTracker, bool reportError);
	int GetFactoryHelpersCount();

	vector<list<int> > IdleUnits;

	vector<list<BuildTask*> > BuildTasks; // TODO: update useless code as the BuildTask's are free objects now.
	vector<list<TaskPlan*> > TaskPlans; // TODO: update useless code as the TaskPlan's are free objects now.
	vector<list<int> > AllUnitsByCat;
	vector<list<int> > AllUnitsByType;
	list<Factory> Factories;

	vector<int> UncloakedBuildings;
	vector<int> UncloakedUnits;
	vector<int> CloakedBuildings;
	vector<int> CloakedUnits;

	vector<int> StockpileDefenceUnits;
	vector<int> StockpileAttackUnits;

	list<integer2> Limbo;
	
	list<BuilderTracker*> BuilderTrackers;

	CMetalMaker* metalMaker;

private:
	AIClasses* ai;
	int taskPlanCounter;
	bool debugPoints;
};

#endif /* UNITHANDLER_H */
