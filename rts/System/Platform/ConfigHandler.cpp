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

/**
 * @brief instance
 *
 * Default instantiation of ConfigHandler instance
 * is NULL.
 */
ConfigHandler* ConfigHandler::instance=0;

/**
 * Returns reference to the current platform's config class.
 * If none exists, create one.
 */
ConfigHandler& ConfigHandler::GetInstance()
{
	if (!instance) {
#ifdef _WIN32
		instance = new RegHandler("Software\\SJ\\spring");
#elif defined(__APPLE__)
		PreInitMac();
		instance = new UserDefsHandler(); // Config path is based on bundle id
#else
		instance = new DotfileHandler(DOTCONFIGPATH);
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
	instance=0;
}

ConfigHandler::~ConfigHandler() {
}
