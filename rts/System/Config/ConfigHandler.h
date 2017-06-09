/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <functional>

#include "ConfigVariable.h"

/**
 * @brief Config handler interface
 */
class ConfigHandler
{
public:
	/**
	 * @brief Instantiate global configHandler
	 * @param configSource the config file to be used, using the default one if empty
	 *
	 * Re-instantiates if the configHandler already existed.
	 *
	 * Note: we don't use &ConfigSource, because showed in past that under some
	 *    conditions this default variable isn't initialized yet and so getting the
	 *    ref of it fails causing a crash.
	 */
	static void Instantiate(const std::string configSource = "", const bool safemode = false);

	/**
	 * @brief Deallocate global configHandler
	 */
	static void Deallocate();

public:
	/**
	 * @brief Register an observer
	 *
	 * The observer must have a method ConfigNotify, that takes a key-value-pair
	 * using two std::strings as argument. It is called whenever the config
	 * variable changes from within the current application.
	 *
	 * It is not called when the config variable is changed from another
	 * application and the new value was read in a read-modify-write cycle.
	 */
	template<class T>
	void NotifyOnChange(T* observer, const std::vector<std::string>& configs)
	{
		// issues: still needs to call configHandler->Get() on startup, automate it
		AddObserver(std::bind(&T::ConfigNotify, observer, std::placeholders::_1, std::placeholders::_2), (void*) observer, configs);
	}

	template<class T>
	void RemoveObserver(T* observer)
	{
		RemoveObserver((void*) observer);
	}


	/// @see SetString
	template<typename T>
	void Set(const std::string& key, const T& value, bool useOverlay = false)
	{
		std::ostringstream buffer;
		buffer << value;
		SetString(key, buffer.str(), useOverlay);
	}

	/// @brief Get bool, throw if key not present
	bool GetBool(const std::string& key) const { return (Get<bool>(key)); }
	/// @brief Get int, throw if key not present
	int GetInt(const std::string& key) const { return (Get<int>(key)); }
	/// @brief Get int, throw if key not present
	int GetUnsigned(const std::string& key) const { return (Get<unsigned>(key)); }
	/// @brief Get float, throw if key not present
	float GetFloat(const std::string& key) const { return (Get<float>(key)); }

	bool GetBoolSafe(const std::string& key, bool def) const { return (IsSet(key)? GetBool(key): def); }
	int GetIntSafe(const std::string& key, int def) const { return (IsSet(key)? GetInt(key): def); }
	float GetFloatSafe(const std::string& key, float def) const { return (IsSet(key)? GetFloat(key): def); }
	std::string GetStringSafe(const std::string& key, const std::string& def) const { return (IsSet(key)? GetString(key): def); }

public:
	virtual ~ConfigHandler() {}

	/**
	 * @brief Set string config value
	 * @param key name of key to set
	 * @param value string value to set
	 * @param useOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game
	 */
	virtual void SetString(const std::string& key, const std::string& value, bool useOverlay = false) = 0;

	/**
	 * @brief Get string config value
	 * @param key name of key to get
	 * @return string value
	 * @note Throws if key not present! (But if you specified
	 *       a default, then that will always be present.)
	 */
	virtual std::string GetString(const std::string& key) const = 0;

	/**
	 * @brief Queries whether config variable is set anywhere
	 * @param key name of key to query
	 * @return true if the key exists
	 * @note If the config variable has a default value then it is always set!
	 */
	virtual bool IsSet(const std::string& key) const = 0;
	/**
	 * @brief Queries if config variable is readonly
	 * @param key name of key to query
	 * @return true if key is read only
	 */
	virtual bool IsReadOnly(const std::string& key) const = 0;

	/**
	 * @brief Delete a config variable from all mutable config sources
	 * @param key name of key to query
	 */
	virtual void Delete(const std::string& key) = 0;

	/**
	 * @brief Get the name of the main (first) config file
	 */
	virtual std::string GetConfigFile() const = 0;

	/**
	 * @brief Get a map containing all key value pairs
	 * @note This includes default values!
	 */
	virtual const std::map<std::string, std::string> GetData() const = 0;

	/**
	 * @brief Get a map containing all key value pairs, which aren't default values
	 * @note This excludes default values!
	 */
	virtual std::map<std::string, std::string> GetDataWithoutDefaults() const = 0;

	/**
	 * @brief Calls observers if config values changed
	 */
	virtual void Update() = 0;

	/**
	 * @brief Toggles write protection for this confighandler
	 */
	virtual void EnableWriting(bool write) = 0;

protected:
	typedef std::function<void(const std::string&, const std::string&)> ConfigNotifyCallback;

	virtual void AddObserver(ConfigNotifyCallback callback, void* observer, const std::vector<std::string>& configs) = 0;
	virtual void RemoveObserver(void* observer) = 0;

private:
	/// @see GetString
	template<typename T>
	T Get(const std::string& key) const
	{
		std::istringstream buf(GetString(key));
		T temp;
		buf >> temp;
		return temp;
	}

	/// @see Get
	/// @brief <bool> specialization of Get<> (we cannot use template spezialization here, so just overload it)
	bool Get(const std::string& key) const;
};

extern ConfigHandler* configHandler;

#endif /* CONFIGHANDLER_H */
