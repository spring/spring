/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL.h>

#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "Game/GameVersion.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "System/Exceptions.h"
#include "System/UnsyncedRNG.h"



#define LOG_SECTION_DEDICATED_SERVER "DS"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_DEDICATED_SERVER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_DEDICATED_SERVER


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __APPLE__
//FIXME: hack for SDL because of sdl-stubs
#undef main
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
	try {
#endif
	// Initialize crash reporting
	CrashHandler::Install();

	if (argc != 2) {
		LOG_L(L_ERROR, "usage: %s <full_path_to_script | --version>", argv[0]);
		return 1;
	}
	if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
		LOG("%s", (SpringVersion::GetFull()).c_str());
		return 0;
	}

	std::string scriptName(argv[1]);
	std::string scriptText;

	LOG("report any errors to Mantis or the forums.");
	LOG("loading script from file: %s", scriptName.c_str());

	SDL_Init(SDL_INIT_TIMER);

	ConfigHandler::Instantiate(); // use the default config file
	GlobalConfig::Instantiate();
	FileSystemInitializer::Initialize();

	CGameServer* server = NULL;
	CGameSetup* gameSetup = NULL;
	ClientSetup settings;
	CFileHandler fh(scriptName);

	if (!fh.FileExists())
		throw content_error("script does not exist in given location: " + scriptName);

	if (!fh.LoadStringData(scriptText))
		throw content_error("script cannot be read: " + scriptName);

	settings.Init(scriptText);
	gameSetup = new CGameSetup(); // to store the gamedata inside

	if (!gameSetup->Init(scriptText)) {
		// read the script provided by cmdline
		LOG_L(L_ERROR, "failed to load script %s", scriptName.c_str());
		return 1;
	}

	// Create the server, it will run in a separate thread
	GameData data;
	UnsyncedRNG rng;

	rng.Seed(gameSetup->gameSetupText.length());
	rng.Seed(scriptName.length());
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

	LOG("starting server...");

	data.SetSetup(gameSetup->gameSetupText);
	server = new CGameServer(settings.hostIP, settings.hostPort, &data, gameSetup);

	while (!server->HasGameID()) {
		// wait until gameID has been generated or
		// a timeout occurs (if no clients connect)
		if (server->HasFinished()) {
			break;
		}
	}

	while (!server->HasFinished()) {
		static bool printData = true;

		if (printData) {
			printData = false;

			const boost::scoped_ptr<CDemoRecorder>& demoRec = server->GetDemoRecorder();
			const boost::uint8_t* gameID = (demoRec->GetFileHeader()).gameID;

			LOG("recording demo: %s", (demoRec->GetName()).c_str());
			LOG("using mod: %s", (gameSetup->modName).c_str());
			LOG("using map: %s", (gameSetup->mapName).c_str());
			LOG("GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", gameID[0], gameID[1], gameID[2], gameID[3], gameID[4], gameID[5], gameID[6], gameID[7], gameID[8], gameID[9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
		}

		// wait 1 second between checks
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}

	delete server;

	FileSystemInitializer::Cleanup();
	GlobalConfig::Deallocate();
	ConfigHandler::Deallocate();

#ifdef _WIN32
} catch (const std::exception& err) {
	LOG_L(L_ERROR, "Dedicated server failed to start: %s", err.what());
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
