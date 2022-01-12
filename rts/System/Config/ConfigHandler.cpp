/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ConfigHandler.h"
#include "ConfigLocater.h"
#include "ConfigSource.h"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/UnorderedMap.hpp"
#include "System/Threading/SpringThreading.h"

#include <cstdio>
#include <cstring>

#include <stdexcept>

/******************************************************************************/

typedef std::map<std::string, std::string> StringMap;

ConfigHandler* configHandler = nullptr;


/******************************************************************************/

class ConfigHandlerImpl : public ConfigHandler
{
public:
	ConfigHandlerImpl(const std::vector<std::string>& locations, bool safemode);
	~ConfigHandlerImpl() override;

	void SetString(const std::string& key, const std::string& value, bool useOverlay) override;
	std::string GetString(const std::string& key) const override;
	bool IsSet(const std::string& key) const override;
	bool IsReadOnly(const std::string& key) const override;
	void Delete(const std::string& key) override;
	std::string GetConfigFile() const override;
	const StringMap GetData() const override;
	StringMap GetDataWithoutDefaults() const override;
	void Update() override;
	void EnableWriting(bool write) override { writingEnabled = write; }

protected:
	struct NamedConfigNotifyCallback {
		NamedConfigNotifyCallback(ConfigNotifyCallback c, void* o): callback(c), observer(o) {}

		ConfigNotifyCallback callback;
		void* observer;
	};

protected:
	void AddObserver(ConfigNotifyCallback callback, void* observer, const std::vector<std::string>& configs) override;
	void RemoveObserver(void* observer) override;

private:
	void RemoveDefaults();

	OverlayConfigSource* overlay;
	FileConfigSource* writableSource;
	std::vector<ReadOnlyConfigSource*> sources;

	// observer related
	spring::unsynced_map<std::string, std::vector<NamedConfigNotifyCallback>> configsToCallbacks;
	spring::unsynced_map<void*, std::vector<std::string>> observersToConfigs;
	spring::mutex observerMutex;
	StringMap changedValues;
	bool writingEnabled;
};

/******************************************************************************/


/**
 * @brief Fills the list of sources based on locations.
 *
 * Sources at lower indices take precedence over sources at higher indices.
 * First there is the overlay, then one or more file sources, and the last
 * source(s) specify default values.
 */
ConfigHandlerImpl::ConfigHandlerImpl(const std::vector<std::string>& locations, const bool safemode)
{
	writingEnabled = true;
	overlay = new OverlayConfigSource();
	writableSource = new FileConfigSource(locations.front());

	size_t sources_num = 3;
	sources_num += (safemode) ? 1 : 0;
	sources_num += locations.size() - 1;
#ifdef DEDICATED
	sources_num++;
#endif
#ifdef HEADLESS
	sources_num++;
#endif
	sources.reserve(sources_num);

	sources.push_back(overlay);

	if (safemode) {
		sources.push_back(new SafemodeConfigSource());
	}

	sources.push_back(writableSource);

	// skip writableSource
	for (auto loc = ++(locations.cbegin()); loc != locations.cend(); ++loc) {
		sources.push_back(new FileConfigSource(*loc));
	}
#ifdef DEDICATED
	sources.push_back(new DedicatedConfigSource());
#endif
#ifdef HEADLESS
	sources.push_back(new HeadlessConfigSource());
#endif
	sources.push_back(new DefaultConfigSource());

	assert(sources.size() <= sources_num);

	// Perform migrations that need to happen on every load.
	RemoveDefaults();
}

ConfigHandlerImpl::~ConfigHandlerImpl()
{
	//all observers have to be deregistered by RemoveObserver()
	assert(configsToCallbacks.empty());
	assert(observersToConfigs.empty());

	for (ReadOnlyConfigSource* s: sources) {
		delete s;
	}
}

/**
 * @brief Remove config variables that are set to their default value.
 *
 * Algorithm is as follows:
 *
 * 1) Take all defaults from the DefaultConfigSource
 * 2) Go in reverse through the FileConfigSources
 * 3) For each one:
 *  3a) delete setting if equal to the one in `defaults'
 *  3b) add new value to `defaults' if different
 */
void ConfigHandlerImpl::RemoveDefaults()
{
	StringMap defaults = sources.back()->GetData();

	for (auto rsource = sources.crbegin(); rsource != sources.crend(); ++rsource) {
		FileConfigSource* source = dynamic_cast<FileConfigSource*> (*rsource);

		if (source == nullptr)
			continue;

		// Copy the map; we modify the original while iterating over the copy.
		const StringMap file = source->GetData();

		for (const auto& item: file) {
			// Does the key exist in `defaults'?
			const auto pos = defaults.find(item.first);

			if (pos != defaults.end() && pos->second == item.second) {
				// Exists and value is equal => Delete.
				source->Delete(item.first);
			} else {
				// Doesn't exist or is not equal => Store new default.
				// (It will be the default for the next FileConfigSource.)
				defaults[item.first] = item.second;
			}
		}
	}
}


StringMap ConfigHandlerImpl::GetDataWithoutDefaults() const
{
	StringMap cleanConfig;
	StringMap defaults = sources.back()->GetData();

	for (auto rsource = sources.crbegin(); rsource != sources.crend(); ++rsource) {
		const FileConfigSource* source = dynamic_cast<const FileConfigSource*> (*rsource);

		if (source == nullptr)
			continue;

		// Copy the map; we modify the original while iterating over the copy.
		const StringMap file = source->GetData();

		for (const auto& item: file) {
			const auto pos = defaults.find(item.first);
			if (pos != defaults.end() && pos->second == item.second)
				continue;

			cleanConfig[item.first] = item.second;
		}
	}

	return cleanConfig;
}


void ConfigHandlerImpl::Delete(const std::string& key)
{
	for (ReadOnlyConfigSource* s: sources) {
		// The alternative to the dynamic cast is to merge ReadWriteConfigSource
		// with ReadOnlyConfigSource, but then DefaultConfigSource would have to
		// violate Liskov Substitution Principle... (by blocking modifications)
		ReadWriteConfigSource* rwcs = dynamic_cast<ReadWriteConfigSource*>(s);

		if (rwcs == nullptr)
			continue;

		rwcs->Delete(key);
	}
}

bool ConfigHandlerImpl::IsSet(const std::string& key) const
{
	for (const ReadOnlyConfigSource* s: sources) {
		if (s->IsSet(key)) return true;
	}

	return false;
}

bool ConfigHandlerImpl::IsReadOnly(const std::string& key) const
{
	const ConfigVariableMetaData* meta = ConfigVariable::GetMetaData(key);

	if (meta == nullptr)
		return false;

	return (meta->GetReadOnly().Get());
}

std::string ConfigHandlerImpl::GetString(const std::string& key) const
{
	const ConfigVariableMetaData* meta = ConfigVariable::GetMetaData(key);

	for (const ReadOnlyConfigSource* s: sources) {
		if (s->IsSet(key)) {
			std::string value = s->GetString(key);

			if (meta != nullptr)
				value = meta->Clamp(value);

			return value;
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
 * 3) Set data[key] = value.
 * 4) Write file (so we keep the settings in the event of a crash or error)
 * 5) Unlock file.
 *
 * We do not want conflicts when multiple instances are running
 * at the same time (which would cause data loss).
 * This would happen if e.g. unitsync and spring would access
 * the config file at the same time, if we would not lock.
 */
void ConfigHandlerImpl::SetString(const std::string& key, const std::string& value, bool useOverlay)
{
	// if we set something to be persisted,
	// we do want to override the overlay value
	if (!useOverlay)
		overlay->Delete(key);

	// Don't do anything if value didn't change.
	if (IsSet(key) && GetString(key) == value)
		return;

	if (useOverlay) {
		overlay->SetString(key, value);
	} else if (writingEnabled) {
		std::vector<ReadOnlyConfigSource*>::const_iterator it = sources.begin();

		++it; // skip overlay
		++it; // skip writableSource

		bool deleted = false;

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

	std::lock_guard<spring::mutex> lck(observerMutex);
	changedValues[key] = value;
}

void ConfigHandlerImpl::Update()
{
	std::lock_guard<spring::mutex> lck(observerMutex);

	for (StringMap::const_iterator ut = changedValues.begin(); ut != changedValues.end(); ++ut) {
		const std::string& key = ut->first;
		const std::string& value = ut->second;

		if (configsToCallbacks.find(key) != configsToCallbacks.end()) {
			for (const NamedConfigNotifyCallback& ncnc: configsToCallbacks[key]) {
				(ncnc.callback)(key, value);
			}
		}
	}

	changedValues.clear();
}

std::string ConfigHandlerImpl::GetConfigFile() const {
	return writableSource->GetFilename();
}

const StringMap ConfigHandlerImpl::GetData() const {
	StringMap data;
	for (const ReadOnlyConfigSource* s: sources) {
		const StringMap& sourceData = s->GetData();
		// insert doesn't overwrite, so this preserves correct overrides
		data.insert(sourceData.begin(), sourceData.end());
	}
	return data;
}


void ConfigHandlerImpl::AddObserver(ConfigNotifyCallback callback, void* observer, const std::vector<std::string>& configs) {
	std::lock_guard<spring::mutex> lck(observerMutex);

	for (const std::string& config: configs) {
		configsToCallbacks[config].emplace_back(callback, observer);
		observersToConfigs[observer].push_back(config);
	}
}

void ConfigHandlerImpl::RemoveObserver(void* observer) {
	std::lock_guard<spring::mutex> lck(observerMutex);

	for (const std::string& config: observersToConfigs[observer]) {
		spring::VectorEraseIf(configsToCallbacks[config], [&](NamedConfigNotifyCallback& ncnc) {
			return (ncnc.observer == observer);
		});

		if (configsToCallbacks[config].empty())
			configsToCallbacks.erase(config);
	}

	observersToConfigs.erase(observer);
}


/******************************************************************************/

void ConfigHandler::Instantiate(const std::string configSource, const bool safemode)
{
	Deallocate();

	std::vector<std::string> locations;
	if (!configSource.empty()) {
		locations.push_back(configSource);
	} else {
		ConfigLocater::GetDefaultLocations(locations);
	}
	assert(!locations.empty());

	// log here so unitsync shows configuration source(s), too
	LOG("Using writeable configuration source: \"%s\"", locations[0].c_str());
	for (auto loc = ++(locations.cbegin()); loc != locations.cend(); ++loc) {
		LOG("Using additional read-only configuration source: \"%s\"", loc->c_str());
	}

	configHandler = new ConfigHandlerImpl(locations, safemode);

	//assert(configHandler->GetString("test") == "x y z");
}

void ConfigHandler::Deallocate()
{
	spring::SafeDelete(configHandler);
}

bool ConfigHandler::Get(const std::string& key) const
{
	return StringToBool(GetString(key));
}


/******************************************************************************/
