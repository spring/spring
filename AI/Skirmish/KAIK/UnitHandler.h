#ifndef KAIK_UNITHANDLER_HDR
#define KAIK_UNITHANDLER_HDR

#include <vector>

#include "Containers.h"

struct UnitDef;

class CMetalMaker;
struct AIClasses;

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
		int GetIU(UnitCategory);
		int NumIdleUnits(UnitCategory);
		// used to track stuck workers
		void UnitMoveFailed(int unit);

		void MMakerAdd(int unit);
		void MMakerRemove(int unit);
		void MMakerUpdate(int);

		void BuildTaskCreate(int id);
		void BuildTaskRemove(int id);
		void BuildTaskRemove(BuilderTracker* builderTracker);
		bool BuildTaskAddBuilder (int builder, UnitCategory);
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

		std::vector<std::list<int> > IdleUnits;
		std::vector<std::list<BuildTask> > BuildTasks;
		std::vector<std::list<TaskPlan> > TaskPlans;
		std::vector<std::list<int> > AllUnitsByCat;
		std::vector<std::list<int> > AllUnitsByType;

		std::list<Factory> Factories;
		std::list<NukeSilo> NukeSilos;
		std::vector<MetalExtractor> MetalExtractors;

		std::list<integer2> Limbo;
		std::list<BuilderTracker*> BuilderTrackers;

		CMetalMaker* metalMaker;

		int lastCapturedUnitFrame;
		int lastCapturedUnitID;

	private:
		AIClasses* ai;
		int taskPlanCounter;

		std::map<int, UpgradeTask*> upgradeTasks;
};


#endif
