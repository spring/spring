/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

#include "LogOutput.h"
#include "ConfigHandler.h"
#include "FileSystemHandler.h"
#include "FileSystem.h"
#include "CacheDir.h"
#include "mmgr.h"
#include "Exceptions.h"
#include "maindefines.h" // for sPS, cPS, cPD
#include "Platform/Misc.h"

DataDir::DataDir(const std::string& p) : path(p), writable(false)
{
	FileSystemHandler::EnsurePathSepAtEnd(path);
}

DataDirLocater::DataDirLocater() : writedir(NULL)
{
}

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

void DataDirLocater::AddDirs(const std::string& in)
{
	size_t prev_colon = 0, colon;
	while ((colon = in.find(cPD, prev_colon)) != std::string::npos) { // cPD (depending on OS): ';' or ':'
		AddDir(in.substr(prev_colon, colon - prev_colon));
		prev_colon = colon + 1;
	}
	AddDir(in.substr(prev_colon));
}

void DataDirLocater::AddDir(const std::string& dir)
{
	if (!dir.empty()) {
		// to make use of ensure-slash-at-end,
		// we create a DataDir here already
		const DataDir newDataDir(dir);
		bool alreadyAdded = false;

		std::vector<DataDir>::const_iterator ddi;
		for (ddi = datadirs.begin(); ddi != datadirs.end(); ++ddi) {
			if (newDataDir.path == ddi->path) {
				alreadyAdded = true;
				break;
			}
		}

		if (!alreadyAdded) {
			datadirs.push_back(newDataDir);
#ifdef DEBUG
			logOutput.Print("Adding %s to directories", newDataDir.path.c_str());
		} else {
			logOutput.Print("Skipping already added directory %s", newDataDir.path.c_str());
#endif
		}
	}
}

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

#if       defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetModulePath();
#else  // defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetProcessExecutablePath();
#endif // defined(UNITSYNC)

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
	const std::string dd_curWorkDirData = dd_curWorkDir + "/" + SubstEnvVars(DATADIR);
	const std::string dd_curWorkDirLib  = dd_curWorkDir + "/" + SubstEnvVars(LIBDIR);
#else // *nix (-OSX)
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
					// remove the new line char
					*newLineCharPos = '\0';
				}
				// ignore lines consisting of only whitespaces
				if ((strlen(lineBuf) > 0) && strspn(lineBuf, whiteSpaces) != strlen(lineBuf)) {
					// append, separated by sPD (depending on OS): ';' or ':'
					dd_etc = dd_etc + (dd_etc.empty() ? "" : sPD) + SubstEnvVars(lineBuf);
				}
			}
			fclose(fileH);
		}
	}
#endif // defined(WIN32), defined(MACOSX_BUNDLE), else

	// Construct the list of datadirs from various sources.
	datadirs.clear();
	// The first dir added will be the writeable data dir.

	// same on all platforms
	AddDirs(dd_env);    // ENV{SPRING_DATADIR}
	// user defined in spring config handler
	// (Linux: ~/.springrc, Windows: .\springsettings.cfg)
	AddDirs(SubstEnvVars(configHandler->GetString("SpringData", "")));

#ifdef WIN32
	// All MS Windows variants

	if (IsPortableMode()) {
		AddDirs(dd_curWorkDir); // "./"
	}
	AddDirs(dd_myDocsMyGames);  // "C:/.../My Documents/My Games/Spring/"
	AddDirs(dd_myDocs);         // "C:/.../My Documents/Spring/"
	AddDirs(dd_appData);        // "C:/.../All Users/Applications/Spring/"

#elif defined(MACOSX_BUNDLE)
	// Mac OS X

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

#else
	// Linux, FreeBSD, Solaris, Apple non-bundle

	if (IsPortableMode()) {
		// always using this would be unclean, because spring and unitsync
		// would end up with different sets of data-dirs
		AddDirs(dd_curWorkDir); // "./"
	}
	AddDirs(SubstEnvVars("$HOME/.spring")); // "~/.spring/"
	AddDirs(dd_etc);            // from /etc/spring/datadir
#endif

#ifdef SPRING_DATADIR
	AddDirs(SubstEnvVars(SPRING_DATADIR)); // from -DSPRING_DATADIR
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

			// tag the cache dir
			const std::string cacheDir = d->path + "cache";
			if (filesystem.CreateDirectory(cacheDir)) {
				CacheDir::SetCacheDir(cacheDir, true);
			}
		} else {
			logOutput.Print("Using read-only data directory: %s",  d->path.c_str());
		}
	}
}

bool DataDirLocater::IsPortableMode() {

	bool portableMode = false;

#if       defined(UNITSYNC)
	const std::string dirUnitsync = Platform::GetModulePath();

#if       defined(WIN32)
	std::string fileExe = dirUnitsync + "\\spring.exe";
#else
	std::string fileExe = dirUnitsync + "/spring";
#endif // defined(WIN32)
	if (FileSystemHandler::FileExists(fileExe)) {
		portableMode = true;
	}

#else  // !defined(UNITSYNC)
	const std::string dirExe = Platform::GetProcessExecutablePath();

#if       defined(WIN32)
	std::string fileUnitsync = dirExe + "\\unitsync.dll";
#elif     defined(__APPLE__)
	std::string fileUnitsync = dirExe + "/libunitsync.dylib";
#else
	std::string fileUnitsync = dirExe + "/libunitsync.so";
#endif // defined(WIN32)
	if (FileSystemHandler::FileExists(fileUnitsync)) {
		portableMode = true;
	}
#endif // defined(UNITSYNC)

	return portableMode;
}
