/**
 * @file FileSystemHandler.h
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

/**
 * @brief native file system handling abstraction
 */ 
class FileSystemHandler
{
	public:

		static FileSystemHandler& GetInstance();
		static void Cleanup();

		virtual ~FileSystemHandler();
		FileSystemHandler(int native_path_sep = '/');

		// almost direct wrappers to system calls
		virtual FILE* fopen(const std::string& file, const char* mode) const = 0;
		virtual std::ifstream* ifstream(const std::string& file, std::ios_base::openmode mode) const = 0;
		virtual std::ofstream* ofstream(const std::string& file, std::ios_base::openmode mode) const = 0;
		virtual bool mkdir(const std::string& dir) const = 0;

		// custom functions
		virtual size_t GetFilesize(const std::string& file) const;
		virtual std::string GetWriteDir() const;
		virtual std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, bool recurse, bool include_dirs) const;
		virtual std::vector<std::string> GetNativeFilenames(const std::string& file, bool write) const;

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

		FileSystem() {}

		// generic functions
		std::string GetWriteDir() const;
		std::vector<std::string> GetNativeFilenames(std::string file, bool write = false) const;

		// data read functions
		FILE* fopen(std::string file, const char* mode) const;
		std::ifstream* ifstream(std::string file, std::ios_base::openmode mode = std::ios_base::in) const;
		std::ofstream* ofstream(std::string file, std::ios_base::openmode mode = std::ios_base::out) const;

		// metadata read functions
		size_t GetFilesize(std::string path) const;

		// directory functions
		bool CreateDirectory(std::string dir) const;
		std::vector<std::string> FindFiles(std::string dir, const std::string& pattern, bool recurse = false, bool include_dirs = false) const;

		// convenience functions
		std::string GetDirectory (const std::string& path) const;
		std::string GetFilename  (const std::string& path) const;
		std::string GetBasename  (const std::string& path) const;
		std::string GetExtension (const std::string& path) const;
		std::string glob_to_regex(const std::string& glob) const;
		std::string& FixSlashes  (std::string& path) const;

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
