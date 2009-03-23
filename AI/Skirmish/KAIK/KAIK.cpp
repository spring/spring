#include <string>
#include <sstream>
#include <fstream>

#include "KAIK.h"
#include "IncGlobalAI.h"

CR_BIND(CKAIK, );
CR_REG_METADATA(CKAIK, (
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

// #define CREX_REG_STATE_COLLECTOR(Name, RootClass)
//    static RootClass=CKAIK* Name##State=KAIKState;
CREX_REG_STATE_COLLECTOR(KAIK, CKAIK);
CKAIK* KAIKStateExt = KAIKState;



CKAIK::CKAIK() {
}

CKAIK::~CKAIK() {
	for (int i = 0; i < MAX_UNITS; i++) {
		delete ai->MyUnits[i]; ai->MyUnits[i] = 0x0;
	}

	delete ai->logger;
	delete ai->ah;
	delete ai->bu;
	delete ai->econTracker;
	delete ai->parser;
	delete ai->math;
	delete ai->pather;
	delete ai->tm;
	delete ai->ut;
	delete ai->mm;
	delete ai->uh;
	delete ai->dgunController;
	delete ai;
}



void CKAIK::Save(std::ostream* ofs) {
	CREX_SC_SAVE(KAIK, ofs);
}

// called instead of InitAI() on load if IsLoadSupported() returns 1
void CKAIK::Load(IGlobalAICallback* callback, std::istream* ifs) {
	ai = new AIClasses();
	ai->cb = callback->GetAICallback();
	ai->cheat = callback->GetCheatInterface();

	// initialize log filename
	std::string mapname(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2 = localtime(&now1);

	std::string cfgFolderStr(CFGFOLDER);
	std::string logFolderStr(LOGFOLDER);

	std::stringstream logFileName;
		logFileName << logFolderStr;
		logFileName << mapname;
		logFileName << "_";
		logFileName << now2->tm_mon + 1;
		logFileName << "-";
		logFileName << now2->tm_mday;
		logFileName << "-";
		logFileName << now2->tm_year + 1900;
		logFileName << "_";
		logFileName << now2->tm_hour;
		logFileName << ":";
		logFileName << now2->tm_min;
		logFileName << "_(";
		logFileName << ai->cb->GetMyTeam();
		logFileName << ").txt";

	memset(m_cfgFolderBuf, 0, 1024);
	memset(m_logFileBuf, 0, 1024);
	SNPRINTF(m_cfgFolderBuf, 1023, "%s", cfgFolderStr.c_str());
	SNPRINTF(m_logFileBuf, 1023, "%s", logFileName.str().c_str());

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, m_cfgFolderBuf);
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, m_logFileBuf);
	ai->logger = new CLogger(logFileName.str());

	CREX_SC_LOAD(KAIK, ifs);
}



void CKAIK::PostLoad(void) {
	// allocate and initialize all non-serialized
	// AI components; the ones that were serialized
	// have their own PostLoad() callins
	ai->math	= new CMaths(ai);
	ai->parser	= new CSunParser(ai);
	ai->mm		= new CMetalMap(ai);
	ai->pather	= new CPathFinder(ai);

	ai->mm->Init();
	ai->pather->Init();
}

void CKAIK::Serialize(creg::ISerializer* s) {
	if (!s->IsWriting()) {
		// if de-serializing a saved state, allocate
		// here instead of in InitAI() which we skip
		ai->unitIDs.resize(MAX_UNITS, -1);
		ai->MyUnits.resize(MAX_UNITS, new CUNIT(ai));
	}

	for (int i = 0; i < MAX_UNITS; i++) {
		if (ai->cheat->GetUnitDef(i)) {
			// do not save non-existing units
			s->SerializeObjectInstance(ai->MyUnits[i], ai->MyUnits[i]->GetClass());
			if (!s->IsWriting()) {
				ai->MyUnits[i]->myid = i;
			}
		} else if (!s->IsWriting()) {
			ai->MyUnits[i]->myid = i;
			ai->MyUnits[i]->groupID = -1;
		}
	}

	s->SerializeObjectInstance(ai, ai->GetClass());
}






void CKAIK::InitAI(IGlobalAICallback* callback, int team) {
	// initialize log filename
	std::string mapname(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size() - 4);

	time_t now1;
	time(&now1);
	struct tm* now2 = localtime(&now1);

	std::string cfgFolderStr(CFGFOLDER);
	std::string logFolderStr(LOGFOLDER);

	std::stringstream logFileName;
		logFileName << logFolderStr;
		logFileName << mapname;
		logFileName << "_";
		logFileName << now2->tm_mon + 1;
		logFileName << "-";
		logFileName << now2->tm_mday;
		logFileName << "-";
		logFileName << now2->tm_year + 1900;
		logFileName << "_";
		logFileName << now2->tm_hour;
		logFileName << ":";
		logFileName << now2->tm_min;
		logFileName << "_(";
		logFileName << team;
		logFileName << ").txt";

	memset(m_cfgFolderBuf, 0, 1024);
	memset(m_logFileBuf, 0, 1024);
	SNPRINTF(m_cfgFolderBuf, 1023, "%s", cfgFolderStr.c_str());
	SNPRINTF(m_logFileBuf, 1023, "%s", logFileName.str().c_str());

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, m_cfgFolderBuf);
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, m_logFileBuf);

	// initialize class wrapper struct
	ai = new AIClasses();
	ai->cb = callback->GetAICallback();
	ai->cheat = callback->GetCheatInterface();

	ai->MyUnits.resize(MAX_UNITS, 0x0);
	ai->unitIDs.resize(MAX_UNITS, -1);

	// initialize MAX_UNITS CUNIT objects
	for (int i = 0; i < MAX_UNITS; i++) {
		ai->MyUnits[i] = new CUNIT(ai);
		ai->MyUnits[i]->myid = i;
		ai->MyUnits[i]->groupID = -1;
	}

	ai->math			= new CMaths(ai);
	ai->logger			= new CLogger(logFileName.str());
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
	ai->dgunController	= new DGunController(ai);

	ai->mm->Init();
	ai->ut->Init();
	ai->pather->Init();

	std::string versMsg = std::string(AI_VERSION) + " initialized succesfully!";
	ai->cb->SendTextMsg(versMsg.c_str(), 0);
	ai->cb->SendTextMsg(AI_CREDITS, 0);
}


void CKAIK::UnitCreated(int unitID) {
	ai->uh->UnitCreated(unitID);
	ai->econTracker->UnitCreated(unitID);
}

void CKAIK::UnitFinished(int unit) {
	ai->econTracker->UnitFinished(unit);
	int frame = ai->cb->GetCurrentFrame();
	const UnitDef* udef = ai->cb->GetUnitDef(unit);

	if (udef) {
		// let attackhandler handle cat_g_attack units
		if (GCAT(unit) == CAT_G_ATTACK) {
			ai->ah->AddUnit(unit);
		} else {
			ai->uh->IdleUnitAdd(unit, frame);
		}

		ai->uh->BuildTaskRemove(unit);
	}
}

void CKAIK::UnitDestroyed(int unit, int attacker) {
	attacker = attacker;
	ai->econTracker->UnitDestroyed(unit);

	if (GUG(unit) != -1) {
		ai->ah->UnitDestroyed(unit);
	}

	ai->uh->UnitDestroyed(unit);
}

void CKAIK::UnitIdle(int unit) {
	if (ai->uh->lastCapturedUnitFrame == ai->cb->GetCurrentFrame()) {
		if (unit == ai->uh->lastCapturedUnitID) {
			// KLOOTNOTE: for some reason this also gets called when one
			// of our units is captured (in the same frame as, but after
			// HandleEvent(AI_EVENT_UNITCAPTURED)), *before* the unit has
			// actually changed teams (ie. for any unit that is no longer
			// on our team but still registers as such)
			ai->uh->lastCapturedUnitFrame = -1;
			ai->uh->lastCapturedUnitID = -1;
			return;
		}
	}

	// AttackHandler handles cat_g_attack units
	if (GCAT(unit) == CAT_G_ATTACK && ai->MyUnits[unit]->groupID != -1) {
		// attackHandler->UnitIdle(unit);
	} else {
		ai->uh->IdleUnitAdd(unit, ai->cb->GetCurrentFrame());
	}
}

void CKAIK::UnitDamaged(int damaged, int attacker, float damage, float3 dir) {
	attacker = attacker;
	dir = dir;
	ai->econTracker->UnitDamaged(damaged, damage);
}

void CKAIK::UnitMoveFailed(int unit) {
	unit = unit;
}



void CKAIK::EnemyEnterLOS(int enemy) {
	enemy = enemy;
}

void CKAIK::EnemyLeaveLOS(int enemy) {
	enemy = enemy;
}

void CKAIK::EnemyEnterRadar(int enemy) {
	enemy = enemy;
}

void CKAIK::EnemyLeaveRadar(int enemy) {
	enemy = enemy;
}

void CKAIK::EnemyDestroyed(int enemy, int attacker) {
	ai->dgunController->handleDestroyEvent(attacker, enemy);
}

void CKAIK::EnemyDamaged(int damaged, int attacker, float damage, float3 dir) {
	damaged = damaged;
	attacker = attacker;
	damage = damage;
	dir = dir;
}



void CKAIK::GotChatMsg(const char* msg, int player) {
	msg = msg;
	player = player;
}


int CKAIK::HandleEvent(int msg, const void* data) {
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

				// FIXME: multiple units captured during same frame?
				ai->uh->lastCapturedUnitFrame = ai->cb->GetCurrentFrame();
				ai->uh->lastCapturedUnitID = cte->unit;
			}
		} break;
	}

	return 0;
}




void CKAIK::Update() {
	int frame = ai->cb->GetCurrentFrame();

	// call economy tracker update routine
	ai->econTracker->frameUpdate(frame);

	if (frame == 1) {
		// init defense matrix
		ai->dm->Init();
	}
	if (frame > 60) {
		// call buildup manager and unit handler (idle) update routine
		ai->bu->Update(frame);
		ai->uh->IdleUnitUpdate(frame);
	}

	ai->dgunController->update(frame);

	// call attack handler and unit handler (metal maker) update routines
	ai->ah->Update(frame);
	ai->uh->MMakerUpdate(frame);
}
