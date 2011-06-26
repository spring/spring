/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Util.h"
#include "ConfigHandler.h"

#ifdef WIN32
	#include <io.h>
#endif
#include <stdio.h>
#include <string.h>

#include <list>
#include <stdexcept>

#include <boost/thread/mutex.hpp>

#include "Platform/ConfigLocater.h"
#include "Platform/ScopedFileLock.h"
#include "LogOutput.h"

using std::list;
using std::map;
using std::string;
using std::vector;

ConfigHandler* configHandler = NULL;

/******************************************************************************/

/**
 * @brief The data associated with one configuration key.
 * That is: a value and all preceding comments.
 */
class ConfigValue
{
public:
	ConfigValue(const string& value = string(), const string& comment = string()):
		comment(comment), value(value) {}

	const string& GetValue() const { return value; }
	const string& GetComment() const { return comment; }

	void SetValue(const string& newValue) { value = newValue; }

private:
	string value;
	string comment;
};

/******************************************************************************/

/**
 * @brief One configuration source (either a file or the overlay).
 */
class ConfigSource
{
public:
	ConfigSource(const string& filename);

	/**
	 * The overlay will not be written to file,
	 * and will thus be discarded when the game ends.
	 */
	bool IsOverlay() const { return filename.empty(); }

	bool IsSet(const string& key) const;
	string GetString(const string& key) const;
	void SetString(const string& key, const string& value);
	void Delete(const string& key);

	string GetFilename() const { return filename; }

private:
	void SetStringInternal(const string& key, const string& value);
	void DeleteInternal(const string& key);
	void ReadModifyWrite(boost::function<void ()> modify);

	string filename;
	map<string, ConfigValue> data;

	// helper functions
	void Read(FILE* file);
	void Write(FILE* file);
	char* Strip(char* begin, char* end);
};

/******************************************************************************/

class ConfigHandlerImpl : public ConfigHandler
{
public:
	ConfigHandlerImpl(const vector<string>& locations);
	~ConfigHandlerImpl();

	void SetString(const string& key, const string& value, bool useOverlay);
	string GetString(const string& key, const string& def, bool setInOverlay);
	bool IsSet(const string& key) const;
	void Delete(const string& name);
	string GetConfigFile() const;
	const map<string, string>& GetData() const;
	void Update();

protected:
	void AddObserver(ConfigNotifyCallback observer);

private:
	ConfigSource& GetOverlay() { return sources.front(); }
	const ConfigSource& GetOverlay() const { return sources.front(); }
	ConfigSource& GetSink() { return sources.at(1); }
	const ConfigSource& GetSink() const { return sources.at(1); }

	/**
	 * @brief config file names
	 */
	vector<ConfigSource> sources;

	/**
	 * @brief data map
	 *
	 * Map used to internally cache data
	 * instead of constantly rereading from the file.
	 * This is to mirror the config file.
	 */
	map<string, string> data;

	// observer related
	list<ConfigNotifyCallback> observers;
	boost::mutex observerMutex;
	map<string, string> changedValues;
};

/******************************************************************************/

ConfigSource::ConfigSource(const string& filename) : filename(filename) {
	FILE* file;

	if ((file = fopen(filename.c_str(), "r"))) {
		ScopedFileLock scoped_lock(fileno(file), false);
		Read(file);
	} else {
		if (!(file = fopen(filename.c_str(), "a")))
			throw std::runtime_error("ConfigSource: Error: Could not write to config file \"" + filename + "\"");
	}
	fclose(file);
}

bool ConfigSource::IsSet(const string& key) const
{
	return data.find(key) != data.end();
}

string ConfigSource::GetString(const string& key) const
{
	map<string, ConfigValue>::const_iterator pos = data.find(key);
	if (pos == data.end()) {
		throw std::runtime_error("ConfigSource: Error: Key does not exist: " + key);
	}
	return pos->second.GetValue();
}

void ConfigSource::SetStringInternal(const string& key, const string& value)
{
	map<string, ConfigValue>::iterator pos = data.find(key);
	if (pos == data.end()) {
		data[key] = ConfigValue(value);
	}
	else {
		pos->second.SetValue(value);
	}
}

void ConfigSource::SetString(const string& key, const string& value)
{
	ReadModifyWrite(boost::bind(&ConfigSource::SetStringInternal, this, key, value));
}

void ConfigSource::DeleteInternal(const string& key)
{
	data.erase(key);
}

void ConfigSource::Delete(const string& key)
{
	ReadModifyWrite(boost::bind(&ConfigSource::DeleteInternal, this, key));
}

void ConfigSource::ReadModifyWrite(boost::function<void ()> modify) {
	if (IsOverlay()) {
		modify();
	}
	else {
		FILE* file = fopen(filename.c_str(), "r+");

		if (file) {
			ScopedFileLock scoped_lock(fileno(file), true);
			Read(file);
			modify();
			Write(file);
		}
		else {
			modify();
		}

		// must be outside above 'if (file)' block because of the lock.
		if (file) {
			fclose(file);
		}
	}
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
char* ConfigSource::Strip(char* begin, char* end) {
	while (isspace(*begin)) ++begin;
	while (end >= begin && isspace(*end)) --end;
	*(end + 1) = '\0';
	return begin;
}

/**
 * @brief Rewind file and re-read it.
 */
void ConfigSource::Read(FILE* file)
{
	std::ostringstream commentBuffer;
	char line[500];
	rewind(file);
	while (fgets(line, sizeof(line), file)) {
		char* line_stripped = Strip(line, strchr(line, '\0'));
		if (*line_stripped == '\0' || *line_stripped == '#') {
			// an empty line or a comment line
			// note: trailing whitespace has been removed by Strip
			commentBuffer << line << "\n";
		}
		else {
			char* eq = strchr(line_stripped, '=');
			if (eq) {
				// a "key=value" line
				char* key = Strip(line_stripped, eq - 1);
				char* value = Strip(eq + 1, strchr(eq + 1, '\0') - 1);
				data[key] = ConfigValue(value, commentBuffer.str());
				// reset the ostringstream
				commentBuffer.clear();
				commentBuffer.str("");
			}
			else {
				// neither a comment nor an empty line nor a key=value line
				logOutput.Print("ConfigSource: Error: Can not parse line: %s\n", line);
			}
		}
	}
}

/**
 * @brief Truncate file and write data to it.
 */
void ConfigSource::Write(FILE* file)
{
	rewind(file);
#ifdef WIN32
	int err = _chsize(fileno(file), 0);
#else
	int err = ftruncate(fileno(file), 0);
#endif
	if (err != 0) {
		logOutput.Print("ConfigSource: Error: Failed truncating config file.");
	}
	for(map<string, ConfigValue>::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
		fputs(iter->second.GetComment().c_str(), file);
		fprintf(file, "%s = %s\n", iter->first.c_str(), iter->second.GetValue().c_str());
	}
}

/******************************************************************************/

ConfigHandlerImpl::ConfigHandlerImpl(const vector<string>& locations)
{
	sources.reserve(1 + locations.size());
	sources.push_back(ConfigSource("")); // overlay
	for (vector<string>::const_iterator loc = locations.begin(); loc != locations.end(); ++loc) {
		sources.push_back(ConfigSource(*loc));
	}
}

void ConfigHandlerImpl::Delete(const string& key)
{
	for (vector<ConfigSource>::iterator it = sources.begin(); it != sources.end(); ++it) {
		it->Delete(key);
	}
}

bool ConfigHandlerImpl::IsSet(const string& key) const
{
	for (vector<ConfigSource>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		if (it->IsSet(key)) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Gets string value from config
 *
 * If string value isn't set, it returns def AND it sets data[name] = def,
 * so the defaults end up in the config file.
 */
string ConfigHandlerImpl::GetString(const string& key, const string& def, bool setInOverlay)
{
	for (vector<ConfigSource>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		if (it->IsSet(key)) {
			return it->GetString(key);
		}
	}
	if (setInOverlay) {
		GetOverlay().SetString(key, def);
	}
	return def;
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
void ConfigHandlerImpl::SetString(const string& key, const string& value, bool useOverlay)
{
	// Don't do anything if value didn't change.
	if (GetString(key, "", false) == value) {
		return;
	}

	if (useOverlay) {
		GetOverlay().SetString(key, value);
	}
	else {
		// if we set something to be persisted,
		// we do want to override the overlay value
		GetOverlay().Delete(key);

		// FIXME: this needs more subtle code
		// i.e. delete the key from overriding config files when it becomes equal to the default specified in another source
		GetSink().SetString(key, value);
	}

	boost::mutex::scoped_lock lck(observerMutex);
	changedValues[key] = value;
}

/**
 * @brief call observers if a config changed
 */
void ConfigHandlerImpl::Update()
{
	boost::mutex::scoped_lock lck(observerMutex);
	for (map<string, string>::const_iterator ut = changedValues.begin(); ut != changedValues.end(); ++ut) {
		const string& key = ut->first;
		const string& value = ut->second;
		for (list<ConfigNotifyCallback>::const_iterator it = observers.begin(); it != observers.end(); ++it) {
			(*it)(key, value);
		}
	}
	changedValues.clear();
}


string ConfigHandlerImpl::GetConfigFile() const {
	return GetSink().GetFilename();
}


const map<string, string>& ConfigHandlerImpl::GetData() const {
	return data;
}


void ConfigHandlerImpl::AddObserver(ConfigNotifyCallback observer) {
	boost::mutex::scoped_lock lck(observerMutex);
	observers.push_back(observer);
}

/******************************************************************************/

/**
 * Instantiates a copy of the current platform's config class.
 * Re-instantiates if the configHandler already existed.
 */
void ConfigHandler::Instantiate(string configSource)
{
	Deallocate();

	vector<string> locations;

	if (configSource.empty()) {
		ConfigLocater::GetDefaultLocations(locations);
	}
	else {
		locations.push_back(configSource);
	}

	// log here so unitsync shows configuration source(s), too
	vector<string>::const_iterator loc = locations.begin();
	logOutput.Print("Using configuration source: \"" + *loc + "\"\n");
	for (++loc; loc != locations.end(); ++loc) {
		logOutput.Print("Using additional configuration source: \"" + *loc + "\"\n");
	}

	configHandler = new ConfigHandlerImpl(locations);
}


/**
 * Destroys existing ConfigHandler instance.
 */
void ConfigHandler::Deallocate()
{
	SafeDelete(configHandler);
}

/******************************************************************************/
