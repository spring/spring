/**
 * @file ConfigHandler.h
 * @brief config base
 * @author Christopher Han <xiphux@gmail.com>
 * 
 * Definition of config structure class
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>

/**
 * @brief configHandler
 *
 * Just a shortcut to prevent having to replace
 * configHandler throughout the entire engine
 */
#define configHandler (ConfigHandler::GetInstance())

/**
 * @brief config handler base
 * 
 * This is the abstract configuration handler class used
 * for polymorphic configuration.  Platform-specifics should derive
 * from this.
 */
class ConfigHandler
{
public:
	/**
	 * @brief set integer
	 * @param name name of key to set
	 * @param value integer value to set
	 */
	virtual void SetInt(std::string name, int value) = 0;

	/**
	 * @brief set string
	 * @param name name of key to set
	 * @param value string value to set
	 */
	virtual void SetString(std::string name, std::string value) = 0;

	/**
	 * @brief get string
	 * @param name name of key to get
	 * @param def default string value to use if key is not found
	 * @return string value
	 */
	virtual std::string GetString(std::string name, std::string def) = 0;

	/**
	 * @brief get integer
	 * @param name name of key to get
	 * @param def default integer value to use if key is not found
	 * @return integer value
	 */
	virtual int GetInt(std::string name, int def) = 0;
	
	/**
	@brief get float value
	*/
	virtual float GetFloat(const std::string& name, const float def);

	/**
	@brief set float value
	*/
	virtual void SetFloat(const std::string& name, float value);

	/**
	 * @brief get instance
	 * @return reference to current platform's ConfigHandler
	 */
	static ConfigHandler& GetInstance();

	static bool SetConfigSource(const std::string& name);
	static const std::string& GetConfigSource();

	/**
	 * @brief deallocate
	 */
	static void Deallocate();

	virtual ~ConfigHandler();

protected:
	static std::string configSource;

	/**
	 * @brief instance
	 *
	 * ConfigHandler pointer pointing to current platform's
	 * ConfigHandler instance
	 */
	static ConfigHandler* instance;
};

#endif /* CONFIGHANDLER_H */
