/**
 * @file WinFileSystemHandler.cpp
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * Windows implementation supporting only one directory.
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#include "StdAfx.h"
#include <fstream>
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "WinFileSystemHandler.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "mmgr.h"

// fix for windows
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & 0170000) == 0040000) /* directory */
#endif

/**
 * @brief Creates the archive scanner and vfs handler
 */
WinFileSystemHandler::WinFileSystemHandler() : FileSystemHandler('\\')
{
	//  (same code as originally in CSpringApp::InitVFS())

	// Create the archive scanner and vfs handler
	archiveScanner = new CArchiveScanner();
	archiveScanner->ReadCacheData("archivecache.txt");
	archiveScanner->Scan("./maps", true);
	archiveScanner->Scan("./base", true);
	archiveScanner->Scan("./mods", true);
	archiveScanner->WriteCacheData("archivecache.txt");
	hpiHandler = new CVFSHandler();
}

WinFileSystemHandler::~WinFileSystemHandler()
{
}

/**
 * @brief FILE* fp = fopen() wrapper
 */
FILE* FileSystemHandler::fopen(const std::string& file, const char* mode) const
{
	return ::fopen(file.c_str(), mode);
}

/**
 * @brief std::ifstream* ifs = new std::ifstream() wrapper
 */
std::ifstream* FileSystemHandler::ifstream(const std::string& file, std::ios_base::openmode mode) const
{
	std::ifstream* ifs = new std::ifstream(file.c_str(), mode);
	if (ifs->good() && ifs->is_open())
		return ifs;
	delete ifs;
	return NULL;
}

/**
 * @brief std::ofstream* ofs = new std::ofstream() wrapper
 */
std::ofstream* FileSystemHandler::ofstream(const std::string& file, std::ios_base::openmode mode) const
{
	std::ofstream* ofs = new std::ofstream(file.c_str(), mode);
	if (ofs->good() && ofs->is_open())
		return ofs;
	delete ofs;
	return NULL;
}

/**
 * @brief creates a directory
 *
 * Returns true if the postcondition of this function is that dir exists.
 *
 * (Looks quite like the UNIX version but mkdir has different arguments.)
 */
bool WinFileSystemHandler::mkdir(const std::string& dir) const
{
	struct stat info;

	// First check if directory exists. We'll return success if it does.
	if (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
		return true;

	// If it doesn't exist we try to mkdir it and return success if that succeeds.
	if (::mkdir(path.c_str()) == 0)
		return true;

	// Otherwise we return false.
	return false;
}
