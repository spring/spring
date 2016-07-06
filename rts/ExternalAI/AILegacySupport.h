/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_LEGACY_SUPPORT_H
#define AI_LEGACY_SUPPORT_H

#include "System/float3.h"
#include "System/Color.h"

#include <string>

// GetProperty() constants ------------ data type that buffer will be filled with:
#define AIVAL_UNITDEF					1 // const UnitDef*
#define AIVAL_CURRENT_FUEL				2 // float
#define AIVAL_STOCKPILED				3 // int
#define AIVAL_STOCKPILE_QUED			4 // int
#define AIVAL_UNIT_MAXSPEED				5 // float

// GetValue() constants
#define AIVAL_NUMDAMAGETYPES			1 // int
#define AIVAL_MAP_CHECKSUM				3 // unsinged int
#define AIVAL_DEBUG_MODE				4 // bool
//#define AIVAL_GAME_MODE					5 // int // deprecated
#define AIVAL_GAME_PAUSED				6 // bool
#define AIVAL_GAME_SPEED_FACTOR			7 // float
#define AIVAL_GUI_VIEW_RANGE			8 // float
#define AIVAL_GUI_SCREENX				9 // float
#define AIVAL_GUI_SCREENY				10 // float
#define AIVAL_GUI_CAMERA_DIR			11 // float3
#define AIVAL_GUI_CAMERA_POS			12 // float3
#define AIVAL_LOCATE_FILE_R				15 // char*
#define AIVAL_LOCATE_FILE_W				16 // char*
#define AIVAL_UNIT_LIMIT				17 // int
#define AIVAL_SCRIPT					18 // const char* - buffer for pointer to char


#ifdef    BUILDING_AI
namespace springLegacyAI {
#endif // BUILDING_AI

struct UnitDef;
struct FeatureDef;
struct WeaponDef;


struct UnitResourceInfo
{
	float metalUse;
	float energyUse;
	float metalMake;
	float energyMake;
};

struct PointMarker
{
	float3 pos;

	SColor color;

	// don't store this pointer anywhere, it may become
	// invalid at any time after GetMapPoints()
	const char* label;
};

struct LineMarker {
	float3 pos;
	float3 pos2;

	SColor color;
};

// HandleCommand structs:

#define AIHCQuerySubVersionId   0
#define AIHCAddMapPointId       1
#define AIHCAddMapLineId        2
#define AIHCRemoveMapPointId    3
#define AIHCSendStartPosId      4
#define AIHCGetUnitDefByIdId    5
#define AIHCGetWeaponDefByIdId  6
#define AIHCGetFeatureDefByIdId 7
#define AIHCTraceRayId          8
#define AIHCPauseId             9
#define AIHCGetDataDirId        10
#define AIHCDebugDrawId         11
#define AIHCFeatureTraceRayId   12

struct AIHCAddMapPoint ///< result of HandleCommand is 1 - ok supported
{
	float3 pos; ///< on this position, only x and z matter
	const char* label; ///< create this text on pos in my team color
};

struct AIHCAddMapLine ///< result of HandleCommand is 1 - ok supported
{
	float3 posfrom; ///< draw line from this pos
	float3 posto; ///< to this pos, again only x and z matter
};

struct AIHCRemoveMapPoint ///< result of HandleCommand is 1 - ok supported
{
	float3 pos; ///< remove map points and lines near this point (100 distance)
};

struct AIHCSendStartPos ///< result of HandleCommand is 1 - ok supported
{
	bool ready;
	float3 pos;
};

struct AIHCGetUnitDefById ///< result of HandleCommand is 1 - ok supported
{
	int unitDefId;
	const UnitDef* ret;
};

struct AIHCGetWeaponDefById ///< result of HandleCommand is 1 - ok supported
{
	int weaponDefId;
	const WeaponDef* ret;
};

struct AIHCGetFeatureDefById ///< result of HandleCommand is 1 - ok supported
{
	int featureDefId;
	const FeatureDef* ret;
};

struct AIHCTraceRay
{
	float3 rayPos;
	float3 rayDir;
	float  rayLen;
	int    srcUID;
	int    hitUID;
	int    flags;
};

struct AIHCFeatureTraceRay
{
	float3 rayPos;
	float3 rayDir;
	float  rayLen;
	int    srcUID;
	int    hitFID;
	int    flags;
};

struct AIHCPause
{
	bool        enable;
	const char* reason;
};

/**
 * See Clb_DataDirs_allocatePath in rts/ExternalAI/Interface/SSkirmishAICallback.h
 * ret_path should be copied by the AI after retrieving it, but not freed!
 */
struct AIHCGetDataDir ///< result of HandleCommand is 1 for if path fetched, 0 for fail
{
	const char* relPath;
	bool writeable;
	bool create;
	bool dir;
	bool common;
	char* ret_path;
};

struct AIHCDebugDraw
{
	enum {
		AIHC_DEBUGDRAWER_MODE_ADD_GRAPH_POINT           =  0,
		AIHC_DEBUGDRAWER_MODE_DEL_GRAPH_POINTS          =  1,
		AIHC_DEBUGDRAWER_MODE_SET_GRAPH_POS             =  2,
		AIHC_DEBUGDRAWER_MODE_SET_GRAPH_SIZE            =  3,
		AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_COLOR      =  4,
		AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_LABEL      =  5,

		AIHC_DEBUGDRAWER_MODE_ADD_OVERLAY_TEXTURE       =  6,
		AIHC_DEBUGDRAWER_MODE_UPDATE_OVERLAY_TEXTURE    =  7,
		AIHC_DEBUGDRAWER_MODE_DEL_OVERLAY_TEXTURE       =  8,
		AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_POS   =  9,
		AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_SIZE  = 10,
		AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_LABEL = 11,
	};

	int cmdMode;
	float x, y;           // in-params
	float w, h;           // in-params
	int lineId;           // in-param
	int numPoints;        // in-param
	float3 color;         // in-param
	std::string label;    // in-param
	int texHandle;        // in/out-param
	const float* texData; // in-param
};

#ifdef    BUILDING_AI
} // namespace springLegacyAI
#endif // BUILDING_AI

#endif // AI_LEGACY_SUPPORT_H
