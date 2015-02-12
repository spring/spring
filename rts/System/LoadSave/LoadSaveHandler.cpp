/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "LoadSaveHandler.h"
#include "CregLoadSaveHandler.h"
#include "LuaLoadSaveHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"


CONFIG(bool, UseCREGSaveLoad).defaultValue(false);


ILoadSaveHandler* ILoadSaveHandler::Create()
{
	if (configHandler->GetBool("UseCREGSaveLoad"))
		return new CCregLoadSaveHandler();
	else
		return new CLuaLoadSaveHandler();
}


ILoadSaveHandler::~ILoadSaveHandler()
{
}


std::string ILoadSaveHandler::FindSaveFile(const std::string& file)
{
	if (!FileSystem::IsAbsolutePath(file)) {
		return FileSystem::EnsurePathSepAtEnd("Saves") + file;
	}

	return file;
}
