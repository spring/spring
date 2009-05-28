#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/UnsyncedRNG.h"

#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	try {
	std::cout << "If you find any errors, report them to mantis or the forums." << std::endl << std::endl;
	ConfigHandler::Instantiate("");
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);
	CGameServer* server = 0;
	CGameSetup* gameSetup = 0;

	if (argc > 1)
	{
		const std::string script(argv[0]);
		std::cout << "Loading script from file: " << script << std::endl;

		ClientSetup settings;
		CFileHandler fh(argv[1]);
		if (!fh.FileExists())
			throw content_error("Setupscript doesn't exists in given location: "+script);
		
		std::string buf;
		if (!fh.LoadStringData(buf))
			throw content_error("Setupscript cannot be read: "+script);
		settings.Init(buf);
		
		gameSetup = new CGameSetup();	// to store the gamedata inside
		if (!gameSetup->Init(buf))	// read the script provided by cmdline
		{
			std::cout << "Failed to load script" << std::endl;
			return 1;
		}
		
		std::cout << "Starting server..." << std::endl;
		// Create the server, it will run in a separate thread
		GameData* data = new GameData();
		UnsyncedRNG rng;
		rng.Seed(gameSetup->gameSetupText.length());
		rng.Seed(script.length());
		data->SetRandomSeed(rng.RandInt());

		//  Use script provided hashes if they exist
		if (gameSetup->mapHash != 0)
		{
			data->SetMap(gameSetup->mapName, gameSetup->mapHash);
			gameSetup->LoadStartPositions(false); // reduced mode
		}
		else
		{
			data->SetMap(gameSetup->mapName, archiveScanner->GetMapChecksum(gameSetup->mapName));

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
			gameSetup->LoadStartPositions(); // full mode
		}

		if (gameSetup->modHash != 0) {
			data->SetMod(gameSetup->baseMod, gameSetup->modHash);
		} else {
			const std::string modArchive = archiveScanner->ModNameToModArchive(gameSetup->baseMod);
			data->SetMod(gameSetup->baseMod, archiveScanner->GetModChecksum(modArchive));
		}

		data->SetScript(gameSetup->scriptName);
		data->SetSetup(gameSetup->gameSetupText);
		server = new CGameServer(&settings, false, data, gameSetup);

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
		std::cout << "usage: spring-dedicated <full_path_to_script>" << std::endl;
	}
	
	FileSystemHandler::Cleanup();
	}
	catch (const std::exception& err)
	{
		std::cout << "Exception raised: " << err.what() << std::endl;
	}
	return 0;
}
