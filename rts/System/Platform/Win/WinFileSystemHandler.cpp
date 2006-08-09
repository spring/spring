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
#include <boost/regex.hpp>
#include <fstream>
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "WinFileSystemHandler.h"
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
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
FILE* WinFileSystemHandler::fopen(const std::string& file, const char* mode) const
{
	return ::fopen(file.c_str(), mode);
}

/**
 * @brief std::ifstream* ifs = new std::ifstream() wrapper
 */
std::ifstream* WinFileSystemHandler::ifstream(const std::string& file, std::ios_base::openmode mode) const
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
std::ofstream* WinFileSystemHandler::ofstream(const std::string& file, std::ios_base::openmode mode) const
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
bool WinFileSystemHandler::mkdir(const std::string& path) const
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

static void FindFiles(std::vector<std::string>& matches, const std::string& dir, const boost::regex &regexpattern, const bool recurse, const bool include_dirs)
{
	struct _finddata_t files;
	long hFile;

	if( (hFile = _findfirst( (dir + "*.*").c_str() , &files )) == -1L )
		return;

	do {
		// exclude hidden/system files
		if (!(files.attrib & (_A_HIDDEN | _A_SYSTEM))) {
			// is it a file?
			if (!(files.attrib & _A_SUBDIR)) {
				if (boost::regex_match(files.name, regexpattern))
					matches.push_back(dir + files.name);
			}
			// or a directory?
			else if (recurse) {
				if (include_dirs && boost::regex_match(files.name, regexpattern))
					matches.push_back(dir + files.name);
				FindFiles(matches, dir + files.name + "\\", regexpattern, recurse, include_dirs);
			}
		}
	} while (_findnext( hFile, &files ) == 0);

	_findclose( hFile );
}

std::vector<std::string> WinFileSystemHandler::FindFiles(const std::string& dir, const std::string &pattern, const bool recurse, const bool include_dirs) const
{
	assert(!dir.empty() && dir[dir.length() - 1] == '\\');

	std::vector<std::string> matches;
	boost::regex regexpattern(filesystem.glob_to_regex(pattern));

	::FindFiles(matches, dir, regexpattern, recurse, include_dirs);

	return matches;
}
