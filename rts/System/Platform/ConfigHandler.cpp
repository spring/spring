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


ConfigHandler* _configHandler;


/**
 * Instantiates a copy of the current platform's config class.
 * Re-instantiates if the configHandler already existed.
 */
void ConfigHandler::Instantiate(std::string configSource)
{
	Deallocate();

	if (configSource.empty()) {
#ifdef _WIN32
		configSource = "Software\\SJ\\Spring";
		const std::string version = SpringVersion::Get();
		if (version.size()>0 && version[version.size()-1] == '+')
			configSource += " SVN";
#elif defined(__APPLE__)
		configSource = "this string is not currently used";
#else
		configSource = DotfileHandler::GetDefaultConfig();
#endif
	}

#ifdef _WIN32
	_configHandler = SAFE_NEW RegHandler(configSource);
#elif defined(__APPLE__)
	PreInitMac();
	_configHandler = SAFE_NEW UserDefsHandler(); // Config path is based on bundle id
#else
	_configHandler = SAFE_NEW DotfileHandler(configSource);
#endif
}


/**
 * Destroys existing ConfigHandler instance.
 */
void ConfigHandler::Deallocate()
{
	// can not use SafeDelete because ~ConfigHandler is protected
	delete _configHandler;
	_configHandler = NULL;
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
