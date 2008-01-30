#include "Game/GameServer.h"
#include "GameSetup.h"
#include "System/Platform/FileSystem.h"

#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);
	
	CGameServer* server = 0;
	CGameSetup* gameSetup = 0;
	
	if (argc > 1)
	{
		std::string script = argv[1];
		std::cout << "Loading script: " << script << std::endl;
		gameSetup = new CGameSetup();      
		gameSetup->Init(script);
		server = new CGameServer(gameSetup->hostport, gameSetup->mapName, gameSetup->baseMod, gameSetup->scriptName, gameSetup);

		if (argc > 2)
			server->AddAutohostInterface(8453, atoi(argv[2]));
		
		while (!server->HasFinished())
			sleep(1);
		delete server;
	}
	else
	{
		std::cout << "usage: dedicated <full_path_to_script> <portnumber for AutohostInterface>" << std::endl;
	}
	
	FileSystemHandler::Cleanup();
	
	return 0;
}
