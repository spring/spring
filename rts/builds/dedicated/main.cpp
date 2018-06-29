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
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/GlobalRNG.h"
#include "System/Config/ConfigHandler.h"
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
#include "System/Misc/SpringTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"

#include <gflags/gflags.h>

#define LOG_SECTION_DEDICATED_SERVER "DedicatedServer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_DEDICATED_SERVER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_DEDICATED_SERVER

DEFINE_bool_EX  (sync_version,     "sync-version",     false, "Display program sync version (for online gaming)");
DEFINE_string   (config,                               "",    "Exclusive configuration file");
DEFINE_bool_EX  (list_config_vars, "list-config-vars", false, "Dump a list of config vars and meta data to stdout");
DEFINE_bool     (isolation,                            false, "Limit the data-dir (games & maps) scanner to one directory");
DEFINE_string_EX(isolation_dir,    "isolation-dir",    "",    "Specify the isolation-mode data-dir (see --isolation)");
DEFINE_bool     (nocolor,                              false, "Disables colorized stdout");
DEFINE_uint32   (sleeptime,                            1,     "Number of seconds to sleep between game-over checks");

#ifdef __cplusplus
extern "C"
{
#endif

void ParseCmdLine(int argc, char* argv[], std::string& scriptName)
{
	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFAULT

#ifndef WIN32
	if (!FLAGS_nocolor && (getenv("SPRING_NOCOLOR") == nullptr)) {
		// don't colorize, if our output is piped to a diff tool or file
		if (isatty(fileno(stdout)))
			log_console_colorizedOutput(true);
	}
#endif
	if (FLAGS_sync_version) {
		LOG("%s", (SpringVersion::GetSync()).c_str());
		exit(0);
	}

	if (argc >= 2)
		scriptName = argv[1];

	if (scriptName.empty() && !FLAGS_list_config_vars) {
		gflags::ShowUsageWithFlags(argv[0]);
		exit(1);
	}

	if (FLAGS_isolation)
		dataDirLocater.SetIsolationMode(true);

	if (!FLAGS_isolation_dir.empty()) {
		dataDirLocater.SetIsolationMode(true);
		dataDirLocater.SetIsolationModeDir(FLAGS_isolation_dir);
	}


	if (FLAGS_list_config_vars) {
		LOG_DISABLE();
		FileSystemInitializer::PreInitializeConfigHandler(FLAGS_config);
		FileSystemInitializer::InitializeLogOutput();
		LOG_ENABLE();
		ConfigVariable::OutputMetaDataMap();
		exit(0);
	}

	//LOG("Run: %s", cmdLine.GetCmdLine().c_str());
	FileSystemInitializer::PreInitializeConfigHandler(FLAGS_config);

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

		CLogOutput::LogSystemInfo();

		std::string scriptName;
		std::string scriptText;
		std::string binaryName = argv[0];

		gflags::SetUsageMessage("Usage: " + binaryName + " [options] path_to_script.txt");
		gflags::SetVersionString(SpringVersion::GetFull());
		gflags::ParseCommandLineFlags(&argc, &argv, true);
		ParseCmdLine(argc, argv, scriptName);

		globalConfig.Init();
		FileSystemInitializer::InitializeLogOutput();
		FileSystemInitializer::Initialize();

		// Initialize crash reporting
		CrashHandler::Install();

		LOG("report any errors to Mantis or the forums.");
		LOG("loading script from file: %s", scriptName.c_str());

		// server will take ownership of these
		std::shared_ptr<ClientSetup> dsClientSetup(new ClientSetup());
		std::shared_ptr<GameData> dsGameData(new GameData());
		std::shared_ptr<CGameSetup> dsGameSetup(new CGameSetup());

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

		// create the server, it will run in a separate thread
		CGlobalUnsyncedRNG rng;

		const uint32_t sleepTime = FLAGS_sleeptime;
		const uint32_t randSeed = time(nullptr) % ((spring_gettime().toNanoSecsi() + 1) * 9007);

		rng.Seed(randSeed);
		dsGameData->SetRandomSeed(rng.NextInt());

		{
			sha512::raw_digest dsMapChecksum;
			sha512::raw_digest dsModChecksum;
			sha512::hex_digest dsMapChecksumHex;
			sha512::hex_digest dsModChecksumHex;

			std::memcpy(dsMapChecksum.data(), &dsGameSetup->dsMapHash[0], sizeof(dsGameSetup->dsMapHash));
			std::memcpy(dsModChecksum.data(), &dsGameSetup->dsModHash[0], sizeof(dsGameSetup->dsModHash));
			sha512::dump_digest(dsMapChecksum, dsMapChecksumHex);
			sha512::dump_digest(dsModChecksum, dsModChecksumHex);

			LOG("[script-checksums]\n\tmap=%s\n\tmod=%s", dsMapChecksumHex.data(), dsModChecksumHex.data());

			// use script-provided hashes if any byte is non-zero; these
			// are only used by some client-side (pregame) sanity checks
			const auto hashPred = [](uint8_t byte) { return (byte != 0); };

			if (std::find_if(dsMapChecksum.begin(), dsMapChecksum.end(), hashPred) != dsMapChecksum.end()) {
				dsGameData->SetMapChecksum(dsMapChecksum.data());
				dsGameSetup->LoadStartPositions(false); // reduced mode
			} else {
				dsGameData->SetMapChecksum(&archiveScanner->GetArchiveCompleteChecksumBytes(dsGameSetup->mapName)[0]);

				CFileHandler f("maps/" + dsGameSetup->mapName);
				if (!f.FileExists())
					vfsHandler->AddArchiveWithDeps(dsGameSetup->mapName, false);

				dsGameSetup->LoadStartPositions(); // full mode
			}

			if (std::find_if(dsModChecksum.begin(), dsModChecksum.end(), hashPred) != dsModChecksum.end()) {
				dsGameData->SetModChecksum(dsModChecksum.data());
			} else {
				const std::string& modArchive = archiveScanner->ArchiveFromName(dsGameSetup->modName);
				const sha512::raw_digest& modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(modArchive);

				dsGameData->SetModChecksum(&modCheckSum[0]);
			}
		}

		LOG("starting server...");

		{
			dsGameData->SetSetupText(dsGameSetup->setupText);
			CGameServer server(dsClientSetup, dsGameData, dsGameSetup);

			while (!server.HasGameID()) {
				// wait until gameID has been generated or
				// a timeout occurs (if no clients connect)
				if (server.HasFinished())
					break;

				spring_sleep(spring_secs(sleepTime));
			}

			while (!server.HasFinished()) {
				static bool printData = (server.GetDemoRecorder() != nullptr);

				if (printData) {
					printData = false;

					const std::unique_ptr<CDemoRecorder>& demoRec = server.GetDemoRecorder();
					const std::uint8_t* gameID = (demoRec->GetFileHeader()).gameID;

					LOG("recording demo: %s", (demoRec->GetName()).c_str());
					LOG("using mod: %s", (dsGameSetup->modName).c_str());
					LOG("using map: %s", (dsGameSetup->mapName).c_str());
					LOG("GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", gameID[0], gameID[1], gameID[2], gameID[3], gameID[4], gameID[5], gameID[6], gameID[7], gameID[8], gameID[9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
				}

				spring_secs(sleepTime).sleep(true);
			}
		}

		LOG("exiting");
		FileSystemInitializer::Cleanup();
		DataDirLocater::FreeInstance();

		spring_clock::PopTickRate();
		LOG("exited");
	}
	CATCH_SPRING_ERRORS

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
