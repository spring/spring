/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cfloat>

#include <SDL_keycode.h>

#include "PreGame.h"

#include "ClientData.h"
#include "ClientSetup.h"
#include "System/Sync/FPUCheck.h"
#include "Game.h"
#include "GameData.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "GlobalUnsynced.h"
#include "LoadScreen.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "UI/InfoConsole.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Map/Generation/SimpleMapGenerator.h"
#include "Menu/LuaMenuController.h"
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"
#ifndef HEADLESS
#include "aGui/Gui.h"
#endif
#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/SpringExitCode.h"
#include "System/TimeProfiler.h"
#include "System/TdfParser.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/LoadSave/DemoReader.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Misc.h"
#include "System/Sync/SyncedPrimitiveBase.h"
#include "lib/luasocket/src/restrictions.h"
#ifdef SYNCDEBUG
	#include "System/Sync/SyncDebugger.h"
#endif


using netcode::RawPacket;
using std::string;

CONFIG(bool, DemoFromDemo).defaultValue(false);

static char mapChecksumMsgBuf[1024] = {0};
static char modChecksumMsgBuf[1024] = {0};

CPreGame* pregame = nullptr;

CPreGame::CPreGame(std::shared_ptr<ClientSetup> setup)
	: clientSetup(setup)
	, saveFileHandler(nullptr)
	, connectTimer(spring_gettime())
	, wantDemo(true)
{
	assert(clientNet == nullptr);

	clientNet = new CNetProtocol();
	activeController = this;

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->Initialize(clientSetup->isHost, 64); //FIXME: add actual number of player
#endif

	if (!clientSetup->isHost) {
		LOG("[%s] using client IP %s and port %i", __func__, clientSetup->hostIP.c_str(), clientSetup->hostPort);
		// don't allow luasocket to connect to the host
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, clientSetup->hostIP, clientSetup->hostPort, false);
		clientNet->InitClient(clientSetup, SpringVersion::GetSync(), Platform::GetPlatformStr());
	} else {
		LOG("[%s] using server IP %s and port %i", __func__, clientSetup->hostIP.c_str(), clientSetup->hostPort);
		clientNet->InitLocalClient();
	}
}


CPreGame::~CPreGame()
{
	#ifndef HEADLESS
	// delete leftover aGUI elements but not infoconsole, it is reused by CGame
	agui::gui->Clean();
	#endif

	pregame = nullptr;
}

void CPreGame::LoadSetupScript(const std::string& script)
{
	assert(clientSetup->isHost);
	StartServer(script);
}

void CPreGame::LoadDemoFile(const std::string& demo)
{
	assert(clientSetup->isHost);
	wantDemo &= configHandler->GetBool("DemoFromDemo");

	ReadDataFromDemo(demo);
}

void CPreGame::LoadSaveFile(const std::string& save)
{
	assert(clientSetup->isHost);

	saveFileHandler = ILoadSaveHandler::CreateHandler(save);
	saveFileHandler->LoadGameStartInfo(save);

	StartServer(saveFileHandler->GetScriptText());
}

int CPreGame::KeyPressed(int k, bool isRepeat)
{
	if (k != SDLK_ESCAPE)
		return 0;

	if (!KeyInput::GetKeyModState(KMOD_SHIFT)) {
		LOG("[PreGame::%s] press shift+escape to abort loading or exit", __func__);
		return 0;
	}

	if (CLuaMenuController::ActivateInstance("[PreGame] User Aborted Loading")) {
		assert(pregame == this);
		spring::SafeDelete(pregame);
		return 0;
	}

	LOG("[PreGame::%s] user exited", __func__);
	gu->globalQuit = true;
	return 0;
}


bool CPreGame::Draw()
{
	spring_msecs(10).sleep(true);
	ClearScreen();

	if (!clientNet->Connected()) {
		if (clientSetup->isHost)
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Waiting for server to start");
		else
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Connecting to server (%ds)", (spring_gettime() - connectTimer).toSecsi());
	} else {
		font->glPrint(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Waiting for server response");
	}

	font->glFormat(0.60f, 0.40f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Connecting to: %s", clientNet->ConnectionStr().c_str());
	font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "User name: %s", clientSetup->myPlayerName.c_str());

	font->glFormat(0.5f, 0.25f, 0.8f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Press SHIFT + ESC to quit");
	// credits
	font->glFormat(0.5f, 0.06f, 1.0f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Spring %s", SpringVersion::GetFull().c_str());
	font->glPrint(0.5f, 0.02f, 0.6f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "This program is distributed under the GNU General Public License, see doc/LICENSE for more info");
	font->DrawBufferedGL4();

	return true;
}


bool CPreGame::Update()
{
	ENTER_SYNCED_CODE();
	good_fpu_control_registers("CPreGame::Update");
	UpdateClientNet();
	LEAVE_SYNCED_CODE();

	return true;
}


void CPreGame::AddMapArchivesToVFS(const CGameSetup* setup)
{
	// map gets added in StartServer if we are the host, so this can show twice
	// StartServerForDemo does *not* add the map but waits for GameDataReceived
	LOG("[PreGame::%s][server=%p] using map \"%s\" (loaded=%d cached=%d)", __func__, gameServer, setup->mapName.c_str(), vfsHandler->HasArchive(setup->mapName), vfsHandler->HasTempArchive(setup->mapName));

	// load map archive
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);
}

void CPreGame::AddModArchivesToVFS(const CGameSetup* setup)
{
	LOG("[PreGame::%s][server=%p] using game \"%s\" (loaded=%d cached=%d)", __func__, gameServer, setup->modName.c_str(), vfsHandler->HasArchive(setup->modName), vfsHandler->HasTempArchive(setup->modName));

	// load mutators (if any); use WithDeps since mutators depend on the archives they override
	for (const std::string& mut: setup->GetMutatorsCont()) {
		LOG("[PreGame::%s] using mutator \"%s\"", __func__, mut.c_str());

		vfsHandler->AddArchiveWithDeps(mut, true);
	}

	// load game archive
	vfsHandler->AddArchiveWithDeps(setup->modName, false);

	modFileName = archiveScanner->ArchiveFromName(setup->modName);
}


void CPreGame::StartServer(const std::string& setupscript)
{
	assert(gameServer == nullptr);
	ScopedOnceTimer timer("PreGame::StartServer");

	std::shared_ptr<GameData> startGameData(new GameData());
	std::shared_ptr<CGameSetup> startGameSetup(new CGameSetup());

	startGameSetup->Init(setupscript);
	startGameData->SetRandomSeed(static_cast<unsigned>(guRNG.NextInt()));

	if (startGameSetup->mapName.empty())
		throw content_error("No map selected in startscript");

	if (startGameSetup->mapSeed != 0) {
		CSimpleMapGenerator gen(startGameSetup.get());
		gen.Generate();
	}


	// We must map the map into VFS this early, because server needs the start positions.
	// Take care that MapInfo isn't loaded here, as map options aren't available to it yet.
	AddMapArchivesToVFS(startGameSetup.get());

	// Loading the start positions executes the map's Lua.
	// This means start positions can NOT be influenced by map options.
	// (Which is OK, since unitsync does not have map options available either.)
	startGameSetup->LoadStartPositions();

	{
		const std::string& modArchive = archiveScanner->ArchiveFromName(startGameSetup->modName);
		const std::string& mapArchive = archiveScanner->ArchiveFromName(startGameSetup->mapName);

		const sha512::raw_digest& mapChecksum = archiveScanner->GetArchiveCompleteChecksumBytes(mapArchive);
		const sha512::raw_digest& modChecksum = archiveScanner->GetArchiveCompleteChecksumBytes(modArchive);

		startGameData->SetMapChecksum(mapChecksum.data());
		startGameData->SetModChecksum(modChecksum.data());

		sha512::hex_digest mapChecksumHex;
		sha512::hex_digest modChecksumHex;
		sha512::dump_digest(mapChecksum, mapChecksumHex);
		sha512::dump_digest(modChecksum, modChecksumHex);

		LOG("[PreGame::%s]\n\tmod-checksum=%s\n\tmap-checksum=%s", __func__, modChecksumHex.data(), mapChecksumHex.data());
	}

	good_fpu_control_registers("before CGameServer creation");
	startGameData->SetSetupText(startGameSetup->setupText);
	gameServer = new CGameServer(clientSetup, startGameData, startGameSetup);

	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	//FIXME move this code to a external file and move that to rts/Net/

	clientNet->Update();

	if (clientNet->CheckTimeout(0, true)) {
		if (CLuaMenuController::ActivateInstance("[PreGame] Server Connection Timeout")) {
			assert(pregame == this);
			spring::SafeDelete(pregame);
			return;
		}

		LOG_L(L_ERROR, "[PreGame] Server Connection Timeout");

		spring::exitCode = spring::EXIT_CODE_TIMEOUT;
		gu->globalQuit = true;
		return;
	}

	std::shared_ptr<const RawPacket> packet;

	while ((packet = clientNet->GetData(gs->frameNum))) {
		const unsigned char* inbuf = packet->data;

		if (packet->length <= 0) {
			LOG_L(L_WARNING, "[PreGame::%s] zero-length packet (header: %i)", __func__, inbuf[0]);
			continue;
		}

		switch (inbuf[0]) {
			case NETMSG_REJECT_CONNECT:
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;

					pckt >> message;

					// (re)activate LuaMenu if user failed to connect
					if (CLuaMenuController::ActivateInstance(message)) {
						assert(pregame == this);
						spring::SafeDelete(pregame);
						return;
					}

					// force exit to system if no menu
					LOG("[PreGame::%s] server requested quit or rejected connection (reason \"%s\")", __func__, message.c_str());
					handleerror(nullptr, "server requested quit or rejected connection", "Quit message", MBF_OK | MBF_EXCL);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s][NETMSG_{QUIT,REJECT_CONNECT}] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_CREATE_NEWPLAYER: {
				// server will send this first if we're using mid-game join
				// feature to let us know about ourselves (we won't be in
				// gamedata), otherwise skip to gamedata
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string name;

					uint8_t playerNum;
					uint8_t spectator;
					uint8_t team;

					// since the >> operator uses dest size to extract data from
					// the packet, we need to use temp variables of the same
					// size of the packet before converting to dest variable
					pckt >> playerNum;
					pckt >> spectator;
					pckt >> team;
					pckt >> name;

					CPlayer player;
					player.name = name;
					player.spectator = spectator;
					player.team = team;
					player.playerNum = playerNum;

					// add ourselves to avoid crashing if our player-num gets queried
					// we will receive this message a second time (the global broadcast
					// version) which will overwrite the player with the same values as
					// set here
					playerHandler.AddPlayer(player);

					LOG("[PreGame::%s] added new player \"%s\" with number %d to team %d (#active=%d)", __func__, name.c_str(), player.playerNum, player.team, playerHandler.ActivePlayers());
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s][NETMSG_CREATE_NEWPLAYER] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_GAMEDATA: {
				// server first sends this to let us know about teams, allyteams
				// etc. (not if we are joining mid-game as an extra player), see
				// NETMSG_SETPLAYERNUM
				GameDataReceived(packet);
			} break;

			case NETMSG_SETPLAYERNUM: {
				// this is sent after NETMSG_GAMEDATA, to let us know which
				// player number we have (server assigns them based on order
				// of connection)
				if (!CGameSetup::ScriptLoaded())
					throw content_error("No game data received from server");

				const uint8_t playerNum = packet->data[1];

				if (!playerHandler.IsValidPlayer(playerNum))
					throw content_error("Invalid player number received from server");

				// respond with the client data and content checksums
				gu->SetMyPlayer(playerNum);
				clientNet->Send(CBaseNetProtocol::Get().SendClientData(playerNum, ClientData::GetCompressed()));

				LOG("[PreGame::%s] received local player number %i (team %i, allyteam %i), creating LoadScreen", __func__, gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, mapChecksumMsgBuf);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, modChecksumMsgBuf);

				CLoadScreen::CreateDeleteInstance(std::move(gameSetup->MapFileName()), std::move(modFileName), saveFileHandler);

				assert(pregame == this);
				spring::SafeDelete(pregame);
				return;
			} break;

			default: {
				LOG_L(L_WARNING, "[PreGame::%s] unknown packet type (header: %i)", __func__, inbuf[0]);
			} break;
		}
	}
}


void CPreGame::StartServerForDemo(const std::string& demoName)
{
	TdfParser script((gameData->GetSetupText()).c_str(), (gameData->GetSetupText()).size());
	TdfParser::TdfSection* tgame = script.GetRootSection()->sections["game"];

	std::ostringstream moddedDemoScript;

	{
		// server will always use a modified copy of this
		assert(gameSetup->ScriptLoaded());

		// modify the demo's start-script so it can be used to watch the demo
		tgame->AddPair("MapName", gameSetup->mapName);
		tgame->AddPair("Gametype", gameSetup->modName);
		tgame->AddPair("Demofile", demoName);
		tgame->remove("OnlyLocal", false);
		tgame->remove("HostIP", false);
		tgame->remove("HostPort", false);
		tgame->remove("AutohostPort", false);
		tgame->remove("SourcePort", false);
		//tgame->remove("IsHost", false);

		for (auto& section: tgame->sections) {
			if (section.first.size() > 6 && section.first.substr(0, 6) == "player") {
				section.second->AddPair("isfromdemo", 1);
			}
		}

		// is this needed?
		TdfParser::TdfSection* modopts = tgame->construct_subsection("MODOPTIONS");
		modopts->remove("maxspeed", false);
		modopts->remove("minspeed", false);
	}

	script.print(moddedDemoScript);
	gameData->SetSetupText(moddedDemoScript.str());

	// create the server-private demo GameSetup containing the additional player
	std::shared_ptr<CGameSetup> demoGameSetup(new CGameSetup());

	if (!demoGameSetup->Init(moddedDemoScript.str()))
		throw content_error("Demo contains incorrect script");

	LOG("[PreGame::%s] starting GameServer", __func__);
	good_fpu_control_registers("before CGameServer creation");

	gameServer = new CGameServer(clientSetup, gameData, demoGameSetup);
	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());

	good_fpu_control_registers("after CGameServer creation");
	LOG("[PreGame::%s] started GameServer", __func__);
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	ScopedOnceTimer timer("PreGame::ReadDataFromDemo");
	assert(gameServer == nullptr);
	LOG("[PreGame::%s] pre-scanning demo file \"%s\" for game data...", __func__, demoName.c_str());
	CDemoReader scanner(demoName, 0.0f);

	{
		// this does not extract the RNG preseed, use first packet
		// gameData.reset(new GameData(scanner.GetSetupScript()));
		gameData.reset(new GameData(std::shared_ptr<netcode::RawPacket>(scanner.GetData(0.0f))));
		assert(gameData->GetSetupText() == scanner.GetSetupScript());

		if (CGameSetup::LoadReceivedScript(gameData->GetSetupText(), true)) {
			StartServerForDemo(demoName);
		} else {
			throw content_error("Demo contains incorrect script");
		}
	}

	assert(gameServer != nullptr);
}

void CPreGame::GameDataReceived(std::shared_ptr<const netcode::RawPacket> packet)
{
	ScopedOnceTimer timer("PreGame::GameDataReceived");

	try {
		// in demos, gameData is first new'ed in ReadDataFromDemo()
		// in live games it will always still be NULL at this point
		gameData.reset(new GameData(packet));
	} catch (const netcode::UnpackPacketException& ex) {
		throw content_error(std::string("invalid GameData received: ") + ex.what());
	}

	// preseed the synced RNG until GameID-based NETMSG_RANDSEED arrives
	// allows proper randomness in LuaParser when executing defs.lua, etc
	gsRNG.SetSeed(gameData->GetRandomSeed(), true);

	// for demos, ReadDataFromDemo precedes UpdateClientNet -> GameDataReceived
	// this means gameSetup contains data from the original game but we need the
	// modified version (cf StartServerForDemo) which the server already has that
	// contains an extra player
	gameSetup->ResetState();

	if (CGameSetup::LoadReceivedScript(gameData->GetSetupText(), clientSetup->isHost)) {
		assert(gameSetup->ScriptLoaded());
		gu->LoadFromSetup(gameSetup);
		gs->LoadFromSetup(gameSetup);
		// do we really need to do this so early?
		CPlayer::UpdateControlledTeams();
	} else {
		throw content_error("error loading received setup-script");
	}

	// some sanity checks
	for (int p = 0; p < playerHandler.ActivePlayers(); ++p) {
		const CPlayer* player = playerHandler.Player(p);

		if (!playerHandler.IsValidPlayer(player->playerNum))
			throw content_error("Invalid player in game-data");

		if (!teamHandler.IsValidTeam(player->team))
			throw content_error("Invalid team in game-data");

		// TODO: seems not to make sense really
		if (!teamHandler.IsValidAllyTeam(teamHandler.AllyTeam(player->team)))
			throw content_error("Invalid allyteam in game-data");
	}

	// load archives into VFS
	AddMapArchivesToVFS(gameSetup);
	AddModArchivesToVFS(gameSetup);

	{
		// check checksums of map & game
		// mismatches happen on dedicated servers between host and clients
		// we want to know whether the *locally calculated* checksums also
		// differ among clients so use the opportunity
		// NOTE: gu->myPlayerNum is not valid yet, GameData arrives first
		sha512::raw_digest gdMapChecksum;
		sha512::raw_digest asMapChecksum;
		sha512::raw_digest gdModChecksum;
		sha512::raw_digest asModChecksum;
		sha512::hex_digest gdMapChecksumHex;
		sha512::hex_digest asMapChecksumHex;
		sha512::hex_digest gdModChecksumHex;
		sha512::hex_digest asModChecksumHex;

		std::copy(gameData->GetMapChecksum(), gameData->GetMapChecksum() + sha512::SHA_LEN, gdMapChecksum.begin());
		std::copy(gameData->GetModChecksum(), gameData->GetModChecksum() + sha512::SHA_LEN, gdModChecksum.begin());
		std::fill(asMapChecksum.begin(), asMapChecksum.end(), 0);
		std::fill(asModChecksum.begin(), asModChecksum.end(), 0);

		try {
			// gameSetup->MapFileName()
			archiveScanner->CheckArchive(gameSetup->mapName, gdMapChecksum, asMapChecksum);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
		}
		try {
			archiveScanner->CheckArchive(modFileName, gdModChecksum, asModChecksum);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
		}

		sha512::dump_digest(gdMapChecksum, gdMapChecksumHex);
		sha512::dump_digest(gdModChecksum, gdModChecksumHex);
		sha512::dump_digest(asMapChecksum, asMapChecksumHex);
		sha512::dump_digest(asModChecksum, asModChecksumHex);

		std::memset(mapChecksumMsgBuf, 0, sizeof(mapChecksumMsgBuf));
		std::memset(modChecksumMsgBuf, 0, sizeof(modChecksumMsgBuf));
		std::snprintf(mapChecksumMsgBuf, sizeof(mapChecksumMsgBuf), "[PreGame::%s][map-checksums]\n\tserver=%s\n\tclient=%s", __func__, gdMapChecksumHex.data(), asMapChecksumHex.data());
		std::snprintf(modChecksumMsgBuf, sizeof(modChecksumMsgBuf), "[PreGame::%s][mod-checksums]\n\tserver=%s\n\tclient=%s", __func__, gdModChecksumHex.data(), asModChecksumHex.data());
	}

	// script.txt allows to disable demo file recording (host only, used for menu)
	if (clientSetup->isHost && !gameSetup->recordDemo)
		wantDemo = false;

	if (clientNet != nullptr && wantDemo) {
		CDemoRecorder recorder = {gameSetup->mapName, gameSetup->modName, false};

		recorder.WriteSetupText(gameData->GetSetupText());
		recorder.SaveToDemo(packet->data, packet->length, clientNet->GetPacketTime(gs->frameNum));

		assert(!clientNet->GetDemoRecorder()->IsValid());
		clientNet->SetDemoRecorder(std::move(recorder));
		assert(clientNet->GetDemoRecorder()->IsValid());

		LOG("[PreGame::%s] recording demo to \"%s\"", __func__, (clientNet->GetDemoRecorder()->GetName()).c_str());
	}
}

