/**
 * @file ConfigHandler.cpp
 * @brief config implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Implementation of config structure class
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include "ConfigHandler.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <unistd.h>

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

#include "Game/GameVersion.h"

using std::string;

ConfigHandler* _configHandler;

/**
 * @brief POSIX file locking class
 */
class ScopedFileLock
{
public:
	ScopedFileLock(int fd, bool write);
	~ScopedFileLock();
private:
	int filedes;
};

/**
 * @brief lock fd
 *
 * Lock file descriptor fd for reading (write == false) or writing
 * (write == true).
 */
ScopedFileLock::ScopedFileLock(int fd, bool write) : filedes(fd)
{
#ifndef _WIN32
	struct flock lock;
	lock.l_type = write ? F_WRLCK : F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(filedes, F_SETLKW, &lock)) {
		// not a fatal error
		//handleerror(0, "Could not lock config file", "DotfileHandler", 0);
	}
#endif
}

/**
 * @brief unlock fd
 */
ScopedFileLock::~ScopedFileLock()
{
#ifndef _WIN32
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(filedes, F_SETLKW, &lock)) {
		// not a fatal error
		//handleerror(0, "Could not unlock config file", "DotfileHandler", 0);
	}
#endif
}


/**
 * Instantiates a copy of the current platform's config class.
 * Re-instantiates if the configHandler already existed.
 */
std::string ConfigHandler::Instantiate(std::string configSource)
{
	Deallocate();

	if (configSource.empty()) {
		configSource = GetDefaultConfig();
	}
	_configHandler = new ConfigHandler(configSource);
	return configSource;
}


/**
 * Destroys existing ConfigHandler instance.
 */
void ConfigHandler::Deallocate()
{
	// can not use SafeDelete because ~ConfigHandler is protected
	delete _configHandler;
	_configHandler = NULL;
}

ConfigHandler::ConfigHandler(const std::string& configFile)
{
	filename = configFile;
	FILE* file;

	if ((file = fopen(filename.c_str(), "r"))) {
		ScopedFileLock scoped_lock(fileno(file), false);
		Read(file);
	} else {
		if (!(file = fopen(filename.c_str(), "a")))
			throw std::runtime_error("DotfileHandler: Could not write to config file");
	}
	fclose(file);
}

ConfigHandler::~ConfigHandler()
{
}


/**
 * @brief Gets string value from config
 *
 * If string value isn't set, it returns def AND it sets data[name] = def,
 * so the defaults end up in the config file.
 */
string ConfigHandler::GetString(const string name, const string def)
{
	std::map<string,string>::iterator pos = data.find(name);
	if (pos == data.end()) {
		SetString(name, def);
		return def;
	}
	return pos->second;
}


/**
 * @brief Sets a config string
 *
 * This does:
 * 1) Lock file.
 * 2) Read file (in case another program modified something)
 * 3) Set data[name] = value.
 * 4) Write file (so we keep the settings in the event of a crash or error)
 * 5) Unlock file.
 *
 * Pretty hackish, but Windows registry changes
 * save immediately, while with DotfileHandler the
 * data is stored in an internal data structure for
 * fast access.  So we want to keep the settings in
 * the event of a crash, but at the same time we don't
 * want conflicts when multiple instances are running
 * at the same time (which would cause data loss)
 * (e.g. unitsync and spring).
 */
void ConfigHandler::SetString(const string name, const string value)
{
	FILE* file = fopen(filename.c_str(), "r+");

	if (file) {
		ScopedFileLock scoped_lock(fileno(file), true);
		Read(file);
		data[name] = value;
		Write(file);
	} else
		data[name] = value;

	// must be outside above 'if (file)' block because of the lock.
		if (file)
			fclose(file);
}

/**
 * @brief Get the name of the default configuration file
 */
string ConfigHandler::GetDefaultConfig()
{
	string cfg;
#ifndef _WIN32
	const string base = ".springrc";
	const string home = getenv("HOME");

	const string defCfg = home + "/" + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();


	struct stat st;
	if (stat(verCfg.c_str(), &st) == 0) {
		cfg = verCfg; // use the versionned config file
	} else {
		cfg = defCfg; // use the default config file
	}
#else
	// appdata
	TCHAR strPath[MAX_PATH];
	SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath);
	cfg = strPath;
	cfg += "\\springsettings.cfg"; // e.g. F:\Dokumente und Einstellungen\MyUser\Anwendungsdaten
#endif
	
	return cfg;
}

/**
 * @brief strip whitespace
 *
 * Strips whitespace off the string [begin, end] by setting the first
 * non-whitespace character from the end to 0 and returning a pointer
 * to the first non-whitespace character from the begin.
 *
 * Precondition: end must point to the last character of the string,
 * ie. the one before the terminating null.
 */
char* ConfigHandler::Strip(char* begin, char* end) {
	while (isspace(*begin)) ++begin;
	while (end >= begin && isspace(*end)) --end;
	*(end + 1) = 0;
	return begin;
}

/**
 * @brief process one line read from file
 *
 * Parses the 'key=value' assignment in buf and sets data[key] to value.
 */
void ConfigHandler::AppendLine(char* buf) {
	char* eq = strchr(buf, '=');
	if (eq) {
		char* key = Strip(buf, eq - 1);
		char* value = Strip(eq + 1, strchr(eq + 1, 0) - 1);
		data[key] = value;
	}
}

/**
 * @brief Rewind file and re-read it.
 */
void ConfigHandler::Read(FILE* file)
{
	char buf[500];
	rewind(file);
	while (fgets(buf, sizeof(buf), file))
		AppendLine(buf);
}

/**
 * @brief Truncate file and write data to it.
 */
void ConfigHandler::Write(FILE* file)
{
	rewind(file);
	ftruncate(fileno(file), 0);
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); ++iter)
		fprintf(file, "%s=%s\n", iter->first.c_str(), iter->second.c_str());
}
