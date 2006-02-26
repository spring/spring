/*
 * ConfigHandler.cpp
 * Definition of PListHandler class
 * Copyright (C) 2006 Lorenz Pretterhofer
 */

#ifndef USERDEFSHANDLER_H
#define USERDEFSHANDLER_H

#include "Platform/ConfigHandler.h"

#import "Foundation/NSDictionary.h"

class UserDefsHandler: public ConfigHandler
{
public:
	UserDefsHandler();
	virtual ~UserDefsHandler();
	virtual void SetInt(std::string name, unsigned int value);
	virtual void SetString(std::string name, std::string value);
	virtual std::string GetString(std::string name, std::string def);
	virtual unsigned int GetInt(std::string name, unsigned int def);
private:
	NSMutableDictionary *dictionary_;
};

#endif // USERDEFSHANDLER_H
