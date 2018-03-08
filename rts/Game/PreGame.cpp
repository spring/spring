/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
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

#include "aGui/Gui.h"

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
	, savefile(nullptr)
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
		LOG("[%s] client using IP %s and port %i", __func__, clientSetup->hostIP.c_str(), clientSetup->hostPort);
		// don't allow luasocket to connect to the host
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, clientSetup->hostIP, clientSetup->hostPort, false);
		clientNet->InitClient(clientSetup, SpringVersion::GetFull() + " [" + Platform::GetPlatformStr() + "]");
	} else {
		LOG("[%s] server using IP %s and port %i", __func__, clientSetup->hostIP.c_str(), clientSetup->hostPort);
		clientNet->InitLocalClient();
	}
}


CPreGame::~CPreGame()
{
	// delete leftover elements (remove once the gui is drawn ingame)
	// but do not delete infoconsole, it is reused by CGame
	agui::gui->Draw();

	pregame = nullptr;
}

void CPreGame::LoadSetupscript(const std::string& script)
{
	assert(clientSetup->isHost);
	StartServer(script);
}

void CPreGame::LoadDemo(const std::string& demo)
{
	assert(clientSetup->isHost);

	if (!configHandler->GetBool("DemoFromDemo"))
		wantDemo = false;

	ReadDataFromDemo(demo);
}

void CPreGame::LoadSavefile(const std::string& save, bool usecreg)
{
	assert(clientSetup->isHost);
	savefile = ILoadSaveHandler::Create(usecreg);
	savefile->LoadGameStartInfo(save.c_str());

	StartServer(savefile->scriptText);
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
	agui::gui->Draw();

	font->Begin();

	if (!clientNet->Connected()) {
		if (clientSetup->isHost)
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server to start");
		else
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Connecting to server (%ds)", (spring_gettime() - connectTimer).toSecsi());
	} else {
		font->glPrint(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server response");
	}

	font->glFormat(0.60f, 0.40f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to: %s", clientNet->ConnectionStr().c_str());
	font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "User name: %s", clientSetup->myPlayerName.c_str());

	font->glFormat(0.5f,0.25f,0.8f,FONT_CENTER | FONT_SCALE | FONT_NORM, "Press SHIFT + ESC to quit");
	// credits
	font->glFormat(0.5f,0.06f,1.0f,FONT_CENTER | FONT_SCALE | FONT_NORM, "Spring %s", SpringVersion::GetFull().c_str());
	font->glPrint(0.5f,0.02f,0.6f,FONT_CENTER | FONT_SCALE | FONT_NORM, "This program is distributed under the GNU General Public License, see doc/LICENSE for more info");

	font->End();

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

void CPreGame::AddGameSetupArchivesToVFS(const CGameSetup* setup, bool mapOnly)
{
	LOG("[PreGame::%s] using map: %s", __func__, setup->mapName.c_str());
	// Load Map archive
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);

	if (mapOnly)
		return;

	// Load Mutators (if any)
	for (const std::string& mut: setup->GetMutatorsCont()) {
		LOG("[PreGame::%s] using mutator: %s", __func__, mut.c_str());
		vfsHandler->AddArchiveWithDeps(mut, false);
	}

	// Load Game archive
	vfsHandler->AddArchiveWithDeps(setup->modName, false);

	modArchive = archiveScanner->ArchiveFromName(setup->modName);
	LOG("[PreGame::%s] using game: %s (archive: %s)", __func__, setup->modName.c_str(), modArchive.c_str());
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
	AddGameSetupArchivesToVFS(startGameSetup.get(), true);

	// Loading the start positions executes the map's Lua.
	// This means start positions can NOT be influenced by map options.
	// (Which is OK, since unitsync does not have map options available either.)
	startGameSetup->LoadStartPositions();

	const std::string& modArchive = archiveScanner->ArchiveFromName(startGameSetup->modName);
	const std::string& mapArchive = archiveScanner->ArchiveFromName(startGameSetup->mapName);
	const auto modChecksum = archiveScanner->GetArchiveCompleteChecksum(modArchive);
	const auto mapChecksum = archiveScanner->GetArchiveCompleteChecksum(mapArchive);
	startGameData->SetModChecksum(modChecksum);
	startGameData->SetMapChecksum(mapChecksum);
	LOG("[PreGame::%s] checksums: game=0x%X map=0x%X", __func__, modChecksum, mapChecksum);

	good_fpu_control_registers("before CGameServer creation");
	startGameData->SetSetupText(startGameSetup->setupText);
	gameServer = new CGameServer(clientSetup, startGameData, startGameSetup);

	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetFull() + " [" + Platform::GetPlatformStr() + "]");
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
					handleerror(nullptr, "server requested quit or rejected connection: " + message, "Quit message", MBF_OK | MBF_EXCL);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s][NETMSG_{QUIT,REJECT_CONNECT}] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_CREATE_NEWPLAYER: {
				// server will send this first if we're using mid-game join
				// feature, to let us know about ourselves (we won't be in
				// gamedata), otherwise skip to gamedata
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string name;

					uint8_t playerNum;
					uint8_t spectator;
					uint8_t team;

					// since the >> operator uses dest size to extract data from
					// the packet, we need to use temp variables of the same
					// size of the packet, before converting to dest variable
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
					playerHandler->AddPlayer(player);

					LOG("[PreGame::%s] added new player %s with number %d to team %d", __func__, name.c_str(), player.playerNum, player.team);
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
				if (gameSetup == nullptr)
					throw content_error("No game data received from server");

				const uint8_t playerNum = packet->data[1];

				if (!playerHandler->IsValidPlayer(playerNum))
					throw content_error("Invalid player number received from server");

				gu->SetMyPlayer(playerNum);

				LOG("[PreGame::%s] received user number %i (team %i, allyteam %i), creating load-screen", __func__, gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				// respond with the client data and content checksums
				clientNet->Send(CBaseNetProtocol::Get().SendClientData(playerNum, ClientData::GetCompressed()));

				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, mapChecksumMsgBuf);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, modChecksumMsgBuf);

				CLoadScreen::CreateDeleteInstance(gameSetup->MapFile(), modArchive, savefile);

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
		assert(gameSetup != nullptr);

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

		for (auto it = tgame->sections.begin(); it != tgame->sections.end(); ++it) {
			if (it->first.size() > 6 && it->first.substr(0, 6) == "player") {
				it->second->AddPair("isfromdemo", 1);
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
	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetFull() + " [" + Platform::GetPlatformStr() + "]");

	good_fpu_control_registers("after CGameServer creation");
	LOG("[PreGame::%s] started GameServer", __func__);
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	ScopedOnceTimer timer("PreGame::ReadDataFromDemo");
	assert(gameServer == nullptr);
	LOG("[PreGame::%s] pre-scanning demo file for game data...", __func__);
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
		throw content_error(std::string("Server sent us invalid GameData: ") + ex.what());
	}

	// preseed the synced RNG until GameID-based NETMSG_RANDSEED arrives
	// allows proper randomness in LuaParser when executing defs.lua, etc
	gsRNG.SetSeed(gameData->GetRandomSeed(), true);

	// for demos, ReadDataFromDemo precedes UpdateClientNet -> GameDataReceived
	// this means gameSetup contains data from the original game but we need the
	// modified version (cf StartServerForDemo) which the server already has that
	// contains an extra player
	if (gameSetup != nullptr)
		spring::SafeDelete(gameSetup);

	if (CGameSetup::LoadReceivedScript(gameData->GetSetupText(), clientSetup->isHost)) {
		assert(gameSetup != nullptr);
		gu->LoadFromSetup(gameSetup);
		gs->LoadFromSetup(gameSetup);
		// do we really need to do this so early?
		CPlayer::UpdateControlledTeams();
	} else {
		throw content_error("Server sent us incorrect script");
	}

	// some sanity checks
	for (int p = 0; p < playerHandler->ActivePlayers(); ++p) {
		const CPlayer* player = playerHandler->Player(p);

		if (!playerHandler->IsValidPlayer(player->playerNum))
			throw content_error("Invalid player in game data");

		if (!teamHandler.IsValidTeam(player->team))
			throw content_error("Invalid team in game data");

		// TODO: seems not to make sense really
		if (!teamHandler.IsValidAllyTeam(teamHandler.AllyTeam(player->team)))
			throw content_error("Invalid ally team in game data");

	}

	// load archives into VFS
	AddGameSetupArchivesToVFS(gameSetup, false);

	// check checksums of map & game
	// mismatches happen on dedicated servers between host and clients
	// we want to know whether the *locally calculated* checksums also
	// differ among clients so use the opportunity to send them
	// NOTE: gu->myPlayerNum is not valid yet, GameData arrives first
	std::pair<unsigned int, unsigned int> mapChecksums = {gameData->GetMapChecksum(), 0};
	std::pair<unsigned int, unsigned int> modChecksums = {gameData->GetModChecksum(), 0};

	try {
		archiveScanner->CheckArchive(gameSetup->mapName, mapChecksums.first, mapChecksums.second);
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
	}
	try {
		archiveScanner->CheckArchive(modArchive, modChecksums.first, modChecksums.second);
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
	}

	std::memset(mapChecksumMsgBuf, 0, sizeof(mapChecksumMsgBuf));
	std::memset(modChecksumMsgBuf, 0, sizeof(modChecksumMsgBuf));
	std::snprintf(mapChecksumMsgBuf, sizeof(mapChecksumMsgBuf), "[PreGame::%s][map-checksums={0x%x,0x%x}]", __func__, mapChecksums.first, mapChecksums.second);
	std::snprintf(modChecksumMsgBuf, sizeof(modChecksumMsgBuf), "[PreGame::%s][mod-checksums={0x%x,0x%x}]", __func__, modChecksums.first, modChecksums.second);

	// script.txt allows to disable demo file recording (host only, used for menu)
	if (clientSetup->isHost && !gameSetup->recordDemo)
		wantDemo = false;

	if (clientNet != nullptr && wantDemo) {
		assert(clientNet->GetDemoRecorder() == nullptr);

		CDemoRecorder* recorder = new CDemoRecorder(gameSetup->mapName, gameSetup->modName, false);
		recorder->WriteSetupText(gameData->GetSetupText());
		recorder->SaveToDemo(packet->data, packet->length, clientNet->GetPacketTime(gs->frameNum));
		clientNet->SetDemoRecorder(recorder);

		LOG("PreGame::%s] recording demo to \"%s\"", __func__, (recorder->GetName()).c_str());
	}
}

