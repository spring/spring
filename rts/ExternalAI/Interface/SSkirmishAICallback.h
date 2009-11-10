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
	int               (CALLING_CONV *Engine_handleCommand)(int teamId, int toId, int commandId, int commandTopic, void* commandData);

	/** Returns the major engine revision number (e.g. 0.77) */
	const char*       (CALLING_CONV *Engine_Version_getMajor)(int teamId);
	/** Returns the minor engine revision */
	const char*       (CALLING_CONV *Engine_Version_getMinor)(int teamId);
	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
	const char*       (CALLING_CONV *Engine_Version_getPatchset)(int teamId);
	/** Returns additional information (compiler flags, svn revision etc.) */
	const char*       (CALLING_CONV *Engine_Version_getAdditional)(int teamId);
	/** Returns the time of build */
	const char*       (CALLING_CONV *Engine_Version_getBuildTime)(int teamId);
	/** Returns "Major.Minor" */
	const char*       (CALLING_CONV *Engine_Version_getNormal)(int teamId);
	/** Returns "Major.Minor.Patchset (Additional)" */
	const char*       (CALLING_CONV *Engine_Version_getFull)(int teamId);

	/** Returns the number of teams in this game */
	int               (CALLING_CONV *Teams_getSize)(int teamId);

	/** Returns the number of skirmish AIs in this game */
	int               (CALLING_CONV *SkirmishAIs_getSize)(int teamId);
	/** Returns the maximum number of skirmish AIs in any game */
	int               (CALLING_CONV *SkirmishAIs_getMax)(int teamId);

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *SkirmishAI_Info_getSize)(int teamId);
	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getKey)(int teamId, int infoIndex);
	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getValue)(int teamId, int infoIndex);
	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getDescription)(int teamId, int infoIndex);
	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getValueByKey)(int teamId, const char* const key);

	/**
	 * Returns the number of option key-value pairs in the options map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *SkirmishAI_OptionValues_getSize)(int teamId);
	/**
	 * Returns the key at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getKey)(int teamId, int optionIndex);
	/**
	 * Returns the value at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getValue)(int teamId, int optionIndex);
	/**
	 * Returns the value associated with the given key in the options map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getValueByKey)(int teamId, const char* const key);

	/** This will end up in infolog */
	void              (CALLING_CONV *Log_log)(int teamId, const char* const msg);
	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
	void               (CALLING_CONV *Log_exception)(int teamId, const char* const msg, int severety, bool die);

	/** Returns '/' on posix and '\\' on windows */
	char               (CALLING_CONV *DataDirs_getPathSeparator)(int teamId);
	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *DataDirs_getConfigDir)(int teamId);
	/**
	 * This interfaces writeable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *DataDirs_getWriteableDir)(int teamId);
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
	bool              (CALLING_CONV *DataDirs_locatePath)(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);
	/**
	 * @see     locatePath()
	 */
	char*             (CALLING_CONV *DataDirs_allocatePath)(int teamId, const char* const relPath, bool writeable, bool create, bool dir, bool common);
	/** Returns the number of springs data dirs. */
	int               (CALLING_CONV *DataDirs_Roots_getSize)(int teamId);
	/** Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1). */
	bool              (CALLING_CONV *DataDirs_Roots_getDir)(int teamId, char* path, int path_sizeMax, int dirIndex);
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
	bool              (CALLING_CONV *DataDirs_Roots_locatePath)(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
	char*             (CALLING_CONV *DataDirs_Roots_allocatePath)(int teamId, const char* const relPath, bool writeable, bool create, bool dir);

// BEGINN misc callback functions
	/**
	 * Returns the current game time measured in frames (the
	 * simulation runs at 30 frames per second at normal speed)
	 *
	 * This should not be used, as we get the frame from the SUpdateEvent.
	 * @deprecated
	 */
	int               (CALLING_CONV *Game_getCurrentFrame)(int teamId);
	int               (CALLING_CONV *Game_getAiInterfaceVersion)(int teamId);
	int               (CALLING_CONV *Game_getMyTeam)(int teamId);
	int               (CALLING_CONV *Game_getMyAllyTeam)(int teamId);
	int               (CALLING_CONV *Game_getPlayerTeam)(int teamId, int playerId);
	const char*       (CALLING_CONV *Game_getTeamSide)(int teamId, int otherTeamId);
	bool              (CALLING_CONV *Game_isExceptionHandlingEnabled)(int teamId);
	bool              (CALLING_CONV *Game_isDebugModeEnabled)(int teamId);
	int               (CALLING_CONV *Game_getMode)(int teamId);
	bool              (CALLING_CONV *Game_isPaused)(int teamId);
	float             (CALLING_CONV *Game_getSpeedFactor)(int teamId);
	const char*       (CALLING_CONV *Game_getSetupScript)(int teamId);
// END misc callback functions


// BEGINN Visualization related callback functions
	float             (CALLING_CONV *Gui_getViewRange)(int teamId);
	float             (CALLING_CONV *Gui_getScreenX)(int teamId);
	float             (CALLING_CONV *Gui_getScreenY)(int teamId);
	void              (CALLING_CONV *Gui_Camera_getDirection)(int teamId, float* return_posF3_out);
	void              (CALLING_CONV *Gui_Camera_getPosition)(int teamId, float* return_posF3_out);
// END Visualization related callback functions


// BEGINN kind of deprecated; it is recommended not to use these
//	bool              (CALLING_CONV *getProperty)(int teamId, int id, int property, void* dst);
//	bool              (CALLING_CONV *getValue)(int teamId, int id, void* dst);
// END kind of deprecated; it is recommended not to use these


// BEGINN OBJECT Cheats
	bool              (CALLING_CONV *Cheats_isEnabled)(int teamId);
	bool              (CALLING_CONV *Cheats_setEnabled)(int teamId, bool enable);
	bool              (CALLING_CONV *Cheats_setEventsEnabled)(int teamId, bool enabled);
	bool              (CALLING_CONV *Cheats_isOnlyPassive)(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT Resource
	int               (CALLING_CONV *getResources)(int teamId); // FETCHER:MULTI:NUM:Resource
	int               (CALLING_CONV *getResourceByName)(int teamId, const char* resourceName); // FETCHER:SINGLE:Resource
	const char*       (CALLING_CONV *Resource_getName)(int teamId, int resourceId);
	float             (CALLING_CONV *Resource_getOptimum)(int teamId, int resourceId);
	float             (CALLING_CONV *Economy_getCurrent)(int teamId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *Economy_getIncome)(int teamId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *Economy_getUsage)(int teamId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *Economy_getStorage)(int teamId, int resourceId); // REF:resourceId->Resource
// END OBJECT Resource


// BEGINN OBJECT File
	/** Return -1 when the file does not exist */
	int               (CALLING_CONV *File_getSize)(int teamId, const char* fileName);
	/** Returns false when file does not exist, or the buffer is too small */
	bool              (CALLING_CONV *File_getContent)(int teamId, const char* fileName, void* buffer, int bufferLen);
//	bool              (CALLING_CONV *File_locateForReading)(int teamId, char* fileName, int fileName_sizeMax);
//	bool              (CALLING_CONV *File_locateForWriting)(int teamId, char* fileName, int fileName_sizeMax);
// END OBJECT File


// BEGINN OBJECT UnitDef
	/**
	 * A UnitDef contains all properties of a unit that are specific to its type,
	 * for example the number and type of weapons or max-speed.
	 * These properties are usually fixed, and not meant to change during a game.
	 * The unitId is a unique id for this type of unit.
	 */
	int               (CALLING_CONV *getUnitDefs)(int teamId, int* unitDefIds, int unitDefIds_sizeMax); // FETCHER:MULTI:IDs:UnitDef:unitDefIds
	int               (CALLING_CONV *getUnitDefByName)(int teamId, const char* unitName); // FETCHER:SINGLE:UnitDef
//	int               (CALLING_CONV *UnitDef_getId)(int teamId, int unitDefId);
	/** Forces loading of the unit model */
	float             (CALLING_CONV *UnitDef_getHeight)(int teamId, int unitDefId);
	/** Forces loading of the unit model */
	float             (CALLING_CONV *UnitDef_getRadius)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isValid)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getName)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getHumanName)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getFileName)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getAiHint)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getCobId)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getTechLevel)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getGaia)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getUpkeep)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	/** This amount of the resource will always be created. */
	float             (CALLING_CONV *UnitDef_getResourceMake)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	/**
	 * This amount of the resource will be created when the unit is on and enough
	 * energy can be drained.
	 */
	float             (CALLING_CONV *UnitDef_getMakesResource)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getCost)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getExtractsResource)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getResourceExtractorRange)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getWindResourceGenerator)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getTidalResourceGenerator)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getStorage)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	bool              (CALLING_CONV *UnitDef_isSquareResourceExtractor)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	bool              (CALLING_CONV *UnitDef_isResourceMaker)(int teamId, int unitDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *UnitDef_getBuildTime)(int teamId, int unitDefId);
	/** This amount of auto-heal will always be applied. */
	float             (CALLING_CONV *UnitDef_getAutoHeal)(int teamId, int unitDefId);
	/** This amount of auto-heal will only be applied while the unit is idling. */
	float             (CALLING_CONV *UnitDef_getIdleAutoHeal)(int teamId, int unitDefId);
	/** Time a unit needs to idle before it is considered idling. */
	int               (CALLING_CONV *UnitDef_getIdleTime)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getPower)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getHealth)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getCategory)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTurnRate)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isTurnInPlace)(int teamId, int unitDefId);
	/**
	 * Units above this distance to goal will try to turn while keeping
	 * some of their speed.
	 * 0 to disable
	 */
	float             (CALLING_CONV *UnitDef_getTurnInPlaceDistance)(int teamId, int unitDefId);
	/**
	 * Units below this speed will turn in place regardless of their
	 * turnInPlace setting.
	 */
	float             (CALLING_CONV *UnitDef_getTurnInPlaceSpeedLimit)(int teamId, int unitDefId);
// use UnitDef_MoveData_getMoveType instead
//	int               (CALLING_CONV *UnitDef_getMoveType)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isUpright)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isCollide)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getControlRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getLosRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getAirLosRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getLosHeight)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getRadarRadius)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getSonarRadius)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getJammerRadius)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getSonarJamRadius)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getSeismicRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getSeismicSignature)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isStealth)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isSonarStealth)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isBuildRange3D)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getBuildDistance)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getBuildSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getReclaimSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getRepairSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxRepairSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getResurrectSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getCaptureSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTerraformSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMass)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isPushResistant)(int teamId, int unitDefId);
	/** Should the unit move sideways when it can not shoot? */
	bool              (CALLING_CONV *UnitDef_isStrafeToAttack)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMinCollisionSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getSlideTolerance)(int teamId, int unitDefId);
	/**
	 * Build location relevant maximum steepness of the underlaying terrain.
	 * Used to calculate the maxHeightDif.
	 */
	float             (CALLING_CONV *UnitDef_getMaxSlope)(int teamId, int unitDefId);
	/**
	 * Maximum terra-form height this building allows.
	 * If this value is 0.0, you can only build this structure on
	 * totally flat terrain.
	 */
	float             (CALLING_CONV *UnitDef_getMaxHeightDif)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMinWaterDepth)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getWaterline)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxWaterDepth)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getArmoredMultiple)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getArmorType)(int teamId, int unitDefId);
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
	int               (CALLING_CONV *UnitDef_FlankingBonus_getMode)(int teamId, int unitDefId);
	/**
	 * The unit takes less damage when attacked from this direction.
	 * This encourage flanking fire.
	 */
	void              (CALLING_CONV *UnitDef_FlankingBonus_getDir)(int teamId, int unitDefId, float* return_posF3_out);
	/** Damage factor for the least protected direction */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMax)(int teamId, int unitDefId);
	/** Damage factor for the most protected direction */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMin)(int teamId, int unitDefId);
	/**
	 * How much the ability of the flanking bonus direction to move builds up each
	 * frame.
	 */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMobilityAdd)(int teamId, int unitDefId);
	/**
	 * The type of the collision volume's form.
	 *
	 * @return  "Ell"
	 *          "Cyl[T]" (where [T] is one of ['X', 'Y', 'Z'])
	 *          "Box"
	 */
	const char*       (CALLING_CONV *UnitDef_CollisionVolume_getType)(int teamId, int unitDefId);
	/** The collision volume's full axis lengths. */
	void              (CALLING_CONV *UnitDef_CollisionVolume_getScales)(int teamId, int unitDefId, float* return_posF3_out);
	/** The collision volume's offset relative to the unit's center position */
	void              (CALLING_CONV *UnitDef_CollisionVolume_getOffsets)(int teamId, int unitDefId, float* return_posF3_out);
	/**
	 * Collission test algorithm used.
	 *
	 * @return  0: discrete
	 *          1: continuous
	 */
	int               (CALLING_CONV *UnitDef_CollisionVolume_getTest)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxWeaponRange)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getType)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getTooltip)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getWreckName)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getDeathExplosion)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getSelfDExplosion)(int teamId, int unitDefId);
//	TODO: this might be changed later for something better
	const char*       (CALLING_CONV *UnitDef_getTedClassString)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_getCategoryString)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToSelfD)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getSelfDCountdown)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToSubmerge)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToFly)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToMove)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToHover)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isFloater)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isBuilder)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isActivateWhenBuilt)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isOnOffable)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isFullHealthFactory)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isFactoryHeadingTakeoff)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isReclaimable)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isCapturable)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToRestore)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToRepair)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToSelfRepair)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToReclaim)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToAttack)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToPatrol)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToFight)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToGuard)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToAssist)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAssistable)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToRepeat)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToFireControl)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getFireState)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getMoveState)(int teamId, int unitDefId);
// beginn: aircraft stuff
	float             (CALLING_CONV *UnitDef_getWingDrag)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getWingAngle)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getDrag)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getFrontToSpeed)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getSpeedToFront)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMyGravity)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxBank)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxPitch)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTurnRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getWantedHeight)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getVerticalSpeed)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToCrash)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isHoverAttack)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAirStrafe)(int teamId, int unitDefId);
	/**
	 * @return  < 0:  it can land
	 *          >= 0: how much the unit will move during hovering on the spot
	 */
	float             (CALLING_CONV *UnitDef_getDlHoverFactor)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxAcceleration)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxDeceleration)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxAileron)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxElevator)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMaxRudder)(int teamId, int unitDefId);
// end: aircraft stuff
	/**
	 * The yard map defines which parts of the square a unit occupies
	 * can still be walked on by other units.
	 * Example:
	 * In the BA Arm T2 K-Bot lab, htere is a line in hte middle where units
	 * walk, otherwise they would not be able ot exit the lab once they are
	 * built.
	 */
	int               (CALLING_CONV *UnitDef_getYardMap)(int teamId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax); // ARRAY:yardMap
	int               (CALLING_CONV *UnitDef_getXSize)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getZSize)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getBuildAngle)(int teamId, int unitDefId);
// beginn: transports stuff
	float             (CALLING_CONV *UnitDef_getLoadingRadius)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getUnloadSpread)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getTransportCapacity)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getTransportSize)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getMinTransportSize)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAirBase)(int teamId, int unitDefId);
	/**  */
	bool              (CALLING_CONV *UnitDef_isFirePlatform)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTransportMass)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getMinTransportMass)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isHoldSteady)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isReleaseHeld)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isNotTransportable)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isTransportByEnemy)(int teamId, int unitDefId);
	/**
	 * @return  0: land unload
	 *          1: fly-over drop
	 *          2: land flood
	 */
	int               (CALLING_CONV *UnitDef_getTransportUnloadMethod)(int teamId, int unitDefId);
	/**
	 * Dictates fall speed of all transported units.
	 * This only makes sense for air transports,
	 * if they an drop units while in the air.
	 */
	float             (CALLING_CONV *UnitDef_getFallSpeed)(int teamId, int unitDefId);
	/** Sets the transported units FBI, overrides fallSpeed */
	float             (CALLING_CONV *UnitDef_getUnitFallSpeed)(int teamId, int unitDefId);
// end: transports stuff
	/** If the unit can cloak */
	bool              (CALLING_CONV *UnitDef_isAbleToCloak)(int teamId, int unitDefId);
	/** If the unit wants to start out cloaked */
	bool              (CALLING_CONV *UnitDef_isStartCloaked)(int teamId, int unitDefId);
	/** Energy cost per second to stay cloaked when stationary */
	float             (CALLING_CONV *UnitDef_getCloakCost)(int teamId, int unitDefId);
	/** Energy cost per second to stay cloaked when moving */
	float             (CALLING_CONV *UnitDef_getCloakCostMoving)(int teamId, int unitDefId);
	/** If enemy unit comes within this range, decloaking is forced */
	float             (CALLING_CONV *UnitDef_getDecloakDistance)(int teamId, int unitDefId);
	/** Use a spherical, instead of a cylindrical test? */
	bool              (CALLING_CONV *UnitDef_isDecloakSpherical)(int teamId, int unitDefId);
	/** Will the unit decloak upon firing? */
	bool              (CALLING_CONV *UnitDef_isDecloakOnFire)(int teamId, int unitDefId);
	/** Will the unit self destruct if an enemy comes to close? */
	bool              (CALLING_CONV *UnitDef_isAbleToKamikaze)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getKamikazeDist)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isTargetingFacility)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToDGun)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isNeedGeo)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isFeature)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isHideDamage)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isCommander)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isShowPlayerName)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToResurrect)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToCapture)(int teamId, int unitDefId);
	/**
	 * Indicates the trajectory types supported by this unit.
	 *
	 * @return  0: (default) = only low
	 *          1: only high
	 *          2: choose
	 */
	int               (CALLING_CONV *UnitDef_getHighTrajectoryType)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getNoChaseCategory)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isLeaveTracks)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTrackWidth)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTrackOffset)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTrackStrength)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getTrackStretch)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getTrackType)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isAbleToDropFlare)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getFlareReloadTime)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getFlareEfficiency)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getFlareDelay)(int teamId, int unitDefId);
	void              (CALLING_CONV *UnitDef_getFlareDropVector)(int teamId, int unitDefId, float* return_posF3_out);
	int               (CALLING_CONV *UnitDef_getFlareTime)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getFlareSalvoSize)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getFlareSalvoDelay)(int teamId, int unitDefId);
	/** Only matters for fighter aircraft */
	bool              (CALLING_CONV *UnitDef_isAbleToLoopbackAttack)(int teamId, int unitDefId);
	/**
	 * Indicates whether the ground will be leveled/flattened out
	 * after this building has been built on it.
	 * Only matters for buildings.
	 */
	bool              (CALLING_CONV *UnitDef_isLevelGround)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_isUseBuildingGroundDecal)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getBuildingDecalType)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getBuildingDecalSizeX)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getBuildingDecalSizeY)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_getBuildingDecalDecaySpeed)(int teamId, int unitDefId);
	/**
	 * Maximum flight time in seconds before the aircraft needs
	 * to return to an air repair bay to refuel.
	 */
	float             (CALLING_CONV *UnitDef_getMaxFuel)(int teamId, int unitDefId);
	/** Time to fully refuel the unit */
	float             (CALLING_CONV *UnitDef_getRefuelTime)(int teamId, int unitDefId);
	/** Minimum build power of airbases that this aircraft can land on */
	float             (CALLING_CONV *UnitDef_getMinAirBasePower)(int teamId, int unitDefId);
	/** Number of units of this type allowed simultaneously in the game */
	int               (CALLING_CONV *UnitDef_getMaxThisUnit)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getDecoyDef)(int teamId, int unitDefId); // RETURN-REF:int->UnitDef
	bool              (CALLING_CONV *UnitDef_isDontLand)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_getShieldDef)(int teamId, int unitDefId); // RETURN-REF:int->WeaponDef
	int               (CALLING_CONV *UnitDef_getStockpileDef)(int teamId, int unitDefId); // RETURN-REF:int->WeaponDef
	int               (CALLING_CONV *UnitDef_getBuildOptions)(int teamId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax); // ARRAY:unitDefIds
	int               (CALLING_CONV *UnitDef_getCustomParams)(int teamId, int unitDefId, const char** keys, const char** values); // MAP



	bool              (CALLING_CONV *UnitDef_isMoveDataAvailable)(int teamId, int unitDefId); // AVAILABLE:MoveData
	float             (CALLING_CONV *UnitDef_MoveData_getMaxAcceleration)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getMaxBreaking)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getMaxSpeed)(int teamId, int unitDefId);
	short             (CALLING_CONV *UnitDef_MoveData_getMaxTurnRate)(int teamId, int unitDefId);

	int               (CALLING_CONV *UnitDef_MoveData_getSize)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getDepth)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getMaxSlope)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getSlopeMod)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getDepthMod)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_MoveData_getPathType)(int teamId, int unitDefId);
	float             (CALLING_CONV *UnitDef_MoveData_getCrushStrength)(int teamId, int unitDefId);
	/** enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 }; */
	int               (CALLING_CONV *UnitDef_MoveData_getMoveType)(int teamId, int unitDefId);
	/** enum MoveFamily { Tank=0, KBot=1, Hover=2, Ship=3 }; */
	int               (CALLING_CONV *UnitDef_MoveData_getMoveFamily)(int teamId, int unitDefId);
	int               (CALLING_CONV *UnitDef_MoveData_getTerrainClass)(int teamId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_MoveData_getFollowGround)(int teamId, int unitDefId);
	bool              (CALLING_CONV *UnitDef_MoveData_isSubMarine)(int teamId, int unitDefId);
	const char*       (CALLING_CONV *UnitDef_MoveData_getName)(int teamId, int unitDefId);



	int               (CALLING_CONV *UnitDef_getWeaponMounts)(int teamId, int unitDefId); // FETCHER:MULTI:NUM:WeaponMount
	const char*       (CALLING_CONV *UnitDef_WeaponMount_getName)(int teamId, int unitDefId, int weaponMountId);
	int               (CALLING_CONV *UnitDef_WeaponMount_getWeaponDef)(int teamId, int unitDefId, int weaponMountId); // RETURN-REF:int->WeaponDef
	int               (CALLING_CONV *UnitDef_WeaponMount_getSlavedTo)(int teamId, int unitDefId, int weaponMountId);
	void              (CALLING_CONV *UnitDef_WeaponMount_getMainDir)(int teamId, int unitDefId, int weaponMountId, float* return_posF3_out);
	float             (CALLING_CONV *UnitDef_WeaponMount_getMaxAngleDif)(int teamId, int unitDefId, int weaponMountId);
	/**
	 * How many seconds of fuel it costs for the owning unit to fire this weapon.
	 */
	float             (CALLING_CONV *UnitDef_WeaponMount_getFuelUsage)(int teamId, int unitDefId, int weaponMountId);
	int               (CALLING_CONV *UnitDef_WeaponMount_getBadTargetCategory)(int teamId, int unitDefId, int weaponMountId);
	int               (CALLING_CONV *UnitDef_WeaponMount_getOnlyTargetCategory)(int teamId, int unitDefId, int weaponMountId);
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
	int               (CALLING_CONV *Unit_getLimit)(int teamId); // STATIC
	/**
	 * Returns all units that are not in this teams ally-team nor neutral and are
	 * in LOS.
	 */
	int               (CALLING_CONV *getEnemyUnits)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are not in this teams ally-team nor neutral and are
	 * in LOS plus they have to be located in the specified area of the map.
	 */
	int               (CALLING_CONV *getEnemyUnitsIn)(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are not in this teams ally-team nor neutral and are in
	 * some way visible (sight or radar).
	 */
	int               (CALLING_CONV *getEnemyUnitsInRadarAndLos)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units.
	 */
	int               (CALLING_CONV *getFriendlyUnits)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units plus they have to be located in the specified area of the map.
	 */
	int               (CALLING_CONV *getFriendlyUnitsIn)(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are neutral and are in LOS.
	 */
	int               (CALLING_CONV *getNeutralUnits)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are neutral and are in LOS plus they have to be
	 * located in the specified area of the map.
	 */
	int               (CALLING_CONV *getNeutralUnitsIn)(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are of the team controlled by this AI instance. This
	 * list can also be created dynamically by the AI, through updating a list on
	 * each unit-created and unit-destroyed event.
	 */
	int               (CALLING_CONV *getTeamUnits)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns all units that are currently selected
	 * (usually only contains units if a human payer
	 * is controlling this team as well).
	 */
	int               (CALLING_CONV *getSelectedUnits)(int teamId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds
	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
	int               (CALLING_CONV *Unit_getDef)(int teamId, int unitId); // RETURN-REF:int->UnitDef
	/**
	 * This is a set of parameters that is initialized
	 * in CreateUnitRulesParams() and may change during the game.
	 * Each parameter is uniquely identified only by its id
	 * (which is the index in the vector).
	 * Parameters may or may not have a name.
	 */
	int               (CALLING_CONV *Unit_getModParams)(int teamId, int unitId); // FETCHER:MULTI:NUM:ModParam
	const char*       (CALLING_CONV *Unit_ModParam_getName)(int teamId, int unitId, int modParamId);
	float             (CALLING_CONV *Unit_ModParam_getValue)(int teamId, int unitId, int modParamId);
	int               (CALLING_CONV *Unit_getTeam)(int teamId, int unitId);
	int               (CALLING_CONV *Unit_getAllyTeam)(int teamId, int unitId);
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
	int               (CALLING_CONV *Unit_getLineage)(int teamId, int unitId);
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
	int               (CALLING_CONV *Unit_getAiHint)(int teamId, int unitId);
	int               (CALLING_CONV *Unit_getStockpile)(int teamId, int unitId);
	int               (CALLING_CONV *Unit_getStockpileQueued)(int teamId, int unitId);
	float             (CALLING_CONV *Unit_getCurrentFuel)(int teamId, int unitId);
	/** The unit's max speed */
	float             (CALLING_CONV *Unit_getMaxSpeed)(int teamId, int unitId);
	/** The furthest any weapon of the unit can fire */
	float             (CALLING_CONV *Unit_getMaxRange)(int teamId, int unitId);
	/** The unit's max health */
	float             (CALLING_CONV *Unit_getMaxHealth)(int teamId, int unitId);
	/** How experienced the unit is (0.0f-1.0f) */
	float             (CALLING_CONV *Unit_getExperience)(int teamId, int unitId);
	/** Returns the group a unit belongs to, -1 if none */
	int               (CALLING_CONV *Unit_getGroup)(int teamId, int unitId);
	int               (CALLING_CONV *Unit_getCurrentCommands)(int teamId, int unitId); // FETCHER:MULTI:NUM:CurrentCommand-Command
	/**
	 * For the type of the command queue, see CCommandQueue::CommandQueueType
	 * in Sim/Unit/CommandAI/CommandQueue.h
	 */
	int               (CALLING_CONV *Unit_CurrentCommand_getType)(int teamId, int unitId); // STATIC
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Unit_CurrentCommand_getId)(int teamId, int unitId, int commandId);
	short             (CALLING_CONV *Unit_CurrentCommand_getOptions)(int teamId, int unitId, int commandId);
	int               (CALLING_CONV *Unit_CurrentCommand_getTag)(int teamId, int unitId, int commandId);
	int               (CALLING_CONV *Unit_CurrentCommand_getTimeOut)(int teamId, int unitId, int commandId);
	int               (CALLING_CONV *Unit_CurrentCommand_getParams)(int teamId, int unitId, int commandId, float* params, int params_max); // ARRAY:params
	/** The commands that this unit can understand, other commands will be ignored */
	int               (CALLING_CONV *Unit_getSupportedCommands)(int teamId, int unitId); // FETCHER:MULTI:NUM:SupportedCommand-CommandDescription
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Unit_SupportedCommand_getId)(int teamId, int unitId, int commandId);
	const char*       (CALLING_CONV *Unit_SupportedCommand_getName)(int teamId, int unitId, int commandId);
	const char*       (CALLING_CONV *Unit_SupportedCommand_getToolTip)(int teamId, int unitId, int commandId);
	bool              (CALLING_CONV *Unit_SupportedCommand_isShowUnique)(int teamId, int unitId, int commandId);
	bool              (CALLING_CONV *Unit_SupportedCommand_isDisabled)(int teamId, int unitId, int commandId);
	int               (CALLING_CONV *Unit_SupportedCommand_getParams)(int teamId, int unitId, int commandId, const char** params, int params_max); // ARRAY:params

	/** The unit's current health */
	float             (CALLING_CONV *Unit_getHealth)(int teamId, int unitId);
	float             (CALLING_CONV *Unit_getSpeed)(int teamId, int unitId);
	/**
	 * Indicate the relative power of the unit,
	 * used for experience calulations etc.
	 * This is sort of the measure of the units overall power.
	 */
	float             (CALLING_CONV *Unit_getPower)(int teamId, int unitId);
//	int               (CALLING_CONV *Unit_getResourceInfos)(int teamId, int unitId); // FETCHER:MULTI:NUM:ResourceInfo
	float             (CALLING_CONV *Unit_getResourceUse)(int teamId, int unitId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *Unit_getResourceMake)(int teamId, int unitId, int resourceId); // REF:resourceId->Resource
	void              (CALLING_CONV *Unit_getPos)(int teamId, int unitId, float* return_posF3_out);
	bool              (CALLING_CONV *Unit_isActivated)(int teamId, int unitId);
	/** Returns true if the unit is currently being built */
	bool              (CALLING_CONV *Unit_isBeingBuilt)(int teamId, int unitId);
	bool              (CALLING_CONV *Unit_isCloaked)(int teamId, int unitId);
	bool              (CALLING_CONV *Unit_isParalyzed)(int teamId, int unitId);
	bool              (CALLING_CONV *Unit_isNeutral)(int teamId, int unitId);
	/** Returns the unit's build facing (0-3) */
	int               (CALLING_CONV *Unit_getBuildingFacing)(int teamId, int unitId);
	/** Number of the last frame this unit received an order from a player. */
	int               (CALLING_CONV *Unit_getLastUserOrderFrame)(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
	int               (CALLING_CONV *getGroups)(int teamId, int* groupIds, int groupIds_sizeMax); // FETCHER:MULTI:IDs:Group:groupIds
	int               (CALLING_CONV *Group_getSupportedCommands)(int teamId, int groupId); // FETCHER:MULTI:NUM:SupportedCommand-CommandDescription
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Group_SupportedCommand_getId)(int teamId, int groupId, int commandId);
	const char*       (CALLING_CONV *Group_SupportedCommand_getName)(int teamId, int groupId, int commandId);
	const char*       (CALLING_CONV *Group_SupportedCommand_getToolTip)(int teamId, int groupId, int commandId);
	bool              (CALLING_CONV *Group_SupportedCommand_isShowUnique)(int teamId, int groupId, int commandId);
	bool              (CALLING_CONV *Group_SupportedCommand_isDisabled)(int teamId, int groupId, int commandId);
	int               (CALLING_CONV *Group_SupportedCommand_getParams)(int teamId, int groupId, int commandId, const char** params, int params_max); // ARRAY:params
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Group_OrderPreview_getId)(int teamId, int groupId);
	short             (CALLING_CONV *Group_OrderPreview_getOptions)(int teamId, int groupId);
	int               (CALLING_CONV *Group_OrderPreview_getTag)(int teamId, int groupId);
	int               (CALLING_CONV *Group_OrderPreview_getTimeOut)(int teamId, int groupId);
	int               (CALLING_CONV *Group_OrderPreview_getParams)(int teamId, int groupId, float* params, int params_max); // ARRAY:params
	bool              (CALLING_CONV *Group_isSelected)(int teamId, int groupId);
// END OBJECT Group



// BEGINN OBJECT Mod

	/**
	 * archive filename
	 */
	const char*       (CALLING_CONV *Mod_getFileName)(int teamId);
	const char*       (CALLING_CONV *Mod_getHumanName)(int teamId);
	const char*       (CALLING_CONV *Mod_getShortName)(int teamId);
	const char*       (CALLING_CONV *Mod_getVersion)(int teamId);
	const char*       (CALLING_CONV *Mod_getMutator)(int teamId);
	const char*       (CALLING_CONV *Mod_getDescription)(int teamId);

	bool              (CALLING_CONV *Mod_getAllowTeamColors)(int teamId);

	/**
	 * Should constructions without builders decay?
	 */
	bool              (CALLING_CONV *Mod_getConstructionDecay)(int teamId);
	/**
	 * How long until they start decaying?
	 */
	int               (CALLING_CONV *Mod_getConstructionDecayTime)(int teamId);
	/**
	 * How fast do they decay?
	 */
	float             (CALLING_CONV *Mod_getConstructionDecaySpeed)(int teamId);

	/**
	 * 0 = 1 reclaimer per feature max, otherwise unlimited
	 */
	int               (CALLING_CONV *Mod_getMultiReclaim)(int teamId);
	/**
	 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	 */
	int               (CALLING_CONV *Mod_getReclaimMethod)(int teamId);
	/**
	 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	 */
	int               (CALLING_CONV *Mod_getReclaimUnitMethod)(int teamId);
	/**
	 * How much energy should reclaiming a unit cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getReclaimUnitEnergyCostFactor)(int teamId);
	/**
	 * How much metal should reclaim return, default 1.0
	 */
	float             (CALLING_CONV *Mod_getReclaimUnitEfficiency)(int teamId);
	/**
	 * How much should energy should reclaiming a feature cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getReclaimFeatureEnergyCostFactor)(int teamId);
	/**
	 * Allow reclaiming enemies? default true
	 */
	bool              (CALLING_CONV *Mod_getReclaimAllowEnemies)(int teamId);
	/**
	 * Allow reclaiming allies? default true
	 */
	bool              (CALLING_CONV *Mod_getReclaimAllowAllies)(int teamId);

	/**
	 * How much should energy should repair cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getRepairEnergyCostFactor)(int teamId);

	/**
	 * How much should energy should resurrect cost, default 0.5
	 */
	float             (CALLING_CONV *Mod_getResurrectEnergyCostFactor)(int teamId);

	/**
	 * How much should energy should capture cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getCaptureEnergyCostFactor)(int teamId);

	/**
	 * 0 = all ground units cannot be transported,
	 * 1 = all ground units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 1.
	 */
	int               (CALLING_CONV *Mod_getTransportGround)(int teamId);
	/**
	 * 0 = all hover units cannot be transported,
	 * 1 = all hover units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportHover)(int teamId);
	/**
	 * 0 = all naval units cannot be transported,
	 * 1 = all naval units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportShip)(int teamId);
	/**
	 * 0 = all air units cannot be transported,
	 * 1 = all air units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportAir)(int teamId);

	/**
	 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	 */
	int               (CALLING_CONV *Mod_getFireAtKilled)(int teamId);
	/**
	 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	 */
	int               (CALLING_CONV *Mod_getFireAtCrashing)(int teamId);

	/**
	 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	 */
	int               (CALLING_CONV *Mod_getFlankingBonusModeDefault)(int teamId);

	/**
	 * miplevel for los
	 */
	int               (CALLING_CONV *Mod_getLosMipLevel)(int teamId);
	/**
	 * miplevel to use for airlos
	 */
	int               (CALLING_CONV *Mod_getAirMipLevel)(int teamId);
	/**
	 * units sightdistance will be multiplied with this, for testing purposes
	 */
	float             (CALLING_CONV *Mod_getLosMul)(int teamId);
	/**
	 * units airsightdistance will be multiplied with this, for testing purposes
	 */
	float             (CALLING_CONV *Mod_getAirLosMul)(int teamId);
	/**
	 * when underwater, units are not in LOS unless also in sonar
	 */
	bool              (CALLING_CONV *Mod_getRequireSonarUnderWater)(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
	int               (CALLING_CONV *Map_getChecksum)(int teamId);
	void              (CALLING_CONV *Map_getStartPos)(int teamId, float* return_posF3_out);
	void              (CALLING_CONV *Map_getMousePos)(int teamId, float* return_posF3_out);
	bool              (CALLING_CONV *Map_isPosInCamera)(int teamId, float* pos_posF3, float radius);
	/** Returns the maps width in full resolution */
	int               (CALLING_CONV *Map_getWidth)(int teamId);
	/** Returns the maps height in full resolution */
	int               (CALLING_CONV *Map_getHeight)(int teamId);
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
	 *
	 * @see getCornersHeightMap()
	 */
	int               (CALLING_CONV *Map_getHeightMap)(int teamId, float* heights, int heights_max); // ARRAY:heights
	/**
	 * Returns the height for the corners of the squares.
	 * This is the same like the drawn map.
	 * It is one unit wider and one higher then the centers height map.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - 4 points mark the edges of an area of 8*8 in size
	 * - the value for upper left corner of the full resolution position (x, z) is at index (x/8 * width + z/8)
	 * - the last value, bottom right, is at index ((width/8+1) * (height/8+1) - 1)
	 *
	 * @see getHeightMap()
	 */
	int               (CALLING_CONV *Map_getCornersHeightMap)(int teamId, float* cornerHeights, int cornerHeights_max); // ARRAY:cornerHeights
	float             (CALLING_CONV *Map_getMinHeight)(int teamId);
	float             (CALLING_CONV *Map_getMaxHeight)(int teamId);
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
	int               (CALLING_CONV *Map_getSlopeMap)(int teamId, float* slopes, int slopes_max); // ARRAY:slopes
	/**
	 * @brief the level of sight map
	 * gs->mapx >> losMipLevel
	 * A square with value zero means you do not have LOS coverage on it.
	 *Mod_getLosMipLevel
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - resolution factor (res) is min(1, 1 << Mod_getLosMipLevel())
	 *   examples:
	 *   	+ losMipLevel(0) -> res(1)
	 *   	+ losMipLevel(1) -> res(2)
	 *   	+ losMipLevel(2) -> res(4)
	 *   	+ losMipLevel(3) -> res(8)
	 * - each data position is res*res in size
	 * - the value for the full resolution position (x, z) is at index (x/res * width + z/res)
	 * - the last value, bottom right, is at index (width/res * height/res - 1)
	 */
	int               (CALLING_CONV *Map_getLosMap)(int teamId, int* losValues, int losValues_max); // ARRAY:losValues
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
	int               (CALLING_CONV *Map_getRadarMap)(int teamId, int* radarValues, int radarValues_max); // ARRAY:losValues
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
	int               (CALLING_CONV *Map_getJammerMap)(int teamId, int* jammerValues, int jammerValues_max); // ARRAY:losValues
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
	int               (CALLING_CONV *Map_getResourceMapRaw)(int teamId, int resourceId, short* resources, int resources_max); // REF:resourceId->Resource ARRAY:resources
	/**
	 * Returns positions indicating where to place resource extractors on the map.
	 * Only the x and z values give the location of the spots, while the y values
	 * represents the actual amount of resource an extractor placed there can make.
	 * You should only compare the y values to each other, and not try to estimate
	 * effective output from spots.
	 */
	int               (CALLING_CONV *Map_getResourceMapSpotsPositions)(int teamId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax); // REF:resourceId->Resource ARRAY:spots_AposF3
	/**
	 * Returns the average resource income for an extractor on one of the evaluated positions.
	 */
	float             (CALLING_CONV *Map_getResourceMapSpotsAverageIncome)(int teamId, int resourceId); // REF:resourceId->Resource
	/**
	 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
	 */
	void              (CALLING_CONV *Map_getResourceMapSpotsNearest)(int teamId, int resourceId, float* pos_posF3, float* return_posF3_out); // REF:resourceId->Resource

	const char*       (CALLING_CONV *Map_getName)(int teamId);
	/** Gets the elevation of the map at position (x, z) */
	float             (CALLING_CONV *Map_getElevationAt)(int teamId, float x, float z);


	/** Returns what value 255 in the resource map is worth */
	float             (CALLING_CONV *Map_getMaxResource)(int teamId, int resourceId); // REF:resourceId->Resource
	/** Returns extraction radius for resource extractors */
	float             (CALLING_CONV *Map_getExtractorRadius)(int teamId, int resourceId); // REF:resourceId->Resource

	float             (CALLING_CONV *Map_getMinWind)(int teamId);
	float             (CALLING_CONV *Map_getMaxWind)(int teamId);
	float             (CALLING_CONV *Map_getCurWind)(int teamId);
	float             (CALLING_CONV *Map_getTidalStrength)(int teamId);
	float             (CALLING_CONV *Map_getGravity)(int teamId);


	/**
	 * Returns all points drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	int               (CALLING_CONV *Map_getPoints)(int teamId, bool includeAllies); // FETCHER:MULTI:NUM:Point
	void              (CALLING_CONV *Map_Point_getPosition)(int teamId, int pointId, float* return_posF3_out);
	void              (CALLING_CONV *Map_Point_getColor)(int teamId, int pointId, short* return_colorS3_out);
	const char*       (CALLING_CONV *Map_Point_getLabel)(int teamId, int pointId);
	/**
	 * Returns all lines drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	int               (CALLING_CONV *Map_getLines)(int teamId, bool includeAllies); // FETCHER:MULTI:NUM:Line
	void              (CALLING_CONV *Map_Line_getFirstPosition)(int teamId, int lineId, float* return_posF3_out);
	void              (CALLING_CONV *Map_Line_getSecondPosition)(int teamId, int lineId, float* return_posF3_out);
	void              (CALLING_CONV *Map_Line_getColor)(int teamId, int lineId, short* return_colorS3_out);
	bool              (CALLING_CONV *Map_isPossibleToBuildAt)(int teamId, int unitDefId, float* pos_posF3, int facing); // REF:unitDefId->UnitDef
	/**
	 * Returns the closest position from a given position that a building can be built at.
	 * @param minDist the distance in squares that the building must keep to other buildings,
	 *                to make it easier to keep free paths through a base
	 * @return actual map position with x, y and z all beeing positive,
	 *         or float[3]{-1, 0, 0} if no suitable position is found.
	 */
	void              (CALLING_CONV *Map_findClosestBuildSite)(int teamId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out); // REF:unitDefId->UnitDef
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
	int               (CALLING_CONV *getFeatureDefs)(int teamId, int* featureDefIds, int featureDefIds_sizeMax); // FETCHER:MULTI:IDs:FeatureDef:featureDefIds
//	int (CALLING_CONV *FeatureDef_getId)(int teamId, int featureDefId);
	const char*       (CALLING_CONV *FeatureDef_getName)(int teamId, int featureDefId);
	const char*       (CALLING_CONV *FeatureDef_getDescription)(int teamId, int featureDefId);
	const char*       (CALLING_CONV *FeatureDef_getFileName)(int teamId, int featureDefId);
	float             (CALLING_CONV *FeatureDef_getContainedResource)(int teamId, int featureDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *FeatureDef_getMaxHealth)(int teamId, int featureDefId);
	float             (CALLING_CONV *FeatureDef_getReclaimTime)(int teamId, int featureDefId);
	/** Used to see if the object can be overrun by units of a certain heavyness */
	float             (CALLING_CONV *FeatureDef_getMass)(int teamId, int featureDefId);
	/**
	 * The type of the collision volume's form.
	 *
	 * @return  "Ell"
	 *          "Cyl[T]" (where [T] is one of ['X', 'Y', 'Z'])
	 *          "Box"
	 */
	const char*       (CALLING_CONV *FeatureDef_CollisionVolume_getType)(int teamId, int featureDefId);
	/** The collision volume's full axis lengths. */
	void              (CALLING_CONV *FeatureDef_CollisionVolume_getScales)(int teamId, int featureDefId, float* return_posF3_out);
	/** The collision volume's offset relative to the feature's center position */
	void              (CALLING_CONV *FeatureDef_CollisionVolume_getOffsets)(int teamId, int featureDefId, float* return_posF3_out);
	/**
	 * Collission test algorithm used.
	 *
	 * @return  0: discrete
	 *          1: continuous
	 */
	int               (CALLING_CONV *FeatureDef_CollisionVolume_getTest)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isUpright)(int teamId, int featureDefId);
	int               (CALLING_CONV *FeatureDef_getDrawType)(int teamId, int featureDefId);
	const char*       (CALLING_CONV *FeatureDef_getModelName)(int teamId, int featureDefId);
	/**
	 * Used to determine whether the feature is resurrectable.
	 *
	 * @return  -1: (default) only if it is the 1st wreckage of
	 *              the UnitDef it originates from
	 *           0: no, never
	 *           1: yes, always
	 */
	int               (CALLING_CONV *FeatureDef_getResurrectable)(int teamId, int featureDefId);
	int               (CALLING_CONV *FeatureDef_getSmokeTime)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isDestructable)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isReclaimable)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isBlocking)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isBurnable)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isFloating)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isNoSelect)(int teamId, int featureDefId);
	bool              (CALLING_CONV *FeatureDef_isGeoThermal)(int teamId, int featureDefId);
	/** Name of the FeatureDef that this turns into when killed (not reclaimed). */
	const char*       (CALLING_CONV *FeatureDef_getDeathFeature)(int teamId, int featureDefId);
	/**
	 * Size of the feature along the X axis - in other words: height.
	 * each size is 8 units
	 */
	int               (CALLING_CONV *FeatureDef_getXSize)(int teamId, int featureDefId);
	/**
	 * Size of the feature along the Z axis - in other words: width.
	 * each size is 8 units
	 */
	int               (CALLING_CONV *FeatureDef_getZSize)(int teamId, int featureDefId);
	int               (CALLING_CONV *FeatureDef_getCustomParams)(int teamId, int featureDefId, const char** keys, const char** values); // MAP
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
	/**
	 * Returns all features currently in LOS, or all features on the map
	 * if cheating is enabled.
	 */
	int               (CALLING_CONV *getFeatures)(int teamId, int* featureIds, int featureIds_sizeMax); // FETCHER:MULTI:IDs:Feature:featureIds
	/**
	 * Returns all features in a specified area that are currently in LOS,
	 * or all features in this area if cheating is enabled.
	 */
	int               (CALLING_CONV *getFeaturesIn)(int teamId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax); // FETCHER:MULTI:IDs:Feature:featureIds
	int               (CALLING_CONV *Feature_getDef)(int teamId, int featureId); // RETURN-REF:int->FeatureDef
	float             (CALLING_CONV *Feature_getHealth)(int teamId, int featureId);
	float             (CALLING_CONV *Feature_getReclaimLeft)(int teamId, int featureId);
	void              (CALLING_CONV *Feature_getPosition)(int teamId, int featureId, float* return_posF3_out);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
	int               (CALLING_CONV *getWeaponDefs)(int teamId); // FETCHER:MULTI:NUM:WeaponDef
	int               (CALLING_CONV *getWeaponDefByName)(int teamId, const char* weaponDefName); // FETCHER:SINGLE:WeaponDef
	const char*       (CALLING_CONV *WeaponDef_getName)(int teamId, int weaponDefId);
	const char*       (CALLING_CONV *WeaponDef_getType)(int teamId, int weaponDefId);
	const char*       (CALLING_CONV *WeaponDef_getDescription)(int teamId, int weaponDefId);
	const char*       (CALLING_CONV *WeaponDef_getFileName)(int teamId, int weaponDefId);
	const char*       (CALLING_CONV *WeaponDef_getCegTag)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getRange)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getHeightMod)(int teamId, int weaponDefId);
	/** Inaccuracy of whole burst */
	float             (CALLING_CONV *WeaponDef_getAccuracy)(int teamId, int weaponDefId);
	/** Inaccuracy of individual shots inside burst */
	float             (CALLING_CONV *WeaponDef_getSprayAngle)(int teamId, int weaponDefId);
	/** Inaccuracy while owner moving */
	float             (CALLING_CONV *WeaponDef_getMovingAccuracy)(int teamId, int weaponDefId);
	/** Fraction of targets move speed that is used as error offset */
	float             (CALLING_CONV *WeaponDef_getTargetMoveError)(int teamId, int weaponDefId);
	/** Maximum distance the weapon will lead the target */
	float             (CALLING_CONV *WeaponDef_getLeadLimit)(int teamId, int weaponDefId);
	/** Factor for increasing the leadLimit with experience */
	float             (CALLING_CONV *WeaponDef_getLeadBonus)(int teamId, int weaponDefId);
	/** Replaces hardcoded behaviour for burnblow cannons */
	float             (CALLING_CONV *WeaponDef_getPredictBoost)(int teamId, int weaponDefId);

//	TODO: Deprecate the following function, if no longer needed by legacy Cpp AIs
	int               (CALLING_CONV *WeaponDef_getNumDamageTypes)(int teamId); // STATIC
//	DamageArray (CALLING_CONV *WeaponDef_getDamages)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_Damage_getParalyzeDamageTime)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_Damage_getImpulseFactor)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_Damage_getImpulseBoost)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_Damage_getCraterMult)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_Damage_getCraterBoost)(int teamId, int weaponDefId);
//	float (CALLING_CONV *WeaponDef_Damage_getType)(int teamId, int weaponDefId, int typeId);
	int               (CALLING_CONV *WeaponDef_Damage_getTypes)(int teamId, int weaponDefId, float* types, int types_sizeMax); // ARRAY:types

//	int (CALLING_CONV *WeaponDef_getId)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getAreaOfEffect)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isNoSelfDamage)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getFireStarter)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getEdgeEffectiveness)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getSize)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getSizeGrowth)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getCollisionSize)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getSalvoSize)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getSalvoDelay)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getReload)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getBeamTime)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isBeamBurst)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isWaterBounce)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isGroundBounce)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getBounceRebound)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getBounceSlip)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getNumBounce)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getMaxAngle)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getRestTime)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getUpTime)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getFlightTime)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getCost)(int teamId, int weaponDefId, int resourceId); // REF:resourceId->Resource
	float             (CALLING_CONV *WeaponDef_getSupplyCost)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getProjectilesPerShot)(int teamId, int weaponDefId);
//	/** The "id=" tag in the TDF */
//	int (CALLING_CONV *WeaponDef_getTdfId)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isTurret)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isOnlyForward)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isFixedLauncher)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isWaterWeapon)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isFireSubmersed)(int teamId, int weaponDefId);
	/** Lets a torpedo travel above water like it does below water */
	bool              (CALLING_CONV *WeaponDef_isSubMissile)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isTracks)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isDropped)(int teamId, int weaponDefId);
	/** The weapon will only paralyze, not do real damage. */
	bool              (CALLING_CONV *WeaponDef_isParalyzer)(int teamId, int weaponDefId);
	/** The weapon damages by impacting, not by exploding. */
	bool              (CALLING_CONV *WeaponDef_isImpactOnly)(int teamId, int weaponDefId);
	/** Can not target anything (for example: anti-nuke, D-Gun) */
	bool              (CALLING_CONV *WeaponDef_isNoAutoTarget)(int teamId, int weaponDefId);
	/** Has to be fired manually (by the player or an AI, example: D-Gun) */
	bool              (CALLING_CONV *WeaponDef_isManualFire)(int teamId, int weaponDefId);
	/**
	 * Can intercept targetable weapons shots.
	 *
	 * example: anti-nuke
	 *
	 * @see  getTargetable()
	 */
	int               (CALLING_CONV *WeaponDef_getInterceptor)(int teamId, int weaponDefId);
	/**
	 * Shoots interceptable projectiles.
	 * Shots can be intercepted by interceptors.
	 *
	 * example: nuke
	 *
	 * @see  getInterceptor()
	 */
	int               (CALLING_CONV *WeaponDef_getTargetable)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isStockpileable)(int teamId, int weaponDefId);
	/**
	 * Range of interceptors.
	 *
	 * example: anti-nuke
	 *
	 * @see  getInterceptor()
	 */
	float             (CALLING_CONV *WeaponDef_getCoverageRange)(int teamId, int weaponDefId);
	/** Build time of a missile */
	float             (CALLING_CONV *WeaponDef_getStockpileTime)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getIntensity)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getThickness)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getLaserFlareSize)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getCoreThickness)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getDuration)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getLodDistance)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getFalloffRate)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getGraphicsType)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isSoundTrigger)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isSelfExplode)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isGravityAffected)(int teamId, int weaponDefId);
	/**
	 * Per weapon high trajectory setting.
	 * UnitDef also has this property.
	 *
	 * @return  0: low
	 *          1: high
	 *          2: unit
	 */
	int               (CALLING_CONV *WeaponDef_getHighTrajectory)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getMyGravity)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isNoExplode)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getStartVelocity)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getWeaponAcceleration)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getTurnRate)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getMaxVelocity)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getProjectileSpeed)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getExplosionSpeed)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getOnlyTargetCategory)(int teamId, int weaponDefId);
	/** How much the missile will wobble around its course. */
	float             (CALLING_CONV *WeaponDef_getWobble)(int teamId, int weaponDefId);
	/** How much the missile will dance. */
	float             (CALLING_CONV *WeaponDef_getDance)(int teamId, int weaponDefId);
	/** How high trajectory missiles will try to fly in. */
	float             (CALLING_CONV *WeaponDef_getTrajectoryHeight)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isLargeBeamLaser)(int teamId, int weaponDefId);
	/** If the weapon is a shield rather than a weapon. */
	bool              (CALLING_CONV *WeaponDef_isShield)(int teamId, int weaponDefId);
	/** If the weapon should be repulsed or absorbed. */
	bool              (CALLING_CONV *WeaponDef_isShieldRepulser)(int teamId, int weaponDefId);
	/** If the shield only affects enemy projectiles. */
	bool              (CALLING_CONV *WeaponDef_isSmartShield)(int teamId, int weaponDefId);
	/** If the shield only affects stuff coming from outside shield radius. */
	bool              (CALLING_CONV *WeaponDef_isExteriorShield)(int teamId, int weaponDefId);
	/** If the shield should be graphically shown. */
	bool              (CALLING_CONV *WeaponDef_isVisibleShield)(int teamId, int weaponDefId);
	/** If a small graphic should be shown at each repulse. */
	bool              (CALLING_CONV *WeaponDef_isVisibleShieldRepulse)(int teamId, int weaponDefId);
	/** The number of frames to draw the shield after it has been hit. */
	int               (CALLING_CONV *WeaponDef_getVisibleShieldHitFrames)(int teamId, int weaponDefId);
	/**
	 * Amount of the resource used per shot or per second,
	 * depending on the type of projectile.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getResourceUse)(int teamId, int weaponDefId, int resourceId); // REF:resourceId->Resource
	/** Size of shield covered area */
	float             (CALLING_CONV *WeaponDef_Shield_getRadius)(int teamId, int weaponDefId);
	/**
	 * Shield acceleration on plasma stuff.
	 * How much will plasma be accelerated into the other direction
	 * when it hits the shield.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getForce)(int teamId, int weaponDefId);
	/** Maximum speed to which the shield can repulse plasma. */
	float             (CALLING_CONV *WeaponDef_Shield_getMaxSpeed)(int teamId, int weaponDefId);
	/** Amount of damage the shield can reflect. (0=infinite) */
	float             (CALLING_CONV *WeaponDef_Shield_getPower)(int teamId, int weaponDefId);
	/** Amount of power that is regenerated per second. */
	float             (CALLING_CONV *WeaponDef_Shield_getPowerRegen)(int teamId, int weaponDefId);
	/**
	 * How much of a given resource is needed to regenerate power
	 * with max speed per second.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getPowerRegenResource)(int teamId, int weaponDefId, int resourceId); // REF:resourceId->Resource
	/** How much power the shield has when it is created. */
	float             (CALLING_CONV *WeaponDef_Shield_getStartingPower)(int teamId, int weaponDefId);
	/** Number of frames to delay recharging by after each hit. */
	int               (CALLING_CONV *WeaponDef_Shield_getRechargeDelay)(int teamId, int weaponDefId);
	/** The color of the shield when it is at full power. */
	void              (CALLING_CONV *WeaponDef_Shield_getGoodColor)(int teamId, int weaponDefId, short* return_colorS3_out);
	/** The color of the shield when it is empty. */
	void              (CALLING_CONV *WeaponDef_Shield_getBadColor)(int teamId, int weaponDefId, short* return_colorS3_out);
	/** The shields alpha value. */
	float             (CALLING_CONV *WeaponDef_Shield_getAlpha)(int teamId, int weaponDefId);
	/**
	 * The type of the shield (bitfield).
	 * Defines what weapons can be intercepted by the shield.
	 *
	 * @see  getInterceptedByShieldType()
	 */
	int               (CALLING_CONV *WeaponDef_Shield_getInterceptType)(int teamId, int weaponDefId);
	/**
	 * The type of shields that can intercept this weapon (bitfield).
	 * The weapon can be affected by shields if:
	 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
	 *
	 * @see  getInterceptType()
	 */
	int               (CALLING_CONV *WeaponDef_getInterceptedByShieldType)(int teamId, int weaponDefId);
	/** Tries to avoid friendly units while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidFriendly)(int teamId, int weaponDefId);
	/** Tries to avoid features while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidFeature)(int teamId, int weaponDefId);
	/** Tries to avoid neutral units while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidNeutral)(int teamId, int weaponDefId);
	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
	float             (CALLING_CONV *WeaponDef_getTargetBorder)(int teamId, int weaponDefId);
	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
	float             (CALLING_CONV *WeaponDef_getCylinderTargetting)(int teamId, int weaponDefId);
	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
	float             (CALLING_CONV *WeaponDef_getMinIntensity)(int teamId, int weaponDefId);
	/**
	 * Controls cannon range height boost.
	 *
	 * default: -1: automatically calculate a more or less sane value
	 */
	float             (CALLING_CONV *WeaponDef_getHeightBoostFactor)(int teamId, int weaponDefId);
	/** Multiplier for the distance to the target for priority calculations. */
	float             (CALLING_CONV *WeaponDef_getProximityPriority)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getCollisionFlags)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isSweepFire)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isAbleToAttackGround)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getCameraShake)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getDynDamageExp)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getDynDamageMin)(int teamId, int weaponDefId);
	float             (CALLING_CONV *WeaponDef_getDynDamageRange)(int teamId, int weaponDefId);
	bool              (CALLING_CONV *WeaponDef_isDynDamageInverted)(int teamId, int weaponDefId);
	int               (CALLING_CONV *WeaponDef_getCustomParams)(int teamId, int weaponDefId, const char** keys, const char** values); // MAP
// END OBJECT WeaponDef

};

#if	defined(__cplusplus)
} // extern "C"
#endif

#endif // _SKIRMISHAICALLBACK_H
