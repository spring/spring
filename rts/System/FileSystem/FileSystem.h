/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <vector>
#include <string>

// winapi redifines these which breaks things
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
class FileSystem
{
	public:
		// flags for FindFiles
		enum FindFilesBits {
			RECURSE      = (1 << 0),
			INCLUDE_DIRS = (1 << 1),
			ONLY_DIRS    = (1 << 2),
		};
		// flags for LocateFile
		enum LocateFileBits {
			WRITE       = (1 << 0),
			CREATE_DIRS = (1 << 1),
		};

	public:
		FileSystem() {}

		// generic functions
		/**
		 * @brief locate a file
		 *
		 * Attempts to locate a file.
		 *
		 * If the FileSystem::WRITE flag is set, the path is assembled in the
		 * writeable data-dir.
		 * If FileSystem::CREATE_DIRS is set it tries to create the subdirectory
		 * the file should live in.
		 *
		 * Otherwise (if flags == 0), it dispatches the call to
		 * FileSystemHandler::LocateFile(), which either searches for it
		 * in all the data directories.
		 *
		 * @return  an absolute path to file on success, or the argument
		 *          (relative path) on failure
		 */
		std::string LocateFile(std::string file, int flags = 0) const;
		/**
		 * @brief remove a file
		 *
		 * Operates on the current working directory.
		 */
		bool Remove(std::string file) const;

		std::string LocateDir(std::string dir, int flags = 0) const;
		std::vector<std::string> LocateDirs(const std::string& dir) const;

		std::vector<std::string> FindDirsInDirectSubDirs(const std::string& relPath) const;

		/// @name meta-data
		///@{
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
		 */
		static bool CreateDirectory(std::string dir);
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
		 * @brief Converts a glob expression to a regex
		 * @param glob string containing glob
		 * @return string containing regex
		 */
		static std::string glob_to_regex(const std::string& glob);
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
		 * As it is not known what the person initially creating
		 * this function, intended to do with prefix, it is ignored.
		 */
		bool InWriteDir(const std::string& path, const std::string& prefix = "");
		///@}

	private:
		/**
		 * @brief does a little checking of a filename
		 */
		static bool CheckFile(const std::string& file);
//		bool CheckDir(const std::string& dir) const;

//		FileSystem(const FileSystem&);
//		FileSystem& operator=(const FileSystem&);
};

// basically acts like a namespace, but with semantics like configHandler has
extern FileSystem filesystem;

#endif // !FILESYSTEM_H
