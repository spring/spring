/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOADSAVEHANDLER_H
#define LOADSAVEHANDLER_H

class ILoadSaveHandler
{
public:
	static ILoadSaveHandler* Create();

protected:
	std::string FindSaveFile(const std::string& file);

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

#endif // LOADSAVEHANDLER_H
