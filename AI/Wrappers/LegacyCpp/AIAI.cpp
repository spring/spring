/*
	Copyright 2008  Nicolas Wu

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AIAI.h"
#include "Event/AIEvents.h"
#include "ExternalAI/IGlobalAI.h"
#include "ExternalAI/Interface/AISEvents.h"

CAIAI::CAIAI() : team(-1), ai(NULL) {}

CAIAI::CAIAI(int team, IGlobalAI* ai) : team(team), ai(ai) {}


int CAIAI::handleEvent(int topic, const void* data) {

	static IGlobalAICallback* wrappedGlobalAICallback = NULL;

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
						new CAIInitEvent(*((const SInitEvent*) data));
				// The following two lines should not ever be needed,
				// but they do not hurt either.
				delete wrappedGlobalAICallback;
				wrappedGlobalAICallback = NULL;
				wrappedGlobalAICallback = initE->GetWrappedGlobalAICallback();
				e = initE;
				break;
			}
			case EVENT_RELEASE: {
				delete wrappedGlobalAICallback;
				wrappedGlobalAICallback = NULL;
				e = new CAIReleaseEvent(*((const SReleaseEvent*) data));
				break;
			}
			case EVENT_UPDATE: {
				e = new CAIUpdateEvent(*((const SUpdateEvent*) data));
				break;
			}
			case EVENT_MESSAGE: {
				e = new CAIMessageEvent(*((const SMessageEvent*) data));
				break;
			}
			case EVENT_UNIT_CREATED: {
				e = new CAIUnitCreatedEvent(*((const SUnitCreatedEvent*) data));
				break;
			}
			case EVENT_UNIT_FINISHED: {
				e = new CAIUnitFinishedEvent(
						*((const SUnitFinishedEvent*) data));
				break;
			}
			case EVENT_UNIT_IDLE: {
				e = new CAIUnitIdleEvent(*((const SUnitIdleEvent*) data));
				break;
			}
			case EVENT_UNIT_MOVE_FAILED: {
				e = new CAIUnitMoveFailedEvent(
						*((const SUnitMoveFailedEvent*) data));
				break;
			}
			case EVENT_UNIT_DAMAGED: {
				e = new CAIUnitDamagedEvent(*((const SUnitDamagedEvent*) data));
				break;
			}
			case EVENT_UNIT_DESTROYED: {
				e = new CAIUnitDestroyedEvent(
						*((const SUnitDestroyedEvent*) data));
				break;
			}
			case EVENT_UNIT_GIVEN: {
				e = new CAIUnitGivenEvent(*((const SUnitGivenEvent*) data));
				break;
			}
			case EVENT_UNIT_CAPTURED: {
				e = new CAIUnitCapturedEvent(
						*((const SUnitCapturedEvent*) data));
				break;
			}
			case EVENT_ENEMY_ENTER_LOS: {
				e = new CAIEnemyEnterLOSEvent(
						*((const SEnemyEnterLOSEvent*) data));
				break;
			}
			case EVENT_ENEMY_LEAVE_LOS: {
				e = new CAIEnemyLeaveLOSEvent(
						*((const SEnemyLeaveLOSEvent*) data));
				break;
			}
			case EVENT_ENEMY_ENTER_RADAR: {
				e = new CAIEnemyEnterRadarEvent(
						*((const SEnemyEnterRadarEvent*) data));
				break;
			}
			case EVENT_ENEMY_LEAVE_RADAR: {
				e = new CAIEnemyLeaveRadarEvent(
						*((const SEnemyLeaveRadarEvent*) data));
				break;
			}
			case EVENT_ENEMY_DAMAGED: {
				e = new CAIEnemyDamagedEvent(
						*((const SEnemyDamagedEvent*) data));
				break;
			}
			case EVENT_ENEMY_DESTROYED: {
				e = new CAIEnemyDestroyedEvent(
						*((const SEnemyDestroyedEvent*) data));
				break;
			}
			case EVENT_WEAPON_FIRED:{
				e = new CAIWeaponFiredEvent(*((const SWeaponFiredEvent*) data));
				break;
			}
			case EVENT_PLAYER_COMMAND:  {
				e = new CAIPlayerCommandEvent(
						*((const SPlayerCommandEvent*) data));
				break;
			}
			case EVENT_SEISMIC_PING:{
				e = new CAISeismicPingEvent(*((const SSeismicPingEvent*) data));
				break;
			}
			default: {
				e = new CAINullEvent();
				break;
			}
		}

		try {
			// handle the event
			e->Run(*ai, wrappedGlobalAICallback);
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
