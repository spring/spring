/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _COMBINED_CALLBACK_BRIDGE_H
#define _COMBINED_CALLBACK_BRIDGE_H

#include "ExternalAI/Interface/aidefines.h"

#include <stdlib.h>  // size_t
#if !defined(__cplusplus)
	#if defined(_MSC_VER)
		#include "System/booldefines.h" // bool, true, false
	#else
		#include <stdbool.h> // bool, true, false
	#endif
#endif

struct SSkirmishAICallback;

#ifdef __cplusplus
extern "C" {
#endif

void funcPntBrdg_addCallback(const size_t skirmishAIId, const struct SSkirmishAICallback* clb);
void funcPntBrdg_removeCallback(const size_t skirmishAIId);

/**
 * Returns the major engine revision number (e.g. 83)
 */
EXPORT(const char*) bridged_Engine_Version_getMajor(int skirmishAIId);

/**
 * Minor version number (e.g. "5")
 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
 */
EXPORT(const char*) bridged_Engine_Version_getMinor(int skirmishAIId);

/**
 * Clients that only differ in patchset can still play together.
 * Also demos should be compatible between patchsets.
 */
EXPORT(const char*) bridged_Engine_Version_getPatchset(int skirmishAIId);

/**
 * SCM Commits version part (e.g. "" or "13")
 * Number of commits since the last version tag.
 * This matches the regex "[0-9]*".
 */
EXPORT(const char*) bridged_Engine_Version_getCommits(int skirmishAIId);

/**
 * SCM unique identifier for the current commit.
 * This matches the regex "([0-9a-f]{6})?".
 */
EXPORT(const char*) bridged_Engine_Version_getHash(int skirmishAIId);

/**
 * SCM branch name (e.g. "master" or "develop")
 */
EXPORT(const char*) bridged_Engine_Version_getBranch(int skirmishAIId);

/**
 * Additional information (compiler flags, svn revision etc.)
 */
EXPORT(const char*) bridged_Engine_Version_getAdditional(int skirmishAIId);

/**
 * time of build
 */
EXPORT(const char*) bridged_Engine_Version_getBuildTime(int skirmishAIId);

/**
 * Returns whether this is a release build of the engine
 */
EXPORT(bool) bridged_Engine_Version_isRelease(int skirmishAIId);

/**
 * The basic part of a spring version.
 * This may only be used for sync-checking if IsRelease() returns true.
 * @return "Major.PatchSet" or "Major.PatchSet.1"
 */
EXPORT(const char*) bridged_Engine_Version_getNormal(int skirmishAIId);

/**
 * The sync relevant part of a spring version.
 * This may be used for sync-checking through a simple string-equality test.
 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
 */
EXPORT(const char*) bridged_Engine_Version_getSync(int skirmishAIId);

/**
 * The verbose, human readable version.
 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
 */
EXPORT(const char*) bridged_Engine_Version_getFull(int skirmishAIId);

/**
 * Returns the number of teams in this game
 */
EXPORT(int) bridged_Teams_getSize(int skirmishAIId);

/**
 * Returns the number of skirmish AIs in this game
 */
EXPORT(int) bridged_SkirmishAIs_getSize(int skirmishAIId);

/**
 * Returns the maximum number of skirmish AIs in any game
 */
EXPORT(int) bridged_SkirmishAIs_getMax(int skirmishAIId);

/**
 * Returns the ID of the team controled by this Skirmish AI.
 */
EXPORT(int) bridged_SkirmishAI_getTeamId(int skirmishAIId);

/**
 * Returns the number of info key-value pairs in the info map
 * for this Skirmish AI.
 */
EXPORT(int) bridged_SkirmishAI_Info_getSize(int skirmishAIId);

/**
 * Returns the key at index infoIndex in the info map
 * for this Skirmish AI, or NULL if the infoIndex is invalid.
 */
EXPORT(const char*) bridged_SkirmishAI_Info_getKey(int skirmishAIId, int infoIndex);

/**
 * Returns the value at index infoIndex in the info map
 * for this Skirmish AI, or NULL if the infoIndex is invalid.
 */
EXPORT(const char*) bridged_SkirmishAI_Info_getValue(int skirmishAIId, int infoIndex);

/**
 * Returns the description of the key at index infoIndex in the info map
 * for this Skirmish AI, or NULL if the infoIndex is invalid.
 */
EXPORT(const char*) bridged_SkirmishAI_Info_getDescription(int skirmishAIId, int infoIndex);

/**
 * Returns the value associated with the given key in the info map
 * for this Skirmish AI, or NULL if not found.
 */
EXPORT(const char*) bridged_SkirmishAI_Info_getValueByKey(int skirmishAIId, const char* const key);

/**
 * Returns the number of option key-value pairs in the options map
 * for this Skirmish AI.
 */
EXPORT(int) bridged_SkirmishAI_OptionValues_getSize(int skirmishAIId);

/**
 * Returns the key at index optionIndex in the options map
 * for this Skirmish AI, or NULL if the optionIndex is invalid.
 */
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getKey(int skirmishAIId, int optionIndex);

/**
 * Returns the value at index optionIndex in the options map
 * for this Skirmish AI, or NULL if the optionIndex is invalid.
 */
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getValue(int skirmishAIId, int optionIndex);

/**
 * Returns the value associated with the given key in the options map
 * for this Skirmish AI, or NULL if not found.
 */
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getValueByKey(int skirmishAIId, const char* const key);

/**
 * This will end up in infolog
 */
EXPORT(void) bridged_Log_log(int skirmishAIId, const char* const msg);

/**
 * Inform the engine of an error that happend in the interface.
 * @param   msg       error message
 * @param   severety  from 10 for minor to 0 for fatal
 * @param   die       if this is set to true, the engine assumes
 *                    the interface is in an irreparable state, and it will
 *                    unload it immediately.
 */
EXPORT(void) bridged_Log_exception(int skirmishAIId, const char* const msg, int severety, bool die);

/**
 * Returns '/' on posix and '\\' on windows
 */
EXPORT(char) bridged_DataDirs_getPathSeparator(int skirmishAIId);

/**
 * This interfaces main data dir, which is where the shared library
 * and the InterfaceInfo.lua file are located, e.g.:
 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
 */
EXPORT(const char*) bridged_DataDirs_getConfigDir(int skirmishAIId);

/**
 * This interfaces writable data dir, which is where eg logs, caches
 * and learning data should be stored, e.g.:
 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
 */
EXPORT(const char*) bridged_DataDirs_getWriteableDir(int skirmishAIId);

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
EXPORT(bool) bridged_DataDirs_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);

/**
 * @see     locatePath()
 */
EXPORT(char*) bridged_DataDirs_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir, bool common);

/**
 * Returns the number of springs data dirs.
 */
EXPORT(int) bridged_DataDirs_Roots_getSize(int skirmishAIId);

/**
 * Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1).
 */
EXPORT(bool) bridged_DataDirs_Roots_getDir(int skirmishAIId, char* path, int path_sizeMax, int dirIndex);

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
EXPORT(bool) bridged_DataDirs_Roots_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);

EXPORT(char*) bridged_DataDirs_Roots_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir);

/**
 * Returns the current game time measured in frames (the
 * simulation runs at 30 frames per second at normal speed)
 * 
 * This should not be used, as we get the frame from the SUpdateEvent.
 * @deprecated
 */
EXPORT(int) bridged_Game_getCurrentFrame(int skirmishAIId);

EXPORT(int) bridged_Game_getAiInterfaceVersion(int skirmishAIId);

EXPORT(int) bridged_Game_getMyTeam(int skirmishAIId);

EXPORT(int) bridged_Game_getMyAllyTeam(int skirmishAIId);

EXPORT(int) bridged_Game_getPlayerTeam(int skirmishAIId, int playerId);

/**
 * Returns the number of active teams participating
 * in the currently running game.
 * A return value of 6 for example, would mean that teams 0 till 5
 * take part in the game.
 */
EXPORT(int) bridged_Game_getTeams(int skirmishAIId);

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
EXPORT(const char*) bridged_Game_getTeamSide(int skirmishAIId, int otherTeamId);

/**
 * Returns the color of a team in the game.
 * 
 * This should only be used when drawing stuff,
 * and not for team-identification.
 * @return the RGB color of a team, with values in [0, 255]
 */
EXPORT(void) bridged_Game_getTeamColor(int skirmishAIId, int otherTeamId, short* return_colorS3_out);

/**
 * Returns the income multiplier of a team in the game.
 * All the teams resource income is multiplied by this factor.
 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
 */
EXPORT(float) bridged_Game_getTeamIncomeMultiplier(int skirmishAIId, int otherTeamId);

/**
 * Returns the ally-team of a team
 */
EXPORT(int) bridged_Game_getTeamAllyTeam(int skirmishAIId, int otherTeamId);

/**
 * Returns the current level of a resource of another team.
 * Allways works for allied teams.
 * Works for all teams when cheating is enabled.
 * @return current level of the requested resource of the other team, or -1.0 on an invalid request
 */
EXPORT(float) bridged_Game_getTeamResourceCurrent(int skirmishAIId, int otherTeamId, int resourceId);

/**
 * Returns the current income of a resource of another team.
 * Allways works for allied teams.
 * Works for all teams when cheating is enabled.
 * @return current income of the requested resource of the other team, or -1.0 on an invalid request
 */
EXPORT(float) bridged_Game_getTeamResourceIncome(int skirmishAIId, int otherTeamId, int resourceId);

/**
 * Returns the current usage of a resource of another team.
 * Allways works for allied teams.
 * Works for all teams when cheating is enabled.
 * @return current usage of the requested resource of the other team, or -1.0 on an invalid request
 */
EXPORT(float) bridged_Game_getTeamResourceUsage(int skirmishAIId, int otherTeamId, int resourceId);

/**
 * Returns the storage capacity for a resource of another team.
 * Allways works for allied teams.
 * Works for all teams when cheating is enabled.
 * @return storage capacity for the requested resource of the other team, or -1.0 on an invalid request
 */
EXPORT(float) bridged_Game_getTeamResourceStorage(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float) bridged_Game_getTeamResourcePull(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float) bridged_Game_getTeamResourceShare(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float) bridged_Game_getTeamResourceSent(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float) bridged_Game_getTeamResourceReceived(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float) bridged_Game_getTeamResourceExcess(int skirmishAIId, int otherTeamId, int resourceId);

/**
 * Returns true, if the two supplied ally-teams are currently allied
 */
EXPORT(bool) bridged_Game_isAllied(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId);

EXPORT(bool) bridged_Game_isExceptionHandlingEnabled(int skirmishAIId);

EXPORT(bool) bridged_Game_isDebugModeEnabled(int skirmishAIId);

EXPORT(int) bridged_Game_getMode(int skirmishAIId);

EXPORT(bool) bridged_Game_isPaused(int skirmishAIId);

EXPORT(float) bridged_Game_getSpeedFactor(int skirmishAIId);

EXPORT(const char*) bridged_Game_getSetupScript(int skirmishAIId);

/**
 * Returns the categories bit field value.
 * @return the categories bit field value or 0,
 *         in case of empty name or too many categories
 * @see getCategoryName
 */
EXPORT(int) bridged_Game_getCategoryFlag(int skirmishAIId, const char* categoryName);

/**
 * Returns the bitfield values of a list of category names.
 * @param categoryNames space delimited list of names
 * @see Game#getCategoryFlag
 */
EXPORT(int) bridged_Game_getCategoriesFlag(int skirmishAIId, const char* categoryNames);

/**
 * Return the name of the category described by a category flag.
 * @see Game#getCategoryFlag
 */
EXPORT(void) bridged_Game_getCategoryName(int skirmishAIId, int categoryFlag, char* name, int name_sizeMax);

/**
 * This is a set of parameters that is created by SetGameRulesParam() and may change during the game.
 * Each parameter is uniquely identified only by its id (which is the index in the vector).
 * Parameters may or may not have a name.
 * @return visible to skirmishAIId parameters.
 * If cheats are enabled, this will return all parameters.
 */
EXPORT(int) bridged_Game_getGameRulesParams(int skirmishAIId, int* paramIds, int paramIds_sizeMax); // FETCHER:MULTI:IDs:GameRulesParam:paramIds

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Game_getGameRulesParamByName(int skirmishAIId, const char* rulesParamName); // REF:RETURN->GameRulesParam

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Game_getGameRulesParamById(int skirmishAIId, int rulesParamId); // REF:RETURN->GameRulesParam

/**
 * Not every mod parameter has a name.
 */
EXPORT(const char*) bridged_GameRulesParam_getName(int skirmishAIId, int gameRulesParamId);

/**
 * @return float value of parameter if it's set, 0.0 otherwise.
 */
EXPORT(float) bridged_GameRulesParam_getValueFloat(int skirmishAIId, int gameRulesParamId);

/**
 * @return string value of parameter if it's set, empty string otherwise.
 */
EXPORT(const char*) bridged_GameRulesParam_getValueString(int skirmishAIId, int gameRulesParamId);

EXPORT(float) bridged_Gui_getViewRange(int skirmishAIId);

EXPORT(float) bridged_Gui_getScreenX(int skirmishAIId);

EXPORT(float) bridged_Gui_getScreenY(int skirmishAIId);

EXPORT(void) bridged_Gui_Camera_getDirection(int skirmishAIId, float* return_posF3_out);

EXPORT(void) bridged_Gui_Camera_getPosition(int skirmishAIId, float* return_posF3_out);

/**
 * Returns whether this AI may use active cheats.
 */
EXPORT(bool) bridged_Cheats_isEnabled(int skirmishAIId);

/**
 * Set whether this AI may use active cheats.
 */
EXPORT(bool) bridged_Cheats_setEnabled(int skirmishAIId, bool enable);

/**
 * Set whether this AI may receive cheat events.
 * When enabled, you would for example get informed when enemy units are
 * created, even without sensor coverage.
 */
EXPORT(bool) bridged_Cheats_setEventsEnabled(int skirmishAIId, bool enabled);

/**
 * Returns whether cheats will desync if used by an AI.
 * @return always true, unless we are both the host and the only client.
 */
EXPORT(bool) bridged_Cheats_isOnlyPassive(int skirmishAIId);

EXPORT(int) bridged_getResources(int skirmishAIId); // FETCHER:MULTI:NUM:Resource

EXPORT(int) bridged_getResourceByName(int skirmishAIId, const char* resourceName); // REF:RETURN->Resource

EXPORT(const char*) bridged_Resource_getName(int skirmishAIId, int resourceId);

EXPORT(float) bridged_Resource_getOptimum(int skirmishAIId, int resourceId);

EXPORT(float) bridged_Economy_getCurrent(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getIncome(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getUsage(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getStorage(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getPull(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getShare(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getSent(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getReceived(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Economy_getExcess(int skirmishAIId, int resourceId); // REF:resourceId->Resource

/**
 * Return -1 when the file does not exist
 */
EXPORT(int) bridged_File_getSize(int skirmishAIId, const char* fileName);

/**
 * Returns false when file does not exist, or the buffer is too small
 */
EXPORT(bool) bridged_File_getContent(int skirmishAIId, const char* fileName, void* buffer, int bufferLen);

/**
 * A UnitDef contains all properties of a unit that are specific to its type,
 * for example the number and type of weapons or max-speed.
 * These properties are usually fixed, and not meant to change during a game.
 * The unitId is a unique id for this type of unit.
 */
EXPORT(int) bridged_getUnitDefs(int skirmishAIId, int* unitDefIds, int unitDefIds_sizeMax); // FETCHER:MULTI:IDs:UnitDef:unitDefIds

EXPORT(int) bridged_getUnitDefByName(int skirmishAIId, const char* unitName); // REF:RETURN->UnitDef

/**
 * Forces loading of the unit model
 */
EXPORT(float) bridged_UnitDef_getHeight(int skirmishAIId, int unitDefId);

/**
 * Forces loading of the unit model
 */
EXPORT(float) bridged_UnitDef_getRadius(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_getName(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_getHumanName(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_getFileName(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(int) bridged_UnitDef_getAiHint(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getCobId(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getTechLevel(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(const char*) bridged_UnitDef_getGaia(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getUpkeep(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

/**
 * This amount of the resource will always be created.
 */
EXPORT(float) bridged_UnitDef_getResourceMake(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

/**
 * This amount of the resource will be created when the unit is on and enough
 * energy can be drained.
 */
EXPORT(float) bridged_UnitDef_getMakesResource(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getCost(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getExtractsResource(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getResourceExtractorRange(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getWindResourceGenerator(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getTidalResourceGenerator(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getStorage(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(bool) bridged_UnitDef_isSquareResourceExtractor(int skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_UnitDef_getBuildTime(int skirmishAIId, int unitDefId);

/**
 * This amount of auto-heal will always be applied.
 */
EXPORT(float) bridged_UnitDef_getAutoHeal(int skirmishAIId, int unitDefId);

/**
 * This amount of auto-heal will only be applied while the unit is idling.
 */
EXPORT(float) bridged_UnitDef_getIdleAutoHeal(int skirmishAIId, int unitDefId);

/**
 * Time a unit needs to idle before it is considered idling.
 */
EXPORT(int) bridged_UnitDef_getIdleTime(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getPower(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getHealth(int skirmishAIId, int unitDefId);

/**
 * Returns the bit field value denoting the categories this unit is in.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(int) bridged_UnitDef_getCategory(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTurnRate(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isTurnInPlace(int skirmishAIId, int unitDefId);

/**
 * Units above this distance to goal will try to turn while keeping
 * some of their speed.
 * 0 to disable
 */
EXPORT(float) bridged_UnitDef_getTurnInPlaceDistance(int skirmishAIId, int unitDefId);

/**
 * Units below this speed will turn in place regardless of their
 * turnInPlace setting.
 */
EXPORT(float) bridged_UnitDef_getTurnInPlaceSpeedLimit(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isUpright(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isCollide(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getLosRadius(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getAirLosRadius(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getLosHeight(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getRadarRadius(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getSonarRadius(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getJammerRadius(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getSonarJamRadius(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getSeismicRadius(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getSeismicSignature(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isStealth(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isSonarStealth(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isBuildRange3D(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getBuildDistance(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getBuildSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getReclaimSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getRepairSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxRepairSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getResurrectSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getCaptureSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTerraformSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMass(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isPushResistant(int skirmishAIId, int unitDefId);

/**
 * Should the unit move sideways when it can not shoot?
 */
EXPORT(bool) bridged_UnitDef_isStrafeToAttack(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMinCollisionSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getSlideTolerance(int skirmishAIId, int unitDefId);

/**
 * Build location relevant maximum steepness of the underlaying terrain.
 * Used to calculate the maxHeightDif.
 */
EXPORT(float) bridged_UnitDef_getMaxSlope(int skirmishAIId, int unitDefId);

/**
 * Maximum terra-form height this building allows.
 * If this value is 0.0, you can only build this structure on
 * totally flat terrain.
 */
EXPORT(float) bridged_UnitDef_getMaxHeightDif(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMinWaterDepth(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getWaterline(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxWaterDepth(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getArmoredMultiple(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getArmorType(int skirmishAIId, int unitDefId);

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
EXPORT(int) bridged_UnitDef_FlankingBonus_getMode(int skirmishAIId, int unitDefId);

/**
 * The unit takes less damage when attacked from this direction.
 * This encourage flanking fire.
 */
EXPORT(void) bridged_UnitDef_FlankingBonus_getDir(int skirmishAIId, int unitDefId, float* return_posF3_out);

/**
 * Damage factor for the least protected direction
 */
EXPORT(float) bridged_UnitDef_FlankingBonus_getMax(int skirmishAIId, int unitDefId);

/**
 * Damage factor for the most protected direction
 */
EXPORT(float) bridged_UnitDef_FlankingBonus_getMin(int skirmishAIId, int unitDefId);

/**
 * How much the ability of the flanking bonus direction to move builds up each
 * frame.
 */
EXPORT(float) bridged_UnitDef_FlankingBonus_getMobilityAdd(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxWeaponRange(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(const char*) bridged_UnitDef_getType(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_getTooltip(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_getWreckName(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getDeathExplosion(int skirmishAIId, int unitDefId); // REF:RETURN->WeaponDef

EXPORT(int) bridged_UnitDef_getSelfDExplosion(int skirmishAIId, int unitDefId); // REF:RETURN->WeaponDef

/**
 * Returns the name of the category this unit is in.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(const char*) bridged_UnitDef_getCategoryString(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToSelfD(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getSelfDCountdown(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToSubmerge(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToFly(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToMove(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToHover(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isFloater(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isBuilder(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isActivateWhenBuilt(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isOnOffable(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isFullHealthFactory(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isFactoryHeadingTakeoff(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isReclaimable(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isCapturable(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToRestore(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToRepair(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToSelfRepair(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToReclaim(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToAttack(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToPatrol(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToFight(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToGuard(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToAssist(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAssistable(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToRepeat(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToFireControl(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getFireState(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getMoveState(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getWingDrag(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getWingAngle(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getDrag(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getFrontToSpeed(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getSpeedToFront(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMyGravity(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxBank(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxPitch(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTurnRadius(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getWantedHeight(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getVerticalSpeed(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(bool) bridged_UnitDef_isAbleToCrash(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(bool) bridged_UnitDef_isHoverAttack(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAirStrafe(int skirmishAIId, int unitDefId);

/**
 * @return  < 0:  it can land
 *          >= 0: how much the unit will move during hovering on the spot
 */
EXPORT(float) bridged_UnitDef_getDlHoverFactor(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxAcceleration(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxDeceleration(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxAileron(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxElevator(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMaxRudder(int skirmishAIId, int unitDefId);

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
EXPORT(int) bridged_UnitDef_getYardMap(int skirmishAIId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax); // ARRAY:yardMap

EXPORT(int) bridged_UnitDef_getXSize(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getZSize(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(int) bridged_UnitDef_getBuildAngle(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getLoadingRadius(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getUnloadSpread(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getTransportCapacity(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getTransportSize(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getMinTransportSize(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAirBase(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isFirePlatform(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTransportMass(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getMinTransportMass(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isHoldSteady(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isReleaseHeld(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isNotTransportable(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isTransportByEnemy(int skirmishAIId, int unitDefId);

/**
 * @return  0: land unload
 *          1: fly-over drop
 *          2: land flood
 */
EXPORT(int) bridged_UnitDef_getTransportUnloadMethod(int skirmishAIId, int unitDefId);

/**
 * Dictates fall speed of all transported units.
 * This only makes sense for air transports,
 * if they an drop units while in the air.
 */
EXPORT(float) bridged_UnitDef_getFallSpeed(int skirmishAIId, int unitDefId);

/**
 * Sets the transported units FBI, overrides fallSpeed
 */
EXPORT(float) bridged_UnitDef_getUnitFallSpeed(int skirmishAIId, int unitDefId);

/**
 * If the unit can cloak
 */
EXPORT(bool) bridged_UnitDef_isAbleToCloak(int skirmishAIId, int unitDefId);

/**
 * If the unit wants to start out cloaked
 */
EXPORT(bool) bridged_UnitDef_isStartCloaked(int skirmishAIId, int unitDefId);

/**
 * Energy cost per second to stay cloaked when stationary
 */
EXPORT(float) bridged_UnitDef_getCloakCost(int skirmishAIId, int unitDefId);

/**
 * Energy cost per second to stay cloaked when moving
 */
EXPORT(float) bridged_UnitDef_getCloakCostMoving(int skirmishAIId, int unitDefId);

/**
 * If enemy unit comes within this range, decloaking is forced
 */
EXPORT(float) bridged_UnitDef_getDecloakDistance(int skirmishAIId, int unitDefId);

/**
 * Use a spherical, instead of a cylindrical test?
 */
EXPORT(bool) bridged_UnitDef_isDecloakSpherical(int skirmishAIId, int unitDefId);

/**
 * Will the unit decloak upon firing?
 */
EXPORT(bool) bridged_UnitDef_isDecloakOnFire(int skirmishAIId, int unitDefId);

/**
 * Will the unit self destruct if an enemy comes to close?
 */
EXPORT(bool) bridged_UnitDef_isAbleToKamikaze(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getKamikazeDist(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isTargetingFacility(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_canManualFire(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isNeedGeo(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isFeature(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isHideDamage(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isCommander(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isShowPlayerName(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToResurrect(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToCapture(int skirmishAIId, int unitDefId);

/**
 * Indicates the trajectory types supported by this unit.
 * 
 * @return  0: (default) = only low
 *          1: only high
 *          2: choose
 */
EXPORT(int) bridged_UnitDef_getHighTrajectoryType(int skirmishAIId, int unitDefId);

/**
 * Returns the bit field value denoting the categories this unit shall not
 * chase.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(int) bridged_UnitDef_getNoChaseCategory(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isLeaveTracks(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTrackWidth(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTrackOffset(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTrackStrength(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getTrackStretch(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getTrackType(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isAbleToDropFlare(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getFlareReloadTime(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getFlareEfficiency(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getFlareDelay(int skirmishAIId, int unitDefId);

EXPORT(void) bridged_UnitDef_getFlareDropVector(int skirmishAIId, int unitDefId, float* return_posF3_out);

EXPORT(int) bridged_UnitDef_getFlareTime(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getFlareSalvoSize(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getFlareSalvoDelay(int skirmishAIId, int unitDefId);

/**
 * Only matters for fighter aircraft
 */
EXPORT(bool) bridged_UnitDef_isAbleToLoopbackAttack(int skirmishAIId, int unitDefId);

/**
 * Indicates whether the ground will be leveled/flattened out
 * after this building has been built on it.
 * Only matters for buildings.
 */
EXPORT(bool) bridged_UnitDef_isLevelGround(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_isUseBuildingGroundDecal(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getBuildingDecalType(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getBuildingDecalSizeX(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getBuildingDecalSizeY(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_getBuildingDecalDecaySpeed(int skirmishAIId, int unitDefId);

/**
 * Maximum flight time in seconds before the aircraft needs
 * to return to an air repair bay to refuel.
 */
EXPORT(float) bridged_UnitDef_getMaxFuel(int skirmishAIId, int unitDefId);

/**
 * Time to fully refuel the unit
 */
EXPORT(float) bridged_UnitDef_getRefuelTime(int skirmishAIId, int unitDefId);

/**
 * Minimum build power of airbases that this aircraft can land on
 */
EXPORT(float) bridged_UnitDef_getMinAirBasePower(int skirmishAIId, int unitDefId);

/**
 * Number of units of this type allowed simultaneously in the game
 */
EXPORT(int) bridged_UnitDef_getMaxThisUnit(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getDecoyDef(int skirmishAIId, int unitDefId); // REF:RETURN->UnitDef

EXPORT(bool) bridged_UnitDef_isDontLand(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getShieldDef(int skirmishAIId, int unitDefId); // REF:RETURN->WeaponDef

EXPORT(int) bridged_UnitDef_getStockpileDef(int skirmishAIId, int unitDefId); // REF:RETURN->WeaponDef

EXPORT(int) bridged_UnitDef_getBuildOptions(int skirmishAIId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax); // REF:MULTI:unitDefIds->UnitDef

EXPORT(int) bridged_UnitDef_getCustomParams(int skirmishAIId, int unitDefId, const char** keys, const char** values); // MAP

EXPORT(bool) bridged_UnitDef_isMoveDataAvailable(int skirmishAIId, int unitDefId); // AVAILABLE:MoveData

/**
 * @deprecated
 */
EXPORT(float) bridged_UnitDef_MoveData_getMaxAcceleration(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(float) bridged_UnitDef_MoveData_getMaxBreaking(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(float) bridged_UnitDef_MoveData_getMaxSpeed(int skirmishAIId, int unitDefId);

/**
 * @deprecated
 */
EXPORT(short) bridged_UnitDef_MoveData_getMaxTurnRate(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_MoveData_getXSize(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_MoveData_getZSize(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_MoveData_getDepth(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_MoveData_getMaxSlope(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_MoveData_getSlopeMod(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_MoveData_getDepthMod(int skirmishAIId, int unitDefId, float height);

EXPORT(int) bridged_UnitDef_MoveData_getPathType(int skirmishAIId, int unitDefId);

EXPORT(float) bridged_UnitDef_MoveData_getCrushStrength(int skirmishAIId, int unitDefId);

/**
 * enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 };
 */
EXPORT(int) bridged_UnitDef_MoveData_getMoveType(int skirmishAIId, int unitDefId);

/**
 * enum SpeedModClass { Tank=0, KBot=1, Hover=2, Ship=3 };
 */
EXPORT(int) bridged_UnitDef_MoveData_getSpeedModClass(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_MoveData_getTerrainClass(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_MoveData_getFollowGround(int skirmishAIId, int unitDefId);

EXPORT(bool) bridged_UnitDef_MoveData_isSubMarine(int skirmishAIId, int unitDefId);

EXPORT(const char*) bridged_UnitDef_MoveData_getName(int skirmishAIId, int unitDefId);

EXPORT(int) bridged_UnitDef_getWeaponMounts(int skirmishAIId, int unitDefId); // FETCHER:MULTI:NUM:WeaponMount

EXPORT(const char*) bridged_UnitDef_WeaponMount_getName(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(int) bridged_UnitDef_WeaponMount_getWeaponDef(int skirmishAIId, int unitDefId, int weaponMountId); // REF:RETURN->WeaponDef

EXPORT(int) bridged_UnitDef_WeaponMount_getSlavedTo(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(void) bridged_UnitDef_WeaponMount_getMainDir(int skirmishAIId, int unitDefId, int weaponMountId, float* return_posF3_out);

EXPORT(float) bridged_UnitDef_WeaponMount_getMaxAngleDif(int skirmishAIId, int unitDefId, int weaponMountId);

/**
 * How many seconds of fuel it costs for the owning unit to fire this weapon.
 */
EXPORT(float) bridged_UnitDef_WeaponMount_getFuelUsage(int skirmishAIId, int unitDefId, int weaponMountId);

/**
 * Returns the bit field value denoting the categories this weapon should
 * not target.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(int) bridged_UnitDef_WeaponMount_getBadTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId);

/**
 * Returns the bit field value denoting the categories this weapon should
 * target, excluding all others.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(int) bridged_UnitDef_WeaponMount_getOnlyTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId);

/**
 * Returns the number of units a team can have, after which it can not build
 * any more. It is possible that a team has more units then this value at
 * some point in the game. This is possible for example with taking,
 * reclaiming or capturing units.
 * This value is usefull for controlling game performance, and will
 * therefore often be set on games with old hardware to prevent lagging
 * because of too many units.
 */
EXPORT(int) bridged_Unit_getLimit(int skirmishAIId); // STATIC

/**
 * Returns the maximum total number of units that may exist at any one point
 * in time induring the current game.
 */
EXPORT(int) bridged_Unit_getMax(int skirmishAIId); // STATIC

/**
 * Returns all units that are not in this teams ally-team nor neutral
 * and are in LOS.
 * If cheats are enabled, this will return all enemies on the map.
 */
EXPORT(int) bridged_getEnemyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are not in this teams ally-team nor neutral
 * and are in LOS plus they have to be located in the specified area
 * of the map.
 * If cheats are enabled, this will return all enemies
 * in the specified area.
 */
EXPORT(int) bridged_getEnemyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are not in this teams ally-team nor neutral
 * and are in under sensor coverage (sight or radar).
 * If cheats are enabled, this will return all enemies on the map.
 */
EXPORT(int) bridged_getEnemyUnitsInRadarAndLos(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are in this teams ally-team, including this teams
 * units.
 */
EXPORT(int) bridged_getFriendlyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are in this teams ally-team, including this teams
 * units plus they have to be located in the specified area of the map.
 */
EXPORT(int) bridged_getFriendlyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are neutral and are in LOS.
 */
EXPORT(int) bridged_getNeutralUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are neutral and are in LOS plus they have to be
 * located in the specified area of the map.
 */
EXPORT(int) bridged_getNeutralUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are of the team controlled by this AI instance. This
 * list can also be created dynamically by the AI, through updating a list on
 * each unit-created and unit-destroyed event.
 */
EXPORT(int) bridged_getTeamUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns all units that are currently selected
 * (usually only contains units if a human player
 * is controlling this team as well).
 */
EXPORT(int) bridged_getSelectedUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax); // FETCHER:MULTI:IDs:Unit:unitIds

/**
 * Returns the unit's unitdef struct from which you can read all
 * the statistics of the unit, do NOT try to change any values in it.
 */
EXPORT(int) bridged_Unit_getDef(int skirmishAIId, int unitId); // REF:RETURN->UnitDef

/**
 * This is a set of parameters that is created by SetUnitRulesParam() and may change during the game.
 * Each parameter is uniquely identified only by its id (which is the index in the vector).
 * Parameters may or may not have a name.
 * @return visible to skirmishAIId parameters.
 * If cheats are enabled, this will return all parameters.
 */
EXPORT(int) bridged_Unit_getUnitRulesParams(int skirmishAIId, int unitId, int* paramIds, int paramIds_sizeMax); // FETCHER:MULTI:IDs:UnitRulesParam:paramIds

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Unit_getUnitRulesParamByName(int skirmishAIId, int unitId, const char* rulesParamName); // REF:RETURN->UnitRulesParam

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Unit_getUnitRulesParamById(int skirmishAIId, int unitId, int rulesParamId); // REF:RETURN->UnitRulesParam

/**
 * Not every mod parameter has a name.
 */
EXPORT(const char*) bridged_Unit_UnitRulesParam_getName(int skirmishAIId, int unitId, int unitRulesParamId);

/**
 * @return float value of parameter if it's set, 0.0 otherwise.
 */
EXPORT(float) bridged_Unit_UnitRulesParam_getValueFloat(int skirmishAIId, int unitId, int unitRulesParamId);

/**
 * @return string value of parameter if it's set, empty string otherwise.
 */
EXPORT(const char*) bridged_Unit_UnitRulesParam_getValueString(int skirmishAIId, int unitId, int unitRulesParamId);

EXPORT(int) bridged_Unit_getTeam(int skirmishAIId, int unitId);

EXPORT(int) bridged_Unit_getAllyTeam(int skirmishAIId, int unitId);

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
 * @deprecated
 */
EXPORT(int) bridged_Unit_getAiHint(int skirmishAIId, int unitId);

EXPORT(int) bridged_Unit_getStockpile(int skirmishAIId, int unitId);

EXPORT(int) bridged_Unit_getStockpileQueued(int skirmishAIId, int unitId);

EXPORT(float) bridged_Unit_getCurrentFuel(int skirmishAIId, int unitId);

/**
 * The unit's max speed
 */
EXPORT(float) bridged_Unit_getMaxSpeed(int skirmishAIId, int unitId);

/**
 * The furthest any weapon of the unit can fire
 */
EXPORT(float) bridged_Unit_getMaxRange(int skirmishAIId, int unitId);

/**
 * The unit's max health
 */
EXPORT(float) bridged_Unit_getMaxHealth(int skirmishAIId, int unitId);

/**
 * How experienced the unit is (0.0f - 1.0f)
 */
EXPORT(float) bridged_Unit_getExperience(int skirmishAIId, int unitId);

/**
 * Returns the group a unit belongs to, -1 if none
 */
EXPORT(int) bridged_Unit_getGroup(int skirmishAIId, int unitId);

EXPORT(int) bridged_Unit_getCurrentCommands(int skirmishAIId, int unitId); // FETCHER:MULTI:NUM:CurrentCommand-Command

/**
 * For the type of the command queue, see CCommandQueue::CommandQueueType
 * in Sim/Unit/CommandAI/CommandQueue.h
 */
EXPORT(int) bridged_Unit_CurrentCommand_getType(int skirmishAIId, int unitId); // STATIC

/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
EXPORT(int) bridged_Unit_CurrentCommand_getId(int skirmishAIId, int unitId, int commandId);

EXPORT(short) bridged_Unit_CurrentCommand_getOptions(int skirmishAIId, int unitId, int commandId);

EXPORT(int) bridged_Unit_CurrentCommand_getTag(int skirmishAIId, int unitId, int commandId);

EXPORT(int) bridged_Unit_CurrentCommand_getTimeOut(int skirmishAIId, int unitId, int commandId);

EXPORT(int) bridged_Unit_CurrentCommand_getParams(int skirmishAIId, int unitId, int commandId, float* params, int params_sizeMax); // ARRAY:params

/**
 * The commands that this unit can understand, other commands will be ignored
 */
EXPORT(int) bridged_Unit_getSupportedCommands(int skirmishAIId, int unitId); // FETCHER:MULTI:NUM:SupportedCommand-CommandDescription

/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
EXPORT(int) bridged_Unit_SupportedCommand_getId(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(const char*) bridged_Unit_SupportedCommand_getName(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(const char*) bridged_Unit_SupportedCommand_getToolTip(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(bool) bridged_Unit_SupportedCommand_isShowUnique(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(bool) bridged_Unit_SupportedCommand_isDisabled(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(int) bridged_Unit_SupportedCommand_getParams(int skirmishAIId, int unitId, int supportedCommandId, const char** params, int params_sizeMax); // ARRAY:params

/**
 * The unit's current health
 */
EXPORT(float) bridged_Unit_getHealth(int skirmishAIId, int unitId);

EXPORT(float) bridged_Unit_getSpeed(int skirmishAIId, int unitId);

/**
 * Indicate the relative power of the unit,
 * used for experience calulations etc.
 * This is sort of the measure of the units overall power.
 */
EXPORT(float) bridged_Unit_getPower(int skirmishAIId, int unitId);

EXPORT(float) bridged_Unit_getResourceUse(int skirmishAIId, int unitId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Unit_getResourceMake(int skirmishAIId, int unitId, int resourceId); // REF:resourceId->Resource

EXPORT(void) bridged_Unit_getPos(int skirmishAIId, int unitId, float* return_posF3_out);

EXPORT(void) bridged_Unit_getVel(int skirmishAIId, int unitId, float* return_posF3_out);

EXPORT(bool) bridged_Unit_isActivated(int skirmishAIId, int unitId);

/**
 * Returns true if the unit is currently being built
 */
EXPORT(bool) bridged_Unit_isBeingBuilt(int skirmishAIId, int unitId);

EXPORT(bool) bridged_Unit_isCloaked(int skirmishAIId, int unitId);

EXPORT(bool) bridged_Unit_isParalyzed(int skirmishAIId, int unitId);

EXPORT(bool) bridged_Unit_isNeutral(int skirmishAIId, int unitId);

/**
 * Returns the unit's build facing (0-3)
 */
EXPORT(int) bridged_Unit_getBuildingFacing(int skirmishAIId, int unitId);

/**
 * Number of the last frame this unit received an order from a player.
 */
EXPORT(int) bridged_Unit_getLastUserOrderFrame(int skirmishAIId, int unitId);

EXPORT(bool) bridged_Team_hasAIController(int skirmishAIId, int teamId);

EXPORT(int) bridged_getEnemyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax); // FETCHER:MULTI:IDs:Team:teamIds

EXPORT(int) bridged_getAllyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax); // FETCHER:MULTI:IDs:Team:teamIds

/**
 * This is a set of parameters that is created by SetTeamRulesParam() and may change during the game.
 * Each parameter is uniquely identified only by its id (which is the index in the vector).
 * Parameters may or may not have a name.
 * @return visible to skirmishAIId parameters.
 * If cheats are enabled, this will return all parameters.
 */
EXPORT(int) bridged_Team_getTeamRulesParams(int skirmishAIId, int teamId, int* paramIds, int paramIds_sizeMax); // FETCHER:MULTI:IDs:TeamRulesParam:paramIds

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Team_getTeamRulesParamByName(int skirmishAIId, int teamId, const char* rulesParamName); // REF:RETURN->TeamRulesParam

/**
 * @return only visible to skirmishAIId parameter.
 * If cheats are enabled, this will return parameter despite it's losStatus.
 */
EXPORT(int) bridged_Team_getTeamRulesParamById(int skirmishAIId, int teamId, int rulesParamId); // REF:RETURN->TeamRulesParam

/**
 * Not every mod parameter has a name.
 */
EXPORT(const char*) bridged_Team_TeamRulesParam_getName(int skirmishAIId, int teamId, int teamRulesParamId);

/**
 * @return float value of parameter if it's set, 0.0 otherwise.
 */
EXPORT(float) bridged_Team_TeamRulesParam_getValueFloat(int skirmishAIId, int teamId, int teamRulesParamId);

/**
 * @return string value of parameter if it's set, empty string otherwise.
 */
EXPORT(const char*) bridged_Team_TeamRulesParam_getValueString(int skirmishAIId, int teamId, int teamRulesParamId);

EXPORT(int) bridged_getGroups(int skirmishAIId, int* groupIds, int groupIds_sizeMax); // FETCHER:MULTI:IDs:Group:groupIds

EXPORT(int) bridged_Group_getSupportedCommands(int skirmishAIId, int groupId); // FETCHER:MULTI:NUM:SupportedCommand-CommandDescription

/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
EXPORT(int) bridged_Group_SupportedCommand_getId(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(const char*) bridged_Group_SupportedCommand_getName(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(const char*) bridged_Group_SupportedCommand_getToolTip(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(bool) bridged_Group_SupportedCommand_isShowUnique(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(bool) bridged_Group_SupportedCommand_isDisabled(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(int) bridged_Group_SupportedCommand_getParams(int skirmishAIId, int groupId, int supportedCommandId, const char** params, int params_sizeMax); // ARRAY:params

/**
 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
 * (custom codes can also be used)
 */
EXPORT(int) bridged_Group_OrderPreview_getId(int skirmishAIId, int groupId);

EXPORT(short) bridged_Group_OrderPreview_getOptions(int skirmishAIId, int groupId);

EXPORT(int) bridged_Group_OrderPreview_getTag(int skirmishAIId, int groupId);

EXPORT(int) bridged_Group_OrderPreview_getTimeOut(int skirmishAIId, int groupId);

EXPORT(int) bridged_Group_OrderPreview_getParams(int skirmishAIId, int groupId, float* params, int params_sizeMax); // ARRAY:params

EXPORT(bool) bridged_Group_isSelected(int skirmishAIId, int groupId);

/**
 * Returns the mod archive file name.
 * CAUTION:
 * Never use this as reference in eg. cache- or config-file names,
 * as one and the same mod can be packaged in different ways.
 * Use the human name instead.
 * @see getHumanName()
 * @deprecated
 */
EXPORT(const char*) bridged_Mod_getFileName(int skirmishAIId);

/**
 * Returns the archive hash of the mod.
 * Use this for reference to the mod, eg. in a cache-file, wherever human
 * readability does not matter.
 * This value will never be the same for two mods not having equal content.
 * Tip: convert to 64 Hex chars for use in file names.
 * @see getHumanName()
 */
EXPORT(int) bridged_Mod_getHash(int skirmishAIId);

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
EXPORT(const char*) bridged_Mod_getHumanName(int skirmishAIId);

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
EXPORT(const char*) bridged_Mod_getShortName(int skirmishAIId);

EXPORT(const char*) bridged_Mod_getVersion(int skirmishAIId);

EXPORT(const char*) bridged_Mod_getMutator(int skirmishAIId);

EXPORT(const char*) bridged_Mod_getDescription(int skirmishAIId);

EXPORT(bool) bridged_Mod_getAllowTeamColors(int skirmishAIId);

/**
 * Should constructions without builders decay?
 */
EXPORT(bool) bridged_Mod_getConstructionDecay(int skirmishAIId);

/**
 * How long until they start decaying?
 */
EXPORT(int) bridged_Mod_getConstructionDecayTime(int skirmishAIId);

/**
 * How fast do they decay?
 */
EXPORT(float) bridged_Mod_getConstructionDecaySpeed(int skirmishAIId);

/**
 * 0 = 1 reclaimer per feature max, otherwise unlimited
 */
EXPORT(int) bridged_Mod_getMultiReclaim(int skirmishAIId);

/**
 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
 */
EXPORT(int) bridged_Mod_getReclaimMethod(int skirmishAIId);

/**
 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
 */
EXPORT(int) bridged_Mod_getReclaimUnitMethod(int skirmishAIId);

/**
 * How much energy should reclaiming a unit cost, default 0.0
 */
EXPORT(float) bridged_Mod_getReclaimUnitEnergyCostFactor(int skirmishAIId);

/**
 * How much metal should reclaim return, default 1.0
 */
EXPORT(float) bridged_Mod_getReclaimUnitEfficiency(int skirmishAIId);

/**
 * How much should energy should reclaiming a feature cost, default 0.0
 */
EXPORT(float) bridged_Mod_getReclaimFeatureEnergyCostFactor(int skirmishAIId);

/**
 * Allow reclaiming enemies? default true
 */
EXPORT(bool) bridged_Mod_getReclaimAllowEnemies(int skirmishAIId);

/**
 * Allow reclaiming allies? default true
 */
EXPORT(bool) bridged_Mod_getReclaimAllowAllies(int skirmishAIId);

/**
 * How much should energy should repair cost, default 0.0
 */
EXPORT(float) bridged_Mod_getRepairEnergyCostFactor(int skirmishAIId);

/**
 * How much should energy should resurrect cost, default 0.5
 */
EXPORT(float) bridged_Mod_getResurrectEnergyCostFactor(int skirmishAIId);

/**
 * How much should energy should capture cost, default 0.0
 */
EXPORT(float) bridged_Mod_getCaptureEnergyCostFactor(int skirmishAIId);

/**
 * 0 = all ground units cannot be transported,
 * 1 = all ground units can be transported
 * (mass and size restrictions still apply).
 * Defaults to 1.
 */
EXPORT(int) bridged_Mod_getTransportGround(int skirmishAIId);

/**
 * 0 = all hover units cannot be transported,
 * 1 = all hover units can be transported
 * (mass and size restrictions still apply).
 * Defaults to 0.
 */
EXPORT(int) bridged_Mod_getTransportHover(int skirmishAIId);

/**
 * 0 = all naval units cannot be transported,
 * 1 = all naval units can be transported
 * (mass and size restrictions still apply).
 * Defaults to 0.
 */
EXPORT(int) bridged_Mod_getTransportShip(int skirmishAIId);

/**
 * 0 = all air units cannot be transported,
 * 1 = all air units can be transported
 * (mass and size restrictions still apply).
 * Defaults to 0.
 */
EXPORT(int) bridged_Mod_getTransportAir(int skirmishAIId);

/**
 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
 */
EXPORT(int) bridged_Mod_getFireAtKilled(int skirmishAIId);

/**
 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
 */
EXPORT(int) bridged_Mod_getFireAtCrashing(int skirmishAIId);

/**
 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
 */
EXPORT(int) bridged_Mod_getFlankingBonusModeDefault(int skirmishAIId);

/**
 * miplevel for los
 */
EXPORT(int) bridged_Mod_getLosMipLevel(int skirmishAIId);

/**
 * miplevel to use for airlos
 */
EXPORT(int) bridged_Mod_getAirMipLevel(int skirmishAIId);

/**
 * units sightdistance will be multiplied with this, for testing purposes
 */
EXPORT(float) bridged_Mod_getLosMul(int skirmishAIId);

/**
 * units airsightdistance will be multiplied with this, for testing purposes
 */
EXPORT(float) bridged_Mod_getAirLosMul(int skirmishAIId);

/**
 * when underwater, units are not in LOS unless also in sonar
 */
EXPORT(bool) bridged_Mod_getRequireSonarUnderWater(int skirmishAIId);

EXPORT(int) bridged_Map_getChecksum(int skirmishAIId);

EXPORT(void) bridged_Map_getStartPos(int skirmishAIId, float* return_posF3_out);

EXPORT(void) bridged_Map_getMousePos(int skirmishAIId, float* return_posF3_out);

EXPORT(bool) bridged_Map_isPosInCamera(int skirmishAIId, float* pos_posF3, float radius);

/**
 * Returns the maps center heightmap width.
 * @see getHeightMap()
 */
EXPORT(int) bridged_Map_getWidth(int skirmishAIId);

/**
 * Returns the maps center heightmap height.
 * @see getHeightMap()
 */
EXPORT(int) bridged_Map_getHeight(int skirmishAIId);

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
EXPORT(int) bridged_Map_getHeightMap(int skirmishAIId, float* heights, int heights_sizeMax); // ARRAY:heights

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
EXPORT(int) bridged_Map_getCornersHeightMap(int skirmishAIId, float* cornerHeights, int cornerHeights_sizeMax); // ARRAY:cornerHeights

EXPORT(float) bridged_Map_getMinHeight(int skirmishAIId);

EXPORT(float) bridged_Map_getMaxHeight(int skirmishAIId);

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
EXPORT(int) bridged_Map_getSlopeMap(int skirmishAIId, float* slopes, int slopes_sizeMax); // ARRAY:slopes

/**
 * @brief the level of sight map
 * mapDims.mapx >> losMipLevel
 * A square with value zero means you do not have LOS coverage on it.
 * Mod_getLosMipLevel
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
EXPORT(int) bridged_Map_getLosMap(int skirmishAIId, int* losValues, int losValues_sizeMax); // ARRAY:losValues

/**
 * @brief the radar map
 * A square with value 0 means you do not have radar coverage on it.
 * 
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 8*8 in size
 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 8)
 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
 */
EXPORT(int) bridged_Map_getRadarMap(int skirmishAIId, int* radarValues, int radarValues_sizeMax); // ARRAY:radarValues

/**
 * @brief the radar jammer map
 * A square with value 0 means you do not have radar jamming coverage.
 * 
 * - do NOT modify or delete the height-map (native code relevant only)
 * - index 0 is top left
 * - each data position is 8*8 in size
 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 8)
 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
 */
EXPORT(int) bridged_Map_getJammerMap(int skirmishAIId, int* jammerValues, int jammerValues_sizeMax); // ARRAY:jammerValues

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
EXPORT(int) bridged_Map_getResourceMapRaw(int skirmishAIId, int resourceId, short* resources, int resources_sizeMax); // REF:resourceId->Resource ARRAY:resources

/**
 * Returns positions indicating where to place resource extractors on the map.
 * Only the x and z values give the location of the spots, while the y values
 * represents the actual amount of resource an extractor placed there can make.
 * You should only compare the y values to each other, and not try to estimate
 * effective output from spots.
 */
EXPORT(int) bridged_Map_getResourceMapSpotsPositions(int skirmishAIId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax); // REF:resourceId->Resource ARRAY:spots_AposF3

/**
 * Returns the average resource income for an extractor on one of the evaluated positions.
 */
EXPORT(float) bridged_Map_getResourceMapSpotsAverageIncome(int skirmishAIId, int resourceId); // REF:resourceId->Resource

/**
 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
 */
EXPORT(void) bridged_Map_getResourceMapSpotsNearest(int skirmishAIId, int resourceId, float* pos_posF3, float* return_posF3_out); // REF:resourceId->Resource

/**
 * Returns the archive hash of the map.
 * Use this for reference to the map, eg. in a cache-file, wherever human
 * readability does not matter.
 * This value will never be the same for two maps not having equal content.
 * Tip: convert to 64 Hex chars for use in file names.
 * @see getName()
 */
EXPORT(int) bridged_Map_getHash(int skirmishAIId);

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
EXPORT(const char*) bridged_Map_getName(int skirmishAIId);

/**
 * Returns the human readbale name of the map.
 * @see getName()
 */
EXPORT(const char*) bridged_Map_getHumanName(int skirmishAIId);

/**
 * Gets the elevation of the map at position (x, z)
 */
EXPORT(float) bridged_Map_getElevationAt(int skirmishAIId, float x, float z);

/**
 * Returns what value 255 in the resource map is worth
 */
EXPORT(float) bridged_Map_getMaxResource(int skirmishAIId, int resourceId); // REF:resourceId->Resource

/**
 * Returns extraction radius for resource extractors
 */
EXPORT(float) bridged_Map_getExtractorRadius(int skirmishAIId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_Map_getMinWind(int skirmishAIId);

EXPORT(float) bridged_Map_getMaxWind(int skirmishAIId);

EXPORT(float) bridged_Map_getCurWind(int skirmishAIId);

EXPORT(float) bridged_Map_getTidalStrength(int skirmishAIId);

EXPORT(float) bridged_Map_getGravity(int skirmishAIId);

EXPORT(float) bridged_Map_getWaterDamage(int skirmishAIId);

/**
 * Returns all points drawn with this AIs team color,
 * and additionally the ones drawn with allied team colors,
 * if <code>includeAllies</code> is true.
 */
EXPORT(int) bridged_Map_getPoints(int skirmishAIId, bool includeAllies); // FETCHER:MULTI:NUM:Point

EXPORT(void) bridged_Map_Point_getPosition(int skirmishAIId, int pointId, float* return_posF3_out);

EXPORT(void) bridged_Map_Point_getColor(int skirmishAIId, int pointId, short* return_colorS3_out);

EXPORT(const char*) bridged_Map_Point_getLabel(int skirmishAIId, int pointId);

/**
 * Returns all lines drawn with this AIs team color,
 * and additionally the ones drawn with allied team colors,
 * if <code>includeAllies</code> is true.
 */
EXPORT(int) bridged_Map_getLines(int skirmishAIId, bool includeAllies); // FETCHER:MULTI:NUM:Line

EXPORT(void) bridged_Map_Line_getFirstPosition(int skirmishAIId, int lineId, float* return_posF3_out);

EXPORT(void) bridged_Map_Line_getSecondPosition(int skirmishAIId, int lineId, float* return_posF3_out);

EXPORT(void) bridged_Map_Line_getColor(int skirmishAIId, int lineId, short* return_colorS3_out);

EXPORT(bool) bridged_Map_isPossibleToBuildAt(int skirmishAIId, int unitDefId, float* pos_posF3, int facing); // REF:unitDefId->UnitDef

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
EXPORT(void) bridged_Map_findClosestBuildSite(int skirmishAIId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out); // REF:unitDefId->UnitDef

EXPORT(int) bridged_getFeatureDefs(int skirmishAIId, int* featureDefIds, int featureDefIds_sizeMax); // FETCHER:MULTI:IDs:FeatureDef:featureDefIds

EXPORT(const char*) bridged_FeatureDef_getName(int skirmishAIId, int featureDefId);

EXPORT(const char*) bridged_FeatureDef_getDescription(int skirmishAIId, int featureDefId);

EXPORT(const char*) bridged_FeatureDef_getFileName(int skirmishAIId, int featureDefId);

EXPORT(float) bridged_FeatureDef_getContainedResource(int skirmishAIId, int featureDefId, int resourceId); // REF:resourceId->Resource

EXPORT(float) bridged_FeatureDef_getMaxHealth(int skirmishAIId, int featureDefId);

EXPORT(float) bridged_FeatureDef_getReclaimTime(int skirmishAIId, int featureDefId);

/**
 * Used to see if the object can be overrun by units of a certain heavyness
 */
EXPORT(float) bridged_FeatureDef_getMass(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isUpright(int skirmishAIId, int featureDefId);

EXPORT(int) bridged_FeatureDef_getDrawType(int skirmishAIId, int featureDefId);

EXPORT(const char*) bridged_FeatureDef_getModelName(int skirmishAIId, int featureDefId);

/**
 * Used to determine whether the feature is resurrectable.
 * 
 * @return  -1: (default) only if it is the 1st wreckage of
 *              the UnitDef it originates from
 *           0: no, never
 *           1: yes, always
 */
EXPORT(int) bridged_FeatureDef_getResurrectable(int skirmishAIId, int featureDefId);

EXPORT(int) bridged_FeatureDef_getSmokeTime(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isDestructable(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isReclaimable(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isBlocking(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isBurnable(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isFloating(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isNoSelect(int skirmishAIId, int featureDefId);

EXPORT(bool) bridged_FeatureDef_isGeoThermal(int skirmishAIId, int featureDefId);

/**
 * Name of the FeatureDef that this turns into when killed (not reclaimed).
 */
EXPORT(const char*) bridged_FeatureDef_getDeathFeature(int skirmishAIId, int featureDefId);

/**
 * Size of the feature along the X axis - in other words: height.
 * each size is 8 units
 */
EXPORT(int) bridged_FeatureDef_getXSize(int skirmishAIId, int featureDefId);

/**
 * Size of the feature along the Z axis - in other words: width.
 * each size is 8 units
 */
EXPORT(int) bridged_FeatureDef_getZSize(int skirmishAIId, int featureDefId);

EXPORT(int) bridged_FeatureDef_getCustomParams(int skirmishAIId, int featureDefId, const char** keys, const char** values); // MAP

/**
 * Returns all features currently in LOS, or all features on the map
 * if cheating is enabled.
 */
EXPORT(int) bridged_getFeatures(int skirmishAIId, int* featureIds, int featureIds_sizeMax); // REF:MULTI:featureIds->Feature

/**
 * Returns all features in a specified area that are currently in LOS,
 * or all features in this area if cheating is enabled.
 */
EXPORT(int) bridged_getFeaturesIn(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax); // REF:MULTI:featureIds->Feature

EXPORT(int) bridged_Feature_getDef(int skirmishAIId, int featureId); // REF:RETURN->FeatureDef

EXPORT(float) bridged_Feature_getHealth(int skirmishAIId, int featureId);

EXPORT(float) bridged_Feature_getReclaimLeft(int skirmishAIId, int featureId);

EXPORT(void) bridged_Feature_getPosition(int skirmishAIId, int featureId, float* return_posF3_out);

EXPORT(int) bridged_getWeaponDefs(int skirmishAIId); // FETCHER:MULTI:NUM:WeaponDef

EXPORT(int) bridged_getWeaponDefByName(int skirmishAIId, const char* weaponDefName); // REF:RETURN->WeaponDef

EXPORT(const char*) bridged_WeaponDef_getName(int skirmishAIId, int weaponDefId);

EXPORT(const char*) bridged_WeaponDef_getType(int skirmishAIId, int weaponDefId);

EXPORT(const char*) bridged_WeaponDef_getDescription(int skirmishAIId, int weaponDefId);

EXPORT(const char*) bridged_WeaponDef_getFileName(int skirmishAIId, int weaponDefId);

EXPORT(const char*) bridged_WeaponDef_getCegTag(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getRange(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getHeightMod(int skirmishAIId, int weaponDefId);

/**
 * Inaccuracy of whole burst
 */
EXPORT(float) bridged_WeaponDef_getAccuracy(int skirmishAIId, int weaponDefId);

/**
 * Inaccuracy of individual shots inside burst
 */
EXPORT(float) bridged_WeaponDef_getSprayAngle(int skirmishAIId, int weaponDefId);

/**
 * Inaccuracy while owner moving
 */
EXPORT(float) bridged_WeaponDef_getMovingAccuracy(int skirmishAIId, int weaponDefId);

/**
 * Fraction of targets move speed that is used as error offset
 */
EXPORT(float) bridged_WeaponDef_getTargetMoveError(int skirmishAIId, int weaponDefId);

/**
 * Maximum distance the weapon will lead the target
 */
EXPORT(float) bridged_WeaponDef_getLeadLimit(int skirmishAIId, int weaponDefId);

/**
 * Factor for increasing the leadLimit with experience
 */
EXPORT(float) bridged_WeaponDef_getLeadBonus(int skirmishAIId, int weaponDefId);

/**
 * Replaces hardcoded behaviour for burnblow cannons
 */
EXPORT(float) bridged_WeaponDef_getPredictBoost(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getNumDamageTypes(int skirmishAIId); // STATIC

EXPORT(int) bridged_WeaponDef_Damage_getParalyzeDamageTime(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_Damage_getImpulseFactor(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_Damage_getImpulseBoost(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_Damage_getCraterMult(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_Damage_getCraterBoost(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_Damage_getTypes(int skirmishAIId, int weaponDefId, float* types, int types_sizeMax); // ARRAY:types

EXPORT(float) bridged_WeaponDef_getAreaOfEffect(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isNoSelfDamage(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getFireStarter(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getEdgeEffectiveness(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getSize(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getSizeGrowth(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getCollisionSize(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getSalvoSize(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getSalvoDelay(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getReload(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getBeamTime(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isBeamBurst(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isWaterBounce(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isGroundBounce(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getBounceRebound(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getBounceSlip(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getNumBounce(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getMaxAngle(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getUpTime(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getFlightTime(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getCost(int skirmishAIId, int weaponDefId, int resourceId); // REF:resourceId->Resource

EXPORT(int) bridged_WeaponDef_getProjectilesPerShot(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isTurret(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isOnlyForward(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isFixedLauncher(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isWaterWeapon(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isFireSubmersed(int skirmishAIId, int weaponDefId);

/**
 * Lets a torpedo travel above water like it does below water
 */
EXPORT(bool) bridged_WeaponDef_isSubMissile(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isTracks(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isDropped(int skirmishAIId, int weaponDefId);

/**
 * The weapon will only paralyze, not do real damage.
 */
EXPORT(bool) bridged_WeaponDef_isParalyzer(int skirmishAIId, int weaponDefId);

/**
 * The weapon damages by impacting, not by exploding.
 */
EXPORT(bool) bridged_WeaponDef_isImpactOnly(int skirmishAIId, int weaponDefId);

/**
 * Can not target anything (for example: anti-nuke, D-Gun)
 */
EXPORT(bool) bridged_WeaponDef_isNoAutoTarget(int skirmishAIId, int weaponDefId);

/**
 * Has to be fired manually (by the player or an AI, example: D-Gun)
 */
EXPORT(bool) bridged_WeaponDef_isManualFire(int skirmishAIId, int weaponDefId);

/**
 * Can intercept targetable weapons shots.
 * 
 * example: anti-nuke
 * 
 * @see  getTargetable()
 */
EXPORT(int) bridged_WeaponDef_getInterceptor(int skirmishAIId, int weaponDefId);

/**
 * Shoots interceptable projectiles.
 * Shots can be intercepted by interceptors.
 * 
 * example: nuke
 * 
 * @see  getInterceptor()
 */
EXPORT(int) bridged_WeaponDef_getTargetable(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isStockpileable(int skirmishAIId, int weaponDefId);

/**
 * Range of interceptors.
 * 
 * example: anti-nuke
 * 
 * @see  getInterceptor()
 */
EXPORT(float) bridged_WeaponDef_getCoverageRange(int skirmishAIId, int weaponDefId);

/**
 * Build time of a missile
 */
EXPORT(float) bridged_WeaponDef_getStockpileTime(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getIntensity(int skirmishAIId, int weaponDefId);

/**
 * @deprecated only relevant for visualization
 */
EXPORT(float) bridged_WeaponDef_getThickness(int skirmishAIId, int weaponDefId);

/**
 * @deprecated only relevant for visualization
 */
EXPORT(float) bridged_WeaponDef_getLaserFlareSize(int skirmishAIId, int weaponDefId);

/**
 * @deprecated only relevant for visualization
 */
EXPORT(float) bridged_WeaponDef_getCoreThickness(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getDuration(int skirmishAIId, int weaponDefId);

/**
 * @deprecated only relevant for visualization
 */
EXPORT(int) bridged_WeaponDef_getLodDistance(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getFalloffRate(int skirmishAIId, int weaponDefId);

/**
 * @deprecated only relevant for visualization
 */
EXPORT(int) bridged_WeaponDef_getGraphicsType(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isSoundTrigger(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isSelfExplode(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isGravityAffected(int skirmishAIId, int weaponDefId);

/**
 * Per weapon high trajectory setting.
 * UnitDef also has this property.
 * 
 * @return  0: low
 *          1: high
 *          2: unit
 */
EXPORT(int) bridged_WeaponDef_getHighTrajectory(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getMyGravity(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isNoExplode(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getStartVelocity(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getWeaponAcceleration(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getTurnRate(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getMaxVelocity(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getProjectileSpeed(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getExplosionSpeed(int skirmishAIId, int weaponDefId);

/**
 * Returns the bit field value denoting the categories this weapon should
 * target, excluding all others.
 * @see Game#getCategoryFlag
 * @see Game#getCategoryName
 */
EXPORT(int) bridged_WeaponDef_getOnlyTargetCategory(int skirmishAIId, int weaponDefId);

/**
 * How much the missile will wobble around its course.
 */
EXPORT(float) bridged_WeaponDef_getWobble(int skirmishAIId, int weaponDefId);

/**
 * How much the missile will dance.
 */
EXPORT(float) bridged_WeaponDef_getDance(int skirmishAIId, int weaponDefId);

/**
 * How high trajectory missiles will try to fly in.
 */
EXPORT(float) bridged_WeaponDef_getTrajectoryHeight(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isLargeBeamLaser(int skirmishAIId, int weaponDefId);

/**
 * If the weapon is a shield rather than a weapon.
 */
EXPORT(bool) bridged_WeaponDef_isShield(int skirmishAIId, int weaponDefId);

/**
 * If the weapon should be repulsed or absorbed.
 */
EXPORT(bool) bridged_WeaponDef_isShieldRepulser(int skirmishAIId, int weaponDefId);

/**
 * If the shield only affects enemy projectiles.
 */
EXPORT(bool) bridged_WeaponDef_isSmartShield(int skirmishAIId, int weaponDefId);

/**
 * If the shield only affects stuff coming from outside shield radius.
 */
EXPORT(bool) bridged_WeaponDef_isExteriorShield(int skirmishAIId, int weaponDefId);

/**
 * If the shield should be graphically shown.
 */
EXPORT(bool) bridged_WeaponDef_isVisibleShield(int skirmishAIId, int weaponDefId);

/**
 * If a small graphic should be shown at each repulse.
 */
EXPORT(bool) bridged_WeaponDef_isVisibleShieldRepulse(int skirmishAIId, int weaponDefId);

/**
 * The number of frames to draw the shield after it has been hit.
 */
EXPORT(int) bridged_WeaponDef_getVisibleShieldHitFrames(int skirmishAIId, int weaponDefId);

/**
 * Amount of the resource used per shot or per second,
 * depending on the type of projectile.
 */
EXPORT(float) bridged_WeaponDef_Shield_getResourceUse(int skirmishAIId, int weaponDefId, int resourceId); // REF:resourceId->Resource

/**
 * Size of shield covered area
 */
EXPORT(float) bridged_WeaponDef_Shield_getRadius(int skirmishAIId, int weaponDefId);

/**
 * Shield acceleration on plasma stuff.
 * How much will plasma be accelerated into the other direction
 * when it hits the shield.
 */
EXPORT(float) bridged_WeaponDef_Shield_getForce(int skirmishAIId, int weaponDefId);

/**
 * Maximum speed to which the shield can repulse plasma.
 */
EXPORT(float) bridged_WeaponDef_Shield_getMaxSpeed(int skirmishAIId, int weaponDefId);

/**
 * Amount of damage the shield can reflect. (0=infinite)
 */
EXPORT(float) bridged_WeaponDef_Shield_getPower(int skirmishAIId, int weaponDefId);

/**
 * Amount of power that is regenerated per second.
 */
EXPORT(float) bridged_WeaponDef_Shield_getPowerRegen(int skirmishAIId, int weaponDefId);

/**
 * How much of a given resource is needed to regenerate power
 * with max speed per second.
 */
EXPORT(float) bridged_WeaponDef_Shield_getPowerRegenResource(int skirmishAIId, int weaponDefId, int resourceId); // REF:resourceId->Resource

/**
 * How much power the shield has when it is created.
 */
EXPORT(float) bridged_WeaponDef_Shield_getStartingPower(int skirmishAIId, int weaponDefId);

/**
 * Number of frames to delay recharging by after each hit.
 */
EXPORT(int) bridged_WeaponDef_Shield_getRechargeDelay(int skirmishAIId, int weaponDefId);

/**
 * The color of the shield when it is at full power.
 */
EXPORT(void) bridged_WeaponDef_Shield_getGoodColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out);

/**
 * The color of the shield when it is empty.
 */
EXPORT(void) bridged_WeaponDef_Shield_getBadColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out);

/**
 * The shields alpha value.
 */
EXPORT(short) bridged_WeaponDef_Shield_getAlpha(int skirmishAIId, int weaponDefId);

/**
 * The type of the shield (bitfield).
 * Defines what weapons can be intercepted by the shield.
 * 
 * @see  getInterceptedByShieldType()
 */
EXPORT(int) bridged_WeaponDef_Shield_getInterceptType(int skirmishAIId, int weaponDefId);

/**
 * The type of shields that can intercept this weapon (bitfield).
 * The weapon can be affected by shields if:
 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
 * 
 * @see  getInterceptType()
 */
EXPORT(int) bridged_WeaponDef_getInterceptedByShieldType(int skirmishAIId, int weaponDefId);

/**
 * Tries to avoid friendly units while aiming?
 */
EXPORT(bool) bridged_WeaponDef_isAvoidFriendly(int skirmishAIId, int weaponDefId);

/**
 * Tries to avoid features while aiming?
 */
EXPORT(bool) bridged_WeaponDef_isAvoidFeature(int skirmishAIId, int weaponDefId);

/**
 * Tries to avoid neutral units while aiming?
 */
EXPORT(bool) bridged_WeaponDef_isAvoidNeutral(int skirmishAIId, int weaponDefId);

/**
 * If nonzero, targetting units will TryTarget at the edge of collision sphere
 * (radius*tag value, [-1;1]) instead of its centre.
 */
EXPORT(float) bridged_WeaponDef_getTargetBorder(int skirmishAIId, int weaponDefId);

/**
 * If greater than 0, the range will be checked in a cylinder
 * (height=range*cylinderTargetting) instead of a sphere.
 */
EXPORT(float) bridged_WeaponDef_getCylinderTargetting(int skirmishAIId, int weaponDefId);

/**
 * For beam-lasers only - always hit with some minimum intensity
 * (a damage coeffcient normally dependent on distance).
 * Do not confuse this with the intensity tag, it i completely unrelated.
 */
EXPORT(float) bridged_WeaponDef_getMinIntensity(int skirmishAIId, int weaponDefId);

/**
 * Controls cannon range height boost.
 * 
 * default: -1: automatically calculate a more or less sane value
 */
EXPORT(float) bridged_WeaponDef_getHeightBoostFactor(int skirmishAIId, int weaponDefId);

/**
 * Multiplier for the distance to the target for priority calculations.
 */
EXPORT(float) bridged_WeaponDef_getProximityPriority(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getCollisionFlags(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isSweepFire(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isAbleToAttackGround(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getCameraShake(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getDynDamageExp(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getDynDamageMin(int skirmishAIId, int weaponDefId);

EXPORT(float) bridged_WeaponDef_getDynDamageRange(int skirmishAIId, int weaponDefId);

EXPORT(bool) bridged_WeaponDef_isDynDamageInverted(int skirmishAIId, int weaponDefId);

EXPORT(int) bridged_WeaponDef_getCustomParams(int skirmishAIId, int weaponDefId, const char** keys, const char** values); // MAP

EXPORT(bool) bridged_Debug_GraphDrawer_isEnabled(int skirmishAIId);


// END: COMMAND_WRAPPERS

/**
 * Allows one to give an income (dis-)advantage to the team
 * controlled by the Skirmish AI.
 * This value can also be set through the GameSetup script,
 * with the difference that it causes an instant desync when set here.
 * @param factor  default: 1.0; common: [0.0, 2.0]; valid: [0.0, FLOAT_MAX]
 */
EXPORT(int) bridged_Cheats_setMyIncomeMultiplier(int skirmishAIId, float factor); // error-return:0=OK

/**
 * The AI team receives the specified amount of units of the specified resource.
 */
EXPORT(int) bridged_Cheats_giveMeResource(int skirmishAIId, int resourceId, float amount); // REF:resourceId->Resource error-return:0=OK

/**
 * Creates a new unit with the selected name at pos,
 * and returns its unit ID in ret_newUnitId.
 */
EXPORT(int) bridged_Cheats_giveMeUnit(int skirmishAIId, int unitDefId, float* pos_posF3); // REF:unitDefId->UnitDef REF:ret_newUnitId->Unit

/**
 * @brief Sends a chat/text message to other players.
 * This text will also end up in infolog.txt.
 */
EXPORT(int) bridged_Game_sendTextMessage(int skirmishAIId, const char* text, int zone); // error-return:0=OK

/**
 * Assigns a map location to the last text message sent by the AI.
 */
EXPORT(int) bridged_Game_setLastMessagePosition(int skirmishAIId, float* pos_posF3); // error-return:0=OK

/**
 * Give \<amount\> units of resource \<resourceId\> to team \<receivingTeam\>.
 * - the amount is capped to the AI team's resource levels
 * - does not check for alliance with \<receivingTeam\>
 * - LuaRules might not allow resource transfers, AI's must verify the deduction
 */
EXPORT(bool) bridged_Economy_sendResource(int skirmishAIId, int resourceId, float amount, int receivingTeamId); // REF:resourceId->Resource REF:receivingTeamId->Team

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
EXPORT(int) bridged_Economy_sendUnits(int skirmishAIId, int* unitIds, int unitIds_size, int receivingTeamId); // REF:MULTI:unitIds->Unit REF:receivingTeamId->Team

/**
 * Creates a group and returns the id it was given, returns -1 on failure
 */
EXPORT(int) bridged_Group_create(int skirmishAIId); // REF:ret_groupId->Group STATIC

/**
 * Erases a specified group
 */
EXPORT(int) bridged_Group_erase(int skirmishAIId, int groupId); // REF:groupId->Group error-return:0=OK

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
 * @param start_posF3  The starting location of the requested path
 * @param end_posF3  The goal location of the requested path
 * @param pathType  For what type of unit should the path be calculated
 * @param goalRadius  default: 8.0f
 */
EXPORT(int) bridged_Pathing_initPath(int skirmishAIId, float* start_posF3, float* end_posF3, int pathType, float goalRadius); // REF:ret_pathId->Path

/**
 * Returns the approximate path cost between two points.
 * - for pathType @see UnitDef_MoveData_getPathType()
 * - goalRadius defines a goal area within which any square could be accepted as
 *   path target. If a singular goal position is wanted, use 0.0f.
 *   default: 8.0f
 * @param start_posF3  The starting location of the requested path
 * @param end_posF3  The goal location of the requested path
 * @param pathType  For what type of unit should the path be calculated
 * @param goalRadius  default: 8.0f
 */
EXPORT(float) bridged_Pathing_getApproximateLength(int skirmishAIId, float* start_posF3, float* end_posF3, int pathType, float goalRadius);

EXPORT(int) bridged_Pathing_getNextWaypoint(int skirmishAIId, int pathId, float* ret_nextWaypoint_posF3_out); // REF:pathId->Path error-return:0=OK

EXPORT(int) bridged_Pathing_freePath(int skirmishAIId, int pathId); // REF:pathId->Path error-return:0=OK

/**
 * @param inData  Can be set to NULL to skip passing in a string
 * @param inSize  If this is less than 0, the data size is calculated using strlen()
 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
 */
EXPORT(int) bridged_Lua_callRules(int skirmishAIId, const char* inData, int inSize, const char* ret_outData); // error-return:0=OK

/**
 * @param inData  Can be set to NULL to skip passing in a string
 * @param inSize  If this is less than 0, the data size is calculated using strlen()
 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
 */
EXPORT(int) bridged_Lua_callUI(int skirmishAIId, const char* inData, int inSize, const char* ret_outData); // error-return:0=OK

/**
 * @param pos_posF3  on this position, only x and z matter
 */
EXPORT(int) bridged_Game_sendStartPosition(int skirmishAIId, bool ready, float* pos_posF3); // error-return:0=OK

/**
 * @param pos_posF3  on this position, only x and z matter
 */
EXPORT(int) bridged_Map_Drawer_addNotification(int skirmishAIId, float* pos_posF3, short* color_colorS3, short alpha); // error-return:0=OK

/**
 * @param pos_posF3  on this position, only x and z matter
 * @param label  create this text on pos in my team color
 */
EXPORT(int) bridged_Map_Drawer_addPoint(int skirmishAIId, float* pos_posF3, const char* label); // error-return:0=OK

/**
 * @param pos_posF3  remove map points and lines near this point (100 distance)
 */
EXPORT(int) bridged_Map_Drawer_deletePointsAndLines(int skirmishAIId, float* pos_posF3); // error-return:0=OK

/**
 * @param posFrom_posF3  draw line from this pos
 * @param posTo_posF3  to this pos, again only x and z matter
 */
EXPORT(int) bridged_Map_Drawer_addLine(int skirmishAIId, float* posFrom_posF3, float* posTo_posF3); // error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_start(int skirmishAIId, float* pos_posF3, short* color_colorS3, short alpha); // error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_finish(int skirmishAIId, bool iAmUseless); // error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawLine(int skirmishAIId, float* endPos_posF3, short* color_colorS3, short alpha); // error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawLineAndCommandIcon(int skirmishAIId, int cmdId, float* endPos_posF3, short* color_colorS3, short alpha); // REF:cmdId->Command error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawIcon(int skirmishAIId, int cmdId); // REF:cmdId->Command error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_suspend(int skirmishAIId, float* endPos_posF3, short* color_colorS3, short alpha); // error-return:0=OK

EXPORT(int) bridged_Map_Drawer_PathDrawer_restart(int skirmishAIId, bool sameColor); // error-return:0=OK

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
 * @param arrow  true: means that the figure will get an arrow at the end
 * @param lifeTime  how many frames a figure should live before being autoremoved, 0 means no removal
 * @param figureGroupId  use 0 to get a new group
 * @param ret_newFigureGroupId  the new group
 */
EXPORT(int) bridged_Map_Drawer_Figure_drawSpline(int skirmishAIId, float* pos1_posF3, float* pos2_posF3, float* pos3_posF3, float* pos4_posF3, float width, bool arrow, int lifeTime, int figureGroupId); // REF:figureGroupId->FigureGroup REF:ret_newFigureGroupId->FigureGroup

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
 * @param arrow  true: means that the figure will get an arrow at the end
 * @param lifeTime  how many frames a figure should live before being autoremoved, 0 means no removal
 * @param figureGroupId  use 0 to get a new group
 * @param ret_newFigureGroupId  the new group
 */
EXPORT(int) bridged_Map_Drawer_Figure_drawLine(int skirmishAIId, float* pos1_posF3, float* pos2_posF3, float width, bool arrow, int lifeTime, int figureGroupId); // REF:figureGroupId->FigureGroup REF:ret_newFigureGroupId->FigureGroup

/**
 * Sets the color used to draw all lines of figures in a figure group.
 * @param color_colorS3  (x, y, z) -> (red, green, blue)
 */
EXPORT(int) bridged_Map_Drawer_Figure_setColor(int skirmishAIId, int figureGroupId, short* color_colorS3, short alpha); // REF:figureGroupId->FigureGroup error-return:0=OK

/**
 * Removes a figure group, which means it will not be drawn anymore.
 */
EXPORT(int) bridged_Map_Drawer_Figure_remove(int skirmishAIId, int figureGroupId); // REF:figureGroupId->FigureGroup error-return:0=OK

/**
 * This function allows you to draw units onto the map.
 * - they only show up on the local player's screen
 * - they will be drawn in the "standard pose" (as if before any COB scripts are run)
 * @param rotation  in radians
 * @param lifeTime  specifies how many frames a figure should live before being auto-removed; 0 means no removal
 * @param teamId  teamId affects the color of the unit
 */
EXPORT(int) bridged_Map_Drawer_drawUnit(int skirmishAIId, int toDrawUnitDefId, float* pos_posF3, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing); // REF:toDrawUnitDefId->UnitDef error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param facing  set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
 */
EXPORT(int) bridged_Unit_build(int skirmishAIId, int unitId, int toBuildUnitDefId, float* buildPos_posF3, int facing, short options, int timeOut); // REF:toBuildUnitDefId->UnitDef error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param facing  set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
 */
EXPORT(int) bridged_Group_build(int skirmishAIId, int groupId, int toBuildUnitDefId, float* buildPos_posF3, int facing, short options, int timeOut); // REF:toBuildUnitDefId->UnitDef error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_stop(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_stop(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_wait(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_wait(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param time  the time in seconds to wait
 */
EXPORT(int) bridged_Unit_waitFor(int skirmishAIId, int unitId, int time, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param time  the time in seconds to wait
 */
EXPORT(int) bridged_Group_waitFor(int skirmishAIId, int groupId, int time, short options, int timeOut); // error-return:0=OK

/**
 * Wait until another unit is dead, units will not wait on themselves.
 * Example:
 * A group of aircrafts waits for an enemy's anti-air defenses to die,
 * before passing over their ruins to attack.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param toDieUnitId  wait until this unit is dead
 */
EXPORT(int) bridged_Unit_waitForDeathOf(int skirmishAIId, int unitId, int toDieUnitId, short options, int timeOut); // REF:toDieUnitId->Unit error-return:0=OK

/**
 * Wait until another unit is dead, units will not wait on themselves.
 * Example:
 * A group of aircrafts waits for an enemy's anti-air defenses to die,
 * before passing over their ruins to attack.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param toDieUnitId  wait until this unit is dead
 */
EXPORT(int) bridged_Group_waitForDeathOf(int skirmishAIId, int groupId, int toDieUnitId, short options, int timeOut); // REF:toDieUnitId->Unit error-return:0=OK

/**
 * Wait for a specific ammount of units.
 * Usually used with factories, but does work on groups without a factory too.
 * Example:
 * Pick a factory and give it a rallypoint, then add a SquadWait command
 * with the number of units you want in your squads.
 * Units will wait at the initial rally point until enough of them
 * have arrived to make up a squad, then they will continue along their queue.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_waitForSquadSize(int skirmishAIId, int unitId, int numUnits, short options, int timeOut); // error-return:0=OK

/**
 * Wait for a specific ammount of units.
 * Usually used with factories, but does work on groups without a factory too.
 * Example:
 * Pick a factory and give it a rallypoint, then add a SquadWait command
 * with the number of units you want in your squads.
 * Units will wait at the initial rally point until enough of them
 * have arrived to make up a squad, then they will continue along their queue.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_waitForSquadSize(int skirmishAIId, int groupId, int numUnits, short options, int timeOut); // error-return:0=OK

/**
 * Wait for the arrival of all units included in the command.
 * Only makes sense for a group of units.
 * Use it after a movement command of some sort (move / fight).
 * Units will wait until all members of the GatherWait command have arrived
 * at their destinations before continuing.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_waitForAll(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * Wait for the arrival of all units included in the command.
 * Only makes sense for a group of units.
 * Use it after a movement command of some sort (move / fight).
 * Units will wait until all members of the GatherWait command have arrived
 * at their destinations before continuing.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_waitForAll(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_moveTo(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_moveTo(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_patrolTo(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_patrolTo(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_fight(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_fight(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_attack(int skirmishAIId, int unitId, int toAttackUnitId, short options, int timeOut); // REF:toAttackUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_attack(int skirmishAIId, int groupId, int toAttackUnitId, short options, int timeOut); // REF:toAttackUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_attackArea(int skirmishAIId, int unitId, float* toAttackPos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_attackArea(int skirmishAIId, int groupId, float* toAttackPos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_guard(int skirmishAIId, int unitId, int toGuardUnitId, short options, int timeOut); // REF:toGuardUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_guard(int skirmishAIId, int groupId, int toGuardUnitId, short options, int timeOut); // REF:toGuardUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_aiSelect(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_aiSelect(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_addToGroup(int skirmishAIId, int unitId, int toGroupId, short options, int timeOut); // REF:toGroupId->Group error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_addToGroup(int skirmishAIId, int groupId, int toGroupId, short options, int timeOut); // REF:toGroupId->Group error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_removeFromGroup(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_removeFromGroup(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_repair(int skirmishAIId, int unitId, int toRepairUnitId, short options, int timeOut); // REF:toRepairUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_repair(int skirmishAIId, int groupId, int toRepairUnitId, short options, int timeOut); // REF:toRepairUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param fireState  can be: 0=hold fire, 1=return fire, 2=fire at will
 */
EXPORT(int) bridged_Unit_setFireState(int skirmishAIId, int unitId, int fireState, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param fireState  can be: 0=hold fire, 1=return fire, 2=fire at will
 */
EXPORT(int) bridged_Group_setFireState(int skirmishAIId, int groupId, int fireState, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param moveState  0=hold pos, 1=maneuvre, 2=roam
 */
EXPORT(int) bridged_Unit_setMoveState(int skirmishAIId, int unitId, int moveState, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param moveState  0=hold pos, 1=maneuvre, 2=roam
 */
EXPORT(int) bridged_Group_setMoveState(int skirmishAIId, int groupId, int moveState, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_setBase(int skirmishAIId, int unitId, float* basePos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_setBase(int skirmishAIId, int groupId, float* basePos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_selfDestruct(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_selfDestruct(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_setWantedMaxSpeed(int skirmishAIId, int unitId, float wantedMaxSpeed, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_setWantedMaxSpeed(int skirmishAIId, int groupId, float wantedMaxSpeed, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 */
EXPORT(int) bridged_Unit_loadUnits(int skirmishAIId, int unitId, int* toLoadUnitIds, int toLoadUnitIds_size, short options, int timeOut); // REF:MULTI:toLoadUnitIds->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 */
EXPORT(int) bridged_Group_loadUnits(int skirmishAIId, int groupId, int* toLoadUnitIds, int toLoadUnitIds_size, short options, int timeOut); // REF:MULTI:toLoadUnitIds->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_loadUnitsInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_loadUnitsInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_loadOnto(int skirmishAIId, int unitId, int transporterUnitId, short options, int timeOut); // REF:transporterUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_loadOnto(int skirmishAIId, int groupId, int transporterUnitId, short options, int timeOut); // REF:transporterUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_unload(int skirmishAIId, int unitId, float* toPos_posF3, int toUnloadUnitId, short options, int timeOut); // REF:toUnloadUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_unload(int skirmishAIId, int groupId, float* toPos_posF3, int toUnloadUnitId, short options, int timeOut); // REF:toUnloadUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_unloadUnitsInArea(int skirmishAIId, int unitId, float* toPos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_unloadUnitsInArea(int skirmishAIId, int groupId, float* toPos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_setOn(int skirmishAIId, int unitId, bool on, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_setOn(int skirmishAIId, int groupId, bool on, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_reclaimUnit(int skirmishAIId, int unitId, int toReclaimUnitId, short options, int timeOut); // REF:toReclaimUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_reclaimUnit(int skirmishAIId, int groupId, int toReclaimUnitId, short options, int timeOut); // REF:toReclaimUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_reclaimFeature(int skirmishAIId, int unitId, int toReclaimFeatureId, short options, int timeOut); // REF:toReclaimFeatureId->Feature error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_reclaimFeature(int skirmishAIId, int groupId, int toReclaimFeatureId, short options, int timeOut); // REF:toReclaimFeatureId->Feature error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_reclaimInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_reclaimInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_cloak(int skirmishAIId, int unitId, bool cloak, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_cloak(int skirmishAIId, int groupId, bool cloak, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_stockpile(int skirmishAIId, int unitId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_stockpile(int skirmishAIId, int groupId, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_dGun(int skirmishAIId, int unitId, int toAttackUnitId, short options, int timeOut); // REF:toAttackUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_dGun(int skirmishAIId, int groupId, int toAttackUnitId, short options, int timeOut); // REF:toAttackUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_dGunPosition(int skirmishAIId, int unitId, float* pos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_dGunPosition(int skirmishAIId, int groupId, float* pos_posF3, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_restoreArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_restoreArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_setRepeat(int skirmishAIId, int unitId, bool repeat, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_setRepeat(int skirmishAIId, int groupId, bool repeat, short options, int timeOut); // error-return:0=OK

/**
 * Tells weapons that support it to try to use a high trajectory
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param trajectory  0: low-trajectory, 1: high-trajectory
 */
EXPORT(int) bridged_Unit_setTrajectory(int skirmishAIId, int unitId, int trajectory, short options, int timeOut); // error-return:0=OK

/**
 * Tells weapons that support it to try to use a high trajectory
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param trajectory  0: low-trajectory, 1: high-trajectory
 */
EXPORT(int) bridged_Group_setTrajectory(int skirmishAIId, int groupId, int trajectory, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_resurrect(int skirmishAIId, int unitId, int toResurrectFeatureId, short options, int timeOut); // REF:toResurrectFeatureId->Feature error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_resurrect(int skirmishAIId, int groupId, int toResurrectFeatureId, short options, int timeOut); // REF:toResurrectFeatureId->Feature error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_resurrectInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_resurrectInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_capture(int skirmishAIId, int unitId, int toCaptureUnitId, short options, int timeOut); // REF:toCaptureUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_capture(int skirmishAIId, int groupId, int toCaptureUnitId, short options, int timeOut); // REF:toCaptureUnitId->Unit error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_captureInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_captureInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut); // error-return:0=OK

/**
 * Set the percentage of health at which a unit will return to a save place.
 * This only works for a few units so far, mainly aircraft.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param autoRepairLevel  0: 0%, 1: 30%, 2: 50%, 3: 80%
 */
EXPORT(int) bridged_Unit_setAutoRepairLevel(int skirmishAIId, int unitId, int autoRepairLevel, short options, int timeOut); // error-return:0=OK

/**
 * Set the percentage of health at which a unit will return to a save place.
 * This only works for a few units so far, mainly aircraft.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param autoRepairLevel  0: 0%, 1: 30%, 2: 50%, 3: 80%
 */
EXPORT(int) bridged_Group_setAutoRepairLevel(int skirmishAIId, int groupId, int autoRepairLevel, short options, int timeOut); // error-return:0=OK

/**
 * Set what a unit should do when it is idle.
 * This only works for a few units so far, mainly aircraft.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param idleMode  0: fly, 1: land
 */
EXPORT(int) bridged_Unit_setIdleMode(int skirmishAIId, int unitId, int idleMode, short options, int timeOut); // error-return:0=OK

/**
 * Set what a unit should do when it is idle.
 * This only works for a few units so far, mainly aircraft.
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 * @param idleMode  0: fly, 1: land
 */
EXPORT(int) bridged_Group_setIdleMode(int skirmishAIId, int groupId, int idleMode, short options, int timeOut); // error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Unit_executeCustomCommand(int skirmishAIId, int unitId, int cmdId, float* params, int params_size, short options, int timeOut); // ARRAY:params error-return:0=OK

/**
 * @param options  see enum UnitCommandOptions
 * @param timeOut  At which frame the command will time-out and consequently be removed,
 *                 if execution of it has not yet begun.
 *                 Can only be set locally, is not sent over the network, and is used
 *                 for temporary orders.
 *                 default: MAX_INT (-> do not time-out)
 *                 example: currentFrame + 15
 */
EXPORT(int) bridged_Group_executeCustomCommand(int skirmishAIId, int groupId, int cmdId, float* params, int params_size, short options, int timeOut); // ARRAY:params error-return:0=OK

EXPORT(int) bridged_Map_Drawer_traceRay(int skirmishAIId, float* rayPos_posF3, float* rayDir_posF3, float rayLen, int srcUnitId, int flags); // REF:srcUnitId->Unit REF:ret_hitUnitId->Unit

EXPORT(int) bridged_Map_Drawer_traceRayFeature(int skirmishAIId, float* rayPos_posF3, float* rayDir_posF3, float rayLen, int srcUnitId, int flags); // REF:srcUnitId->Unit REF:ret_hitFeatureId->Feature

/**
 * Pause or unpauses the game.
 * This is meant for debugging purposes.
 * Keep in mind that pause does not happen immediately.
 * It can take 1-2 frames in single- and up to 10 frames in multiplayer matches.
 * @param reason  reason for the (un-)pause, or NULL
 */
EXPORT(int) bridged_Game_setPause(int skirmishAIId, bool enable, const char* reason); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_setPosition(int skirmishAIId, float x, float y); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_setSize(int skirmishAIId, float w, float h); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_addPoint(int skirmishAIId, int lineId, float x, float y); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_deletePoints(int skirmishAIId, int lineId, int numPoints); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_setColor(int skirmishAIId, int lineId, short* color_colorS3); // error-return:0=OK

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_setLabel(int skirmishAIId, int lineId, const char* label); // error-return:0=OK

EXPORT(int) bridged_Debug_addOverlayTexture(int skirmishAIId, const float* texData, int w, int h); // REF:ret_textureId->OverlayTexture

EXPORT(int) bridged_Debug_OverlayTexture_update(int skirmishAIId, int overlayTextureId, const float* texData, int x, int y, int w, int h); // error-return:0=OK

EXPORT(int) bridged_Debug_OverlayTexture_remove(int skirmishAIId, int overlayTextureId); // error-return:0=OK

EXPORT(int) bridged_Debug_OverlayTexture_setPosition(int skirmishAIId, int overlayTextureId, float x, float y); // error-return:0=OK

EXPORT(int) bridged_Debug_OverlayTexture_setSize(int skirmishAIId, int overlayTextureId, float w, float h); // error-return:0=OK

EXPORT(int) bridged_Debug_OverlayTexture_setLabel(int skirmishAIId, int overlayTextureId, const char* label); // error-return:0=OK

// END: COMMAND_WRAPPERS

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _COMBINED_CALLBACK_BRIDGE_H

