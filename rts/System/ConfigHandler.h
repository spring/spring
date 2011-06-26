/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <stdio.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

/**
 * @brief config handler base
 */
class ConfigHandler
{
public:
	/**
	 * @brief instantiate global configHandler
	 * @param configSource the config file to be used, using the default one if empty
	 * @see GetDefaultConfig()
	 */
	static void Instantiate(std::string configSource = "");

	/**
	 * @brief deallocate
	 */
	static void Deallocate();

public:
	template<class T>
	void NotifyOnChange(T* observer)
	{
		// issues: still needs to call configHandler->Get() on startup, automate it
		AddObserver(boost::bind(&T::ConfigNotify, observer, _1, _2));
	};

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

public:
	/**
	 * @brief set string
	 * @param name name of key to set
	 * @param value string value to set
	 * @param useOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game
	 */
	virtual void SetString(std::string name, std::string value, bool useOverlay = false) = 0;

	/**
	 * @brief get string
	 * @param name name of key to get
	 * @param def default string value to use if key is not found
	 * @param setInOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game.
	 *        This only has an effect if the value was not yet set.
	 * @return string value
	 */
	virtual std::string GetString(std::string name, std::string def, bool setInOverlay = false) = 0;

	virtual bool IsSet(const std::string& key) const = 0;

	virtual void Delete(const std::string& name) = 0;

	virtual std::string GetConfigFile() const = 0;

	virtual const std::map<std::string, std::string>& GetData() const = 0;

	/**
	 * @brief update
	 * calls observers if configs changed
	 */
	virtual void Update() = 0;

protected:
	typedef boost::function<void(const std::string&, const std::string&)> ConfigNotifyCallback;

	virtual void AddObserver(ConfigNotifyCallback observer) = 0;
};

extern ConfigHandler* configHandler;

#endif /* CONFIGHANDLER_H */
