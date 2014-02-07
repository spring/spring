/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "FileSystemAbstraction.h"

#include <string>

// Win-API redifines these, which breaks things
#if defined(CreateDirectory)
	#undef CreateDirectory
#endif
#if defined(DeleteFile)
	#undef DeleteFile
#endif

/**
 * @brief filesystem interface
 *
 * Abstracts locating of content on different platforms.
 * Use this from the rest of the spring code, not FileSystemHandler!
 */
class FileSystem : public FileSystemAbstraction
{
public:

	/**
	 * @brief remove a file
	 *
	 * Operates on the current working directory.
	 */
	static bool Remove(std::string file);

	/**
	 * @brief Compares if 2 paths point to the same file/directory
	 *
	 */
	static bool ComparePaths(std::string path1, std::string path2);

	/// @name meta-data
	///@{
	/// Returns true if the file exists, and is not a directory
	static bool FileExists(std::string path);
	/**
	 * @brief get filesize
	 *
	 * @return the filesize or 0 if the file doesn't exist.
	 */
	static size_t GetFileSize(std::string path);
	///@}

	/// @name directory
	///@{
	/**
	 * @brief creates a directory recursively
	 *
	 * Works like mkdir -p, ie. attempts to create parent directories too.
	 * Operates on the current working directory.
	 * @return true if the postcondition of this function is that dir exists
	 *   in the write directory.
	 */
	static bool CreateDirectory(std::string dir);
	///@}


	static bool TouchFile(std::string filePath);

	/// @name convenience
	///@{
	/**
	 * @brief Returns the directory part of a path
	 * "/home/user/.spring/test.txt" -> "/home/user/.spring/"
	 * "test.txt" -> ""
	 */
	static std::string GetDirectory(const std::string& path);
	/**
	 * @brief Returns the filename part of a path
	 * "/home/user/.spring/test.txt" -> "test.txt"
	 */
	static std::string GetFilename(const std::string& path);
	/**
	 * @brief Returns the basename part of a path
	 * This is equvalent to the filename without extension.
	 * "/home/user/.spring/test.txt" -> "test"
	 */
	static std::string GetBasename(const std::string& path);
	/**
	 * @brief Returns the extension of the filename part of the path
	 * "/home/user/.spring/test.txt" -> "txt"
	 * "/home/user/.spring/test.txt..." -> "txt"
	 * "/home/user/.spring/test.txt. . ." -> "txt"
	 */
	static std::string GetExtension(const std::string& path);
	/**
	 * Converts the given path into a canonicalized one.
	 * CAUTION: be careful where using this, as it easily allows to link to
	 * outside a certain parent dir, for example a data-dir.
	 * @param path could be something like
	 *   "./symLinkToHome/foo/bar///./../test.log"
	 * @return with the example given in path, it could be
	 *   "./symLinkToHome/foo/test.log"
	 */
	static std::string GetNormalizedPath(const std::string& path);
	/**
	 * @brief Converts a glob expression to a regex
	 * @param glob string containing glob
	 * @return string containing regex
	 */
	static std::string ConvertGlobToRegex(const std::string& glob);
	/**
	 * Converts all slashes and backslashes in path to the
	 * native_path_separator.
	 */
	static std::string& FixSlashes(std::string& path);
	/**
	 * @brief converts backslashes in path to forward slashes
	 */
	static std::string& ForwardSlashes(std::string& path);
	///@}

	/**
	 * @brief does a little checking of a filename
	 */
	static bool CheckFile(const std::string& file);
//	static bool CheckDir(const std::string& dir) const;

	static const std::string& GetCacheBaseDir();
	static const std::string& GetCacheDir();
};

#endif // !FILE_SYSTEM_H
