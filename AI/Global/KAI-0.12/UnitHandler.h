#ifndef UNITHANDLER_H
#define UNITHANDLER_H


#include "GlobalAI.h"
#include "MetalMaker.h"

class CUnitHandler {
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
		// Will be used to track stuck workers
		void UnitMoveFailed(int unit);

		void MMakerAdd(int unit);
		void MMakerRemove(int unit);
		void MMakerUpdate();

		void BuildTaskCreate(int id);
		void BuildTaskRemove(int id);
		void BuildTaskRemove(BuilderTracker* builderTracker);
		bool BuildTaskAddBuilder (int builder, int category);
		void BuildTaskAddBuilder(BuildTask* buildTask, BuilderTracker* builderTracker);
		BuildTask* GetBuildTask(int buildTaskId);
		BuildTask* BuildTaskExist(float3 pos, const UnitDef* builtdef);

		void TaskPlanCreate(int builder, float3 pos, const UnitDef* builtdef);
		void TaskPlanRemove(BuilderTracker* builderTracker);
		bool TaskPlanExist(float3 pos, const UnitDef* builtdef);
		void TaskPlanAdd(TaskPlan* taskPlan, BuilderTracker* builderTracker);
		TaskPlan* GetTaskPlan(int taskPlanId);

		BuilderTracker* GetBuilderTracker(int builder);

		void FactoryAdd(int id);
		void FactoryRemove(int id);
		bool FactoryBuilderAdd(int builder);
		bool FactoryBuilderAdd(BuilderTracker* builderTracker);
		void FactoryBuilderRemove(BuilderTracker* builderTracker);

		// Use this to tell the tracker that the builder is on a reclaim job
		void BuilderReclaimOrder(int builderId, float3 pos);

		bool VerifyOrder(BuilderTracker* builderTracker);
		void ClearOrder(BuilderTracker* builderTracker, bool reportError);
		void DecodeOrder(BuilderTracker* builderTracker, bool reportError);

		vector<list<int>*> IdleUnits;

		vector<list<BuildTask>*> BuildTasks;
		vector<list<TaskPlan>*> TaskPlans;
		vector<list<int>*> AllUnitsByCat;
		vector<list<int>*> AllUnitsByType;

		list<Factory> Factories;
		list<integer2> Limbo;
		list<BuilderTracker*> BuilderTrackers;

		CMetalMaker* metalMaker;

	private:
		AIClasses* ai;
		int taskPlanCounter;
};


#endif
