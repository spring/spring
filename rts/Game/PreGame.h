#ifndef PREGAME_H
#define PREGAME_H

#include <string>

#include "GameController.h"

class CglList;
class CInfoConsole;
class CLoadSaveHandler;
class GameData;
namespace netcode {
	class RawPacket;
}

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
	static CglList* showList;
	
	/// Choose the script we will tell the server to start with
	static void SelectScript(std::string s);
	static void SelectMap(std::string s);
	static void SelectMod(std::string s);
	
	/// Load map and dependend archives into archive scanner
	static void LoadMap(const std::string& mapName);
	
	/// Map all required archives depending on selected mod(s)
	static void LoadMod(const std::string& modName);

	void GameDataRecieved(netcode::RawPacket* packet);
	
	const bool server;
	enum State {
		UNKNOWN,
		WAIT_ON_ADDRESS, // wait for user to write server address
		WAIT_ON_USERINPUT, // wait for user to set script, map, mod
		WAIT_CONNECTING, // connecting to server
		WAIT_ON_GAMEDATA, // wait for the server to send us the GameData
		ALL_READY, // ready to start
	};
	State state;

	const bool hasDemo,hasSave;

	/**
	@brief GameData we recieved from server
	
	We won't start until we recieved this
	*/
	const GameData* gameData;
	
	/// all the GameData (script, map, mod and checksums) needed to start the server
	GameData* serverStartupData;
	std::string modArchive;
	std::string demoFile;
	CLoadSaveHandler *savefile;
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
