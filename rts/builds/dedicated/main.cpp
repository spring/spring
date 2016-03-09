/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "Game/GameVersion.h"
#include "Net/GameServer.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Log/ConsoleSink.h"
#include "System/Log/ILog.h"
#include "System/Log/DefaultFilter.h"
#include "System/LogOutput.h"
#include "System/Platform/CmdLineParams.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Config/ConfigHandler.h"
#include "System/Misc/SpringTime.h"
#include "System/GlobalConfig.h"
#include "System/Exceptions.h"
#include "System/UnsyncedRNG.h"



#define LOG_SECTION_DEDICATED_SERVER "DedicatedServer"
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

void ParseCmdLine(CmdLineParams& cmdLine, std::string& scriptName)
{
	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFAULT

	cmdLine.AddSwitch(0,   "sync-version",       "Display program sync version (for online gaming)");
	cmdLine.AddString('C', "config",             "Exclusive configuration file");
	cmdLine.AddSwitch(0,   "list-config-vars",   "Dump a list of config vars and meta data to stdout");
	cmdLine.AddSwitch('i', "isolation",          "Limit the data-dir (games & maps) scanner to one directory");
	cmdLine.AddString(0,   "isolation-dir",      "Specify the isolation-mode data-dir (see --isolation)");
	cmdLine.AddSwitch(0,   "nocolor",            "Disables colorized stdout");
	cmdLine.AddSwitch('q', "quiet",              "Ignore unrecognized arguments");
	cmdLine.AddInt   (0,   "sleeptime",          "Number of seconds to sleep between game-over checks");

	try {
		cmdLine.Parse();
	} catch (const CmdLineParams::unrecognized_option& err) {
		LOG_L(L_ERROR, "%s\n", err.what());
		if (!cmdLine.IsSet("quiet")) {
			cmdLine.PrintUsage();
			exit(EXIT_FAILURE);
		}
	}

#ifndef WIN32
	if (!cmdLine.IsSet("nocolor") && (getenv("SPRING_NOCOLOR") == NULL)) {
		// don't colorize, if our output is piped to a diff tool or file
		if (isatty(fileno(stdout)))
			log_console_colorizedOutput(true);
	}
#endif

	if (cmdLine.IsSet("help")) {
		cmdLine.PrintUsage();
		exit(0);
	}
	if (cmdLine.IsSet("version")) {
		LOG("%s", (SpringVersion::GetFull()).c_str());
		exit(0);
	}
	if (cmdLine.IsSet("sync-version")) {
		LOG("%s", (SpringVersion::GetSync()).c_str());
		exit(0);
	}


	scriptName = cmdLine.GetInputFile();

	if (scriptName.empty() && !cmdLine.IsSet("list-config-vars")) {
		cmdLine.PrintUsage();
		exit(1);
	}

	if (cmdLine.IsSet("isolation")) {
		dataDirLocater.SetIsolationMode(true);
	}

	if (cmdLine.IsSet("isolation-dir")) {
		dataDirLocater.SetIsolationMode(true);
		dataDirLocater.SetIsolationModeDir(cmdLine.GetString("isolation-dir"));
	}

	const std::string configSource = cmdLine.IsSet("config") ? cmdLine.GetString("config") : "";

	if (cmdLine.IsSet("list-config-vars")) {
		LOG_DISABLE();
		FileSystemInitializer::PreInitializeConfigHandler(configSource);
		FileSystemInitializer::InitializeLogOutput();
		LOG_ENABLE();
		ConfigVariable::OutputMetaDataMap();
		exit(0);
	}

	LOG("Run: %s", cmdLine.GetCmdLine().c_str());
	FileSystemInitializer::PreInitializeConfigHandler(configSource);

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEDICATED_SERVER
}



int main(int argc, char* argv[])
{
	Threading::SetMainThread();
	try {
		spring_clock::PushTickRate();
		// initialize start time (can safely be done before SDL_Init
		// since we are not using SDL_GetTicks as our clock anymore)
		spring_time::setstarttime(spring_time::gettime(true));

		CmdLineParams cmdLine(argc, argv);
		CLogOutput::LogSystemInfo();

		std::string scriptName;
		std::string scriptText;
		std::string binaryName = argv[0];

		cmdLine.SetUsageDescription("Usage: " + binaryName + " [options] path_to_script.txt");
		ParseCmdLine(cmdLine, scriptName);

		GlobalConfig::Instantiate();
		FileSystemInitializer::InitializeLogOutput();
		FileSystemInitializer::Initialize();

		// Initialize crash reporting
		CrashHandler::Install();

		LOG("report any errors to Mantis or the forums.");
		LOG("loading script from file: %s", scriptName.c_str());

		CGameServer* server = NULL;

		// server will take ownership of these
		boost::shared_ptr<ClientSetup> dsClientSetup(new ClientSetup());
		boost::shared_ptr<GameData> dsGameData(new GameData());
		boost::shared_ptr<CGameSetup> dsGameSetup(new CGameSetup());

		CFileHandler fh(scriptName);

		if (!fh.FileExists())
			throw content_error("script does not exist in given location: " + scriptName);

		if (!fh.LoadStringData(scriptText))
			throw content_error("script cannot be read: " + scriptName);

		dsClientSetup->LoadFromStartScript(scriptText);

		if (!dsGameSetup->Init(scriptText)) {
			// read the script provided by cmdline
			LOG_L(L_ERROR, "failed to load script %s", scriptName.c_str());
			return 1;
		}

		// Create the server, it will run in a separate thread
		UnsyncedRNG rng;

		const unsigned sleepTime = cmdLine.IsSet("sleeptime")? cmdLine.GetInt("sleeptime"): 1;
		const unsigned randSeed = time(NULL) % ((spring_gettime().toNanoSecsi() + 1) * 9007);

		rng.Seed(randSeed);
		dsGameData->SetRandomSeed(rng.RandInt());

		//  Use script provided hashes if they exist
		if (dsGameSetup->mapHash != 0) {
			dsGameData->SetMapChecksum(dsGameSetup->mapHash);
			dsGameSetup->LoadStartPositions(false); // reduced mode
		} else {
			dsGameData->SetMapChecksum(archiveScanner->GetArchiveCompleteChecksum(dsGameSetup->mapName));

			CFileHandler f("maps/" + dsGameSetup->mapName);
			if (!f.FileExists()) {
				vfsHandler->AddArchiveWithDeps(dsGameSetup->mapName, false);
			}
			dsGameSetup->LoadStartPositions(); // full mode
		}

		if (dsGameSetup->modHash != 0) {
			dsGameData->SetModChecksum(dsGameSetup->modHash);
		} else {
			const std::string& modArchive = archiveScanner->ArchiveFromName(dsGameSetup->modName);
			const unsigned int modCheckSum = archiveScanner->GetArchiveCompleteChecksum(modArchive);
			dsGameData->SetModChecksum(modCheckSum);
		}

		LOG("starting server...");

		dsGameData->SetSetupText(dsGameSetup->setupText);
		server = new CGameServer(dsClientSetup, dsGameData, dsGameSetup);

		while (!server->HasGameID()) {
			// wait until gameID has been generated or
			// a timeout occurs (if no clients connect)
			if (server->HasFinished()) {
				break;
			}

			spring_sleep(spring_secs(sleepTime));
		}

		while (!server->HasFinished()) {
			static bool printData = (server->GetDemoRecorder() != NULL);

			if (printData) {
				printData = false;

				const boost::scoped_ptr<CDemoRecorder>& demoRec = server->GetDemoRecorder();
				const boost::uint8_t* gameID = (demoRec->GetFileHeader()).gameID;

				LOG("recording demo: %s", (demoRec->GetName()).c_str());
				LOG("using mod: %s", (dsGameSetup->modName).c_str());
				LOG("using map: %s", (dsGameSetup->mapName).c_str());
				LOG("GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", gameID[0], gameID[1], gameID[2], gameID[3], gameID[4], gameID[5], gameID[6], gameID[7], gameID[8], gameID[9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
			}

			spring_sleep(spring_secs(sleepTime));
		}

		LOG("exiting");

		delete server;

		FileSystemInitializer::Cleanup();
		GlobalConfig::Deallocate();
		DataDirLocater::FreeInstance();

		spring_clock::PopTickRate();
		LOG("exited");
	}
	CATCH_SPRING_ERRORS

	return GetExitCode();
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
