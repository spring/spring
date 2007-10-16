/**
 * @file WinFileSystemHandler.h
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * Windows implementation.
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#ifndef WINFILESYSTEMHANDLER_H
#define WINFILESYSTEMHANDLER_H

#include "Platform/FileSystem.h"

/**
 * @brief Windows data directory handler
 *
 * Pretty boring class with almost nothing in it.
 */
class WinFileSystemHandler : public FileSystemHandler
{
	public:

		virtual ~WinFileSystemHandler();
		WinFileSystemHandler(bool verbose);

		virtual bool mkdir(const std::string& dir) const;

		virtual std::vector<std::string> FindFiles(const std::string& dir, const std::string& pattern, int flags) const;
};

#endif // !WINFILESYSTEMHANDLER_H
