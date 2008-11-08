/**
 * @file DotfileHandler.cpp
 * @brief Linux dotfile config implementation
 * @author Christopher Han <xiphux@gmail.com>
 * @author Tobi Vollebregt <tobivollebregt@gmail.com>
 *
 * DotfileHandler configuration class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#include "StdAfx.h"
#include "DotfileHandler.h"
#include <sstream>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "Game/GameVersion.h"
#include "Exceptions.h"


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
	struct flock lock;
	lock.l_type = write ? F_WRLCK : F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(filedes, F_SETLKW, &lock)) {
		// not a fatal error
		//handleerror(0, "Could not lock config file", "DotfileHandler", 0);
	}
}

/**
 * @brief unlock fd
 */
ScopedFileLock::~ScopedFileLock()
{
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(filedes, F_SETLKW, &lock)) {
		// not a fatal error
		//handleerror(0, "Could not unlock config file", "DotfileHandler", 0);
	}
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
char* DotfileHandler::Strip(char* begin, char* end) {
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
void DotfileHandler::AppendLine(char* buf) {
	char* eq = strchr(buf, '=');
	if (eq) {
		char* key = Strip(buf, eq - 1);
		char* value = Strip(eq + 1, strchr(eq + 1, 0) - 1);
		data[key] = value;
	}
}

/**
 * @brief Open file
 *
 * Attempts to open an existing config file.
 * If none exists, create one.
 */
DotfileHandler::DotfileHandler(const std::string& fname) : filename(fname)
{
	FILE* file;

	if ((file = fopen(fname.c_str(), "r"))) {
		ScopedFileLock scoped_lock(fileno(file), false);
		Read(file);
	} else {
		if (!(file = fopen(fname.c_str(), "a")))
			throw std::runtime_error("DotfileHandler: Could not write to config file");
	}
	fclose(file);
}

DotfileHandler::~DotfileHandler()
{
}

/**
 * @brief Gets integer value from config
 */
int DotfileHandler::GetInt(const string name, const int def)
{
	std::map<string,string>::iterator pos = data.find(name);
	if (pos == data.end()) {
		SetInt(name, def);
		return def;
	}
	return atoi(pos->second.c_str());
}

/**
 * @brief Gets string value from config
 *
 * If string value isn't set, it returns def AND it sets data[name] = def,
 * so the defaults end up in the config file.
 */
string DotfileHandler::GetString(const string name, const string def)
{
	std::map<string,string>::iterator pos = data.find(name);
	if (pos == data.end()) {
		SetString(name, def);
		return def;
	}
	return pos->second;
}

/**
 * @brief Rewind file and re-read it.
 */
void DotfileHandler::Read(FILE* file)
{
	char buf[500];
	rewind(file);
	while (fgets(buf, sizeof(buf), file))
		AppendLine(buf);
}

/**
 * @brief Truncate file and write data to it.
 */
void DotfileHandler::Write(FILE* file)
{
	rewind(file);
	ftruncate(fileno(file), 0);
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); ++iter)
		fprintf(file, "%s=%s\n", iter->first.c_str(), iter->second.c_str());
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
void DotfileHandler::SetString(const string name, const string value)
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
 * @brief Sets a config integer
 */
void DotfileHandler::SetInt(const string name, const int value)
{
	std::stringstream ss;
	ss << value;
	SetString(name, ss.str());
}


/**
 * @brief Get the name of the default configuration file
 */
string DotfileHandler::GetDefaultConfig()
{
	const string base = ".springrc";
	const string home = getenv("HOME");

	const string defCfg = home + "/" + base;
	const string verCfg = defCfg + "-" + SpringVersion::Get();

	string cfg;

	struct stat st;
	if (stat(verCfg.c_str(), &st) == 0) {
		cfg = verCfg; // use the versionned config file
	} else {
		cfg = defCfg; // use the default config file
	}

	return cfg;
}

