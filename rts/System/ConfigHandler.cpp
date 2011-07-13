/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/ConfigHandler.h"

#include <string.h>
#include <stdexcept>

#include "System/Platform/ConfigLocater.h"
#include "System/Platform/ScopedFileLock.h"
#include "System/LogOutput.h"
#ifdef WIN32
#include <io.h>
#endif

using std::string;

ConfigHandler* configHandler = NULL;

void ConfigHandler::Delete(const std::string& name, bool inOverlay)
{
	if (inOverlay) {
		std::map<std::string, std::string>::iterator pos = overlay.find(name);
		if (pos != overlay.end()) {
			overlay.erase(pos);
		}
	} else {
		FILE* file = fopen(filename.c_str(), "r+");
		if (file) {
			ScopedFileLock scoped_lock(fileno(file), true);
			Read(file);
			std::map<std::string, std::string>::iterator pos = data.find(name);
			if (pos != data.end()) {
				data.erase(pos);
			}
			Write(file);
		} else {
			std::map<std::string, std::string>::iterator pos = data.find(name);
			if (pos != data.end()) {
				data.erase(pos);
			}
		}

		// must be outside above 'if (file)' block because of the lock.
		if (file) {
			fclose(file);
		}
	}
}

/**
 * Instantiates a copy of the current platform's config class.
 * Re-instantiates if the configHandler already existed.
 */
std::string ConfigHandler::Instantiate(std::string configSource)
{
	Deallocate();

	if (configSource.empty()) {
		configSource = ConfigLocater::GetDefaultLocation();
	}
	configHandler = new ConfigHandler(configSource);

	return configSource;
}


/**
 * Destroys existing ConfigHandler instance.
 */
void ConfigHandler::Deallocate()
{
	// can not use SafeDelete because ~ConfigHandler is protected
	delete configHandler;
	configHandler = NULL;
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


bool ConfigHandler::IsSet(const std::string& key) const
{
	return ((overlay.find(key) != overlay.end()) || (data.find(key) != data.end()));
}


/**
 * @brief Gets string value from config
 *
 * If string value isn't set, it returns def AND it sets data[name] = def,
 * so the defaults end up in the config file.
 */
string ConfigHandler::GetString(const string name, const string def, bool setInOverlay)
{
	std::map<string,string>::iterator pos;
	pos = overlay.find(name);
	if (pos != overlay.end()) {
		return pos->second;
	}

	pos = data.find(name);
	if (pos == data.end()) {
		SetString(name, def, setInOverlay);
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
 * Data is stored in an internal data structure for
 * fast access.
 * Currently, settings are lost in the event of a crash.
 * We do not want conflicts when multiple instances are running
 * at the same time (which would cause data loss).
 * This would happen if e.g. unitsync and spring would access
 * the config file at the same time, if we would not lock.
 */
void ConfigHandler::SetString(const string name, const string value, bool useOverlay)
{
	// if we set something to be persisted,
	// we do want to override the overlay value
	if (!useOverlay) {
		overlay.erase(name);
	}

	// Don't do anything if value didn't change.
	// Can't use GetString because of risk for infinite loop.
	// (GetString writes default value if key isn't present.)
	std::map<string, string>& dataCont = useOverlay ? overlay : data;
	std::map<string,string>::iterator pos = dataCont.find(name);
	if (pos != dataCont.end() && pos->second == value) {
		return;
	}

	if (useOverlay) {
		overlay[name] = value;
	} else {
		FILE* file = fopen(filename.c_str(), "r+");

		if (file) {
			ScopedFileLock scoped_lock(fileno(file), true);
			Read(file);
			data[name] = value;
			Write(file);
		} else {
			data[name] = value;
		}

		// must be outside above 'if (file)' block because of the lock.
		if (file) {
			fclose(file);
		}
	}

	boost::mutex::scoped_lock lck(observerMutex);
	changedValues[name] = value;
}

/**
 * @brief call observers if a config changed
 */
void ConfigHandler::Update()
{
	boost::mutex::scoped_lock lck(observerMutex);
	for (std::map<std::string, std::string>::const_iterator ut = changedValues.begin(); ut != changedValues.end(); ++ut) {
		const std::string& name = ut->first;
		const std::string& value = ut->second;
		for (std::list<ConfigNotifyCallback>::const_iterator it = observers.begin(); it != observers.end(); ++it) {
			(*it)(name, value);
		}
	}
	changedValues.clear();
}

void ConfigHandler::SetOverlay(std::string name, std::string value)
{
	SetString(name, value, true);
}

/**
 * @brief strip whitespace
 *
 * Strips whitespace off the string [begin, end] by setting the first
 * non-whitespace character from the end to 0 and returning a pointer
 * to the first non-whitespace character from the begining.
 *
 * Precondition: end must point to the last character of the string,
 * ie. the one before the terminating '\0'.
 */
char* ConfigHandler::Strip(char* begin, char* end) {
	while (isspace(*begin)) ++begin;
	while (end >= begin && isspace(*end)) --end;
	*(end + 1) = '\0';
	return begin;
}

/**
 * @brief process one line read from file
 *
 * Parses the 'key=value' assignment in line and sets data[key] to value.
 */
void ConfigHandler::AppendLine(char* line) {

	char* line_stripped = Strip(line, strchr(line, '\0'));

	if (*line_stripped != '\0') {
		// line is not empty
		if (line_stripped[0] == '#') {
			// a comment line
			return;
		}

		char* eq = strchr(line_stripped, '=');
		if (eq) {
			// a "key=value" line
			char* key = Strip(line_stripped, eq - 1);
			char* value = Strip(eq + 1, strchr(eq + 1, 0) - 1);
			data[key] = value;
		}
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
#ifdef WIN32
	int err = _chsize(fileno(file), 0);
#else
	int err = ftruncate(fileno(file), 0);
#endif
	if (err != 0) {
		logOutput.Print("Error: Failed truncating config file.");
	}
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); ++iter)
		fprintf(file, "%s=%s\n", iter->first.c_str(), iter->second.c_str());
}


const std::map<std::string, std::string> &ConfigHandler::GetData() const {
	return data;
}
