/**
 * @file DataDirLocater.cpp
 * @author Tobi Vollebregt
 *
 * Copyright (C) 2006-2008 Tobi Vollebregt
 * Licensed under the terms of the GNU GPL, v2 or later
 */

#include "StdAfx.h"
#include "DataDirLocater.h"
#include <sstream>
#include <string.h>
#include "System/FileSystem/VFSHandler.h"
#include "System/LogOutput.h"
#include "System/Platform/ConfigHandler.h"
#include "System/Platform/FileSystem.h"
#include "mmgr.h"

/**
 * @brief construct a data directory object
 *
 * Appends a slash to the end of the path if there isn't one already.
 */
DataDir::DataDir(const std::string& p) : path(p), writable(false)
{
	if (path.empty())
		path = "./";
	if (path[path.size() - 1] != '/')
		path += '/';
}

/**
 * @brief construct a data directory locater
 *
 * Does not locate data directories, use LocateDataDirs() for that.
 */
DataDirLocater::DataDirLocater() : writedir(NULL)
{
}

/**
 * @brief substitute environment variables with their values
 */
std::string DataDirLocater::SubstEnvVars(const std::string& in) const
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
void DataDirLocater::AddDirs(const std::string& in)
{
	size_t prev_colon = 0, colon;
	while ((colon = in.find(':', prev_colon)) != std::string::npos) {
		datadirs.push_back(in.substr(prev_colon, colon - prev_colon));
		prev_colon = colon + 1;
	}
	datadirs.push_back(in.substr(prev_colon));
}

/**
 * @brief Figure out permissions we have for a single data directory d.
 * @returns whether we have permissions to read the data directory.
 */
bool DataDirLocater::DeterminePermissions(DataDir* d)
{
	if (d->path.c_str()[0] != '/' || d->path.find("..") != std::string::npos)
		throw content_error("specify data directories using absolute paths please");
	// Figure out whether we have read/write permissions
	// First check read access, if we got that check write access too
	// (no support for write-only directories)
	// Note: we check for executable bit otherwise we can't browse the directory
	// Note: we fail to test whether the path actually is a directory
	// Note: modifying permissions while or after this function runs has undefined behaviour
	if (access(d->path.c_str(), R_OK | X_OK | F_OK) == 0) {
		// Note: disallow multiple write directories.
		// There isn't really a use for it as every thing is written to the first one anyway,
		// and it may give funny effects on errors, e.g. it probably only gives funny things
		// like network mounted datadir lost connection and suddenly files end up in some
		// other random writedir you didn't even remember you had added it.
		if (!writedir && access(d->path.c_str(), W_OK) == 0) {
			d->writable = true;
			writedir = &*d;
		}
		return true;
	} else {
		if (filesystem.CreateDirectory(d->path)) {
			// it didn't exist before, now it does and we just created it with rw access,
			// so we just assume we still have read-write acces...
			if (!writedir) {
				d->writable = true;
				writedir = d;
			}
			return true;
		}
	}
	return false;
}

/**
 * @brief Figure out permissions we have for the data directories.
 */
void DataDirLocater::DeterminePermissions()
{
	std::vector<DataDir> newDatadirs;
	std::string previous; // used to filter out consecutive duplicates
	// (I didn't bother filtering out non-consecutive duplicates because then
	//  there is the question which of the multiple instances to purge.)

	writedir = NULL;

	for (std::vector<DataDir>::iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->path != previous && DeterminePermissions(&*d)) {
			newDatadirs.push_back(*d);
			previous = d->path;
		}
	}

	datadirs = newDatadirs;
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
 * - 'SPRING_DATADIR' environment variable. (colon separated list, like PATH)
 * - 'SpringData=/path/to/data' declaration in '~/.springrc'. (colon separated list)
 *   This defaults to "$HOME/.spring"
 * - In the same order any line in '/etc/spring/datadir', if that file exists.
 * - 'datadir=/path/to/data' option passed to 'scons configure'.
 * - 'prefix=/install/path' option passed to scons configure. The datadir is
 *   assumed to be at '$prefix/games/spring' in this case.
 * - the default datadirs in the default prefix, ie. '/usr/local/games/spring'
 *   (This is set by the build system, ie. SPRING_DATADIR and SPRING_DATADIR_2
 *   preprocessor definitions.)
 *
 * All of the above methods support environment variable substitution, eg.
 * '$HOME/myspringdatadir' will be converted by spring to something like
 * '/home/username/myspringdatadir'.
 *
 * If it fails to chdir to the above specified directory spring will asume the
 * current working directory is the data directory.
 */
void DataDirLocater::LocateDataDirs()
{
	// Construct the list of datadirs from various sources.
	datadirs.clear();

	char* env = getenv("SPRING_DATADIR");
	if (env && *env)
		AddDirs(SubstEnvVars(env));

	std::string cfg = configHandler.GetString("SpringData", "$HOME/.spring");
	if (!cfg.empty()) {
		AddDirs(SubstEnvVars(cfg));
	}

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

#ifdef SPRING_DATADIR
	datadirs.push_back(SubstEnvVars(SPRING_DATADIR));
#endif
#ifdef SPRING_DATADIR_2
	datadirs.push_back(SubstEnvVars(SPRING_DATADIR_2));
#endif

	// Figure out permissions of all datadirs
	DeterminePermissions();

	if (!writedir) {
		// bail out
		throw content_error("Not a single writable data directory found!\n\n"
				"Configure a writable data directory using either:\n"
				"- the SPRING_DATADIR environment variable,\n"
				"- a SpringData=/path/to/data declaration in ~/.springrc or\n"
				"- the configuration file /etc/spring/datadir");
	}

	// for now, chdir to the datadirectory as a safety measure:
	// all AIs still just assume it's ok to put their stuff in the current directory after all
	// Not only safety anymore, it's just easier if other code can safely assume that
	// writedir == current working directory
	chdir(GetWriteDir()->path.c_str());

	// Logging MAY NOT start before the chdir, otherwise the logfile ends up
	// in the wrong directory.
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->writable)
			logOutput.Print("Using read-write data directory: %s", d->path.c_str());
		else
			logOutput.Print("Using read-only  data directory: %s", d->path.c_str());
	}
}
