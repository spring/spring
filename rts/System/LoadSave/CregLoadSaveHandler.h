/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CREG_LOAD_SAVE_HANDLER_H
#define CREG_LOAD_SAVE_HANDLER_H

#include <string>
#include <sstream>
#include "LoadSaveHandler.h"

class CCregLoadSaveHandler : public ILoadSaveHandler
{
public:
	CCregLoadSaveHandler();
	~CCregLoadSaveHandler();
	void SaveGame(const std::string& path);
	/// load things such as map and mod, needed to fire up the engine
	void LoadGameStartInfo(const std::string& path);
	void LoadGame();

protected:
	std::stringstream iss;
};

#endif // CREG_LOAD_SAVE_HANDLER_H
