#include "GlobalAI.h"
#include "Unit.h"

// TODO: move to GlobalAI.h
CR_BIND(CGlobalAI, );
CR_REG_METADATA(CGlobalAI, (
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

// TODO: REG_METADATA for each of these...
CR_BIND(CAttackGroup, (NULL, 0));
CR_BIND(CAttackHandler, (NULL));
CR_BIND(CBuildUp, (NULL));
CR_BIND(CDefenseMatrix, (NULL));
CR_BIND(DGunController, (NULL));
CR_BIND(CEconomyTracker, (NULL));
CR_BIND(BuildingTracker, );
CR_BIND(EconomyUnitTracker, );
CR_BIND(CMetalMaker, (NULL));
CR_BIND(CMetalMaker::UnitInfo, );
CR_BIND(CThreatMap, (NULL));
CR_BIND(CUNIT, );
CR_BIND(CUnitHandler, (NULL));



// TODO: move to Containers.h
CR_BIND(AIClasses, );
CR_REG_METADATA(AIClasses, (
	CR_MEMBER(econTracker),
	CR_MEMBER(bu),
	CR_MEMBER(tm),
	CR_MEMBER(uh),
	CR_MEMBER(dm),
	CR_MEMBER(ah),
	CR_MEMBER(dgunController)
));

// TODO: move to Containers.h
CR_BIND(integer2, );
CR_REG_METADATA(integer2, (
	CR_MEMBER(x),
	CR_MEMBER(y)
));

// TODO: move to Containers.h
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
	CR_MEMBER(distanceToSiteBeforeItCanStartBuilding)
));

// TODO: move to Containers.h
CR_BIND(BuildTask, );
CR_REG_METADATA(BuildTask, (
	CR_MEMBER(id),
	CR_MEMBER(category),
	CR_MEMBER(builders),
	CR_MEMBER(builderTrackers),
	CR_MEMBER(currentBuildPower),
	// CR_MEMBER(def),
	CR_MEMBER(pos),
	CR_POSTLOAD(PostLoad)
));

// TODO: move to Containers.h
CR_BIND(TaskPlan, );
CR_REG_METADATA(TaskPlan, (
	CR_MEMBER(id),
	CR_MEMBER(builders),
	CR_MEMBER(builderTrackers),
	CR_MEMBER(currentBuildPower),
	// CR_MEMBER(def),
	CR_MEMBER(pos),
	CR_POSTLOAD(PostLoad)
));

// TODO: move to Containers.h
CR_BIND(Factory, );
CR_REG_METADATA(Factory, (
	CR_MEMBER(id),
	CR_MEMBER(supportbuilders),
	CR_MEMBER(supportBuilderTrackers)
));

// TODO: move to Containers.h
CR_BIND(NukeSilo, );
CR_REG_METADATA(NukeSilo, (
	CR_MEMBER(id),
	CR_MEMBER(numNukesReady),
	CR_MEMBER(numNukesQueued)
));

// TODO: move to Containers.h
CR_BIND(MetalExtractor, );
CR_REG_METADATA(MetalExtractor, (
	CR_MEMBER(id),
	CR_MEMBER(buildFrame)
));

// TODO: move to DGunController.hpp
CR_BIND(ControllerState, );
CR_REG_METADATA(ControllerState, (
	CR_MEMBER(inited),
	CR_MEMBER(dgunOrderFrame),
	CR_MEMBER(reclaimOrderFrame),
	CR_MEMBER(targetSelectionFrame),
	CR_MEMBER(targetID),
	CR_MEMBER(oldTargetPos)
));

CREX_REG_STATE_COLLECTOR(KAI, CGlobalAI);

// TODO: move to Containers.h
void BuildTask::PostLoad(void) { def = KAIState->ai->cb->GetUnitDef(id); }
void TaskPlan::PostLoad(void) { def = KAIState->ai->cb->GetUnitDef(id); }








CGlobalAI::CGlobalAI() {
}

CGlobalAI::~CGlobalAI() {
	delete ai->LOGGER;
	delete ai->ah;
	delete ai->bu;
	delete ai->econTracker;
	delete ai->parser;
	delete ai->math;
	delete ai->debug;
	delete ai->pather;
	delete ai->tm;
	delete ai->ut;
	delete ai->mm;
	delete ai->uh;
	delete ai->dgunController;
	delete ai;
}


// called instead of InitAI() on load if IsLoadSupported() returns 1
void CGlobalAI::Load(IGlobalAICallback* callback, std::istream* ifs) {
	ai = new AIClasses;
	ai->cb = callback->GetAICallback();
	ai->cheat = callback->GetCheatInterface();

	// initialize log filename
	string mapname = string(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2;
	now2 = localtime(&now1);

	int team = ai->cb->GetMyTeam();

	sprintf(c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",
		string(LOGFOLDER).c_str(), mapname.c_str(), now2->tm_mon + 1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour, now2->tm_min, team);

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, c);
	ai->LOGGER = new std::ofstream(c);

	CREX_SC_LOAD(KAI, ifs);
}

void CGlobalAI::Save(std::ostream* ofs) {
	CREX_SC_SAVE(KAI, ofs);
}



void CGlobalAI::PostLoad(void) {
	// init non-serialized objects
	ai->debug	= new CDebug(ai);
	ai->math	= new CMaths(ai);
	ai->parser	= new CSunParser(ai);
	ai->ut		= new CUnitTable(ai);
	ai->mm		= new CMetalMap(ai);
	ai->pather	= new CPathFinder(ai);
	ai->em		= new CEconomyManager(ai);

	ai->mm->Init();
	ai->ut->Init();
	ai->pather->Init();
}

void CGlobalAI::Serialize(creg::ISerializer* s) {
	if (!s->IsWriting())
		MyUnits.resize(MAXUNITS, CUNIT(ai));

	for (int i = 0; i < MAXUNITS; i++) {
		if (ai->cheat->GetUnitDef(i)) {
			// do not save non-existing units
			s->SerializeObjectInstance(&(MyUnits[i]), MyUnits[i].GetClass());
			if (!s->IsWriting()) {
				MyUnits[i].myid = i;
			}
		} else if (!s->IsWriting()) {
			MyUnits[i].myid = i;
			MyUnits[i].groupID = -1;
		}
		if (!s->IsWriting())
			ai->MyUnits.push_back(&MyUnits[i]);
	}

	s->SerializeObjectInstance(ai, ai->GetClass());
}





void CGlobalAI::InitAI(IGlobalAICallback* callback, int team) {
	// initialize log filename
	string mapname = string(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2 = localtime(&now1);

	// timestamp logfile name
	sprintf(this->c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",
		string(LOGFOLDER).c_str(), mapname.c_str(), (now2->tm_mon + 1), now2->tm_mday, (now2->tm_year + 1900), now2->tm_hour, now2->tm_min, team);

	// initialize class wrapper struct
	ai = new AIClasses;
	ai->cb = callback->GetAICallback();
	ai->cheat = callback->GetCheatInterface();
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, this->c);

	MyUnits.reserve(MAXUNITS);
	ai->MyUnits.reserve(MAXUNITS);

	// initialize 5000 CUNIT objects
	for (int i = 0; i < MAXUNITS; i++) {
		MyUnits.push_back(CUNIT(ai));
		MyUnits[i].myid = i;
		MyUnits[i].groupID = -1;

		ai->MyUnits.push_back(&MyUnits[i]);
	}

	ai->debug			= new CDebug(ai);
	ai->math			= new CMaths(ai);
	ai->LOGGER			= new std::ofstream(this->c);
	ai->parser			= new CSunParser(ai);
	ai->ut				= new CUnitTable(ai);
	ai->mm				= new CMetalMap(ai);
	ai->pather			= new CPathFinder(ai);
	ai->tm				= new CThreatMap(ai);
	ai->uh				= new CUnitHandler(ai);
	ai->dm				= new CDefenseMatrix(ai);
	ai->econTracker		= new CEconomyTracker(ai);
	ai->bu				= new CBuildUp(ai);
	ai->ah				= new CAttackHandler(ai);
	ai->dgunController	= new DGunController(ai->cb);

	ai->mm->Init();
	ai->ut->Init();
	ai->pather->Init();

	ai->cb->SendTextMsg(AI_VERSION " initialized succesfully!", 0);
	ai->cb->SendTextMsg(AI_CREDITS, 0);
}


void CGlobalAI::UnitCreated(int unit) {
	ai->uh->UnitCreated(unit);
	ai->econTracker->UnitCreated(unit);

	const UnitDef* ud = ai->cb->GetUnitDef(unit);

	if (ud && ud->isCommander && ud->canDGun) {
		ai->dgunController->init(unit);
	}
}

void CGlobalAI::UnitFinished(int unit) {
	// let attackhandler handle cat_g_attack units
	ai->econTracker->UnitFinished(unit);
	int frame = ai->cb->GetCurrentFrame();
	const UnitDef* udef = ai->cb->GetUnitDef(unit);

	if (GCAT(unit) == CAT_G_ATTACK) {
		ai->ah->AddUnit(unit);
	}
	else if (frame < 20 || (udef && udef->speed <= 0.0f)) {
		// Add commander at begginning of game
		// and factories when they are built
		ai->uh->IdleUnitAdd(unit, frame);
	}

	ai->uh->BuildTaskRemove(unit);
}

void CGlobalAI::UnitDestroyed(int unit, int attacker) {
	attacker = attacker;
	ai->econTracker->UnitDestroyed(unit);

	if (GUG(unit) != -1) {
		ai->ah->UnitDestroyed(unit);
	}

	ai->uh->UnitDestroyed(unit);
}

void CGlobalAI::UnitIdle(int unit) {
	// AttackHandler handles cat_g_attack units
	if (GCAT(unit) == CAT_G_ATTACK && ai->MyUnits.at(unit)->groupID != -1) {
		// attackHandler->UnitIdle(unit);
	} else {
		ai->uh->IdleUnitAdd(unit, ai->cb->GetCurrentFrame());
	}
}

void CGlobalAI::UnitDamaged(int damaged, int attacker, float damage, float3 dir) {
	attacker = attacker;
	dir = dir;
	ai->econTracker->UnitDamaged(damaged, damage);
}

void CGlobalAI::UnitMoveFailed(int unit) {
	unit = unit;
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

void CGlobalAI::EnemyDestroyed(int enemy, int attacker) {
	ai->dgunController->handleDestroyEvent(attacker, enemy);
}

void CGlobalAI::EnemyDamaged(int damaged, int attacker, float damage, float3 dir) {
	damaged = damaged;
	attacker = attacker;
	damage = damage;
	dir = dir;
}



void CGlobalAI::GotChatMsg(const char* msg, int player) {
	msg = msg;
	player = player;
}


int CGlobalAI::HandleEvent(int msg, const void* data) {
	switch (msg) {
		case AI_EVENT_UNITGIVEN: {
			const ChangeTeamEvent* cte = (const ChangeTeamEvent*) data;

			if ((cte->newteam) == (ai->cb->GetMyTeam())) {
				// got a unit
				UnitCreated(cte->unit);
				UnitFinished(cte->unit);
				ai->uh->IdleUnitAdd(cte->unit, ai->cb->GetCurrentFrame());
			}
		} break;
		case AI_EVENT_UNITCAPTURED: {
			const ChangeTeamEvent* cte = (const ChangeTeamEvent*) data;

			if ((cte->oldteam) == (ai->cb->GetMyTeam())) {
				// lost a unit
				UnitDestroyed(cte->unit, 0);
			}
		} break;
	}

	return 0;
}




void CGlobalAI::Update() {
	int frame = ai->cb->GetCurrentFrame();

	// call economy tracker update routine
	ai->econTracker->frameUpdate(frame);

	if (frame == 1) {
		// init defense matrix
		ai->dm->Init();
	}
	if (frame > 80) {
		// call buildup manager and unit handler (idle) update routine
		ai->bu->Update(frame);
		ai->uh->IdleUnitUpdate(frame);
	}

	ai->dgunController->update(frame);

	// call attack handler and unit handler (metal maker) update routines
	ai->ah->Update(frame);
	ai->uh->MMakerUpdate(frame);
}
