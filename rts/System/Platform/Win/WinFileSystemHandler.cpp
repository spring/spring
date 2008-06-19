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
#include <limits.h>
#include <boost/regex.hpp>
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
WinFileSystemHandler::WinFileSystemHandler(bool verbose) : FileSystemHandler('\\')
{
	//  (same code as originally in CSpringApp::InitVFS())

	// Create the archive scanner and vfs handler
	archiveScanner = new CArchiveScanner();
	archiveScanner->ReadCacheData(archiveScanner->GetFilename());
	std::vector<std::string> scanDirs;
	scanDirs.push_back("./maps");
	scanDirs.push_back("./base");
	scanDirs.push_back("./mods");
	archiveScanner->ScanDirs(scanDirs, true);
	archiveScanner->WriteCacheData(archiveScanner->GetFilename());
	hpiHandler = new CVFSHandler();
}

WinFileSystemHandler::~WinFileSystemHandler()
{
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

static void FindFiles(std::vector<std::string>& matches, const std::string& dir, const boost::regex &regexpattern, int flags)
{
	struct _finddata_t files;
	long hFile;

	if( (hFile = _findfirst( (dir + "*.*").c_str() , &files )) == -1L )
		return;

	do {
		// exclude hidden/system files
		if (files.name[0] != '.' && !(files.attrib & (_A_HIDDEN | _A_SYSTEM))) {
			// is it a file?
			if (!(files.attrib & _A_SUBDIR)) {
				if ((flags & FileSystem::ONLY_DIRS) == 0) {
					if (boost::regex_match(files.name, regexpattern)) {
						matches.push_back(dir + files.name);
					}
				}
			}
			// or a directory?
			else {
				if (flags & FileSystem::INCLUDE_DIRS) {
					if (boost::regex_match(files.name, regexpattern)) {
						matches.push_back(dir + files.name + '\\');
					}
				}					
				if (flags & FileSystem::RECURSE) {
					FindFiles(matches, dir + files.name + '\\', regexpattern, flags);
				}
			}
		}
	} while (_findnext( hFile, &files ) == 0);

	_findclose( hFile );
}

/**
 * @brief find files
 * @param dir path in which to start looking
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
std::vector<std::string> WinFileSystemHandler::FindFiles(const std::string& dir, const std::string &pattern, int flags) const
{
	assert(!dir.empty() && dir[dir.length() - 1] == '\\');

	std::vector<std::string> matches;
	boost::regex regexpattern(filesystem.glob_to_regex(pattern));

	::FindFiles(matches, dir, regexpattern, flags);

	return matches;
}
