/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <memory>

#include "AIAI.h"
#include "IGlobalAI.h"
#include "Event/AIEvents.h"
#include "ExternalAI/Interface/AISEvents.h"
#include "System/Util.h"

springLegacyAI::CAIAI::CAIAI(springLegacyAI::IGlobalAI* gAI):
	ai(gAI),
	globalAICallback(nullptr) {
}

springLegacyAI::CAIAI::~CAIAI() {
	SafeDelete(ai);
	SafeDelete(globalAICallback);
}


int springLegacyAI::CAIAI::handleEvent(int topic, const void* data) {

	int ret = -1; // if this values remains, something went wrong

	if (ai == nullptr)
		return ret;

	std::unique_ptr<CAIEvent> e;

	// create the legacy C++ event
	switch (topic) {
		case EVENT_INIT: {
			e.reset(new CAIInitEvent(*static_cast<const SInitEvent*>(data)));

			// should not ever be needed, but does not hurt either
			SafeDelete(globalAICallback);

			globalAICallback = (static_cast<CAIInitEvent*>(e.get()))->GetWrappedGlobalAICallback();
		} break;
		case EVENT_RELEASE: {
			e.reset(new CAIReleaseEvent(*static_cast<const SReleaseEvent*>(data)));
		} break;
		case EVENT_UPDATE: {
			e.reset(new CAIUpdateEvent(*static_cast<const SUpdateEvent*>(data)));
		} break;

		case EVENT_MESSAGE: {
			e.reset(new CAIChatMessageEvent(*static_cast<const SMessageEvent*>(data)));
		} break;
		case EVENT_LUA_MESSAGE: {
			e.reset(new CAILuaMessageEvent(*static_cast<const SLuaMessageEvent*>(data)));
		} break;

		case EVENT_UNIT_CREATED: {
			e.reset(new CAIUnitCreatedEvent(*static_cast<const SUnitCreatedEvent*>(data)));
		} break;
		case EVENT_UNIT_FINISHED: {
			e.reset(new CAIUnitFinishedEvent(*static_cast<const SUnitFinishedEvent*>(data)));
		} break;
		case EVENT_UNIT_IDLE: {
			e.reset(new CAIUnitIdleEvent(*static_cast<const SUnitIdleEvent*>(data)));
		} break;
		case EVENT_UNIT_MOVE_FAILED: {
			e.reset(new CAIUnitMoveFailedEvent(*static_cast<const SUnitMoveFailedEvent*>(data)));
		} break;
		case EVENT_UNIT_DAMAGED: {
			e.reset(new CAIUnitDamagedEvent(*static_cast<const SUnitDamagedEvent*>(data)));
		} break;
		case EVENT_UNIT_DESTROYED: {
			e.reset(new CAIUnitDestroyedEvent(*static_cast<const SUnitDestroyedEvent*>(data)));
		} break;
		case EVENT_UNIT_GIVEN: {
			e.reset(new CAIUnitGivenEvent(*static_cast<const SUnitGivenEvent*>(data)));
		} break;
		case EVENT_UNIT_CAPTURED: {
			e.reset(new CAIUnitCapturedEvent(*static_cast<const SUnitCapturedEvent*>(data)));
		} break;

		case EVENT_ENEMY_ENTER_LOS: {
			e.reset(new CAIEnemyEnterLOSEvent(*static_cast<const SEnemyEnterLOSEvent*>(data)));
		} break;
		case EVENT_ENEMY_LEAVE_LOS: {
			e.reset(new CAIEnemyLeaveLOSEvent(*static_cast<const SEnemyLeaveLOSEvent*>(data)));
		} break;
		case EVENT_ENEMY_ENTER_RADAR: {
			e.reset(new CAIEnemyEnterRadarEvent(*static_cast<const SEnemyEnterRadarEvent*>(data)));
		} break;
		case EVENT_ENEMY_LEAVE_RADAR: {
			e.reset(new CAIEnemyLeaveRadarEvent(*static_cast<const SEnemyLeaveRadarEvent*>(data)));
		} break;
		case EVENT_ENEMY_DAMAGED: {
			e.reset(new CAIEnemyDamagedEvent(*static_cast<const SEnemyDamagedEvent*>(data)));
		} break;
		case EVENT_ENEMY_DESTROYED: {
			e.reset(new CAIEnemyDestroyedEvent(*static_cast<const SEnemyDestroyedEvent*>(data)));
		} break;
		case EVENT_WEAPON_FIRED:{
			e.reset(new CAIWeaponFiredEvent(*static_cast<const SWeaponFiredEvent*>(data)));
		} break;
		case EVENT_PLAYER_COMMAND:  {
			e.reset(new CAIPlayerCommandEvent(*static_cast<const SPlayerCommandEvent*>(data)));
		} break;
		case EVENT_SEISMIC_PING:{
			e.reset(new CAISeismicPingEvent(*static_cast<const SSeismicPingEvent*>(data)));
		} break;
		case EVENT_ENEMY_CREATED: {
			e.reset(new CAIEnemyCreatedEvent(*static_cast<const SEnemyCreatedEvent*>(data)));
		} break;
		case EVENT_ENEMY_FINISHED: {
			e.reset(new CAIEnemyFinishedEvent(*static_cast<const SEnemyFinishedEvent*>(data)));
		} break;

		case EVENT_NULL:
		default: {
			e.reset(new CAINullEvent());
		} break;
	}

	try {
		// handle the event
		e->Run(*ai, globalAICallback);
		ret = 0;
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

	return ret;
}

