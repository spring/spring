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

#include <stdio.h>
#include <ios>
#include <iosfwd>
#include <vector>

// winapi redifines this which breaks things
#if defined(CreateDirectory)
# undef CreateDirectory
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

		virtual ~FileSystemHandler();
		FileSystemHandler(int native_path_sep = '/');

		// almost direct wrappers to system calls
		virtual bool mkdir(const std::string& dir) const = 0;

		// custom functions
		virtual std::string GetWriteDir() const;
		virtual std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, int flags) const = 0;
		virtual std::string LocateFile(const std::string& file) const;
		virtual std::vector<std::string> GetDataDirectories() const;

		int GetNativePathSeparator() const { return native_path_separator; }

	private:

		static FileSystemHandler* instance;

		int native_path_separator;
};

/**
 * @brief filesystem interface
 *
 * Use this from the rest of the spring code!
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
		std::string LocateFile(std::string file, int flags = 0) const;
		bool Remove(std::string file) const;

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

	private:
		bool CheckFile(const std::string& file) const;
		bool CheckDir(const std::string& dir) const;
		bool CheckMode(const char* mode) const;

		FileSystem(const FileSystem&);
		FileSystem& operator=(const FileSystem&);
};

// basically acts like a namespace, but with semantics like configHandler has
extern FileSystem filesystem;

#endif // !FILESYSTEM_H
