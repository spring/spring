/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIAI.h"
#include "Event/AIEvents.h"
#include "IGlobalAI.h"
#include "ExternalAI/Interface/AISEvents.h"

springLegacyAI::CAIAI::CAIAI(springLegacyAI::IGlobalAI* gAI):
	ai(gAI),
	globalAICallback(NULL) {
}

springLegacyAI::CAIAI::~CAIAI() {

	delete ai;
	ai = NULL;

	delete globalAICallback;
	globalAICallback = NULL;
}


int springLegacyAI::CAIAI::handleEvent(int topic, const void* data) {

	int ret = -1; // if this values remains, something went wrong

	if (ai != NULL) {
		CAIEvent* e = NULL;

		// create the legacy C++ event
		switch (topic) {
			case EVENT_NULL: {
				e = new CAINullEvent();
				break;
			}
			case EVENT_INIT: {
				CAIInitEvent* initE =
						new CAIInitEvent(*static_cast<const SInitEvent*>(data));
				// The following two lines should not ever be needed,
				// but they do not hurt either.
				delete globalAICallback;
				globalAICallback = NULL;
				globalAICallback = initE->GetWrappedGlobalAICallback();
				e = initE;
				break;
			}
			case EVENT_RELEASE: {
				e = new CAIReleaseEvent(
						*static_cast<const SReleaseEvent*>(data));
				break;
			}
			case EVENT_UPDATE: {
				e = new CAIUpdateEvent(
						*static_cast<const SUpdateEvent*>(data));
				break;
			}

			case EVENT_MESSAGE: {
				e = new CAIChatMessageEvent(
						*static_cast<const SMessageEvent*>(data));
			} break;
			case EVENT_LUA_MESSAGE: {
				e = new CAILuaMessageEvent(
						*static_cast<const SLuaMessageEvent*>(data));
			} break;

			case EVENT_UNIT_CREATED: {
				e = new CAIUnitCreatedEvent(
						*static_cast<const SUnitCreatedEvent*>(data));
				break;
			}
			case EVENT_UNIT_FINISHED: {
				e = new CAIUnitFinishedEvent(
						*static_cast<const SUnitFinishedEvent*>(data));
				break;
			}
			case EVENT_UNIT_IDLE: {
				e = new CAIUnitIdleEvent(
						*static_cast<const SUnitIdleEvent*>(data));
				break;
			}
			case EVENT_UNIT_MOVE_FAILED: {
				e = new CAIUnitMoveFailedEvent(
						*static_cast<const SUnitMoveFailedEvent*>(data));
				break;
			}
			case EVENT_UNIT_DAMAGED: {
				e = new CAIUnitDamagedEvent(
						*static_cast<const SUnitDamagedEvent*>(data));
				break;
			}
			case EVENT_UNIT_DESTROYED: {
				e = new CAIUnitDestroyedEvent(
						*static_cast<const SUnitDestroyedEvent*>(data));
				break;
			}
			case EVENT_UNIT_GIVEN: {
				e = new CAIUnitGivenEvent(
						*static_cast<const SUnitGivenEvent*>(data));
				break;
			}
			case EVENT_UNIT_CAPTURED: {
				e = new CAIUnitCapturedEvent(
						*static_cast<const SUnitCapturedEvent*>(data));
				break;
			}

			case EVENT_ENEMY_ENTER_LOS: {
				e = new CAIEnemyEnterLOSEvent(
						*static_cast<const SEnemyEnterLOSEvent*>(data));
				break;
			}
			case EVENT_ENEMY_LEAVE_LOS: {
				e = new CAIEnemyLeaveLOSEvent(
						*static_cast<const SEnemyLeaveLOSEvent*>(data));
				break;
			}
			case EVENT_ENEMY_ENTER_RADAR: {
				e = new CAIEnemyEnterRadarEvent(
						*static_cast<const SEnemyEnterRadarEvent*>(data));
				break;
			}
			case EVENT_ENEMY_LEAVE_RADAR: {
				e = new CAIEnemyLeaveRadarEvent(
						*static_cast<const SEnemyLeaveRadarEvent*>(data));
				break;
			}
			case EVENT_ENEMY_DAMAGED: {
				e = new CAIEnemyDamagedEvent(
						*static_cast<const SEnemyDamagedEvent*>(data));
				break;
			}
			case EVENT_ENEMY_DESTROYED: {
				e = new CAIEnemyDestroyedEvent(
						*static_cast<const SEnemyDestroyedEvent*>(data));
				break;
			}
			case EVENT_WEAPON_FIRED:{
				e = new CAIWeaponFiredEvent(
						*static_cast<const SWeaponFiredEvent*>(data));
				break;
			}
			case EVENT_PLAYER_COMMAND:  {
				e = new CAIPlayerCommandEvent(
						*static_cast<const SPlayerCommandEvent*>(data));
				break;
			}
			case EVENT_SEISMIC_PING:{
				e = new CAISeismicPingEvent(
						*static_cast<const SSeismicPingEvent*>(data));
				break;
			}
			case EVENT_ENEMY_CREATED: {
				e = new CAIEnemyCreatedEvent(
						*static_cast<const SEnemyCreatedEvent*>(data));
				break;
			}
			case EVENT_ENEMY_FINISHED: {
				e = new CAIEnemyFinishedEvent(
						*static_cast<const SEnemyFinishedEvent*>(data));
				break;
			}
			default: {
				e = new CAINullEvent();
				break;
			}
		}

		try {
			// handle the event
			e->Run(*ai, globalAICallback);
			ret = 0;
			delete e;
			e = NULL;
		} catch (int err) {
			if (err == 0) {
				// fail even if the error is set to 0,
				// because we got an exception
				ret = -3;
			} else {
				ret = err;
			}
		} catch (...) {
			ret = -2;
		}
	}

	return ret;
}
