/**
 * @file FileSystem.h
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <vector>
#include <string>

#include "DataDirLocater.h"

// winapi redifines these which breaks things
#if defined(CreateDirectory)
	#undef CreateDirectory
#endif
#if defined(DeleteFile)
	#undef DeleteFile
#endif

/**
 * @brief native file system handling abstraction
 */
class FileSystemHandler
{
public:

	static FileSystemHandler& GetInstance();
	static void Initialize(bool verbose);
	static void Cleanup();

	~FileSystemHandler();
	FileSystemHandler();
	void Initialize();

	// almost direct wrappers to system calls
	bool mkdir(const std::string& dir) const;
	static bool DeleteFile(const std::string& file);
	static bool FileExists(const std::string& file);
	static bool DirExists(const std::string& dir);
	/// oddly, this is non-trivial on Windows
	static bool DirIsWritable(const std::string& dir);

	void Chdir(const std::string& dir);
	/**
	 * Returns true if path matches regex ...
	 * on windows:          ^[a-zA-Z]\:[\\/]?$
	 * on all other systems: ^/$
	 */
	static bool IsFSRoot(const std::string& path);

	// custom functions
	std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, int flags) const;
	bool IsReadableFile(const std::string& file) const;
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

protected:
	void InitVFS() const;
	void FindFilesSingleDir(std::vector<std::string>& matches, const std::string& dir, const std::string &pattern, int flags) const;

	DataDirLocater locater;
	static FileSystemHandler* instance;

	static const int native_path_separator;
};

/**
 * @brief filesystem interface
 *
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
		size_t GetFilesize(std::string path) const;

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
