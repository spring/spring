#ifndef UNITHANDLER_H
#define UNITHANDLER_H


#include "GlobalAI.h"
#include "MetalMaker.h"

using std::list;
using std::vector;

class CMetalMaker;

class CUnitHandler {
	public:
		CR_DECLARE(CUnitHandler);

		CUnitHandler(AIClasses* ai);
		~CUnitHandler();

		void UnitCreated(int unit);
		void UnitDestroyed(int unit);

		void IdleUnitUpdate(int);
		void IdleUnitAdd(int unit, int);
		void IdleUnitRemove(int unit);
		int GetIU(int category);
		int NumIdleUnits(int category);
		// used to track stuck workers
		void UnitMoveFailed(int unit);

		void MMakerAdd(int unit);
		void MMakerRemove(int unit);
		void MMakerUpdate(int);

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

		void NukeSiloAdd(int siloID);
		void NukeSiloRemove(int siloID);
		void MetalExtractorAdd(int extractorID);
		void MetalExtractorRemove(int extractorID);
		int GetOldestMetalExtractor(void);

		void FactoryAdd(int id);
		void FactoryRemove(int id);
		bool FactoryBuilderAdd(int builder);
		bool FactoryBuilderAdd(BuilderTracker* builderTracker);
		void FactoryBuilderRemove(BuilderTracker* builderTracker);

		UpgradeTask* CreateUpgradeTask(int oldBuildingID, const float3& oldBuildingPos, const UnitDef* newBuildingDef);
		UpgradeTask* FindUpgradeTask(int oldBuildingID);
		void RemoveUpgradeTask(UpgradeTask* task);
		bool AddUpgradeTaskBuilder(UpgradeTask* task, int builderID);
		void UpdateUpgradeTasks(int frame);

		// use this to tell the tracker that the builder is on a reclaim job
		void BuilderReclaimOrder(int builderId, float3 pos);

		bool VerifyOrder(BuilderTracker* builderTracker);
		void ClearOrder(BuilderTracker* builderTracker, bool reportError);
		void DecodeOrder(BuilderTracker* builderTracker, bool reportError);

		vector<list<int> > IdleUnits;
		vector<list<BuildTask> > BuildTasks;
		vector<list<TaskPlan> > TaskPlans;
		vector<list<int> > AllUnitsByCat;
		vector<list<int> > AllUnitsByType;

		list<Factory> Factories;
		list<NukeSilo> NukeSilos;
		vector<MetalExtractor> MetalExtractors;

		list<integer2> Limbo;
		list<BuilderTracker*> BuilderTrackers;

		CMetalMaker* metalMaker;

		int lastCapturedUnitFrame;
		int lastCapturedUnitID;

	private:
		AIClasses* ai;
		int taskPlanCounter;

		std::map<int, UpgradeTask*> upgradeTasks;
};


#endif
