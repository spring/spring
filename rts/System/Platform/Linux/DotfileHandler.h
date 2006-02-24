/*
 * DotfileHandler.h
 * DotfileHandler configuration class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#ifndef _DOTFILEHANDLER_H
#define _DOTFILEHANDLER_H

#include "Platform/ConfigHandler.h"

#include <string>
#include <fstream>
#include <map>

using std::string;

#define DOTCONFIGFILE ".springrc"
#define DOTCONFIGPATH (string(getenv("HOME")).append("/").append(DOTCONFIGFILE))

class DotfileHandler: public ConfigHandler
{
public:
	DotfileHandler(string fname);
	virtual ~DotfileHandler();
	virtual void SetInt(std::string name, unsigned int value);
	virtual void SetString(std::string name, std::string value);
	virtual std::string GetString(std::string name, std::string def);
	virtual unsigned int GetInt(std::string name, unsigned int def);
protected:
	std::string filename;
	std::ofstream file;
	std::map<string,string> data;
	virtual void flushfile(void);
	virtual void truncatefile(void);
	
};

//extern DotfileHandler regHandler;

#endif /* _DOTFILEHANDLER_H */
