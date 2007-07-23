#include "GlobalAI.h"
#include "Unit.h"
#include "CoverageHandler.h"
#include "creg/cregex.h"
#include "creg/Serializer.h"
#include "creg/STL_List.h"

CR_BIND(CGlobalAI,)

CR_REG_METADATA(CGlobalAI,(
//				CR_MEMBER(MyUnits),
/*
				CR_MEMBER(totalSumTime),
				CR_MEMBER(updateTimerGroup),
				CR_MEMBER(econTrackerFrameUpdate),
				CR_MEMBER(updateTheirDistributionTime),
				CR_MEMBER(updateMyDistributionTime),
				CR_MEMBER(builUpTime),
				CR_MEMBER(idleUnitUpdateTime),
				CR_MEMBER(attackHandlerUpdateTime),
				CR_MEMBER(MMakerUpdateTime),
				CR_MEMBER(unitCreatedTime),
				CR_MEMBER(unitFinishedTime),
				CR_MEMBER(unitDestroyedTime),
				CR_MEMBER(unitIdleTime),
				CR_MEMBER(economyManagerUpdateTime),
				CR_MEMBER(globalAILogTime),
				CR_MEMBER(threatMapTime),
*/
				CR_RESERVED(64),
				CR_SERIALIZER(Serialize),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(AIClasses ,)

CR_REG_METADATA(AIClasses,(
				CR_MEMBER(econTracker),
				CR_MEMBER(bu),
			//	CR_MEMBER(parser),
			//	CR_MEMBER(mm),
			//	CR_MEMBER(math),
			//	CR_MEMBER(debug),
			//	CR_MEMBER(pather),
			//	CR_MEMBER(ut),
				CR_MEMBER(tm),
				CR_MEMBER(uh),
				CR_MEMBER(dm),
				CR_MEMBER(ah),
			//	CR_MEMBER(em),
			//	CR_MEMBER(dc),
			//	CR_MEMBER(sh),
			//	CR_MEMBER(MyUnits),
			//	CR_MEMBER(LOGGER),
				CR_MEMBER(dgunController),
				CR_MEMBER(radarCoverage),
				CR_MEMBER(sonarCoverage),
				CR_MEMBER(rjammerCoverage),
				CR_MEMBER(sjammerCoverage),
				CR_MEMBER(antinukeCoverage),
				CR_MEMBER(shieldCoverage),
				CR_RESERVED(128)
//				CR_POSTLOAD(PostLoad)
				));

CR_BIND(integer2 ,)

CR_REG_METADATA(integer2,(
				CR_MEMBER(x),
				CR_MEMBER(y)
				));

//CR_BIND(KAIMoveType ,)

////CR_REG_METADATA(KAIMoveType,(
////				));

CR_BIND(BuilderTracker ,)

CR_REG_METADATA(BuilderTracker,(
				CR_MEMBER(builderID),
				CR_MEMBER(buildTaskId),
				CR_MEMBER(taskPlanId),
				CR_MEMBER(factoryId),
				CR_MEMBER(customOrderId),

				CR_MEMBER(stuckCount),
				CR_MEMBER(idleStartFrame),
				CR_MEMBER(commandOrderPushFrame),
				CR_MEMBER(categoryMaker),

				//CR_MEMBER(def),

				CR_MEMBER(estimateRealStartFrame),
				CR_MEMBER(estimateFramesForNanoBuildActivation),
				CR_MEMBER(estimateETAforMoveingToBuildSite),
				CR_MEMBER(distanceToSiteBeforeItCanStartBuilding),
				CR_RESERVED(128),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(BuildTask ,)

CR_REG_METADATA(BuildTask,(
				CR_MEMBER(id),
				CR_MEMBER(category),

				CR_MEMBER(builderTrackers),

				CR_MEMBER(currentBuildPower),
				CR_MEMBER(pos),
				CR_RESERVED(64),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(TaskPlan ,)

CR_REG_METADATA(TaskPlan,(
				CR_MEMBER(id),

				CR_MEMBER(builderTrackers),

				CR_MEMBER(currentBuildPower),
				CR_MEMBER(defname),
				CR_MEMBER(pos),
				CR_RESERVED(64),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(Factory ,)

CR_REG_METADATA(Factory,(
				CR_MEMBER(id),

				CR_MEMBER(supportBuilderTrackers),

				CR_MEMBER(currentBuildPower),
				CR_MEMBER(currentTargetBuildPower),
				CR_RESERVED(128),
				CR_POSTLOAD(PostLoad)
				));


void AIClasses::PostLoad()
{
}

CREX_REG_STATE_COLLECTOR(KAI,CGlobalAI);

void BuilderTracker::PostLoad()
{
	def = KAIState->ai->cb->GetUnitDef(builderID);
}

void BuildTask::PostLoad()
{
	def = KAIState->ai->cb->GetUnitDef(id);
}

void TaskPlan::PostLoad()
{
	def = KAIState->ai->cb->GetUnitDef(defname.c_str());
}

void Factory::PostLoad()
{
	factoryDef = KAIState->ai->cb->GetUnitDef(id);
}

CGlobalAI::CGlobalAI() {
}

CGlobalAI::~CGlobalAI() {
	delete ai -> LOGGER;
	delete ai -> ah;
	delete ai -> bu;
	delete ai -> econTracker;
	delete ai -> parser;
	delete ai -> math;
	delete ai -> debug;
	delete ai -> pather;
	delete ai -> tm;
	delete ai -> ut;
	delete ai -> mm;
	delete ai -> uh;
	// added by Kloot
	delete ai -> dgunController;

	delete ai -> radarCoverage;
	delete ai -> sonarCoverage;
	delete ai -> rjammerCoverage;
	delete ai -> sjammerCoverage;
	delete ai -> antinukeCoverage;
	delete ai -> shieldCoverage;
	delete ai;
}

void CGlobalAI::Serialize(creg::ISerializer *s)
{
	if (!s->IsWriting()) MyUnits.resize(MAXUNITS,CUNIT(ai));
	for (int i = 0; i < MAXUNITS; i++) {
		if (ai->cheat->GetUnitDef(i)) // do not save non existing units
			s->SerializeObjectInstance(&(MyUnits[i]),MyUnits[i].GetClass());
			if (!s->IsWriting()) MyUnits[i].myid = i;
		else if (!s->IsWriting()) {
			MyUnits[i].myid = i;
			MyUnits[i].groupID = -1;
		}
		if (!s->IsWriting()) ai -> MyUnits.push_back(&MyUnits[i]);
	}
	s->SerializeObjectInstance(ai,ai->GetClass());
}

void CGlobalAI::PostLoad()
{
	ai -> debug				= new CDebug(ai);
	ai -> math				= new CMaths(ai);
	ai -> parser			= new CSunParser(ai);
	ai -> sh				= new CSurveillanceHandler(ai);
	ai -> ut				= new CUnitTable(ai);
	ai -> mm				= new CMetalMap(ai);
	ai -> pather			= new CPathFinder(ai);
//	ai -> tm				= new CThreatMap(ai);
//	ai -> uh				= new CUnitHandler(ai);
//	ai -> dm				= new CDefenseMatrix(ai);
//	ai -> econTracker		= new CEconomyTracker(ai);
//	ai -> bu				= new CBuildUp(ai);
//	ai -> ah				= new CAttackHandler(ai);
	ai -> dc				= new CDamageControl(ai);
	ai -> em				= new CEconomyManager(ai);
	// added by Kloot
//	ai -> dgunController	= new DGunController(ai);

//	ai -> radarCoverage		= new CCoverageHandler(ai,CCoverageHandler::Radar);
//	ai -> sonarCoverage		= new CCoverageHandler(ai,CCoverageHandler::Sonar);
//	ai -> rjammerCoverage	= new CCoverageHandler(ai,CCoverageHandler::RJammer);
//	ai -> sjammerCoverage	= new CCoverageHandler(ai,CCoverageHandler::SJammer);
//	ai -> antinukeCoverage	= new CCoverageHandler(ai,CCoverageHandler::AntiNuke);
//	ai -> shieldCoverage	= new CCoverageHandler(ai,CCoverageHandler::Shield);


	totalSumTime				= 0;
	updateTimerGroup			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::Update()");
	econTrackerFrameUpdate		= ai -> math -> GetNewTimerGroupNumber("ai -> econTracker -> frameUpdate()");
	updateTheirDistributionTime	= ai -> math -> GetNewTimerGroupNumber("ai -> dc -> UpdateTheirDistribution()");
	updateMyDistributionTime	= ai -> math -> GetNewTimerGroupNumber("ai -> dc -> UpdateMyDistribution()");
	threatMapTime				= ai -> math -> GetNewTimerGroupNumber("ai -> tm -> Create()  (threatMap)");
	builUpTime					= ai -> math -> GetNewTimerGroupNumber("ai -> bu -> Update()  (buildup)");
	idleUnitUpdateTime			= ai -> math -> GetNewTimerGroupNumber("idleUnitUpdateTime");
	attackHandlerUpdateTime		= ai -> math -> GetNewTimerGroupNumber("ai -> ah -> Update()  (attackHandler)");
	MMakerUpdateTime			= ai -> math -> GetNewTimerGroupNumber("ai -> uh -> MMakerUpdate()");
	economyManagerUpdateTime	= ai -> math -> GetNewTimerGroupNumber("ai -> em -> Update()  (economyManager)");
	globalAILogTime				= ai -> math -> GetNewTimerGroupNumber("GlobalAI log time  ( L() )");
	unitCreatedTime				= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitCreated(int unit)");
	unitFinishedTime			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitFinished(int unit)");
	unitDestroyedTime			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitDestroyed(int unit,int attacker)");
	unitIdleTime				= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitIdle(int unit)");
	L("Timers initialized");

	ai -> mm -> Init();
	L("ai -> mm -> Init(); done");
	ai -> ut -> Init();
	L("ai -> ut -> Init(); done");
	ai -> pather -> Init();
	L("post load done");
	dminited = !ai->dm->ChokePointArray.empty();
//	ai -> dc -> GenerateDPSTables();
//	L("GenerateDPSTables done");
}

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team) {
	// initialize log filename
	string mapname = string(callback -> GetAICallback() -> GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2;
	now2 = localtime(&now1);

	// date logfile name
	sprintf(c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",
			string(LOGFOLDER).c_str(), mapname.c_str(), now2 -> tm_mon + 1, now2 -> tm_mday, now2 -> tm_year + 1900, now2 -> tm_hour, now2 -> tm_min, team);

	ai = new AIClasses;

	ai -> cb	= callback -> GetAICallback();
	ai -> cheat	= callback -> GetCheatInterface();

	ai -> cb -> GetValue(AIVAL_LOCATE_FILE_W, c);

	MyUnits.reserve(MAXUNITS);
	ai -> MyUnits.reserve(MAXUNITS);

	for (int i = 0; i < MAXUNITS; i++) {
		MyUnits.push_back(CUNIT(ai));
		MyUnits[i].myid = i;
		MyUnits[i].groupID = -1;

		ai -> MyUnits.push_back(&MyUnits[i]);
	}

	ai -> debug				= new CDebug(ai);
	ai -> math				= new CMaths(ai);
	ai -> LOGGER			= new std::ofstream(c);
	ai -> parser			= new CSunParser(ai);
	ai -> sh				= new CSurveillanceHandler(ai);
	ai -> ut				= new CUnitTable(ai);
	ai -> mm				= new CMetalMap(ai);
	ai -> pather			= new CPathFinder(ai);
	ai -> tm				= new CThreatMap(ai);
	ai -> uh				= new CUnitHandler(ai);
	ai -> dm				= new CDefenseMatrix(ai);
	ai -> econTracker		= new CEconomyTracker(ai);
	ai -> bu				= new CBuildUp(ai);
	ai -> ah				= new CAttackHandler(ai);
	ai -> dc				= new CDamageControl(ai);
	ai -> em				= new CEconomyManager(ai);
	// added by Kloot
	ai -> dgunController	= new DGunController(ai);

	ai -> radarCoverage		= new CCoverageHandler(ai,CCoverageHandler::Radar);
	ai -> sonarCoverage		= new CCoverageHandler(ai,CCoverageHandler::Sonar);
	ai -> rjammerCoverage	= new CCoverageHandler(ai,CCoverageHandler::RJammer);
	ai -> sjammerCoverage	= new CCoverageHandler(ai,CCoverageHandler::SJammer);
	ai -> antinukeCoverage	= new CCoverageHandler(ai,CCoverageHandler::AntiNuke);
	ai -> shieldCoverage	= new CCoverageHandler(ai,CCoverageHandler::Shield);

	L("All Class pointers initialized");

	// make the timer numbers
	totalSumTime				= 0;
	updateTimerGroup			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::Update()");
	econTrackerFrameUpdate		= ai -> math -> GetNewTimerGroupNumber("ai -> econTracker -> frameUpdate()");
	updateTheirDistributionTime	= ai -> math -> GetNewTimerGroupNumber("ai -> dc -> UpdateTheirDistribution()");
	updateMyDistributionTime	= ai -> math -> GetNewTimerGroupNumber("ai -> dc -> UpdateMyDistribution()");
	threatMapTime				= ai -> math -> GetNewTimerGroupNumber("ai -> tm -> Create()  (threatMap)");
	builUpTime					= ai -> math -> GetNewTimerGroupNumber("ai -> bu -> Update()  (buildup)");
	idleUnitUpdateTime			= ai -> math -> GetNewTimerGroupNumber("idleUnitUpdateTime");
	attackHandlerUpdateTime		= ai -> math -> GetNewTimerGroupNumber("ai -> ah -> Update()  (attackHandler)");
	MMakerUpdateTime			= ai -> math -> GetNewTimerGroupNumber("ai -> uh -> MMakerUpdate()");
	economyManagerUpdateTime	= ai -> math -> GetNewTimerGroupNumber("ai -> em -> Update()  (economyManager)");
	globalAILogTime				= ai -> math -> GetNewTimerGroupNumber("GlobalAI log time  ( L() )");
	unitCreatedTime				= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitCreated(int unit)");
	unitFinishedTime			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitFinished(int unit)");
	unitDestroyedTime			= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitDestroyed(int unit,int attacker)");
	unitIdleTime				= ai -> math -> GetNewTimerGroupNumber("CGlobalAI::UnitIdle(int unit)");

	L("Timers initialized");
	ai -> mm -> Init();
	L("ai -> mm -> Init(); done");
	ai -> ut -> Init();
	L("ai -> ut -> Init(); done");
	ai -> pather -> Init();
	L("init done");
//	ai -> dc -> GenerateDPSTables();
//	L("GenerateDPSTables done");

	dminited = false;

	ai -> cb -> SendTextMsg("KAI v0.23b1 loaded!", 0);
}



void CGlobalAI::UnitCreated(int unit) {
	ai -> math -> StartTimer(totalSumTime);
	ai -> math -> StartTimer(globalAILogTime);
	L("GlobalAI::UnitCreated is called on unit:" << unit << ". its groupid:" << ai -> MyUnits[unit] -> groupID << " name:" << ai -> MyUnits[unit] -> def() -> humanName);
	ai -> math -> StopTimer(globalAILogTime);
	ai -> math -> StartTimer(unitCreatedTime);
	ai -> uh -> UnitCreated(unit);
	ai -> econTracker -> UnitCreated(unit);
	ai -> math -> StopTimer(unitCreatedTime);
	ai -> math -> StopTimer(totalSumTime);


	// added by Kloot
	const UnitDef* ud = ((this -> ai) -> cb) -> GetUnitDef(unit);

	if (ud -> isCommander && ud -> canDGun) {
		((this -> ai) -> dgunController) -> init((this -> ai) -> cb, unit);
	}
	ai -> radarCoverage -> Change(unit,false);
	ai -> sonarCoverage -> Change(unit,false);
	ai -> rjammerCoverage -> Change(unit,false);
	ai -> sjammerCoverage -> Change(unit,false);
	ai -> antinukeCoverage -> Change(unit,false);
	ai -> shieldCoverage -> Change(unit,false);
}

void CGlobalAI::UnitFinished(int unit) {
	ai -> math -> StartTimer(totalSumTime);
	ai -> math -> StartTimer(globalAILogTime);
	L("GlobalAI::UnitFinished is called on unit:" << unit << ". its groupid:" << ai -> MyUnits[unit] -> groupID << " name:" << ai -> MyUnits[unit] -> def() -> humanName);
	ai -> math -> StopTimer(globalAILogTime);
	ai -> math -> StartTimer(unitFinishedTime);
	ai -> econTracker -> UnitFinished(unit);

	if (ai -> ah -> CanHandleThisUnit(unit)) {
		ai -> ah -> AddUnit(unit);
	}
	// add comm at begginning of game and factories when theyre built
	else if((ai -> cb -> GetCurrentFrame() < 2 || ai -> cb -> GetUnitDef(unit) -> speed <= 0)) {
		ai -> uh -> IdleUnitAdd(unit);
	}

	ai -> uh -> BuildTaskRemove(unit);
	ai -> math -> StopTimer(unitFinishedTime);
	ai -> math -> StopTimer(totalSumTime);
}

void CGlobalAI::UnitDestroyed(int unit,int attacker) {
	attacker = attacker;

	ai -> bu -> MexUpgraders.remove(unit);
	ai -> math -> StartTimer(totalSumTime);
	ai -> math -> StartTimer(globalAILogTime);
	L("GlobalAI::UnitDestroyed is called on unit:" << unit << ". its groupid:" << ai -> MyUnits[unit] -> groupID << " name:" << ai -> MyUnits[unit] -> def() -> humanName);
	ai -> math -> StopTimer(globalAILogTime);
	ai -> math -> StartTimer(unitDestroyedTime);
	ai -> econTracker -> UnitDestroyed(unit);

	if (GUG(unit) != -1) {
		ai -> ah -> UnitDestroyed(unit);
	}

	ai -> uh -> UnitDestroyed(unit);
	ai -> math -> StopTimer(unitDestroyedTime);
	ai -> math -> StopTimer(totalSumTime);
	const UnitDef* ud = ((this -> ai) -> cb) -> GetUnitDef(unit);
	if (ud -> isCommander && ud -> canDGun) {
		ai -> dgunController ->inited = false;
	}
	ai -> radarCoverage -> Change(unit,true);
	ai -> sonarCoverage -> Change(unit,true);
	ai -> rjammerCoverage -> Change(unit,true);
	ai -> sjammerCoverage -> Change(unit,true);
	ai -> antinukeCoverage -> Change(unit,true);
	ai -> shieldCoverage -> Change(unit,true);
}

void CGlobalAI::UnitIdle(int unit) {
	ai -> math -> StartTimer(totalSumTime);
	ai -> math -> StartTimer(globalAILogTime);
	L("Idle: " << unit);
	ai -> math -> StopTimer(globalAILogTime);
	ai -> math -> StartTimer(unitIdleTime);
	ai -> econTracker -> frameUpdate();

	// attackhandler handles cat_g_attack units atm
	if ((GCAT(unit) == CAT_ATTACK || GCAT(unit) == CAT_ARTILLERY || GCAT(unit) == CAT_ASSAULT) && ai -> MyUnits.at(unit) -> groupID != -1) {
		// attackHandler -> UnitIdle(unit);
	}
	else {
		if (GCAT(unit)!=CAT_BUILDER || !(ai -> uh -> GetBuilderTracker(unit) -> factoryId))
			ai -> uh -> IdleUnitAdd(unit);
	}
	ai -> math -> StopTimer(unitIdleTime);
	ai -> math -> StopTimer(totalSumTime);
}

void CGlobalAI::UnitDamaged(int damaged, int attacker, float damage, float3 dir) {
	attacker = attacker;
	dir = dir;
	ai -> econTracker -> UnitDamaged(damaged, damage);


	// added by Kloot
	const UnitDef* ud = ((this -> ai) -> cb) -> GetUnitDef(damaged);

	if (ud -> isCommander && ud -> canDGun) {
		((this -> ai) -> dgunController) -> handleAttackEvent(attacker, damage, dir, ((this -> ai) -> cb) -> GetUnitPos(attacker));
	}
}

void CGlobalAI::UnitMoveFailed(int unit) {
	if (ai -> cb -> GetUnitDef(unit) != NULL) {
		ai -> MyUnits[unit] -> stuckCounter++;
	}
}



void CGlobalAI::EnemyEnterLOS(int enemy) {
	enemy = enemy;
}

void CGlobalAI::EnemyLeaveLOS(int enemy) {
	enemy = enemy;
}

void CGlobalAI::EnemyEnterRadar(int enemy) {
	enemy = enemy;
}

void CGlobalAI::EnemyLeaveRadar(int enemy) {
	enemy = enemy;
}

void CGlobalAI::EnemyDamaged(int damaged, int attacker, float damage, float3 dir) {
	damaged = damaged;
	attacker = attacker;
	damage = damage;
	dir = dir;
}

void CGlobalAI::EnemyDestroyed(int enemy, int attacker) {
	// added by Kloot
	((this -> ai) -> dgunController) -> handleDestroyEvent(attacker, enemy);
}



void CGlobalAI::GotChatMsg(const char* msg, int player) {
	msg = msg;
	player = player;
}



int CGlobalAI::HandleEvent(int msg, const void* data) {
	// unit steal and donate support
	L("msg: " << msg);

	if (msg == AI_EVENT_UNITGIVEN) {
		const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;

		if (cte -> newteam == ai -> cb -> GetMyTeam()) {
			// just got a unit
			UnitCreated(cte -> unit);
			UnitFinished(cte -> unit);
			ai -> MyUnits[cte -> unit] -> Stop();
		}
	}
	else if (msg == AI_EVENT_UNITCAPTURED) {
		const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;

		if (cte -> oldteam == ai -> cb -> GetMyTeam()) {
			// just lost a unit
			UnitDestroyed(cte -> unit, 0);
		}
	}

	L("msg end");
	return 0;
}



void CGlobalAI::Update() {
	int frame = ai -> cb -> GetCurrentFrame();

	ai -> math -> StartTimer(totalSumTime);
	ai -> math -> StartTimer(globalAILogTime);
//	L("start: " << frame);
	ai -> math -> StopTimer(globalAILogTime);
	ai -> math -> StartTimer(updateTimerGroup);

	ai -> math -> StartTimer(econTrackerFrameUpdate);
	ai -> econTracker -> frameUpdate();
	ai -> math -> StopTimer(econTrackerFrameUpdate);

	// added by Kloot
	if ((frame % 5) == 0) {
		((this -> ai) -> dgunController) -> update(frame);
	}

	if (frame % 60 == 1) {
		ai -> math -> StartTimer(updateTheirDistributionTime);
		ai -> dc -> UpdateTheirDistribution();
		ai -> math -> StopTimer(updateTheirDistributionTime);
	}

	if (frame % 60 == 20) {
		ai -> math -> StartTimer(updateMyDistributionTime);
		ai -> dc -> UpdateMyDistribution();
		ai -> math -> StopTimer(updateMyDistributionTime);
	}

	if (frame % 60 == 40) {
		ai -> math -> StartTimer(economyManagerUpdateTime);
		ai -> em -> Update();
		ai -> math -> StopTimer(economyManagerUpdateTime);
	}

	if (frame > 80) {
		ai -> math -> StartTimer(threatMapTime);

		if (frame % 15 == 0)
			ai -> tm -> Create();

		ai -> math -> StopTimer(threatMapTime);
		ai -> math -> StartTimer(builUpTime);
		ai -> bu -> Update();
		ai -> math -> StopTimer(builUpTime);
		ai -> math -> StartTimer(idleUnitUpdateTime);
		ai -> uh -> IdleUnitUpdate();
		ai -> math -> StopTimer(idleUnitUpdateTime);

		ai -> uh -> CloakUpdate();
	}

	ai -> math -> StartTimer(attackHandlerUpdateTime);
	ai -> ah -> Update();
	ai -> math -> StopTimer(attackHandlerUpdateTime);

	ai -> math -> StartTimer(MMakerUpdateTime);
	ai -> uh -> MMakerUpdate();
	ai -> math -> StopTimer(MMakerUpdateTime);

	ai -> math -> StartTimer(globalAILogTime);
//	L("end: " << frame << ", updateTime: " << updateTime);
	ai -> math -> StopTimer(globalAILogTime);

	ai -> math -> StopTimer(totalSumTime);

	ai->uh->StockpileUpdate();

	// don't include the setup time of defenseMatrix in the total
	if (!dminited) {
		// ai -> math -> StartTimer(defenseMatrixInitTime);
		ai -> dm -> Init();
		// ai -> math -> StopTimer(defenseMatrixInitTime);
		dminited = true;
	}
	// print the times every 2 mins
	if (frame % 3600 == 0 && frame > 1) {
		L("Here is the time distribution after " << (frame / 1800) << " mins");
		ai -> math -> PrintAllTimes();
	}
}

void CGlobalAI::Load(IGlobalAICallback* callback,std::istream *ifs)
{
	ai = new AIClasses;
	ai -> cb	= callback -> GetAICallback();
	ai -> cheat	= callback -> GetCheatInterface();

	// initialize log filename
	string mapname = string(callback -> GetAICallback() -> GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2;
	now2 = localtime(&now1);

	int team=ai->cb->GetMyTeam();

	sprintf(c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",
			string(LOGFOLDER).c_str(), mapname.c_str(), now2 -> tm_mon + 1, now2 -> tm_mday, now2 -> tm_year + 1900, now2 -> tm_hour, now2 -> tm_min, team);

	ai -> cb -> GetValue(AIVAL_LOCATE_FILE_W, c);

	ai -> LOGGER			= new std::ofstream(c);

	CREX_SC_LOAD(KAI,ifs);
}

void CGlobalAI::Save(std::ostream *ofs)
{
	CREX_SC_SAVE(KAI,ofs);
}
