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

/******************************************************************************/

using std::list;
using std::map;
using std::string;
using std::vector;

typedef map<string, string> StringMap;

ConfigHandler* configHandler = NULL;

/******************************************************************************/

/**
 * @brief One configuration source (either a file or the overlay).
 */
class ConfigSource
{
public:
	virtual ~ConfigSource() {}

	virtual bool IsSet(const string& key) const;
	virtual string GetString(const string& key) const;
	virtual void SetString(const string& key, const string& value);
	virtual void Delete(const string& key);

	const StringMap& GetData() const { return data; }

protected:
	StringMap data;
};

class OverlayConfigSource : public ConfigSource
{
};

class FileConfigSource : public ConfigSource
{
public:
	FileConfigSource(const string& filename);

	void SetString(const string& key, const string& value);
	void Delete(const string& key);

	string GetFilename() const { return filename; }

private:
	void DeleteInternal(const string& key);
	void ReadModifyWrite(boost::function<void ()> modify);

	string filename;
	StringMap comments;

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
	string GetString(const string& key) const;
	bool IsSet(const string& key) const;
	void Delete(const string& key);
	string GetConfigFile() const;
	const StringMap GetData() const;
	void Update();

protected:
	void AddObserver(ConfigNotifyCallback observer);

private:
	OverlayConfigSource* overlay;
	FileConfigSource* writableSource;
	vector<ConfigSource*> sources;

	// observer related
	list<ConfigNotifyCallback> observers;
	boost::mutex observerMutex;
	StringMap changedValues;
};

/******************************************************************************/

bool ConfigSource::IsSet(const string& key) const
{
	return data.find(key) != data.end();
}

string ConfigSource::GetString(const string& key) const
{
	StringMap::const_iterator pos = data.find(key);
	if (pos == data.end()) {
		throw std::runtime_error("ConfigSource: Error: Key does not exist: " + key);
	}
	return pos->second;
}

void ConfigSource::SetString(const string& key, const string& value)
{
	data[key] = value;
}

void ConfigSource::Delete(const string& key)
{
	data.erase(key);
}

/******************************************************************************/

FileConfigSource::FileConfigSource(const string& filename) : filename(filename)
{
	FILE* file;

	if ((file = fopen(filename.c_str(), "r"))) {
		ScopedFileLock scoped_lock(fileno(file), false);
		Read(file);
	}
	else {
		if (!(file = fopen(filename.c_str(), "a")))
			throw std::runtime_error("FileConfigSource: Error: Could not write to config file \"" + filename + "\"");
	}
	fclose(file);
}

void FileConfigSource::SetString(const string& key, const string& value)
{
	ReadModifyWrite(boost::bind(&ConfigSource::SetString, this, key, value));
}

void FileConfigSource::DeleteInternal(const string& key)
{
	ConfigSource::Delete(key);
	comments.erase(key);
}

void FileConfigSource::Delete(const string& key)
{
	ReadModifyWrite(boost::bind(&FileConfigSource::DeleteInternal, this, key));
}

void FileConfigSource::ReadModifyWrite(boost::function<void ()> modify) {
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

/**
 * @brief strip whitespace
 *
 * Strips whitespace off the string [begin, end] by setting the first
 * non-whitespace character from the end to 0 and returning a pointer
 * to the first non-whitespace character from the beginning.
 *
 * Precondition: end must point to the last character of the string,
 * i.e. the one before the terminating '\0'.
 */
char* FileConfigSource::Strip(char* begin, char* end) {
	while (isspace(*begin)) ++begin;
	while (end >= begin && isspace(*end)) --end;
	*(end + 1) = '\0';
	return begin;
}

/**
 * @brief Rewind file and re-read it.
 */
void FileConfigSource::Read(FILE* file)
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
				data[key] = value;
				comments[key] = commentBuffer.str();
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
void FileConfigSource::Write(FILE* file)
{
	rewind(file);
#ifdef WIN32
	int err = _chsize(fileno(file), 0);
#else
	int err = ftruncate(fileno(file), 0);
#endif
	if (err != 0) {
		logOutput.Print("FileConfigSource: Error: Failed truncating config file.");
	}
	for(StringMap::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
		StringMap::const_iterator comment = comments.find(iter->first);
		if (comment != comments.end()) {
			fputs(comment->second.c_str(), file);
		}
		fprintf(file, "%s = %s\n", iter->first.c_str(), iter->second.c_str());
	}
}

/******************************************************************************/

#define for_each_source(it) \
	for (vector<ConfigSource*>::iterator it = sources.begin(); it != sources.end(); ++it)

#define for_each_source_const(it) \
	for (vector<ConfigSource*>::const_iterator it = sources.begin(); it != sources.end(); ++it)

ConfigHandlerImpl::ConfigHandlerImpl(const vector<string>& locations)
{
	overlay = new OverlayConfigSource();
	writableSource = new FileConfigSource(locations.front());

	sources.reserve(1 + locations.size());
	sources.push_back(overlay);
	sources.push_back(writableSource);

	vector<string>::const_iterator loc = locations.begin();
	for (++loc; loc != locations.end(); ++loc) {
		sources.push_back(new FileConfigSource(*loc));
	}
}

ConfigHandlerImpl::~ConfigHandlerImpl()
{
	for_each_source(it) {
		delete (*it);
	}
}

void ConfigHandlerImpl::Delete(const string& key)
{
	for_each_source(it) {
		(*it)->Delete(key);
	}
}

bool ConfigHandlerImpl::IsSet(const string& key) const
{
	for_each_source_const(it) {
		if ((*it)->IsSet(key)) {
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
string ConfigHandlerImpl::GetString(const string& key) const
{
	for_each_source_const(it) {
		if ((*it)->IsSet(key)) {
			return (*it)->GetString(key);
		}
	}
	throw std::runtime_error("ConfigHandler: Error: Key does not exist: " + key +
			"\nPlease add the key to the list of allowed configuration values.");
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
	if (IsSet(key) && GetString(key) == value) {
		return;
	}

	if (useOverlay) {
		overlay->SetString(key, value);
	}
	else {
		// if we set something to be persisted,
		// we do want to override the overlay value
		overlay->Delete(key);

		// FIXME: this needs more subtle code
		// i.e. delete the key from overriding config files when it becomes equal to the default specified in another source
		writableSource->SetString(key, value);
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
	for (StringMap::const_iterator ut = changedValues.begin(); ut != changedValues.end(); ++ut) {
		const string& key = ut->first;
		const string& value = ut->second;
		for (list<ConfigNotifyCallback>::const_iterator it = observers.begin(); it != observers.end(); ++it) {
			(*it)(key, value);
		}
	}
	changedValues.clear();
}

string ConfigHandlerImpl::GetConfigFile() const {
	return writableSource->GetFilename();
}

const StringMap ConfigHandlerImpl::GetData() const {
	StringMap data;
	for_each_source_const(it) {
		const StringMap& sourceData = (*it)->GetData();
		// insert doesn't overwrite, so this preserves correct overrides
		data.insert(sourceData.begin(), sourceData.end());
	}
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
