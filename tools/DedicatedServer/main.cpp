#include "Game/GameServer.h"
#include "GameSetup.h"
#include "System/Platform/FileSystem.h"
#include "EventPrinter.h"

#include <string>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char *argv[])
{
	std::cout << "BIG FAT WARNING: this server is currently under development. If you find any errors (you most likely will)";
	std::cout << " report them to mantis or the forums." << std::endl << std::endl;
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);

	CGameServer* server = 0;
	CGameSetup* gameSetup = 0;
	EventPrinter ep;
	
	if (argc > 1)
	{
		std::string script = argv[1];
		std::cout << "Loading script: " << script << std::endl;
		gameSetup = new CGameSetup();	// to store the gamedata inside      
		gameSetup->Init(script);	// read the script provided by cmdline
		
		// Create the server, it will run in a separate thread
		server = new CGameServer(gameSetup->hostport, gameSetup->mapName, gameSetup->baseMod, gameSetup->scriptName, gameSetup);
		server->log.Subscribe((ServerLog*)&ep);
		
		if (gameSetup->autohostport > 0)
			server->AddAutohostInterface(8453, gameSetup->autohostport);
		else
		{
			std::cout << "You should specify an AutohostPort in the script" << std::endl;
		}

		while (!server->HasFinished()) // check if still running
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);	// if so, wait 1  second
#endif
		delete server;	// delete the server after usage
	}
	else
	{
		std::cout << "usage: dedicated <full_path_to_script>" << std::endl;
	}
	
	FileSystemHandler::Cleanup();
	
	return 0;
}
