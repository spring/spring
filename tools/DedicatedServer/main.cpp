#include "Game/GameServer.h"
#include "GameSetup.h"
#include "GameData.h"
#include "System/Platform/FileSystem.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Exceptions.h"

#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	try {
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
		if (!gameSetup->Init(script))	// read the script provided by cmdline
		{
			std::cout << "Failed to load script" << std::endl;
			return 1;
		}
		
		std::cout << "Starting server..." << std::endl;
		// Create the server, it will run in a separate thread
		GameData* data = new GameData();

		//  Use script provided hashes if they exist
		if (gameSetup->mapHash != 0) {
			data->SetMap(gameSetup->mapName, gameSetup->mapHash);
		} else {
			data->SetMap(gameSetup->mapName, archiveScanner->GetMapChecksum(gameSetup->mapName));
                }

		if (gameSetup->modHash != 0) {
			data->SetMod(gameSetup->baseMod, gameSetup->modHash);
		} else {
			const std::string modArchive = archiveScanner->ModNameToModArchive(gameSetup->baseMod);
			data->SetMod(gameSetup->baseMod, archiveScanner->GetModChecksum(modArchive));
		}

		data->SetScript(gameSetup->scriptName);

		CFileHandler* f = new CFileHandler("maps/" + gameSetup->mapName);
		if (!f->FileExists()) {
			std::vector<std::string> ars = archiveScanner->GetArchivesForMap(gameSetup->mapName);
			if (ars.empty()) {
				throw content_error("Couldn't find any archives for map '" + gameSetup->mapName + "'.");
			}
			for (std::vector<std::string>::iterator i = ars.begin(); i != ars.end(); ++i) {
				if (!vfsHandler->AddArchive(*i, false)) {
					throw content_error("Couldn't load archive '" + *i + "' for map '" + gameSetup->mapName + "'.");
				}
			}
		}
		delete f;
		
		gameSetup->LoadStartPositions();
		

		server = new CGameServer(gameSetup->hostport, false, data, gameSetup);
		
		if (gameSetup->autohostport > 0)
			server->AddAutohostInterface(gameSetup->autohostport);
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
		delete gameSetup;
	}
	else
	{
		std::cout << "usage: dedicated <full_path_to_script>" << std::endl;
	}
	
	FileSystemHandler::Cleanup();
	}
	catch (const std::exception& err)
	{
		std::cout << "Exception raised: " << err.what() << std::endl;
	}
	return 0;
}
