/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PREGAME_H
#define PREGAME_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "GameController.h"
#include "System/Misc/SpringTime.h"

class ILoadSaveHandler;
class GameData;
class CGameSetup;
class ClientSetup;


namespace netcode {
	class RawPacket;
}

/**
 * @brief This controls the game start
 *
 * Before a game starts, this class does everything that needs to be done before.
 * It basically goes like this:
 * For servers:
 * 1. Find out which map, script and mod to use
 * 2. Start the server with this settings
 * 3. continue with "For clients"
 *
 * For Clients:
 * 1. Connect to the server
 * 2. Receive GameData from server
 * 3. Start the CGame with the information provided by server
 * */
class CPreGame : public CGameController
{
public:
	CPreGame(boost::shared_ptr<ClientSetup> setup);
	virtual ~CPreGame();

	void LoadSetupscript(const std::string& script);
	void LoadDemo(const std::string& demo);
	void LoadSavefile(const std::string& save, bool usecreg);

	bool Draw();
	int KeyPressed(int k, bool isRepeat);
	bool Update();

private:
	void AddGameSetupArchivesToVFS(const CGameSetup* setup, bool mapOnly);
	void StartServer(const std::string& setupscript);
	void StartServerForDemo(const std::string& demoName);

	/// reads out map, mod and script from demos (with or without a gameSetupScript)
	void ReadDataFromDemo(const std::string& demoName);

	/// receive network traffic
	void UpdateClientNet();

	void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);

	/**
	@brief GameData we received from server

	We won't start until we received this (NULL until GameDataReceived)
	*/
	boost::shared_ptr<GameData> gameData;
	boost::shared_ptr<ClientSetup> clientSetup;

	std::string modArchive;
	ILoadSaveHandler* savefile;

	spring_time timer;
	bool wantDemo;
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
