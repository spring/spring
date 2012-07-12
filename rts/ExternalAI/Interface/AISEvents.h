/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_S_EVENTS_H
#define AI_S_EVENTS_H

// IMPORTANT NOTE: external systems parse this file,
// so DO NOT CHANGE the style and format it uses without
// major though in advance, and deliberation with hoijui!

#ifdef	__cplusplus
extern "C" {
#endif

// NOTE structs should not be empty (C90), so add a useless member if needed

#include "SSkirmishAICallback.h"

/**
 * Each event type can be identified through a unique ID,
 * which we call event topic.
 * Events are sent from the engine to AIs.
 *
 * Note: Do NOT change the values assigned to these topics,
 * as this would be bad for inter-version compatibility.
 * You should always append new event topics at the end of this list,
 * and adjust NUM_EVENTS.
 *
 * @see SSkirmishAILibrary.handleEvent()
 */
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
	EVENT_LOAD                         = 23,
	EVENT_SAVE                         = 24,
	EVENT_ENEMY_CREATED                = 25,
	EVENT_ENEMY_FINISHED               = 26,
	EVENT_LUA_MESSAGE                  = 27,
};
const int NUM_EVENTS = 28;



#define AIINTERFACE_EVENTS_ABI_VERSION     ( \
		  sizeof(struct SInitEvent) \
		+ sizeof(struct SReleaseEvent) \
		+ sizeof(struct SUpdateEvent) \
		+ sizeof(struct SMessageEvent) \
		+ sizeof(struct SUnitCreatedEvent) \
		+ sizeof(struct SUnitFinishedEvent) \
		+ sizeof(struct SUnitIdleEvent) \
		+ sizeof(struct SUnitMoveFailedEvent) \
		+ sizeof(struct SUnitDamagedEvent) \
		+ sizeof(struct SUnitDestroyedEvent) \
		+ sizeof(struct SUnitGivenEvent) \
		+ sizeof(struct SUnitCapturedEvent) \
		+ sizeof(struct SEnemyEnterLOSEvent) \
		+ sizeof(struct SEnemyLeaveLOSEvent) \
		+ sizeof(struct SEnemyEnterRadarEvent) \
		+ sizeof(struct SEnemyLeaveRadarEvent) \
		+ sizeof(struct SEnemyDamagedEvent) \
		+ sizeof(struct SEnemyDestroyedEvent) \
		+ sizeof(struct SWeaponFiredEvent) \
		+ sizeof(struct SPlayerCommandEvent) \
		+ sizeof(struct SCommandFinishedEvent) \
		+ sizeof(struct SSeismicPingEvent) \
		+ sizeof(struct SLoadEvent) \
		+ sizeof(struct SSaveEvent) \
		+ sizeof(struct SEnemyCreatedEvent) \
		+ sizeof(struct SEnemyFinishedEvent) \
		+ sizeof(struct SLuaMessageEvent) \
		)

/**
 * This AI event initializes a Skirmish AI instance.
 * It is sent only once per AI instance and game, as the very first event.
 */
struct SInitEvent {
	int skirmishAIId;
	const struct SSkirmishAICallback* callback;
}; //$ EVENT_INIT

/**
 * This AI event tells a Skirmish AI instance, that it is no longer needed. It
 * can be used to free memory or do other cleanup work.
 * It is sent only once per AI instance and game, as the very last event.
 * Values description for reason:
 * 0: unspecified
 * 1: game ended
 * 2: team died
 * 3: AI killed
 * 4: AI crashed
 * 5: AI failed to init
 * 6: connection lost
 * 7: other reason
 */
struct SReleaseEvent {
	int reason;
}; //$ EVENT_RELEASE

/**
 * This AI event is sent once per game frame, which is about 30 times per second
 * by default.
 */
struct SUpdateEvent {
	int frame;
}; //$ EVENT_UPDATE

/**
 * This AI event is a notification about a chat message sent by one of the
 * participants of this game, which may be a player or an AI, including this AI.
 */
struct SMessageEvent {
	int player;
	const char* message;
}; //$ EVENT_MESSAGE

/**
 * This AI event triggers whenever any message
 * is sent by a Lua widget or unsynced gadget.
 */
struct SLuaMessageEvent {
	const char* inData;
}; //$ EVENT_LUA_MESSAGE

/**
 * This AI event is sent whenever a unit of this team is created, and contains
 * the created unit. Usually, the unit has only 1 HP at this time, and consists
 * only of a nano frame (-> will not accept commands yet).
 * See also the unit-finished event.
 */
struct SUnitCreatedEvent {
	int unit;
	int builder;
}; //$ EVENT_UNIT_CREATED INTERFACES:Unit(unit),UnitLifeState()

/**
 * This AI event is sent whenever a unit is fully built, and contains the
 * finished unit. Usually, the unit has full health at this time, and is ready
 * to accept commands.
 * See also the unit-created event.
 */
struct SUnitFinishedEvent {
	int unit;
}; //$ EVENT_UNIT_FINISHED INTERFACES:Unit(unit),UnitLifeState()

/**
 * This AI event is sent when a unit finished processing a command or just
 * finished building, and it has currently no commands in it's queue.
 */
struct SUnitIdleEvent {
	int unit;
}; //$ EVENT_UNIT_IDLE INTERFACES:Unit(unit)

/**
 * This AI event is sent when a unit received a move command and is not able to
 * fullfill it. Reasons for this are:
 * - The unit is not able to move
 * - The path to the target location is blocked
 * - The unit can not move on the terain of the target location (for example,
 *   the target is on land, and the unit is a ship)
 */
struct SUnitMoveFailedEvent {
	int unit;
}; //$ EVENT_UNIT_MOVE_FAILED INTERFACES:Unit(unit)

/**
 * This AI event is sent when a unit was damaged. It contains the attacked unit,
 * the attacking unit, the ammount of damage and the direction from where the
 * damage was inflickted. In case of a laser weapon, the direction will point
 * directly from the attacker to the attacked unit, while with artillery it will
 * rather be from somewhere up in the sky to the attacked unit.
 * See also the unit-destroyed event.
 */
struct SUnitDamagedEvent {
	int unit;
	/**
	 * may be -1, which means no attacker was directly involveld,
	 * or the attacker is not visible and cheat events are off
	 */
	int attacker;
	float damage;
	float* dir_posF3;
	int weaponDefId;
	/// if true, then damage is paralyzation damage, otherwise it is real damage
	bool paralyzer;
}; //$ EVENT_UNIT_DAMAGED INTERFACES:Unit(unit)

/**
 * This AI event is sent when a unit was destroyed; see also the unit-damaged
 * event.
 */
struct SUnitDestroyedEvent {
	int unit;
	/**
	 * may be -1, which means no attacker was directly involveld,
	 * or the attacker is not visible and cheat events are off
	 */
	int attacker;
}; //$ EVENT_UNIT_DESTROYED INTERFACES:Unit(unit),UnitLifeState()

/**
 * This AI event is sent when a unit changed from one team to another,
 * either because the old owner gave it to the new one, or because the
 * new one took it from the old one; see the /take command.
 * Both giving and receiving team will get this event.
 */
struct SUnitGivenEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
}; //$ EVENT_UNIT_GIVEN INTERFACES:Unit(unitId),UnitLifeState(),UnitTeamChange(oldTeamId,newTeamId)

/**
 * This AI event is sent when a unit changed from one team to an other through
 * capturing.
 * Both giving and receiving team will get this event.
 */
struct SUnitCapturedEvent {
	int unitId;
	int oldTeamId;
	int newTeamId;
}; //$ EVENT_UNIT_CAPTURED INTERFACES:Unit(unitId),UnitLifeState(),UnitTeamChange(oldTeamId,newTeamId)

/**
 * This AI event is sent when an enemy unit entered the LOS of this team.
 */
struct SEnemyEnterLOSEvent {
	int enemy;
}; //$ EVENT_ENEMY_ENTER_LOS INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent when an enemy unit left the LOS of this team.
 */
struct SEnemyLeaveLOSEvent {
	int enemy;
}; //$ EVENT_ENEMY_LEAVE_LOS INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent when an enemy unit entered the radar covered area of
 * this team.
 */
struct SEnemyEnterRadarEvent {
	int enemy;
}; //$ EVENT_ENEMY_ENTER_RADAR INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent when an enemy unit left the radar covered area of this
 * team.
 */
struct SEnemyLeaveRadarEvent {
	int enemy;
}; //$ EVENT_ENEMY_LEAVE_RADAR INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent when an enemy unit was damaged. It contains the
 * attacked unit, the attacking unit, the ammount of damage and the direction
 * from where the damage was inflickted. In case of a laser weapon, the
 * direction will point directly from the attacker to the attacked unit, while
 * with artillery it will rather be from somewhere up in the sky to the attacked
 * unit.
 * See also the enemy-destroyed event.
 */
struct SEnemyDamagedEvent {
	int enemy;
	/**
	 * may be -1, which means no attacker was directly involveld,
	 * or the attacker is not allied with the team receiving this event
	 */
	int attacker;
	float damage;
	float* dir_posF3;
	int weaponDefId;
	/// if true, then damage is paralyzation damage, otherwise it is real damage
	bool paralyzer;
}; //$ EVENT_ENEMY_DAMAGED INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent when an enemy unit was destroyed; see also the
 * enemy-damaged event.
 */
struct SEnemyDestroyedEvent {
	int enemy;
	/**
	 * may be -1, which means no attacker was directly involveld,
	 * or the attacker is not allied with the team receiving this event
	 */
	int attacker;
}; //$ EVENT_ENEMY_DESTROYED INTERFACES:Unit(enemy),Enemy(enemy),UnitLifeState()

/**
 * This AI event is sent when certain weapons are fired.
 * For performance reasons, it is not possible to send this event
 * for all weapons. Therefore, it is currently only sent for manuall-fire
 * weapons like for example the TA Commanders D-Gun or the Nuke.
 */
struct SWeaponFiredEvent {
	int unitId;
	int weaponDefId;
}; //$ EVENT_WEAPON_FIRED INTERFACES:Unit(unitId)

/**
 * This AI event is sent when a user gives a command to one or multiple units
 * belonging to a team controlled by the AI.
 * For more info about the given commands, please use the
 * Unit.getCurrentCommands() method of the callback.
 */
struct SPlayerCommandEvent {
	int* unitIds;
	int unitIds_size;
	/// see COMMAND_* defines in AISCommands.h
	int commandTopicId;
	/// Id of the player that issued the command
	int playerId;
}; //$ EVENT_PLAYER_COMMAND

/**
 * This AI event is sent when a unit finished processing a command.
 * @param commandId      used on asynchronous commands only (is -1 for regular commands).
 *                       this allows the AI to identify a possible result event,
 *                       which would come with the same id
 * @param commandTopicId unique identifier of a command
 *                       (see COMMAND_* defines in AISCommands.h)
 * @see callback.handleCommand(..., int commandId, ...)
 */
struct SCommandFinishedEvent {
	int unitId;
	int commandId;
	int commandTopicId;
}; //$ EVENT_COMMAND_FINISHED INTERFACES:Unit(unitId)

/**
 * This AI event is sent when a unit movement is detected by means of a seismic
 * event. A seismic event means erruption/movement/shakings of the ground. This
 * can be detected by only by special units usually, eg by the seismic detector
 * building in Balanced Annihilation.
 */
struct SSeismicPingEvent {
	float* pos_posF3;
	float strength;
}; //$ EVENT_SEISMIC_PING

/**
 * This AI event is sent when the AI should be loading its full state from a
 * file.
 */
struct SLoadEvent {
	/// Absolute file path, should be treated read-only
	const char* file;
}; //$ EVENT_LOAD INTERFACES:LoadSave(file)

/**
 * This AI event is sent when the AI should be saving its full state to a file.
 */
struct SSaveEvent {
	/// Absolute file path, writable
	const char* file;
}; //$ EVENT_SAVE INTERFACES:LoadSave(file)

/**
 * This AI event is sent whenever a unit of an enemy team is created,
 * and contains the created unit. Usually, the unit has only 1 HP at this time,
 * and consists only of a nano frame.
 * See also the enemy-finished event.
 */
struct SEnemyCreatedEvent {
	int enemy;
}; //$ EVENT_ENEMY_CREATED INTERFACES:Unit(enemy),Enemy(enemy)

/**
 * This AI event is sent whenever an enemy unit is fully built, and contains the
 * finished unit. Usually, the unit has full health at this time.
 * See also the unit-created event.
 */
struct SEnemyFinishedEvent {
	int enemy;
}; //$ EVENT_ENEMY_FINISHED INTERFACES:Unit(enemy),Enemy(enemy)

#ifdef	__cplusplus
} // extern "C"
#endif

#endif // AI_S_EVENTS_H
