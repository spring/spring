/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

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

#ifndef _AISCOMMANDS_H
#define	_AISCOMMANDS_H

// IMPORTANT NOTE: external systems parse this file,
// so DO NOT CHANGE the style and format it uses without
// major though in advance, and deliberation with hoijui!

#include "aidefines.h"
#include "SAIFloat3.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define COMMAND_TO_ID_ENGINE -1

/**
 * Each command type can be identified through a unique ID,
 * which we call command topic.
 * Commands are usually sent from AIs to the engine,
 * but there are plans for the future to allow AI -> AI command scheduling.
 *
 * Note: Do NOT change the values assigned to these topics,
 * as this would be bad for inter-version compatibility.
 * You should always append new command topics at the end of this list,
 * and adjust NUM_CMD_TOPICS.
 *
 * @see SAICallback.handleCommand()
 */
enum CommandTopic {
	COMMAND_NULL                                  =  0,
	COMMAND_DRAWER_POINT_ADD                      =  1,
	COMMAND_DRAWER_LINE_ADD                       =  2,
	COMMAND_DRAWER_POINT_REMOVE                   =  3,
	COMMAND_SEND_START_POS                        =  4,
	COMMAND_CHEATS_SET_MY_HANDICAP                =  5,
	COMMAND_SEND_TEXT_MESSAGE                     =  6,
	COMMAND_SET_LAST_POS_MESSAGE                  =  7,
	COMMAND_SEND_RESOURCES                        =  8,
	COMMAND_SEND_UNITS                            =  9,
	COMMAND_UNUSED_0                              = 10, // unused
	COMMAND_UNUSED_1                              = 11, // unused
	COMMAND_GROUP_CREATE                          = 12,
	COMMAND_GROUP_ERASE                           = 13,
	COMMAND_GROUP_ADD_UNIT                        = 14,
	COMMAND_GROUP_REMOVE_UNIT                     = 15,
	COMMAND_PATH_INIT                             = 16,
	COMMAND_PATH_GET_APPROXIMATE_LENGTH           = 17,
	COMMAND_PATH_GET_NEXT_WAYPOINT                = 18,
	COMMAND_PATH_FREE                             = 19,
	COMMAND_CHEATS_GIVE_ME_RESOURCE               = 20,
	COMMAND_CALL_LUA_RULES                        = 21,
	COMMAND_DRAWER_ADD_NOTIFICATION               = 22,
	COMMAND_DRAWER_DRAW_UNIT                      = 23,
	COMMAND_DRAWER_PATH_START                     = 24,
	COMMAND_DRAWER_PATH_FINISH                    = 25,
	COMMAND_DRAWER_PATH_DRAW_LINE                 = 26,
	COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON        = 27,
	COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS     = 28,
	COMMAND_DRAWER_PATH_BREAK                     = 29,
	COMMAND_DRAWER_PATH_RESTART                   = 30,
	COMMAND_DRAWER_FIGURE_CREATE_SPLINE           = 31,
	COMMAND_DRAWER_FIGURE_CREATE_LINE             = 32,
	COMMAND_DRAWER_FIGURE_SET_COLOR               = 33,
	COMMAND_DRAWER_FIGURE_DELETE                  = 34,
	COMMAND_UNIT_BUILD                            = 35,
	COMMAND_UNIT_STOP                             = 36,
	COMMAND_UNIT_WAIT                             = 37,
	COMMAND_UNIT_WAIT_TIME                        = 38,
	COMMAND_UNIT_WAIT_DEATH                       = 39,
	COMMAND_UNIT_WAIT_SQUAD                       = 40,
	COMMAND_UNIT_WAIT_GATHER                      = 41,
	COMMAND_UNIT_MOVE                             = 42,
	COMMAND_UNIT_PATROL                           = 43,
	COMMAND_UNIT_FIGHT                            = 44,
	COMMAND_UNIT_ATTACK                           = 45,
	COMMAND_UNIT_ATTACK_AREA                      = 46,
	COMMAND_UNIT_GUARD                            = 47,
	COMMAND_UNIT_AI_SELECT                        = 48,
	COMMAND_UNIT_GROUP_ADD                        = 49,
	COMMAND_UNIT_GROUP_CLEAR                      = 50,
	COMMAND_UNIT_REPAIR                           = 51,
	COMMAND_UNIT_SET_FIRE_STATE                   = 52,
	COMMAND_UNIT_SET_MOVE_STATE                   = 53,
	COMMAND_UNIT_SET_BASE                         = 54,
	COMMAND_UNIT_SELF_DESTROY                     = 55,
	COMMAND_UNIT_SET_WANTED_MAX_SPEED             = 56,
	COMMAND_UNIT_LOAD_UNITS                       = 57,
	COMMAND_UNIT_LOAD_UNITS_AREA                  = 58,
	COMMAND_UNIT_LOAD_ONTO                        = 59,
	COMMAND_UNIT_UNLOAD_UNITS_AREA                = 60,
	COMMAND_UNIT_UNLOAD_UNIT                      = 61,
	COMMAND_UNIT_SET_ON_OFF                       = 62,
	COMMAND_UNIT_RECLAIM                          = 63,
	COMMAND_UNIT_RECLAIM_AREA                     = 64,
	COMMAND_UNIT_CLOAK                            = 65,
	COMMAND_UNIT_STOCKPILE                        = 66,
	COMMAND_UNIT_D_GUN                            = 67,
	COMMAND_UNIT_D_GUN_POS                        = 68,
	COMMAND_UNIT_RESTORE_AREA                     = 69,
	COMMAND_UNIT_SET_REPEAT                       = 70,
	COMMAND_UNIT_SET_TRAJECTORY                   = 71,
	COMMAND_UNIT_RESURRECT                        = 72,
	COMMAND_UNIT_RESURRECT_AREA                   = 73,
	COMMAND_UNIT_CAPTURE                          = 74,
	COMMAND_UNIT_CAPTURE_AREA                     = 75,
	COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL            = 76,
	COMMAND_UNIT_SET_IDLE_MODE                    = 77,
	COMMAND_UNIT_CUSTOM                           = 78,
	COMMAND_CHEATS_GIVE_ME_NEW_UNIT               = 79,
//const int COMMAND_UNIT_ATTACK_POS
//const int COMMAND_UNIT_INSERT
//const int COMMAND_UNIT_REMOVE
//const int COMMAND_UNIT_ATTACK_AREA
//const int COMMAND_UNIT_ATTACK_LOOPBACK
//const int COMMAND_UNIT_GROUP_SELECT
//const int COMMAND_UNIT_INTERNAL
};
const unsigned int NUM_CMD_TOPICS                 = 80;


/**
 * These are used in all S*UnitCommand's,
 * in their options field, which is used as a bitfield.
 * This allows to enable special modes of commands,
 * which may be command specific.
 * For example (one you all know):
 * if (SBuildUnitCommand.options & UNIT_COMMAND_OPTION_SHIFT_KEY != 0)
 * then: add to unit command queue, instead of replacing it
 *
 */
enum UnitCommandOptions {
	UNIT_COMMAND_OPTION_DONT_REPEAT       = (1 << 3), //   8
	UNIT_COMMAND_OPTION_RIGHT_MOUSE_KEY   = (1 << 4), //  16
	UNIT_COMMAND_OPTION_SHIFT_KEY         = (1 << 5), //  32
	UNIT_COMMAND_OPTION_CONTROL_KEY       = (1 << 6), //  64
	UNIT_COMMAND_OPTION_ALT_KEY           = (1 << 7), // 128
};


#define UNIT_COMMAND_BUILD_NO_FACING -1



struct SSetMyHandicapCheatCommand {
	float handicap;
}; // COMMAND_CHEATS_SET_MY_HANDICAP
struct SGiveMeResourceCheatCommand {
	int resourceId;
	float amount;
}; // COMMAND_CHEATS_GIVE_ME_RESOURCE
struct SGiveMeNewUnitCheatCommand {
	int unitDefId;
	struct SAIFloat3 pos;
	int ret_newUnitId;
}; // COMMAND_CHEATS_GIVE_ME_NEW_UNIT

/**
 * Sends a chat/text message to other players.
 */
struct SSendTextMessageCommand {
	const char* text;
	int zone;
}; // COMMAND_SEND_TEXT_MESSAGE

struct SSetLastPosMessageCommand {
	struct SAIFloat3 pos;
}; // COMMAND_SET_LAST_POS_MESSAGE

struct SSendResourcesCommand {
	float mAmount;
	float eAmount;
	int receivingTeam;
	bool ret_isExecuted;
}; // COMMAND_SEND_RESOURCES

struct SSendUnitsCommand {
	int* unitIds;
	int numUnitIds;
	int receivingTeam;
	int ret_sentUnits;
}; // COMMAND_SEND_UNITS

struct SCreateGroupCommand {
	const char* libraryName;
	unsigned int aiNumber;
	int ret_groupId;
}; // COMMAND_GROUP_CREATE
struct SEraseGroupCommand {
	int groupId;
}; // COMMAND_GROUP_ERASE
struct SAddUnitToGroupCommand {
	int groupId;
	int unitId;
	bool ret_isExecuted;
}; // COMMAND_GROUP_ADD_UNIT
struct SRemoveUnitFromGroupCommand {
	int unitId;
	bool ret_isExecuted;
}; // COMMAND_GROUP_REMOVE_UNIT


struct SInitPathCommand {
	struct SAIFloat3 start;
	struct SAIFloat3 end;
	int pathType;
	int ret_pathId;
}; // COMMAND_PATH_INIT
/**
 * Returns the approximate path cost between two points
 * note: needs to calculate the complete path so somewhat expensive
 * note: currently disabled, always returns 0
 */
struct SGetApproximateLengthPathCommand {
	struct SAIFloat3 start;
	struct SAIFloat3 end;
	int pathType;
	int ret_approximatePathLength;
}; // COMMAND_PATH_GET_APPROXIMATE_LENGTH
struct SGetNextWaypointPathCommand {
	int pathId;
	struct SAIFloat3 ret_nextWaypoint;
}; // COMMAND_PATH_GET_NEXT_WAYPOINT
struct SFreePathCommand {
	int pathId;
}; // COMMAND_PATH_FREE

struct SCallLuaRulesCommand {
	const char* data;
	int inSize;
	int* outSize;
	const char* ret_outData;
}; // COMMAND_CALL_LUA_RULES

struct SSendStartPosCommand {///< result of HandleCommand is 1 - ok supported
	bool ready;
	struct SAIFloat3 pos;
}; // COMMAND_SEND_START_POS

struct SAddNotificationDrawerCommand {
	struct SAIFloat3 pos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_ADD_NOTIFICATION
struct SAddPointDrawCommand { // result of HandleCommand is 1 - ok supported
	struct SAIFloat3 pos; // on this position, only x and z matter
	const char* label; // create this text on pos in my team color
}; // COMMAND_DRAWER_POINT_ADD
struct SRemovePointDrawCommand { // result of HandleCommand is 1 - ok supported
	struct SAIFloat3 pos; // remove map points and lines near this point (100 distance)
}; // COMMAND_DRAWER_POINT_REMOVE
struct SAddLineDrawCommand { // result of HandleCommand is 1 - ok supported
	struct SAIFloat3 posFrom; // draw line from this pos
	struct SAIFloat3 posTo; // to this pos, again only x and z matter
}; // COMMAND_DRAWER_LINE_ADD

struct SStartPathDrawerCommand {
	struct SAIFloat3 pos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_PATH_START
struct SFinishPathDrawerCommand {
}; // COMMAND_DRAWER_PATH_FINISH
struct SDrawLinePathDrawerCommand {
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_PATH_DRAW_LINE
struct SDrawLineAndIconPathDrawerCommand {
	int cmdId;
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON
struct SDrawIconAtLastPosPathDrawerCommand {
	int cmdId;
}; // COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS
struct SBreakPathDrawerCommand {
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_PATH_BREAK
struct SRestartPathDrawerCommand {
	bool sameColor;
}; // COMMAND_DRAWER_PATH_RESTART

/**
 * Creates a cubic Bezier spline figure (from pos1 to pos4 with control points pos2 and pos3)
 */
struct SCreateSplineFigureDrawerCommand {
	struct SAIFloat3 pos1;
	struct SAIFloat3 pos2;
	struct SAIFloat3 pos3;
	struct SAIFloat3 pos4;
	float width;
	bool arrow; // true: means that the figure will get an arrow at the end
	int lifeTime; // how many frames a figure should live before being autoremoved, 0 means no removal
	int figureGroupId; // use 0 to get a new group
	int ret_newFigureGroupId; // the new group
}; // COMMAND_DRAWER_FIGURE_CREATE_SPLINE
struct SCreateLineFigureDrawerCommand {
	struct SAIFloat3 pos1;
	struct SAIFloat3 pos2;
	float width;
	bool arrow; // true: means that the figure will get an arrow at the end
	int lifeTime; // how many frames a figure should live before being autoremoved, 0 means no removal
	int figureGroupId; // use 0 to get a new group
	int ret_newFigureGroupId; // the new group
}; // COMMAND_DRAWER_FIGURE_CREATE_LINE
struct SSetColorFigureDrawerCommand {
	int figureGroupId;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_FIGURE_SET_COLOR
struct SDeleteFigureDrawerCommand {
	int figureGroupId;
}; // COMMAND_DRAWER_FIGURE_DELETE

struct SDrawUnitDrawerCommand {
	int toDrawUnitDefId;
	struct SAIFloat3 pos;
	float rotation;
	int lifeTime;
	int teamId;
	bool transparent;
	bool drawBorder;
	int facing;
}; // COMMAND_DRAWER_DRAW_UNIT



struct SBuildUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toBuildUnitDefId;
	struct SAIFloat3 buildPos;
	int facing; // set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
}; // COMMAND_UNIT_BUILD

struct SStopUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_STOP

//	struct SInsertUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};
//
//	struct SRemoveUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};

struct SWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_WAIT

struct STimeWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	// the time in seconds to wait
	int time;
}; // COMMAND_UNIT_WAIT_TIME

struct SDeathWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	// wait until this unit is dead
	int toDieUnitId;
}; // COMMAND_UNIT_WAIT_DEATH

struct SSquadWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int numUnits;
}; // COMMAND_UNIT_WAIT_SQUAD

struct SGatherWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_WAIT_GATHER

struct SMoveUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_MOVE

struct SPatrolUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_PATROL

struct SFightUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_FIGHT

struct SAttackUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toAttackUnitId;
}; // COMMAND_UNIT_ATTACK

//	struct SAttackPosUnitCommand {
struct SAttackAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toAttackPos;
	float radius;
}; // COMMAND_UNIT_ATTACK_AREA

//	struct SAttackAreaUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};

struct SGuardUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toGuardUnitId;
}; // COMMAND_UNIT_GUARD

struct SAiSelectUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_AI_SELECT

//	struct SGroupSelectUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};

struct SGroupAddUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toGroupId;
}; // COMMAND_UNIT_GROUP_ADD

struct SGroupClearUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_GROUP_CLEAR

struct SRepairUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toRepairUnitId;
}; // COMMAND_UNIT_REPAIR

struct SSetFireStateUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	// can be: 0=hold fire, 1=return fire, 2=fire at will
	int fireState;
}; // COMMAND_UNIT_SET_FIRE_STATE

struct SSetMoveStateUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int moveState;
}; // COMMAND_UNIT_SET_MOVE_STATE

struct SSetBaseUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 basePos;
}; // COMMAND_UNIT_SET_BASE

//	struct SInternalUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};

struct SSelfDestroyUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_SELF_DESTROY

struct SSetWantedMaxSpeedUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	float wantedMaxSpeed;
}; // COMMAND_UNIT_SET_WANTED_MAX_SPEED

struct SLoadUnitsUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?milli-seconds?

	int* toLoadUnitIds;
	int numToLoadUnits;
}; // COMMAND_UNIT_LOAD_UNITS

struct SLoadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_LOAD_UNITS_AREA

struct SLoadOntoUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int transporterUnitId;
}; // COMMAND_UNIT_LOAD_ONTO

struct SUnloadUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
	int toUnloadUnitId;
}; // COMMAND_UNIT_UNLOAD_UNIT

struct SUnloadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
	float radius;
}; // COMMAND_UNIT_UNLOAD_UNITS_AREA

struct SSetOnOffUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	bool on;
}; // COMMAND_UNIT_SET_ON_OFF

struct SReclaimUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toReclaimUnitIdOrFeatureId;
}; // COMMAND_UNIT_RECLAIM

struct SReclaimAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RECLAIM_AREA

struct SCloakUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	bool cloak;
}; // COMMAND_UNIT_CLOAK

struct SStockpileUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?
}; // COMMAND_UNIT_STOCKPILE

struct SDGunUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toAttackUnitId;
}; // COMMAND_UNIT_D_GUN

struct SDGunPosUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
}; // COMMAND_UNIT_D_GUN_POS

struct SRestoreAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RESTORE_AREA

struct SSetRepeatUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	bool repeat;
}; // COMMAND_UNIT_SET_REPEAT

struct SSetTrajectoryUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int trajectory;
}; // COMMAND_UNIT_SET_TRAJECTORY

struct SResurrectUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toResurrectFeatureId;
}; // COMMAND_UNIT_RESURRECT

struct SResurrectAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RESURRECT_AREA

struct SCaptureUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int toCaptureUnitId;
}; // COMMAND_UNIT_CAPTURE

struct SCaptureAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_CAPTURE_AREA

struct SSetAutoRepairLevelUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int autoRepairLevel;
}; // COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL

//	struct SAttackLoopbackUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see enum UnitCommandOptions
//		int timeOut; // command execution-time in ?seconds?
//	};

struct SSetIdleModeUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int idleMode;
}; // COMMAND_UNIT_SET_IDLE_MODE

struct SCustomUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see enum UnitCommandOptions
	int timeOut; // command execution-time in ?seconds?

	int cmdId;
	float* params;
	int numParams;
}; // COMMAND_UNIT_CUSTOM

/**
 * @brief Sets default values
 */
void initSUnitCommand(void* sUnitCommand);

#ifdef	__cplusplus
}	// extern "C"
#endif


#ifdef	__cplusplus
struct Command;

// legacy support functions
/**
 * @brief Allocates memory for a C Command struct
 */
void* mallocSUnitCommand(int unitId, int groupId, const Command* c, int* sCommandId);
/**
 * @brief Frees memory of a C Command struct
 */
void freeSUnitCommand(void* sCommandData, int sCommandId);
/**
 * @brief creates - with new - an engine C++ Command struct
 */
Command* newCommand(void* sUnitCommandData, int sCommandId);
#endif	// __cplusplus


#endif	// _AISCOMMANDS_H
