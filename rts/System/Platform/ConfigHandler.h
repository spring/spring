/*
 * ConfigHandler.h
 * Definition of config structure class
 * Copyright (C) 2005 Christopher Han
 */
#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>

#define configHandler (ConfigHandler::GetInstance())

class ConfigHandler
{
public:
	virtual void SetInt(std::string name, unsigned int value) = 0;
	virtual void SetString(std::string name, std::string value) = 0;
	virtual std::string GetString(std::string name, std::string def) = 0;
	virtual unsigned int GetInt(std::string name, unsigned int def) = 0;
	static ConfigHandler& GetInstance();
	static void Deallocate();
protected:
	static ConfigHandler* instance;
};

#endif /* CONFIGHANDLER_H */
