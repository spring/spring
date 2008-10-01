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

#ifndef AISEVENTS_H
#define AISEVENTS_H

#include "SAIFloat3.h"
#include "SAICallback.h"

#define EVENT_NULL				0
#define EVENT_INIT				1
#define EVENT_UPDATE			2
#define EVENT_MESSAGE			3
#define EVENT_UNIT_CREATED		4
#define EVENT_UNIT_FINISHED		5
#define EVENT_UNIT_IDLE			6
#define EVENT_UNIT_MOVE_FAILED	7
#define EVENT_UNIT_DAMAGED		8
#define EVENT_UNIT_DESTROYED	9
#define EVENT_UNIT_GIVEN		10
#define EVENT_UNIT_CAPTURED		11
#define EVENT_ENEMY_ENTER_LOS	12
#define EVENT_ENEMY_LEAVE_LOS	13
#define EVENT_ENEMY_ENTER_RADAR 14
#define EVENT_ENEMY_LEAVE_RADAR 15
#define EVENT_ENEMY_DAMAGED		16
#define EVENT_ENEMY_DESTROYED	17
#define EVENT_WEAPON_FIRED		18
#define EVENT_PLAYER_COMMAND	19
#define EVENT_SEISMIC_PING		20

#define NUM_EVENTS				21


struct SInitEvent {
	int team;
	SAICallback* c_callback;
};

struct SUpdateEvent {
	int frame;
};

struct SMessageEvent {
	int player;
	const char* message;
};

struct SUnitCreatedEvent {
	int unit;
};

struct SUnitFinishedEvent {
	int unit;
};

struct SUnitIdleEvent {
	int unit;
};

struct SUnitMoveFailedEvent {
	int unit;
};

struct SUnitDamagedEvent {
	int unit;
	int attacker;
	float damage;
	SAIFloat3 dir;
};

struct SUnitDestroyedEvent {
	int unit;
	int attacker;
};

struct SUnitGivenEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
};

struct SUnitCapturedEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
};

struct SEnemyEnterLOSEvent {
	int enemy;
};

struct SEnemyLeaveLOSEvent {
	int enemy;
};

struct SEnemyEnterRadarEvent {
	int enemy;
};

struct SEnemyLeaveRadarEvent {
	int enemy;
};

struct SEnemyDamagedEvent {
	int enemy;
	int attacker;
	float damage;
	SAIFloat3 dir;
};

struct SEnemyDestroyedEvent {
	int enemy;
	int attacker;
};

struct SWeaponFiredEvent {
	int unitId;
	int weaponDefId;
};

struct SPlayerCommandEvent {
	int* unitIds;
	int numUnitIds;
	int commandTopic; // see AISCommands.h COMMAND_* defines
	void* commandData; // see AISCommands.h S*Command structs
	int playerId;
};

struct SSeismicPingEvent {
	SAIFloat3 pos;
	float strength;
};

#endif /*AISEVENTS_H*/
