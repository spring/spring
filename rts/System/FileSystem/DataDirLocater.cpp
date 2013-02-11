/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#else
	#include <wordexp.h>
#endif

#include "System/Platform/Win/win32.h"
#include <sstream>
#include <cassert>
#include <string.h>

#include "System/Log/ILog.h"
#include "System/LogOutput.h"
#include "System/Config/ConfigHandler.h"
#include "FileSystem.h"
#include "CacheDir.h"
#include "System/Exceptions.h"
#include "System/maindefines.h" // for sPS, cPS, cPD
#include "System/Platform/Misc.h"

CONFIG(std::string, SpringData).defaultValue("")
		.description("List of addidional data-directories, separated by ';' on windows, ':' on other OSs")
		.readOnly(true);


DataDirLocater dataDirLocater;


DataDir::DataDir(const std::string& path)
	: path(path)
	, writable(false)
{
	FileSystem::EnsurePathSepAtEnd(this->path);
}

DataDirLocater::DataDirLocater()
	: isolationMode(false)
	, writeDir(NULL)
{
	UpdateIsolationModeByEnvVar();
}


void DataDirLocater::UpdateIsolationModeByEnvVar()
{
	isolationMode = false;
	isolationModeDir = "";

	const char* const envIsolation = getenv("SPRING_ISOLATED");
	if (envIsolation != NULL) {
		isolationMode = true;
		if (FileSystem::DirExists(SubstEnvVars(envIsolation))) {
			isolationModeDir = envIsolation;
		}
	}
}

const std::vector<DataDir>& DataDirLocater::GetDataDirs() const {

	assert(!dataDirs.empty());
	return dataDirs;
}

std::string DataDirLocater::SubstEnvVars(const std::string& in) const
{
	std::string out;
#ifdef _WIN32
	const size_t maxSize = 32 * 1024;
	char out_c[maxSize];
	ExpandEnvironmentStrings(in.c_str(), out_c, maxSize); // expands %HOME% etc.
	out = out_c;
#else
	wordexp_t pwordexp;
	wordexp(in.c_str(), &pwordexp, WRDE_NOCMD); // expands $FOO, ${FOO}, ~/, etc.
	if (pwordexp.we_wordc > 0) {
		out = pwordexp.we_wordv[0];
	}
	wordfree(&pwordexp);
#endif
	return out;
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
		throw content_error(std::string("a datadir may not be specified with a relative path: \"") + dataDir->path + "\"");
	}
	// Figure out whether we have read/write permissions
	// First check read access, if we got that, check write access too
	// (no support for write-only directories)
	// We check for the executable bit, because otherwise we can not browse the
	// directory.
	// FIXME: We fail to test whether the path actually is a directory
	// Modifying the permissions while or after this function runs has undefined
	// behaviour.
	if (FileSystem::DirExists(dataDir->path))
	{
		if (!writeDir && FileSystem::DirIsWritable(dataDir->path))
		{
			dataDir->writable = true;
			writeDir = dataDir;
		}
		return true;
	}
	else if (!writeDir) // if there is already a rw data directory, do not create new folder for read-only locations
	{
		if (FileSystem::CreateDirectory(dataDir->path))
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
	const std::string curWorkDirParent = FileSystem::GetParent(curWorkDir);

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
#if       defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetModulePath();
#else  // defined(UNITSYNC)
	const std::string dd_curWorkDir = Platform::GetProcessExecutablePath();
#endif // defined(UNITSYNC)

	// Detect some useful dirs
#if    defined(WIN32)
	// fetch my documents path
	TCHAR pathMyDocsC[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, pathMyDocsC);
	const std::string pathMyDocs = pathMyDocsC;

	// e.g. F:\Dokumente und Einstellungen\Karl-Robert\Eigene Dateien\Spring
	const std::string dd_myDocs = pathMyDocs + "\\Spring";

	// My Documents\My Games seems to be the MS standard even if no official guidelines exist
	// most if not all new Games For Windows(TM) games use this dir
	const std::string dd_myDocsMyGames = pathMyDocs + "\\My Games\\Spring";

	// fetch common app-data path
	TCHAR pathAppDataC[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathAppDataC);
	const std::string pathAppData = pathAppDataC;

	// e.g. F:\Dokumente und Einstellungen\All Users\Anwendungsdaten\Spring
	const std::string dd_appData = pathAppData + "\\Spring";
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
	// Note: The first dir added will be the writable data dir!
	dataDirs.clear();

	if (isolationMode) {
		if (isolationModeDir.empty()) {
			AddCwdOrParentDir(dd_curWorkDir, true); // "./" or "../"
		} else {
			if (FileSystem::DirExists(isolationModeDir)) {
				AddDir(isolationModeDir);
			} else {
				throw user_error(std::string("The specified isolation-mode directory does not exist: ") + isolationModeDir);
			}
		}
	}

	// Same on all platforms
	{
		const char* env = getenv("SPRING_DATADIR");
		if (env && *env) {
			AddDirs(SubstEnvVars(env)); // ENV{SPRING_DATADIR}
		}
	}
	AddDirs(SubstEnvVars(configHandler->GetString("SpringData"))); // user defined in spring config (Linux: ~/.springrc, Windows: .\springsettings.cfg)

	if (!isolationMode) {
		if (!isolationModeDir.empty()) {
			LOG_L(L_WARNING, "Isolation directory was specified, but isolation mode is not active.");
		}

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
		const std::string bundleResourceDir = FileSystem::GetParent(dd_curWorkDir);

		// This has to correspond with the value in the build-script
		const std::string dd_curWorkDirData = bundleResourceDir + "/share/games/spring";

		// we need this as default writable dir, because the Bundle.pp dir
		// might not be writable by the user starting the game
		AddDirs(Platform::GetUserDir() + "/.spring"); // "~/.spring/"
		AddDirs(dd_curWorkDirData);             // "Spring.app/Contents/Resources/share/games/spring"
		AddDirs(dd_etc);                        // from /etc/spring/datadir FIXME add in IsolatedMode too?

#else
		// Linux, FreeBSD, Solaris, Apple non-bundle

		AddCwdOrParentDir(dd_curWorkDir); // "./" or "../"
		AddDirs(Platform::GetUserDir() + "/.spring"); // "~/.spring/"
		AddDirs(dd_etc);            // from /etc/spring/datadir FIXME add in IsolatedMode too?
#endif


#ifdef SPRING_DATADIR
		//Note: using the defineflag SPRING_DATADIR & "SPRING_DATADIR" as string works fine, the preprocessor won't touch the 2nd
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
	FileSystem::ChDir(GetWriteDir()->path.c_str());

	// Initialize the log. Only after this moment log will be written to file.
	// Note: Logging MAY NOT start before the chdir, otherwise the logfile ends up
	//       in the wrong directory.
	// Update: now it actually may start before, log has preInitLog.
	logOutput.Initialize();

	for (std::vector<DataDir>::const_iterator d = dataDirs.begin(); d != dataDirs.end(); ++d) {
		if (d->writable) {
			LOG("Using read-write data directory: %s", d->path.c_str());

			// tag the cache dir
			const std::string cacheDir = d->path + "cache";
			if (FileSystem::CreateDirectory(cacheDir)) {
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
	if (FileSystem::FileExists(fileExe)) {
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
	if (FileSystem::FileExists(fileUnitsync)) {
		portableMode = true;
	}
#endif // defined(UNITSYNC)

	return portableMode;
}

bool DataDirLocater::LooksLikeMultiVersionDataDir(const std::string& dirPath) {

	bool looksLikeDataDir = false;

	if (FileSystem::DirExists(dirPath + "/maps")
			&& FileSystem::DirExists(dirPath + "/games")
			&& FileSystem::DirExists(dirPath + "/engines")
			/*&& FileSystem::DirExists(dirPath + "/unitsyncs") TODO uncomment this if the new name for unitsync has been set */)
	{
		looksLikeDataDir = true;
	}

	return looksLikeDataDir;
}


std::string DataDirLocater::GetWriteDirPath() const
{
	const DataDir* writedir = GetWriteDir();
	assert(writedir && writedir->writable); // duh
	return writedir->path;
}

std::vector<std::string> DataDirLocater::GetDataDirPaths() const
{
	assert(!dataDirs.empty());
	std::vector<std::string> dataDirPaths;

	const std::vector<DataDir>& datadirs = GetDataDirs();
	std::vector<DataDir>::const_iterator ddi;
	for (ddi = datadirs.begin(); ddi != datadirs.end(); ++ddi) {
		dataDirPaths.push_back(ddi->path);
	}

	return dataDirPaths;
}
