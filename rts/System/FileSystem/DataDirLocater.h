/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DATA_DIR_LOCATER_H
#define DATA_DIR_LOCATER_H

#include <string>
#include <vector>

struct DataDir
{
	/**
	 * @brief construct a data directory object
	 *
	 * Appends a slash to the end of the path if there isn't one already.
	 */
	DataDir(const std::string& path);

	std::string path;
	bool writable;
};

class DataDirLocater
{
public:
	/**
	 * @brief construct a data directory locater
	 *
	 * Does not locate data directories, use LocateDataDirs() for that.
	 */
	DataDirLocater();

	/**
	 * @brief locate spring data directories
	 *
	 * Attempts to locate a writable data dir, and then tries to
	 * chdir to it.
	 * As the writable data dir will usually be the current dir already under
	 * windows, the chdir will have no effect.
	 *
	 * The first dir added will be the writable data dir.
	 *
	 * How the dirs get assembled
	 * --------------------------
	 * (descending priority -> first entry is searched first)
	 *
	 * If "-i isolationDir" is passed via cli or the environment variable
	 * SPRING_ISOLATED is present, isolation mode is activated. Any valid
	 * path in either of the two is added. If neither conatins a valid path,
	 * _only_ Platform::GetProcessExecutablePath() (non-UNITSYNC)
	 *     or Platform::GetModulePath() (UNITSYNC)
	 * or the relative parent dir, in case of a multi-version engine install,
	 * will be added.
	 * If the var is not present, the following hierarchy is observed:
	 *
	 * Windows:
	 * - SPRING_DATADIR env-variable (semi-colon separated list, like PATH)
	 * - ./springsettings.cfg:SpringData=C:\\data (semi-colon separated list)
	 * - CWD; in portable mode only (usually: on):
	 *   ~ path to the current work-dir/module,
	 *     which is either spring.exe or unitsync.dll, or
	 *   ~ the parent dir of the above, if it is a multi-version data-dir
	 * - "C:/.../My Documents/My Games/Spring/"
	 * - "C:/.../My Documents/Spring/"
	 * - "C:/.../All Users/Applications/Spring/"
	 * - SPRING_DATADIR compiler flag (semi-colon separated list)
	 *
	 * Max OS X:
	 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
	 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
	 * - path to the current work-dir/module
	 *   (either spring(binary) or libunitsync.dylib)
	 * - CWD; allways:
	 *   ~ path to the current work-dir/module,
	 *     which is either the spring binary or libunitsync.so, or
	 *   ~ the parent dir of the above, if it is a multi-version data-dir
	 * - {module-path}/data/
	 * - {module-path}/lib/
	 * - SPRING_DATADIR compiler flag (colon separated list)
	 *
	 * Unixes:
	 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
	 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
	 * - CWD; in portable mode only (usually: off):
	 *   ~ path to the current work-dir/module,
	 *     which is either the spring binary or libunitsync.so, or
	 *   ~ the parent dir of the above, if it is a multi-version data-dir
	 * - "$HOME/.spring"
	 * - from file '/etc/spring/datadir', preserving order
	 *   (new-line separated list)
	 * - SPRING_DATADIR compiler flag (colon separated list)
	 *   This is set by the build system, and will usually contain dirs like:
	 *   * /usr/share/games/spring/
	 *   * /usr/lib/
	 *   * /usr/lib64/
	 *   * /usr/share/lib/
	 *
	 * All of the above methods support environment variable substitution, eg.
	 * '$HOME/myspringdatadir' will be converted by spring to something like
	 * '/home/username/myspringdatadir'.
	 *
	 * If we end up with no data-dir that points to an existing path,
	 * we asume the current working directory is the data directory.
	 * @see IsPortableMode()
	 */
	void LocateDataDirs();

	const std::vector<DataDir>& GetDataDirs() const;
	const DataDir* GetWriteDir() const { return writeDir; }

	/**
	 * @brief returns the highest priority writable directory, aka the writedir
	 */
	std::string GetWriteDirPath() const;
	std::vector<std::string> GetDataDirPaths() const;

	/**
	 * Returns whether isolation-mode is enabled.
	 * In isolation-mode, we will only use a singel data-dir.
	 * This defaults to false, but can be set to true by setting the env var
	 * SPRING_ISOLATED.
	 * @see #GetIsolationModeDir
	 */
	bool IsIsolationMode() const { return isolationMode; }

	/**
	 * Sets whether isolation-mode is enabled.
	 * @see #IsIsolationMode
	 * @see #SetIsolationModeDir
	 */
	void SetIsolationMode(bool enabled) { isolationMode = enabled; }

	/**
	 * Returns the isolation-mode directory, or "", if the default one is used.
	 * The default one is CWD or CWD/.., in case of a versioned data-dir.
	 * If the env var SPRING_ISOLATED is set to a valid directory,
	 * it replaced the above mentioned default.
	 * This is only relevant if isolation-mode is active.
	 * @see #IsIsolationMode
	 */
	std::string GetIsolationModeDir() const { return isolationModeDir; }

	/**
	 * Sets the isolation-mode directory.
	 * If set to "", we use the default one, which is is CWD or CWD/..,
	 * in case of a versioned data-dir.
	 * This is only relevant if isolation-mode is active.
	 * @see #SetIsolationMode
	 */
	void SetIsolationModeDir(const std::string& dir) { isolationModeDir = dir; }


	/**
	 * @brief reads envvar to detect if we should run in isolated mode
	 * used by unitsync.cpp
	 */
	void UpdateIsolationModeByEnvVar();

private:
	void AddCurWorkDir();
	void AddInstallDir();
	void AddHomeDirs();
	void AddOsSpecificDirs();

	/**
	 * @brief substitutes environment variables with their values
	 */
	std::string SubstEnvVars(const std::string& in) const;
	/**
	 * @brief Adds the directories from a colon separated string to the data-dir
	 *   handler.
	 * @param dirs colon separated string, use ';' on Windows and ':' on all
	 *   other OSs
	 */
	void AddDirs(const std::string& dirs);
	/**
	 * @brief Adds a single directory to the datadir handler.
	 * Will only add the directory if it was not already added,
	 * as lower index in the list of dirs means higher prefference,
	 * adding it again would be pointless.
	 */
	void AddDir(const std::string& dir);
	/**
	 * @brief Figure out permissions we have for a single data directory.
	 * @returns whether we have permissions to read the data directory.
	 */
	bool DeterminePermissions(DataDir* dataDir);
	/**
	 * @brief Figure out permissions we have for the data directories.
	 */
	void DeterminePermissions();

	/**
	 * Determines whether we are in portable mode.
	 * It defines portable mode as:
	 * The spring binary (spring binary) and the synchronization
	 * library (unitsync) are in the same directory.
	 * This definition of portable mode is only valid for the data-dirs,
	 * and can not be used spring wide.
	 */
	static bool IsPortableMode();

	/**
	 * Determines whether a given path may be a data-dir for multiple engine
	 * versions.
	 * This is done by checking the precense of some dirs,
	 * like "./maps/" and "./games/".
	 * You may think of this as denoting multi-engine-version portable-mode.
	 * This will return true on a default install on windows.
	 * @param dirPath a path to a dir to check for whether it is a data-dir
	 *   for multiple engine versions.
	 * @returns whether dirPath may be a data-dir for multiple engine versions.
	 */
	static bool LooksLikeMultiVersionDataDir(const std::string& dirPath);

	bool isolationMode;
	std::string isolationModeDir;

	std::vector<DataDir> dataDirs;
	const DataDir* writeDir;
};

extern DataDirLocater dataDirLocater;

#endif // !defined(DATA_DIR_LOCATER_H)
