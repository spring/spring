#ifndef LOADSAVEHANDLER_H
#define LOADSAVEHANDLER_H

#include <string>
#include <fstream>

class CLoadInterface;

class CLoadSaveHandler
{
public:
	CLoadSaveHandler();
	~CLoadSaveHandler();
	void SaveGame(const std::string& file);
	/// load things such as map and mod, needed to fire up the engine
	void LoadGameStartInfo(const std::string& file);
	void LoadGame(); 
	std::string FindSaveFile(const char* name);

	std::string scriptText;
	std::string mapName;
	std::string modName;
protected:
	std::ifstream *ifs;
};

#endif // LOADSAVEHANDLER_H
