/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LUA_LOAD_SAVE_HANDLER_H
#define _LUA_LOAD_SAVE_HANDLER_H

#include "LoadSaveHandler.h"

class IArchive;

#ifndef zipFile
	// might be defined through zip.h already
	typedef void* zipFile;
#endif


class CLuaLoadSaveHandler : public ILoadSaveHandler
{
public:
	CLuaLoadSaveHandler();
	~CLuaLoadSaveHandler();

	void SaveGame(const std::string& file);
	void LoadGameStartInfo(const std::string& file);
	void LoadGame();

protected:
	void SaveEventClients(); // Lua
	void SaveGameStartInfo();
	void SaveAIData();
	void SaveHeightmap();
	void SaveEntireFile(const char* file, const char* what, const void* data, int size, bool throwOnError = false);
	void LoadEventClients();
	void LoadAIData();
	void LoadHeightmap();
	std::string LoadEntireFile(const std::string& file);

	std::string filename;
	zipFile savefile;
	IArchive* loadfile;
};

#endif // _LUA_LOAD_SAVE_HANDLER_H
