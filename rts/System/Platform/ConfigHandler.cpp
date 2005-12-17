/*
 * ConfigHandler.cpp
 * Implementation of config structure class
 * Copyright (C) 2005 Christopher Han
 */
#include "ConfigHandler.h"
#ifdef _WIN32
#include "Win/RegHandler.h"
#else
#include "Linux/dotfileHandler.h"
#endif

ConfigHandler* ConfigHandler::instance=0;

ConfigHandler& ConfigHandler::GetInstance()
{
	if (!instance) {
#ifdef _WIN32
		instance = new RegHandler("Software\\SJ\\spring");
#else
		instance = new dotfileHandler(DOTCONFIGPATH);
#endif
	}
	return *instance;
}

void ConfigHandler::Deallocate()
{
	delete instance;
	instance=0;
}
