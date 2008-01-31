#include "Game/GameServer.h"
#include "GameSetup.h"
#include "System/Platform/FileSystem.h"

#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
	std::cout << "BIG FAT WARNING: this server is currently under development. If you find any errors (you most likely will)";
	std::cout << " report them to mantis or the forums." << std::endl << std::endl;
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);

	CGameServer* server = 0;
	CGameSetup* gameSetup = 0;
	
	if (argc > 1)
	{
		std::string script = argv[1];
		std::cout << "Loading script: " << script << std::endl;
		gameSetup = new CGameSetup();	// to store the gamedata inside      
		gameSetup->Init(script);	// read the script provided by cmdline
		
		// Create the server, it will run in a separate thread
		server = new CGameServer(gameSetup->hostport, gameSetup->mapName, gameSetup->baseMod, gameSetup->scriptName, gameSetup);

		if (argc > 2) // add the communication interface
			server->AddAutohostInterface(8453, atoi(argv[2]));
		
		while (!server->HasFinished()) // check if still running
			sleep(1);	// if so, wait 1  second
		delete server;	// delete the server after usage
	}
	else
	{
		std::cout << "usage: dedicated <full_path_to_script> <portnumber for AutohostInterface>" << std::endl;
	}
	
	FileSystemHandler::Cleanup();
	
	return 0;
}
