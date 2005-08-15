/*
 * dotfileHandler.h
 * dotfileHandler configuration class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#ifndef _DOTFILEHANDLER_H
#define _DOTFILEHANDLER_H

#include <windows.h>
#include <string>
#include <fstream>
#include <map>

using std::string;

#define DOTCONFIGFILE ".springrc"
#define DOTCONFIGPATH (string(getenv("HOME")).append("/").append(DOTCONFIGFILE))

class dotfileHandler
{
public:
	dotfileHandler(string filename);
	~dotfileHandler();
	unsigned int GetInt(string name, unsigned int def);
	string GetString(string name, string def);
	void SetInt(string name, unsigned int value);
	void SetString(string name, string value);
private:
	std::ofstream file;
	std::map<string,string> data;
	void flushfile(void);
};

extern dotfileHandler regHandler;

#endif /* _DOTFILEHANDLER_H */
