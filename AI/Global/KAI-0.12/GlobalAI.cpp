#include "GlobalAI.h"
#include "Unit.h"


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
	// added by Kloot
	delete ai->dgunController;
	delete ai;
}


void CGlobalAI::InitAI(IGlobalAICallback* callback, int team) {
	// Initialize Log filename
	string mapname = string(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2 = localtime(&now1);

	// Date logfile name
	sprintf(this->c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",
		string(LOGFOLDER).c_str(), mapname.c_str(), (now2->tm_mon + 1), now2->tm_mday, (now2->tm_year + 1900), now2->tm_hour, now2->tm_min, team);

	// initialize class wrapper struct
	ai = new AIClasses;
	ai->cb = callback->GetAICallback();
	ai->cheat = callback->GetCheatInterface();
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, this->c);

	MyUnits.reserve(MAXUNITS);
	ai->MyUnits.reserve(MAXUNITS);

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
	// added by Kloot
	ai->dgunController	= new DGunController(ai->cb);

	L("All Class pointers initialized");

	ai->mm->Init();
	ai->ut->Init();
	ai->pather->Init();

	L("InitAI() complete");
}


void CGlobalAI::UnitCreated(int unit) {
	ai->uh->UnitCreated(unit);
	ai->econTracker->UnitCreated(unit);


	// added by Kloot
	const UnitDef* ud = ((this->ai)->cb)->GetUnitDef(unit);

	if (ud->isCommander && ud->canDGun) {
		((this->ai)->dgunController)->init(unit);
	}
}

void CGlobalAI::UnitFinished(int unit) {
	// let attackhandler handle cat_g_attack units
	ai->econTracker->UnitFinished(unit);

	if (GCAT(unit) == CAT_G_ATTACK) {
		ai->ah->AddUnit(unit);
	}
	else if (((ai->cb->GetCurrentFrame() < 20) || (ai->cb->GetUnitDef(unit)->speed <= 0))) {
		// Add comm at begginning of game and factories when they are built
		ai->uh->IdleUnitAdd(unit);
	}

	ai->uh->BuildTaskRemove(unit);
}

void CGlobalAI::UnitDestroyed(int unit, int attacker) {
	attacker = attacker;
	// L("GlobalAI::UnitDestroyed is called on unit:" << unit <<". its groupid:" << GUG(unit));
	ai->econTracker->UnitDestroyed(unit);

	if (GUG(unit) != -1) {
		ai->ah->UnitDestroyed(unit);
	}

	ai->uh->UnitDestroyed(unit);
}

void CGlobalAI::UnitIdle(int unit) {
	// attackhandler handles cat_g_attack units atm
	if (GCAT(unit) == CAT_G_ATTACK && ai->MyUnits.at(unit)->groupID != -1) {
		// attackHandler->UnitIdle(unit);
	}
	else {
		ai->uh->IdleUnitAdd(unit);
	}
}

void CGlobalAI::UnitDamaged(int damaged, int attacker, float damage, float3 dir) {
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
	// added by Kloot
	((this->ai)->dgunController)->handleDestroyEvent(attacker, enemy);
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
				ai->MyUnits[cte->unit]->Stop();
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
	ai->econTracker->frameUpdate();

	if (frame == 1) {
		// init defense matrix
		ai->dm->Init();
	}
	if (frame > 80) {
		// call buildup manager and unit handler (idle) update routine
		ai->bu->Update();
		ai->uh->IdleUnitUpdate();
	}

	// added by Kloot
	((this->ai)->dgunController)->update(frame);

	// call attack handler and unit handler (metal maker) update routines
	ai->ah->Update();
	ai->uh->MMakerUpdate();
}
