#ifndef PREGAME_H
#define PREGAME_H

#include "GameController.h"
#include <string>

class CglList;
class CInfoConsole;
class CLoadSaveHandler;

class CPreGame : public CGameController
{
public:
	CPreGame(bool server, const std::string& demo, const std::string& save);
	virtual ~CPreGame();

	bool Draw();
	int KeyPressed(unsigned short k, bool isRepeat);
	bool Update();

private:
	/// reads out map, mod and script from demos (with or without a gameSetupScript)
	void ReadDataFromDemo(const std::string& demoName);

	/// receive network traffic
	void UpdateClientNet();

	CInfoConsole* infoConsole;

	void ShowScriptList();
	void ShowMapList();
	void ShowModList();
	CglList* showList;
	static void SelectScript(std::string s);
	static void SelectMap(std::string s);
	static void SelectMod(std::string s);
	
	const bool server;
	enum State {
		UNKNOWN,
		WAIT_ON_ADDRESS,
		WAIT_ON_SCRIPT,
		WAIT_ON_MAP,
		WAIT_ON_MOD,
		WAIT_CONNECTING,
		ALL_READY,
	};
	State state;

	const bool hasDemo,hasSave;

	std::string mapName;
	std::string modName;
	std::string modArchive;
	std::string scriptName;
	std::string demoFile;
	CLoadSaveHandler *savefile;
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
