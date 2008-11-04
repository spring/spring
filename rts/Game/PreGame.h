#ifndef PREGAME_H
#define PREGAME_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "GameController.h"

class CInfoConsole;
class CLoadSaveHandler;
class GameData;
class LocalSetup;
namespace netcode {
	class RawPacket;
}

/**
 * @brief This controlls the game start
 * 
 * Before a game starts, this class does everything that needs to be done before.
 * It basically goes like this:
 * For servers:
 * 1. Find out which map, script and mod to use
 * 2. Start the server with this settings
 * 3. continue with "For clients"
 * 
 * ForClients:
 * 1. Connect to the server
 * 2. Receive GameData from server
 * 3. Start the CGame with the information provided by server
 * */
class CPreGame : public CGameController
{
public:
	CPreGame(const LocalSetup* setup);
	virtual ~CPreGame();
	
	void LoadSetupscript(const std::string& script);
	void LoadDemo(const std::string& demo);
	void LoadSavefile(const std::string& save);

	bool Draw();
	int KeyPressed(unsigned short k, bool isRepeat);
	bool Update();

private:
	void StartServer(const std::string& setupscript);
	
	/// reads out map, mod and script from demos (with or without a gameSetupScript)
	void ReadDataFromDemo(const std::string& demoName);

	/// receive network traffic
	void UpdateClientNet();

	CInfoConsole* infoConsole;

	/// Load map and dependend archives into archive scanner
	void LoadMap(const std::string& mapName, const bool forceReload = false);
	
	/// Map all required archives depending on selected mod(s)
	void LoadMod(const std::string& modName);
	
	void LoadLua();

	void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);

	/**
	@brief GameData we recieved from server
	
	We won't start until we recieved this
	*/
	boost::scoped_ptr<const GameData> gameData;
	boost::scoped_ptr<const LocalSetup> settings;
	std::string modArchive;
	CLoadSaveHandler *savefile;
	
	bool localDemoHack;
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
