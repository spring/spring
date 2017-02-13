/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_ABSTACTION_H
#define FILE_SYSTEM_ABSTACTION_H

#include <vector>
#include <string>

#if defined(CreateDirectory)
	#undef CreateDirectory
#endif
#if defined(DeleteFile)
	#undef DeleteFile
#endif

/**
 * Native file-system handling abstraction.
 * This contains only functions that have to implemented differently on
 * different OSs.
 * @note do not use this directly, but use FileSystem instead
 * @see FileSystem
 */
class FileSystemAbstraction
{
public:

	// almost direct wrappers to system calls
	static bool MkDir(const std::string& dir);
	static bool DeleteFile(const std::string& file);
	/// Returns true if the file exists, and is not a directory
	static bool FileExists(const std::string& file);
	static bool DirExists(const std::string& dir);
	/// oddly, this is non-trivial on Windows
	static bool DirIsWritable(const std::string& dir);

	static bool ComparePaths(const std::string& path1, const std::string& path2);

	static std::string GetCwd();
	static void ChDir(const std::string& dir);

	/**
	 * Removes "./" or ".\" from the start of a path string.
	 */
	static std::string RemoveLocalPathPrefix(const std::string& path);

	/**
	 * Returns true if path matches regex ...
	 * on windows:          ^[a-zA-Z]\:[\\/]?$
	 * on all other systems: ^/$
	 */
	static bool IsFSRoot(const std::string& path);

	/**
	 * Returns true if the supplied char is a path separator,
	 * that is either '\' or '/'.
	 */
	static bool IsPathSeparator(char aChar);

	/**
	 * Returns true if the supplied char is a platform native path separator,
	 * that is '\' on windows and '/' on POSIX.
	 */
	static bool IsNativePathSeparator(char aChar);

	/**
	 * Returns true if the path ends with the platform native path separator.
	 * That is '\' on windows and '/' on POSIX.
	 */
	static bool HasPathSepAtEnd(const std::string& path);

	/**
	 * Ensures the path ends with the platform native path separator.
	 * Converts the empty string to ".\" or "./" respectively.
	 * @see #HasPathSepAtEnd()
	 */
	static std::string& EnsurePathSepAtEnd(std::string& path);
	/// @see #EnsurePathSepAtEnd(std::string&)
	static std::string EnsurePathSepAtEnd(const std::string& path);

	/**
	 * Ensures the path does not end with the platform native path separator.
	 * @see #HasPathSepAtEnd()
	 */
	static void EnsureNoPathSepAtEnd(std::string& path);
	/// @see #EnsureNoPathSepAtEnd(std::string&)
	static std::string EnsureNoPathSepAtEnd(const std::string& path);

	/**
	 * Ensures the path does not end with a path separator.
	 */
	static std::string StripTrailingSlashes(const std::string& path);

	/**
	 * Returns the path to the parent of the given path element.
	 * @return the paths parent, including the trailing path separator,
	 *         or "" on error
	 */
	static std::string GetParent(const std::string& path);

	/**
	 * @brief get filesize
	 *
	 * @return the files size or -1, if the file does not exist. Returns
	 *          also -1 if the file is a directory.
	 */
	static size_t GetFileSize(const std::string& file);

	// custom functions
	static bool IsReadableFile(const std::string& file);

	static unsigned int GetFileModificationTime(const std::string& file);
	/**
	 * Returns the last file modification time formatted in a sort friendly
	 * way, with second resolution.
	 * 23:58:59 30 January 1999 -> "19990130235859"
	 *
	 * @return  the last file modification time as described above,
	 *          or "" on error
	 */
	static std::string GetFileModificationDate(const std::string& file);

	static char GetNativePathSeparator();
	static bool IsAbsolutePath(const std::string& path);

	static void FindFiles(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dir, const std::string& regex, int flags);
};

#endif // !FILE_SYSTEM_ABSTACTION_H
