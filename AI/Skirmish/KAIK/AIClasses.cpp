#include <cassert>
#include <string>

#include "System/Util.h"
#include "AIClasses.hpp"
#include "IncGlobalAI.h"

#include "KAIK.h"
extern CKAIK* KAIKStateExt;

CR_BIND(AIClasses, );
CR_REG_METADATA(AIClasses, (
	CR_MEMBER(ecoTracker),
	CR_MEMBER(buildupHandler),
	CR_MEMBER(threatMap),
	CR_MEMBER(unitHandler),
	CR_MEMBER(defenseMatrix),
	CR_MEMBER(attackHandler),
	CR_MEMBER(dgunControllerHandler),
	CR_MEMBER(activeUnits),
	CR_MEMBER(unitIDs),
	CR_MEMBER(initialized),
	CR_MEMBER(initFrame),
	CR_POSTLOAD(Load),
	CR_RESERVED(16)
));

CR_BIND(UnitType, );
CR_REG_METADATA(UnitType, (
	CR_MEMBER(canBuildList),
	CR_MEMBER(builtByList),
	CR_MEMBER(DPSvsUnit),
	// cannot serialize this directly
	// CR_MEMBER(def),
	CR_ENUM_MEMBER(category),
	CR_MEMBER(isHub),
	CR_MEMBER(techLevel),
	CR_MEMBER(costMultiplier),
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
	CR_MEMBER(stuckCount),
	CR_MEMBER(idleStartFrame),
	CR_MEMBER(commandOrderPushFrame),
	CR_ENUM_MEMBER(categoryMaker),
	CR_MEMBER(estimateRealStartFrame),
	CR_MEMBER(estimateFramesForNanoBuildActivation),
	CR_MEMBER(estimateETAforMoveingToBuildSite),
	CR_MEMBER(distanceToSiteBeforeItCanStartBuilding),
	CR_RESERVED(16)
));

CR_BIND(BuildTask, );
CR_REG_METADATA(BuildTask, (
	CR_MEMBER(id),
	CR_ENUM_MEMBER(category),
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



AIClasses::AIClasses(IGlobalAICallback* gcb):
	callbackHandler(NULL),
	ccallbackHandler(NULL),

	ecoTracker(NULL),
	buildupHandler(NULL),
	metalMap(NULL),
	mathHandler(NULL),
	pathFinder(NULL),
	unitTable(NULL),
	threatMap(NULL),
	unitHandler(NULL),
	defenseMatrix(NULL),
	attackHandler(NULL),
	dgunControllerHandler(NULL),
	commandTracker(NULL),
	logHandler(NULL),
	luaConfigParser(NULL)
{
	callbackHandler  = gcb->GetAICallback();
	ccallbackHandler = gcb->GetCheatInterface();

	initialized = false;
	initFrame = callbackHandler->GetCurrentFrame();

	// register for EnemyCreated, etc.
	ccallbackHandler->EnableCheatEvents(true);
}

AIClasses::~AIClasses() {
	if (initialized) {
		for (int i = 0; i < MAX_UNITS; i++) {
			delete activeUnits[i];
		}

		delete attackHandler;
		delete buildupHandler;
		delete ecoTracker;
		delete mathHandler;
		delete pathFinder;
		delete threatMap;
		delete unitTable;
		delete metalMap;
		delete unitHandler;
		delete dgunControllerHandler;

		delete commandTracker;
		delete logHandler;
		delete luaConfigParser;
	}
}

void AIClasses::Init() {
	assert(!initialized);

	const std::map<std::string, std::string>& info = callbackHandler->GetMyInfo();
	const std::map<std::string, std::string>::const_iterator it = info.find("supportedMods");
	// NOTE: use GetModShortName and GetModVersion()?
	const std::string& modArchvName = callbackHandler->GetModName();
	const std::string& modHumanName = callbackHandler->GetModHumanName();

	if (it != info.end()) {
		const std::string& mods = it->second;

		size_t i = 0;
		size_t j = 0;
		size_t p = 0, q = 0;

		std::string s;

		while (!initialized) {
			i = mods.find_first_of('(', i);
			j = mods.find_first_of(')', j);

			if (i == std::string::npos || j == std::string::npos) {
				break;
			}

			// extract a "(shortName, longName, minVersion, maxVersion)" tuple
			s = mods.substr(i + 1, j - i - 1);

			q = s.find_first_of(',', p); std::string shortName  = StringTrim(s.substr(p, q - p)); p = q + 1;
			q = s.find_first_of(',', p); std::string longName   = StringTrim(s.substr(p, q - p)); p = q + 1;
		//	q = s.find_first_of(',', p); std::string minVersion = StringTrim(s.substr(p, q - p)); p = q + 1;
		//	                             std::string maxVersion = StringTrim(s.substr(p, q - p));

			// to determine whether the current mod is supported, test if
			//   1. one of the "shortName" values is a substring of the mod archive-name, or
			//   2. one of the "longName" values is a substring of the mod human-name
			//
			// we do it this way because
			//   1. AI's can not retrieve the mod version (except by "looking" for it in the archive- or
			//      human-name strings) nor the real short-name, so the alternatives are comparing mod
			//      (archive- or human-)names directly or matching hashes
			//   2. a mod can unknowingly or deliberately break AI support when going from v0.1 to v0.2
			//   3. the archive- and human-names of any given mod can be completely unrelated
			//   4. storing the exact human-name of every supported mod in AIInfo is a maintenance PITA
			//      (since these typically change with every mod release, and mods are updated frequently)
			//   5. mod hashes are even more unfriendly to keep in AIInfo and also version-dependent
			//   6. comparing names without the version parts means greater flexibility for end-users
			if (modArchvName.find(shortName) != std::string::npos) { initialized = true; }
			if (modHumanName.find(longName ) != std::string::npos) { initialized = true; }

			i += 1;
			j += 1;
			p  = 0;
			q  = 0;
		}
	}

	if (!initialized) {
		return;
	}

	activeUnits.resize(MAX_UNITS, NULL);
	unitIDs.resize(MAX_UNITS, -1);

	for (int i = 0; i < MAX_UNITS; i++) {
		activeUnits[i]      = new CUNIT(this);
		activeUnits[i]->uid =  i;
	}

	logHandler            = new CLogger(callbackHandler);
	luaConfigParser       = new LuaParser();
	commandTracker        = new CCommandTracker(this);

	mathHandler           = new CMaths(this);
	unitTable             = new CUnitTable(this);
	metalMap              = new CMetalMap(this);
	pathFinder            = new CPathFinder(this);
	threatMap             = new CThreatMap(this);
	unitHandler           = new CUnitHandler(this);
	defenseMatrix         = new CDefenseMatrix(this);
	ecoTracker            = new CEconomyTracker(this);
	buildupHandler        = new CBuildUp(this);
	attackHandler         = new CAttackHandler(this);
	dgunControllerHandler = new CDGunControllerHandler(this);

	metalMap->Init();
	unitTable->Init();
	pathFinder->Init();
}

void AIClasses::Load() {
	assert(callbackHandler != NULL);
	assert(ccallbackHandler != NULL);

	assert(activeUnits.size() == MAX_UNITS);
	assert(unitIDs.size() == MAX_UNITS);

	// allocate and initialize all NON-serialized
	// AI components; the ones that were serialized
	// have their own PostLoad() callins
	logHandler     = new CLogger(callbackHandler);
	commandTracker = new CCommandTracker(this);
	mathHandler    = new CMaths(this);
	metalMap       = new CMetalMap(this);
	pathFinder     = new CPathFinder(this);

	metalMap->Init();
	pathFinder->Init();
}



void BuildTask::PostLoad() { def = KAIKStateExt->GetAi()->cb->GetUnitDef(id); }
void TaskPlan::PostLoad() { def = KAIKStateExt->GetAi()->cb->GetUnitDef(defName.c_str()); }
void UnitType::PostLoad() { /*def = KAIKStateExt->GetAi()->cb->GetUnitDef(?);*/ def = NULL; }
void UpgradeTask::PostLoad() { /*newBuildingDef = KAIKStateExt->GetAi()->cb->GetUnitDef(?);*/ newBuildingDef = NULL; }
