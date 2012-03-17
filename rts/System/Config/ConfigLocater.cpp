/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WIN32
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#else
	#include <io.h>
	#include <direct.h>
	#include <windows.h>
#endif
#include <stdlib.h>

#include "ConfigLocater.h"
#include "Game/GameVersion.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/Misc.h"


using std::string;
using std::vector;

static void GetPortableLocations(vector<string>& locations)
{
#ifndef _WIN32
	const std::string cfgName = ".springrc";
#else
	const std::string cfgName = "springsettings.cfg";
#endif

	if (dataDirLocater.IsIsolationMode()) {
		// Search in IsolatedModeDir
		const std::string confPath = FileSystem::EnsurePathSepAtEnd(dataDirLocater.GetIsolationModeDir()) + cfgName;
		locations.push_back(confPath);
	} else {
		// Search in binary dir
		const std::string confPath = FileSystem::EnsurePathSepAtEnd(Platform::GetProcessExecutablePath()) + cfgName;

		// lets see if the file exists & is writable
		// (otherwise it can fail/segfault/end up in virtualstore...)
		// _access modes: 0 - exists; 2 - write; 4 - read; 6 - r/w
		// doesn't work on directories (precisely, mode is always 0)
		if (access(confPath.c_str(), 6) != -1) {
			locations.push_back(confPath);
		}
	}
}

static void GetPlatformLocations(vector<string>& locations)
{
#ifndef _WIN32
	const string base = ".springrc";
	const string home = FileSystem::EnsurePathSepAtEnd(Platform::GetUserDir());

	const string defCfg = home + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();
#else
	// e.g. "C:\Users\USER\AppData\Local"
	const string userDir = FileSystem::EnsurePathSepAtEnd(Platform::GetUserDir());

	const string defCfg = userDir + "springsettings.cfg";
	const string verCfg = userDir + "springsettings-" + SpringVersion::Get() + ".cfg";
#endif

	if (access(verCfg.c_str(), 6) != -1) { // check for read & write access
		locations.push_back(verCfg);
	}

	locations.push_back(defCfg);
}

/**
 * @brief Get the names of the default configuration files
 */
void ConfigLocater::GetDefaultLocations(vector<string>& locations)
{
	// check if there exists a config file in the same directory as the exe
	GetPortableLocations(locations);

	// portable implies isolated:
	// no extra sources when a portable source has been found!
	if (locations.empty()) {
		GetPlatformLocations(locations);
	}
}
