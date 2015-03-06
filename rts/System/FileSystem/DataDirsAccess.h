/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DATA_DIRS_ACCESS_H
#define DATA_DIRS_ACCESS_H

#include <string>
#include <vector>


class DataDirsAccess {

	/**
	 * @brief find files
	 * @param dir path in which to start looking (tried relative to each data directory)
	 * @param pattern pattern to search for
	 * @param flags possible values: FileSystem::ONLY_DIRS, FileSystem::INCLUDE_DIRS, FileSystem::RECURSE
	 * @return absolute paths to the files
	 *
	 * Will search for a file given a particular pattern.
	 * Starts from dirpath, descending down if recurse is true.
	 */
	std::vector<std::string> FindFilesInternal(const std::string& dir, const std::string& pattern, int flags) const;

	/**
	 * Returns an absolute path if the file was found in one of the data-dirs,
	 * or the argument (relative path) if it was not found.
	 *
	 * @return  an absolute path to file on success, or the argument
	 *          (relative path) on failure
	 */
	std::string LocateFileInternal(const std::string& file) const;

public:
	// generic functions
	/**
	 * @brief locate a file
	 *
	 * Attempts to locate a file.
	 *
	 * If the FileSystem::WRITE flag is set, the path is assembled in the
	 * writable data-dir.
	 * If FileSystem::CREATE_DIRS is set it tries to create the subdirectory
	 * the file should live in.
	 *
	 * Otherwise (if flags == 0), it dispatches the call to
	 * FileSystem::LocateFile(), which either searches for it
	 * in all the data directories.
	 *
	 * @return  an absolute path to file on success, or the argument
	 *          (relative path) on failure
	 */
	std::string LocateFile(std::string file, int flags = 0) const;

	std::string LocateDir(std::string dir, int flags = 0) const;
	std::vector<std::string> LocateDirs(std::string dir) const;

	std::vector<std::string> FindDirsInDirectSubDirs(const std::string& relPath) const;
	/**
	 * @brief find files
	 *
	 * Compiles a std::vector of all files below dir that match pattern.
	 *
	 * If FileSystem::RECURSE flag is set it recurses into subdirectories.
	 * If FileSystem::INCLUDE_DIRS flag is set it includes directories in
	 * the result too, as long as they match the pattern.
	 *
	 * Note that pattern doesn't apply to the names of recursed directories,
	 * it does apply to the files inside though.
	 */
	std::vector<std::string> FindFiles(std::string dir, const std::string& pattern, int flags = 0) const;
	///@}

	/// @name access-check
	///@{
	/**
	 * Returns true if path is a relative path that exists in one of the
	 * data-dirs.
	 */
	bool InReadDir(const std::string& path);
	/**
	 * Returns true if path is a relative path that exists in the writable
	 * data-dir.
	 */
	bool InWriteDir(const std::string& path);
	///@}

private:

	/**
	 * @brief internal find-files-in-a-single-datadir-function
	 * @param datadir root of the VFS data directory. This part of the path IS NOT included in returned matches.
	 * @param dir path in which to start looking. This part of path IS included in returned matches.
	 * @param pattern pattern to search for
	 * @param flags possible values: FileSystem::ONLY_DIRS, FileSystem::INCLUDE_DIRS, FileSystem::RECURSE
	 *
	 * Will search for dirs given a particular pattern.
	 * Starts from dir, descending down if FileSystem::ONLY_DIRS is set in flags.
	 */
	void FindFilesSingleDir(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const std::string &pattern, int flags) const;
};

extern DataDirsAccess dataDirsAccess;

#endif // DATA_DIRS_ACCESS_H
