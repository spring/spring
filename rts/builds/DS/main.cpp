#ifdef _MSC_VER
#include "StdAfx.h"
#endif

#include <string>
#include <iostream>
#include <SDL.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "System/FileSystem/FileSystemHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/UnsyncedRNG.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
	try {
#endif

	SDL_Init(SDL_INIT_TIMER);

	std::cout << "[DS] if you find any errors, report them to mantis or the forums." << std::endl << std::endl;

	ConfigHandler::Instantiate(); // use the default config file
	FileSystemHandler::Initialize(false);

	CGameServer* server = 0;
	CGameSetup* gameSetup = 0;

	if (argc == 2) {
		const std::string script(argv[1]);
		std::string buf;

		std::cout << "[DS] loading script from file: " << script << std::endl;

		ClientSetup settings;
		CFileHandler fh(argv[1]);

		if (!fh.FileExists())
			throw content_error("[DS] script does not exist in given location: " + script);

		if (!fh.LoadStringData(buf))
			throw content_error("[DS] script cannot be read: " + script);
		settings.Init(buf);

		gameSetup = new CGameSetup();	// to store the gamedata inside

		if (!gameSetup->Init(buf)) {
			// read the script provided by cmdline
			std::cout << "[DS] failed to load script" << std::endl;
			return 1;
		}

		std::cout << "[DS] starting server..." << std::endl;

		// Create the server, it will run in a separate thread
		GameData data;
		UnsyncedRNG rng;

		rng.Seed(gameSetup->gameSetupText.length());
		rng.Seed(script.length());
		data.SetRandomSeed(rng.RandInt());

		//  Use script provided hashes if they exist
		if (gameSetup->mapHash != 0) {
			data.SetMapChecksum(gameSetup->mapHash);
			gameSetup->LoadStartPositions(false); // reduced mode
		} else {
			data.SetMapChecksum(archiveScanner->GetArchiveCompleteChecksum(gameSetup->mapName));

			CFileHandler f("maps/" + gameSetup->mapName);
			if (!f.FileExists()) {
				vfsHandler->AddArchiveWithDeps(gameSetup->mapName, false);
			}
			gameSetup->LoadStartPositions(); // full mode
		}

		if (gameSetup->modHash != 0) {
			data.SetModChecksum(gameSetup->modHash);
		} else {
			const std::string& modArchive = archiveScanner->ArchiveFromName(gameSetup->modName);
			const unsigned int modCheckSum = archiveScanner->GetArchiveCompleteChecksum(modArchive);
			data.SetModChecksum(modCheckSum);
		}

		data.SetSetup(gameSetup->gameSetupText);
		server = new CGameServer(settings.hostIP, settings.hostPort, &data, gameSetup);

		while (!server->HasGameID()) {
			// wait until gameID has been generated
		}

		{
			const boost::scoped_ptr<CDemoRecorder>& demoRec = server->GetDemoRecorder();
			const boost::uint8_t* gameID = (demoRec->GetFileHeader()).gameID;

			printf("[DS] recording demo: %s\n", (demoRec->GetName()).c_str());
			printf("[DS] using mod: %s\n", (gameSetup->modName).c_str());
			printf("[DS] using map: %s\n", (gameSetup->mapName).c_str());
			printf("[DS] GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				gameID[ 0], gameID[ 1], gameID[ 2], gameID[ 3], gameID[ 4], gameID[ 5], gameID[ 6], gameID[ 7],
				gameID[ 8], gameID[ 9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]
			);
		}

		while (!server->HasFinished()) {
			// wait 1 second between checks
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}

		delete server; // delete the server after usage
	} else {
		std::cout << "[DS] usage: " << argv[0] << " <full_path_to_script>" << std::endl;
	}

	FileSystemHandler::Cleanup();
	ConfigHandler::Deallocate();

#ifdef _WIN32
	} catch (const std::exception& err) {
		std::cout << "[DS] exception raised: " << err.what() << std::endl;
		return 1;
	}
#endif
	return 0;
}

#if defined(WIN32) && !defined(_MSC_VER)
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif
