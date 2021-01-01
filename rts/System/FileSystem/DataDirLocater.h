/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DATA_DIR_LOCATER_H
#define DATA_DIR_LOCATER_H

#include <string>
#include <array>
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
	bool writable = false;
};

class DataDirLocater
{
public:
	static DataDirLocater& GetInstance();
	static void FreeInstance();

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
	 * @see Manpage
	 */
	void LocateDataDirs();

	/**
	 * @brief call after LocateDataDirs()
	 */
	void Check();

	const std::vector<DataDir>& GetDataDirs() const;
	const DataDir* GetWriteDir() const { return writeDir; }

	/**
	 * @brief returns the highest priority writable directory, aka the writedir
	 */
	std::string GetWriteDirPath() const;

	std::vector<std::string> GetDataDirPaths() const;
	std::array<std::string, 5> GetDataDirRoots() const;

	/**
	 * Returns whether isolation-mode is enabled.
	 * In isolation-mode, we will only use a single data-dir.
	 * This defaults to false, but can be set to true by setting the env var
	 * SPRING_ISOLATED.
	 * @see #GetIsolationModeDir
	 */
	bool IsIsolationMode() const { return isolationMode; }

	/**
	 * Determines whether we are in portable mode.
	 * It defines portable mode as:
	 * The spring binary (spring binary), the unitsync
	 * and springsettings.cfg are in the same directory.
	 */
	static bool IsPortableMode();

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
	 * Force the write directory.
	 * If set to "", we use the default one.
	 */
	void SetWriteDir(const std::string& dir) { forcedWriteDir = dir; }

	/**
	 * @brief reads envvar to detect if we should run in isolated mode
	 * used by unitsync.cpp
	 */
	void UpdateIsolationModeByEnvVar();

	/**
	 * @brief cd's into writedir
	 */
	void ChangeCwdToWriteDir();

	void CreateCacheDir(const std::string& cacheDir);

private:
	void AddCurWorkDir();
	void AddPortableDir();
	void AddHomeDirs();
	void AddEtcDirs();
	void AddShareDirs();

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
	void FilterUsableDataDirs();

	bool IsWriteableDir(DataDir* dataDir);
	void FindWriteableDataDir();

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
	static bool IsInstallDirDataDir();
	static bool LooksLikeMultiVersionDataDir(const std::string& dirPath);


private:
	bool isolationMode = false;

	std::string isolationModeDir;
	std::string forcedWriteDir;
	std::vector<DataDir> dataDirs;

	const DataDir* writeDir = nullptr;
};

#define dataDirLocater DataDirLocater::GetInstance()

#endif // !defined(DATA_DIR_LOCATER_H)
