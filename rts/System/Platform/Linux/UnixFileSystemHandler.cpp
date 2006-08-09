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
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Platform/ConfigHandler.h"
#include "UnixFileSystemHandler.h"
#include "mmgr.h"

/**
 * @brief creates a directory
 *
 * Returns true if postcondition is that directory exists.
 * Used in UnixFileSystemHandler::mkdir() and to try to create data directories
 * if they don't yet exist.
 */
static bool mkdir_helper(const std::string& dir, bool verbose = false)
{
	struct stat info;

	// First check if directory exists. We'll return success if it does.
	if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
		return true;

	if (verbose)
		fprintf(stderr, "WARNING: trying to create directory: %s ... ", dir.c_str());

	// If it doesn't exist we try to mkdir it and return success if that succeeds.
	if (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
		if (verbose)
			fprintf(stderr, "Success\n");
		return true;
	}

	if (verbose)
		perror("");

	// Otherwise we return false.
	return false;
}

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
				case '\\':
					escape = true;
					break;
				case '$':  {
					std::ostringstream envvar;
					for (++ch; ch != in.end() && (isalnum(*ch) || *ch == '_'); ++ch)
						envvar << *ch;
					--ch;
					char* subst = getenv(envvar.str().c_str());
					if (subst && *subst)
						out << subst;
					break; }
				default:
					out << *ch;
					break;
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
				fprintf(stderr, "using read-write data directory: %s\n", d->path.c_str());
				writedir = &*d;
			} else {
				fprintf(stderr, "using read-only  data directory: %s\n", d->path.c_str());
			}
		} else {
			fprintf(stderr, "WARNING: can not access data directory: %s\n", d->path.c_str());
			size_t prev_slash = 0, slash;
			bool success = true;
			while ((slash = d->path.find('/', prev_slash + 1)) != std::string::npos) {
				if (!mkdir_helper(d->path.substr(0, slash), true)) {
					success = false;
					break;
				}
				prev_slash = slash;
			}
			if (success) {
				// it didn't exist before, now it does and we just created it with rw access,
				// so we just assume we still have read-write acces...
				d->readable = true;
				if (!writedir) {
					fprintf(stderr, "using read-write data directory: %s\n", d->path.c_str());
					d->writable = true;
					writedir = &*d;
				} else {
					fprintf(stderr, "using read-only  data directory: %s\n", d->path.c_str());
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

	DeterminePermissions();

	if (!writedir) {
		// add current working directory to search path & try again
		fprintf(stderr, "WARNING: adding current working directory to search path\n");
		char buf[4096];
		getcwd(buf, sizeof(buf));
		buf[sizeof(buf) - 1] = 0;
		datadirs.push_back(DataDir(buf));
		DeterminePermissions(datadirs.size() - 1);
	}

	if (!writedir) {
		// bail out
		throw content_error("not a single read-write data directory found!");
	}
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
	archiveScanner->ReadCacheData(writedir->path + "archivecache.txt");
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		if (d->readable) {
			archiveScanner->Scan(d->path + "maps", true);
			archiveScanner->Scan(d->path + "base", true);
			archiveScanner->Scan(d->path + "mods", true);
		}
	}
	archiveScanner->WriteCacheData(writedir->path + "archivecache.txt");

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
UnixFileSystemHandler::UnixFileSystemHandler()
{
	LocateDataDirs();
	InitVFS();
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

std::vector<std::string> UnixFileSystemHandler::FindFiles(const std::string& dir, const std::string& pattern, bool recurse, bool include_dirs) const
{
	// if it's an absolute path, don't look for it in the data directories
	if (dir[0] == '/')
		return FileSystemHandler::FindFiles(dir, pattern, recurse, include_dirs);

	std::vector<std::string> match;
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			std::vector<std::string> submatch = FileSystemHandler::FindFiles(d->path + dir, pattern, recurse, include_dirs);
			match.insert(match.end(), submatch.begin(), submatch.end());
		}
	}
	return match;
}

std::vector<std::string> UnixFileSystemHandler::GetNativeFilenames(const std::string& file, bool write) const
{
	std::vector<std::string> f;

	// if it's an absolute path, don't look for it in the data directories
	if (file[0] == '/') {
		f.push_back(file);
		return f;
	}

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			if (!write || d->writable)
				f.push_back(d->path + file);
		}
	}
	return f;
}

/**
 * @brief FILE* fp = fopen() wrapper
 *
 * Walks through all data directories until it finds one in
 * which it has sufficient privileges to open the file.
 */
FILE* UnixFileSystemHandler::fopen(const std::string& file, const char* mode) const
{
	// if it's an absolute path, don't look for it in the data directories
	if (file[0] == '/')
		return ::fopen(file.c_str(), mode);

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if ((mode[0] == 'r' && !strchr(mode, '+') && d->readable) || d->writable) {
			FILE* f = ::fopen((d->path + file).c_str(), mode);
			if (f)
				return f;
		}
	}
	return NULL;
}

/**
 * @brief std::ifstream* ifs = new std::ifstream() wrapper
 *
 * Walks through all data directories until it finds one in
 * which it has sufficient privileges to open the file.
 */
std::ifstream* UnixFileSystemHandler::ifstream(const std::string& file, std::ios_base::openmode mode) const
{
	std::ifstream* ifs = new std::ifstream;

	// if it's an absolute path, don't look for it in the data directories
	if (file[0] == '/') {
		ifs->open(file.c_str(), mode);
		if (ifs->good() && ifs->is_open())
			return ifs;
		delete ifs;
		return NULL;
	}

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			ifs->clear();
			ifs->open((d->path + file).c_str(), mode);
			if (ifs->good() && ifs->is_open())
				return ifs;
		}
	}
	delete ifs;
	return NULL;
}

/**
 * @brief std::ofstream* ofs = new std::ofstream() wrapper
 *
 * Walks through all data directories until it finds one in
 * which it has sufficient privileges to open the file.
 */
std::ofstream* UnixFileSystemHandler::ofstream(const std::string& file, std::ios_base::openmode mode) const
{
	std::ofstream* ofs = new std::ofstream;

	// if it's an absolute path, don't look for it in the data directories
	if (file[0] == '/') {
		ofs->open(file.c_str(), mode);
		if (ofs->good() && ofs->is_open())
			return ofs;
		delete ofs;
		return NULL;
	}

	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->writable) {
			ofs->clear();
			ofs->open((d->path + file).c_str(), mode);
			if (ofs->good() && ofs->is_open())
				return ofs;
		}
	}
	delete ofs;
	return NULL;
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
	// if it's an absolute path, don't mkdir inside the data directories
	if (dir[0] == '/')
		return mkdir_helper(dir);

	return mkdir_helper(GetWriteDir() + dir);
}

/**
 * @brief removes a file from all writable data directories
 *
 * Returns true if at least one remove succeeded.
 */
bool UnixFileSystemHandler::remove(const std::string& file) const
{
	// if it's an absolute path, don't remove inside the data directories
	if (file[0] == '/')
		return ::remove(file.c_str()) == 0;

	return ::remove((GetWriteDir() + file).c_str()) == 0;
}
