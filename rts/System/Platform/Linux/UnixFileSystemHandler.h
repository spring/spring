/**
 * @file UnixFileSystemHandler.h
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * Unix implementation
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#ifndef UNIXFILESYSTEMHANDLER_H
#define UNIXFILESYSTEMHANDLER_H

#include <string>
#include <vector>
#include "DataDirLocater.h"
#include "System/Platform/FileSystem.h"

/**
 * @brief Unix file system / data directory handler
 */
class UnixFileSystemHandler : public FileSystemHandler
{
	public:

		virtual ~UnixFileSystemHandler();
		UnixFileSystemHandler(bool verbose);
		virtual void Initialize();

		virtual bool mkdir(const std::string& dir) const;

		virtual std::string GetWriteDir() const;
		virtual std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, int flags) const;
		virtual std::string LocateFile(const std::string& file) const;
		virtual std::vector<std::string> GetDataDirectories() const;

	protected:

		void InitVFS() const;
		void FindFilesSingleDir(std::vector<std::string>& matches, const std::string& dir, const std::string &pattern, int flags) const;

		DataDirLocater locater;
};

#endif // !UNIXFILESYSTEMHANDLER_H
