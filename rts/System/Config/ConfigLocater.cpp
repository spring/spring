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
#include <cstdlib>

#include "ConfigLocater.h"
#include "Game/GameVersion.h"
#include "System/Exceptions.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"
#include "System/Platform/Win/win32.h"

static void AddCfgFile(std::vector<std::string>& locations, const std::string& filepath)
{
	for (const std::string& fp: locations) {
		if (FileSystem::ComparePaths(fp, filepath))
			return;
	}

	locations.push_back(filepath);
}


static void LoadCfgs(std::vector<std::string>& locations, const std::string& defCfg, const std::string& verCfg)
{
	if (locations.empty()) {
		// add the config file we write to
		AddCfgFile(locations, defCfg);

		// create file if it doesn't exist
		FileSystem::TouchFile(defCfg);

		// warn user if the file is not both readable and writable
		// (otherwise it can fail/segfault/end up in virtualstore/...)
		if (access(defCfg.c_str(), R_OK | W_OK) == -1) {
			#ifndef _WIN32
			LOG_L(L_WARNING, "default config-file \"%s\" not both readable and writeable", defCfg.c_str());
			#else
			throw content_error(std::string("default config-file \"") + defCfg + "\" not both readable and writeable");
			#endif
		}
	}

	if (access(verCfg.c_str(), R_OK) != -1)
		AddCfgFile(locations, verCfg);

	if (access(defCfg.c_str(), R_OK) != -1)
		AddCfgFile(locations, defCfg);
}


static void LoadCfgsInFolder(std::vector<std::string>& locations, const std::string& path, const bool hidden = false)
{
	// all platforms: springsettings.cfg
	const std::string defCfg = path + "springsettings.cfg";
	const std::string verCfg = path + "springsettings-" + SpringVersion::Get() + ".cfg";
	LoadCfgs(locations, defCfg, verCfg);

#ifndef _WIN32
	// unix only: (.)springrc (lower priority than springsettings.cfg!)
	const std::string base = (hidden) ? ".springrc" : "springrc";
	const std::string unixDefCfg = path + base;
	const std::string unixVerCfg = unixDefCfg + "-" + SpringVersion::Get();
	LoadCfgs(locations, unixDefCfg, unixVerCfg);
#endif
}



/**
 * @brief Get the names of the default configuration files
 */
void ConfigLocater::GetDefaultLocations(std::vector<std::string>& locations)
{
	// first, add writeable config file
	LoadCfgsInFolder(locations, dataDirLocater.GetWriteDirPath(), false);

	// add additional readonly config files
	for (const std::string& path: dataDirLocater.GetDataDirPaths()) {
		LoadCfgsInFolder(locations, path);
	}
}

