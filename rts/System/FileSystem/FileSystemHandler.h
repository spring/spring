/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_HANDLER_H
#define FILE_SYSTEM_HANDLER_H

#include <vector>
#include <string>

#include "DataDirLocater.h"

/**
 * @brief native file system handling abstraction
 */
class FileSystemHandler
{
	~FileSystemHandler();
	FileSystemHandler();

public:
	static FileSystemHandler& GetInstance();
	static void Initialize(bool verbose);
	static void Cleanup();

	// almost direct wrappers to system calls
	static bool mkdir(const std::string& dir);
	static bool DeleteFile(const std::string& file);
	static bool FileExists(const std::string& file);
	static bool DirExists(const std::string& dir);
	/// oddly, this is non-trivial on Windows
	static bool DirIsWritable(const std::string& dir);

	static std::string GetCwd();
	static void Chdir(const std::string& dir);
	/**
	 * Returns true if path matches regex ...
	 * on windows:          ^[a-zA-Z]\:[\\/]?$
	 * on all other systems: ^/$
	 */
	static bool IsFSRoot(const std::string& path);

	/**
	 * Returns true if the path ends with the platform native path separator
	 * (win: '\\', rest: '/').
	 */
	static bool HasPathSepAtEnd(const std::string& path);
	/**
	 * Ensures the path ends with the platform native path separator
	 * (win: '\\', rest: '/').
	 */
	static void EnsurePathSepAtEnd(std::string& path);

	/**
	 * @brief get filesize
	 *
	 * @return the filesize or 0 if the file doesn't exist.
	 */
	static size_t GetFileSize(const std::string& file);

	// custom functions
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
	std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, int flags) const;
	static bool IsReadableFile(const std::string& file);
	/**
	 * Returns the last file modification time formatted in a sort friendly
	 * way, with second resolution.
	 * 23:58:59 30 January 1999 -> "19990130235859"
	 *
	 * @return  the last file modification time as described above,
	 *          or "" on error
	 */
	static std::string GetFileModificationDate(const std::string& file);
	/**
	 * Returns an absolute path if the file was found in one of the data-dirs,
	 * or the argument (relative path) if it was not found.
	 *
	 * @return  an absolute path to file on success, or the argument
	 *          (relative path) on failure
	 */
	std::string LocateFile(const std::string& file) const;
	std::string GetWriteDir() const;
	std::vector<std::string> GetDataDirectories() const;

	static int GetNativePathSeparator() { return native_path_separator; }
	static bool IsAbsolutePath(const std::string& path);

private:
	/**
	 * @brief internal find-files-in-a-single-datadir-function
	 * @param absolute paths to the dirs found will be added to this
	 * @param datadir root of the VFS data directory. This part of the path IS NOT included in returned matches.
	 * @param dir path in which to start looking. This part of path IS included in returned matches.
	 * @param pattern pattern to search for
	 * @param flags possible values: FileSystem::ONLY_DIRS, FileSystem::INCLUDE_DIRS, FileSystem::RECURSE
	 *
	 * Will search for dirs given a particular pattern.
	 * Starts from dir, descending down if FileSystem::ONLY_DIRS is set in flags.
	 */
	void FindFilesSingleDir(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const std::string &pattern, int flags) const;

	DataDirLocater locater;
	static FileSystemHandler* instance;

	static const int native_path_separator;
};

#endif // !FILE_SYSTEM_HANDLER_H
