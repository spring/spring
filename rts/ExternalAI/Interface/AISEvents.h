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

	@author Nicolas Wu
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef _AISEVENTS_H
#define _AISEVENTS_H

// IMPORTANT NOTE: external systems parse this file,
// so DO NOT CHANGE the style and format it uses without
// major though in advance, and deliberation with hoijui!

#ifdef	__cplusplus
extern "C" {
#endif

#include "SAIFloat3.h"
#include "SAICallback.h"

enum EventTopic {
	EVENT_NULL                         =  0,
	EVENT_INIT                         =  1,
	EVENT_RELEASE                      =  2,
	EVENT_UPDATE                       =  3,
	EVENT_MESSAGE                      =  4,
	EVENT_UNIT_CREATED                 =  5,
	EVENT_UNIT_FINISHED                =  6,
	EVENT_UNIT_IDLE                    =  7,
	EVENT_UNIT_MOVE_FAILED             =  8,
	EVENT_UNIT_DAMAGED                 =  9,
	EVENT_UNIT_DESTROYED               = 10,
	EVENT_UNIT_GIVEN                   = 11,
	EVENT_UNIT_CAPTURED                = 12,
	EVENT_ENEMY_ENTER_LOS              = 13,
	EVENT_ENEMY_LEAVE_LOS              = 14,
	EVENT_ENEMY_ENTER_RADAR            = 15,
	EVENT_ENEMY_LEAVE_RADAR            = 16,
	EVENT_ENEMY_DAMAGED                = 17,
	EVENT_ENEMY_DESTROYED              = 18,
	EVENT_WEAPON_FIRED                 = 19,
	EVENT_PLAYER_COMMAND               = 20,
	EVENT_SEISMIC_PING                 = 21,
	EVENT_COMMAND_FINISHED             = 22,
};
const unsigned int NUM_EVENTS          = 23;


struct SInitEvent {
	int team;
	struct SAICallback* callback;
//	unsigned int sizeInfo;
//	const struct InfoItem* info;
	unsigned int infoSize;
	const char** infoKeys;
	const char** infoValues;
	unsigned int optionsSize;
	const char** optionsKeys;
	const char** optionsValues;
}; // EVENT_INIT

struct SReleaseEvent {
}; // EVENT_RELEASE

struct SUpdateEvent {
	int frame;
}; // EVENT_UPDATE

struct SMessageEvent {
	int player;
	const char* message;
}; // EVENT_MESSAGE

struct SUnitCreatedEvent {
	int unit;
}; // EVENT_UNIT_CREATED

struct SUnitFinishedEvent {
	int unit;
}; // EVENT_UNIT_FINISHED

struct SUnitIdleEvent {
	int unit;
}; // EVENT_UNIT_IDLE

struct SUnitMoveFailedEvent {
	int unit;
}; // EVENT_UNIT_MOVE_FAILED

struct SUnitDamagedEvent {
	int unit;
	int attacker;
	float damage;
	struct SAIFloat3 dir;
}; // EVENT_UNIT_DAMAGED

struct SUnitDestroyedEvent {
	int unit;
	int attacker;
}; // EVENT_UNIT_DESTROYED

struct SUnitGivenEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
}; // EVENT_UNIT_GIVEN

struct SUnitCapturedEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
}; // EVENT_UNIT_CAPTURED

struct SEnemyEnterLOSEvent {
	int enemy;
}; // EVENT_ENEMY_ENTER_LOS

struct SEnemyLeaveLOSEvent {
	int enemy;
}; // EVENT_ENEMY_LEAVE_LOS

struct SEnemyEnterRadarEvent {
	int enemy;
}; // EVENT_ENEMY_ENTER_RADAR

struct SEnemyLeaveRadarEvent {
	int enemy;
}; // EVENT_ENEMY_LEAVE_RADAR

struct SEnemyDamagedEvent {
	int enemy;
	int attacker;
	float damage;
	struct SAIFloat3 dir;
}; // EVENT_ENEMY_DAMAGED

struct SEnemyDestroyedEvent {
	int enemy;
	int attacker;
}; // EVENT_ENEMY_DESTROYED

struct SWeaponFiredEvent {
	int unitId;
	int weaponDefId;
}; // EVENT_WEAPON_FIRED

struct SPlayerCommandEvent {
	int* unitIds;
	int numUnitIds;
	int commandTopic; // see AISCommands.h COMMAND_* defines
	void* commandData; // see AISCommands.h S*Command structs
	int playerId;
}; // EVENT_PLAYER_COMMAND

struct SCommandFinishedEvent {
	int unitId;
	int commandTopicId;
}; // EVENT_COMMAND_FINISHED

struct SSeismicPingEvent {
	struct SAIFloat3 pos;
	float strength;
}; // EVENT_SEISMIC_PING

#ifdef	__cplusplus
}	// extern "C"
#endif

#endif	// _AISEVENTS_H
