/*
 * dotfileHandler.h
 * dotfileHandler configuration class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#ifndef _DOTFILEHANDLER_H
#define _DOTFILEHANDLER_H

#include "System/Platform/ConfigHandler.h"

#include <string>
#include <fstream>
#include <map>

using std::string;

#define DOTCONFIGFILE ".springrc"
#define DOTCONFIGPATH (string(getenv("HOME")).append("/").append(DOTCONFIGFILE))

class dotfileHandler: public ConfigHandler
{
public:
	dotfileHandler(string filename);
	virtual ~dotfileHandler();
	virtual void SetInt(std::string name, unsigned int value);
	virtual void SetString(std::string name, std::string value);
	virtual std::string GetString(std::string name, std::string def);
	virtual unsigned int GetInt(std::string name, unsigned int def);
protected:
	std::ofstream file;
	std::map<string,string> data;
	virtual void flushfile(void);
};

//extern dotfileHandler regHandler;

#endif /* _DOTFILEHANDLER_H */
