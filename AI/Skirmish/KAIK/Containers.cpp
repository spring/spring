#include "Containers.h"
#include "IncGlobalAI.h"

#include "KAIK.h"
extern CKAIK* KAIKStateExt;

CR_BIND(AIClasses, );
CR_REG_METADATA(AIClasses, (
	CR_MEMBER(econTracker),
	CR_MEMBER(bu),
	CR_MEMBER(tm),
	CR_MEMBER(uh),
	CR_MEMBER(dm),
	CR_MEMBER(ah),
	CR_MEMBER(dgunConHandler),
	CR_RESERVED(16)
));

CR_BIND(UnitType, );
CR_REG_METADATA(UnitType, (
	CR_MEMBER(canBuildList),
	CR_MEMBER(builtByList),
	CR_MEMBER(DPSvsUnit),
	CR_MEMBER(TargetCategories),
	// cannot serialize this directly
	// CR_MEMBER(def),
	CR_MEMBER(category),
	CR_MEMBER(isHub),
	CR_MEMBER(techLevel),
	CR_MEMBER(costMultiplier),
	CR_MEMBER(sides),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(integer2, );
CR_REG_METADATA(integer2, (
	CR_MEMBER(x),
	CR_MEMBER(y)
));

CR_BIND(BuilderTracker, );
CR_REG_METADATA(BuilderTracker, (
	CR_MEMBER(builderID),
	CR_MEMBER(buildTaskId),
	CR_MEMBER(taskPlanId),
	CR_MEMBER(factoryId),
	CR_MEMBER(customOrderId),
	CR_MEMBER(stuckCount),
	CR_MEMBER(idleStartFrame),
	CR_MEMBER(commandOrderPushFrame),
	CR_MEMBER(categoryMaker),
	CR_MEMBER(estimateRealStartFrame),
	CR_MEMBER(estimateFramesForNanoBuildActivation),
	CR_MEMBER(estimateETAforMoveingToBuildSite),
	CR_MEMBER(distanceToSiteBeforeItCanStartBuilding),
	CR_RESERVED(16)
));

CR_BIND(BuildTask, );
CR_REG_METADATA(BuildTask, (
	CR_MEMBER(id),
	CR_MEMBER(category),
	CR_MEMBER(builders),
	CR_MEMBER(builderTrackers),
	CR_MEMBER(currentBuildPower),
	// cannot serialize this directly
	// CR_MEMBER(def),
	CR_MEMBER(pos),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(TaskPlan, );
CR_REG_METADATA(TaskPlan, (
	CR_MEMBER(id),
	CR_MEMBER(builders),
	CR_MEMBER(builderTrackers),
	CR_MEMBER(currentBuildPower),
	// cannot serialize this directly
	// CR_MEMBER(def),
	CR_MEMBER(defName),
	CR_MEMBER(pos),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(UpgradeTask, );
CR_REG_METADATA(UpgradeTask, (
	CR_MEMBER(oldBuildingID),
	CR_MEMBER(oldBuildingPos),
	// cannot serialize this directly
	// CR_MEMBER(newBuildingDef),
	CR_MEMBER(creationFrame),
	CR_MEMBER(reclaimStatus),
	CR_MEMBER(builderIDs),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(Factory, );
CR_REG_METADATA(Factory, (
	CR_MEMBER(id),
	CR_MEMBER(supportbuilders),
	CR_MEMBER(supportBuilderTrackers),
	CR_RESERVED(8)
));

CR_BIND(NukeSilo, );
CR_REG_METADATA(NukeSilo, (
	CR_MEMBER(id),
	CR_MEMBER(numNukesReady),
	CR_MEMBER(numNukesQueued),
	CR_RESERVED(8)
));

CR_BIND(MetalExtractor, );
CR_REG_METADATA(MetalExtractor, (
	CR_MEMBER(id),
	CR_MEMBER(buildFrame),
	CR_RESERVED(8)
));

void BuildTask::PostLoad() { def = KAIKStateExt->GetAi()->cb->GetUnitDef(id); }
void TaskPlan::PostLoad() { def = KAIKStateExt->GetAi()->cb->GetUnitDef(defName.c_str()); }
void UnitType::PostLoad() { /*def = KAIKStateExt->GetAi()->cb->GetUnitDef(?);*/ def = NULL; }
void UpgradeTask::PostLoad() { /*newBuildingDef = KAIKStateExt->GetAi()->cb->GetUnitDef(?);*/ newBuildingDef = NULL; }
