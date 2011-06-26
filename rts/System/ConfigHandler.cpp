/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Stack of maps
 *
 * - Maps earlier in the chain override later ones
 * - Last one contains the static defaults
 * - First one is what is now called the "overlay"
 * - A value identical to the value later in the chain is not stored
 *
 * The sources are (later sources override earlier sources):
 * - Static defaults (might still differ per user/PC)
 * - Sources from ConfigLocater::GetDefaultLocations()
 * - Overlay (i.e. config values from start script)
 */

#include "StdAfx.h"
#include "Util.h"
#include "ConfigHandler.h"
#include "ConfigSource.h"

#ifdef WIN32
	#include <io.h>
#endif
#include <stdio.h>
#include <string.h>

#include <list>
#include <stdexcept>

#include <boost/thread/mutex.hpp>

#include "Platform/ConfigLocater.h"
#include "LogOutput.h"

/******************************************************************************/

using std::list;
using std::map;
using std::string;
using std::vector;

typedef map<string, string> StringMap;

ConfigHandler* configHandler = NULL;

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
	++loc; // skip writableSource
	for (; loc != locations.end(); ++loc) {
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
	// if we set something to be persisted,
	// we do want to override the overlay value
	if (!useOverlay) {
		overlay->Delete(key);
	}

	// Don't do anything if value didn't change.
	if (IsSet(key) && GetString(key) == value) {
		return;
	}

	if (useOverlay) {
		overlay->SetString(key, value);
	}
	else {
		vector<ConfigSource*>::const_iterator it = sources.begin();
		bool deleted = false;

		++it; // skip overlay
		++it; // skip writableSource

		for (; it != sources.end(); ++it) {
			if ((*it)->IsSet(key)) {
				if ((*it)->GetString(key) == value) {
					// key is being set to the default value,
					// delete the key instead of setting it.
					writableSource->Delete(key);
					deleted = true;
				}
				break;
			}
		}

		if (!deleted) {
			// set key to the specified non-default value
			writableSource->SetString(key, value);
		}
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
