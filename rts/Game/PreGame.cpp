/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <cfloat>

#include <SDL_keycode.h>

#include "PreGame.h"

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
#include "Net/GameServer.h"
#include "System/TimeProfiler.h"
#include "UI/InfoConsole.h"
#include "Map/Generation/SimpleMapGenerator.h"

#include "aGui/Gui.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/TdfParser.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
//// #include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/LoadSave/DemoReader.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/Platform/errorhandler.h"
#include "System/Sync/SyncedPrimitiveBase.h"
#include "lib/luasocket/src/restrictions.h"
#ifdef SYNCDEBUG
	#include "System/Sync/SyncDebugger.h"
#endif


using netcode::RawPacket;
using std::string;

CONFIG(bool, DemoFromDemo).defaultValue(false);

CPreGame* pregame = NULL;

CPreGame::CPreGame(boost::shared_ptr<ClientSetup> setup)
	: clientSetup(setup)
	, savefile(NULL)
	, timer(spring_gettime())
	, wantDemo(true)
{
	assert(clientNet == NULL);

	clientNet = new CNetProtocol();
	activeController = this;

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->Initialize(clientSetup->isHost, 64); //FIXME: add actual number of player
#endif

	if (!clientSetup->isHost) {
		//don't allow luasocket to connect to the host
		LOG("Connecting to: %s:%i", clientSetup->hostIP.c_str(), clientSetup->hostPort);
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, clientSetup->hostIP, clientSetup->hostPort, false);
		clientNet->InitClient(clientSetup->hostIP.c_str(), clientSetup->hostPort, clientSetup->myPlayerName, clientSetup->myPasswd, SpringVersion::GetFull());
	} else {
		LOG("Hosting on: %s:%i", clientSetup->hostIP.c_str(), clientSetup->hostPort);
		clientNet->InitLocalClient();
	}
}


CPreGame::~CPreGame()
{
	// delete leftover elements (remove once the gui is drawn ingame)
	// but do not delete infoconsole, it is reused by CGame
	agui::gui->Draw();

	pregame = NULL;
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
	if (k == SDLK_ESCAPE) {
		if (KeyInput::GetKeyModState(KMOD_SHIFT)) {
			LOG("[%s] user exited", __FUNCTION__);
			gu->globalQuit = true;
		} else {
			LOG("Use shift-esc to quit");
		}
	}
	return 0;
}


bool CPreGame::Draw()
{
	spring_msecs(10).sleep();
	ClearScreen();
	agui::gui->Draw();

	font->Begin();

	if (!clientNet->Connected()) {
		if (clientSetup->isHost)
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server to start");
		else
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Connecting to server (%ds)", (spring_gettime() - timer).toSecsi());
	} else {
		font->glPrint(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server response");
	}

	font->glFormat(0.60f, 0.40f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to:   %s", clientNet->ConnectionStr().c_str());

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
	LOG("[%s] using map: %s", __FUNCTION__, setup->mapName.c_str());
	// Load Map archive
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);

	if (mapOnly)
		return;

	// Load Mutators (if any)
	for (const std::string& mut: setup->GetMutatorsCont()) {
		LOG("[%s] using mutator: %s", __FUNCTION__, mut.c_str());
		vfsHandler->AddArchiveWithDeps(mut, false);
	}

	// Load Game archive
	vfsHandler->AddArchiveWithDeps(setup->modName, false);

	modArchive = archiveScanner->ArchiveFromName(setup->modName);
	LOG("[%s] using game: %s (archive: %s)", __FUNCTION__, setup->modName.c_str(), modArchive.c_str());
}

void CPreGame::StartServer(const std::string& setupscript)
{
	assert(!gameServer);
	ScopedOnceTimer startserver("PreGame::StartServer");

	boost::shared_ptr<GameData> startGameData(new GameData());
	boost::shared_ptr<CGameSetup> startGameSetup(new CGameSetup());

	startGameSetup->Init(setupscript);

	startGameData->SetRandomSeed(static_cast<unsigned>(gu->RandInt()));

	if (startGameSetup->mapName.empty()) {
		throw content_error("No map selected in startscript");
	}

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
	LOG("Checksums: game=0x%X map=0x%X", modChecksum, mapChecksum);

	good_fpu_control_registers("before CGameServer creation");
	startGameData->SetSetupText(startGameSetup->setupText);
	gameServer = new CGameServer(clientSetup, startGameData, startGameSetup);

	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetFull());
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	//FIXME move this code to a external file and move that to rts/Net/

	clientNet->Update();

	if (clientNet->CheckTimeout(0, true)) {
		LOG_L(L_WARNING, "Server not reachable");
		SetExitCode(1);
		gu->globalQuit = true;
		return;
	}

	boost::shared_ptr<const RawPacket> packet;

	while ((packet = clientNet->GetData(gs->frameNum))) {
		const unsigned char* inbuf = packet->data;

		if (packet->length <= 0) {
			LOG_L(L_WARNING, "[PreGame::%s] zero-length packet (header: %i)", __FUNCTION__, inbuf[0]);
			continue;
		}

		switch (inbuf[0]) {
			case NETMSG_REJECT_CONNECT:
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;
					pckt >> message;
					LOG("%s", message.c_str());
					handleerror(NULL, "Remote requested quit: " + message, "Quit message", MBF_OK | MBF_EXCL);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid QuitMessage: %s", ex.what());
				}
				break;
			}

			case NETMSG_CREATE_NEWPLAYER: {
				// server will send this first if we're using mid-game join
				// feature, to let us know about ourselves (we won't be in
				// gamedata), otherwise skip to gamedata
				try {
					netcode::UnpackPacket pckt(packet, 3);
					unsigned char spectator, team, playerNum;
					std::string name;
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
					// add ourself, to avoid crashing if our player num gets
					// queried we will receive the same message later, in the
					// game class, which is the global broadcast version
					// the global broadcast will overwrite the user with the
					// same values as here
					playerHandler->AddPlayer(player);

					LOG("[PreGame::%s] added new player %s with number %d to team %d", __FUNCTION__, name.c_str(), player.playerNum, player.team);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s] got invalid NETMSG_CREATE_NEWPLAYER: %s", __FUNCTION__, ex.what());
				}
				break;
			}

			case NETMSG_GAMEDATA: {
				// server first sends this to let us know about teams, allyteams
				// etc. (not if we are joining mid-game as an extra player), see
				// NETMSG_SETPLAYERNUM
				GameDataReceived(packet);
				break;
			}

			case NETMSG_SETPLAYERNUM: {
				// this is sent after NETMSG_GAMEDATA, to let us know which
				// player number we have (server assigns them based on order
				// of connection)
				if (gameSetup == NULL)
					throw content_error("No game data received from server");

				const unsigned char playerNum = packet->data[1];

				if (!playerHandler->IsValidPlayer(playerNum))
					throw content_error("Invalid player number received from server");

				gu->SetMyPlayer(playerNum);

				LOG("[PreGame::%s] user number %i (team %i, allyteam %i)", __FUNCTION__, gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				CLoadScreen::CreateInstance(gameSetup->MapFile(), modArchive, savefile);

				pregame = NULL;
				delete this;
				return;
			}

			default: {
				LOG_L(L_WARNING, "[PreGame::%s] unknown packet type (header: %i)", __FUNCTION__, inbuf[0]);
				break;
			}
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
		assert(gameSetup != NULL);

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

		for (std::map<std::string, TdfParser::TdfSection*>::iterator it = tgame->sections.begin(); it != tgame->sections.end(); ++it) {
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
	boost::shared_ptr<CGameSetup> demoGameSetup(new CGameSetup());

	if (!demoGameSetup->Init(moddedDemoScript.str()))
		throw content_error("Demo contains incorrect script");

	LOG("[%s] starting GameServer", __FUNCTION__);
	good_fpu_control_registers("before CGameServer creation");

	gameServer = new CGameServer(clientSetup, gameData, demoGameSetup);
	gameServer->AddLocalClient(clientSetup->myPlayerName, SpringVersion::GetFull());

	good_fpu_control_registers("after CGameServer creation");
	LOG("[%s] started GameServer", __FUNCTION__);
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	ScopedOnceTimer startserver("PreGame::ReadDataFromDemo");
	assert(gameServer == NULL);
	LOG("[%s] pre-scanning demo file for game data...", __FUNCTION__);
	CDemoReader scanner(demoName, 0);

	{
		gameData.reset(new GameData(scanner.GetSetupScript()));
		if (CGameSetup::LoadReceivedScript(gameData->GetSetupText(), true)) {
			StartServerForDemo(demoName);
		} else {
			throw content_error("Demo contains incorrect script");
		}
	}

	assert(gameServer != NULL);
}

void CPreGame::GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	ScopedOnceTimer startserver("PreGame::GameDataReceived");

	try {
		gameData.reset(new GameData(packet));
	} catch (const netcode::UnpackPacketException& ex) {
		throw content_error(std::string("Server sent us invalid GameData: ") + ex.what());
	}

	// for demos, ReadDataFromDemo precedes UpdateClientNet -> GameDataReceived
	// this means gameSetup contains data from the original game but we need the
	// modified version (cf StartServerForDemo) which the server already has that
	// contains an extra player
	if (gameSetup != NULL)
		SafeDelete(gameSetup);

	if (CGameSetup::LoadReceivedScript(gameData->GetSetupText(), clientSetup->isHost)) {
		assert(gameSetup != NULL);
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
		if (!playerHandler->IsValidPlayer(player->playerNum)) {
			throw content_error("Invalid player in game data");
		}
		if (!teamHandler->IsValidTeam(player->team)) {
			throw content_error("Invalid team in game data");
		}
		if (!teamHandler->IsValidAllyTeam(teamHandler->AllyTeam(player->team))) { // TODO: seems not to make sense really
			throw content_error("Invalid ally team in game data");
		}
	}

	// Load archives into VFS
	AddGameSetupArchivesToVFS(gameSetup, false);

	// Check checksums of map & game
	try {
		archiveScanner->CheckArchive(gameSetup->mapName, gameData->GetMapChecksum());
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "Incompatible map-checksum: %s", ex.what());
	}
	try {
		archiveScanner->CheckArchive(modArchive, gameData->GetModChecksum());
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "Incompatible game-checksum: %s", ex.what());
	}

	if (clientSetup->isHost && !gameSetup->recordDemo) { //script.txt allows to disable demo file recording (host only, used for menu)
		wantDemo = false;
	}

	if (clientNet != NULL && wantDemo) {
		assert(clientNet->GetDemoRecorder() == NULL);

		CDemoRecorder* recorder = new CDemoRecorder(gameSetup->mapName, gameSetup->modName, false);
		recorder->WriteSetupText(gameData->GetSetupText());
		recorder->SaveToDemo(packet->data, packet->length, clientNet->GetPacketTime(gs->frameNum));
		clientNet->SetDemoRecorder(recorder);

		LOG("Recording demo to: %s", (recorder->GetName()).c_str());
	}
}

