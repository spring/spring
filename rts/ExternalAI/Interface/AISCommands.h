/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
 * Commands are used for all activities that change game state,
 * in spring terms: synced events
 * Activities that leave the game state as it is (-> unsynced events)
 * are handled through function pointers in SSkirmishAICallback.h.
 *
 * Each command type can be identified through a unique ID,
 * which we call command topic.
 * Commands are usually sent from AIs to the engine,
 * but there are plans for the future to allow AI -> AI command scheduling.
 *
 * Note: Do NOT change the values assigned to these topics in enum CommandTopic,
 * as this would be bad for inter-version compatibility.
 * You should always append new command topics at the end of this list,
 * adjust NUM_CMD_TOPICS and add the struct in AIINTERFACE_COMMANDS_ABI_VERSION.
 *
 * @see SSkirmishAICallback.handleCommand()
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
	COMMAND_TRACE_RAY                             = 80,
	COMMAND_PAUSE                                 = 81,
//const int COMMAND_UNIT_ATTACK_POS
//const int COMMAND_UNIT_INSERT
//const int COMMAND_UNIT_REMOVE
//const int COMMAND_UNIT_ATTACK_AREA
//const int COMMAND_UNIT_ATTACK_LOOPBACK
//const int COMMAND_UNIT_GROUP_SELECT
//const int COMMAND_UNIT_INTERNAL
	COMMAND_DEBUG_DRAWER_ADD_GRAPH_POINT           = 82,
	COMMAND_DEBUG_DRAWER_DELETE_GRAPH_POINTS       = 83,
	COMMAND_DEBUG_DRAWER_SET_GRAPH_POS             = 84,
	COMMAND_DEBUG_DRAWER_SET_GRAPH_SIZE            = 85,
	COMMAND_DEBUG_DRAWER_SET_GRAPH_LINE_COLOR      = 86,
	COMMAND_DEBUG_DRAWER_SET_GRAPH_LINE_LABEL      = 87,

	COMMAND_DEBUG_DRAWER_ADD_OVERLAY_TEXTURE       = 88,
	COMMAND_DEBUG_DRAWER_UPDATE_OVERLAY_TEXTURE    = 89,
	COMMAND_DEBUG_DRAWER_DEL_OVERLAY_TEXTURE       = 90,
	COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_POS   = 91,
	COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_SIZE  = 92,
	COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_LABEL = 93,
};

const unsigned int NUM_CMD_TOPICS = 94;


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


#define AIINTERFACE_COMMANDS_ABI_VERSION     ( \
		  sizeof(struct SSetMyHandicapCheatCommand) \
		+ sizeof(struct SGiveMeResourceCheatCommand) \
		+ sizeof(struct SGiveMeNewUnitCheatCommand) \
		+ sizeof(struct SSendTextMessageCommand) \
		+ sizeof(struct SSetLastPosMessageCommand) \
		+ sizeof(struct SSendResourcesCommand) \
		+ sizeof(struct SSendUnitsCommand) \
		+ sizeof(struct SCreateGroupCommand) \
		+ sizeof(struct SEraseGroupCommand) \
		+ sizeof(struct SAddUnitToGroupCommand) \
		+ sizeof(struct SRemoveUnitFromGroupCommand) \
		+ sizeof(struct SInitPathCommand) \
		+ sizeof(struct SGetApproximateLengthPathCommand) \
		+ sizeof(struct SGetNextWaypointPathCommand) \
		+ sizeof(struct SFreePathCommand) \
		+ sizeof(struct SCallLuaRulesCommand) \
		+ sizeof(struct SSendStartPosCommand) \
		+ sizeof(struct SAddNotificationDrawerCommand) \
		+ sizeof(struct SAddPointDrawCommand) \
		+ sizeof(struct SRemovePointDrawCommand) \
		+ sizeof(struct SAddLineDrawCommand) \
		+ sizeof(struct SStartPathDrawerCommand) \
		+ sizeof(struct SFinishPathDrawerCommand) \
		+ sizeof(struct SDrawLinePathDrawerCommand) \
		+ sizeof(struct SDrawLineAndIconPathDrawerCommand) \
		+ sizeof(struct SDrawIconAtLastPosPathDrawerCommand) \
		+ sizeof(struct SBreakPathDrawerCommand) \
		+ sizeof(struct SRestartPathDrawerCommand) \
		+ sizeof(struct SCreateSplineFigureDrawerCommand) \
		+ sizeof(struct SCreateLineFigureDrawerCommand) \
		+ sizeof(struct SSetColorFigureDrawerCommand) \
		+ sizeof(struct SDeleteFigureDrawerCommand) \
		+ sizeof(struct SDrawUnitDrawerCommand) \
		+ sizeof(struct SBuildUnitCommand) \
		+ sizeof(struct SStopUnitCommand) \
		+ sizeof(struct SWaitUnitCommand) \
		+ sizeof(struct STimeWaitUnitCommand) \
		+ sizeof(struct SDeathWaitUnitCommand) \
		+ sizeof(struct SSquadWaitUnitCommand) \
		+ sizeof(struct SGatherWaitUnitCommand) \
		+ sizeof(struct SMoveUnitCommand) \
		+ sizeof(struct SPatrolUnitCommand) \
		+ sizeof(struct SFightUnitCommand) \
		+ sizeof(struct SAttackUnitCommand) \
		+ sizeof(struct SAttackAreaUnitCommand) \
		+ sizeof(struct SGuardUnitCommand) \
		+ sizeof(struct SAiSelectUnitCommand) \
		+ sizeof(struct SGroupAddUnitCommand) \
		+ sizeof(struct SGroupClearUnitCommand) \
		+ sizeof(struct SRepairUnitCommand) \
		+ sizeof(struct SSetFireStateUnitCommand) \
		+ sizeof(struct SSetMoveStateUnitCommand) \
		+ sizeof(struct SSetBaseUnitCommand) \
		+ sizeof(struct SSelfDestroyUnitCommand) \
		+ sizeof(struct SSetWantedMaxSpeedUnitCommand) \
		+ sizeof(struct SLoadUnitsUnitCommand) \
		+ sizeof(struct SLoadUnitsAreaUnitCommand) \
		+ sizeof(struct SLoadOntoUnitCommand) \
		+ sizeof(struct SUnloadUnitCommand) \
		+ sizeof(struct SUnloadUnitsAreaUnitCommand) \
		+ sizeof(struct SSetOnOffUnitCommand) \
		+ sizeof(struct SReclaimUnitCommand) \
		+ sizeof(struct SReclaimAreaUnitCommand) \
		+ sizeof(struct SCloakUnitCommand) \
		+ sizeof(struct SStockpileUnitCommand) \
		+ sizeof(struct SDGunUnitCommand) \
		+ sizeof(struct SDGunPosUnitCommand) \
		+ sizeof(struct SRestoreAreaUnitCommand) \
		+ sizeof(struct SSetRepeatUnitCommand) \
		+ sizeof(struct SSetTrajectoryUnitCommand) \
		+ sizeof(struct SResurrectUnitCommand) \
		+ sizeof(struct SResurrectAreaUnitCommand) \
		+ sizeof(struct SCaptureUnitCommand) \
		+ sizeof(struct SCaptureAreaUnitCommand) \
		+ sizeof(struct SSetAutoRepairLevelUnitCommand) \
		+ sizeof(struct SSetIdleModeUnitCommand) \
		+ sizeof(struct SCustomUnitCommand) \
		+ sizeof(struct STraceRayCommand) \
		+ sizeof(struct SPauseCommand) \
		+ sizeof(struct SDebugDrawerAddGraphPointCommand) \
		+ sizeof(struct SDebugDrawerDeleteGraphPointsCommand) \
		+ sizeof(struct SDebugDrawerSetGraphPositionCommand) \
		+ sizeof(struct SDebugDrawerSetGraphSizeCommand) \
		+ sizeof(struct SDebugDrawerSetGraphLineColorCommand) \
		+ sizeof(struct SDebugDrawerSetGraphLineLabelCommand) \
		+ sizeof(struct SDebugDrawerAddOverlayTextureCommand) \
		+ sizeof(struct SDebugDrawerUpdateOverlayTextureCommand) \
		+ sizeof(struct SDebugDrawerDelOverlayTextureCommand) \
		+ sizeof(struct SDebugDrawerSetOverlayTexturePosCommand) \
		+ sizeof(struct SDebugDrawerSetOverlayTextureSizeCommand) \
		+ sizeof(struct SDebugDrawerSetOverlayTextureLabelCommand) \
		)

/**
 * This function has the same effect as setting a handicap value
 * in the GameSetup script (currently gives a bonus on collected
 * resources)
 */
struct SSetMyHandicapCheatCommand {
	float handicap;
}; // COMMAND_CHEATS_SET_MY_HANDICAP
/**
 * The AI team receives the specified amount of units of the specified resource.
 */
struct SGiveMeResourceCheatCommand {
	int resourceId;
	float amount;
}; // COMMAND_CHEATS_GIVE_ME_RESOURCE
/**
 * Creates a new unit with the selected name at pos,
 * and returns its unit ID in ret_newUnitId.
 */
struct SGiveMeNewUnitCheatCommand {
	int unitDefId;
	struct SAIFloat3 pos;
	int ret_newUnitId;
}; // COMMAND_CHEATS_GIVE_ME_NEW_UNIT

/**
 * @brief Sends a chat/text message to other players.
 * This text will also end up in infolog.txt.
 */
struct SSendTextMessageCommand {
	const char* text;
	int zone;
}; // COMMAND_SEND_TEXT_MESSAGE

/**
 * Assigns a map location to the last text message sent by the AI.
 */
struct SSetLastPosMessageCommand {
	struct SAIFloat3 pos;
}; // COMMAND_SET_LAST_POS_MESSAGE

/**
 * Give <amount> units of resource <resourceId> to team <receivingTeam>.
 * - the amount is capped to the AI team's resource levels
 * - does not check for alliance with <receivingTeam>
 * - LuaRules might not allow resource transfers, AI's must verify the deduction
 */
struct SSendResourcesCommand {
	int resourceId;
	float amount;
	int receivingTeam;
	bool ret_isExecuted;
}; // COMMAND_SEND_RESOURCES

/**
 * Give units specified by <unitIds> to team <receivingTeam>.
 * <ret_sentUnits> represents how many actually were transferred.
 * Make sure this always matches the size of <unitIds> you passed in.
 * If it does not, then some unitId's were filtered out.
 * - does not check for alliance with <receivingTeam>
 * - AI's should check each unit if it is still under control of their
 *   team after the transaction via UnitTaken() and UnitGiven(), since
 *   LuaRules might block part of it
 */
struct SSendUnitsCommand {
	int* unitIds;
	int numUnitIds;
	int receivingTeam;
	int ret_sentUnits;
}; // COMMAND_SEND_UNITS

/// Creates a group and returns the id it was given, returns -1 on failure
struct SCreateGroupCommand {
	int ret_groupId;
}; // COMMAND_GROUP_CREATE
/// Erases a specified group
struct SEraseGroupCommand {
	int groupId;
}; // COMMAND_GROUP_ERASE
/**
 * @brief Adds a unit to a specific group.
 * If it was previously in a group, it is removed from that.
 * Returns false if the group did not exist or did not accept the unit.
 */
struct SAddUnitToGroupCommand {
	int groupId;
	int unitId;
	bool ret_isExecuted;
}; // COMMAND_GROUP_ADD_UNIT
/// Removes a unit from its group
struct SRemoveUnitFromGroupCommand {
	int unitId;
	bool ret_isExecuted;
}; // COMMAND_GROUP_REMOVE_UNIT


/**
 * The following functions allow the AI to use the built-in path-finder.
 *
 * - call InitPath and you get a pathId back
 * - use this to call GetNextWaypoint to get subsequent waypoints;
 *   the waypoints are centered on 8*8 squares
 * - note that the pathfinder calculates the waypoints as needed,
 *   so do not retrieve them until they are needed
 * - the waypoint's x and z coordinates are returned in x and z,
 *   while y is used for error codes:
 *   y >= 0: worked ok
 *   y = -2: still thinking, call again
 *   y = -1: end of path reached or path is invalid
 * - for pathType {Ground_Move=0, Hover_Move=1, Ship_Move=2},
 *   @see UnitDef_MoveData_getMoveType()
 * - goalRadius defines a goal area within which any square could be accepted as
 *   path target. If a singular goal position is wanted, use 0.0f.
 *   default: 8.0f
 */
struct SInitPathCommand {
	/// The starting location of the requested path
	struct SAIFloat3 start;
	/// The goal location of the requested path
	struct SAIFloat3 end;
	/// For what type of unit should the path be calculated
	int pathType;
	/// default: 8.0f
	float goalRadius;
	int ret_pathId;
}; // COMMAND_PATH_INIT
/**
 * Returns the approximate path cost between two points.
 * - for pathType {Ground_Move=0, Hover_Move=1, Ship_Move=2},
 *   @see UnitDef_MoveData_getMoveType()
 * - goalRadius defines a goal area within which any square could be accepted as
 *   path target. If a singular goal position is wanted, use 0.0f.
 *   default: 8.0f
 */
struct SGetApproximateLengthPathCommand {
	/// The starting location of the requested path
	struct SAIFloat3 start;
	/// The goal location of the requested path
	struct SAIFloat3 end;
	/// For what type of unit should the path be calculated
	int pathType;
	/// default: 8.0f
	float goalRadius;
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
	/// Can be setup to NULL to skip passing in a string
	const char* data;
	/// If this is less than 0, the data size is calculated using strlen()
	int inSize;
	int* outSize;
	/// this is subject to lua garbage collection, copy it if you wish to continue using it
	const char* ret_outData;
}; // COMMAND_CALL_LUA_RULES

struct SSendStartPosCommand {
	bool ready;
	/// on this position, only x and z matter
	struct SAIFloat3 pos;
}; // COMMAND_SEND_START_POS

struct SAddNotificationDrawerCommand {
	/// on this position, only x and z matter
	struct SAIFloat3 pos;
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_ADD_NOTIFICATION
struct SAddPointDrawCommand {
	/// on this position, only x and z matter
	struct SAIFloat3 pos;
	/// create this text on pos in my team color
	const char* label;
}; // COMMAND_DRAWER_POINT_ADD
struct SRemovePointDrawCommand {
	/// remove map points and lines near this point (100 distance)
	struct SAIFloat3 pos;
}; // COMMAND_DRAWER_POINT_REMOVE
struct SAddLineDrawCommand {
	/// draw line from this pos
	struct SAIFloat3 posFrom;
	/// to this pos, again only x and z matter
	struct SAIFloat3 posTo;
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
 * @brief Creates a cubic Bezier spline figure
 * Creates a cubic Bezier spline figure from pos1 to pos4, with control points pos2 and pos3.
 *
 * - Each figure is part of a figure group
 * - When creating figures, use 0 as <figureGroupId> to create a new figure group.
 *   The id of this figure group is returned in <ret_newFigureGroupId>
 * - <lifeTime> specifies how many frames a figure should live before being auto-removed;
 *   0 means no removal
 * - <arrow> == true means that the figure will get an arrow at the end
 */
struct SCreateSplineFigureDrawerCommand {
	struct SAIFloat3 pos1;
	struct SAIFloat3 pos2;
	struct SAIFloat3 pos3;
	struct SAIFloat3 pos4;
	float width;
	/// true: means that the figure will get an arrow at the end
	bool arrow;
	/// how many frames a figure should live before being autoremoved, 0 means no removal
	int lifeTime;
	/// use 0 to get a new group
	int figureGroupId;
	/// the new group
	int ret_newFigureGroupId;
}; // COMMAND_DRAWER_FIGURE_CREATE_SPLINE
/**
 * @brief Creates a straight line
 * Creates a straight line from pos1 to pos2.
 *
 * - Each figure is part of a figure group
 * - When creating figures, use 0 as <figureGroupId> to create a new figure group.
 *   The id of this figure group is returned in <ret_newFigureGroupId>
 * @param lifeTime specifies how many frames a figure should live before being auto-removed;
 *                 0 means no removal
 * @param arrow true means that the figure will get an arrow at the end
 */
struct SCreateLineFigureDrawerCommand {
	struct SAIFloat3 pos1;
	struct SAIFloat3 pos2;
	float width;
	/// true: means that the figure will get an arrow at the end
	bool arrow;
	/// how many frames a figure should live before being autoremoved, 0 means no removal
	int lifeTime;
	/// use 0 to get a new group
	int figureGroupId;
	/// the new group
	int ret_newFigureGroupId;
}; // COMMAND_DRAWER_FIGURE_CREATE_LINE
/**
 * Sets the color used to draw all lines of figures in a figure group.
 */
struct SSetColorFigureDrawerCommand {
	int figureGroupId;
	/// (x, y, z) -> (red, green, blue)
	struct SAIFloat3 color;
	float alpha;
}; // COMMAND_DRAWER_FIGURE_SET_COLOR
/**
 * Removes a figure group, which means it will not be drawn anymore.
 */
struct SDeleteFigureDrawerCommand {
	int figureGroupId;
}; // COMMAND_DRAWER_FIGURE_DELETE

/**
 * This function allows you to draw units onto the map.
 * - they only show up on the local player's screen
 * - they will be drawn in the "standard pose" (as if before any COB scripts are run)
 */
struct SDrawUnitDrawerCommand {
	int toDrawUnitDefId;
	struct SAIFloat3 pos;
	/// in radians
	float rotation;
	/// specifies how many frames a figure should live before being auto-removed; 0 means no removal
	int lifeTime;
	/// teamId affects the color of the unit
	int teamId;
	bool transparent;
	bool drawBorder;
	int facing;
}; // COMMAND_DRAWER_DRAW_UNIT



struct SBuildUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toBuildUnitDefId;
	struct SAIFloat3 buildPos;
	/// set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
	int facing;
}; // COMMAND_UNIT_BUILD

struct SStopUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_STOP

//struct SInsertUnitCommand {
//	int unitId;
//	int groupId;
//	unsigned int options; // see enum UnitCommandOptions
//	int timeOut; // command execution-time in ?seconds?
//};
//
//struct SRemoveUnitCommand {
//	int unitId;
//	int groupId;
//	unsigned int options; // see enum UnitCommandOptions
//	int timeOut; // command execution-time in ?seconds?
//};

struct SWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_WAIT

struct STimeWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	/// the time in seconds to wait
	int time;
}; // COMMAND_UNIT_WAIT_TIME

struct SDeathWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	/// wait until this unit is dead
	int toDieUnitId;
}; // COMMAND_UNIT_WAIT_DEATH

struct SSquadWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int numUnits;
}; // COMMAND_UNIT_WAIT_SQUAD

struct SGatherWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_WAIT_GATHER

struct SMoveUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_MOVE

struct SPatrolUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_PATROL

struct SFightUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toPos;
}; // COMMAND_UNIT_FIGHT

struct SAttackUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toAttackUnitId;
}; // COMMAND_UNIT_ATTACK

//	struct SAttackPosUnitCommand {
struct SAttackAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toAttackPos;
	float radius;
}; // COMMAND_UNIT_ATTACK_AREA

//struct SAttackAreaUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	unsigned int options;
//	/// (frames) abort if it takes longer then this to start execution of the command
//	int timeOut;
//};

struct SGuardUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toGuardUnitId;
}; // COMMAND_UNIT_GUARD

struct SAiSelectUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_AI_SELECT

//struct SGroupSelectUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	unsigned int options;
//	/// (frames) abort if it takes longer then this to start execution of the command
//	int timeOut;
//};

struct SGroupAddUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toGroupId;
}; // COMMAND_UNIT_GROUP_ADD

struct SGroupClearUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_GROUP_CLEAR

struct SRepairUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toRepairUnitId;
}; // COMMAND_UNIT_REPAIR

struct SSetFireStateUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	/// can be: 0=hold fire, 1=return fire, 2=fire at will
	int fireState;
}; // COMMAND_UNIT_SET_FIRE_STATE

struct SSetMoveStateUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	/// 0=hold pos, 1=maneuvre, 2=roam
	int moveState;
}; // COMMAND_UNIT_SET_MOVE_STATE

struct SSetBaseUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 basePos;
}; // COMMAND_UNIT_SET_BASE

//struct SInternalUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	unsigned int options;
//	/// (frames) abort if it takes longer then this to start execution of the command
//	int timeOut;
//};

struct SSelfDestroyUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_SELF_DESTROY

struct SSetWantedMaxSpeedUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	float wantedMaxSpeed;
}; // COMMAND_UNIT_SET_WANTED_MAX_SPEED

struct SLoadUnitsUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	int timeOut; // command execution-time in ?milli-seconds?

	int* toLoadUnitIds;
	int numToLoadUnits;
}; // COMMAND_UNIT_LOAD_UNITS

struct SLoadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_LOAD_UNITS_AREA

struct SLoadOntoUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int transporterUnitId;
}; // COMMAND_UNIT_LOAD_ONTO

struct SUnloadUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toPos;
	int toUnloadUnitId;
}; // COMMAND_UNIT_UNLOAD_UNIT

struct SUnloadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 toPos;
	float radius;
}; // COMMAND_UNIT_UNLOAD_UNITS_AREA

struct SSetOnOffUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	bool on;
}; // COMMAND_UNIT_SET_ON_OFF

struct SReclaimUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	/// if < maxUnits -> unitId, else -> featureId
	int toReclaimUnitIdOrFeatureId;
}; // COMMAND_UNIT_RECLAIM

struct SReclaimAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RECLAIM_AREA

struct SCloakUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	bool cloak;
}; // COMMAND_UNIT_CLOAK

struct SStockpileUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;
}; // COMMAND_UNIT_STOCKPILE

struct SDGunUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toAttackUnitId;
}; // COMMAND_UNIT_D_GUN

struct SDGunPosUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
}; // COMMAND_UNIT_D_GUN_POS

struct SRestoreAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RESTORE_AREA

struct SSetRepeatUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	bool repeat;
}; // COMMAND_UNIT_SET_REPEAT

struct SSetTrajectoryUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int trajectory;
}; // COMMAND_UNIT_SET_TRAJECTORY

struct SResurrectUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toResurrectFeatureId;
}; // COMMAND_UNIT_RESURRECT

struct SResurrectAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_RESURRECT_AREA

struct SCaptureUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int toCaptureUnitId;
}; // COMMAND_UNIT_CAPTURE

struct SCaptureAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	struct SAIFloat3 pos;
	float radius;
}; // COMMAND_UNIT_CAPTURE_AREA

struct SSetAutoRepairLevelUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int autoRepairLevel;
}; // COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL

//struct SAttackLoopbackUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	unsigned int options;
//	/// (frames) abort if it takes longer then this to start execution of the command
//	int timeOut;
//};

struct SSetIdleModeUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int idleMode;
}; // COMMAND_UNIT_SET_IDLE_MODE

struct SCustomUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	unsigned int options;
	/// (frames) abort if it takes longer then this to start execution of the command
	int timeOut;

	int cmdId;
	float* params;
	int numParams;
}; // COMMAND_UNIT_CUSTOM

struct STraceRayCommand {
	struct SAIFloat3 rayPos;
	struct SAIFloat3 rayDir;
	float rayLen;
	int srcUID;
	int hitUID;
	int flags;
}; // COMMAND_TRACE_RAY

/**
 * Pause or unpauses the game.
 * This is meant for debugging purposes.
 * Keep in mind that pause does not happen immediately.
 * It can take 1-2 frames in single- and up to 10 frames in multiplayer matches.
 */
struct SPauseCommand {
	bool enable;
	/// reason for the (un-)pause, or NULL
	const char* reason;
}; // COMMAND_PAUSE



struct SDebugDrawerAddGraphPointCommand {
	float x;
	float y;
	int lineId;
}; // COMMAND_DEBUG_DRAWER_ADD_GRAPH_POINT

struct SDebugDrawerDeleteGraphPointsCommand {
	int lineId;
	int numPoints;
}; // COMMAND_DEBUG_DRAWER_DELETE_GRAPH_POINTS

struct SDebugDrawerSetGraphPositionCommand {
	float x;
	float y;
}; // COMMAND_DEBUG_DRAWER_SET_GRAPH_POS

struct SDebugDrawerSetGraphSizeCommand {
	float w;
	float h;
}; // COMMAND_DEBUG_DRAWER_SET_GRAPH_SIZE

struct SDebugDrawerSetGraphLineColorCommand {
	int lineId;
	struct SAIFloat3 color;
}; // COMMAND_DEBUG_DRAWER_SET_GRAPH_LINE_COLOR

struct SDebugDrawerSetGraphLineLabelCommand {
	int lineId;
	const char* label;
}; // COMMAND_DEBUG_DRAWER_SET_GRAPH_LINE_LABEL


struct SDebugDrawerAddOverlayTextureCommand {
	int texHandle;
	const float* texData;
	int w;
	int h;
}; // COMMAND_DEBUG_DRAWER_ADD_OVERLAY_TEXTURE

struct SDebugDrawerUpdateOverlayTextureCommand {
	int texHandle;
	const float* texData;
	int x;
	int y;
	int w;
	int h;
}; // COMMAND_DEBUG_DRAWER_UPDATE_OVERLAY_TEXTURE

struct SDebugDrawerDelOverlayTextureCommand {
	int texHandle;
}; // COMMAND_DEBUG_DRAWER_DEL_OVERLAY_TEXTURE

struct SDebugDrawerSetOverlayTexturePosCommand {
	int texHandle;
	float x;
	float y;
}; // COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_POS

struct SDebugDrawerSetOverlayTextureSizeCommand {
	int texHandle;
	float w;
	float h;
}; // COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_SIZE

struct SDebugDrawerSetOverlayTextureLabelCommand {
	int texHandle;
	const char* label;
}; // COMMAND_DEBUG_DRAWER_SET_OVERLAY_TEXTURE_LABEL



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
 * Returns the engine internal C++ unit command (topic) ID
 * that corresponds to the C AI Interface command topic ID specified by
 * <code>aiCmdTopic</code>.
 */
int toInternalUnitCommandTopic(int aiCmdTopic, void* sUnitCommandData);

/**
 * Returns the C AI Interface command topic ID that corresponds
 * to the engine internal C++ unit command (topic) ID specified by
 * <code>internalUnitCmdTopic</code>.
 */
int extractAICommandTopic(const Command* internalUnitCmd);

/**
 * @brief creates - with new - an engine C++ Command struct
 */
Command* newCommand(void* sUnitCommandData, int sCommandId);
#endif	// __cplusplus


#endif	// _AISCOMMANDS_H
