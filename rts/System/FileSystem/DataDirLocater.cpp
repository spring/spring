/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
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

#include "System/Log/ILog.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"
#include "FileSystemHandler.h"
#include "FileSystem.h"
#include "CacheDir.h"
#include "System/mmgr.h"
#include "System/Exceptions.h"
#include "System/maindefines.h" // for sPS, cPS, cPD
#include "System/Platform/Misc.h"

DataDir::DataDir(const std::string& path)
	: path(path)
	, writable(false)
{
	FileSystemHandler::EnsurePathSepAtEnd(this->path);
}

DataDirLocater::DataDirLocater()
	: writeDir(NULL)
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

void DataDirLocater::AddDirs(const std::string& dirs)
{
	size_t prev_colon = 0;
	size_t colon;
	while ((colon = dirs.find(cPD, prev_colon)) != std::string::npos) { // cPD (depending on OS): ';' or ':'
		AddDir(dirs.substr(prev_colon, colon - prev_colon));
		prev_colon = colon + 1;
	}
	AddDir(dirs.substr(prev_colon));
}

void DataDirLocater::AddDir(const std::string& dir)
{
	if (!dir.empty()) {
		// to make use of ensure-slash-at-end,
		// we create a DataDir here already
		const DataDir newDataDir(dir);
		bool alreadyAdded = false;

		std::vector<DataDir>::const_iterator ddi;
		for (ddi = dataDirs.begin(); ddi != dataDirs.end(); ++ddi) {
			if (newDataDir.path == ddi->path) {
				alreadyAdded = true;
				break;
			}
		}

		if (!alreadyAdded) {
			dataDirs.push_back(newDataDir);
			LOG_L(L_DEBUG, "Adding %s to directories", newDataDir.path.c_str());
		} else {
			LOG_L(L_DEBUG, "Skipping already added directory %s", newDataDir.path.c_str());
		}
	}
}

bool DataDirLocater::DeterminePermissions(DataDir* dataDir)
{
#ifndef _WIN32
	if ((dataDir->path.c_str()[0] != '/') || (dataDir->path.find("..") != std::string::npos))
#else
	if (dataDir->path.find("..") != std::string::npos)
#endif
	{
		throw content_error(std::string("Error: datadir specified with relative path: \"") + dataDir->path + "\""); // FIXME remove "Error: " prefix
	}
	// Figure out whether we have read/write permissions
	// First check read access, if we got that, check write access too
	// (no support for write-only directories)
	// We check for the executable bit, because otherwise we can not browse the
	// directory.
	// FIXME: We fail to test whether the path actually is a directory
	// Modifying the permissions while or after this function runs has undefined
	// behaviour.
	if (FileSystemHandler::GetInstance().DirExists(dataDir->path))
	{
		if (!writeDir && FileSystemHandler::GetInstance().DirIsWritable(dataDir->path))
		{
			dataDir->writable = true;
			writeDir = dataDir;
		}
		return true;
	}
	else if (!writeDir) // if there is already a rw data directory, do not create new folder for read-only locations
	{
		if (filesystem.CreateDirectory(dataDir->path))
		{
			// it did not exist before, now it does and we just created it with
			// rw access, so we just assume we still have read-write access ...
			dataDir->writable = true;
			writeDir = dataDir;
			return true;
		}
	}
	return false;
}

void DataDirLocater::DeterminePermissions()
{
	std::vector<DataDir> newDatadirs;
	std::string previous; // used to filter out consecutive duplicates
	// (I did not bother filtering out non-consecutive duplicates because then
	//  there is the question which of the multiple instances to purge.)

	writeDir = NULL;

	for (std::vector<DataDir>::iterator d = dataDirs.begin(); d != dataDirs.end(); ++d) {
		if ((d->path != previous) && DeterminePermissions(&*d)) {
			newDatadirs.push_back(*d);
			previous = d->path;
		}
	}

	dataDirs = newDatadirs;
}

void DataDirLocater::AddCwdOrParentDir(const std::string& curWorkDir, bool forceAdd)
{
	// This is useful in case of multiple engine/unitsync versions installed
	// together in a sub-dir of the data-dir
	// The data-dir structure then might look similar to this:
	// maps/
	// games/
	// engines/engine-0.83.0.0.exe
	// engines/engine-0.83.1.0.exe
	// unitsyncs/unitsync-0.83.0.0.exe
	// unitsyncs/unitsync-0.83.1.0.exe
	const std::string curWorkDirParent = FileSystemHandler::GetParent(curWorkDir);

	// we can not add both ./ and ../ as data-dir
	if ((curWorkDirParent != "") && LooksLikeMultiVersionDataDir(curWorkDirParent)) {
		AddDirs(curWorkDirParent); // "../"
	} else if (IsPortableMode() || forceAdd) {
		// always using this would be unclean, because spring and unitsync
		// would end up with different sets of data-dirs
		AddDirs(curWorkDir); // "./"
	}
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

	// If this is true, ie the var is present in env, we will only add the dir
	// where both binary and unitysnc lib reside in Portable mode,
	// or the parent dir, if it is a versioned data-dir.
	const bool isolationMode = (getenv("SPRING_ISOLATED") != NULL);

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

	// Construct the list of dataDirs from various sources.
	dataDirs.clear();
	// The first dir added will be the writeable data dir.

	if (isolationMode) {
		AddCwdOrParentDir(dd_curWorkDir, true); // "./" or "../"
	} else {
		// same on all platforms
		AddDirs(dd_env);    // ENV{SPRING_DATADIR}
		// user defined in spring config handler
		// (Linux: ~/.springrc, Windows: .\springsettings.cfg)
		AddDirs(SubstEnvVars(configHandler->GetString("SpringData", "")));

#ifdef WIN32
		// All MS Windows variants

		AddCwdOrParentDir(dd_curWorkDir); // "./" or "../"
		AddDirs(dd_myDocsMyGames);  // "C:/.../My Documents/My Games/Spring/"
		AddDirs(dd_myDocs);         // "C:/.../My Documents/Spring/"
		AddDirs(dd_appData);        // "C:/.../All Users/Applications/Spring/"

#elif defined(MACOSX_BUNDLE)
		// Mac OS X Application Bundle (*.app) - single file install

		// directory structure (Apple standard):
		// Spring.app/Contents/MacOS/springlobby
		// Spring.app/Contents/Resources/bin/spring
		// Spring.app/Contents/Resources/lib/unitsync.dylib
		// Spring.app/Contents/Resources/share/games/spring/base/

		// This corresponds to Spring.app/Contents/Resources/
		const std::string bundleResourceDir = FileSystemHandler::GetParent(dd_curWorkDir);

		// This has to correspond with the value in the build-script
		const std::string dd_curWorkDirData = bundleResourceDir + "/share/games/spring";

		// we need this as default writeable dir, because the Bundle.pp dir
		// might not be writeable by the user starting the game
		AddDirs(SubstEnvVars("$HOME/.spring")); // "~/.spring/"
		AddDirs(dd_curWorkDirData);             // "Spring.app/Contents/Resources/share/games/spring"
		AddDirs(dd_etc);                        // from /etc/spring/datadir

#else
		// Linux, FreeBSD, Solaris, Apple non-bundle

		AddCwdOrParentDir(dd_curWorkDir); // "./" or "../"
		AddDirs(SubstEnvVars("$HOME/.spring")); // "~/.spring/"
		AddDirs(dd_etc);            // from /etc/spring/datadir
#endif

#ifdef SPRING_DATADIR
		AddDirs(SubstEnvVars(SPRING_DATADIR)); // from -DSPRING_DATADIR
#endif
	}

	// Figure out permissions of all dataDirs
	DeterminePermissions();

	if (!writeDir) {
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
	// writeDir == current working directory
	FileSystemHandler::GetInstance().Chdir(GetWriteDir()->path.c_str());

	// Initialize the log. Only after this moment log will be written to file.
	logOutput.Initialize();
	// Logging MAY NOT start before the chdir, otherwise the logfile ends up
	// in the wrong directory.
	// Update: now it actually may start before, log has preInitLog.
	for (std::vector<DataDir>::const_iterator d = dataDirs.begin(); d != dataDirs.end(); ++d) {
		if (d->writable) {
			LOG("Using read-write data directory: %s", d->path.c_str());

			// tag the cache dir
			const std::string cacheDir = d->path + "cache";
			if (filesystem.CreateDirectory(cacheDir)) {
				CacheDir::SetCacheDir(cacheDir, true);
			}
		} else {
			LOG("Using read-only data directory: %s",  d->path.c_str());
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

bool DataDirLocater::LooksLikeMultiVersionDataDir(const std::string& dirPath) {

	bool looksLikeDataDir = false;

	if (FileSystemHandler::DirExists(dirPath + "/maps")
			&& FileSystemHandler::DirExists(dirPath + "/games")
			&& FileSystemHandler::DirExists(dirPath + "/engines")
			/*&& FileSystemHandler::DirExists(dirPath + "/unitsyncs") TODO uncomment this if the new name for unitsync has been set */)
	{
		looksLikeDataDir = true;
	}

	return looksLikeDataDir;
}
