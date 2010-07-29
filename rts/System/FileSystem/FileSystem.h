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
		bool Remove(std::string file) const;

		std::string LocateDir(std::string dir, int flags = 0) const;
		std::vector<std::string> LocateDirs(const std::string& dir) const;

		std::vector<std::string> FindDirsInDirectSubDirs(
				const std::string& relPath) const;

		// metadata read functions
		bool FileExists(std::string path) const;
		size_t GetFileSize(std::string path) const;

		// directory functions
		bool CreateDirectory(std::string dir) const;
		std::vector<std::string> FindFiles(std::string dir, const std::string& pattern, int flags = 0) const;

		// convenience functions
		std::string GetDirectory (const std::string& path) const;
		std::string GetFilename  (const std::string& path) const;
		std::string GetBasename  (const std::string& path) const;
		std::string GetExtension (const std::string& path) const;
		std::string glob_to_regex(const std::string& glob) const;
		std::string& FixSlashes  (std::string& path) const;
		std::string& ForwardSlashes(std::string& path) const;

		// access check functions
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

	private:
		bool CheckFile(const std::string& file) const;
//		bool CheckDir(const std::string& dir) const;

//		FileSystem(const FileSystem&);
//		FileSystem& operator=(const FileSystem&);
};

// basically acts like a namespace, but with semantics like configHandler has
extern FileSystem filesystem;

#endif // !FILESYSTEM_H
