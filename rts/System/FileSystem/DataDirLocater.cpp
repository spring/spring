/**
 * @file DataDirLocater.cpp
 * @author Tobi Vollebregt
 *
 * Copyright (C) 2006-2008 Tobi Vollebregt
 * Licensed under the terms of the GNU GPL, v2 or later
 */

#include "StdAfx.h"
#include "DataDirLocater.h"

#include <cstdlib>
#ifdef WIN32
	#include <io.h>
  #include <direct.h>
  #include <windows.h>
	#include <shlobj.h>
	#include <shlwapi.h>
	#ifndef SHGFP_TYPE_CURRENT
		#define SHGFP_TYPE_CURRENT 0
	#endif
#endif
#include <sstream>
#include <string.h>
#include "FileSystem/VFSHandler.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "FileSystem.h"
#include "mmgr.h"
#include "Exceptions.h"
#include "Platform/Misc.h"

/**
 * @brief construct a data directory object
 *
 * Appends a slash to the end of the path if there isn't one already.
 */
DataDir::DataDir(const std::string& p) : path(p), writable(false)
{
#ifndef _WIN32
	if (path.empty())
		path = "./";
	if (path[path.size() - 1] != '/')
		path += '/';
#else
	if (path.empty())
		path = ".\\";
	if (path[path.size() - 1] != '\\')
		path += '\\';
#endif
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
#ifndef _WIN32
				case '\\': {
					escape = true;
					break;
				}
#endif
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
#ifndef _WIN32
	while ((colon = in.find(':', prev_colon)) != std::string::npos) {
#else
	while ((colon = in.find(';', prev_colon)) != std::string::npos) {
#endif
		datadirs.push_back(in.substr(prev_colon, colon - prev_colon));
#ifdef DEBUG
		logOutput.Print("Adding %s to directories" , in.substr(prev_colon, colon - prev_colon).c_str());
#endif
		prev_colon = colon + 1;
	}
#ifdef DEBUG
	logOutput.Print("Adding %s to directories" , in.substr(prev_colon).c_str());
#endif
	datadirs.push_back(in.substr(prev_colon));
}

/**
 * @brief Figure out permissions we have for a single data directory d.
 * @returns whether we have permissions to read the data directory.
 */
bool DataDirLocater::DeterminePermissions(DataDir* d)
{
#ifndef _WIN32
	if (d->path.c_str()[0] != '/' || d->path.find("..") != std::string::npos)
#else
	if (d->path.find("..") != std::string::npos)
#endif
		throw content_error("specify data directories using absolute paths please");
	// Figure out whether we have read/write permissions
	// First check read access, if we got that check write access too
	// (no support for write-only directories)
	// Note: we check for executable bit otherwise we can't browse the directory
	// Note: we fail to test whether the path actually is a directory
	// Note: modifying permissions while or after this function runs has undefined behaviour
#ifndef _WIN32
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
	}
#else
	if (_access(d->path.c_str(), 4) == 0
			&& FileSystemHandler::GetInstance().DirIsWritable(d->path)) {
		if (!writedir) {
			d->writable = true;
			writedir = &*d;
		}
		return true;
	}
#endif
	else {
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
 * In Unixes, the data directory to chdir to is determined by the following, in this
 * order (first items override lower items):
 *
 * - 'SPRING_DATADIR' environment variable. (colon separated list, like PATH)
 * - 'SpringData=/path/to/data' declaration in '~/.springrc'. (colon separated list)
 * - "$HOME/.spring"
 * - In the same order any line in '/etc/spring/datadir', if that file exists.
 * - 'datadir=/path/to/data' option passed to 'scons configure'.
 * - 'prefix=/install/path' option passed to scons configure. The datadir is
 *   assumed to be at '$prefix/games/spring' in this case.
 * - the default datadirs in the default prefix, ie. '/usr/local/games/spring'
 *   (This is set by the build system, ie. SPRING_DATADIR and SPRING_DATADIR_2
 *   preprocessor definitions.)
 *
 * In Windows, its:
 * - SPRING_DATADIR env-variable
 * - user configurable (SpringData in registry)
 * - location of the binary dir (like it has been until 0.76b1)
 * - the Users 'Documents'-directory (in subdirectory Spring), unless spring is configured to use another
 * - all users app-data (in subdirectory Spring)
 * - compiler flags SPRING_DATADIR and SPRING_DATADIR_2
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

	// environment variable
	char* env = getenv("SPRING_DATADIR");
	if (env && *env)
		AddDirs(SubstEnvVars(env));

	// user defined (in spring config handler (Linux: ~/.springrc, Windows: registry))
	std::string userDef = configHandler->GetString("SpringData", "");
	if (!userDef.empty()) {
		AddDirs(SubstEnvVars(userDef));
	}

#ifdef WIN32
	// try current directory first, exe dir later
	TCHAR currentDir[MAX_PATH];
	::GetCurrentDirectory(sizeof(currentDir) - 1, currentDir);
	std::string curPath = currentDir;
	AddDirs(std::string(currentDir));
#endif

#ifndef UNITSYNC
	AddDirs(Platform::GetBinaryPath());
#endif

#ifdef WIN32
	// my documents
	TCHAR strPath[MAX_PATH];
	SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, strPath);
	std::string cfg = strPath;
	cfg += "\\Spring"; // e.g. F:\Dokumente und Einstellungen\Karl-Robert\Eigene Dateien\Spring
	AddDirs(cfg);
	cfg.clear();

	// appdata
	SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath);
	cfg = strPath;
	cfg += "\\Spring"; // e.g. F:\Dokumente und Einstellungen\All Users\Anwendungsdaten\Spring
	AddDirs(cfg);
// TODO: enable Mac OS X specific bundle code again
// #elif defined(__APPLE__)
	// // copied from old MacFileSystemHandler, won't compile here, but would not compile in its old location either
	// // needs fixing for new DataDirLocater-structure
	// // Get the path to the application bundle we are running:
	// char cPath[1024];
	// CFBundleRef mainBundle = CFBundleGetMainBundle();
	// if(!mainBundle)
		// throw content_error("Could not determine bundle path");

	// CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	// if(!mainBundleURL)
		// throw content_error("Could not determine bundle path");

	// CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
	// if(!cfStringRef)
		// throw content_error("Could not determine bundle path");

	// CFStringGetCString(cfStringRef, cPath, 1024, kCFStringEncodingASCII);

	// CFRelease(mainBundleURL);
	// CFRelease(cfStringRef);
	// std::string path(cPath);

	// datadirs.clear();
	// writedir = NULL;

	// // Add bundle resources:
	// datadirs.push_back(path + "/Contents/Resources/");
	// datadirs.rbegin()->readable = true;
	// // Add the directory surrounding the bundle, for users to add mods and maps in:
	// datadirs.push_back(filesystem.GetDirectory(path));
	// // Use surrounding directory as writedir for now, should propably
	// // change this to something inside the home directory:
	// datadirs.rbegin()->writable = true;
	// datadirs.rbegin()->readable = true;
	// writedir = &*datadirs.rbegin();
#else
	// home
	AddDirs(SubstEnvVars("$HOME/.spring"));

	// settings in /etc
	FILE* f = ::fopen("/etc/spring/datadir", "r");
	if (f) {
		char buf[1024];
		while (fgets(buf, sizeof(buf), f)) {
			char* newl = strchr(buf, '\n');
			if (newl)
				*newl = 0;
			char white[3] = {'\t', ' ', 0};
			if (strlen(buf) > 0 && strspn(buf, white) != strlen(buf)) // don't count lines of whitespaces / tabulators
				AddDirs(SubstEnvVars(buf));
		}
		fclose(f);
	}
#endif

	// compiler flags
#ifdef SPRING_DATADIR
	AddDirs(SubstEnvVars(SPRING_DATADIR));
#endif

	// Figure out permissions of all datadirs
	DeterminePermissions();

	if (!writedir) {
		// bail out
#ifdef WIN32
		const std::string errstr = "Not a single writable data directory found!\n\n"
				"Configure a writable data directory using either:\n"
				"- the SPRING_DATADIR environment variable,\n"
				"- a SpringData=C:/path/to/data declaration in spring's registry entry or\n"
				"- by giving you write access to the installation directory";
#else
		const std::string errstr = "Not a single writable data directory found!\n\n"
				"Configure a writable data directory using either:\n"
				"- the SPRING_DATADIR environment variable,\n"
				"- a SpringData=/path/to/data declaration in ~/.springrc or\n"
				"- the configuration file /etc/spring/datadir";
#endif
		throw content_error(errstr);
	}

	// for now, chdir to the datadirectory as a safety measure:
	// all AIs still just assume it's ok to put their stuff in the current directory after all
	// Not only safety anymore, it's just easier if other code can safely assume that
	// writedir == current working directory
#ifndef _WIN32
	int err = chdir(GetWriteDir()->path.c_str());
	if (err)
		throw content_error("Could not chdir into SPRING_DATADIR");
#else
	_chdir(GetWriteDir()->path.c_str());
#endif
	// Initialize the log. Only after this moment log will be written to file.
	logOutput.Initialize();
	// Logging MAY NOT start before the chdir, otherwise the logfile ends up
	// in the wrong directory.
	// Update: now it actually may start before, log has preInitLog.
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->writable)
			logOutput.Print("Using read-write data directory: %s", d->path.c_str());
		else
			logOutput.Print("Using read-only  data directory: %s", d->path.c_str());
	}
}
