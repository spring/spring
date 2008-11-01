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

#ifdef	__cplusplus
extern "C" {
#endif

#include "aidefines.h"
	

#define COMMAND_TO_ID_ENGINE -1
	
	
#define COMMAND_NULL 0
#define COMMAND_DRAWER_POINT_ADD 1
#define COMMAND_DRAWER_LINE_ADD 2
#define COMMAND_DRAWER_POINT_REMOVE 3
#define COMMAND_SEND_START_POS 4
#define COMMAND_CHEATS_SET_MY_HANDICAP 5
#define COMMAND_SEND_TEXT_MESSAGE 6
#define COMMAND_SET_LAST_POS_MESSAGE 7
#define COMMAND_SEND_RESOURCES 8
#define COMMAND_SEND_UNITS 9
#define COMMAND_SHARED_MEM_AREA_CREATE 10
#define COMMAND_SHARED_MEM_AREA_RELEASE 11
#define COMMAND_GROUP_CREATE 12
#define COMMAND_GROUP_ERASE 13
#define COMMAND_GROUP_ADD_UNIT 14
#define COMMAND_GROUP_REMOVE_UNIT 15
#define COMMAND_PATH_INIT 16
#define COMMAND_PATH_GET_APPROXIMATE_LENGTH 17
#define COMMAND_PATH_GET_NEXT_WAYPOINT 18
#define COMMAND_PATH_FREE 19
#define COMMAND_CHEATS_GIVE_ME_METAL 20
#define COMMAND_CALL_LUA_RULES 21
#define COMMAND_DRAWER_ADD_NOTIFICATION 22
#define COMMAND_DRAWER_DRAW_UNIT 23
#define COMMAND_DRAWER_PATH_START 24
#define COMMAND_DRAWER_PATH_FINISH 25
#define COMMAND_DRAWER_PATH_DRAW_LINE 26
#define COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON 27
#define COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS 28
#define COMMAND_DRAWER_PATH_BREAK 29
#define COMMAND_DRAWER_PATH_RESTART 30
#define COMMAND_DRAWER_FIGURE_CREATE_SPLINE 31
#define COMMAND_DRAWER_FIGURE_CREATE_LINE 32
#define COMMAND_DRAWER_FIGURE_SET_COLOR 33
#define COMMAND_DRAWER_FIGURE_DELETE 34
#define COMMAND_UNIT_BUILD 35
#define COMMAND_UNIT_STOP 36
#define COMMAND_UNIT_WAIT 37
#define COMMAND_UNIT_WAIT_TIME 38
#define COMMAND_UNIT_WAIT_DEATH 39
#define COMMAND_UNIT_WAIT_SQUAD 40
#define COMMAND_UNIT_WAIT_GATHER 41
#define COMMAND_UNIT_MOVE 42
#define COMMAND_UNIT_PATROL 43
#define COMMAND_UNIT_FIGHT 44
#define COMMAND_UNIT_ATTACK 45
#define COMMAND_UNIT_ATTACK_AREA 46
#define COMMAND_UNIT_GUARD 47
#define COMMAND_UNIT_AI_SELECT 48
#define COMMAND_UNIT_GROUP_ADD 49
#define COMMAND_UNIT_GROUP_CLEAR 50
#define COMMAND_UNIT_REPAIR 51
#define COMMAND_UNIT_SET_FIRE_STATE 52
#define COMMAND_UNIT_SET_MOVE_STATE 53
#define COMMAND_UNIT_SET_BASE 54
#define COMMAND_UNIT_SELF_DESTROY 55
#define COMMAND_UNIT_SET_WANTED_MAX_SPEED 56
#define COMMAND_UNIT_LOAD_UNITS 57
#define COMMAND_UNIT_LOAD_UNITS_AREA 58
#define COMMAND_UNIT_LOAD_ONTO 59
#define COMMAND_UNIT_UNLOAD_UNITS_AREA 60
#define COMMAND_UNIT_UNLOAD_UNIT 61
#define COMMAND_UNIT_SET_ON_OFF 62
#define COMMAND_UNIT_RECLAIM 63
#define COMMAND_UNIT_RECLAIM_AREA 64
#define COMMAND_UNIT_CLOAK 65
#define COMMAND_UNIT_STOCKPILE 66
#define COMMAND_UNIT_D_GUN 67
#define COMMAND_UNIT_D_GUN_POS 68
#define COMMAND_UNIT_RESTORE_AREA 69
#define COMMAND_UNIT_SET_REPEAT 70
#define COMMAND_UNIT_SET_TRAJECTORY 71
#define COMMAND_UNIT_RESURRECT 72
#define COMMAND_UNIT_RESURRECT_AREA 73
#define COMMAND_UNIT_CAPTURE 74
#define COMMAND_UNIT_CAPTURE_AREA 75
#define COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL 76
#define COMMAND_UNIT_SET_IDLE_MODE 77
#define COMMAND_UNIT_CUSTOM 78
//#define COMMAND_UNIT_ATTACK_POS 
//#define COMMAND_UNIT_INSERT 
//#define COMMAND_UNIT_REMOVE 
//#define COMMAND_UNIT_ATTACK_AREA 
//#define COMMAND_UNIT_ATTACK_LOOPBACK 
//#define COMMAND_UNIT_GROUP_SELECT 
//#define COMMAND_UNIT_INTERNAL 
#define COMMAND_CHEATS_GIVE_ME_ENERGY 79
#define COMMAND_CHEATS_GIVE_ME_NEW_UNIT 80

#define NUM_CMD_TOPICS 81

	
#define UNIT_COMMAND_OPTION_DONT_REPEAT	 (1 << 3) //   8
#define UNIT_COMMAND_OPTION_RIGHT_MOUSE_KEY (1 << 4) //  16
#define UNIT_COMMAND_OPTION_SHIFT_KEY	   (1 << 5) //  32
#define UNIT_COMMAND_OPTION_CONTROL_KEY	 (1 << 6) //  64
#define UNIT_COMMAND_OPTION_ALT_KEY		 (1 << 7) // 128


#define UNIT_COMMAND_BUILD_NO_FACING -1



struct SSetMyHandicapCheatCommand {
	float handicap;
};
struct SGiveMeMetalCheatCommand {
	float amount;
};
struct SGiveMeEnergyCheatCommand {
	float amount;
};
struct SGiveMeNewUnitCheatCommand {
	int unitDefId;
	struct SAIFloat3 pos;
	int ret_newUnitId;
};

struct SSendTextMessageCommand {
	const char* text;
	int zone;
};

struct SSetLastPosMessageCommand {
	struct SAIFloat3 pos;
};

struct SSendResourcesCommand {
	float mAmount;
	float eAmount;
	int receivingTeam;
	bool ret_isExecuted;
};

struct SSendUnitsCommand {
	int* unitIds;
	int numUnitIds;
	int receivingTeam;
	int ret_sentUnits;
};

struct SCreateSharedMemAreaCommand {
	char* name;
	int size;
	void* ret_sharedMemArea;
};

struct SReleaseSharedMemAreaCommand {
	char* name;
};

struct SCreateGroupCommand {
	const char* libraryName;
	unsigned int aiNumber;
	int ret_groupId;
};
struct SEraseGroupCommand {
	int groupId;
};
struct SAddUnitToGroupCommand {
	int groupId;
	int unitId;
	bool ret_isExecuted;
};
struct SRemoveUnitFromGroupCommand {
	int unitId;
	bool ret_isExecuted;
};


struct SInitPathCommand {
	struct SAIFloat3 start;
	struct SAIFloat3 end;
	int pathType;
	int ret_pathId;
};
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
};
struct SGetNextWaypointPathCommand {
	int pathId;
	struct SAIFloat3 ret_nextWaypoint;
};
struct SFreePathCommand {
	int pathId;
};

struct SCallLuaRulesCommand {
	const char* data;
	int inSize;
	int* outSize;
	const char* ret_outData;
};

struct SSendStartPosCommand {///< result of HandleCommand is 1 - ok supported
	bool ready;
	struct SAIFloat3 pos;
};

struct SAddNotificationDrawerCommand {
	struct SAIFloat3 pos;
	struct SAIFloat3 color;
	float alpha;
};
struct SAddPointDrawCommand ///< result of HandleCommand is 1 - ok supported
{
	struct SAIFloat3 pos; ///< on this position, only x and z matter
	const char* label; ///< create this text on pos in my team color
};
struct SRemovePointDrawCommand ///< result of HandleCommand is 1 - ok supported
{
	struct SAIFloat3 pos; ///< remove map points and lines near this point (100 distance)
};
struct SAddLineDrawCommand ///< result of HandleCommand is 1 - ok supported
{
	struct SAIFloat3 posFrom; ///< draw line from this pos
	struct SAIFloat3 posTo; ///< to this pos, again only x and z matter
};

struct SStartPathDrawerCommand {
	struct SAIFloat3 pos;
	struct SAIFloat3 color;
	float alpha;
};
struct SFinishPathDrawerCommand {
};
struct SDrawLinePathDrawerCommand {
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
};
struct SDrawLineAndIconPathDrawerCommand {
	int cmdId;
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
};
struct SDrawIconAtLastPosPathDrawerCommand {
	int cmdId;
};
struct SBreakPathDrawerCommand {
	struct SAIFloat3 endPos;
	struct SAIFloat3 color;
	float alpha;
};
struct SRestartPathDrawerCommand {
	bool sameColor;
};

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
};
struct SCreateLineFigureDrawerCommand {
	struct SAIFloat3 pos1;
	struct SAIFloat3 pos2;
	float width;
	bool arrow; // true: means that the figure will get an arrow at the end
	int lifeTime; // how many frames a figure should live before being autoremoved, 0 means no removal
	int figureGroupId; // use 0 to get a new group
	int ret_newFigureGroupId; // the new group
};
struct SSetColorFigureDrawerCommand {
	int figureGroupId;
	struct SAIFloat3 color;
	float alpha;
};
struct SDeleteFigureDrawerCommand {
	int figureGroupId;
};

struct SDrawUnitDrawerCommand {
	int toDrawUnitDefId;
	struct SAIFloat3 pos;
	float rotation;
	int lifeTime;
	int teamId;
	bool transparent;
	bool drawBorder;
	int facing;
};



struct SBuildUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toBuildUnitDefId;
	struct SAIFloat3 buildPos;
	int facing; // set to UNIT_COMMAND_BUILD_NO_FACING if you want to specify no facing
};

struct SStopUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

//	struct SInsertUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	
//
//	struct SRemoveUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	

struct SWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

struct STimeWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	// the time in seconds to wait
	int time;
};	

struct SDeathWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	// wait until this unit is dead
	int toDieUnitId;
};	

struct SSquadWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int numUnits;
};	

struct SGatherWaitUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

struct SMoveUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
};	

struct SPatrolUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
};	

struct SFightUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
};	

struct SAttackUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toAttackUnitId;
};	

//	struct SAttackPosUnitCommand {
struct SAttackAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toAttackPos;
	float radius;
};	

//	struct SAttackAreaUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	

struct SGuardUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toGuardUnitId;
};	

struct SAiSelectUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

//	struct SGroupSelectUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	

struct SGroupAddUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toGroupId;
};	

struct SGroupClearUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

struct SRepairUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toRepairUnitId;
};	

struct SSetFireStateUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	// can be: 0=hold fire, 1=return fire, 2=fire at will
	int fireState;
};	

struct SSetMoveStateUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int moveState;
};	

struct SSetBaseUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 basePos;
};	

//	struct SInternalUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	

struct SSelfDestroyUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

struct SSetWantedMaxSpeedUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	float wantedMaxSpeed;
};	

struct SLoadUnitsUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?milli-seconds?

	int* toLoadUnitIds;
	int numToLoadUnits;
};	

struct SLoadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
};	

struct SLoadOntoUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int transporterUnitId;
};	

struct SUnloadUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
	int toUnloadUnitId;
};	

struct SUnloadUnitsAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 toPos;
	float radius;
};	

struct SSetOnOffUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	bool on;
};	

struct SReclaimUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toReclaimUnitIdOrFeatureId;
};	

struct SReclaimAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
};	

struct SCloakUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	bool cloak;
};	

struct SStockpileUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?
};	

struct SDGunUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toAttackUnitId;
};	

struct SDGunPosUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
};	

struct SRestoreAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
};	

struct SSetRepeatUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	bool repeat;
};	

struct SSetTrajectoryUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int trajectory;
};	

struct SResurrectUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toResurrectFeatureId;
};	

struct SResurrectAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
};	

struct SCaptureUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int toCaptureUnitId;
};	

struct SCaptureAreaUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	struct SAIFloat3 pos;
	float radius;
};	

struct SSetAutoRepairLevelUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int autoRepairLevel;
};	

//	struct SAttackLoopbackUnitCommand {
//		int unitId;
//		int groupId;
//		unsigned int options; // see UNIT_COMMAND_OPTION_* defines
//		int timeOut; // command execution-time in ?seconds?
//	};	

struct SSetIdleModeUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int idleMode;
};	

struct SCustomUnitCommand {
	int unitId;
	int groupId;
	unsigned int options; // see UNIT_COMMAND_OPTION_* defines
	int timeOut; // command execution-time in ?seconds?

	int cmdId;
	float* params;
	int numParams;
};

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

