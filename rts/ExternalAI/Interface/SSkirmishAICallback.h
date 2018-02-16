/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S_SKIRMISH_AI_CALLBACK_H
#define	S_SKIRMISH_AI_CALLBACK_H

#include "aidefines.h"

#if	defined(__cplusplus)
extern "C" {
#endif


/**
 * @brief Skirmish AI Callback function pointers.
 * Each Skirmish AI instance will receive an instance of this struct
 * in its init(skirmishAIId) function and with the SInitEvent.
 *
 * This struct contains only activities that leave the game state as it is,
 * in spring terms: unsynced events
 * Activities that change game state (-> synced events) are handled through
 * AI commands, defined in AISCommands.h.
 *
 * The skirmishAIId passed as the first parameter to each function in this
 * struct has to be the ID of the AI instance using the callback.
 */
struct SSkirmishAICallback {

	/**
	 * Whenever an AI wants to change the engine state in any way,
	 * it has to call this method.
	 * In other words, all commands from AIs to the engine (and other AIs)
	 * go through this method.
	 *
	 * @param skirmishAIId the ID of the AI that sends the command
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
	int               (CALLING_CONV *Engine_handleCommand)(int skirmishAIId, int toId, int commandId, int commandTopic, void* commandData);

	int               (CALLING_CONV *Engine_executeCommand)(int skirmishAIId, int unitId, int groupId, void* commandData);


	/// Returns the major engine revision number (e.g. 83)
	const char*       (CALLING_CONV *Engine_Version_getMajor)(int skirmishAIId);

	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 */
	const char*       (CALLING_CONV *Engine_Version_getMinor)(int skirmishAIId);

	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
	const char*       (CALLING_CONV *Engine_Version_getPatchset)(int skirmishAIId);

	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
	const char*       (CALLING_CONV *Engine_Version_getCommits)(int skirmishAIId);

	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
	const char*       (CALLING_CONV *Engine_Version_getHash)(int skirmishAIId);

	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
	const char*       (CALLING_CONV *Engine_Version_getBranch)(int skirmishAIId);

	/// Additional information (compiler flags, svn revision etc.)
	const char*       (CALLING_CONV *Engine_Version_getAdditional)(int skirmishAIId);

	/// time of build
	const char*       (CALLING_CONV *Engine_Version_getBuildTime)(int skirmishAIId);

	/// Returns whether this is a release build of the engine
	bool              (CALLING_CONV *Engine_Version_isRelease)(int skirmishAIId);

	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 */
	const char*       (CALLING_CONV *Engine_Version_getNormal)(int skirmishAIId);

	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
	const char*       (CALLING_CONV *Engine_Version_getSync)(int skirmishAIId);

	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 */
	const char*       (CALLING_CONV *Engine_Version_getFull)(int skirmishAIId);

	/** Returns the number of teams in this game */
	int               (CALLING_CONV *Teams_getSize)(int skirmishAIId);

	/** Returns the number of skirmish AIs in this game */
	int               (CALLING_CONV *SkirmishAIs_getSize)(int skirmishAIId);

	/** Returns the maximum number of skirmish AIs in any game */
	int               (CALLING_CONV *SkirmishAIs_getMax)(int skirmishAIId);

	/**
	 * Returns the ID of the team controled by this Skirmish AI.
	 */
	int               (CALLING_CONV *SkirmishAI_getTeamId)(int skirmishAIId);

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *SkirmishAI_Info_getSize)(int skirmishAIId);

	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getKey)(int skirmishAIId, int infoIndex);

	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getValue)(int skirmishAIId, int infoIndex);

	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getDescription)(int skirmishAIId, int infoIndex);

	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *SkirmishAI_Info_getValueByKey)(int skirmishAIId, const char* const key);

	/**
	 * Returns the number of option key-value pairs in the options map
	 * for this Skirmish AI.
	 */
	int               (CALLING_CONV *SkirmishAI_OptionValues_getSize)(int skirmishAIId);

	/**
	 * Returns the key at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getKey)(int skirmishAIId, int optionIndex);

	/**
	 * Returns the value at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getValue)(int skirmishAIId, int optionIndex);

	/**
	 * Returns the value associated with the given key in the options map
	 * for this Skirmish AI, or NULL if not found.
	 */
	const char*       (CALLING_CONV *SkirmishAI_OptionValues_getValueByKey)(int skirmishAIId, const char* const key);

	/** This will end up in infolog */
	void              (CALLING_CONV *Log_log)(int skirmishAIId, const char* const msg);

	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
	void               (CALLING_CONV *Log_exception)(int skirmishAIId, const char* const msg, int severety, bool die);

	/** Returns '/' on posix and '\\' on windows */
	char               (CALLING_CONV *DataDirs_getPathSeparator)(int skirmishAIId);

	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *DataDirs_getConfigDir)(int skirmishAIId);

	/**
	 * This interfaces writable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
	 */
	const char*       (CALLING_CONV *DataDirs_getWriteableDir)(int skirmishAIId);

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
	 * @param   writeable  if true, only the writable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @param   common     if true, the version independent data-dir is formed,
	 *                     which uses "common" instead of the version, eg:
	 *                     "/home/userX/.spring/AI/Skirmish/RAI/common/..."
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
	bool              (CALLING_CONV *DataDirs_locatePath)(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);

	/**
	 * @see     locatePath()
	 */
	char*             (CALLING_CONV *DataDirs_allocatePath)(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir, bool common);

	/** Returns the number of springs data dirs. */
	int               (CALLING_CONV *DataDirs_Roots_getSize)(int skirmishAIId);

	/** Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1). */
	bool              (CALLING_CONV *DataDirs_Roots_getDir)(int skirmishAIId, char* path, int path_sizeMax, int dirIndex);

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
	 * @param   writeable  if true, only the writable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
	bool              (CALLING_CONV *DataDirs_Roots_locatePath)(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);

	char*             (CALLING_CONV *DataDirs_Roots_allocatePath)(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir);

// BEGINN misc callback functions
	/**
	 * Returns the current game time measured in frames (the
	 * simulation runs at 30 frames per second at normal speed)
	 *
	 * This should not be used, as we get the frame from the SUpdateEvent.
	 * @deprecated
	 */
	int               (CALLING_CONV *Game_getCurrentFrame)(int skirmishAIId);

	int               (CALLING_CONV *Game_getAiInterfaceVersion)(int skirmishAIId);

	int               (CALLING_CONV *Game_getMyTeam)(int skirmishAIId);

	int               (CALLING_CONV *Game_getMyAllyTeam)(int skirmishAIId);

	int               (CALLING_CONV *Game_getPlayerTeam)(int skirmishAIId, int playerId);

	/**
	 * Returns the number of active teams participating
	 * in the currently running game.
	 * A return value of 6 for example, would mean that teams 0 till 5
	 * take part in the game.
	 */
	int               (CALLING_CONV *Game_getTeams)(int skirmishAIId);

	/**
	 * Returns the name of the side of a team in the game.
	 *
	 * This should not be used, as it may be "",
	 * and as the AI should rather rely on the units it has,
	 * which will lead to a more stable and versatile AI.
	 * @deprecated
	 *
	 * @return eg. "ARM" or "CORE"; may be "", depending on how the game was setup
	 */
	const char*       (CALLING_CONV *Game_getTeamSide)(int skirmishAIId, int otherTeamId);

	/**
	 * Returns the color of a team in the game.
	 *
	 * This should only be used when drawing stuff,
	 * and not for team-identification.
	 * @return the RGB color of a team, with values in [0, 255]
	 */
	void              (CALLING_CONV *Game_getTeamColor)(int skirmishAIId, int otherTeamId, short* return_colorS3_out);

	/**
	 * Returns the income multiplier of a team in the game.
	 * All the teams resource income is multiplied by this factor.
	 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
	 */
	float             (CALLING_CONV *Game_getTeamIncomeMultiplier)(int skirmishAIId, int otherTeamId);

	/// Returns the ally-team of a team
	int               (CALLING_CONV *Game_getTeamAllyTeam)(int skirmishAIId, int otherTeamId);

	/**
	 * Returns the current level of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current level of the requested resource of the other team, or -1.0 on an invalid request
	 */
	float             (CALLING_CONV *Game_getTeamResourceCurrent)(int skirmishAIId, int otherTeamId, int resourceId);

	/**
	 * Returns the current income of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current income of the requested resource of the other team, or -1.0 on an invalid request
	 */
	float             (CALLING_CONV *Game_getTeamResourceIncome)(int skirmishAIId, int otherTeamId, int resourceId);

	/**
	 * Returns the current usage of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current usage of the requested resource of the other team, or -1.0 on an invalid request
	 */
	float             (CALLING_CONV *Game_getTeamResourceUsage)(int skirmishAIId, int otherTeamId, int resourceId);

	/**
	 * Returns the storage capacity for a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return storage capacity for the requested resource of the other team, or -1.0 on an invalid request
	 */
	float             (CALLING_CONV *Game_getTeamResourceStorage)(int skirmishAIId, int otherTeamId, int resourceId);

	float             (CALLING_CONV *Game_getTeamResourcePull)(int skirmishAIId, int otherTeamId, int resourceId);

	float             (CALLING_CONV *Game_getTeamResourceShare)(int skirmishAIId, int otherTeamId, int resourceId);

	float             (CALLING_CONV *Game_getTeamResourceSent)(int skirmishAIId, int otherTeamId, int resourceId);

	float             (CALLING_CONV *Game_getTeamResourceReceived)(int skirmishAIId, int otherTeamId, int resourceId);

	float             (CALLING_CONV *Game_getTeamResourceExcess)(int skirmishAIId, int otherTeamId, int resourceId);

	/// Returns true, if the two supplied ally-teams are currently allied
	bool              (CALLING_CONV *Game_isAllied)(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId);

	bool              (CALLING_CONV *Game_isDebugModeEnabled)(int skirmishAIId);

	int               (CALLING_CONV *Game_getMode)(int skirmishAIId);

	bool              (CALLING_CONV *Game_isPaused)(int skirmishAIId);

	float             (CALLING_CONV *Game_getSpeedFactor)(int skirmishAIId);

	const char*       (CALLING_CONV *Game_getSetupScript)(int skirmishAIId);

	/**
	 * Returns the categories bit field value.
	 * @return the categories bit field value or 0,
	 *         in case of empty name or too many categories
	 * @see getCategoryName
	 */
	int               (CALLING_CONV *Game_getCategoryFlag)(int skirmishAIId, const char* categoryName);

	/**
	 * Returns the bitfield values of a list of category names.
	 * @param categoryNames space delimited list of names
	 * @see Game#getCategoryFlag
	 */
	int               (CALLING_CONV *Game_getCategoriesFlag)(int skirmishAIId, const char* categoryNames);

	/**
	 * Return the name of the category described by a category flag.
	 * @see Game#getCategoryFlag
	 */
	void              (CALLING_CONV *Game_getCategoryName)(int skirmishAIId, int categoryFlag, char* name, int name_sizeMax);

	/**
	 * @return float value of parameter if it's set, defaultValue otherwise.
	 */
	float             (CALLING_CONV *Game_getRulesParamFloat)(int skirmishAIId, const char* gameRulesParamName, float defaultValue);

	/**
	 * @return string value of parameter if it's set, defaultValue otherwise.
	 */
	const char*       (CALLING_CONV *Game_getRulesParamString)(int skirmishAIId, const char* gameRulesParamName, const char* defaultValue);

// END misc callback functions


// BEGINN Visualization related callback functions
	float             (CALLING_CONV *Gui_getViewRange)(int skirmishAIId);

	float             (CALLING_CONV *Gui_getScreenX)(int skirmishAIId);

	float             (CALLING_CONV *Gui_getScreenY)(int skirmishAIId);

	void              (CALLING_CONV *Gui_Camera_getDirection)(int skirmishAIId, float* return_posF3_out);

	void              (CALLING_CONV *Gui_Camera_getPosition)(int skirmishAIId, float* return_posF3_out);

// END Visualization related callback functions


// BEGINN OBJECT Cheats
	/**
	 * Returns whether this AI may use active cheats.
	 */
	bool              (CALLING_CONV *Cheats_isEnabled)(int skirmishAIId);

	/**
	 * Set whether this AI may use active cheats.
	 */
	bool              (CALLING_CONV *Cheats_setEnabled)(int skirmishAIId, bool enable);

	/**
	 * Set whether this AI may receive cheat events.
	 * When enabled, you would for example get informed when enemy units are
	 * created, even without sensor coverage.
	 */
	bool              (CALLING_CONV *Cheats_setEventsEnabled)(int skirmishAIId, bool enabled);

	/**
	 * Returns whether cheats will desync if used by an AI.
	 * @return always true, unless we are both the host and the only client.
	 */
	bool              (CALLING_CONV *Cheats_isOnlyPassive)(int skirmishAIId);

// END OBJECT Cheats


// BEGINN OBJECT Resource
	int               (CALLING_CONV *getResources)(int skirmishAIId); //$ FETCHER:MULTI:NUM:Resource

	int               (CALLING_CONV *getResourceByName)(int skirmishAIId, const char* resourceName); //$ REF:RETURN->Resource

	const char*       (CALLING_CONV *Resource_getName)(int skirmishAIId, int resourceId);

	float             (CALLING_CONV *Resource_getOptimum)(int skirmishAIId, int resourceId);

	float             (CALLING_CONV *Economy_getCurrent)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getIncome)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getUsage)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getStorage)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getPull)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getShare)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getSent)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getReceived)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Economy_getExcess)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

// END OBJECT Resource


// BEGINN OBJECT File
	/** Return -1 when the file does not exist */
	int               (CALLING_CONV *File_getSize)(int skirmishAIId, const char* fileName);

	/** Returns false when file does not exist, or the buffer is too small */
	bool              (CALLING_CONV *File_getContent)(int skirmishAIId, const char* fileName, void* buffer, int bufferLen);

//	bool              (CALLING_CONV *File_locateForReading)(int skirmishAIId, char* fileName, int fileName_sizeMax);

//	bool              (CALLING_CONV *File_locateForWriting)(int skirmishAIId, char* fileName, int fileName_sizeMax);

// END OBJECT File


// BEGINN OBJECT UnitDef
	/**
	 * A UnitDef contains all properties of a unit that are specific to its type,
	 * for example the number and type of weapons or max-speed.
	 * These properties are usually fixed, and not meant to change during a game.
	 * The unitId is a unique id for this type of unit.
	 */
	int               (CALLING_CONV *getUnitDefs)(int skirmishAIId, int* unitDefIds, int unitDefIds_sizeMax); //$ FETCHER:MULTI:IDs:UnitDef:unitDefIds

	int               (CALLING_CONV *getUnitDefByName)(int skirmishAIId, const char* unitName); //$ REF:RETURN->UnitDef


	/** Forces loading of the unit model */
	float             (CALLING_CONV *UnitDef_getHeight)(int skirmishAIId, int unitDefId);

	/** Forces loading of the unit model */
	float             (CALLING_CONV *UnitDef_getRadius)(int skirmishAIId, int unitDefId);

	const char*       (CALLING_CONV *UnitDef_getName)(int skirmishAIId, int unitDefId);

	const char*       (CALLING_CONV *UnitDef_getHumanName)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getUpkeep)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	/** This amount of the resource will always be created. */
	float             (CALLING_CONV *UnitDef_getResourceMake)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	/**
	 * This amount of the resource will be created when the unit is on and enough
	 * energy can be drained.
	 */
	float             (CALLING_CONV *UnitDef_getMakesResource)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getCost)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getExtractsResource)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getResourceExtractorRange)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getWindResourceGenerator)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getTidalResourceGenerator)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *UnitDef_getStorage)(int skirmishAIId, int unitDefId, int resourceId); //$ REF:resourceId->Resource


	float             (CALLING_CONV *UnitDef_getBuildTime)(int skirmishAIId, int unitDefId);

	/** This amount of auto-heal will always be applied. */
	float             (CALLING_CONV *UnitDef_getAutoHeal)(int skirmishAIId, int unitDefId);

	/** This amount of auto-heal will only be applied while the unit is idling. */
	float             (CALLING_CONV *UnitDef_getIdleAutoHeal)(int skirmishAIId, int unitDefId);

	/** Time a unit needs to idle before it is considered idling. */
	int               (CALLING_CONV *UnitDef_getIdleTime)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getPower)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getHealth)(int skirmishAIId, int unitDefId);

	/**
	 * Returns the bit field value denoting the categories this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	int               (CALLING_CONV *UnitDef_getCategory)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getTurnRate)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isTurnInPlace)(int skirmishAIId, int unitDefId);

	/**
	 * Units above this distance to goal will try to turn while keeping
	 * some of their speed.
	 * 0 to disable
	 */
	float             (CALLING_CONV *UnitDef_getTurnInPlaceDistance)(int skirmishAIId, int unitDefId);

	/**
	 * Units below this speed will turn in place regardless of their
	 * turnInPlace setting.
	 */
	float             (CALLING_CONV *UnitDef_getTurnInPlaceSpeedLimit)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isUpright)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isCollide)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getLosRadius)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getAirLosRadius)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getLosHeight)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getRadarRadius)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getSonarRadius)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getJammerRadius)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getSonarJamRadius)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getSeismicRadius)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getSeismicSignature)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isStealth)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isSonarStealth)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isBuildRange3D)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getBuildDistance)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getBuildSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getReclaimSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getRepairSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxRepairSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getResurrectSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getCaptureSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getTerraformSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMass)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isPushResistant)(int skirmishAIId, int unitDefId);

	/** Should the unit move sideways when it can not shoot? */
	bool              (CALLING_CONV *UnitDef_isStrafeToAttack)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMinCollisionSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getSlideTolerance)(int skirmishAIId, int unitDefId);

	/**
	 * Maximum terra-form height this building allows.
	 * If this value is 0.0, you can only build this structure on
	 * totally flat terrain.
	 */
	float             (CALLING_CONV *UnitDef_getMaxHeightDif)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMinWaterDepth)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getWaterline)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxWaterDepth)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getArmoredMultiple)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getArmorType)(int skirmishAIId, int unitDefId);

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
	int               (CALLING_CONV *UnitDef_FlankingBonus_getMode)(int skirmishAIId, int unitDefId);

	/**
	 * The unit takes less damage when attacked from this direction.
	 * This encourage flanking fire.
	 */
	void              (CALLING_CONV *UnitDef_FlankingBonus_getDir)(int skirmishAIId, int unitDefId, float* return_posF3_out);

	/** Damage factor for the least protected direction */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMax)(int skirmishAIId, int unitDefId);

	/** Damage factor for the most protected direction */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMin)(int skirmishAIId, int unitDefId);

	/**
	 * How much the ability of the flanking bonus direction to move builds up each
	 * frame.
	 */
	float             (CALLING_CONV *UnitDef_FlankingBonus_getMobilityAdd)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxWeaponRange)(int skirmishAIId, int unitDefId);


	const char*       (CALLING_CONV *UnitDef_getTooltip)(int skirmishAIId, int unitDefId);

	const char*       (CALLING_CONV *UnitDef_getWreckName)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getDeathExplosion)(int skirmishAIId, int unitDefId); //$ REF:RETURN->WeaponDef

	int               (CALLING_CONV *UnitDef_getSelfDExplosion)(int skirmishAIId, int unitDefId); //$ REF:RETURN->WeaponDef

	/**
	 * Returns the name of the category this unit is in.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	const char*       (CALLING_CONV *UnitDef_getCategoryString)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToSelfD)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getSelfDCountdown)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToSubmerge)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToFly)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToMove)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToHover)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isFloater)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isBuilder)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isActivateWhenBuilt)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isOnOffable)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isFullHealthFactory)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isFactoryHeadingTakeoff)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isReclaimable)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isCapturable)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToRestore)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToRepair)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToSelfRepair)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToReclaim)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToAttack)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToPatrol)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToFight)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToGuard)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToAssist)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAssistable)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToRepeat)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToFireControl)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getFireState)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getMoveState)(int skirmishAIId, int unitDefId);

// beginn: aircraft stuff
	float             (CALLING_CONV *UnitDef_getWingDrag)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getWingAngle)(int skirmishAIId, int unitDefId);


	float             (CALLING_CONV *UnitDef_getFrontToSpeed)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getSpeedToFront)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMyGravity)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxBank)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxPitch)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getTurnRadius)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getWantedHeight)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getVerticalSpeed)(int skirmishAIId, int unitDefId);


	bool              (CALLING_CONV *UnitDef_isHoverAttack)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAirStrafe)(int skirmishAIId, int unitDefId);


	/**
	 * @return  < 0:  it can land
	 *          >= 0: how much the unit will move during hovering on the spot
	 */
	float             (CALLING_CONV *UnitDef_getDlHoverFactor)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxAcceleration)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxDeceleration)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxAileron)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxElevator)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMaxRudder)(int skirmishAIId, int unitDefId);

// end: aircraft stuff
	/**
	 * The yard map defines which parts of the square a unit occupies
	 * can still be walked on by other units.
	 * Example:
	 * In the BA Arm T2 K-Bot lab, htere is a line in hte middle where units
	 * walk, otherwise they would not be able ot exit the lab once they are
	 * built.
	 * @return 0 if invalid facing or the unit has no yard-map defined,
	 *         the size of the yard-map otherwise: getXSize() * getXSize()
	 */
	int               (CALLING_CONV *UnitDef_getYardMap)(int skirmishAIId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax); //$ ARRAY:yardMap

	int               (CALLING_CONV *UnitDef_getXSize)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getZSize)(int skirmishAIId, int unitDefId);


// beginn: transports stuff
	float             (CALLING_CONV *UnitDef_getLoadingRadius)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getUnloadSpread)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getTransportCapacity)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getTransportSize)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getMinTransportSize)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAirBase)(int skirmishAIId, int unitDefId);

	/**  */
	bool              (CALLING_CONV *UnitDef_isFirePlatform)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getTransportMass)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getMinTransportMass)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isHoldSteady)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isReleaseHeld)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isNotTransportable)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isTransportByEnemy)(int skirmishAIId, int unitDefId);

	/**
	 * @return  0: land unload
	 *          1: fly-over drop
	 *          2: land flood
	 */
	int               (CALLING_CONV *UnitDef_getTransportUnloadMethod)(int skirmishAIId, int unitDefId);

	/**
	 * Dictates fall speed of all transported units.
	 * This only makes sense for air transports,
	 * if they an drop units while in the air.
	 */
	float             (CALLING_CONV *UnitDef_getFallSpeed)(int skirmishAIId, int unitDefId);

	/** Sets the transported units FBI, overrides fallSpeed */
	float             (CALLING_CONV *UnitDef_getUnitFallSpeed)(int skirmishAIId, int unitDefId);

// end: transports stuff
	/** If the unit can cloak */
	bool              (CALLING_CONV *UnitDef_isAbleToCloak)(int skirmishAIId, int unitDefId);

	/** If the unit wants to start out cloaked */
	bool              (CALLING_CONV *UnitDef_isStartCloaked)(int skirmishAIId, int unitDefId);

	/** Energy cost per second to stay cloaked when stationary */
	float             (CALLING_CONV *UnitDef_getCloakCost)(int skirmishAIId, int unitDefId);

	/** Energy cost per second to stay cloaked when moving */
	float             (CALLING_CONV *UnitDef_getCloakCostMoving)(int skirmishAIId, int unitDefId);

	/** If enemy unit comes within this range, decloaking is forced */
	float             (CALLING_CONV *UnitDef_getDecloakDistance)(int skirmishAIId, int unitDefId);

	/** Use a spherical, instead of a cylindrical test? */
	bool              (CALLING_CONV *UnitDef_isDecloakSpherical)(int skirmishAIId, int unitDefId);

	/** Will the unit decloak upon firing? */
	bool              (CALLING_CONV *UnitDef_isDecloakOnFire)(int skirmishAIId, int unitDefId);

	/** Will the unit self destruct if an enemy comes to close? */
	bool              (CALLING_CONV *UnitDef_isAbleToKamikaze)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getKamikazeDist)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isTargetingFacility)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_canManualFire)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isNeedGeo)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isFeature)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isHideDamage)(int skirmishAIId, int unitDefId);


	bool              (CALLING_CONV *UnitDef_isShowPlayerName)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToResurrect)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_isAbleToCapture)(int skirmishAIId, int unitDefId);

	/**
	 * Indicates the trajectory types supported by this unit.
	 *
	 * @return  0: (default) = only low
	 *          1: only high
	 *          2: choose
	 */
	int               (CALLING_CONV *UnitDef_getHighTrajectoryType)(int skirmishAIId, int unitDefId);

	/**
	 * Returns the bit field value denoting the categories this unit shall not
	 * chase.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	int               (CALLING_CONV *UnitDef_getNoChaseCategory)(int skirmishAIId, int unitDefId);


	bool              (CALLING_CONV *UnitDef_isAbleToDropFlare)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getFlareReloadTime)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getFlareEfficiency)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_getFlareDelay)(int skirmishAIId, int unitDefId);

	void              (CALLING_CONV *UnitDef_getFlareDropVector)(int skirmishAIId, int unitDefId, float* return_posF3_out);

	int               (CALLING_CONV *UnitDef_getFlareTime)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getFlareSalvoSize)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getFlareSalvoDelay)(int skirmishAIId, int unitDefId);

	/** Only matters for fighter aircraft */
	bool              (CALLING_CONV *UnitDef_isAbleToLoopbackAttack)(int skirmishAIId, int unitDefId);

	/**
	 * Indicates whether the ground will be leveled/flattened out
	 * after this building has been built on it.
	 * Only matters for buildings.
	 */
	bool              (CALLING_CONV *UnitDef_isLevelGround)(int skirmishAIId, int unitDefId);


	/** Number of units of this type allowed simultaneously in the game */
	int               (CALLING_CONV *UnitDef_getMaxThisUnit)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getDecoyDef)(int skirmishAIId, int unitDefId); //$ REF:RETURN->UnitDef

	bool              (CALLING_CONV *UnitDef_isDontLand)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_getShieldDef)(int skirmishAIId, int unitDefId); //$ REF:RETURN->WeaponDef

	int               (CALLING_CONV *UnitDef_getStockpileDef)(int skirmishAIId, int unitDefId); //$ REF:RETURN->WeaponDef

	int               (CALLING_CONV *UnitDef_getBuildOptions)(int skirmishAIId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax); //$ REF:MULTI:unitDefIds->UnitDef

	int               (CALLING_CONV *UnitDef_getCustomParams)(int skirmishAIId, int unitDefId, const char** keys, const char** values); //$ MAP



	bool              (CALLING_CONV *UnitDef_isMoveDataAvailable)(int skirmishAIId, int unitDefId); //$ AVAILABLE:MoveData

	int               (CALLING_CONV *UnitDef_MoveData_getXSize)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_MoveData_getZSize)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_MoveData_getDepth)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_MoveData_getMaxSlope)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_MoveData_getSlopeMod)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_MoveData_getDepthMod)(int skirmishAIId, int unitDefId, float height);

	int               (CALLING_CONV *UnitDef_MoveData_getPathType)(int skirmishAIId, int unitDefId);

	float             (CALLING_CONV *UnitDef_MoveData_getCrushStrength)(int skirmishAIId, int unitDefId);


	/** enum SpeedModClass { Tank=0, KBot=1, Hover=2, Ship=3 }; */
	int               (CALLING_CONV *UnitDef_MoveData_getSpeedModClass)(int skirmishAIId, int unitDefId);

	int               (CALLING_CONV *UnitDef_MoveData_getTerrainClass)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_MoveData_getFollowGround)(int skirmishAIId, int unitDefId);

	bool              (CALLING_CONV *UnitDef_MoveData_isSubMarine)(int skirmishAIId, int unitDefId);

	const char*       (CALLING_CONV *UnitDef_MoveData_getName)(int skirmishAIId, int unitDefId);



	int               (CALLING_CONV *UnitDef_getWeaponMounts)(int skirmishAIId, int unitDefId); //$ FETCHER:MULTI:NUM:WeaponMount

	const char*       (CALLING_CONV *UnitDef_WeaponMount_getName)(int skirmishAIId, int unitDefId, int weaponMountId);

	int               (CALLING_CONV *UnitDef_WeaponMount_getWeaponDef)(int skirmishAIId, int unitDefId, int weaponMountId); //$ REF:RETURN->WeaponDef

	int               (CALLING_CONV *UnitDef_WeaponMount_getSlavedTo)(int skirmishAIId, int unitDefId, int weaponMountId);

	void              (CALLING_CONV *UnitDef_WeaponMount_getMainDir)(int skirmishAIId, int unitDefId, int weaponMountId, float* return_posF3_out);

	float             (CALLING_CONV *UnitDef_WeaponMount_getMaxAngleDif)(int skirmishAIId, int unitDefId, int weaponMountId);

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * not target.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	int               (CALLING_CONV *UnitDef_WeaponMount_getBadTargetCategory)(int skirmishAIId, int unitDefId, int weaponMountId);

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	int               (CALLING_CONV *UnitDef_WeaponMount_getOnlyTargetCategory)(int skirmishAIId, int unitDefId, int weaponMountId);

// END OBJECT UnitDef



// BEGINN OBJECT Unit
	/**
	 * Returns the number of units a team can have, after which it can not build
	 * any more. It is possible that a team has more units then this value at
	 * some point in the game. This is possible for example with taking,
	 * reclaiming or capturing units.
	 * This value is usefull for controlling game performance, and will
	 * therefore often be set on games with old hardware to prevent lagging
	 * because of too many units.
	 */
	int               (CALLING_CONV *Unit_getLimit)(int skirmishAIId); //$ STATIC

	/**
	 * Returns the maximum total number of units that may exist at any one point
	 * in time induring the current game.
	 */
	int               (CALLING_CONV *Unit_getMax)(int skirmishAIId); //$ STATIC

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS.
	 * If cheats are enabled, this will return all enemies on the map.
	 */
	int               (CALLING_CONV *getEnemyUnits)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS plus they have to be located in the specified area
	 * of the map.
	 * If cheats are enabled, this will return all enemies
	 * in the specified area.
	 */
	int               (CALLING_CONV *getEnemyUnitsIn)(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in under sensor coverage (sight or radar).
	 * If cheats are enabled, this will return all enemies on the map.
	 */
	int               (CALLING_CONV *getEnemyUnitsInRadarAndLos)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units.
	 */
	int               (CALLING_CONV *getFriendlyUnits)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units plus they have to be located in the specified area of the map.
	 */
	int               (CALLING_CONV *getFriendlyUnitsIn)(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are neutral and are in LOS.
	 */
	int               (CALLING_CONV *getNeutralUnits)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are neutral and are in LOS plus they have to be
	 * located in the specified area of the map.
	 */
	int               (CALLING_CONV *getNeutralUnitsIn)(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are of the team controlled by this AI instance. This
	 * list can also be created dynamically by the AI, through updating a list on
	 * each unit-created and unit-destroyed event.
	 */
	int               (CALLING_CONV *getTeamUnits)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns all units that are currently selected
	 * (usually only contains units if a human player
	 * is controlling this team as well).
	 */
	int               (CALLING_CONV *getSelectedUnits)(int skirmishAIId, int* unitIds, int unitIds_sizeMax); //$ FETCHER:MULTI:IDs:Unit:unitIds

	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
	int               (CALLING_CONV *Unit_getDef)(int skirmishAIId, int unitId); //$ REF:RETURN->UnitDef

	/**
	 * @return float value of parameter if it's set, defaultValue otherwise.
	 */
	float             (CALLING_CONV *Unit_getRulesParamFloat)(int skirmishAIId, int unitId, const char* unitRulesParamName, float defaultValue);

	/**
	 * @return string value of parameter if it's set, defaultValue otherwise.
	 */
	const char*       (CALLING_CONV *Unit_getRulesParamString)(int skirmishAIId, int unitId, const char* unitRulesParamName, const char* defaultValue);

	int               (CALLING_CONV *Unit_getTeam)(int skirmishAIId, int unitId);

	int               (CALLING_CONV *Unit_getAllyTeam)(int skirmishAIId, int unitId);


	int               (CALLING_CONV *Unit_getStockpile)(int skirmishAIId, int unitId);

	int               (CALLING_CONV *Unit_getStockpileQueued)(int skirmishAIId, int unitId);

	/** The unit's max speed */
	float             (CALLING_CONV *Unit_getMaxSpeed)(int skirmishAIId, int unitId);

	/** The furthest any weapon of the unit can fire */
	float             (CALLING_CONV *Unit_getMaxRange)(int skirmishAIId, int unitId);

	/** The unit's max health */
	float             (CALLING_CONV *Unit_getMaxHealth)(int skirmishAIId, int unitId);

	/** How experienced the unit is (0.0f - 1.0f) */
	float             (CALLING_CONV *Unit_getExperience)(int skirmishAIId, int unitId);

	/** Returns the group a unit belongs to, -1 if none */
	int               (CALLING_CONV *Unit_getGroup)(int skirmishAIId, int unitId);

	int               (CALLING_CONV *Unit_getCurrentCommands)(int skirmishAIId, int unitId); //$ FETCHER:MULTI:NUM:CurrentCommand-Command

	/**
	 * For the type of the command queue, see CCommandQueue::CommandQueueType
	 * in Sim/Unit/CommandAI/CommandQueue.h
	 */
	int               (CALLING_CONV *Unit_CurrentCommand_getType)(int skirmishAIId, int unitId); //$ STATIC

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Unit_CurrentCommand_getId)(int skirmishAIId, int unitId, int commandId);

	short             (CALLING_CONV *Unit_CurrentCommand_getOptions)(int skirmishAIId, int unitId, int commandId);

	int               (CALLING_CONV *Unit_CurrentCommand_getTag)(int skirmishAIId, int unitId, int commandId);

	int               (CALLING_CONV *Unit_CurrentCommand_getTimeOut)(int skirmishAIId, int unitId, int commandId);

	int               (CALLING_CONV *Unit_CurrentCommand_getParams)(int skirmishAIId, int unitId, int commandId, float* params, int params_sizeMax); //$ ARRAY:params

	/** The commands that this unit can understand, other commands will be ignored */
	int               (CALLING_CONV *Unit_getSupportedCommands)(int skirmishAIId, int unitId); //$ FETCHER:MULTI:NUM:SupportedCommand-CommandDescription

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Unit_SupportedCommand_getId)(int skirmishAIId, int unitId, int supportedCommandId);

	const char*       (CALLING_CONV *Unit_SupportedCommand_getName)(int skirmishAIId, int unitId, int supportedCommandId);

	const char*       (CALLING_CONV *Unit_SupportedCommand_getToolTip)(int skirmishAIId, int unitId, int supportedCommandId);

	bool              (CALLING_CONV *Unit_SupportedCommand_isShowUnique)(int skirmishAIId, int unitId, int supportedCommandId);

	bool              (CALLING_CONV *Unit_SupportedCommand_isDisabled)(int skirmishAIId, int unitId, int supportedCommandId);

	int               (CALLING_CONV *Unit_SupportedCommand_getParams)(int skirmishAIId, int unitId, int supportedCommandId, const char** params, int params_sizeMax); //$ ARRAY:params

	/** The unit's current health */
	float             (CALLING_CONV *Unit_getHealth)(int skirmishAIId, int unitId);

	float             (CALLING_CONV *Unit_getParalyzeDamage)(int skirmishAIId, int unitId);

	float             (CALLING_CONV *Unit_getCaptureProgress)(int skirmishAIId, int unitId);

	float             (CALLING_CONV *Unit_getBuildProgress)(int skirmishAIId, int unitId);

	float             (CALLING_CONV *Unit_getSpeed)(int skirmishAIId, int unitId);

	/**
	 * Indicate the relative power of the unit,
	 * used for experience calulations etc.
	 * This is sort of the measure of the units overall power.
	 */
	float             (CALLING_CONV *Unit_getPower)(int skirmishAIId, int unitId);

//	int               (CALLING_CONV *Unit_getResourceInfos)(int skirmishAIId, int unitId); //$ FETCHER:MULTI:NUM:ResourceInfo

	float             (CALLING_CONV *Unit_getResourceUse)(int skirmishAIId, int unitId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Unit_getResourceMake)(int skirmishAIId, int unitId, int resourceId); //$ REF:resourceId->Resource

	void              (CALLING_CONV *Unit_getPos)(int skirmishAIId, int unitId, float* return_posF3_out);

	void              (CALLING_CONV *Unit_getVel)(int skirmishAIId, int unitId, float* return_posF3_out);

	bool              (CALLING_CONV *Unit_isActivated)(int skirmishAIId, int unitId);

	/** Returns true if the unit is currently being built */
	bool              (CALLING_CONV *Unit_isBeingBuilt)(int skirmishAIId, int unitId);

	bool              (CALLING_CONV *Unit_isCloaked)(int skirmishAIId, int unitId);

	bool              (CALLING_CONV *Unit_isParalyzed)(int skirmishAIId, int unitId);

	bool              (CALLING_CONV *Unit_isNeutral)(int skirmishAIId, int unitId);

	/** Returns the unit's build facing (0-3) */
	int               (CALLING_CONV *Unit_getBuildingFacing)(int skirmishAIId, int unitId);

	/** Number of the last frame this unit received an order from a player. */
	int               (CALLING_CONV *Unit_getLastUserOrderFrame)(int skirmishAIId, int unitId);

	int               (CALLING_CONV *Unit_getWeapons)(int skirmishAIId, int unitId); //$ FETCHER:MULTI:NUM:Weapon

	int               (CALLING_CONV *Unit_getWeapon)(int skirmishAIId, int unitId, int weaponMountId); //$ REF:weaponMountId->WeaponMount REF:RETURN->Weapon
// END OBJECT Unit


// BEGINN OBJECT Team
	bool              (CALLING_CONV *Team_hasAIController)(int skirmishAIId, int teamId);

	int               (CALLING_CONV *getEnemyTeams)(int skirmishAIId, int* teamIds, int teamIds_sizeMax); //$ FETCHER:MULTI:IDs:Team:teamIds

	int               (CALLING_CONV *getAllyTeams)(int skirmishAIId, int* teamIds, int teamIds_sizeMax); //$ FETCHER:MULTI:IDs:Team:teamIds

	/**
	 * @return float value of parameter if it's set, defaultValue otherwise.
	 */
	float             (CALLING_CONV *Team_getRulesParamFloat)(int skirmishAIId, int teamId, const char* teamRulesParamName, float defaultValue);

	/**
	 * @return string value of parameter if it's set, defaultValue otherwise.
	 */
	const char*       (CALLING_CONV *Team_getRulesParamString)(int skirmishAIId, int teamId, const char* teamRulesParamName, const char* defaultValue);

// END OBJECT Team


// BEGINN OBJECT Group
	int               (CALLING_CONV *getGroups)(int skirmishAIId, int* groupIds, int groupIds_sizeMax); //$ FETCHER:MULTI:IDs:Group:groupIds

	int               (CALLING_CONV *Group_getSupportedCommands)(int skirmishAIId, int groupId); //$ FETCHER:MULTI:NUM:SupportedCommand-CommandDescription

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Group_SupportedCommand_getId)(int skirmishAIId, int groupId, int supportedCommandId);

	const char*       (CALLING_CONV *Group_SupportedCommand_getName)(int skirmishAIId, int groupId, int supportedCommandId);

	const char*       (CALLING_CONV *Group_SupportedCommand_getToolTip)(int skirmishAIId, int groupId, int supportedCommandId);

	bool              (CALLING_CONV *Group_SupportedCommand_isShowUnique)(int skirmishAIId, int groupId, int supportedCommandId);

	bool              (CALLING_CONV *Group_SupportedCommand_isDisabled)(int skirmishAIId, int groupId, int supportedCommandId);

	int               (CALLING_CONV *Group_SupportedCommand_getParams)(int skirmishAIId, int groupId, int supportedCommandId, const char** params, int params_sizeMax); //$ ARRAY:params

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	int               (CALLING_CONV *Group_OrderPreview_getId)(int skirmishAIId, int groupId);

	short             (CALLING_CONV *Group_OrderPreview_getOptions)(int skirmishAIId, int groupId);

	int               (CALLING_CONV *Group_OrderPreview_getTag)(int skirmishAIId, int groupId);

	int               (CALLING_CONV *Group_OrderPreview_getTimeOut)(int skirmishAIId, int groupId);

	int               (CALLING_CONV *Group_OrderPreview_getParams)(int skirmishAIId, int groupId, float* params, int params_sizeMax); //$ ARRAY:params

	bool              (CALLING_CONV *Group_isSelected)(int skirmishAIId, int groupId);

// END OBJECT Group



// BEGINN OBJECT Mod

	/**
	 * Returns the mod archive file name.
	 * CAUTION:
	 * Never use this as reference in eg. cache- or config-file names,
	 * as one and the same mod can be packaged in different ways.
	 * Use the human name instead.
	 * @see getHumanName()
	 * @deprecated
	 */
	const char*       (CALLING_CONV *Mod_getFileName)(int skirmishAIId);

	/**
	 * Returns the archive hash of the mod.
	 * Use this for reference to the mod, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two mods not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getHumanName()
	 */
	int               (CALLING_CONV *Mod_getHash)(int skirmishAIId);

	/**
	 * Returns the human readable name of the mod, which includes the version.
	 * Use this for reference to the mod (including version), eg. in cache- or
	 * config-file names which are mod related, and wherever humans may come
	 * in contact with the reference.
	 * Be aware though, that this may contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Alternatively, you may use the short name only, or the short name plus
	 * version. You should generally never use the file name.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getHash()
	 * @see getShortName()
	 * @see getFileName()
	 * @see getVersion()
	 */
	const char*       (CALLING_CONV *Mod_getHumanName)(int skirmishAIId);

	/**
	 * Returns the short name of the mod, which does not include the version.
	 * Use this for reference to the mod in general, eg. as version independent
	 * reference.
	 * Be aware though, that this still contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getVersion()
	 * @see getHumanName()
	 */
	const char*       (CALLING_CONV *Mod_getShortName)(int skirmishAIId);

	const char*       (CALLING_CONV *Mod_getVersion)(int skirmishAIId);

	const char*       (CALLING_CONV *Mod_getMutator)(int skirmishAIId);

	const char*       (CALLING_CONV *Mod_getDescription)(int skirmishAIId);


	/**
	 * Should constructions without builders decay?
	 */
	bool              (CALLING_CONV *Mod_getConstructionDecay)(int skirmishAIId);

	/**
	 * How long until they start decaying?
	 */
	int               (CALLING_CONV *Mod_getConstructionDecayTime)(int skirmishAIId);

	/**
	 * How fast do they decay?
	 */
	float             (CALLING_CONV *Mod_getConstructionDecaySpeed)(int skirmishAIId);

	/**
	 * 0 = 1 reclaimer per feature max, otherwise unlimited
	 */
	int               (CALLING_CONV *Mod_getMultiReclaim)(int skirmishAIId);

	/**
	 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	 */
	int               (CALLING_CONV *Mod_getReclaimMethod)(int skirmishAIId);

	/**
	 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	 */
	int               (CALLING_CONV *Mod_getReclaimUnitMethod)(int skirmishAIId);

	/**
	 * How much energy should reclaiming a unit cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getReclaimUnitEnergyCostFactor)(int skirmishAIId);

	/**
	 * How much metal should reclaim return, default 1.0
	 */
	float             (CALLING_CONV *Mod_getReclaimUnitEfficiency)(int skirmishAIId);

	/**
	 * How much should energy should reclaiming a feature cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getReclaimFeatureEnergyCostFactor)(int skirmishAIId);

	/**
	 * Allow reclaiming enemies? default true
	 */
	bool              (CALLING_CONV *Mod_getReclaimAllowEnemies)(int skirmishAIId);

	/**
	 * Allow reclaiming allies? default true
	 */
	bool              (CALLING_CONV *Mod_getReclaimAllowAllies)(int skirmishAIId);

	/**
	 * How much should energy should repair cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getRepairEnergyCostFactor)(int skirmishAIId);

	/**
	 * How much should energy should resurrect cost, default 0.5
	 */
	float             (CALLING_CONV *Mod_getResurrectEnergyCostFactor)(int skirmishAIId);

	/**
	 * How much should energy should capture cost, default 0.0
	 */
	float             (CALLING_CONV *Mod_getCaptureEnergyCostFactor)(int skirmishAIId);

	/**
	 * 0 = all ground units cannot be transported,
	 * 1 = all ground units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 1.
	 */
	int               (CALLING_CONV *Mod_getTransportGround)(int skirmishAIId);

	/**
	 * 0 = all hover units cannot be transported,
	 * 1 = all hover units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportHover)(int skirmishAIId);

	/**
	 * 0 = all naval units cannot be transported,
	 * 1 = all naval units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportShip)(int skirmishAIId);

	/**
	 * 0 = all air units cannot be transported,
	 * 1 = all air units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
	int               (CALLING_CONV *Mod_getTransportAir)(int skirmishAIId);

	/**
	 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	 */
	int               (CALLING_CONV *Mod_getFireAtKilled)(int skirmishAIId);

	/**
	 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	 */
	int               (CALLING_CONV *Mod_getFireAtCrashing)(int skirmishAIId);

	/**
	 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	 */
	int               (CALLING_CONV *Mod_getFlankingBonusModeDefault)(int skirmishAIId);

	/**
	 * miplevel for los
	 */
	int               (CALLING_CONV *Mod_getLosMipLevel)(int skirmishAIId);

	/**
	 * miplevel to use for airlos
	 */
	int               (CALLING_CONV *Mod_getAirMipLevel)(int skirmishAIId);

	/**
	 * miplevel for radar
	 */
	int               (CALLING_CONV *Mod_getRadarMipLevel)(int skirmishAIId);

	/**
	 * when underwater, units are not in LOS unless also in sonar
	 */
	bool              (CALLING_CONV *Mod_getRequireSonarUnderWater)(int skirmishAIId);

// END OBJECT Mod



// BEGINN OBJECT Map
	int               (CALLING_CONV *Map_getChecksum)(int skirmishAIId);

	void              (CALLING_CONV *Map_getStartPos)(int skirmishAIId, float* return_posF3_out);

	void              (CALLING_CONV *Map_getMousePos)(int skirmishAIId, float* return_posF3_out);

	bool              (CALLING_CONV *Map_isPosInCamera)(int skirmishAIId, float* pos_posF3, float radius);

	/**
	 * Returns the maps center heightmap width.
	 * @see getHeightMap()
	 */
	int               (CALLING_CONV *Map_getWidth)(int skirmishAIId);

	/**
	 * Returns the maps center heightmap height.
	 * @see getHeightMap()
	 */
	int               (CALLING_CONV *Map_getHeight)(int skirmishAIId);

	/**
	 * Returns the height for the center of the squares.
	 * This differs slightly from the drawn map, since
	 * that one uses the height at the corners.
	 * Note that the actual map is 8 times larger (in each dimension) and
	 * all other maps (slope, los, resources, etc.) are relative to the
	 * size of the heightmap.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 8*8 in size
	 * - the value for the full resolution position (x, z) is at index (z * width + x)
	 * - the last value, bottom right, is at index (width * height - 1)
	 *
	 * @see getCornersHeightMap()
	 */
	int               (CALLING_CONV *Map_getHeightMap)(int skirmishAIId, float* heights, int heights_sizeMax); //$ ARRAY:heights

	/**
	 * Returns the height for the corners of the squares.
	 * This is the same like the drawn map.
	 * It is one unit wider and one higher then the centers height map.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - 4 points mark the edges of an area of 8*8 in size
	 * - the value for upper left corner of the full resolution position (x, z) is at index (z * width + x)
	 * - the last value, bottom right, is at index ((width+1) * (height+1) - 1)
	 *
	 * @see getHeightMap()
	 */
	int               (CALLING_CONV *Map_getCornersHeightMap)(int skirmishAIId, float* cornerHeights, int cornerHeights_sizeMax); //$ ARRAY:cornerHeights

	float             (CALLING_CONV *Map_getMinHeight)(int skirmishAIId);

	float             (CALLING_CONV *Map_getMaxHeight)(int skirmishAIId);

	/**
	 * @brief the slope map
	 * The values are 1 minus the y-component of the (average) facenormal of the square.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 2*2 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 */
	int               (CALLING_CONV *Map_getSlopeMap)(int skirmishAIId, float* slopes, int slopes_sizeMax); //$ ARRAY:slopes

	/**
	 * @brief the level of sight map
	 * mapDims.mapx >> losMipLevel
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
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / res)
	 * - the last value, bottom right, is at index (width/res * height/res - 1)
	 */
	int               (CALLING_CONV *Map_getLosMap)(int skirmishAIId, int* losValues, int losValues_sizeMax); //$ ARRAY:losValues

	/**
	 * @brief the level of sight map
	 * mapDims.mapx >> airMipLevel
	 * @see getLosMap()
	 */
	int               (CALLING_CONV *Map_getAirLosMap)(int skirmishAIId, int* airLosValues, int airLosValues_sizeMax); //$ ARRAY:airLosValues

	/**
	 * @brief the radar map
	 * mapDims.mapx >> radarMipLevel
	 * @see getLosMap()
	 */
	int               (CALLING_CONV *Map_getRadarMap)(int skirmishAIId, int* radarValues, int radarValues_sizeMax); //$ ARRAY:radarValues

	/** @see getRadarMap() */
	int               (CALLING_CONV *Map_getSonarMap)(int skirmishAIId, int* sonarValues, int sonarValues_sizeMax); //$ ARRAY:sonarValues

	/** @see getRadarMap() */
	int               (CALLING_CONV *Map_getSeismicMap)(int skirmishAIId, int* seismicValues, int seismicValues_sizeMax); //$ ARRAY:seismicValues

	/** @see getRadarMap() */
	int               (CALLING_CONV *Map_getJammerMap)(int skirmishAIId, int* jammerValues, int jammerValues_sizeMax); //$ ARRAY:jammerValues

	/** @see getRadarMap() */
	int               (CALLING_CONV *Map_getSonarJammerMap)(int skirmishAIId, int* sonarJammerValues, int sonarJammerValues_sizeMax); //$ ARRAY:sonarJammerValues

	/**
	 * @brief resource maps
	 * This map shows the resource density on the map.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 2*2 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 */
	int               (CALLING_CONV *Map_getResourceMapRaw)(int skirmishAIId, int resourceId, short* resources, int resources_sizeMax); //$ REF:resourceId->Resource ARRAY:resources

	/**
	 * Returns positions indicating where to place resource extractors on the map.
	 * Only the x and z values give the location of the spots, while the y values
	 * represents the actual amount of resource an extractor placed there can make.
	 * You should only compare the y values to each other, and not try to estimate
	 * effective output from spots.
	 */
	int               (CALLING_CONV *Map_getResourceMapSpotsPositions)(int skirmishAIId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax); //$ REF:resourceId->Resource ARRAY:spots_AposF3

	/**
	 * Returns the average resource income for an extractor on one of the evaluated positions.
	 */
	float             (CALLING_CONV *Map_getResourceMapSpotsAverageIncome)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	/**
	 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
	 */
	void              (CALLING_CONV *Map_getResourceMapSpotsNearest)(int skirmishAIId, int resourceId, float* pos_posF3, float* return_posF3_out); //$ REF:resourceId->Resource

	/**
	 * Returns the archive hash of the map.
	 * Use this for reference to the map, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two maps not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getName()
	 */
	int               (CALLING_CONV *Map_getHash)(int skirmishAIId);

	/**
	 * Returns the name of the map.
	 * Use this for reference to the map, eg. in cache- or config-file names
	 * which are map related, wherever humans may come in contact with the reference.
	 * Be aware though, that this may contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getHash()
	 * @see getHumanName()
	 */
	const char*       (CALLING_CONV *Map_getName)(int skirmishAIId);

	/**
	 * Returns the human readbale name of the map.
	 * @see getName()
	 */
	const char*       (CALLING_CONV *Map_getHumanName)(int skirmishAIId);

	/** Gets the elevation of the map at position (x, z) */
	float             (CALLING_CONV *Map_getElevationAt)(int skirmishAIId, float x, float z);


	/** Returns what value 255 in the resource map is worth */
	float             (CALLING_CONV *Map_getMaxResource)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	/** Returns extraction radius for resource extractors */
	float             (CALLING_CONV *Map_getExtractorRadius)(int skirmishAIId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *Map_getMinWind)(int skirmishAIId);

	float             (CALLING_CONV *Map_getMaxWind)(int skirmishAIId);

	float             (CALLING_CONV *Map_getCurWind)(int skirmishAIId);

	float             (CALLING_CONV *Map_getTidalStrength)(int skirmishAIId);

	float             (CALLING_CONV *Map_getGravity)(int skirmishAIId);

	float             (CALLING_CONV *Map_getWaterDamage)(int skirmishAIId);

	bool              (CALLING_CONV *Map_isDeformable)(int skirmishAIId);

	/** Returns global map hardness */
	float             (CALLING_CONV *Map_getHardness)(int skirmishAIId);

	/**
	 * Returns hardness modifiers of the squares adjusted by terrain type.
	 *
	 * - index 0 is top left
	 * - each data position is 2*2 in size (relative to heightmap)
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 *
	 * @see getHardness()
	 */
	int               (CALLING_CONV *Map_getHardnessModMap)(int skirmishAIId, float* hardMods, int hardMods_sizeMax); //$ ARRAY:hardMods

	/**
	 * Returns speed modifiers of the squares
	 * for specific speedModClass adjusted by terrain type.
	 *
	 * - index 0 is top left
	 * - each data position is 2*2 in size (relative to heightmap)
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 *
	 * @see MoveData#getSpeedModClass
	 */
	int               (CALLING_CONV *Map_getSpeedModMap)(int skirmishAIId, int speedModClass, float* speedMods, int speedMods_sizeMax); //$ ARRAY:speedMods

	/**
	 * Returns all points drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	int               (CALLING_CONV *Map_getPoints)(int skirmishAIId, bool includeAllies); //$ FETCHER:MULTI:NUM:Point

	void              (CALLING_CONV *Map_Point_getPosition)(int skirmishAIId, int pointId, float* return_posF3_out);

	void              (CALLING_CONV *Map_Point_getColor)(int skirmishAIId, int pointId, short* return_colorS3_out);

	const char*       (CALLING_CONV *Map_Point_getLabel)(int skirmishAIId, int pointId);

	/**
	 * Returns all lines drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	int               (CALLING_CONV *Map_getLines)(int skirmishAIId, bool includeAllies); //$ FETCHER:MULTI:NUM:Line

	void              (CALLING_CONV *Map_Line_getFirstPosition)(int skirmishAIId, int lineId, float* return_posF3_out);

	void              (CALLING_CONV *Map_Line_getSecondPosition)(int skirmishAIId, int lineId, float* return_posF3_out);

	void              (CALLING_CONV *Map_Line_getColor)(int skirmishAIId, int lineId, short* return_colorS3_out);

	bool              (CALLING_CONV *Map_isPossibleToBuildAt)(int skirmishAIId, int unitDefId, float* pos_posF3, int facing); //$ REF:unitDefId->UnitDef

	/**
	 * Returns the closest position from a given position that a building can be
	 * built at.
	 * @param minDist the distance in 1/(SQUARE_SIZE * 2) = 1/16 of full map
	 *                resolution, that the building must keep to other
	 *                buildings; this makes it easier to keep free paths through
	 *                a base
	 * @return actual map position with x, y and z all beeing positive,
	 *         or float[3]{-1, 0, 0} if no suitable position is found.
	 */
	void              (CALLING_CONV *Map_findClosestBuildSite)(int skirmishAIId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out); //$ REF:unitDefId->UnitDef

// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
	int               (CALLING_CONV *getFeatureDefs)(int skirmishAIId, int* featureDefIds, int featureDefIds_sizeMax); //$ FETCHER:MULTI:IDs:FeatureDef:featureDefIds


	const char*       (CALLING_CONV *FeatureDef_getName)(int skirmishAIId, int featureDefId);

	const char*       (CALLING_CONV *FeatureDef_getDescription)(int skirmishAIId, int featureDefId);


	float             (CALLING_CONV *FeatureDef_getContainedResource)(int skirmishAIId, int featureDefId, int resourceId); //$ REF:resourceId->Resource

	float             (CALLING_CONV *FeatureDef_getMaxHealth)(int skirmishAIId, int featureDefId);

	float             (CALLING_CONV *FeatureDef_getReclaimTime)(int skirmishAIId, int featureDefId);

	/** Used to see if the object can be overrun by units of a certain heavyness */
	float             (CALLING_CONV *FeatureDef_getMass)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isUpright)(int skirmishAIId, int featureDefId);

	int               (CALLING_CONV *FeatureDef_getDrawType)(int skirmishAIId, int featureDefId);

	const char*       (CALLING_CONV *FeatureDef_getModelName)(int skirmishAIId, int featureDefId);

	/**
	 * Used to determine whether the feature is resurrectable.
	 *
	 * @return  -1: (default) only if it is the 1st wreckage of
	 *              the UnitDef it originates from
	 *           0: no, never
	 *           1: yes, always
	 */
	int               (CALLING_CONV *FeatureDef_getResurrectable)(int skirmishAIId, int featureDefId);

	int               (CALLING_CONV *FeatureDef_getSmokeTime)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isDestructable)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isReclaimable)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isBlocking)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isBurnable)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isFloating)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isNoSelect)(int skirmishAIId, int featureDefId);

	bool              (CALLING_CONV *FeatureDef_isGeoThermal)(int skirmishAIId, int featureDefId);


	/**
	 * Size of the feature along the X axis - in other words: height.
	 * each size is 8 units
	 */
	int               (CALLING_CONV *FeatureDef_getXSize)(int skirmishAIId, int featureDefId);

	/**
	 * Size of the feature along the Z axis - in other words: width.
	 * each size is 8 units
	 */
	int               (CALLING_CONV *FeatureDef_getZSize)(int skirmishAIId, int featureDefId);

	int               (CALLING_CONV *FeatureDef_getCustomParams)(int skirmishAIId, int featureDefId, const char** keys, const char** values); //$ MAP

// END OBJECT FeatureDef


// BEGINN OBJECT Feature
	/**
	 * Returns all features currently in LOS, or all features on the map
	 * if cheating is enabled.
	 */
	int               (CALLING_CONV *getFeatures)(int skirmishAIId, int* featureIds, int featureIds_sizeMax); //$ REF:MULTI:featureIds->Feature

	/**
	 * Returns all features in a specified area that are currently in LOS,
	 * or all features in this area if cheating is enabled.
	 */
	int               (CALLING_CONV *getFeaturesIn)(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax); //$ REF:MULTI:featureIds->Feature

	int               (CALLING_CONV *Feature_getDef)(int skirmishAIId, int featureId); //$ REF:RETURN->FeatureDef

	float             (CALLING_CONV *Feature_getHealth)(int skirmishAIId, int featureId);

	float             (CALLING_CONV *Feature_getReclaimLeft)(int skirmishAIId, int featureId);

	void              (CALLING_CONV *Feature_getPosition)(int skirmishAIId, int featureId, float* return_posF3_out);

	/**
	 * @return float value of parameter if it's set, defaultValue otherwise.
	 */
	float             (CALLING_CONV *Feature_getRulesParamFloat)(int skirmishAIId, int unitId, const char* featureRulesParamName, float defaultValue);

	/**
	 * @return string value of parameter if it's set, defaultValue otherwise.
	 */
	const char*       (CALLING_CONV *Feature_getRulesParamString)(int skirmishAIId, int unitId, const char* featureRulesParamName, const char* defaultValue);

// END OBJECT Feature



// BEGINN OBJECT WeaponDef
	int               (CALLING_CONV *getWeaponDefs)(int skirmishAIId); //$ FETCHER:MULTI:NUM:WeaponDef

	int               (CALLING_CONV *getWeaponDefByName)(int skirmishAIId, const char* weaponDefName); //$ REF:RETURN->WeaponDef

	const char*       (CALLING_CONV *WeaponDef_getName)(int skirmishAIId, int weaponDefId);

	const char*       (CALLING_CONV *WeaponDef_getType)(int skirmishAIId, int weaponDefId);

	const char*       (CALLING_CONV *WeaponDef_getDescription)(int skirmishAIId, int weaponDefId);


	float             (CALLING_CONV *WeaponDef_getRange)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getHeightMod)(int skirmishAIId, int weaponDefId);

	/** Inaccuracy of whole burst */
	float             (CALLING_CONV *WeaponDef_getAccuracy)(int skirmishAIId, int weaponDefId);

	/** Inaccuracy of individual shots inside burst */
	float             (CALLING_CONV *WeaponDef_getSprayAngle)(int skirmishAIId, int weaponDefId);

	/** Inaccuracy while owner moving */
	float             (CALLING_CONV *WeaponDef_getMovingAccuracy)(int skirmishAIId, int weaponDefId);

	/** Fraction of targets move speed that is used as error offset */
	float             (CALLING_CONV *WeaponDef_getTargetMoveError)(int skirmishAIId, int weaponDefId);

	/** Maximum distance the weapon will lead the target */
	float             (CALLING_CONV *WeaponDef_getLeadLimit)(int skirmishAIId, int weaponDefId);

	/** Factor for increasing the leadLimit with experience */
	float             (CALLING_CONV *WeaponDef_getLeadBonus)(int skirmishAIId, int weaponDefId);

	/** Replaces hardcoded behaviour for burnblow cannons */
	float             (CALLING_CONV *WeaponDef_getPredictBoost)(int skirmishAIId, int weaponDefId);

//	TODO: Deprecate the following function, if no longer needed by legacy Cpp AIs
	int               (CALLING_CONV *WeaponDef_getNumDamageTypes)(int skirmishAIId); //$ STATIC

//	DamageArray (CALLING_CONV *WeaponDef_getDamages)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_Damage_getParalyzeDamageTime)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_Damage_getImpulseFactor)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_Damage_getImpulseBoost)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_Damage_getCraterMult)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_Damage_getCraterBoost)(int skirmishAIId, int weaponDefId);

//	float (CALLING_CONV *WeaponDef_Damage_getType)(int skirmishAIId, int weaponDefId, int typeId);

	int               (CALLING_CONV *WeaponDef_Damage_getTypes)(int skirmishAIId, int weaponDefId, float* types, int types_sizeMax); //$ ARRAY:types


	float             (CALLING_CONV *WeaponDef_getAreaOfEffect)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isNoSelfDamage)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getFireStarter)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getEdgeEffectiveness)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getSize)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getSizeGrowth)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getCollisionSize)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_getSalvoSize)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getSalvoDelay)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getReload)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getBeamTime)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isBeamBurst)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isWaterBounce)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isGroundBounce)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getBounceRebound)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getBounceSlip)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_getNumBounce)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getMaxAngle)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getUpTime)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_getFlightTime)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getCost)(int skirmishAIId, int weaponDefId, int resourceId); //$ REF:resourceId->Resource

	int               (CALLING_CONV *WeaponDef_getProjectilesPerShot)(int skirmishAIId, int weaponDefId);

//	/** The "id=" tag in the TDF */
//	int (CALLING_CONV *WeaponDef_getTdfId)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isTurret)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isOnlyForward)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isFixedLauncher)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isWaterWeapon)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isFireSubmersed)(int skirmishAIId, int weaponDefId);

	/** Lets a torpedo travel above water like it does below water */
	bool              (CALLING_CONV *WeaponDef_isSubMissile)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isTracks)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isDropped)(int skirmishAIId, int weaponDefId);

	/** The weapon will only paralyze, not do real damage. */
	bool              (CALLING_CONV *WeaponDef_isParalyzer)(int skirmishAIId, int weaponDefId);

	/** The weapon damages by impacting, not by exploding. */
	bool              (CALLING_CONV *WeaponDef_isImpactOnly)(int skirmishAIId, int weaponDefId);

	/** Can not target anything (for example: anti-nuke, D-Gun) */
	bool              (CALLING_CONV *WeaponDef_isNoAutoTarget)(int skirmishAIId, int weaponDefId);

	/** Has to be fired manually (by the player or an AI, example: D-Gun) */
	bool              (CALLING_CONV *WeaponDef_isManualFire)(int skirmishAIId, int weaponDefId);

	/**
	 * Can intercept targetable weapons shots.
	 *
	 * example: anti-nuke
	 *
	 * @see  getTargetable()
	 */
	int               (CALLING_CONV *WeaponDef_getInterceptor)(int skirmishAIId, int weaponDefId);

	/**
	 * Shoots interceptable projectiles.
	 * Shots can be intercepted by interceptors.
	 *
	 * example: nuke
	 *
	 * @see  getInterceptor()
	 */
	int               (CALLING_CONV *WeaponDef_getTargetable)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isStockpileable)(int skirmishAIId, int weaponDefId);

	/**
	 * Range of interceptors.
	 *
	 * example: anti-nuke
	 *
	 * @see  getInterceptor()
	 */
	float             (CALLING_CONV *WeaponDef_getCoverageRange)(int skirmishAIId, int weaponDefId);

	/** Build time of a missile */
	float             (CALLING_CONV *WeaponDef_getStockpileTime)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getIntensity)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getDuration)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getFalloffRate)(int skirmishAIId, int weaponDefId);


	bool              (CALLING_CONV *WeaponDef_isSoundTrigger)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isSelfExplode)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isGravityAffected)(int skirmishAIId, int weaponDefId);

	/**
	 * Per weapon high trajectory setting.
	 * UnitDef also has this property.
	 *
	 * @return  0: low
	 *          1: high
	 *          2: unit
	 */
	int               (CALLING_CONV *WeaponDef_getHighTrajectory)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getMyGravity)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isNoExplode)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getStartVelocity)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getWeaponAcceleration)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getTurnRate)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getMaxVelocity)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getProjectileSpeed)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getExplosionSpeed)(int skirmishAIId, int weaponDefId);

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
	int               (CALLING_CONV *WeaponDef_getOnlyTargetCategory)(int skirmishAIId, int weaponDefId);

	/** How much the missile will wobble around its course. */
	float             (CALLING_CONV *WeaponDef_getWobble)(int skirmishAIId, int weaponDefId);

	/** How much the missile will dance. */
	float             (CALLING_CONV *WeaponDef_getDance)(int skirmishAIId, int weaponDefId);

	/** How high trajectory missiles will try to fly in. */
	float             (CALLING_CONV *WeaponDef_getTrajectoryHeight)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isLargeBeamLaser)(int skirmishAIId, int weaponDefId);

	/** If the weapon is a shield rather than a weapon. */
	bool              (CALLING_CONV *WeaponDef_isShield)(int skirmishAIId, int weaponDefId);

	/** If the weapon should be repulsed or absorbed. */
	bool              (CALLING_CONV *WeaponDef_isShieldRepulser)(int skirmishAIId, int weaponDefId);

	/** If the shield only affects enemy projectiles. */
	bool              (CALLING_CONV *WeaponDef_isSmartShield)(int skirmishAIId, int weaponDefId);

	/** If the shield only affects stuff coming from outside shield radius. */
	bool              (CALLING_CONV *WeaponDef_isExteriorShield)(int skirmishAIId, int weaponDefId);

	/** If the shield should be graphically shown. */
	bool              (CALLING_CONV *WeaponDef_isVisibleShield)(int skirmishAIId, int weaponDefId);

	/** If a small graphic should be shown at each repulse. */
	bool              (CALLING_CONV *WeaponDef_isVisibleShieldRepulse)(int skirmishAIId, int weaponDefId);

	/** The number of frames to draw the shield after it has been hit. */
	int               (CALLING_CONV *WeaponDef_getVisibleShieldHitFrames)(int skirmishAIId, int weaponDefId);

	/**
	 * Amount of the resource used per shot or per second,
	 * depending on the type of projectile.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getResourceUse)(int skirmishAIId, int weaponDefId, int resourceId); //$ REF:resourceId->Resource

	/** Size of shield covered area */
	float             (CALLING_CONV *WeaponDef_Shield_getRadius)(int skirmishAIId, int weaponDefId);

	/**
	 * Shield acceleration on plasma stuff.
	 * How much will plasma be accelerated into the other direction
	 * when it hits the shield.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getForce)(int skirmishAIId, int weaponDefId);

	/** Maximum speed to which the shield can repulse plasma. */
	float             (CALLING_CONV *WeaponDef_Shield_getMaxSpeed)(int skirmishAIId, int weaponDefId);

	/** Amount of damage the shield can reflect. (0=infinite) */
	float             (CALLING_CONV *WeaponDef_Shield_getPower)(int skirmishAIId, int weaponDefId);

	/** Amount of power that is regenerated per second. */
	float             (CALLING_CONV *WeaponDef_Shield_getPowerRegen)(int skirmishAIId, int weaponDefId);

	/**
	 * How much of a given resource is needed to regenerate power
	 * with max speed per second.
	 */
	float             (CALLING_CONV *WeaponDef_Shield_getPowerRegenResource)(int skirmishAIId, int weaponDefId, int resourceId); //$ REF:resourceId->Resource

	/** How much power the shield has when it is created. */
	float             (CALLING_CONV *WeaponDef_Shield_getStartingPower)(int skirmishAIId, int weaponDefId);

	/** Number of frames to delay recharging by after each hit. */
	int               (CALLING_CONV *WeaponDef_Shield_getRechargeDelay)(int skirmishAIId, int weaponDefId);


	/**
	 * The type of the shield (bitfield).
	 * Defines what weapons can be intercepted by the shield.
	 *
	 * @see  getInterceptedByShieldType()
	 */
	int               (CALLING_CONV *WeaponDef_Shield_getInterceptType)(int skirmishAIId, int weaponDefId);

	/**
	 * The type of shields that can intercept this weapon (bitfield).
	 * The weapon can be affected by shields if:
	 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
	 *
	 * @see  getInterceptType()
	 */
	int               (CALLING_CONV *WeaponDef_getInterceptedByShieldType)(int skirmishAIId, int weaponDefId);

	/** Tries to avoid friendly units while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidFriendly)(int skirmishAIId, int weaponDefId);

	/** Tries to avoid features while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidFeature)(int skirmishAIId, int weaponDefId);

	/** Tries to avoid neutral units while aiming? */
	bool              (CALLING_CONV *WeaponDef_isAvoidNeutral)(int skirmishAIId, int weaponDefId);

	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
	float             (CALLING_CONV *WeaponDef_getTargetBorder)(int skirmishAIId, int weaponDefId);

	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
	float             (CALLING_CONV *WeaponDef_getCylinderTargetting)(int skirmishAIId, int weaponDefId);

	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
	float             (CALLING_CONV *WeaponDef_getMinIntensity)(int skirmishAIId, int weaponDefId);

	/**
	 * Controls cannon range height boost.
	 *
	 * default: -1: automatically calculate a more or less sane value
	 */
	float             (CALLING_CONV *WeaponDef_getHeightBoostFactor)(int skirmishAIId, int weaponDefId);

	/** Multiplier for the distance to the target for priority calculations. */
	float             (CALLING_CONV *WeaponDef_getProximityPriority)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_getCollisionFlags)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isSweepFire)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isAbleToAttackGround)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getCameraShake)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getDynDamageExp)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getDynDamageMin)(int skirmishAIId, int weaponDefId);

	float             (CALLING_CONV *WeaponDef_getDynDamageRange)(int skirmishAIId, int weaponDefId);

	bool              (CALLING_CONV *WeaponDef_isDynDamageInverted)(int skirmishAIId, int weaponDefId);

	int               (CALLING_CONV *WeaponDef_getCustomParams)(int skirmishAIId, int weaponDefId, const char** keys, const char** values); //$ MAP

// END OBJECT WeaponDef


// BEGINN OBJECT Weapon
	int               (CALLING_CONV *Unit_Weapon_getDef)(int skirmishAIId, int unitId, int weaponId); //$ REF:RETURN->WeaponDef

	/** Next tick the weapon can fire again. */
	int               (CALLING_CONV *Unit_Weapon_getReloadFrame)(int skirmishAIId, int unitId, int weaponId);

	/** Time between succesive fires in ticks. */
	int               (CALLING_CONV *Unit_Weapon_getReloadTime)(int skirmishAIId, int unitId, int weaponId);

	float             (CALLING_CONV *Unit_Weapon_getRange)(int skirmishAIId, int unitId, int weaponId);

	bool              (CALLING_CONV *Unit_Weapon_isShieldEnabled)(int skirmishAIId, int unitId, int weaponId);

	float             (CALLING_CONV *Unit_Weapon_getShieldPower)(int skirmishAIId, int unitId, int weaponId);

// END OBJECT Weapon

	bool              (CALLING_CONV *Debug_GraphDrawer_isEnabled)(int skirmishAIId);

};

#if	defined(__cplusplus)
} // extern "C"
#endif

#endif // S_SKIRMISH_AI_CALLBACK_H
