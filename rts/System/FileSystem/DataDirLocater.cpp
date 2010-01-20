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
#include "FileSystemHandler.h"
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
		const std::string newPath = in.substr(prev_colon, colon - prev_colon);
		if (!newPath.empty())
		{
			datadirs.push_back(newPath);
#ifdef DEBUG
			logOutput.Print("Adding %s to directories" , newPath.c_str());
#endif
		}
		prev_colon = colon + 1;
	}
	const std::string newPath = in.substr(prev_colon);
	if (!newPath.empty())
	{
		datadirs.push_back(newPath);
#ifdef DEBUG
		logOutput.Print("Adding %s to directories" , newPath.c_str());
#endif
	}
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
	{
		throw content_error(std::string("Error: datadir specified with relative path: \"")+d->path+"\"");
	}
	// Figure out whether we have read/write permissions
	// First check read access, if we got that check write access too
	// (no support for write-only directories)
	// Note: we check for executable bit otherwise we can't browse the directory
	// Note: we fail to test whether the path actually is a directory
	// Note: modifying permissions while or after this function runs has undefined behaviour
	if (FileSystemHandler::GetInstance().DirExists(d->path))
	{
		if (!writedir && FileSystemHandler::GetInstance().DirIsWritable(d->path))
		{
			d->writable = true;
			writedir = &*d;
		}
		return true;
	}
	else if (!writedir) // if there is already a rw data directory, do not create new folder for read-only locations
	{
		if (filesystem.CreateDirectory(d->path))
		{
			// it didn't exist before, now it does and we just created it with rw access,
			// so we just assume we still have read-write acces...
			d->writable = true;
			writedir = d;
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
 * @brief locate spring data directories
 *
 * Attempts to locate a writeable data dir, and then tries to
 * chdir to it.
 * As the writeable data dir will usually be the current dir already under windows,
 * the chdir will have no effect.
 *
 * The first dir added will be the writeable data dir.
 *
 * How the dirs get assembled
 * --------------------------
 * (descending priority -> first entry is searched first)
 *
 * Windows:
 * - SPRING_DATADIR env-variable (semi-colon separated list, like PATH)
 * - ./springsettings.cfg:SpringData=C:\data (semi-colon separated list)
 * - path to the current work-dir/module (either spring.exe or unitsync.dll)
 * - "C:/.../My Documents/My Games/Spring/"
 * - "C:/.../My Documents/Spring/"
 * - "C:/.../All Users/Applications/Spring/"
 * - SPRING_DATADIR compiler flag (semi-colon separated list)
 *
 * Max OS X:
 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
 * - path to the current work-dir/module (either spring(binary) or libunitsync.dylib)
 * - {module-path}/data/
 * - {module-path}/lib/
 * - SPRING_DATADIR compiler flag (colon separated list)
 *
 * Unixes:
 * - SPRING_DATADIR env-variable (colon separated list, like PATH)
 * - ~/.springrc:SpringData=/path/to/data (colon separated list)
 * - "$HOME/.spring"
 * - from file '/etc/spring/datadir', preserving order (new-line separated list)
 * - SPRING_DATADIR compiler flag (colon separated list)
 *   This is set by the build system, and will usually contain dirs like:
 *   * /usr/share/games/spring/
 *   * /usr/lib/
 *   * /usr/lib64/
 *   * /usr/share/lib/
 *
 * All of the above methods support environment variable substitution, eg.
 * '$HOME/myspringdatadir' will be converted by spring to something like
 * '/home/username/myspringdatadir'.
 *
 * If we end up with no data-dir that points to an existing path,
 * we asume the current working directory is the data directory.
 */
void DataDirLocater::LocateDataDirs()
{
	// Prepare the data-dirs defined in different places

	// environment variable
	std::string dd_env = "";
	{
		char* env = getenv("SPRING_DATADIR");
		if (env && *env) {
			dd_env = SubstEnvVars(env);
		}
	}

	// user defined in spring config handler
	// (Linux: ~/.springrc, Windows: .\springsettings.cfg)
	const std::string dd_config = SubstEnvVars(configHandler->GetString("SpringData", ""));

	// compiler flag
	std::string dd_compilerFlag = "";
#ifdef SPRING_DATADIR
	dd_compilerFlag = SubstEnvVars(SPRING_DATADIR);
#endif



#if       defined(WIN32) || defined(MACOSX_BUNDLE)
#if       defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetModulePath();
#else  // defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetProcessExecutablePath();
#endif // defined(UNITSYNC)
#endif // defined(WIN32) || defined(MACOSX_BUNDLE)

#if    defined(WIN32)
	// fetch my documents path
	TCHAR pathMyDocs[MAX_PATH];
	SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, pathMyDocs);

	// fetch app-data path
	TCHAR pathAppData[MAX_PATH];
	SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathAppData);

	std::string dd_myDocs = pathMyDocs;
	// e.g. F:\Dokumente und Einstellungen\Karl-Robert\Eigene Dateien\Spring
	dd_myDocs += "\\Spring";

	std::string dd_myDocsMyGames = pathMyDocs;
	// My Documents\My Games seems to be the MS standard even if no official guidelines exist
	// most if not all new Games For Windows(TM) games use this dir
	dd_myDocsMyGames += "\\My Games\\Spring";

	std::string dd_appData = pathAppData;
	// e.g. F:\Dokumente und Einstellungen\All Users\Anwendungsdaten\Spring
	dd_appData += "\\Spring";
#elif     defined(MACOSX_BUNDLE)
	const std::string dd_curWorkDirData = dd_curWorkDir + "/" + SubstEnvVars(DATADIR));
	const std::string dd_curWorkDirLib  = dd_curWorkDir + "/" + SubstEnvVars(LIBDIR));
#else // *nix (-OSX)
	const std::string dd_home = SubstEnvVars("$HOME/.spring");

	// settings in /etc
	std::string dd_etc = "";
	{
		FILE* fileH = ::fopen("/etc/spring/datadir", "r");
		if (fileH) {
			const char whiteSpaces[3] = {'\t', ' ', '\0'};
			char lineBuf[1024];
			while (fgets(lineBuf, sizeof(lineBuf), fileH)) {
				char* newLineCharPos = strchr(lineBuf, '\n');
				if (newLineCharPos) {
					// end the string at the  it an empty string
					*newLineCharPos = '\0';
				}
				// ignore lines consisting of only whitespaces
				if ((strlen(lineBuf) > 0) && strspn(lineBuf, whiteSpaces) != strlen(lineBuf)) {
					dd_etc = dd_etc + " " + SubstEnvVars(lineBuf);
				}
			}
			fclose(fileH);
		}
	}
#endif // defined(WIN32), defined(MACOSX_BUNDLE), else



	// Construct the list of datadirs from various sources.
	datadirs.clear();
	// The first dir added will be the writeable data dir.

#ifdef WIN32
	// All MS Windows variants

	AddDirs(dd_env);           // from ENV{SPRING_DATADIR}
	AddDirs(dd_config);        // from ./springsettings.cfg:SpringData=...
	AddDirs(dd_curWorkDir);    // "./"
	AddDirs(dd_myDocsMyGames); // "C:/.../My Documents/My Games/Spring/"
	AddDirs(dd_myDocs);        // "C:/.../My Documents/Spring/"
	AddDirs(dd_appData);       // "C:/.../All Users/Applications/Spring/"
	AddDirs(dd_compilerFlag);  // from -DSPRING_DATADIR

#elif defined(MACOSX_BUNDLE)
	// Mac OS X

	AddDirs(dd_env);    // ENV{SPRING_DATADIR}
	AddDirs(dd_config); // ~/springrc:SpringData=...

	// Maps and mods are supposed to be located in spring's executable location on Mac, but unitsync
	// cannot find them since it does not know spring binary path. I have no idea but to force users 
	// to locate lobby executables in the same as spring's dir and add its location to search dirs.
	#ifdef UNITSYNC
	AddDirs(dd_curWorkDir);     // "./"
	#endif

	// libs and data are supposed to be located in subdirectories of spring executable, so they
	// sould be added instead of SPRING_DATADIR definition.
	AddDirs(dd_curWorkDirData); // "./data/"
	AddDirs(dd_curWorkDirLib);  // "./lib/"
	AddDirs(dd_compilerFlag);  // from -DSPRING_DATADIR

#else
	// Linux, FreeBSD, Solaris

	AddDirs(dd_env);          // ENV{SPRING_DATADIR}
	AddDirs(dd_config);       // ~/springrc:SpringData=...
	AddDirs(dd_home);         // "~/.spring/"
	AddDirs(dd_etc);          // from /etc/spring/datadir
	AddDirs(dd_compilerFlag); // from -DSPRING_DATADIR
#endif



	// Figure out permissions of all datadirs
	DeterminePermissions();

	if (!writedir) {
		// bail out
		const std::string errstr = "Not a single writable data directory found!\n\n"
				"Configure a writable data directory using either:\n"
				"- the SPRING_DATADIR environment variable,\n"
#ifdef WIN32
				"- a SpringData=C:/path/to/data declaration in spring's config file ./springsettings.cfg\n"
				"- by giving you write access to the installation directory";
#else
				"- a SpringData=/path/to/data declaration in ~/.springrc or\n"
				"- the configuration file /etc/spring/datadir";
#endif
		throw content_error(errstr);
	}

	// for now, chdir to the data directory as a safety measure:
	// Not only safety anymore, it's just easier if other code can safely assume that
	// writedir == current working directory
	FileSystemHandler::GetInstance().Chdir(GetWriteDir()->path.c_str());

	// Initialize the log. Only after this moment log will be written to file.
	logOutput.Initialize();
	// Logging MAY NOT start before the chdir, otherwise the logfile ends up
	// in the wrong directory.
	// Update: now it actually may start before, log has preInitLog.
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->writable) {
			logOutput.Print("Using read-write data directory: %s", d->path.c_str());
		} else {
			logOutput.Print("Using read-only data directory: %s",  d->path.c_str());
		}
	}
}
