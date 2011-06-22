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
#include "System/LogOutput.h"
#include "ConfigLocater.h"


using std::string;

/**
 * @brief Get the name of the default configuration file
 */
string ConfigLocater::GetDefaultLocation()
{
	string binaryPath = Platform::GetProcessExecutablePath() + "/";
	string portableConfPath = binaryPath + "springsettings.cfg";
	if (access(portableConfPath.c_str(), 6) != -1) {
		return portableConfPath;
	}

	string cfg;

#ifndef _WIN32
	const string base = ".springrc";
	const string home = getenv("HOME");

	const string defCfg = home + "/" + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();

	struct stat st;
	if (stat(verCfg.c_str(), &st) == 0) { // check if file exists
		cfg = verCfg; // use the versionned config file
	} else {
		cfg = defCfg; // use the default config file
	}
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
		cfg = verConfigPath;
	} else {
		cfg = strPath;
		cfg += "\\springsettings.cfg"; // e.g. F:\Documents and Settings\MyUser\Local Settings\App Data
	}
	// log here so unitsync shows configuration source, too
	logOutput.Print("default config file: " + cfg + "\n");
#endif

	return cfg;
}
