/* Author: Tobi Vollebregt */

#ifndef LUALOADSAVEHANDLER_H
#define LUALOADSAVEHANDLER_H

#include "LoadSaveHandler.h"

class CArchiveBase;


class CLuaLoadSaveHandler : public ILoadSaveHandler
{
public:
	CLuaLoadSaveHandler();
	~CLuaLoadSaveHandler();

	void SaveGame(const std::string& file);
	void LoadGameStartInfo(const std::string& file);
	void LoadGame();

protected:
	std::string LoadEntireFile(const std::string& file);

	CArchiveBase* loadfile;
};

#endif // LUALOADSAVEHANDLER_H
