/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include "GlobalConfig.h"
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <stdio.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

#ifdef __FreeBSD__
#include <sys/stat.h>
#endif

/**
 * @brief config handler base
 *
 * This is the abstract configuration handler class used
 * for polymorphic configuration.  Platform-specifics should derive
 * from this.
 */
class ConfigHandler
{
	typedef boost::function<void(const std::string&, const std::string&)> ConfigNotifyCallback;
public:
	template<class T>
	void NotifyOnChange(T* observer)
	{
		// issues: still needs to call configHandler->Get() on startup, automate it
		boost::mutex::scoped_lock lck(observerMutex);
		observers.push_back(boost::bind(&T::ConfigNotify, observer, _1, _2));
	};

	/**
	 * @brief set string
	 * @param name name of key to set
	 * @param value string value to set
	 * @param useOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game
	 */
	void SetString(std::string name, std::string value, bool useOverlay = false);

	/**
	 * @brief set configure option for this instance only
	 * @deprecated use instead: SetString(name, value, true)
	 */
	void SetOverlay(std::string name, std::string value);

	/**
	 * @brief get string
	 * @param name name of key to get
	 * @param def default string value to use if key is not found
	 * @param setInOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game.
	 *        This only has an effect if the value was not yet set.
	 * @return string value
	 */
	std::string GetString(std::string name, std::string def, bool setInOverlay = false);

	/// @see SetString
	template<typename T>
	void Set(const std::string& name, const T& value, bool useOverlay = false)
	{
		std::ostringstream buffer;
		buffer << value;
		SetString(name, buffer.str(), useOverlay);
	}

	/// @see GetString
	template<typename T>
	T Get(const std::string& name, const T& def, bool setInOverlay = false)
	{
		if (IsSet(name)) {
			std::istringstream buf(GetString(name, ""/* will not be used */, setInOverlay));
			T temp;
			buf >> temp;
			return temp;
		} else {
			Set(name, def, setInOverlay);
			return def;
		}
	}

	bool IsSet(const std::string& key) const;

	void Delete(const std::string& name, bool inOverlay = false);

	/**
	 * @brief instantiate global configHandler
	 * @param configSource the config file to be used, using the default one if empty
	 * @return name of the configfile used
	 * @see GetDefaultConfig()
	 */
	static std::string Instantiate(std::string configSource = "");

	std::string GetConfigFile() const
	{
		return filename;
	}

	/**
	 * @brief deallocate
	 */
	static void Deallocate();

	const std::map<std::string, std::string>& GetData() const;

	/**
	 * @brief update
	 * calls observers if configs changed
	 */
	void Update();

private:
	ConfigHandler(const std::string& configFile);
	~ConfigHandler();

	/**
	 * @brief config file name
	 */
	std::string filename;

	/**
	 * @brief data map
	 *
	 * Map used to internally cache data
	 * instead of constantly rereading from the file.
	 * This is to mirror the config file.
	 */
	std::map<std::string, std::string> data;

	/**
	 * @brief config overlay
	 *
	 * Will not be written to file, and will thus be discarded
	 * when the game ends.
	 */
	std::map<std::string, std::string> overlay;

	/**
	 * @brief Get the name of the default configuration file
	 */
	static std::string GetDefaultConfig();

	// helper functions
	void Read(FILE* file);
	void Write(FILE* file);
	char* Strip(char* begin, char* end);
	void AppendLine(char* line);

	// observer related
	std::list<ConfigNotifyCallback> observers;
	boost::mutex observerMutex;
	std::map<std::string, std::string> changedValues;
};

extern ConfigHandler* configHandler;

#endif /* CONFIGHANDLER_H */
