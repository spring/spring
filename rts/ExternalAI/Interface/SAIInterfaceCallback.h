/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S_AI_INTERFACE_CALLBACK_H
#define S_AI_INTERFACE_CALLBACK_H

#include "aidefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AI Interface -> engine callback.
 * Each AI Interface will receive an instance of this struct at initialization.
 * The interfaceId passed as the first parameter to each function in this struct
 * has to be the same as the one received by the interface,
 * when it received the instance of this struct.
 * @see SAIInterfaceLibrary
 */
struct SAIInterfaceCallback {

	/**
	 * Returns the Application Binary Interface version, fail part.
	 * If the engine and the AI INterface differ in this,
	 * the AI Interface will not be used.
	 * Changes here usually indicate that struct memebers were
	 * added or removed.
	 */
	int               (CALLING_CONV *Engine_AIInterface_ABIVersion_getFailPart)(int interfaceId);

	/**
	 * Returns the Application Binary Interface version, warning part.
	 * Interface and engine will try to run/work together,
	 * if they differ only in the warning part of the ABI version.
	 * Changes here could indicate that function arguments got changed,
	 * which could cause a crash, but it could be unimportant changes
	 * like added comments or code reformatting aswell.
	 */
	int               (CALLING_CONV *Engine_AIInterface_ABIVersion_getWarningPart)(int interfaceId);

	/// Returns the major engine revision number (e.g. 83)
	const char*       (CALLING_CONV *Engine_Version_getMajor)(int interfaceId);

	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 */
	const char*       (CALLING_CONV *Engine_Version_getMinor)(int interfaceId);

	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
	const char*       (CALLING_CONV *Engine_Version_getPatchset)(int interfaceId);

	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
	const char*       (CALLING_CONV *Engine_Version_getCommits)(int interfaceId);

	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
	const char*       (CALLING_CONV *Engine_Version_getHash)(int interfaceId);

	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
	const char*       (CALLING_CONV *Engine_Version_getBranch)(int interfaceId);

	/// Additional information (compiler flags, svn revision etc.)
	const char*       (CALLING_CONV *Engine_Version_getAdditional)(int interfaceId);

	/// time of build
	const char*       (CALLING_CONV *Engine_Version_getBuildTime)(int interfaceId);

	/// Returns whether this is a release build of the engine
	bool              (CALLING_CONV *Engine_Version_isRelease)(int interfaceId);

	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 */
	const char*       (CALLING_CONV *Engine_Version_getNormal)(int interfaceId);

	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
	const char*       (CALLING_CONV *Engine_Version_getSync)(int interfaceId);

	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 */
	const char*       (CALLING_CONV *Engine_Version_getFull)(int interfaceId);

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this interface.
	 */
	int               (CALLING_CONV *AIInterface_Info_getSize)(int interfaceId);

	/**
	 * Returns the key at index infoIndex in the info map
	 * for this interface, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *AIInterface_Info_getKey)(int interfaceId, int infoIndex);

	/**
	 * Returns the value at index infoIndex in the info map
	 * for this interface, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *AIInterface_Info_getValue)(int interfaceId, int infoIndex);

	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this interface, or NULL if the infoIndex is invalid.
	 */
	const char*       (CALLING_CONV *AIInterface_Info_getDescription)(int interfaceId, int infoIndex);

	/**
	 * Returns the value associated with the given key in the info map
	 * for this interface, or NULL if not found.
	 */
	const char*       (CALLING_CONV *AIInterface_Info_getValueByKey)(int interfaceId, const char* const key);


	/// the number of teams in this game
	int               (CALLING_CONV *Teams_getSize)(int interfaceId);

	/// the number of skirmish AIs in this game
	int               (CALLING_CONV *SkirmishAIs_getSize)(int interfaceId);

	/// the maximum number of skirmish AIs in any game
	int               (CALLING_CONV *SkirmishAIs_getMax)(int interfaceId);

	/// Returns the value accosiated to the given key from the skirmish AIs info map
	const char*       (CALLING_CONV *SkirmishAIs_Info_getValueByKey)(int interfaceId, const char* const shortName, const char* const version, const char* const key);

	/// This will end up in infolog
	void              (CALLING_CONV *Log_log)(int interfaceId, const char* const msg);

	/// This will end up in infolog
	void              (CALLING_CONV *Log_logsl)(int interfaceId, const char* section, int loglevel, const char* const msg);

	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
	void               (CALLING_CONV *Log_exception)(int interfaceId, const char* const msg, int severety, bool die);

	/// Basically returns '/' on posix and '\\' on windows
	char (CALLING_CONV *DataDirs_getPathSeparator)(int interfaceId);

	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Interfaces/C/0.1/
	 */
	const char*       (CALLING_CONV *DataDirs_getConfigDir)(int interfaceId);

	/**
	 * This interfaces writable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Interfaces/C/0.1/
	 */
	const char*       (CALLING_CONV *DataDirs_getWriteableDir)(int interfaceId);

	/**
	 * Returns an absolute path which consists of:
	 * data-dir + AI-Interface-path + relative-path.
	 *
	 * example:
	 * input:  "log/main.log", writeable, create, !dir, !common
	 * output: "/home/userX/.spring/AI/Interfaces/C/0.1/log/main.log"
	 * The path "/home/userX/.spring/AI/Interfaces/C/0.1/log/" is created,
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
	 *                     "/home/userX/.spring/AI/Interfaces/C/common/..."
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
	bool              (CALLING_CONV *DataDirs_locatePath)(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);

	/**
	 * @see     locatePath()
	 */
	char*             (CALLING_CONV *DataDirs_allocatePath)(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir, bool common);

	/// Returns the number of springs data dirs.
	int               (CALLING_CONV *DataDirs_Roots_getSize)(int interfaceId);

	/// Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1).
	bool              (CALLING_CONV *DataDirs_Roots_getDir)(int interfaceId, char* path, int path_sizeMax, int dirIndex);

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
	bool              (CALLING_CONV *DataDirs_Roots_locatePath)(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);

	char*             (CALLING_CONV *DataDirs_Roots_allocatePath)(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir);

	// Reading the file is better done directly by the interface, eg with fopen()
	//int               (CALLING_CONV *DataDirs_File_getSize)(int interfaceId, const char* const filePath);
	//int               (CALLING_CONV *DataDirs_File_getContent)(int interfaceId, void* buffer, int buffer_sizeMax, const char* const filePath);
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // S_AI_INTERFACE_CALLBACK_H
