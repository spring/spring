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
#include <boost/foreach.hpp>


using std::string;
using std::vector;


static void AddCfgFile(vector<string>& locations, const  std::string& filepath)
{
	BOOST_FOREACH(std::string& fp, locations) {
		if (fp == filepath) {
			return;
		}
	}
	locations.push_back(filepath);
}


static void LoadCfgInFolder(vector<string>& locations, const std::string& path, const bool hidden = false, const bool force = false)
{
#ifndef _WIN32
	const string base = (hidden) ? ".springrc" : "springrc";
	const string defCfg = path + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();
#else
	const string defCfg = path + "springsettings.cfg";
	const string verCfg = path + "springsettings-" + SpringVersion::Get() + ".cfg";
#endif

	// lets see if the file exists & is writable
	// (otherwise it can fail/segfault/end up in virtualstore...)
	// _access modes: 0 - exists; 2 - write; 4 - read; 6 - r/w
	// doesn't work on directories (precisely, mode is always 0)
	const int _w = 6;
	const int _r = 4;

	if (locations.empty()) {
		// add the config file we write to
		AddCfgFile(locations, defCfg);

		FileSystem::TouchFile(defCfg); // create file if doesn't exists
		if (access(defCfg.c_str(), _w) == -1) { // check for write access
			throw content_error(std::string("config file not writeable: \"") + defCfg + "\"");
		}
	}

	if (access(verCfg.c_str(), _r) != -1) { // check for read access
		AddCfgFile(locations, verCfg);
	}
	if (access(defCfg.c_str(), _r) != -1) { // check for read access
		AddCfgFile(locations, defCfg);
	}
}


/**
 * @brief Get the names of the default configuration files
 */
void ConfigLocater::GetDefaultLocations(vector<string>& locations)
{
	// first, add writeable config file
	LoadCfgInFolder(locations, dataDirLocater.GetWriteDirPath(), false, true);

	// old primary
	// e.g. linux: "~/.springrc"; windows: "C:\Users\USER\AppData\Local\springsettings.cfg"
	const string userDir = FileSystem::EnsurePathSepAtEnd(Platform::GetUserDir());
	LoadCfgInFolder(locations, userDir, true);

	// add additional readonly config files
	BOOST_FOREACH(std::string path, dataDirLocater.GetDataDirPaths()) {
		LoadCfgInFolder(locations, path);
	}
}
