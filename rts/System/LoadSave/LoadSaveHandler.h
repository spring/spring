/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOAD_SAVE_HANDLER_H
#define _LOAD_SAVE_HANDLER_H

#include <string>


class ILoadSaveHandler
{
public:
	static ILoadSaveHandler* CreateHandler(const std::string& saveFile);
	static bool CreateSave(
		const std::string& saveFile,
		const std::string& saveArgs,
		const std::string& mapName,
		const std::string& modName
	);

protected:
	static std::string FindSaveFile(const std::string& file);

public:
	virtual ~ILoadSaveHandler() = default;

	virtual void SaveGame(const std::string& file) = 0;
	/// load scriptText and (for creg saves) {map,mod}Name needed to fire up the engine
	virtual void LoadGameStartInfo(const std::string& file) = 0;
	virtual void LoadGame() = 0;

	void SaveInfo(const std::string& _mapName, const std::string& _modName) {
		mapName = _mapName;
		modName = _modName;
	}

	const std::string& GetScriptText() const { return scriptText; }

protected:
	std::string scriptText;
	std::string mapName;
	std::string modName;
};

#endif // _LOAD_SAVE_HANDLER_H
