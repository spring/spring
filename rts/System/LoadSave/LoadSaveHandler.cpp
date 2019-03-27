/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoadSaveHandler.h"
#include "CregLoadSaveHandler.h"
#include "LuaLoadSaveHandler.h"
#include "Game/GameSetup.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"


ILoadSaveHandler* ILoadSaveHandler::CreateHandler(const std::string& saveFile)
{
	const std::string& ext = FileSystem::GetExtension(saveFile);

	if (ext == "ssf")
		return (new CCregLoadSaveHandler());
	if (ext == "slsf")
		return (new CLuaLoadSaveHandler());

	throw content_error("Unknown save extension: " + ext);
}

bool ILoadSaveHandler::CreateSave(
	const std::string& saveFile,
	const std::string& saveArgs
) {
	if (!FileSystem::CreateDirectory("Saves"))
		return false;

	if (saveArgs != "-y" && FileSystem::FileExists(saveFile)) {
		LOG_L(L_WARNING, "[ILoadSaveHandler::%s] file \"%s\" already exists (use /save -y to override)", __func__, saveFile.c_str());
		return false;
	}

	ILoadSaveHandler* ls = CreateHandler(saveFile);

	ls->SaveInfo(gameSetup->mapName, gameSetup->mapName);
	ls->SaveGame(saveFile);
	LOG("[ILoadSaveHandler::%s] saved game to file \"%s\"", __func__, saveFile.c_str());
	delete ls;
	return true;
}

std::string ILoadSaveHandler::FindSaveFile(const std::string& file)
{
	if (FileSystem::FileExists(file))
		return file;

	return (FileSystem::EnsurePathSepAtEnd("Saves") + file);
}

