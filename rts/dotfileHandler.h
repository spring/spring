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

#define regHandler (dotfileHandler::GetInstance())

class dotfileHandler
{
public:
	unsigned int GetInt(string name, unsigned int def);
	string GetString(string name, string def);
	void SetInt(string name, unsigned int value);
	void SetString(string name, string value);
	static dotfileHandler& GetInstance();
	static void Deallocate();
protected:
	dotfileHandler(string filename);
	~dotfileHandler();
	std::ofstream file;
	std::map<string,string> data;
	void flushfile(void);
	static dotfileHandler* instance;
};

//extern dotfileHandler regHandler;

#endif /* _DOTFILEHANDLER_H */
