/**
 * @file UnixFileSystemHandler.cpp
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * Unix implementation, supporting multiple data directories / search paths.
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#include "StdAfx.h"
#include <boost/regex.hpp>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "LogOutput.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Platform/ConfigHandler.h"
#include "UnixFileSystemHandler.h"
#include "mmgr.h"

/**
 * @brief construct a data directory object
 *
 * Appends a slash to the end of the path if there isn't one already.
 * Initializes the directory to be neither readable nor writable.
 */
UnixFileSystemHandler::DataDir::DataDir(const std::string& p) :
	path(p), readable(false), writable(false)
{
	if (path.empty())
		path = "./";
	if (path[path.size() - 1] != '/')
		path += '/';
}

/**
 * @brief substitute environment variables with their values
 */
std::string UnixFileSystemHandler::SubstEnvVars(const std::string& in) const
{
	bool escape = false;
	std::ostringstream out;
	for (std::string::const_iterator ch = in.begin(); ch != in.end(); ++ch) {
		if (escape) {
			escape = false;
			out << *ch;
		} else {
			switch (*ch) {
				case '\\': {
					escape = true;
					break;
				}
				case '$': {
					std::ostringstream envvar;
					for (++ch; ch != in.end() && (isalnum(*ch) || *ch == '_'); ++ch)
						envvar << *ch;
					--ch;
					char* subst = getenv(envvar.str().c_str());
					if (subst && *subst)
						out << subst;
					break;
				}
				default: {
					out << *ch;
					break;
				}
			}
		}
	}
	return out.str();
}

/**
 * @brief Adds the directories in the colon separated string to the datadir handler.
 */
void UnixFileSystemHandler::AddDirs(const std::string& in)
{
	size_t prev_colon = 0, colon;
	while ((colon = in.find(':', prev_colon)) != std::string::npos) {
		datadirs.push_back(in.substr(prev_colon, colon - prev_colon));
		prev_colon = colon + 1;
	}
	datadirs.push_back(in.substr(prev_colon));
}

/**
 * @brief Figure out permissions we have for the data directories.
 */
void UnixFileSystemHandler::DeterminePermissions(int start_at)
{
	writedir = NULL;

	for (std::vector<DataDir>::iterator d = datadirs.begin() + start_at; d != datadirs.end(); ++d) {
		if (d->path.c_str()[0] != '/' || d->path.find("..") != std::string::npos)
			throw content_error("specify data directories using absolute paths please");
		// Figure out whether we have read/write permissions
		// First check read access, if we got that check write access too
		// (no support for write-only directories)
		// Note: we check for executable bit otherwise we can't browse the directory
		// Note: we fail to test whether the path actually is a directory
		// Note: modifying permissions while or after this function runs has undefined behaviour
		if (access(d->path.c_str(), R_OK | X_OK | F_OK) == 0) {
			d->readable = true;
			// Note: disallow multiple write directories.
			// There isn't really a use for it as every thing is written to the first one anyway,
			// and it may give funny effects on errors, e.g. it probably only gives funny things
			// like network mounted datadir lost connection and suddenly files end up in some
			// other random writedir you didn't even remember you had added it.
			if (!writedir && access(d->path.c_str(), W_OK) == 0) {
				d->writable = true;
				writedir = &*d;
			}
		} else {
			if (filesystem.CreateDirectory(d->path)) {
				// it didn't exist before, now it does and we just created it with rw access,
				// so we just assume we still have read-write acces...
				d->readable = true;
				if (!writedir) {
					d->writable = true;
					writedir = &*d;
				}
			}
		}
	}
}

/**
 * @brief locate spring data directory
 *
 * On *nix platforms, attempts to locate
 * and change to the spring data directory
 *
 * The data directory to chdir to is determined by the following, in this
 * order (first items override lower items): 
 *
 * - 'SpringData=/path/to/data' declaration in '~/.springrc'. (colon separated list)
 * - 'SPRING_DATADIR' environment variable. (colon separated list, like PATH)
 * - In the same order any line in '/etc/spring/datadir', if that file exists.
 * - 'datadir=/path/to/data' option passed to 'scons configure'.
 * - 'prefix=/install/path' option passed to scons configure. The datadir is
 *   assumed to be at '$prefix/games/taspring' in this case.
 * - the default datadir in the default prefix, ie. '/usr/local/games/taspring'
 *
 * All of the above methods support environment variable substitution, eg.
 * '$HOME/myspringdatadir' will be converted by spring to something like
 * '/home/username/myspringdatadir'.
 *
 * If it fails to chdir to the above specified directory spring will asume the
 * current working directory is the data directory.
 */
void UnixFileSystemHandler::LocateDataDirs()
{
	// Construct the list of datadirs from various sources.

	datadirs.clear();

	std::string cfg = configHandler.GetString("SpringData","");
	if (!cfg.empty())
		AddDirs(SubstEnvVars(cfg));

	char* env = getenv("SPRING_DATADIR");
	if (env && *env)
		AddDirs(SubstEnvVars(env));

	FILE* f = ::fopen("/etc/spring/datadir", "r");
	if (f) {
		char buf[1024];
		while (fgets(buf, sizeof(buf), f)) {
			char* newl = strchr(buf, '\n');
			if (newl)
				*newl = 0;
			datadirs.push_back(SubstEnvVars(buf));
		}
		fclose(f);
	}

	datadirs.push_back(SubstEnvVars(SPRING_DATADIR));

	// Figure out permissions of all datadirs
	bool cwdWarning = false;

	DeterminePermissions();

	if (!writedir) {
		// add current working directory to search path & try again
		char buf[4096];
		getcwd(buf, sizeof(buf));
		buf[sizeof(buf) - 1] = 0;
		datadirs.push_back(DataDir(buf));
		DeterminePermissions(datadirs.size() - 1);
		cwdWarning = true;
	}

	if (!writedir) {
		// bail out
		throw content_error("not a single read-write data directory found!");
	}

	// for now, chdir to the datadirectory as a safety measure:
	// all AIs still just assume it's ok to put their stuff in the current directory after all
	// Not only safety anymore, it's just easier if other code can safely assume that
	// writedir == current working directory
	chdir(GetWriteDir().c_str());

	// delayed warning message (needs to go after chdir otherwise log file ends up in wrong directory)
	if (cwdWarning)
		logOutput.Print("Warning: Adding current working directory to search path.");
}

/**
 * @brief Creates the archive scanner and vfs handler
 *
 * For the archiveScanner, it keeps cache data ("archivecache.txt") in the
 * writedir. Cache data in other directories is ignored.
 * It scans maps, base and mods subdirectories of all readable datadirs
 * for archives. Archives in higher priority datadirs override archives
 * in lower priority datadirs.
 *
 * Note that the archive namespace is global, ie. each archive basename may
 * only occur once in all data directories. If a name is found more then once,
 * then higher priority datadirs override lower priority datadirs and per
 * datadir the 'mods' subdir overrides 'base' which overrides 'maps'.
 *
 * Basically the same applies to the VFS handler. It maps the contents of all
 * archives (see CVFSHandler::MapArchives()) in the root of each datadir to
 * the virtual filesystem. Files in archives in higher priority datadirs
 * override files in archives in lower priority datadirs.
 */
void UnixFileSystemHandler::InitVFS(bool mapArchives) const
{
	archiveScanner = new CArchiveScanner();
	archiveScanner->ReadCacheData(writedir->path + archiveScanner->GetFilename());
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		if (d->readable) {
			archiveScanner->Scan(d->path + "maps", true);
			archiveScanner->Scan(d->path + "base", true);
			archiveScanner->Scan(d->path + "mods", true);
		}
	}
	archiveScanner->WriteCacheData(writedir->path + archiveScanner->GetFilename());

	hpiHandler = new CVFSHandler(false);
	if (mapArchives) {
		for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
			if (d->readable) {
				hpiHandler->MapArchives(d->path);
			}
		}
	}
}

/**
 * @brief Creates the archive scanner and vfs handler
 *
 * Locates data directories and initializes the VFS.
 */
UnixFileSystemHandler::UnixFileSystemHandler(bool verbose, bool mapArchives)
{
	LocateDataDirs();
	InitVFS(mapArchives);

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			if (d->writable)
				logOutput.Print("Using read-write data directory: %s", d->path.c_str());
			else
				logOutput.Print("Using read-only  data directory: %s", d->path.c_str());
		}
	}
}


UnixFileSystemHandler::~UnixFileSystemHandler()
{
}

/**
 * @brief returns the highest priority writable directory, aka the writedir
 */
std::string UnixFileSystemHandler::GetWriteDir() const
{
	assert(writedir && writedir->writable); //duh
	return writedir->path;
}

/**
 * @brief find files
 * @param dir path in which to start looking (tried relative to each data directory)
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings containing absolute paths to the files
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
std::vector<std::string> UnixFileSystemHandler::FindFiles(const std::string& dir, const std::string& pattern, int flags) const
{
	std::vector<std::string> matches;

	// if it's an absolute path, don't look for it in the data directories
	if (dir[0] == '/') {
		FindFilesSingleDir(matches, dir, pattern, flags);
		return matches;
	}

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			FindFilesSingleDir(matches, d->path + dir, pattern, flags);
		}
	}
	return matches;
}

std::string UnixFileSystemHandler::LocateFile(const std::string& file) const
{
	// if it's an absolute path, don't look for it in the data directories
	if (file[0] == '/')
		return file;

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			std::string fn(d->path + file);
			if (access(fn.c_str(), R_OK | F_OK) == 0)
				return fn;
		}
	}
	return file;
}

std::vector<std::string> UnixFileSystemHandler::GetDataDirectories() const
{
	std::vector<std::string> f;

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable)
			f.push_back(d->path);
	}
	return f;
}

/**
 * @brief creates a rwxr-xr-x dir in the writedir
 *
 * Returns true if the postcondition of this function is that dir exists in
 * the write directory.
 *
 * Note that this function does not check access to the dir, ie. if you've
 * created it manually with 0000 permissions then this function may return
 * true, subsequent operation on files inside the directory may still fail.
 *
 * As a rule of thumb, set identical permissions on identical items in the
 * data directory, ie. all subdirectories the same perms, all files the same
 * perms.
 */
bool UnixFileSystemHandler::mkdir(const std::string& dir) const
{
	struct stat info;

	// First check if directory exists. We'll return success if it does.
	if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
		return true;

	// If it doesn't exist we try to mkdir it and return success if that succeeds.
	if (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0)
		return true;

	// Otherwise we return false.
	return false;
}

static void FindFiles(std::vector<std::string>& matches, const std::string& dir, const boost::regex &regexpattern, int flags)
{
	DIR* dp;
	struct dirent* ep;

	if (!(dp = opendir(dir.c_str())))
		return;

	while ((ep = readdir(dp))) {
		// exclude hidden files
		if (ep->d_name[0] != '.') {
			// is it a file? (we just treat sockets / pipes / fifos / character&block devices as files...)
			// (need to stat because d_type is DT_UNKNOWN on linux :-/)
			struct stat info;
			if (stat((dir + ep->d_name).c_str(), &info) == 0) {
				if (!S_ISDIR(info.st_mode)) {
					if (boost::regex_match(ep->d_name, regexpattern))
						matches.push_back(dir + ep->d_name);
				}
				// or a directory?
				else if (flags & FileSystem::RECURSE) {
					if ((flags & FileSystem::INCLUDE_DIRS) && boost::regex_match(ep->d_name, regexpattern))
						matches.push_back(dir + ep->d_name);
					FindFiles(matches, dir + ep->d_name + '/', regexpattern, flags);
				}
			}
		}
	}
	closedir(dp);
}

/**
 * @brief internal find-files-in-a-single-datadir-function
 * @param dir path in which to start looking
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
void UnixFileSystemHandler::FindFilesSingleDir(std::vector<std::string>& matches, const std::string& dir, const std::string &pattern, int flags) const
{
	assert(!dir.empty() && dir[dir.length() - 1] == '/');

	boost::regex regexpattern(filesystem.glob_to_regex(pattern));

	::FindFiles(matches, dir, regexpattern, flags);
}
