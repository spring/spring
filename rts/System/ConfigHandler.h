/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>
#include <sstream>
#include <map>

#include <boost/function.hpp>
#include <boost/bind.hpp>

/**
 * @brief Configuration value declaration
 */
class ConfigValue
{
public:
	template<typename T>
	ConfigValue(const std::string& key, const T& value, const std::string& comment = ""):
			next(head), key(key), value(ToString(value)), comment(comment)
	{
		head = this;
	}

private:
	template<typename T>
	static std::string ToString(const T& value)
	{
		std::ostringstream buffer;
		buffer << value;
		return buffer.str();
	}

	static ConfigValue* head;

private:
	ConfigValue* next;
	std::string key;
	std::string value;
	std::string comment;

	friend class ConfigHandler;
};

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
	void Set(const std::string& key, const T& value, bool useOverlay = false)
	{
		std::ostringstream buffer;
		buffer << value;
		SetString(key, buffer.str(), useOverlay);
	}

	bool  GetBool(const std::string& key)  const { return Get<bool>(key); }
	int   GetInt(const std::string& key)   const { return Get<int>(key); }
	float GetFloat(const std::string& key) const { return Get<float>(key); }

public:
	/**
	 * @brief set string
	 * @param name name of key to set
	 * @param value string value to set
	 * @param useOverlay if true, the value will only be set in memory,
	 *        and therefore be lost for the next game
	 */
	virtual void SetString(const std::string& name, const std::string& value, bool useOverlay = false) = 0;

	/**
	 * @brief get string
	 * @param name name of key to get
	 * @return string value
	 */
	virtual std::string GetString(const std::string& key) const = 0;

	virtual bool IsSet(const std::string& key) const = 0;

	virtual void Delete(const std::string& key) = 0;

	virtual std::string GetConfigFile() const = 0;

	virtual const std::map<std::string, std::string> GetData() const = 0;

	/**
	 * @brief update
	 * calls observers if configs changed
	 */
	virtual void Update() = 0;

protected:
	typedef boost::function<void(const std::string&, const std::string&)> ConfigNotifyCallback;

	virtual void AddObserver(ConfigNotifyCallback observer) = 0;

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
};

extern ConfigHandler* configHandler;

#endif /* CONFIGHANDLER_H */
