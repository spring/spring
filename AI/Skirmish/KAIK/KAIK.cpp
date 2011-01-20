#ifdef LUA_LIB_EXT
#include <lua5.1/lua.hpp>
#else
#include "lib/lua/include/lua.h"
#include "lib/lua/include/lualib.h"
#include "lib/lua/include/lauxlib.h"
#endif

#include "KAIK.h"
#include "IncGlobalAI.h"

CR_BIND(CKAIK, );
CR_REG_METADATA(CKAIK, (
	CR_MEMBER(ai),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

CREX_REG_STATE_COLLECTOR(KAIK, CKAIK);
// #define CREX_REG_STATE_COLLECTOR(Name, RootClass)
//    static RootClass* Name ## State;
CKAIK* KAIKStateExt = KAIKState;



// called instead of InitAI() on load if IsLoadSupported() returns 1
void CKAIK::Load(IGlobalAICallback* callback, std::istream* ifs) { CREX_SC_LOAD(KAIK, ifs); }
void CKAIK::Save(std::ostream* ofs) { CREX_SC_SAVE(KAIK, ofs); }

void CKAIK::PostLoad(void) {
	assert((ai != NULL) && ai->Initialized());
}

void CKAIK::Serialize(creg::ISerializer* s) {
	if (ai->Initialized()) {
		for (int i = 0; i < MAX_UNITS; i++) {
			CUNIT* u = ai->GetUnit(i);

			if (ai->ccb->GetUnitDef(i) != NULL) {
				// do not save non-existing units
				s->SerializeObjectInstance(u, u->GetClass());

				if (!s->IsWriting()) {
					u->uid = i;
				}
			} else if (!s->IsWriting()) {
				u->uid = i;
			}
		}

		s->SerializeObjectInstance(ai, ai->GetClass());
	}
}

void CKAIK::InitAI(IGlobalAICallback* callback, int team) {
	ai = new AIClasses(callback); ai->Init();

	std::string verMsg =
		std::string(AI_VERSION) + (ai->Initialized()? " initialized successfully!": " failed to initialize");
	std::string logMsg =
		ai->Initialized()? ("logging events to " + ai->GetLogger()->GetLogName()): "not logging events";

	ai->cb->SendTextMsg(verMsg.c_str(), 0);
	ai->cb->SendTextMsg(logMsg.c_str(), 0);
	ai->cb->SendTextMsg(AI_CREDITS, 0);
}

void CKAIK::ReleaseAI() {
	delete ai; ai = NULL;
}



void CKAIK::UnitCreated(int unitID, int builderID) {
	if (ai->Initialized()) {
		ai->uh->UnitCreated(unitID);
		ai->econTracker->UnitCreated(unitID);
	}
}

void CKAIK::UnitFinished(int unitID) {
	if (ai->Initialized()) {
		ai->econTracker->UnitFinished(unitID);

		if (ai->cb->GetUnitDef(unitID) != NULL) {
			// let attackhandler handle cat_g_attack units
			if (GCAT(unitID) == CAT_G_ATTACK) {
				ai->ah->AddUnit(unitID);
			} else {
				ai->uh->IdleUnitAdd(unitID, ai->cb->GetCurrentFrame());
			}

			ai->uh->BuildTaskRemove(unitID);
		}
	}
}

void CKAIK::UnitDestroyed(int unitID, int attackerUnitID) {
	if (ai->Initialized()) {
		attackerUnitID = attackerUnitID;
		ai->econTracker->UnitDestroyed(unitID);

		if (ai->GetUnit(unitID)->groupID != -1) {
			ai->ah->UnitDestroyed(unitID);
		}

		ai->uh->UnitDestroyed(unitID);
	}
}

void CKAIK::UnitIdle(int unitID) {
	if (ai->Initialized()) {
		if (ai->GetUnit(unitID)->isDead) {
			return;
		}

		if (ai->uh->lastCapturedUnitFrame == ai->cb->GetCurrentFrame()) {
			if (unitID == ai->uh->lastCapturedUnitID) {
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
		if (GCAT(unitID) == CAT_G_ATTACK && ai->GetUnit(unitID)->groupID != -1) {
			// attackHandler->UnitIdle(unit);
		} else {
			ai->uh->IdleUnitAdd(unitID, ai->cb->GetCurrentFrame());
		}
	}
}

void CKAIK::UnitDamaged(int unitID, int attackerID, float damage, float3 dir) {
	if (ai->GetUnit(unitID)->isDead) {
		return;
	}

	if (ai->Initialized()) {
		attackerID = attackerID;
		dir = dir;

		ai->econTracker->UnitDamaged(unitID, damage);
	}
}

void CKAIK::UnitMoveFailed(int unitID) {
	if (ai->Initialized()) {
		ai->uh->UnitMoveFailed(unitID);
	}
}



void CKAIK::EnemyEnterLOS(int enemyUnitID) {
	if (ai->Initialized()) {
		enemyUnitID = enemyUnitID;
	}
}

void CKAIK::EnemyLeaveLOS(int enemyUnitID) {
	if (ai->Initialized()) {
		enemyUnitID = enemyUnitID;
	}
}

void CKAIK::EnemyEnterRadar(int enemyUnitID) {
	if (ai->Initialized()) {
		enemyUnitID = enemyUnitID;
	}
}

void CKAIK::EnemyLeaveRadar(int enemyUnitID) {
	if (ai->Initialized()) {
		enemyUnitID = enemyUnitID;
	}
}

void CKAIK::EnemyDestroyed(int enemyUnitID, int attackerUnitID) {
	if (ai->Initialized()) {
		ai->dgunConHandler->NotifyEnemyDestroyed(enemyUnitID, attackerUnitID);
		ai->tm->EnemyDestroyed(enemyUnitID, attackerUnitID);
	}
}

void CKAIK::EnemyDamaged(int enemyUnitID, int attackerUnitID, float damage, float3 dir) {
	if (ai->Initialized()) {
		ai->tm->EnemyDamaged(enemyUnitID, attackerUnitID);

		damage = damage;
		dir = dir;
	}
}

void CKAIK::EnemyCreated(int enemyUnitID) {
	if (ai->Initialized()) {
		ai->tm->EnemyCreated(enemyUnitID);
	}
}
void CKAIK::EnemyFinished(int enemyUnitID) {
	if (ai->Initialized()) {
		ai->tm->EnemyFinished(enemyUnitID);
	}
}



void CKAIK::GotChatMsg(const char* msg, int player) {
	if (ai->Initialized()) {
		player = player;

		if ((msg = strstr(msg, "KAIK::")) == NULL) {
			return;
		}

		if ((msg = strstr(msg, "ThreatMap::DBG")) != NULL) {
			ai->tm->ToggleVisOverlay();
		}
	}
}


int CKAIK::HandleEvent(int msg, const void* data) {
	if (ai->Initialized()) {
		switch (msg) {
			case AI_EVENT_UNITGIVEN:
			case AI_EVENT_UNITCAPTURED: {
				const ChangeTeamEvent* cte = (const ChangeTeamEvent*) data;

				const int myAllyTeamId = ai->cb->GetMyAllyTeam();
				const bool oldEnemy = !ai->cb->IsAllied(myAllyTeamId, ai->cb->GetTeamAllyTeam(cte->oldteam));
				const bool newEnemy = !ai->cb->IsAllied(myAllyTeamId, ai->cb->GetTeamAllyTeam(cte->newteam));

				if (oldEnemy && !newEnemy) {
					// unit changed from an enemy to an allied team
					// we got a new friend! :)
					EnemyDestroyed(cte->unit, -1);
				} else if (!oldEnemy && newEnemy) {
					// unit changed from an ally to an enemy team
					// we lost a friend! :(
					EnemyCreated(cte->unit);

					if (!ai->cb->UnitBeingBuilt(cte->unit)) {
						EnemyFinished(cte->unit);
					}
				}

				if (cte->oldteam == ai->cb->GetMyTeam()) {
					// we lost a unit
					UnitDestroyed(cte->unit, -1);

					// FIXME: multiple units given during same frame?
					ai->uh->lastCapturedUnitFrame = ai->cb->GetCurrentFrame();
					ai->uh->lastCapturedUnitID = cte->unit;
				} else if (cte->newteam == ai->cb->GetMyTeam()) {
					// we have a new unit
					UnitCreated(cte->unit, -1);

					if (!ai->cb->UnitBeingBuilt(cte->unit)) {
						UnitFinished(cte->unit);
						ai->uh->IdleUnitAdd(cte->unit, ai->cb->GetCurrentFrame());
					}
				}
			} break;
		}
	}

	return 0;
}




void CKAIK::Update() {
	if (ai->Initialized()) {
		const int frame = ai->cb->GetCurrentFrame();

		ai->econTracker->frameUpdate(frame);
		ai->dgunConHandler->Update(frame);

		if ((frame - ai->InitFrame()) == 1) {
			// ai->tm->Init();
			ai->dm->Init();
		}

		ai->bu->Update(frame);
		ai->uh->IdleUnitUpdate(frame);

		ai->ah->Update(frame);
		ai->uh->MMakerUpdate(frame);
		ai->ct->Update(frame);
	}
}
