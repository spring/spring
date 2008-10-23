/**
 * @file ConfigHandler.cpp
 * @brief config implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Implementation of config structure class
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include "ConfigHandler.h"
#ifdef _WIN32
#include "Win/RegHandler.h"
#elif defined(__APPLE__)
extern "C" void PreInitMac();
#include "Mac/UserDefsHandler.h"
#else
#include "Linux/DotfileHandler.h"
#endif
#include <sstream>
#include "Game/GameVersion.h"


/**
 * @brief instance
 *
 * Default instantiation of ConfigHandler instance
 * is NULL.
 */
#ifndef USE_GML // GML calls GetInstance() from a global scope and needs these to be initialized in gml.cpp instead to avoid crash
ConfigHandler* ConfigHandler::instance = NULL;
std::string ConfigHandler::configSource;
#endif

/**
 * Returns reference to the current platform's config class.
 * If none exists, create one.
 */
ConfigHandler& ConfigHandler::GetInstance()
{
	if (!instance) {
		if (configSource.empty()) {
#ifdef _WIN32
			configSource = "Software\\SJ\\Spring";
			std::string version(VERSION_STRING);
			if (version.size()>0 && version[version.size()-1] == '+')
				configSource += " SVN";
#elif defined(__APPLE__)
			configSource = "this string is not currently used";
#else
			configSource = DotfileHandler::GetDefaultConfig();
#endif
		}

#ifdef _WIN32
		instance = SAFE_NEW RegHandler(configSource);
#elif defined(__APPLE__)
		PreInitMac();
		instance = SAFE_NEW UserDefsHandler(); // Config path is based on bundle id
#else
		instance = SAFE_NEW DotfileHandler(configSource);
#endif
	}
	return *instance;
}


/**
 * Destroys existing ConfigHandler instance.
 */
void ConfigHandler::Deallocate()
{
	if (instance)
		delete instance;
	instance = 0;
}


ConfigHandler::~ConfigHandler()
{
}


float ConfigHandler::GetFloat(const std::string& name, const float def)
{
	std::ostringstream buf1;
	buf1 << def;

	std::istringstream buffer(GetString(name, buf1.str()));
	float val;
	buffer >> val;
	return val;
}


void ConfigHandler::SetFloat(const std::string& name, float value)
{
	std::ostringstream buffer;
	buffer << value;

	SetString(name, buffer.str());
}


bool ConfigHandler::SetConfigSource(const std::string& source)
{
	configSource = source;
	return true;
}


const std::string& ConfigHandler::GetConfigSource()
{
	return configSource;
}
