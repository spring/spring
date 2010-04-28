/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DATADIRLOCATER_H
#define DATADIRLOCATER_H

#include <string>
#include <vector>

struct DataDir
{
	/**
	 * @brief construct a data directory object
	 *
	 * Appends a slash to the end of the path if there isn't one already.
	 */
	DataDir(const std::string& p);

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
	 * Attempts to locate a writeable data dir, and then tries to
	 * chdir to it.
	 * As the writeable data dir will usually be the current dir already under windows,
	 * the chdir will have no effect.
	 *
	 * The first dir added will be the writeable data dir.
	 *
	 * How the dirs get assembled
	 * --------------------------
	 * (descending priority -> first entry is searched first)
	 *
	 * Windows:
	 * - SPRING_DATADIR env-variable (semi-colon separated list, like PATH)
	 * - ./springsettings.cfg:SpringData=C:\data (semi-colon separated list)
	 * - in portable mode only (usually: on): path to the current work-dir/module
	 *   (either spring.exe or unitsync.dll)
	 * - "C:/.../My Documents/My Games/Spring/"
	 * - "C:/.../My Documents/Spring/"
	 * - "C:/.../All Users/Applications/Spring/"
	 * - SPRING_DATADIR compiler flag (semi-colon separated list)
	 *
	 * Max OS X:
	 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
	 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
	 * - path to the current work-dir/module (either spring(binary) or libunitsync.dylib)
	 * - {module-path}/data/
	 * - {module-path}/lib/
	 * - SPRING_DATADIR compiler flag (colon separated list)
	 *
	 * Unixes:
	 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
	 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
	 * - in portable mode only (usually: off): path to the current work-dir/module
	 *   (either spring binary or libunitsync.so)
	 * - "$HOME/.spring"
	 * - from file '/etc/spring/datadir', preserving order (new-line separated list)
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

	const std::vector<DataDir>& GetDataDirs() const { return datadirs; }
	const DataDir* GetWriteDir() const { return writedir; }

private:
	/**
	 * @brief substitutes environment variables with their values
	 */
	std::string SubstEnvVars(const std::string& in) const;
	/**
	 * @brief Adds the directories from a colon separated string to the datadir handler.
	 * @param  in colon separated string, use ';' on Windows and ':' on all other OSs)
	 */
	void AddDirs(const std::string& in);
	/**
	 * @brief Adds a single directory to the datadir handler.
	 * Will only add the directory if it was not already added,
	 * as lower index in the list of dirs means higher prefference,
	 * adding it again would be pointless.
	 */
	void AddDir(const std::string& dir);
	/**
	 * @brief Figure out permissions we have for a single data directory d.
	 * @returns whether we have permissions to read the data directory.
	 */
	bool DeterminePermissions(DataDir* d);
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

	std::vector<DataDir> datadirs;
	const DataDir* writedir;
};

#endif // !defined(DATADIRLOCATER_H)
