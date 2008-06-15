/**
 * @file DotfileHandler.h
 * @brief linux dotfile config base
 * @author Christopher Han <xiphux@gmail.com>
 * @author Tobi Vollebregt <tobivollebregt@gmail.com>
 *
 * DotfileHandler configuration class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#ifndef _DOTFILEHANDLER_H
#define _DOTFILEHANDLER_H

#include "Platform/ConfigHandler.h"

#ifdef __FreeBSD__
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string>
#include <map>

using std::string;

/**
 * @brief DotfileHandler
 *
 * Linux dotfile handler config class, derived
 * from the abstract ConfigHandler.
 */
class DotfileHandler: public ConfigHandler
{
public:

	/**
	 * @brief Constructor
	 * @param fname path to config file
	 */
	DotfileHandler(const std::string& fname);

	/**
	 * @brief Get the name of the default configuration file
	 */
	static string GetDefaultConfig();

	/**
	 * @brief Destructor
	 */
	virtual ~DotfileHandler();

	/**
	 * @brief set integer
	 * @param name name of key to set
	 * @param value integer value to set
	 */
	virtual void SetInt(std::string name, int value);

	/**
	 * @brief set string
	 * @param name name of key to set
	 * @param value string value to set
	 */
	virtual void SetString(std::string name, std::string value);

	/**
	 * @brief get string
	 * @param name name of key to get
	 * @param def default string value to use if key is not found
	 * @return string value
	 */
	virtual std::string GetString(std::string name, std::string def);

	/**
	 * @brief get integer
	 * @param name name of key to get
	 * @param def default integer value to use if key is not found
	 * @return integer value
	 */
	virtual int GetInt(std::string name, int def);

private:

	/**
	 * @brief config file name
	 */
	std::string filename;

	/**
	 * @brief data map
	 *
	 * Map used to internally cache data
	 * instead of constantly rereading from the file
	 */
	std::map<string,string> data;

	// helper functions
	void Read(FILE* file);
	void Write(FILE* file);
	char* Strip(char* begin, char* end);
	void AppendLine(char* buf);
};

#endif /* _DOTFILEHANDLER_H */
