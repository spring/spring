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

/*
#include "AI.h"
#include "Event/AIEvents.h"
#include "ExternalAI/Interface/AISEvents.h"

CAI::CAI() : team(0) { 
	
}


CAI::CAI(int team) : team(team) {
	
}

int CAI::handleEvent(int eventID, void* event) {
	CAIEvent* e;
	switch(eventID) {
		case EVENT_NULL:
			e = new CAINullEvent();
			break;
		case EVENT_INIT:
			e = new CAIInitEvent((SInitEvent*) event);
			break;
		case EVENT_UPDATE:
			e = new CAIUpdateEvent((SUpdateEvent*) event);
			break;
		case EVENT_MESSAGE:	
			e = new CAIMessageEvent((SMessageEvent*) event);
			break;
		case EVENT_UNIT_CREATED:
			e = new CAIUnitCreatedEvent((SUnitCreatedEvent*) event);
			break;
		case EVENT_UNIT_FINISHED:
			e = new CAIUnitFinishedEvent((SUnitFinishedEvent*) event);
			break;
		case EVENT_UNIT_IDLE:
			e = new CAIUnitIdleEvent((SUnitIdleEvent*) event);
			break;
		case EVENT_UNIT_MOVE_FAILED:
			e = new CAIUnitMoveFailedEvent((SUnitMoveFailedEvent*) event);
			break;
		case EVENT_UNIT_DAMAGED:
			e = new CAIUnitDamagedEvent((SUnitDamagedEvent*) event);
			break;
		case EVENT_UNIT_DESTROYED: 
			e = new CAIUnitDestroyedEvent((SUnitDestroyedEvent*) event);
			break;
		case EVENT_UNIT_GIVEN:
			e = new CAIUnitGivenEvent((SUnitGivenEvent*) event);
			break;
		case EVENT_UNIT_CAPTURED:
			e = new CAIUnitCapturedEvent((SUnitCapturedEvent*) event);
			break;
		case EVENT_ENEMY_ENTER_LOS:
			e = new CAIEnemyEnterLOSEvent((SEnemyEnterLOSEvent*) event);
			break;
		case EVENT_ENEMY_LEAVE_LOS:
			e = new CAIEnemyLeaveLOSEvent((SEnemyLeaveLOSEvent*) event);
			break;
		case EVENT_ENEMY_ENTER_RADAR:
			e = new CAIEnemyEnterRadarEvent((SEnemyEnterRadarEvent*) event);
			break;
		case EVENT_ENEMY_LEAVE_RADAR:
			e = new CAIEnemyLeaveRadarEvent((SEnemyLeaveRadarEvent*) event);
			break;
		case EVENT_ENEMY_DAMAGED:
			e = new CAIEnemyDamagedEvent((SEnemyDamagedEvent*) event);
			break;
		case EVENT_ENEMY_DESTROYED: 
			e = new CAIEnemyDestroyedEvent((SEnemyDestroyedEvent*) event);
			break;
		case EVENT_WEAPON_FIRED: 
			e = new CAIWeaponFiredEvent((SWeaponFiredEvent*) event);
			break;
		case EVENT_PLAYER_COMMAND: 
			e = new CAIPlayerCommandEvent((SPlayerCommandEvent*) event);
			break;
		case EVENT_SEISMIC_PING: 
			e = new CAISeismicPingEvent((SSeismicPingEvent*) event);
			break;
		default:
			e = new CAINullEvent();
			break;
	}
	e->run(this);
	delete e;
	return 0;
}
*/
