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

#ifndef _SKIRMISHAICALLBACK_H
#define	_SKIRMISHAICALLBACK_H

#include "aidefines.h"

#if	defined(__cplusplus)
extern "C" {
#endif


/**
 * @brief Skirmish AI Callback function pointers.
 * Each Skirmish AI instance will receive an instance of this struct
 * in its init(teamId) function and with the SInitEvent.
 *
 * This struct contians only activities that leave the game state as it is,
 * in spring terms: unsynced events
 * Activities that change game state (-> synced events) are handled through
 * AI commands, defined in AISCommands.h.
 *
 * The teamId passed as the first parameter to each function in this struct
 * has to be the ID of the team that is controlled by the AI instance
 * using the callback.
 */
struct SSkirmishAICallback {

/**
 * Whenever an AI wants to change the engine state in any way,
 * it has to call this method.
 * In other words, all commands from AIs to the engine (and other AIs)
 * go through this method.
 *
 * @param teamId       the team number of the AI that sends the command
 * @param toId         the team number of the AI that should receive
 *                     the command, or COMMAND_TO_ID_ENGINE if it is addressed
 *                     to the engine
 * @param commandId    used on asynchronous commands, this allows the AI to
 *                     identify a possible result event, which would come
 *                     with the same id
 * @param commandTopic unique identifier of a command
 *                     (see COMMAND_* defines in AISCommands.h)
 * @param commandData  a commandTopic specific struct, which contains
 *                     the data associated with the command
 *                     (see *Command structs)
 * @return     0: if command handling ok
 *          != 0: something else otherwise
 */
int (CALLING_CONV *Clb_Engine_handleCommand)(int teamId, int toId, int commandId,
		int commandTopic, void* commandData);

	/** Returns the major engine revision number (e.g. 0.77) */
	const char*       (CALLING_CONV *Clb_Engine_Version_getMajor)(int teamId);
	/** Returns the minor engine revision */
	const char*       (CALLING_CONV *Clb_Engine_Version_getMinor)(int teamId);
	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
	const char*       (CALLING_CONV *Clb_Engine_Version_getPatchset)(int teamId);
	/** Returns additional information (compiler flags, svn revision etc.) */
	const char*       (CALLING_CONV *Clb_Engine_Version_getAdditional)(int teamId);
	/** Returns the time of build */
	const char*       (CALLING_CONV *Clb_Engine_Version_getBuildTime)(int teamId);
	/** Returns "Major.Minor" */
	const char*       (CALLING_CONV *Clb_Engine_Version_getNormal)(int teamId);
	/** Returns "Major.Minor.Patchset (Additional)" */
	const char*       (CALLING_CONV *Clb_Engine_Version_getFull)(int teamId);

	/** Returns the number of teams in this game */
	int               (CALLING_CONV *Clb_Teams_getSize)(int teamId);

	/** Returns the number of skirmish AIs in this game */
	int               (CALLING_CONV *Clb_SkirmishAIs_getSize)(int teamId);
	/** Returns the maximum number of skirmish AIs in any game */
	int               (CALLING_CONV *Clb_SkirmishAIs_getMax)(int teamId);

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *Clb_SkirmishAI_Info_getSize)(int teamId);
	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_Info_getKey)(int teamId, int infoIndex);
	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_Info_getValue)(int teamId, int infoIndex);
	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_Info_getDescription)(int teamId, int infoIndex);
	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_Info_getValueByKey)(int teamId, const char* const key);

	/**
	 * Returns the number of option key-value pairs in the options map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *Clb_SkirmishAI_OptionValues_getSize)(int teamId);
	/**
	 * Returns the key at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_OptionValues_getKey)(int teamId, int optionIndex);
	/**
	 * Returns the value at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_OptionValues_getValue)(int teamId, int optionIndex);
	/**
	 * Returns the value associated with the given key in the options map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *Clb_SkirmishAI_OptionValues_getValueByKey)(int teamId, const char* const key);

	/** This will end up in infolog */
	void              (CALLING_CONV *Clb_Log_log)(int teamId, const char* const msg);
	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
	void               (CALLING_CONV *Clb_Log_exception)(int teamId, const char* const msg, int severety, bool die);

	/** Returns '/' on posix and '\\' on windows */
	char (CALLING_CONV *Clb_DataDirs_getPathSeparator)(int teamId);
	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *Clb_DataDirs_getConfigDir)(int teamId);
	/**
	 * This interfaces writeable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *Clb_DataDirs_getWriteableDir)(int teamId);
	/**
	 * Returns an absolute path which consists of:
	 * data-dir + Skirmish-AI-path + relative-path.
	 *
	 * example:
	 * input:  "log/main.log", writeable, create, !dir, !common
	 * output: "/home/userX/.spring/AI/Skirmish/RAI/0.601/log/main.log"
	 * The path "/home/userX/.spring/AI/Skirmish/RAI/0.601/log/" is created,
	 * if it does not yet exist.
	 *
	 * @see DataDirs_Roots_locatePath
	 * @param   path          store for the resulting absolute path
	 * @param   path_sizeMax  storage size of the above
	 * @param   writeable  if true, only the writeable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writeable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @param   common     if true, the version independent data-dir is formed,
	 *                     which uses "common" instead of the version, eg:
	 *                     "/home/userX/.spring/AI/Skirmish/RAI/common/..."
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
	bool              (CALLING_CONV *Clb_DataDirs_locatePath)(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);
	/**
	 * @see     locatePath()
	 */
	char*             (CALLING_CONV *Clb_DataDirs_allocatePath)(int teamId, const char* const relPath, bool writeable, bool create, bool dir, bool common);
	/// Returns the number of springs data dirs.
	int               (CALLING_CONV *Clb_DataDirs_Roots_getSize)(int teamId);
	/// Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1).
	bool              (CALLING_CONV *Clb_DataDirs_Roots_getDir)(int teamId, char* path, int path_sizeMax, int dirIndex);
	/**
	 * Returns an absolute path which consists of:
	 * data-dir + relative-path.
	 *
	 * example:
	 * input:  "AI/Skirmish", writeable, create, dir
	 * output: "/home/userX/.spring/AI/Skirmish/"
	 * The path "/home/userX/.spring/AI/Skirmish/" is created,
	 * if it does not yet exist.
	 *
	 * @see DataDirs_locatePath
	 * @param   path          store for the resulting absolute path
	 * @param   path_sizeMax  storage size of the above
	 * @param   relPath    the relative path to find
	 * @param   writeable  if true, only the writeable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writeable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
	bool              (CALLING_CONV *Clb_DataDirs_Roots_locatePath)(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
	char*             (CALLING_CONV *Clb_DataDirs_Roots_allocatePath)(int teamId, const char* const relPath, bool writeable, bool create, bool dir);

// BEGINN misc callback functions
/**
 * Returns the current game time measured in frames (the
 * simulation runs at 30 frames per second at normal speed)
 *
 * DEPRECATED: this should not be used, as we get the frame from the
 * SUpdateEvent
 */
int (CALLING_CONV *Clb_Game_getCurrentFrame)(int teamId);
int (CALLING_CONV *Clb_Game_getAiInterfaceVersion)(int teamId);
int (CALLING_CONV *Clb_Game_getMyTeam)(int teamId);
int (CALLING_CONV *Clb_Game_getMyAllyTeam)(int teamId);
int (CALLING_CONV *Clb_Game_getPlayerTeam)(int teamId, int playerId);
const char* (CALLING_CONV *Clb_Game_getTeamSide)(int teamId, int otherTeamId);
bool (CALLING_CONV *Clb_Game_isExceptionHandlingEnabled)(int teamId);
bool (CALLING_CONV *Clb_Game_isDebugModeEnabled)(int teamId);
int (CALLING_CONV *Clb_Game_getMode)(int teamId);
bool (CALLING_CONV *Clb_Game_isPaused)(int teamId);
float (CALLING_CONV *Clb_Game_getSpeedFactor)(int teamId);
const char* (CALLING_CONV *Clb_Game_getSetupScript)(int teamId);
// END misc callback functions


// BEGINN Visualization related callback functions
float (CALLING_CONV *Clb_Gui_getViewRange)(int teamId);
float (CALLING_CONV *Clb_Gui_getScreenX)(int teamId);
float (CALLING_CONV *Clb_Gui_getScreenY)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Gui_Camera_getDirection)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Gui_Camera_getPosition)(int teamId);
// END Visualization related callback functions


// BEGINN kind of deprecated; it is recommended not to use these
//bool (CALLING_CONV *Clb_getProperty)(int teamId, int id, int property,
//		void* dst);
//bool (CALLING_CONV *Clb_getValue)(int teamId, int id, void* dst);
// END kind of deprecated; it is recommended not to use these


// BEGINN OBJECT Cheats
bool (CALLING_CONV *Clb_Cheats_isEnabled)(int teamId);
bool (CALLING_CONV *Clb_Cheats_setEnabled)(int teamId, bool enable);
bool (CALLING_CONV *Clb_Cheats_setEventsEnabled)(int teamId, bool enabled);
bool (CALLING_CONV *Clb_Cheats_isOnlyPassive)(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT Resource
int (CALLING_CONV *Clb_0MULTI1SIZE0Resource)(int teamId);
int (CALLING_CONV *Clb_0MULTI1FETCH3ResourceByName0Resource)(int teamId,
		const char* resourceName);
const char* (CALLING_CONV *Clb_Resource_getName)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Resource_getOptimum)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getCurrent)(
		int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getIncome)(int teamId,
		int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getUsage)(int teamId,
		int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getStorage)(
		int teamId, int resourceId);
// END OBJECT Resource


// BEGINN OBJECT File
/// Return -1 when the file does not exist
int (CALLING_CONV *Clb_File_getSize)(int teamId, const char* fileName);
/// Returns false when file does not exist, or the buffer is too small
bool (CALLING_CONV *Clb_File_getContent)(int teamId, const char* fileName,
		void* buffer, int bufferLen);
// bool (CALLING_CONV *Clb_File_locateForReading)(int teamId, char* fileName, int fileName_sizeMax);
// bool (CALLING_CONV *Clb_File_locateForWriting)(int teamId, char* fileName, int fileName_sizeMax);
// END OBJECT File


// BEGINN OBJECT UnitDef
/**
 * A UnitDef contains all properties of a unit that are specific to its type,
 * for example the number and type of weapons or max-speed.
 * These properties are usually fixed, and not meant to change during a game.
 * The unitId is a unique id for this type of unit.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE0UnitDef)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0UnitDef)(int teamId, int unitDefIds[],
		int unitDefIds_max);
int (CALLING_CONV *Clb_0MULTI1FETCH3UnitDefByName0UnitDef)(int teamId,
		const char* unitName);
//int (CALLING_CONV *Clb_UnitDef_getId)(int teamId, int unitDefId);
/// Forces loading of the unit model
float (CALLING_CONV *Clb_UnitDef_getHeight)(int teamId, int unitDefId);
/// Forces loading of the unit model
float (CALLING_CONV *Clb_UnitDef_getRadius)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isValid)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getHumanName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getFileName)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getAiHint)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getCobId)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTechLevel)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getGaia)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getUpkeep)(int teamId,
		int unitDefId, int resourceId);
/** This amount of the resource will always be created. */
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getResourceMake)(
		int teamId, int unitDefId, int resourceId);
/**
 * This amount of the resource will be created when the unit is on and enough
 * energy can be drained.
 */
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getMakesResource)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getCost)(int teamId,
		int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getStorage)(
		int teamId, int unitDefId, int resourceId);
bool (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor)(
		int teamId, int unitDefId, int resourceId);
bool (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker)(
		int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_getBuildTime)(int teamId, int unitDefId);
/** This amount of auto-heal will always be applied. */
float (CALLING_CONV *Clb_UnitDef_getAutoHeal)(int teamId, int unitDefId);
/** This amount of auto-heal will only be applied while the unit is idling. */
float (CALLING_CONV *Clb_UnitDef_getIdleAutoHeal)(int teamId, int unitDefId);
/** Time a unit needs to idle before it is considered idling. */
int (CALLING_CONV *Clb_UnitDef_getIdleTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getPower)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getHealth)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *Clb_UnitDef_getCategory)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTurnRate)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTurnInPlace)(int teamId, int unitDefId);
/**
 * Units above this distance to goal will try to turn while keeping
 * some of their speed.
 * 0 to disable
 */
float (CALLING_CONV *Clb_UnitDef_getTurnInPlaceDistance)(int teamId, int unitDefId);
/**
 * Units below this speed will turn in place regardless of their
 * turnInPlace setting.
 */
float (CALLING_CONV *Clb_UnitDef_getTurnInPlaceSpeedLimit)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMoveType)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isUpright)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCollide)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getControlRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getAirLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getLosHeight)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getRadarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSonarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getJammerRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSonarJamRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSeismicRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSeismicSignature)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isSonarStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isBuildRange3D)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildDistance)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getReclaimSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getResurrectSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getCaptureSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTerraformSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMass)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isPushResistant)(int teamId, int unitDefId);
/** Should the unit move sideways when it can not shoot? */
bool (CALLING_CONV *Clb_UnitDef_isStrafeToAttack)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinCollisionSpeed)(int teamId,
		int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSlideTolerance)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxSlope)(int teamId, int unitDefId);
/**
 * Maximum terra-form height this building allows.
 * If this value is 0.0, you can only build this structure on
 * totally flat terrain.
 */
float (CALLING_CONV *Clb_UnitDef_getMaxHeightDif)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWaterline)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getArmoredMultiple)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getArmorType)(int teamId, int unitDefId);
/**
 * The flanking bonus indicates how much additional damage you can inflict to
 * a unit, if it gets attacked from different directions.
 * See the spring source code if you want to know it more precisely.
 *
 * @return  0: no flanking bonus
 *          1: global coords, mobile
 *          2: unit coords, mobile
 *          3: unit coords, locked
 */
int (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMode)(int teamId,
		int unitDefId);
/**
 * The unit takes less damage when attacked from this direction.
 * This encourage flanking fire.
 */
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_FlankingBonus_getDir)(int teamId,
		int unitDefId);
/** Damage factor for the least protected direction */
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMax)(int teamId,
		int unitDefId);
/** Damage factor for the most protected direction */
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMin)(int teamId,
		int unitDefId);
/**
 * How much the ability of the flanking bonus direction to move builds up each
 * frame.
 */
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMobilityAdd)(int teamId,
		int unitDefId);
/**
 * The type of the collision volume's form.
 *
 * @return  "Ell"
 *          "Cyl[T]" (where [T] is one of ['X', 'Y', 'Z'])
 *          "Box"
 */
const char* (CALLING_CONV *Clb_UnitDef_CollisionVolume_getType)(int teamId,
		int unitDefId);
/** The collision volume's full axis lengths. */
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_CollisionVolume_getScales)(
		int teamId, int unitDefId);
/** The collision volume's offset relative to the unit's center position */
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_CollisionVolume_getOffsets)(
		int teamId, int unitDefId);
/**
 * Collission test algorithm used.
 *
 * @return  0: discrete
 *          1: continuous
 */
int (CALLING_CONV *Clb_UnitDef_CollisionVolume_getTest)(int teamId,
		int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxWeaponRange)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getType)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getTooltip)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getWreckName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getDeathExplosion)(int teamId,
		int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getSelfDExplosion)(int teamId,
		int unitDefId);
// this might be changed later for something better
const char* (CALLING_CONV *Clb_UnitDef_getTedClassString)(int teamId,
		int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getCategoryString)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSelfD)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSelfDCountdown)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSubmerge)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFly)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToMove)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToHover)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFloater)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isBuilder)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isActivateWhenBuilt)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isOnOffable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFullHealthFactory)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFactoryHeadingTakeoff)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isReclaimable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCapturable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRestore)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSelfRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToReclaim)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToPatrol)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFight)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToGuard)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToAssist)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAssistable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRepeat)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFireControl)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFireState)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMoveState)(int teamId, int unitDefId);
// beginn: aircraft stuff
float (CALLING_CONV *Clb_UnitDef_getWingDrag)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWingAngle)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getDrag)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFrontToSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSpeedToFront)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMyGravity)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxBank)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxPitch)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTurnRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWantedHeight)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getVerticalSpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToCrash)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHoverAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAirStrafe)(int teamId, int unitDefId);
/**
 * @return  < 0:  it can land
 *          >= 0: how much the unit will move during hovering on the spot
 */
float (CALLING_CONV *Clb_UnitDef_getDlHoverFactor)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxDeceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxAileron)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxElevator)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxRudder)(int teamId, int unitDefId);
// end: aircraft stuff
// /* returned size is 4 */
//const unsigned char*[] (CALLING_CONV *Clb_UnitDef_getYardMaps)(int teamId,
//		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getXSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getZSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildAngle)(int teamId, int unitDefId);
// beginn: transports stuff
float (CALLING_CONV *Clb_UnitDef_getLoadingRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getUnloadSpread)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTransportCapacity)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTransportSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMinTransportSize)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAirBase)(int teamId, int unitDefId);
/**  */
bool (CALLING_CONV *Clb_UnitDef_isFirePlatform)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTransportMass)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinTransportMass)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHoldSteady)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isReleaseHeld)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isNotTransportable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTransportByEnemy)(int teamId, int unitDefId);
/**
 * @return  0: land unload
 *          1: fly-over drop
 *          2: land flood
 */
int (CALLING_CONV *Clb_UnitDef_getTransportUnloadMethod)(int teamId,
		int unitDefId);
/**
 * Dictates fall speed of all transported units.
 * This only makes sense for air transports,
 * if they an drop units while in the air.
 */
float (CALLING_CONV *Clb_UnitDef_getFallSpeed)(int teamId, int unitDefId);
/** Sets the transported units FBI, overrides fallSpeed */
float (CALLING_CONV *Clb_UnitDef_getUnitFallSpeed)(int teamId, int unitDefId);
// end: transports stuff
/** If the unit can cloak */
bool (CALLING_CONV *Clb_UnitDef_isAbleToCloak)(int teamId, int unitDefId);
/** If the unit wants to start out cloaked */
bool (CALLING_CONV *Clb_UnitDef_isStartCloaked)(int teamId, int unitDefId);
/** Energy cost per second to stay cloaked when stationary */
float (CALLING_CONV *Clb_UnitDef_getCloakCost)(int teamId, int unitDefId);
/** Energy cost per second to stay cloaked when moving */
float (CALLING_CONV *Clb_UnitDef_getCloakCostMoving)(int teamId, int unitDefId);
/** If enemy unit comes within this range, decloaking is forced */
float (CALLING_CONV *Clb_UnitDef_getDecloakDistance)(int teamId, int unitDefId);
/** Use a spherical, instead of a cylindrical test? */
bool (CALLING_CONV *Clb_UnitDef_isDecloakSpherical)(int teamId, int unitDefId);
/** Will the unit decloak upon firing? */
bool (CALLING_CONV *Clb_UnitDef_isDecloakOnFire)(int teamId, int unitDefId);
/** Will the unit self destruct if an enemy comes to close? */
bool (CALLING_CONV *Clb_UnitDef_isAbleToKamikaze)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getKamikazeDist)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTargetingFacility)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToDGun)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isNeedGeo)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFeature)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHideDamage)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCommander)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isShowPlayerName)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToResurrect)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToCapture)(int teamId, int unitDefId);
/**
 * Indicates the trajectory types supported by this unit.
 *
 * @return  0: (default) = only low
 *          1: only high
 *          2: choose
 */
int (CALLING_CONV *Clb_UnitDef_getHighTrajectoryType)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *Clb_UnitDef_getNoChaseCategory)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isLeaveTracks)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackWidth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackOffset)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackStrength)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackStretch)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTrackType)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToDropFlare)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareReloadTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareEfficiency)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareDelay)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_getFlareDropVector)(int teamId,
		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareTime)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareSalvoSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareSalvoDelay)(int teamId, int unitDefId);
/** Only matters for fighter aircraft */
bool (CALLING_CONV *Clb_UnitDef_isAbleToLoopbackAttack)(int teamId,
		int unitDefId);
/**
 * Indicates whether the ground will be leveled/flattened out
 * after this building has been built on it.
 * Only matters for buildings.
 */
bool (CALLING_CONV *Clb_UnitDef_isLevelGround)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isUseBuildingGroundDecal)(int teamId,
		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalType)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalSizeX)(int teamId,
		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalSizeY)(int teamId,
		int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildingDecalDecaySpeed)(int teamId,
		int unitDefId);
/**
 * Maximum flight time in seconds before the aircraft needs
 * to return to an air repair bay to refuel.
 */
float (CALLING_CONV *Clb_UnitDef_getMaxFuel)(int teamId, int unitDefId);
/** Time to fully refuel the unit */
float (CALLING_CONV *Clb_UnitDef_getRefuelTime)(int teamId, int unitDefId);
/** Minimum build power of airbases that this aircraft can land on */
float (CALLING_CONV *Clb_UnitDef_getMinAirBasePower)(int teamId, int unitDefId);
/** Number of units of this type allowed simultaneously in the game */
int (CALLING_CONV *Clb_UnitDef_getMaxThisUnit)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef)(int teamId,
		int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isDontLand)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef)(int teamId,
		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef)(
		int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions)(int teamId,
		int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions)(int teamId,
		int unitDefId, int unitDefIds[], int unitDefIds_max);
int (CALLING_CONV *Clb_UnitDef_0MAP1SIZE0getCustomParams)(int teamId,
		int unitDefId);
void (CALLING_CONV *Clb_UnitDef_0MAP1KEYS0getCustomParams)(int teamId,
		int unitDefId, const char* keys[]);
void (CALLING_CONV *Clb_UnitDef_0MAP1VALS0getCustomParams)(int teamId,
		int unitDefId, const char* values[]);



bool (CALLING_CONV *Clb_UnitDef_0AVAILABLE0MoveData)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxBreaking)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxSpeed)(int teamId, int unitDefId);
short (CALLING_CONV *Clb_UnitDef_MoveData_getMaxTurnRate)(int teamId, int unitDefId);

int (CALLING_CONV *Clb_UnitDef_MoveData_getSize)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxSlope)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getSlopeMod)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getDepthMod)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_MoveData_getPathType)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getCrushStrength)(int teamId, int unitDefId);
/** enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 }; */
int (CALLING_CONV *Clb_UnitDef_MoveData_getMoveType)(int teamId, int unitDefId);
/** enum MoveFamily { Tank=0, KBot=1, Hover=2, Ship=3 }; */
int (CALLING_CONV *Clb_UnitDef_MoveData_getMoveFamily)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_MoveData_getTerrainClass)(int teamId, int unitDefId);

bool (CALLING_CONV *Clb_UnitDef_MoveData_getFollowGround)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_MoveData_isSubMarine)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_MoveData_getName)(int teamId, int unitDefId);



int (CALLING_CONV *Clb_UnitDef_0MULTI1SIZE0WeaponMount)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_WeaponMount_getName)(int teamId,
		int unitDefId, int weaponMountId);
int (CALLING_CONV *Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef)(
		int teamId, int unitDefId, int weaponMountId);
int (CALLING_CONV *Clb_UnitDef_WeaponMount_getSlavedTo)(int teamId,
		int unitDefId, int weaponMountId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_WeaponMount_getMainDir)(int teamId,
		int unitDefId, int weaponMountId);
float (CALLING_CONV *Clb_UnitDef_WeaponMount_getMaxAngleDif)(int teamId,
		int unitDefId, int weaponMountId);
/**
 * How many seconds of fuel it costs for the owning unit to fire this weapon.
 */
float (CALLING_CONV *Clb_UnitDef_WeaponMount_getFuelUsage)(int teamId,
		int unitDefId, int weaponMountId);
unsigned int (CALLING_CONV *Clb_UnitDef_WeaponMount_getBadTargetCategory)(
		int teamId, int unitDefId, int weaponMountId);
unsigned int (CALLING_CONV *Clb_UnitDef_WeaponMount_getOnlyTargetCategory)(
		int teamId, int unitDefId, int weaponMountId);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
/**
 * Returns the numnber of units a team can have, after which it can not build
 * any more. It is possible that a team has more units then this value at some
 * point in the game. This is possible for example with taking, reclaiming or
 * capturing units.
 * This value is usefull for controlling game performance (eg. setting it could
 * possibly will prevent old PCs from lagging becuase of games with lots of
 * units).
 */
int (CALLING_CONV *Clb_Unit_0STATIC0getLimit)(int teamId);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are
 * in LOS.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnits0Unit)(int teamId, int unitIds[],
		int unitIds_max);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are
 * in LOS plus they have to be located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are in
 * some way visible (sight or radar).
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit)(int teamId,
		int unitIds[], int unitIds_max);
/**
 * Returns all units that are in this teams ally-team, including this teams
 * units.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3FriendlyUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3FriendlyUnits0Unit)(int teamId,
		int unitIds[], int unitIds_max);
/**
 * Returns all units that are in this teams ally-team, including this teams
 * units plus they have to be located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3FriendlyUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are neutral and are in LOS.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3NeutralUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3NeutralUnits0Unit)(int teamId, int unitIds[],
		int unitIds_max);
/**
 * Returns all units that are neutral and are in LOS plus they have to be
 * located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3NeutralUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3NeutralUnitsIn0Unit)(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are of the team controlled by this AI instance. This
 * list can also be created dynamically by the AI, through updating a list on
 * each unit-created and unit-destroyed event.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3TeamUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3TeamUnits0Unit)(int teamId, int unitIds[],
		int unitIds_max);
/**
 * Returns all units that are currently selected
 * (usually only contains units if a human payer
 * is controlling this team as well).
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3SelectedUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3SelectedUnits0Unit)(int teamId,
		int unitIds[], int unitIds_max);
/**
 * Returns the unit's unitdef struct from which you can read all
 * the statistics of the unit, do NOT try to change any values in it.
 */
int (CALLING_CONV *Clb_Unit_0SINGLE1FETCH2UnitDef0getDef)(int teamId,
		int unitId);
/**
 * This is a set of parameters that is initialized
 * in CreateUnitRulesParams() and may change during the game.
 * Each parameter is uniquely identified only by its id
 * (which is the index in the vector).
 * Parameters may or may not have a name.
 */
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE0ModParam)(int teamId, int unitId);
const char* (CALLING_CONV *Clb_Unit_ModParam_getName)(int teamId, int unitId,
		int modParamId);
float (CALLING_CONV *Clb_Unit_ModParam_getValue)(int teamId, int unitId,
		int modParamId);
int (CALLING_CONV *Clb_Unit_getTeam)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getAllyTeam)(int teamId, int unitId);
/**
 * The unit's origin lies in this team.
 *
 * example:
 * It was created by a factory that was created by a builder
 * from a factory built by a commander of this team.
 * It does not matter at all, to which team
 * the commander/builder/factories were shared.
 * Only capturing can break the chain.
 */
int (CALLING_CONV *Clb_Unit_getLineage)(int teamId, int unitId);
/**
 * Indicates the units main function.
 * This can be used as help for (skirmish) AIs.
 *
 * example:
 * A unit can shoot, build and transport other units.
 * To human players, it is obvious that transportation is the units
 * main function, as it can transport a lot of units,
 * but has only weak build- and fire-power.
 * Instead of letting the AI developers write complex
 * algorithms to find out the same, mod developers can set this value.
 *
 * @return  0: ???
 *          1: ???
 *          2: ???
 *          ...
 */
int (CALLING_CONV *Clb_Unit_getAiHint)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getStockpile)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getStockpileQueued)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getCurrentFuel)(int teamId, int unitId);
/// The unit's max speed
float (CALLING_CONV *Clb_Unit_getMaxSpeed)(int teamId, int unitId);
/// The furthest any weapon of the unit can fire
float (CALLING_CONV *Clb_Unit_getMaxRange)(int teamId, int unitId);
/// The unit's max health
float (CALLING_CONV *Clb_Unit_getMaxHealth)(int teamId, int unitId);
/// How experienced the unit is (0.0f-1.0f)
float (CALLING_CONV *Clb_Unit_getExperience)(int teamId, int unitId);
/// Returns the group a unit belongs to, -1 if none
int (CALLING_CONV *Clb_Unit_getGroup)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE1Command0CurrentCommand)(int teamId,
		int unitId);
/**
 * For the type of the command queue, see CCommandQueue::CommandQueueType
 * in Sim/Unit/CommandAI/CommandQueue.h
 */
int (CALLING_CONV *Clb_Unit_CurrentCommand_0STATIC0getType)(int teamId,
		int unitId);
/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
int (CALLING_CONV *Clb_Unit_CurrentCommand_getId)(int teamId, int unitId,
		int commandId);
unsigned char (CALLING_CONV *Clb_Unit_CurrentCommand_getOptions)(int teamId,
		int unitId, int commandId);
unsigned int (CALLING_CONV *Clb_Unit_CurrentCommand_getTag)(int teamId,
		int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_getTimeOut)(int teamId, int unitId,
		int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams)(int teamId,
		int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams)(int teamId,
		int unitId, int commandId, float params[], int params_max);
/// The commands that this unit can understand, other commands will be ignored
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE0SupportedCommand)(int teamId,
		int unitId);
/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
int (CALLING_CONV *Clb_Unit_SupportedCommand_getId)(int teamId, int unitId,
		int commandId);
const char* (CALLING_CONV *Clb_Unit_SupportedCommand_getName)(int teamId,
		int unitId, int commandId);
const char* (CALLING_CONV *Clb_Unit_SupportedCommand_getToolTip)(int teamId,
		int unitId, int commandId);
bool (CALLING_CONV *Clb_Unit_SupportedCommand_isShowUnique)(int teamId,
		int unitId, int commandId);
bool (CALLING_CONV *Clb_Unit_SupportedCommand_isDisabled)(int teamId,
		int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams)(int teamId,
		int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams)(int teamId,
		int unitId, int commandId, const char* params[], int params_max);

/// The unit's current health
float (CALLING_CONV *Clb_Unit_getHealth)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getSpeed)(int teamId, int unitId);
/**
 * Indicate the relative power of the unit,
 * used for experience calulations etc.
 * This is sort of the measure of the units overall power.
 */
float (CALLING_CONV *Clb_Unit_getPower)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE0ResourceInfo)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_0REF1Resource2resourceId0getResourceUse)(
		int teamId, int unitId, int resourceId);
float (CALLING_CONV *Clb_Unit_0REF1Resource2resourceId0getResourceMake)(
		int teamId, int unitId, int resourceId);
struct SAIFloat3 (CALLING_CONV *Clb_Unit_getPos)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isActivated)(int teamId, int unitId);
/// Returns true if the unit is currently being built
bool (CALLING_CONV *Clb_Unit_isBeingBuilt)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isCloaked)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isParalyzed)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isNeutral)(int teamId, int unitId);
/// Returns the unit's build facing (0-3)
int (CALLING_CONV *Clb_Unit_getBuildingFacing)(int teamId, int unitId);
/** Number of the last frame this unit received an order from a player. */
int (CALLING_CONV *Clb_Unit_getLastUserOrderFrame)(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
int (CALLING_CONV *Clb_0MULTI1SIZE0Group)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0Group)(int teamId, int groupIds[],
		int groupIds_max);
int (CALLING_CONV *Clb_Group_0MULTI1SIZE0SupportedCommand)(int teamId,
		int groupId);
/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
int (CALLING_CONV *Clb_Group_SupportedCommand_getId)(int teamId, int groupId,
		int commandId);
const char* (CALLING_CONV *Clb_Group_SupportedCommand_getName)(int teamId,
		int groupId, int commandId);
const char* (CALLING_CONV *Clb_Group_SupportedCommand_getToolTip)(int teamId,
		int groupId, int commandId);
bool (CALLING_CONV *Clb_Group_SupportedCommand_isShowUnique)(int teamId,
		int groupId, int commandId);
bool (CALLING_CONV *Clb_Group_SupportedCommand_isDisabled)(int teamId,
		int groupId, int commandId);
int (CALLING_CONV *Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams)(int teamId,
		int groupId, int commandId);
int (CALLING_CONV *Clb_Group_SupportedCommand_0ARRAY1VALS0getParams)(int teamId,
		int groupId, int commandId, const char* params[], int params_max);
/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
int (CALLING_CONV *Clb_Group_OrderPreview_getId)(int teamId, int groupId);
unsigned char (CALLING_CONV *Clb_Group_OrderPreview_getOptions)(int teamId,
		int groupId);
unsigned int (CALLING_CONV *Clb_Group_OrderPreview_getTag)(int teamId,
		int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_getTimeOut)(int teamId, int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_0ARRAY1SIZE0getParams)(int teamId,
		int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_0ARRAY1VALS0getParams)(int teamId,
		int groupId, float params[], int params_max);
bool (CALLING_CONV *Clb_Group_isSelected)(int teamId, int groupId);
// END OBJECT Group



// BEGINN OBJECT Mod

/**
 * archive filename
 */
const char*       (CALLING_CONV *Clb_Mod_getFileName)(int teamId);

/**
 * archive filename
 */
const char*       (CALLING_CONV *Clb_Mod_getHumanName)(int teamId);
const char*       (CALLING_CONV *Clb_Mod_getShortName)(int teamId);
const char*       (CALLING_CONV *Clb_Mod_getVersion)(int teamId);
const char*       (CALLING_CONV *Clb_Mod_getMutator)(int teamId);
const char*       (CALLING_CONV *Clb_Mod_getDescription)(int teamId);

bool              (CALLING_CONV *Clb_Mod_getAllowTeamColors)(int teamId);

/**
 * Should constructions without builders decay?
 */
bool              (CALLING_CONV *Clb_Mod_getConstructionDecay)(int teamId);
/**
 * How long until they start decaying?
 */
int               (CALLING_CONV *Clb_Mod_getConstructionDecayTime)(int teamId);
/**
 * How fast do they decay?
 */
float             (CALLING_CONV *Clb_Mod_getConstructionDecaySpeed)(int teamId);

/**
 * 0 = 1 reclaimer per feature max, otherwise unlimited
 */
int               (CALLING_CONV *Clb_Mod_getMultiReclaim)(int teamId);
/**
 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
 */
int               (CALLING_CONV *Clb_Mod_getReclaimMethod)(int teamId);
/**
 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
 */
int               (CALLING_CONV *Clb_Mod_getReclaimUnitMethod)(int teamId);
/**
 * How much energy should reclaiming a unit cost, default 0.0
 */
float             (CALLING_CONV *Clb_Mod_getReclaimUnitEnergyCostFactor)(int teamId);
/**
 * How much metal should reclaim return, default 1.0
 */
float             (CALLING_CONV *Clb_Mod_getReclaimUnitEfficiency)(int teamId);
/**
 * How much should energy should reclaiming a feature cost, default 0.0
 */
float             (CALLING_CONV *Clb_Mod_getReclaimFeatureEnergyCostFactor)(int teamId);
/**
 * Allow reclaiming enemies? default true
 */
bool              (CALLING_CONV *Clb_Mod_getReclaimAllowEnemies)(int teamId);
/**
 * Allow reclaiming allies? default true
 */
bool              (CALLING_CONV *Clb_Mod_getReclaimAllowAllies)(int teamId);

/**
 * How much should energy should repair cost, default 0.0
 */
float             (CALLING_CONV *Clb_Mod_getRepairEnergyCostFactor)(int teamId);

/**
 * How much should energy should resurrect cost, default 0.5
 */
float             (CALLING_CONV *Clb_Mod_getResurrectEnergyCostFactor)(int teamId);

/**
 * How much should energy should capture cost, default 0.0
 */
float             (CALLING_CONV *Clb_Mod_getCaptureEnergyCostFactor)(int teamId);

/**
 * 0 = all ground units cannot be transported, 1 = all ground units can be transported (mass and size restrictions still apply). Defaults to 1.
 */
int               (CALLING_CONV *Clb_Mod_getTransportGround)(int teamId);
/**
 * 0 = all hover units cannot be transported, 1 = all hover units can be transported (mass and size restrictions still apply). Defaults to 0.
 */
int               (CALLING_CONV *Clb_Mod_getTransportHover)(int teamId);
/**
 * 0 = all naval units cannot be transported, 1 = all naval units can be transported (mass and size restrictions still apply). Defaults to 0.
 */
int               (CALLING_CONV *Clb_Mod_getTransportShip)(int teamId);
/**
 * 0 = all air units cannot be transported, 1 = all air units can be transported (mass and size restrictions still apply). Defaults to 0.
 */
int               (CALLING_CONV *Clb_Mod_getTransportAir)(int teamId);

/**
 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
 */
int               (CALLING_CONV *Clb_Mod_getFireAtKilled)(int teamId);
/**
 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
 */
int               (CALLING_CONV *Clb_Mod_getFireAtCrashing)(int teamId);

/**
 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
 */
int               (CALLING_CONV *Clb_Mod_getFlankingBonusModeDefault)(int teamId);

/**
 * miplevel for los
 */
int               (CALLING_CONV *Clb_Mod_getLosMipLevel)(int teamId);
/**
 * miplevel to use for airlos
 */
int               (CALLING_CONV *Clb_Mod_getAirMipLevel)(int teamId);
/**
 * units sightdistance will be multiplied with this, for testing purposes
 */
float             (CALLING_CONV *Clb_Mod_getLosMul)(int teamId);
/**
 * units airsightdistance will be multiplied with this, for testing purposes
 */
float             (CALLING_CONV *Clb_Mod_getAirLosMul)(int teamId);
/**
 * when underwater, units are not in LOS unless also in sonar
 */
bool              (CALLING_CONV *Clb_Mod_getRequireSonarUnderWater)(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
unsigned int (CALLING_CONV *Clb_Map_getChecksum)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_getStartPos)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_getMousePos)(int teamId);
bool (CALLING_CONV *Clb_Map_isPosInCamera)(int teamId, struct SAIFloat3 pos,
		float radius);
/// Returns the maps width in full resolution
int (CALLING_CONV *Clb_Map_getWidth)(int teamId);
/// Returns the maps height in full resolution
int (CALLING_CONV *Clb_Map_getHeight)(int teamId);
/**
 * Returns the height for the center of the squares.
 * This differs slightly from the drawn map, since
 * that one uses the height at the corners.
 *
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 8*8 in size
 * - the value for the full resolution position (x, z) is at index (x/8 * width + z/8)
 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getHeightMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getHeightMap)(int teamId,
		float heights[], int heights_max);
float (CALLING_CONV *Clb_Map_getMinHeight)(int teamId);
float (CALLING_CONV *Clb_Map_getMaxHeight)(int teamId);
/**
 * @brief the slope map
 * The values are 1 minus the y-component of the (average) facenormal of the square.
 *
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 2*2 in size
 * - the value for the full resolution position (x, z) is at index (x/2 * width + z/2)
 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getSlopeMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getSlopeMap)(int teamId, float slopes[],
		int slopes_max);
/**
 * @brief the level of sight map
 * gs->mapx >> losMipLevel
 * A square with value zero means you do not have LOS coverage on it.
 *Clb_Mod_getLosMipLevel
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - resolution factor (res) is min(1, 1 << Clb_Mod_getLosMipLevel())
 *   examples:
 *   	+ losMipLevel(0) -> res(1)
 *   	+ losMipLevel(1) -> res(2)
 *   	+ losMipLevel(2) -> res(4)
 *   	+ losMipLevel(3) -> res(8)
 * - each data position is res*res in size
 * - the value for the full resolution position (x, z) is at index (x/res * width + z/res)
 * - the last value, bottom right, is at index (width/res * height/res - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getLosMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getLosMap)(int teamId,
		unsigned short losValues[], int losValues_max);
/**
 * @brief the radar map
 * A square with value 0 means you do not have radar coverage on it.
 *
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 8*8 in size
 * - the value for the full resolution position (x, z) is at index (x/8 * width + z/8)
 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getRadarMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getRadarMap)(int teamId,
		unsigned short radarValues[], int radarValues_max);
/**
 * @brief the radar jammer map
 * A square with value 0 means you do not have radar jamming coverage.
 *
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 8*8 in size
 * - the value for the full resolution position (x, z) is at index (x/8 * width + z/8)
 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getJammerMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getJammerMap)(int teamId,
		unsigned short jammerValues[], int jammerValues_max);
/**
 * @brief resource maps
 * This map shows the resource density on the map.
 *
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 2*2 in size
 * - the value for the full resolution position (x, z) is at index (x/2 * width + z/2)
 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw)(
		int teamId, int resourceId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw)(
		int teamId, int resourceId, unsigned char resources[], int resources_max);
/**
 * Returns positions indicating where to place resource extractors on the map.
 */
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapSpotsPositions)(
		int teamId, int resourceId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapSpotsPositions)(
		int teamId, int resourceId, struct SAIFloat3* spots, int spots_max);
/**
 * Returns the average resource income for an extractor on one of the evaluated positions.
 */
float (CALLING_CONV *Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsAverageIncome)(
		int teamId, int resourceId);
/**
 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
 */
struct SAIFloat3 (CALLING_CONV *Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsNearest)(
		int teamId, int resourceId, struct SAIFloat3 pos);
const char* (CALLING_CONV *Clb_Map_getName)(int teamId);
/// Gets the elevation of the map at position (x, z)
float (CALLING_CONV *Clb_Map_getElevationAt)(int teamId, float x, float z);
/// Returns what value 255 in the resource map is worth
float (CALLING_CONV *Clb_Map_0REF1Resource2resourceId0getMaxResource)(
		int teamId, int resourceId);
/// Returns extraction radius for resource extractors
float (CALLING_CONV *Clb_Map_0REF1Resource2resourceId0getExtractorRadius)(
		int teamId, int resourceId);
float (CALLING_CONV *Clb_Map_getMinWind)(int teamId);
float (CALLING_CONV *Clb_Map_getMaxWind)(int teamId);
float (CALLING_CONV *Clb_Map_getTidalStrength)(int teamId);
float (CALLING_CONV *Clb_Map_getGravity)(int teamId);
/**
 * Returns all points drawn with this AIs team color,
 * and additionally the ones drawn with allied team colors,
 * if <code>includeAllies</code> is true.
 */
int (CALLING_CONV *Clb_Map_0MULTI1SIZE0Point)(int teamId, bool includeAllies);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Point_getPosition)(int teamId,
		int pointId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Point_getColor)(int teamId,
		int pointId);
const char* (CALLING_CONV *Clb_Map_Point_getLabel)(int teamId, int pointId);
/**
 * Returns all lines drawn with this AIs team color,
 * and additionally the ones drawn with allied team colors,
 * if <code>includeAllies</code> is true.
 */
int (CALLING_CONV *Clb_Map_0MULTI1SIZE0Line)(int teamId, bool includeAllies);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getFirstPosition)(int teamId,
		int lineId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getSecondPosition)(int teamId,
		int lineId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getColor)(int teamId, int lineId);
bool (CALLING_CONV *Clb_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt)(
		int teamId, int unitDefId, struct SAIFloat3 pos, int facing);
/**
 * Returns the closest position from a given position that a building can be built at.
 * @param minDist the distance in squares that the building must keep to other buildings,
 *                to make it easier to keep free paths through a base
 */
struct SAIFloat3 (CALLING_CONV *Clb_Map_0REF1UnitDef2unitDefId0findClosestBuildSite)(int teamId,
		int unitDefId, struct SAIFloat3 pos, float searchRadius, int minDist,
		int facing);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
int (CALLING_CONV *Clb_0MULTI1SIZE0FeatureDef)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0FeatureDef)(int teamId, int featureDefIds[],
		int featureDefIds_max);
//int (CALLING_CONV *Clb_FeatureDef_getId)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getName)(int teamId,
		int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getDescription)(int teamId,
		int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getFileName)(int teamId,
		int featureDefId);
float (CALLING_CONV *Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource)(
		int teamId, int featureDefId, int resourceId);
float (CALLING_CONV *Clb_FeatureDef_getMaxHealth)(int teamId, int featureDefId);
float (CALLING_CONV *Clb_FeatureDef_getReclaimTime)(int teamId,
		int featureDefId);
/** Used to see if the object can be overrun by units of a certain heavyness */
float (CALLING_CONV *Clb_FeatureDef_getMass)(int teamId, int featureDefId);
/**
 * The type of the collision volume's form.
 *
 * @return  "Ell"
 *          "Cyl[T]" (where [T] is one of ['X', 'Y', 'Z'])
 *          "Box"
 */
const char* (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getType)(int teamId,
		int featureDefId);
/** The collision volume's full axis lengths. */
struct SAIFloat3 (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getScales)(
		int teamId, int featureDefId);
/** The collision volume's offset relative to the feature's center position */
struct SAIFloat3 (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getOffsets)(
		int teamId, int featureDefId);
/**
 * Collission test algorithm used.
 *
 * @return  0: discrete
 *          1: continuous
 */
int (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getTest)(int teamId,
		int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isUpright)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getDrawType)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getModelName)(int teamId,
		int featureDefId);
/**
 * Used to determine whether the feature is resurrectable.
 *
 * @return  -1: (default) only if it is the 1st wreckage of
 *              the UnitDef it originates from
 *           0: no, never
 *           1: yes, always
 */
int (CALLING_CONV *Clb_FeatureDef_getResurrectable)(int teamId,
		int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getSmokeTime)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isDestructable)(int teamId,
		int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isReclaimable)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isBlocking)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isBurnable)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isFloating)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isNoSelect)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isGeoThermal)(int teamId, int featureDefId);
/** Name of the FeatureDef that this turns into when killed (not reclaimed). */
const char* (CALLING_CONV *Clb_FeatureDef_getDeathFeature)(int teamId,
		int featureDefId);
/**
 * Size of the feature along the X axis - in other words: height.
 * each size is 8 units
 */
int (CALLING_CONV *Clb_FeatureDef_getXSize)(int teamId, int featureDefId);
/**
 * Size of the feature along the Z axis - in other words: width.
 * each size is 8 units
 */
int (CALLING_CONV *Clb_FeatureDef_getZSize)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_0MAP1SIZE0getCustomParams)(int teamId,
		int featureDefId);
void (CALLING_CONV *Clb_FeatureDef_0MAP1KEYS0getCustomParams)(int teamId,
		int featureDefId, const char* keys[]);
void (CALLING_CONV *Clb_FeatureDef_0MAP1VALS0getCustomParams)(int teamId,
		int featureDefId, const char* values[]);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
/**
 * Returns all features currently in LOS, or all features on the map
 * if cheating is enabled.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE0Feature)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0Feature)(int teamId, int featureIds[],
		int featureIds_max);
/**
 * Returns all features in a specified area that are currently in LOS,
 * or all features in this area if cheating is enabled.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3FeaturesIn0Feature)(int teamId,
		struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3FeaturesIn0Feature)(int teamId,
		struct SAIFloat3 pos, float radius, int featureIds[],
		int featureIds_max);
int (CALLING_CONV *Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef)(int teamId,
		int featureId);
float (CALLING_CONV *Clb_Feature_getHealth)(int teamId, int featureId);
float (CALLING_CONV *Clb_Feature_getReclaimLeft)(int teamId, int featureId);
struct SAIFloat3 (CALLING_CONV *Clb_Feature_getPosition)(int teamId,
		int featureId);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
int (CALLING_CONV *Clb_0MULTI1SIZE0WeaponDef)(int teamId);
int (CALLING_CONV *Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef)(int teamId,
		const char* weaponDefName);
const char* (CALLING_CONV *Clb_WeaponDef_getName)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getType)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getDescription)(int teamId,
		int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getFileName)(int teamId,
		int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getCegTag)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getRange)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getHeightMod)(int teamId, int weaponDefId);
/** Inaccuracy of whole burst */
float (CALLING_CONV *Clb_WeaponDef_getAccuracy)(int teamId, int weaponDefId);
/** Inaccuracy of individual shots inside burst */
float (CALLING_CONV *Clb_WeaponDef_getSprayAngle)(int teamId, int weaponDefId);
/** Inaccuracy while owner moving */
float (CALLING_CONV *Clb_WeaponDef_getMovingAccuracy)(int teamId,
		int weaponDefId);
/** Fraction of targets move speed that is used as error offset */
float (CALLING_CONV *Clb_WeaponDef_getTargetMoveError)(int teamId,
		int weaponDefId);
/** Maximum distance the weapon will lead the target */
float (CALLING_CONV *Clb_WeaponDef_getLeadLimit)(int teamId, int weaponDefId);
/** Factor for increasing the leadLimit with experience */
float (CALLING_CONV *Clb_WeaponDef_getLeadBonus)(int teamId, int weaponDefId);
/** Replaces hardcoded behaviour for burnblow cannons */
float (CALLING_CONV *Clb_WeaponDef_getPredictBoost)(int teamId,
		int weaponDefId);

// Deprecate the following function, if no longer needed by legacy Cpp AIs
int (CALLING_CONV *Clb_WeaponDef_0STATIC0getNumDamageTypes)(int teamId);
//DamageArray (CALLING_CONV *Clb_WeaponDef_getDamages)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_Damage_getParalyzeDamageTime)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getImpulseFactor)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getImpulseBoost)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getCraterMult)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getCraterBoost)(int teamId,
		int weaponDefId);
//float (CALLING_CONV *Clb_WeaponDef_Damage_getType)(int teamId, int weaponDefId, int typeId);
int (CALLING_CONV *Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes)(int teamId,
		int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes)(int teamId,
		int weaponDefId, float types[], int types_max);

//int (CALLING_CONV *Clb_WeaponDef_getId)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getAreaOfEffect)(int teamId,
		int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isNoSelfDamage)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getFireStarter)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getEdgeEffectiveness)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSize)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSizeGrowth)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCollisionSize)(int teamId,
		int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getSalvoSize)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSalvoDelay)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getReload)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBeamTime)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isBeamBurst)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isWaterBounce)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isGroundBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBounceRebound)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBounceSlip)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getNumBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMaxAngle)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getRestTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getUpTime)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getFlightTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_0REF1Resource2resourceId0getCost)(int teamId,
		int weaponDefId, int resourceId);
float (CALLING_CONV *Clb_WeaponDef_getSupplyCost)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getProjectilesPerShot)(int teamId,
		int weaponDefId);
// /** The "id=" tag in the TDF */
//int (CALLING_CONV *Clb_WeaponDef_getTdfId)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isTurret)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isOnlyForward)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isFixedLauncher)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isWaterWeapon)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isFireSubmersed)(int teamId, int weaponDefId);
/** Lets a torpedo travel above water like it does below water */
bool (CALLING_CONV *Clb_WeaponDef_isSubMissile)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isTracks)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isDropped)(int teamId, int weaponDefId);
/** The weapon will only paralyze, not do real damage. */
bool (CALLING_CONV *Clb_WeaponDef_isParalyzer)(int teamId, int weaponDefId);
/** The weapon damages by impacting, not by exploding. */
bool (CALLING_CONV *Clb_WeaponDef_isImpactOnly)(int teamId, int weaponDefId);
/** Can not target anything (for example: anti-nuke, D-Gun) */
bool (CALLING_CONV *Clb_WeaponDef_isNoAutoTarget)(int teamId, int weaponDefId);
/** Has to be fired manually (by the player or an AI, example: D-Gun) */
bool (CALLING_CONV *Clb_WeaponDef_isManualFire)(int teamId, int weaponDefId);
/**
 * Can intercept targetable weapons shots.
 *
 * example: anti-nuke
 *
 * @see  getTargetable()
 */
int (CALLING_CONV *Clb_WeaponDef_getInterceptor)(int teamId, int weaponDefId);
/**
 * Shoots interceptable projectiles.
 * Shots can be intercepted by interceptors.
 *
 * example: nuke
 *
 * @see  getInterceptor()
 */
int (CALLING_CONV *Clb_WeaponDef_getTargetable)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isStockpileable)(int teamId, int weaponDefId);
/**
 * Range of interceptors.
 *
 * example: anti-nuke
 *
 * @see  getInterceptor()
 */
float (CALLING_CONV *Clb_WeaponDef_getCoverageRange)(int teamId,
		int weaponDefId);
/** Build time of a missile */
float (CALLING_CONV *Clb_WeaponDef_getStockpileTime)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getIntensity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getThickness)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getLaserFlareSize)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCoreThickness)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDuration)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getLodDistance)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getFalloffRate)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getGraphicsType)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSoundTrigger)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSelfExplode)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isGravityAffected)(int teamId,
		int weaponDefId);
/**
 * Per weapon high trajectory setting.
 * UnitDef also has this property.
 *
 * @return  0: low
 *          1: high
 *          2: unit
 */
int (CALLING_CONV *Clb_WeaponDef_getHighTrajectory)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMyGravity)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isNoExplode)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getStartVelocity)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getWeaponAcceleration)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getTurnRate)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMaxVelocity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getProjectileSpeed)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getExplosionSpeed)(int teamId,
		int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_getOnlyTargetCategory)(int teamId,
		int weaponDefId);
/** How much the missile will wobble around its course. */
float (CALLING_CONV *Clb_WeaponDef_getWobble)(int teamId, int weaponDefId);
/** How much the missile will dance. */
float (CALLING_CONV *Clb_WeaponDef_getDance)(int teamId, int weaponDefId);
/** How high trajectory missiles will try to fly in. */
float (CALLING_CONV *Clb_WeaponDef_getTrajectoryHeight)(int teamId,
		int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isLargeBeamLaser)(int teamId,
		int weaponDefId);
/** If the weapon is a shield rather than a weapon. */
bool (CALLING_CONV *Clb_WeaponDef_isShield)(int teamId, int weaponDefId);
/** If the weapon should be repulsed or absorbed. */
bool (CALLING_CONV *Clb_WeaponDef_isShieldRepulser)(int teamId,
		int weaponDefId);
/** If the shield only affects enemy projectiles. */
bool (CALLING_CONV *Clb_WeaponDef_isSmartShield)(int teamId, int weaponDefId);
/** If the shield only affects stuff coming from outside shield radius. */
bool (CALLING_CONV *Clb_WeaponDef_isExteriorShield)(int teamId,
		int weaponDefId);
/** If the shield should be graphically shown. */
bool (CALLING_CONV *Clb_WeaponDef_isVisibleShield)(int teamId, int weaponDefId);
/** If a small graphic should be shown at each repulse. */
bool (CALLING_CONV *Clb_WeaponDef_isVisibleShieldRepulse)(int teamId,
		int weaponDefId);
/** The number of frames to draw the shield after it has been hit. */
int (CALLING_CONV *Clb_WeaponDef_getVisibleShieldHitFrames)(int teamId,
		int weaponDefId);
/**
 * Amount of the resource used per shot or per second,
 * depending on the type of projectile.
 */
float (CALLING_CONV *Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse)(
		int teamId, int weaponDefId, int resourceId);
/** Size of shield covered area */
float (CALLING_CONV *Clb_WeaponDef_Shield_getRadius)(int teamId,
		int weaponDefId);
/**
 * Shield acceleration on plasma stuff.
 * How much will plasma be accelerated into the other direction
 * when it hits the shield.
 */
float (CALLING_CONV *Clb_WeaponDef_Shield_getForce)(int teamId,
		int weaponDefId);
/** Maximum speed to which the shield can repulse plasma. */
float (CALLING_CONV *Clb_WeaponDef_Shield_getMaxSpeed)(int teamId,
		int weaponDefId);
/** Amount of damage the shield can reflect. (0=infinite) */
float (CALLING_CONV *Clb_WeaponDef_Shield_getPower)(int teamId,
		int weaponDefId);
/** Amount of power that is regenerated per second. */
float (CALLING_CONV *Clb_WeaponDef_Shield_getPowerRegen)(int teamId,
		int weaponDefId);
/**
 * How much of a given resource is needed to regenerate power
 * with max speed per second.
 */
float (CALLING_CONV *Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource)(
		int teamId, int weaponDefId, int resourceId);
/** How much power the shield has when it is created. */
float (CALLING_CONV *Clb_WeaponDef_Shield_getStartingPower)(int teamId,
		int weaponDefId);
/** Number of frames to delay recharging by after each hit. */
int (CALLING_CONV *Clb_WeaponDef_Shield_getRechargeDelay)(int teamId,
		int weaponDefId);
/** The color of the shield when it is at full power. */
struct SAIFloat3 (CALLING_CONV *Clb_WeaponDef_Shield_getGoodColor)(int teamId,
		int weaponDefId);
/** The color of the shield when it is empty. */
struct SAIFloat3 (CALLING_CONV *Clb_WeaponDef_Shield_getBadColor)(int teamId,
		int weaponDefId);
/** The shields alpha value. */
float (CALLING_CONV *Clb_WeaponDef_Shield_getAlpha)(int teamId,
		int weaponDefId);
/**
 * The type of the shield (bitfield).
 * Defines what weapons can be intercepted by the shield.
 *
 * @see  getInterceptedByShieldType()
 */
unsigned int (CALLING_CONV *Clb_WeaponDef_Shield_getInterceptType)(int teamId,
		int weaponDefId);
/**
 * The type of shields that can intercept this weapon (bitfield).
 * The weapon can be affected by shields if:
 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
 *
 * @see  getInterceptType()
 */
unsigned int (CALLING_CONV *Clb_WeaponDef_getInterceptedByShieldType)(
		int teamId, int weaponDefId);
/** Tries to avoid friendly units while aiming? */
bool (CALLING_CONV *Clb_WeaponDef_isAvoidFriendly)(int teamId, int weaponDefId);
/** Tries to avoid features while aiming? */
bool (CALLING_CONV *Clb_WeaponDef_isAvoidFeature)(int teamId, int weaponDefId);
/** Tries to avoid neutral units while aiming? */
bool (CALLING_CONV *Clb_WeaponDef_isAvoidNeutral)(int teamId, int weaponDefId);
/**
 * If nonzero, targetting units will TryTarget at the edge of collision sphere
 * (radius*tag value, [-1;1]) instead of its centre.
 */
float (CALLING_CONV *Clb_WeaponDef_getTargetBorder)(int teamId,
		int weaponDefId);
/**
 * If greater than 0, the range will be checked in a cylinder
 * (height=range*cylinderTargetting) instead of a sphere.
 */
float (CALLING_CONV *Clb_WeaponDef_getCylinderTargetting)(int teamId,
		int weaponDefId);
/**
 * For beam-lasers only - always hit with some minimum intensity
 * (a damage coeffcient normally dependent on distance).
 * Do not confuse this with the intensity tag, it i completely unrelated.
 */
float (CALLING_CONV *Clb_WeaponDef_getMinIntensity)(int teamId,
		int weaponDefId);
/**
 * Controls cannon range height boost.
 *
 * default: -1: automatically calculate a more or less sane value
 */
float (CALLING_CONV *Clb_WeaponDef_getHeightBoostFactor)(int teamId,
		int weaponDefId);
/** Multiplier for the distance to the target for priority calculations. */
float (CALLING_CONV *Clb_WeaponDef_getProximityPriority)(int teamId,
		int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_getCollisionFlags)(int teamId,
		int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSweepFire)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isAbleToAttackGround)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCameraShake)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageExp)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageMin)(int teamId,
		int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageRange)(int teamId,
		int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isDynDamageInverted)(int teamId,
		int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_0MAP1SIZE0getCustomParams)(int teamId,
		int weaponDefId);
void (CALLING_CONV *Clb_WeaponDef_0MAP1KEYS0getCustomParams)(int teamId,
		int weaponDefId, const char* keys[]);
void (CALLING_CONV *Clb_WeaponDef_0MAP1VALS0getCustomParams)(int teamId,
		int weaponDefId, const char* values[]);
// END OBJECT WeaponDef

};

#if	defined(__cplusplus)
} // extern "C"
#endif

#endif // _SKIRMISHAICALLBACK_H
