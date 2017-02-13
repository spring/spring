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
#include "System/Exceptions.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/Misc.h"
#include "System/Platform/Win/win32.h"


using std::string;
using std::vector;


static void AddCfgFile(vector<string>& locations, const  std::string& filepath)
{
	for(const std::string& fp: locations) {
		if (FileSystem::ComparePaths(fp, filepath)) {
			return;
		}
	}
	locations.push_back(filepath);
}


static void LoadCfgs(vector<string>& locations, const std::string& defCfg, const std::string& verCfg)
{
	// lets see if the file exists & is writable
	// (otherwise it can fail/segfault/end up in virtualstore...)

	if (locations.empty()) {
		// add the config file we write to
		AddCfgFile(locations, defCfg);

		FileSystem::TouchFile(defCfg); // create file if doesn't exists
		if (access(defCfg.c_str(), R_OK | W_OK) == -1) {
			throw content_error(std::string("config file not writeable: \"") + defCfg + "\"");
		}
	}

	if (access(verCfg.c_str(), R_OK) != -1) {
		AddCfgFile(locations, verCfg);
	}
	if (access(defCfg.c_str(), R_OK) != -1) {
		AddCfgFile(locations, defCfg);
	}
}


static void LoadCfgsInFolder(vector<string>& locations, const std::string& path, const bool hidden = false)
{
	// all platforms: springsettings.cfg
	const string defCfg = path + "springsettings.cfg";
	const string verCfg = path + "springsettings-" + SpringVersion::Get() + ".cfg";
	LoadCfgs(locations, defCfg, verCfg);

#ifndef _WIN32
	// unix only: (.)springrc (lower priority than springsettings.cfg!)
	const string base = (hidden) ? ".springrc" : "springrc";
	const string unixDefCfg = path + base;
	const string unixVerCfg = unixDefCfg + "-" + SpringVersion::Get();
	LoadCfgs(locations, unixDefCfg, unixVerCfg);
#endif
}



/**
 * @brief Get the names of the default configuration files
 */
void ConfigLocater::GetDefaultLocations(vector<string>& locations)
{
	// first, add writeable config file
	LoadCfgsInFolder(locations, dataDirLocater.GetWriteDirPath(), false);

	// add additional readonly config files
	for(const std::string& path: dataDirLocater.GetDataDirPaths()) {
		LoadCfgsInFolder(locations, path);
	}
}
