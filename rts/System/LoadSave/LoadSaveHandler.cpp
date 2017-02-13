/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "LoadSaveHandler.h"
#include "CregLoadSaveHandler.h"
#include "LuaLoadSaveHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"


ILoadSaveHandler* ILoadSaveHandler::Create(bool usecreg)
{
	if (usecreg)
		return new CCregLoadSaveHandler();
	else
		return new CLuaLoadSaveHandler();
}


ILoadSaveHandler::~ILoadSaveHandler()
{
}


std::string ILoadSaveHandler::FindSaveFile(const std::string& file)
{
	if (FileSystem::FileExists(file)) {
		return file;
	}
	return FileSystem::EnsurePathSepAtEnd("Saves") + file;
}
