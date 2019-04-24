/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DataDirLocater.h"

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <sstream>

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

#include "CacheDir.h"
#include "FileSystem.h"
#include "System/Exceptions.h"
#include "System/MainDefines.h" // for sPS, cPS, cPD
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"
#include "System/SafeUtil.h"

CONFIG(std::string, SpringData)
	.defaultValue("")
	.description("List of additional data-directories, separated by ';' on Windows and ':' on other OSs")
	.readOnly(true);

CONFIG(std::string, SpringDataRoot)
	.defaultValue("")
	.description("Optional custom data-directory content root ('base', 'maps', ...) to scan for archives")
	.readOnly(true);


static inline std::string GetSpringBinaryName()
{
#if defined(WIN32)
	return "spring.exe";
#else
	return "spring";
#endif
}

static inline std::string GetUnitsyncLibName()
{
#if   defined(WIN32)
	return "unitsync.dll";
#elif defined(__APPLE__)
	return "libunitsync.dylib";
#else
	return "libunitsync.so";
#endif
}


static std::string GetBinaryLocation()
{
#if  defined(UNITSYNC)
	return Platform::GetModulePath();
#else
	return Platform::GetProcessExecutablePath();
#endif
}


static inline void SplitColonString(const std::string& str, const std::function<void(const std::string&)>& cbf)
{
	size_t prev_colon = 0;
	size_t colon;

	// cPD is ';' or ':' depending on OS
	while ((colon = str.find(cPD, prev_colon)) != std::string::npos) {
		cbf(str.substr(prev_colon, colon - prev_colon));
		prev_colon = colon + 1;
	}

	cbf(str.substr(prev_colon));
}



DataDir::DataDir(const std::string& path): path(path)
{
	FileSystem::EnsurePathSepAtEnd(this->path);
}


DataDirLocater::DataDirLocater()
{
	UpdateIsolationModeByEnvVar();
}


void DataDirLocater::UpdateIsolationModeByEnvVar()
{
	isolationMode = false;
	isolationModeDir = "";

	const char* const envIsolation = getenv("SPRING_ISOLATED");
	if (envIsolation != nullptr) {
		SetIsolationMode(true);
		SetIsolationModeDir(envIsolation);
		return;
	}

	const std::string dir = FileSystem::EnsurePathSepAtEnd(GetBinaryLocation());
	if (FileSystem::FileExists(dir + "isolated.txt")) {
		SetIsolationMode(true);
		SetIsolationModeDir(dir);
	}
}

const std::vector<DataDir>& DataDirLocater::GetDataDirs() const
{
	assert(!dataDirs.empty());
	return dataDirs;
}

std::string DataDirLocater::SubstEnvVars(const std::string& in) const
{
	std::string out;

#ifdef _WIN32
	constexpr size_t maxSize = 32 * 1024;
	char out_c[maxSize];
	ExpandEnvironmentStrings(in.c_str(), out_c, maxSize); // expands %HOME% etc.
	out = out_c;
#else
	std::string previous = in;

	for (int i = 0; i < 10; ++i) { // repeat substitution till we got a pure absolute path
		wordexp_t pwordexp;

		// expands $FOO, ${FOO}, ${FOO-DEF} ~/, etc.
		if (wordexp(previous.c_str(), &pwordexp, WRDE_NOCMD) == EXIT_SUCCESS) {
			if (pwordexp.we_wordc > 0) {
				out = pwordexp.we_wordv[0];

				for (unsigned int w = 1; w < pwordexp.we_wordc; ++w) {
					out += " ";
					out += pwordexp.we_wordv[w];
				}
			}
			wordfree(&pwordexp);
		} else {
			out = in;
		}

		if (previous == out)
			break;

		previous.swap(out);
	}
#endif
	return out;
}

void DataDirLocater::AddDirs(const std::string& dirs)
{
	if (dirs.empty())
		return;

	SplitColonString(dirs, [&](const std::string& dir) { AddDir(dir); });
}

void DataDirLocater::AddDir(const std::string& dir)
{
	if (dir.empty())
		return;

	// create DataDir here to ensure comparison includes trailing slash
	const DataDir newDataDir(SubstEnvVars(dir));

	const auto pred = [&](const DataDir& dd) { return (FileSystem::ComparePaths(newDataDir.path, dd.path)); };
	const auto iter = std::find_if(dataDirs.begin(), dataDirs.end(), pred);

	if (iter != dataDirs.end())
		return;

	dataDirs.push_back(newDataDir);
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

	return FileSystem::DirExists(dataDir->path);
}

void DataDirLocater::FilterUsableDataDirs()
{
	std::vector<DataDir> newDatadirs;
	std::string previous; // used to filter out consecutive duplicates
	// (I did not bother filtering out non-consecutive duplicates because then
	//  there is the question which of the multiple instances to purge.)

	for (auto& dd : dataDirs) {
		if (dd.path != previous) {
			if (DeterminePermissions(&dd)) {
				newDatadirs.push_back(dd);
				previous = dd.path;
				if (dd.writable) {
					LOG("[DataDirLocater::%s] using read-write data directory: %s", __func__, dd.path.c_str());
				} else {
					LOG("[DataDirLocater::%s] using read-only data directory: %s", __func__, dd.path.c_str());
				}
			} else {
				LOG_L(L_DEBUG, "[DataDirLocater::%s] potentional data directory: %s", __func__, dd.path.c_str());
			}
		}
	}

	dataDirs = newDatadirs;
}


bool DataDirLocater::IsWriteableDir(DataDir* dataDir)
{
	if (FileSystem::DirExists(dataDir->path))
		return FileSystem::DirIsWritable(dataDir->path);

	// it did not exist before, now it does and we just created it with
	// rw access, so we just assume we still have read-write access ...
	return FileSystem::CreateDirectory(dataDir->path);
}

void DataDirLocater::FindWriteableDataDir()
{
	writeDir = nullptr;

	for (DataDir& d: dataDirs) {
		if ((d.writable = IsWriteableDir(&d))) {
			writeDir = &d;
			break;
		}
	}

	LOG("[DataDirLocater::%s] using writeable data-directory \"%s\"", __func__, (writeDir != nullptr)? writeDir->path.c_str(): "<NULL>");
}


void DataDirLocater::AddCurWorkDir()
{
	AddDir(Platform::GetOrigCWD());
}

void DataDirLocater::AddPortableDir()
{
	const std::string dd_curWorkDir = GetBinaryLocation();

	// This is useful in case of multiple engine/unitsync versions installed
	// together in a sub-dir of the data-dir
	// The data-dir structure then might look similar to this:
	// maps/
	// games/
	// engines/engine-0.83.0.0.exe
	// engines/engine-0.83.1.0.exe
	// unitsyncs/unitsync-0.83.0.0.exe
	// unitsyncs/unitsync-0.83.1.0.exe
	const std::string curWorkDirParent = FileSystem::GetParent(dd_curWorkDir);

	if (!curWorkDirParent.empty() && LooksLikeMultiVersionDataDir(curWorkDirParent))
		AddDirs(curWorkDirParent); // "../"

	AddDirs(dd_curWorkDir);
}


void DataDirLocater::AddHomeDirs()
{
#ifdef WIN32
	// All MS Windows variants

	// fetch my documents path
	TCHAR pathMyDocsC[MAX_PATH];
	TCHAR pathAppDataC[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, pathMyDocsC);
	SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA, nullptr, SHGFP_TYPE_CURRENT, pathAppDataC);
	const std::string pathMyDocs = pathMyDocsC;
	const std::string pathAppData = pathAppDataC;

	// e.g. F:\Dokumente und Einstellungen\Karl-Robert\Eigene Dateien\Spring
	const std::string dd_myDocs = pathMyDocs + "\\Spring";

	// My Documents\My Games seems to be the MS standard even if no official guidelines exist
	// most if not all new Games For Windows(TM) games use this dir
	const std::string dd_myDocsMyGames = pathMyDocs + "\\My Games\\Spring";

	// e.g. F:\Dokumente und Einstellungen\All Users\Anwendungsdaten\Spring
	const std::string dd_appData = pathAppData + "\\Spring";

	AddDirs(dd_myDocsMyGames);  // "C:/.../My Documents/My Games/Spring/"
	AddDirs(dd_myDocs);         // "C:/.../My Documents/Spring/"
	AddDirs(dd_appData);        // "C:/.../All Users/Applications/Spring/"

#else
	// Linux, FreeBSD, Solaris, Apple non-bundle
	AddDirs("${XDG_CONFIG_HOME-\"~/.config\"}/spring");
	AddDirs("~/.spring");
#endif
}


void DataDirLocater::AddEtcDirs()
{
#ifndef WIN32
	// Linux, FreeBSD, Solaris, Apple non-bundle

	// settings in /etc
	std::string dd_etc;
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
					dd_etc = dd_etc + (dd_etc.empty() ? "" : sPD) + lineBuf;
				}
			}
			fclose(fileH);
		}
	}

	AddDirs(dd_etc);  // from /etc/spring/datadir FIXME add in IsolatedMode too?
#endif
}


void DataDirLocater::AddShareDirs()
{
	// always true under Windows and true for `multi-engine` setups under *nix
	if (IsInstallDirDataDir())
		AddDirs(GetBinaryLocation());

#if defined(__APPLE__)
	// Mac OS X Application Bundle (*.app) - single file install

	// directory structure (Apple standard):
	// Spring.app/Contents/MacOS/springlobby
	// Spring.app/Contents/Resources/bin/spring
	// Spring.app/Contents/Resources/lib/unitsync.dylib
	// Spring.app/Contents/Resources/share/games/spring/base/

	const std::string dd_curWorkDir = GetBinaryLocation();

	// This corresponds to Spring.app/Contents/Resources/
	const std::string bundleResourceDir = FileSystem::GetParent(dd_curWorkDir);

	// This has to correspond with the value in the build-script
	const std::string dd_curWorkDirData = bundleResourceDir + "/share/games/spring";

	AddDirs(dd_curWorkDirData);             // "Spring.app/Contents/Resources/share/games/spring"
#endif

#ifdef SPRING_DATADIR
	// CompilerInfo: using the defineflag SPRING_DATADIR & "SPRING_DATADIR" as string works fine, the preprocessor won't touch the 2nd
	AddDirs(SPRING_DATADIR); // from -DSPRING_DATADIR, example /usr/games/share/spring/
#endif
}


void DataDirLocater::LocateDataDirs()
{
	// Construct the list of dataDirs from various sources.
	// Note: The first dir added will be the writable data dir!
	dataDirs.clear();


	// Note: first pushed dir is writeDir & dir priority decreases with pos in queue

	// LEVEL 1: User defined write dirs
	{
		if (!forcedWriteDir.empty())
			AddDirs(forcedWriteDir);

		const char* env = getenv("SPRING_WRITEDIR");

		if (env != nullptr && *env != 0)
			AddDirs(env); // ENV{SPRING_WRITEDIR}
	}

	// LEVEL 2: automated dirs
	if (IsIsolationMode()) {
		// LEVEL 2a: Isolation Mode (either installDir or user set one)
		if (isolationModeDir.empty()) {
			AddPortableDir(); // better use curWorkDir?
		} else {
			AddDirs(isolationModeDir);
		}
	} else {
		// LEVEL 2b: InstallDir, HomeDirs & Shared dirs
		if (IsPortableMode())
			AddPortableDir();

		AddHomeDirs();
		//AddCurWorkDir();
		AddEtcDirs();
		AddShareDirs();
	}

	// LEVEL 3: additional custom data sources
	{
		const char* env = getenv("SPRING_DATADIR");

		if (env != nullptr && *env != 0)
			AddDirs(env); // ENV{SPRING_DATADIR}

		// user defined in spring config (Linux: ~/.springrc, Windows: .\springsettings.cfg)
		if (configHandler != nullptr)
			AddDirs(configHandler->GetString("SpringData"));
	}

	// Find the folder we save to
	FindWriteableDataDir();
}


void DataDirLocater::Check()
{
	if (IsIsolationMode()) {
		LOG("[DataDirLocater::%s] Isolation Mode!", __func__);
	} else if (IsPortableMode()) {
		LOG("[DataDirLocater::%s] Portable Mode!", __func__);
	}

	// Filter usable DataDirs
	FilterUsableDataDirs();

	if (writeDir == nullptr) {
		// bail out
		const std::string errstr =
				"Not a single writable data directory found!\n\n"
				"Configure a writable data directory using either:\n"
				"- the SPRING_DATADIR environment variable,\n"
			#ifdef WIN32
				"- a SpringData=C:/path/to/data declaration in spring's config file ./springsettings.cfg\n"
				"- by giving your user-account write access to the installation directory";
			#else
				"- a SpringData=/path/to/data declaration in ~/.springrc or\n"
				"- the configuration file /etc/spring/datadir";
			#endif
		throw content_error(errstr);
	}

	if (Platform::FreeDiskSpace(writeDir->path) <= 1024)
		throw content_error("not enough free space on drive containing writeable data-directory " + writeDir->path);

	ChangeCwdToWriteDir();
	CreateCacheDir(writeDir->path + FileSystem::GetCacheDir());
}

void DataDirLocater::CreateCacheDir(const std::string& cacheDir)
{
	// tag the cache dir
	if (!FileSystem::CreateDirectory(cacheDir))
		return;

	CacheDir::SetCacheDir(cacheDir, true);
}


void DataDirLocater::ChangeCwdToWriteDir()
{
	// for now, chdir to the data directory as a safety measure:
	// Not only safety anymore, it's just easier if other code can safely assume that
	// writeDir == current working directory
	Platform::SetOrigCWD(); // save old cwd
	FileSystem::ChDir(writeDir->path);
}


bool DataDirLocater::IsInstallDirDataDir()
{
	// Check if spring binary & unitsync library are in the same folder
#if defined(UNITSYNC)
	const std::string dir = Platform::GetModulePath();
	const std::string fileExe = dir + "/" + GetSpringBinaryName();

	return FileSystem::FileExists(fileExe);

#else
	const std::string dir = Platform::GetProcessExecutablePath();
	const std::string fileUnitsync = dir + "/" + GetUnitsyncLibName();

	return FileSystem::FileExists(fileUnitsync);

#endif
}


bool DataDirLocater::IsPortableMode()
{
	// Test 1
	// Check if spring binary & unitsync library are in the same folder
	if (!IsInstallDirDataDir())
		return false;

	// Test 2
	// Check if "springsettings.cfg" is in the same folder, too.
	const std::string dir = GetBinaryLocation();
	if (!FileSystem::FileExists(dir + "/springsettings.cfg"))
		return false;

	// Test 3
	// Check if the directory is writeable
	if (!FileSystem::DirIsWritable(dir + "/"))
		return false;

	// PortableMode (don't use HomeDirs as writedirs, instead save files next to binary)
	return true;
}


bool DataDirLocater::LooksLikeMultiVersionDataDir(const std::string& dirPath)
{
	bool looksLikeDataDir = true;

	looksLikeDataDir = looksLikeDataDir && FileSystem::DirExists(dirPath + "/maps");
	looksLikeDataDir = looksLikeDataDir && FileSystem::DirExists(dirPath + "/games");
	looksLikeDataDir = looksLikeDataDir && FileSystem::DirExists(dirPath + "/engines");
	// looksLikeDataDir = looksLikeDataDir && FileSystem::DirExists(dirPath + "/unitsyncs"); TODO uncomment this if the new name for unitsync has been set

	return looksLikeDataDir;
}


std::string DataDirLocater::GetWriteDirPath() const
{
	if (writeDir == nullptr) {
		LOG_L(L_ERROR, "[DataDirLocater::%s] called before DataDirLocater::LocateDataDirs()", __func__);
		return "";
	}

	assert(writeDir->writable);
	return writeDir->path;
}


std::vector<std::string> DataDirLocater::GetDataDirPaths() const
{
	assert(!dataDirs.empty());
	std::vector<std::string> dataDirPaths;

	for (const DataDir& ddir: GetDataDirs()) {
		dataDirPaths.push_back(ddir.path);
	}

	return dataDirPaths;
}

std::array<std::string, 5> DataDirLocater::GetDataDirRoots() const
{
	return {{"base", "maps", "games", "packages", configHandler->GetString("SpringDataRoot")}};
}


static DataDirLocater* instance = nullptr;
DataDirLocater& DataDirLocater::GetInstance()
{
	if (instance == nullptr)
		instance = new DataDirLocater();

	return *instance;
}

void DataDirLocater::FreeInstance()
{
	spring::SafeDelete(instance);
}

