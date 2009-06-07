#include <string>
#include <sstream>
#include <fstream>

#include "KAIK.h"
#include "IncGlobalAI.h"

CR_BIND(CKAIK, );
CR_REG_METADATA(CKAIK, (
	CR_MEMBER(ai),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

// #define CREX_REG_STATE_COLLECTOR(Name, RootClass)
//    static RootClass=CKAIK* Name##State=KAIKState;
CREX_REG_STATE_COLLECTOR(KAIK, CKAIK);
CKAIK* KAIKStateExt = KAIKState;



void CKAIK::Save(std::ostream* ofs) {
	CREX_SC_SAVE(KAIK, ofs);
}

// called instead of InitAI() on load if IsLoadSupported() returns 1
void CKAIK::Load(IGlobalAICallback* callback, std::istream* ifs) {
	CREX_SC_LOAD(KAIK, ifs);
}

void CKAIK::PostLoad(void) {
	assert(ai != NULL);
}

void CKAIK::Serialize(creg::ISerializer* s) {
	for (int i = 0; i < MAX_UNITS; i++) {
		if (ai->ccb->GetUnitDef(i) != NULL) {
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
	ai = new AIClasses(callback);
	ai->Init();

	std::string verMsg =
		std::string(AI_VERSION) + " initialized succesfully!";
	std::string logMsg =
		"logging events to " + ai->logger->GetLogName();

	ai->cb->SendTextMsg(verMsg.c_str(), 0);
	ai->cb->SendTextMsg(logMsg.c_str(), 0);
	ai->cb->SendTextMsg(AI_CREDITS, 0);
}

void CKAIK::ReleaseAI() {
	delete ai; ai = NULL;
}



void CKAIK::UnitCreated(int unitID, int builderID) {
	ai->uh->UnitCreated(unitID);
	ai->econTracker->UnitCreated(unitID);
}

void CKAIK::UnitFinished(int unit) {
	ai->econTracker->UnitFinished(unit);

	const int      frame = ai->cb->GetCurrentFrame();
	const UnitDef* udef  = ai->cb->GetUnitDef(unit);

	if (udef != NULL) {
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
	ai->dgunConHandler->NotifyEnemyDestroyed(enemy, attacker);
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
				UnitCreated(cte->unit, -1);
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
	const int frame = ai->cb->GetCurrentFrame();

	// call economy tracker update routine
	ai->econTracker->frameUpdate(frame);
	ai->dgunConHandler->Update(frame);

	if (frame == 1) {
		// init defense matrix
		ai->dm->Init();
	}

	if (frame > 60) {
		// call buildup manager and unit handler (idle) update routine
		ai->bu->Update(frame);
		ai->uh->IdleUnitUpdate(frame);
	}

	// call attack handler and unit handler (metal maker) update routines
	ai->ah->Update(frame);
	ai->uh->MMakerUpdate(frame);
	ai->ct->Update(frame);
}
