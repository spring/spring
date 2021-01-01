/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_S_COMMANDS_H
#define	AI_S_COMMANDS_H

// IMPORTANT NOTE: external systems parse this file,
// so DO NOT CHANGE the style and format it uses without
// major though in advance, and deliberation with hoijui!

#include "aidefines.h"

#ifdef	__cplusplus
extern "C" {
#endif

// NOTE structs should not be empty (C90), so add a useless member if needed

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
	COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER       =  5,
	COMMAND_SEND_TEXT_MESSAGE                     =  6,
	COMMAND_SET_LAST_POS_MESSAGE                  =  7,
	COMMAND_SEND_RESOURCES                        =  8,
	COMMAND_SEND_UNITS                            =  9,
	COMMAND_UNUSED_0                              = 10, // unused
	COMMAND_UNUSED_1                              = 11, // unused
	COMMAND_GROUP_CREATE                          = 12, // unused
	COMMAND_GROUP_ERASE                           = 13, // unused
	COMMAND_GROUP_ADD_UNIT                        = 14, // unused
	COMMAND_GROUP_REMOVE_UNIT                     = 15, // unused
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
	COMMAND_UNIT_SET_WANTED_MAX_SPEED             = 56, // unused
	COMMAND_UNIT_LOAD_UNITS                       = 57,
	COMMAND_UNIT_LOAD_UNITS_AREA                  = 58,
	COMMAND_UNIT_LOAD_ONTO                        = 59,
	COMMAND_UNIT_UNLOAD_UNITS_AREA                = 60,
	COMMAND_UNIT_UNLOAD_UNIT                      = 61,
	COMMAND_UNIT_SET_ON_OFF                       = 62,
	COMMAND_UNIT_RECLAIM_UNIT                     = 63,
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
	COMMAND_UNIT_RECLAIM_FEATURE                  = 82,
//const int COMMAND_UNIT_ATTACK_POS
//const int COMMAND_UNIT_INSERT
//const int COMMAND_UNIT_REMOVE
//const int COMMAND_UNIT_ATTACK_AREA
//const int COMMAND_UNIT_ATTACK_LOOPBACK
//const int COMMAND_UNIT_GROUP_SELECT
//const int COMMAND_UNIT_INTERNAL
	COMMAND_DEBUG_DRAWER_GRAPH_SET_POS            = 83,
	COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE           = 84,
	COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT     = 85,
	COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS = 86,
	COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR     = 87,
	COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL     = 88,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD       = 89,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE    = 90,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE    = 91,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS   = 92,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE  = 93,
	COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL = 94,
	COMMAND_TRACE_RAY_FEATURE                     = 95,
	COMMAND_CALL_LUA_UI                           = 96,
};
const int NUM_CMD_TOPICS = 97;


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
	UNIT_COMMAND_OPTION_INTERNAL_ORDER    = (1 << 3), //   8
	UNIT_COMMAND_OPTION_RIGHT_MOUSE_KEY   = (1 << 4), //  16
	UNIT_COMMAND_OPTION_SHIFT_KEY         = (1 << 5), //  32
	UNIT_COMMAND_OPTION_CONTROL_KEY       = (1 << 6), //  64
	UNIT_COMMAND_OPTION_ALT_KEY           = (1 << 7), // 128
};


#define UNIT_COMMAND_BUILD_NO_FACING -1


#define AIINTERFACE_COMMANDS_ABI_VERSION     ( \
		  sizeof(struct SSetMyIncomeMultiplierCheatCommand) \
		+ sizeof(struct SGiveMeResourceCheatCommand) \
		+ sizeof(struct SGiveMeNewUnitCheatCommand) \
		+ sizeof(struct SSendTextMessageCommand) \
		+ sizeof(struct SSetLastPosMessageCommand) \
		+ sizeof(struct SSendResourcesCommand) \
		+ sizeof(struct SSendUnitsCommand) \
		+ sizeof(struct SCreateGroupCommand) \
		+ sizeof(struct SEraseGroupCommand) \
		+ sizeof(struct SInitPathCommand) \
		+ sizeof(struct SGetApproximateLengthPathCommand) \
		+ sizeof(struct SGetNextWaypointPathCommand) \
		+ sizeof(struct SFreePathCommand) \
		+ sizeof(struct SCallLuaRulesCommand) \
		+ sizeof(struct SCallLuaUICommand) \
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
		+ sizeof(struct SLoadUnitsUnitCommand) \
		+ sizeof(struct SLoadUnitsAreaUnitCommand) \
		+ sizeof(struct SLoadOntoUnitCommand) \
		+ sizeof(struct SUnloadUnitCommand) \
		+ sizeof(struct SUnloadUnitsAreaUnitCommand) \
		+ sizeof(struct SSetOnOffUnitCommand) \
		+ sizeof(struct SReclaimUnitUnitCommand) \
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
		+ sizeof(struct SReclaimFeatureUnitCommand) \
		+ sizeof(struct SSetPositionGraphDrawerDebugCommand) \
		+ sizeof(struct SSetSizeGraphDrawerDebugCommand) \
		+ sizeof(struct SAddPointLineGraphDrawerDebugCommand) \
		+ sizeof(struct SDeletePointsLineGraphDrawerDebugCommand) \
		+ sizeof(struct SSetColorLineGraphDrawerDebugCommand) \
		+ sizeof(struct SSetLabelLineGraphDrawerDebugCommand) \
		+ sizeof(struct SAddOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SUpdateOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SDeleteOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SSetPositionOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SSetSizeOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SSetLabelOverlayTextureDrawerDebugCommand) \
		+ sizeof(struct SFeatureTraceRayCommand) \
		)

/**
 * Allows one to give an income (dis-)advantage to the team
 * controlled by the Skirmish AI.
 * This value can also be set through the GameSetup script,
 * with the difference that it causes an instant desync when set here.
 */
struct SSetMyIncomeMultiplierCheatCommand {
	/// default: 1.0; common: [0.0, 2.0]; valid: [0.0, FLOAT_MAX]
	float factor;
}; //$ COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER Cheats_setMyIncomeMultiplier

/**
 * The AI team receives the specified amount of units of the specified resource.
 */
struct SGiveMeResourceCheatCommand {
	int resourceId;
	float amount;
}; //$ COMMAND_CHEATS_GIVE_ME_RESOURCE Cheats_giveMeResource REF:resourceId->Resource

/**
 * Creates a new unit with the selected name at pos,
 * and returns its unit ID in ret_newUnitId.
 */
struct SGiveMeNewUnitCheatCommand {
	int unitDefId;
	float* pos_posF3;
	int ret_newUnitId;
}; //$ COMMAND_CHEATS_GIVE_ME_NEW_UNIT Cheats_giveMeUnit REF:unitDefId->UnitDef REF:ret_newUnitId->Unit

/**
 * @brief Sends a chat/text message to other players.
 * This text will also end up in infolog.txt.
 */
struct SSendTextMessageCommand {
	const char* text;
	int zone;
}; //$ COMMAND_SEND_TEXT_MESSAGE Game_sendTextMessage

/**
 * Assigns a map location to the last text message sent by the AI.
 */
struct SSetLastPosMessageCommand {
	float* pos_posF3;
}; //$ COMMAND_SET_LAST_POS_MESSAGE Game_setLastMessagePosition

/**
 * Give \<amount\> units of resource \<resourceId\> to team \<receivingTeam\>.
 * - the amount is capped to the AI team's resource levels
 * - does not check for alliance with \<receivingTeam\>
 * - LuaRules might not allow resource transfers, AI's must verify the deduction
 */
struct SSendResourcesCommand {
	int resourceId;
	float amount;
	int receivingTeamId;
	bool ret_isExecuted;
}; //$ COMMAND_SEND_RESOURCES Economy_sendResource REF:resourceId->Resource REF:receivingTeamId->Team

/**
 * Give units specified by \<unitIds\> to team \<receivingTeam\>.
 * \<ret_sentUnits\> represents how many actually were transferred.
 * Make sure this always matches the size of \<unitIds\> you passed in.
 * If it does not, then some unitId's were filtered out.
 * - does not check for alliance with \<receivingTeam\>
 * - AI's should check each unit if it is still under control of their
 *   team after the transaction via UnitTaken() and UnitGiven(), since
 *   LuaRules might block part of it
 */
struct SSendUnitsCommand {
	int* unitIds;
	int unitIds_size;
	int receivingTeamId;
	int ret_sentUnits;
}; //$ COMMAND_SEND_UNITS Economy_sendUnits REF:MULTI:unitIds->Unit REF:receivingTeamId->Team

/// Creates a group and returns the id it was given, returns -1 on failure
struct SCreateGroupCommand {
	int ret_groupId;
}; //$ COMMAND_GROUP_CREATE Group_create REF:ret_groupId->Group STATIC

/// Erases a specified group
struct SEraseGroupCommand {
	int groupId;
}; //$ COMMAND_GROUP_ERASE Group_erase REF:groupId->Group

/**
 * The following functions allow the AI to use the built-in path-finder.
 *
 * - call InitPath and you get a pathId back
 * - use this to call GetNextWaypoint to get subsequent waypoints;
 *   the waypoints are centered on 8*8 squares
 * - note that the pathfinder calculates the waypoints as needed,
 *   so do not retrieve them until they are needed
 * - the waypoint's x and z coordinates are returned in x and z,
 *   while y is used for status codes:
 *   y =  0: legal path waypoint IFF x >= 0 and z >= 0
 *   y = -1: temporary waypoint, path not yet available
 * - for pathType, @see UnitDef_MoveData_getPathType()
 * - goalRadius defines a goal area within which any square could be accepted as
 *   path target. If a singular goal position is wanted, use 0.0f.
 *   default: 8.0f
 */
struct SInitPathCommand {
	/// The starting location of the requested path
	float* start_posF3;
	/// The goal location of the requested path
	float* end_posF3;
	/// For what type of unit should the path be calculated
	int pathType;
	/// default: 8.0f
	float goalRadius;
	int ret_pathId;
}; //$ COMMAND_PATH_INIT Pathing_initPath REF:ret_pathId->Path

/**
 * Returns the approximate path cost between two points.
 * - for pathType @see UnitDef_MoveData_getPathType()
 * - goalRadius defines a goal area within which any square could be accepted as
 *   path target. If a singular goal position is wanted, use 0.0f.
 *   default: 8.0f
 */
struct SGetApproximateLengthPathCommand {
	/// The starting location of the requested path
	float* start_posF3;
	/// The goal location of the requested path
	float* end_posF3;
	/// For what type of unit should the path be calculated
	int pathType;
	/// default: 8.0f
	float goalRadius;
	float ret_approximatePathLength;
}; //$ COMMAND_PATH_GET_APPROXIMATE_LENGTH Pathing_getApproximateLength

struct SGetNextWaypointPathCommand {
	int pathId;
	float* ret_nextWaypoint_posF3_out;
}; //$ COMMAND_PATH_GET_NEXT_WAYPOINT Pathing_getNextWaypoint REF:pathId->Path

struct SFreePathCommand {
	int pathId;
}; //$ COMMAND_PATH_FREE Pathing_freePath REF:pathId->Path

struct SCallLuaRulesCommand {
	/// Can be set to NULL to skip passing in a string
	const char* inData;
	/// If this is less than 0, the data size is calculated using strlen()
	int inSize;
	/// Buffer for response, must be const size of MAX_RESPONSE_SIZE bytes
	char* ret_outData;
}; //$ COMMAND_CALL_LUA_RULES Lua_callRules

struct SCallLuaUICommand {
	/// Can be set to NULL to skip passing in a string
	const char* inData;
	/// If this is less than 0, the data size is calculated using strlen()
	int inSize;
	/// Buffer for response, must be const size of MAX_RESPONSE_SIZE bytes
	char* ret_outData;
}; //$ COMMAND_CALL_LUA_UI Lua_callUI

struct SSendStartPosCommand {
	bool ready;
	/// on this position, only x and z matter
	float* pos_posF3;
}; //$ COMMAND_SEND_START_POS Game_sendStartPosition

struct SAddNotificationDrawerCommand {
	/// on this position, only x and z matter
	float* pos_posF3;
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_ADD_NOTIFICATION Map_Drawer_addNotification

struct SAddPointDrawCommand {
	/// on this position, only x and z matter
	float* pos_posF3;
	/// create this text on pos in my team color
	const char* label;
}; //$ COMMAND_DRAWER_POINT_ADD Map_Drawer_addPoint

struct SRemovePointDrawCommand {
	/// remove map points and lines near this point (100 distance)
	float* pos_posF3;
}; //$ COMMAND_DRAWER_POINT_REMOVE Map_Drawer_deletePointsAndLines

struct SAddLineDrawCommand {
	/// draw line from this pos
	float* posFrom_posF3;
	/// to this pos, again only x and z matter
	float* posTo_posF3;
}; //$ COMMAND_DRAWER_LINE_ADD Map_Drawer_addLine

struct SStartPathDrawerCommand {
	float* pos_posF3;
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_PATH_START Map_Drawer_PathDrawer_start

struct SFinishPathDrawerCommand {
	// NOTE structs should not be empty (C90), so we add a useless member
	bool iAmUseless;
}; //$ COMMAND_DRAWER_PATH_FINISH Map_Drawer_PathDrawer_finish

struct SDrawLinePathDrawerCommand {
	float* endPos_posF3;
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_PATH_DRAW_LINE Map_Drawer_PathDrawer_drawLine

struct SDrawLineAndIconPathDrawerCommand {
	int cmdId;
	float* endPos_posF3;
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON Map_Drawer_PathDrawer_drawLineAndCommandIcon REF:cmdId->Command

struct SDrawIconAtLastPosPathDrawerCommand {
	int cmdId;
}; //$ COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS Map_Drawer_PathDrawer_drawIcon REF:cmdId->Command

struct SBreakPathDrawerCommand {
	float* endPos_posF3;
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_PATH_BREAK Map_Drawer_PathDrawer_suspend

struct SRestartPathDrawerCommand {
	bool sameColor;
}; //$ COMMAND_DRAWER_PATH_RESTART Map_Drawer_PathDrawer_restart


/**
 * @brief Creates a cubic Bezier spline figure
 * Creates a cubic Bezier spline figure from pos1 to pos4,
 * with control points pos2 and pos3.
 *
 * - Each figure is part of a figure group
 * - When creating figures, use 0 as \<figureGroupId\> to create
 *   a new figure group.
 *   The id of this figure group is returned in \<ret_newFigureGroupId\>
 * - \<lifeTime\> specifies how many frames a figure should live
 *   before being auto-removed; 0 means no removal
 * - \<arrow\> == true means that the figure will get an arrow at the end
 */
struct SCreateSplineFigureDrawerCommand {
	float* pos1_posF3;
	float* pos2_posF3;
	float* pos3_posF3;
	float* pos4_posF3;
	float width;
	/// true: means that the figure will get an arrow at the end
	bool arrow;
	/// how many frames a figure should live before being autoremoved, 0 means no removal
	int lifeTime;
	/// use 0 to get a new group
	int figureGroupId;
	/// the new group
	int ret_newFigureGroupId;
}; //$ COMMAND_DRAWER_FIGURE_CREATE_SPLINE Map_Drawer_Figure_drawSpline REF:figureGroupId->FigureGroup REF:ret_newFigureGroupId->FigureGroup

/**
 * @brief Creates a straight line
 * Creates a straight line from pos1 to pos2.
 *
 * - Each figure is part of a figure group
 * - When creating figures, use 0 as \<figureGroupId\> to create a new figure group.
 *   The id of this figure group is returned in \<ret_newFigureGroupId\>
 * @param lifeTime specifies how many frames a figure should live before being auto-removed;
 *                 0 means no removal
 * @param arrow true means that the figure will get an arrow at the end
 */
struct SCreateLineFigureDrawerCommand {
	float* pos1_posF3;
	float* pos2_posF3;
	float width;
	/// true: means that the figure will get an arrow at the end
	bool arrow;
	/// how many frames a figure should live before being autoremoved, 0 means no removal
	int lifeTime;
	/// use 0 to get a new group
	int figureGroupId;
	/// the new group
	int ret_newFigureGroupId;
}; //$ COMMAND_DRAWER_FIGURE_CREATE_LINE Map_Drawer_Figure_drawLine REF:figureGroupId->FigureGroup REF:ret_newFigureGroupId->FigureGroup

/**
 * Sets the color used to draw all lines of figures in a figure group.
 */
struct SSetColorFigureDrawerCommand {
	int figureGroupId;
	/// (x, y, z) -> (red, green, blue)
	short* color_colorS3;
	short alpha;
}; //$ COMMAND_DRAWER_FIGURE_SET_COLOR Map_Drawer_Figure_setColor REF:figureGroupId->FigureGroup

/**
 * Removes a figure group, which means it will not be drawn anymore.
 */
struct SDeleteFigureDrawerCommand {
	int figureGroupId;
}; //$ COMMAND_DRAWER_FIGURE_DELETE Map_Drawer_Figure_remove REF:figureGroupId->FigureGroup

/**
 * This function allows you to draw units onto the map.
 * - they only show up on the local player's screen
 * - they will be drawn in the "standard pose" (as if before any COB scripts are run)
 */
struct SDrawUnitDrawerCommand {
	int toDrawUnitDefId;
	float* pos_posF3;
	/// in radians
	float rotation;
	/// specifies how many frames a figure should live before being auto-removed; 0 means no removal
	int lifeTime;
	/// teamId affects the color of the unit
	int teamId;
	bool transparent;
	bool drawBorder;
	int facing;
}; //$ COMMAND_DRAWER_DRAW_UNIT Map_Drawer_drawUnit REF:toDrawUnitDefId->UnitDef



struct SBuildUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toBuildUnitDefId;
	float* buildPos_posF3;
	/// set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
	int facing;
}; //$ COMMAND_UNIT_BUILD Unit_build REF:toBuildUnitDefId->UnitDef

struct SStopUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_STOP Unit_stop

//struct SInsertUnitCommand {
//	int unitId;
//	int groupId;
//	short options; // see enum UnitCommandOptions
//	int timeOut; // command execution-time in ?seconds?
//};
//
//struct SRemoveUnitCommand {
//	int unitId;
//	int groupId;
//	short options; // see enum UnitCommandOptions
//	int timeOut; // command execution-time in ?seconds?
//};

struct SWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_WAIT Unit_wait

struct STimeWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// the time in seconds to wait
	int time;
}; //$ COMMAND_UNIT_WAIT_TIME Unit_waitFor

/**
 * Wait until another unit is dead, units will not wait on themselves.
 * Example:
 * A group of aircrafts waits for an enemy's anti-air defenses to die,
 * before passing over their ruins to attack.
 */
struct SDeathWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// wait until this unit is dead
	int toDieUnitId;
}; //$ COMMAND_UNIT_WAIT_DEATH Unit_waitForDeathOf REF:toDieUnitId->Unit

/**
 * Wait for a specific ammount of units.
 * Usually used with factories, but does work on groups without a factory too.
 * Example:
 * Pick a factory and give it a rallypoint, then add a SquadWait command
 * with the number of units you want in your squads.
 * Units will wait at the initial rally point until enough of them
 * have arrived to make up a squad, then they will continue along their queue.
 */
struct SSquadWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int numUnits;
}; //$ COMMAND_UNIT_WAIT_SQUAD Unit_waitForSquadSize

/**
 * Wait for the arrival of all units included in the command.
 * Only makes sense for a group of units.
 * Use it after a movement command of some sort (move / fight).
 * Units will wait until all members of the GatherWait command have arrived
 * at their destinations before continuing.
 */
struct SGatherWaitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_WAIT_GATHER Unit_waitForAll

struct SMoveUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toPos_posF3;
}; //$ COMMAND_UNIT_MOVE Unit_moveTo

struct SPatrolUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toPos_posF3;
}; //$ COMMAND_UNIT_PATROL Unit_patrolTo

struct SFightUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toPos_posF3;
}; //$ COMMAND_UNIT_FIGHT Unit_fight

struct SAttackUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toAttackUnitId;
}; //$ COMMAND_UNIT_ATTACK Unit_attack REF:toAttackUnitId->Unit

//	struct SAttackPosUnitCommand {

struct SAttackAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toAttackPos_posF3;
	float radius;
}; //$ COMMAND_UNIT_ATTACK_AREA Unit_attackArea

//struct SAttackAreaUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	short options;
//	int timeOut;
//};

struct SGuardUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toGuardUnitId;
}; //$ COMMAND_UNIT_GUARD Unit_guard REF:toGuardUnitId->Unit

// TODO: docu (is it usefull at all?)
struct SAiSelectUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_AI_SELECT Unit_aiSelect

//struct SGroupSelectUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	short options;
//	int timeOut;
//};

struct SGroupAddUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toGroupId;
}; //$ COMMAND_UNIT_GROUP_ADD Unit_addToGroup REF:toGroupId->Group

struct SGroupClearUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_GROUP_CLEAR Unit_removeFromGroup

struct SRepairUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toRepairUnitId;
}; //$ COMMAND_UNIT_REPAIR Unit_repair REF:toRepairUnitId->Unit

struct SSetFireStateUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// can be: 0=hold fire, 1=return fire, 2=fire at will
	int fireState;
}; //$ COMMAND_UNIT_SET_FIRE_STATE Unit_setFireState

struct SSetMoveStateUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// 0=hold pos, 1=maneuvre, 2=roam
	int moveState;
}; //$ COMMAND_UNIT_SET_MOVE_STATE Unit_setMoveState

struct SSetBaseUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* basePos_posF3;
}; //$ COMMAND_UNIT_SET_BASE Unit_setBase

//struct SInternalUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	short options;
//	int timeOut;
//};

struct SSelfDestroyUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_SELF_DESTROY Unit_selfDestruct

struct SLoadUnitsUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	int timeOut; // command execution-time in ?milli-seconds?

	int* toLoadUnitIds;
	int toLoadUnitIds_size;
}; //$ COMMAND_UNIT_LOAD_UNITS Unit_loadUnits REF:MULTI:toLoadUnitIds->Unit

struct SLoadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
	float radius;
}; //$ COMMAND_UNIT_LOAD_UNITS_AREA Unit_loadUnitsInArea

struct SLoadOntoUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int transporterUnitId;
}; //$ COMMAND_UNIT_LOAD_ONTO Unit_loadOnto REF:transporterUnitId->Unit

struct SUnloadUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toPos_posF3;
	int toUnloadUnitId;
}; //$ COMMAND_UNIT_UNLOAD_UNIT Unit_unload REF:toUnloadUnitId->Unit

struct SUnloadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* toPos_posF3;
	float radius;
}; //$ COMMAND_UNIT_UNLOAD_UNITS_AREA Unit_unloadUnitsInArea

struct SSetOnOffUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	bool on;
}; //$ COMMAND_UNIT_SET_ON_OFF Unit_setOn

struct SReclaimUnitUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toReclaimUnitId;
}; //$ COMMAND_UNIT_RECLAIM_UNIT Unit_reclaimUnit REF:toReclaimUnitId->Unit

struct SReclaimFeatureUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toReclaimFeatureId;
}; //$ COMMAND_UNIT_RECLAIM_FEATURE Unit_reclaimFeature REF:toReclaimFeatureId->Feature

struct SReclaimAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
	float radius;
}; //$ COMMAND_UNIT_RECLAIM_AREA Unit_reclaimInArea

struct SCloakUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	bool cloak;
}; //$ COMMAND_UNIT_CLOAK Unit_cloak

struct SStockpileUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;
}; //$ COMMAND_UNIT_STOCKPILE Unit_stockpile

struct SDGunUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toAttackUnitId;
}; //$ COMMAND_UNIT_D_GUN Unit_dGun REF:toAttackUnitId->Unit

struct SDGunPosUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
}; //$ COMMAND_UNIT_D_GUN_POS Unit_dGunPosition

struct SRestoreAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
	float radius;
}; //$ COMMAND_UNIT_RESTORE_AREA Unit_restoreArea

struct SSetRepeatUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	bool repeat;
}; //$ COMMAND_UNIT_SET_REPEAT Unit_setRepeat

/// Tells weapons that support it to try to use a high trajectory
struct SSetTrajectoryUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// 0: low-trajectory, 1: high-trajectory
	int trajectory;
}; //$ COMMAND_UNIT_SET_TRAJECTORY Unit_setTrajectory

struct SResurrectUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toResurrectFeatureId;
}; //$ COMMAND_UNIT_RESURRECT Unit_resurrect REF:toResurrectFeatureId->Feature

struct SResurrectAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
	float radius;
}; //$ COMMAND_UNIT_RESURRECT_AREA Unit_resurrectInArea

struct SCaptureUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int toCaptureUnitId;
}; //$ COMMAND_UNIT_CAPTURE Unit_capture REF:toCaptureUnitId->Unit

struct SCaptureAreaUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	float* pos_posF3;
	float radius;
}; //$ COMMAND_UNIT_CAPTURE_AREA Unit_captureInArea

/**
 * Set the percentage of health at which a unit will return to a save place.
 * This only works for a few units so far, mainly aircraft.
 */
struct SSetAutoRepairLevelUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// 0: 0%, 1: 30%, 2: 50%, 3: 80%
	int autoRepairLevel;
}; //$ COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL Unit_setAutoRepairLevel

//struct SAttackLoopbackUnitCommand {
//	int unitId;
//	int groupId;
//	/// see enum UnitCommandOptions
//	short options;
//	int timeOut;
//};

/**
 * Set what a unit should do when it is idle.
 * This only works for a few units so far, mainly aircraft.
 */
struct SSetIdleModeUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	/// 0: fly, 1: land
	int idleMode;
}; //$ COMMAND_UNIT_SET_IDLE_MODE Unit_setIdleMode

struct SCustomUnitCommand {
	int unitId;
	int groupId;
	/// see enum UnitCommandOptions
	short options;
	/**
	 * At which frame the command will time-out and consequently be removed,
	 * if execution of it has not yet begun.
	 * Can only be set locally, is not sent over the network, and is used
	 * for temporary orders.
	 * default: MAX_INT (-> do not time-out)
	 * example: currentFrame + 15
	 */
	int timeOut;

	int cmdId;
	float* params;
	int params_size;
}; //$ COMMAND_UNIT_CUSTOM Unit_executeCustomCommand ARRAY:params

// TODO: add docu
struct STraceRayCommand {
	float* rayPos_posF3;
	float* rayDir_posF3;
	float rayLen; // would also be ret, but we want only one ret per command
	int srcUnitId;
	int ret_hitUnitId;
	int flags;
}; //$ COMMAND_TRACE_RAY Map_Drawer_traceRay REF:srcUnitId->Unit REF:ret_hitUnitId->Unit

struct SFeatureTraceRayCommand {
	float* rayPos_posF3;
	float* rayDir_posF3;
	float rayLen; // would also be ret, but we want only one ret per command
	int srcUnitId;
	int ret_hitFeatureId;
	int flags;
}; //$ COMMAND_TRACE_RAY_FEATURE Map_Drawer_traceRayFeature REF:srcUnitId->Unit REF:ret_hitFeatureId->Feature

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
}; //$ COMMAND_PAUSE Game_setPause


struct SSetPositionGraphDrawerDebugCommand {
	float x;
	float y;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_SET_POS Debug_GraphDrawer_setPosition

struct SSetSizeGraphDrawerDebugCommand {
	float w;
	float h;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE Debug_GraphDrawer_setSize

struct SAddPointLineGraphDrawerDebugCommand {
	int lineId;
	float x;
	float y;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT Debug_GraphDrawer_GraphLine_addPoint

struct SDeletePointsLineGraphDrawerDebugCommand {
	int lineId;
	int numPoints;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS Debug_GraphDrawer_GraphLine_deletePoints

struct SSetColorLineGraphDrawerDebugCommand {
	int lineId;
	short* color_colorS3;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR Debug_GraphDrawer_GraphLine_setColor

struct SSetLabelLineGraphDrawerDebugCommand {
	int lineId;
	const char* label;
}; //$ COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL Debug_GraphDrawer_GraphLine_setLabel


struct SAddOverlayTextureDrawerDebugCommand {
	int ret_overlayTextureId;
	const float* texData;
	int w;
	int h;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD Debug_addOverlayTexture REF:ret_textureId->OverlayTexture

struct SUpdateOverlayTextureDrawerDebugCommand {
	int overlayTextureId;
	const float* texData;
	int x;
	int y;
	int w;
	int h;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE Debug_OverlayTexture_update

struct SDeleteOverlayTextureDrawerDebugCommand {
	int overlayTextureId;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE Debug_OverlayTexture_remove

struct SSetPositionOverlayTextureDrawerDebugCommand {
	int overlayTextureId;
	float x;
	float y;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS Debug_OverlayTexture_setPosition

struct SSetSizeOverlayTextureDrawerDebugCommand {
	int overlayTextureId;
	float w;
	float h;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE Debug_OverlayTexture_setSize

struct SSetLabelOverlayTextureDrawerDebugCommand {
	int overlayTextureId;
	const char* label;
}; //$ COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL Debug_OverlayTexture_setLabel



#ifdef	__cplusplus
}	// extern "C"
#endif



#ifdef	__cplusplus
struct Command;


// legacy support functions

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
 * @param  maxUnits  should be the value returned by unitHandler.MaxUnits()
 *                   -> max units per team for the current game
 */
int extractAICommandTopic(const Command* internalUnitCmd, int maxUnits);

/**
 * @brief fills an engine C++ Command struct
 */
bool newCommand(void* sUnitCommandData, int sCommandId, int maxUnits, Command* c);

#endif	// __cplusplus



#endif	// AI_S_COMMANDS_H
