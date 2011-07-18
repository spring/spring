/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WIN32
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#else
	#include <io.h>
	#include <direct.h>
	#include <windows.h>
	#include <shlobj.h>
	#include <shlwapi.h>
	#ifndef SHGFP_TYPE_CURRENT
		#define SHGFP_TYPE_CURRENT 0
	#endif
#endif
#include <stdlib.h>
#include "Game/GameVersion.h"
#include "System/Platform/Misc.h"
#include "ConfigLocater.h"


using std::string;
using std::vector;

static void GetPortableLocations(vector<string>& locations)
{
	string binaryPath = Platform::GetProcessExecutablePath() + "/";
	string portableConfPath = binaryPath + "springsettings.cfg";
	if (access(portableConfPath.c_str(), 6) != -1) {
		locations.push_back(portableConfPath);
	}
}

static void GetPlatformLocations(vector<string>& locations)
{
#ifndef _WIN32
	const string base = ".springrc";
	const string home = getenv("HOME");

	const string defCfg = home + "/" + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();

	struct stat st;
	if (stat(verCfg.c_str(), &st) == 0) { // check if file exists
		locations.push_back(verCfg); // use the versionned config file
	}
	locations.push_back(defCfg); // use the default config file
#else
	// first, check if there exists a config file in the same directory as the exe
	// and if the file is writable (otherwise it can fail/segfault/end up in virtualstore...)
	// _access modes: 0 - exists; 2 - write; 4 - read; 6 - r/w
	// doesn't work on directories (precisely, mode is always 0)
	TCHAR strPath[MAX_PATH+1];
	SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath);
	const string userDir = strPath;
	const string verConfigPath = userDir + "\\springsettings-" + SpringVersion::Get() + ".cfg";
	if (_access(verConfigPath.c_str(), 6) != -1) { // check for read & write access
		locations.push_back(verConfigPath);
	}
	// e.g. F:\Documents and Settings\MyUser\Local Settings\App Data
	locations.push_back(string(strPath) + "\\springsettings.cfg");
#endif
}

/**
 * @brief Get the names of the default configuration files
 */
void ConfigLocater::GetDefaultLocations(vector<string>& locations)
{
	GetPortableLocations(locations);

	// portable implies isolated:
	// no extra sources when a portable source has been found!
	if (locations.empty()) {
		GetPlatformLocations(locations);
	}
}
