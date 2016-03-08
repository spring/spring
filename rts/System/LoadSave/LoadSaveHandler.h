/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOAD_SAVE_HANDLER_H
#define _LOAD_SAVE_HANDLER_H

#include <string>


class ILoadSaveHandler
{
public:
	static ILoadSaveHandler* Create(bool usecreg);

protected:
	static std::string FindSaveFile(const std::string& file);

public:
	virtual ~ILoadSaveHandler();

	virtual void SaveGame(const std::string& file) = 0;
	/// load things such as map and mod, needed to fire up the engine
	virtual void LoadGameStartInfo(const std::string& file) = 0;
	virtual void LoadGame() = 0;

	std::string scriptText;
	std::string mapName;
	std::string modName;
};

#endif // _LOAD_SAVE_HANDLER_H
