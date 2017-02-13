/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <memory>

#include "AIAI.h"

#include "ExternalAI/Interface/AISEvents.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/AISEvents.h"

#include "Event/AIEvent.h"
#include "Event/AINullEvent.h"

#include "Event/AIInitEvent.h"
#include "Event/AIReleaseEvent.h"
#include "Event/AIUpdateEvent.h"

#include "Event/AIChatMessageEvent.h"
#include "Event/AILuaMessageEvent.h"

#include "Event/AIUnitCreatedEvent.h"
#include "Event/AIUnitFinishedEvent.h"
#include "Event/AIUnitIdleEvent.h"
#include "Event/AIUnitMoveFailedEvent.h"
#include "Event/AIUnitDamagedEvent.h"
#include "Event/AIUnitDestroyedEvent.h"
#include "Event/AIUnitGivenEvent.h"
#include "Event/AIUnitCapturedEvent.h"

#include "Event/AIEnemyCreatedEvent.h"
#include "Event/AIEnemyFinishedEvent.h"
#include "Event/AIEnemyEnterLOSEvent.h"
#include "Event/AIEnemyLeaveLOSEvent.h"
#include "Event/AIEnemyEnterRadarEvent.h"
#include "Event/AIEnemyLeaveRadarEvent.h"
#include "Event/AIEnemyDamagedEvent.h"
#include "Event/AIEnemyDestroyedEvent.h"

#include "Event/AIWeaponFiredEvent.h"
#include "Event/AIPlayerCommandEvent.h"
#include "Event/AISeismicPingEvent.h"

int springLegacyAI::CAIAI::handleEvent(int topic, const void* data) {
	int ret = -1; // if this values remains, something went wrong

	if (ai.get() == nullptr)
		return ret;

	std::unique_ptr<CAIEvent> e;

	// create the legacy C++ event
	switch (topic) {
		case EVENT_INIT: {
			e.reset(new CAIInitEvent(*static_cast<const SInitEvent*>(data)));

			// type(globalAICallback) := IGlobalAICallback* (CAIGlobalAICallback*), kept here
			// type(globalAICallback->GetAICallback()) := IAICallback* (CAICallback*), kept in SkirmishAIWrapper
			const SInitEvent* sie = static_cast<const SInitEvent*>(data);
			const SSkirmishAICallback* cb = sie->callback;

			assert(globalAICallback.get() == nullptr);
			globalAICallback.reset(new CAIGlobalAICallback(cb, sie->skirmishAIId));

			ai->InitAI(globalAICallback.get(), cb->SkirmishAI_getTeamId(sie->skirmishAIId));
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
		e->Run(*ai, globalAICallback.get());
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

