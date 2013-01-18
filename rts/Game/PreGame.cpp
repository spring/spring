/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <set>
#include <cfloat>

#include "PreGame.h"

#include "ClientSetup.h"
#include "System/Sync/FPUCheck.h"
#include "Game.h"
#include "GameData.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "GlobalUnsynced.h"
#include "LoadScreen.h"
#include "Player.h"
#include "PlayerHandler.h"
#include "System/TimeProfiler.h"
#include "UI/InfoConsole.h"
#include "Map/Generation/SimpleMapGenerator.h"

#include "aGui/Gui.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/glFont.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/NetProtocol.h"
#include "System/TdfParser.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/LoadSave/DemoReader.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/Platform/errorhandler.h"
#include "lib/luasocket/src/restrictions.h"

using netcode::RawPacket;
using std::string;

CONFIG(bool, DemoFromDemo).defaultValue(false);

CPreGame* pregame = NULL;

CPreGame::CPreGame(const ClientSetup* setup) :
	settings(setup),
	savefile(NULL),
	timer(0),
	wantDemo(true)
{
	net = new CNetProtocol();
	activeController = this;

	if (!settings->isHost) {
		//don't allow luasocket to connect to the host
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, settings->hostIP, settings->hostPort, false);
		net->InitClient(settings->hostIP.c_str(), settings->hostPort, settings->myPlayerName, settings->myPasswd, SpringVersion::GetFull());
		timer = SDL_GetTicks();
	} else {
		net->InitLocalClient();
	}
}


CPreGame::~CPreGame()
{
	// don't delete infoconsole, its beeing reused by CGame
	agui::gui->Draw(); // delete leftover gui elements (remove once the gui is drawn ingame)

	pregame = NULL;
}

void CPreGame::LoadSetupscript(const std::string& script)
{
	assert(settings->isHost);
	StartServer(script);
}

void CPreGame::LoadDemo(const std::string& demo)
{
	assert(settings->isHost);

	if (!configHandler->GetBool("DemoFromDemo"))
		wantDemo = false;

	ReadDataFromDemo(demo);
}

void CPreGame::LoadSavefile(const std::string& save)
{
	assert(settings->isHost);
	savefile = ILoadSaveHandler::Create();
	savefile->LoadGameStartInfo(save.c_str());
	StartServer(savefile->scriptText);
}

int CPreGame::KeyPressed(unsigned short k,bool isRepeat)
{
	if (k == SDLK_ESCAPE) {
		if (keyInput->IsKeyPressed(SDLK_LSHIFT)) {
			LOG("User exited");
			gu->globalQuit = true;
		} else {
			LOG("Use shift-esc to quit");
		}
	}
	return 0;
}


bool CPreGame::Draw()
{
	SDL_Delay(10); // milliseconds
	ClearScreen();
	agui::gui->Draw();

	font->Begin();

	if (!net->Connected())
	{
		if (settings->isHost)
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server to start");
		else
		{
			font->glFormat(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Connecting to server (%d s)", (SDL_GetTicks()-timer)/1000);
		}
	}
	else
	{
		font->glPrint(0.5f, 0.48f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server response");
	}

	font->glFormat(0.60f, 0.40f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to:   %s", net->ConnectionStr().c_str());

	font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "User name: %s", settings->myPlayerName.c_str());

	font->glFormat(0.5f,0.25f,0.8f,FONT_CENTER | FONT_SCALE | FONT_NORM, "Press SHIFT + ESC to quit");
	// credits
	font->glFormat(0.5f,0.06f,1.0f,FONT_CENTER | FONT_SCALE | FONT_NORM, "Spring %s", SpringVersion::GetFull().c_str());
	font->glPrint(0.5f,0.02f,0.6f,FONT_CENTER | FONT_SCALE | FONT_NORM, "This program is distributed under the GNU General Public License, see license.html for more info");

	font->End();

	return true;
}


bool CPreGame::Update()
{
	ENTER_SYNCED_CODE();
	good_fpu_control_registers("CPreGame::Update");
	net->Update();
	UpdateClientNet();
	LEAVE_SYNCED_CODE();

	return true;
}


void CPreGame::StartServer(const std::string& setupscript)
{
	assert(!gameServer);
	ScopedOnceTimer startserver("PreGame::StartServer");
	GameData* startupData = new GameData();
	CGameSetup* setup = new CGameSetup();
	setup->Init(setupscript);

	startupData->SetRandomSeed(static_cast<unsigned>(gu->RandInt()));

	if (setup->mapName.empty()) {
		throw content_error("No map selected in startscript");
	}

	if (setup->mapSeed != 0) {
		CSimpleMapGenerator gen(setup);
		gen.Generate();
	}

	// We must map the map into VFS this early, because server needs the start positions.
	// Take care that MapInfo isn't loaded here, as map options aren't available to it yet.
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);

	// Loading the start positions executes the map's Lua.
	// This means start positions can NOT be influenced by map options.
	// (Which is OK, since unitsync does not have map options available either.)
	setup->LoadStartPositions();

	const std::string& modArchive = archiveScanner->ArchiveFromName(setup->modName);
	const std::string& mapArchive = archiveScanner->ArchiveFromName(setup->mapName);
	startupData->SetModChecksum(archiveScanner->GetArchiveCompleteChecksum(modArchive));
	startupData->SetMapChecksum(archiveScanner->GetArchiveCompleteChecksum(mapArchive));

	good_fpu_control_registers("before CGameServer creation");
	startupData->SetSetup(setup->gameSetupText);
	gameServer = new CGameServer(settings->hostIP, settings->hostPort, startupData, setup);
	delete startupData;
	gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	if (net->CheckTimeout(0, true)) {
		LOG_L(L_WARNING, "Server not reachable");
		SetExitCode(1);
		gu->globalQuit = true;
		return;
	}

	boost::shared_ptr<const RawPacket> packet;
	while ((packet = net->GetData(gs->frameNum)))
	{
		const unsigned char* inbuf = packet->data;
		switch (inbuf[0]) {
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
				// feature, to let us know about ourself (we won't be in
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
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid New player message: %s", ex.what());
				}
				break;
			}
			case NETMSG_GAMEDATA: {
				// server first sends this to let us know about teams, allyteams
				// etc.
				// (not if we are joining mid-game as extra players)
				// see NETMSG_SETPLAYERNUM
				if (gameSetup)
					throw content_error("Duplicate game data received from server");
				GameDataReceived(packet);
				break;
			}
			case NETMSG_SETPLAYERNUM: {
				// this is sent after NETMSG_GAMEDATA, to let us know which
				// playernum we have
				if (!gameSetup)
					throw content_error("No game data received from server");

				unsigned char playerNum = packet->data[1];
				if (!playerHandler->IsValidPlayer(playerNum))
					throw content_error("Invalid player number received from server");

				gu->SetMyPlayer(playerNum);
				LOG("User number %i (team %i, allyteam %i)",
						gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				CLoadScreen::CreateInstance(gameSetup->MapFile(), modArchive, savefile);

				pregame = NULL;
				delete this;
				return;
			}
			default: {
				LOG_L(L_WARNING, "Unknown net-msg received from CPreGame: %i",
						(int)(packet->data[0]));
				break;
			}
		}
	}
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	ScopedOnceTimer startserver("PreGame::ReadDataFromDemo");
	assert(!gameServer);
	LOG("Pre-scanning demo file for game data...");
	CDemoReader scanner(demoName, 0);

	boost::shared_ptr<const RawPacket> buf(scanner.GetData(static_cast<float>(FLT_MAX )));
	while ( buf )
	{
		if (buf->data[0] == NETMSG_GAMEDATA)
		{
			GameData* data = NULL;
			try {
				data = new GameData(boost::shared_ptr<const RawPacket>(buf));
			} catch (const netcode::UnpackPacketException& ex) {
				throw content_error(std::string("Demo contains invalid GameData: ") + ex.what());
			}

			CGameSetup* demoScript = new CGameSetup();
			if (!demoScript->Init(data->GetSetup()))
			{
				throw content_error("Demo contains incorrect script");
			}

			// modify the startscriptscript so it can be used to watch the demo
			TdfParser script(data->GetSetup().c_str(), data->GetSetup().size());
			TdfParser::TdfSection* tgame = script.GetRootSection()->sections["game"];

			tgame->AddPair("MapName", demoScript->mapName);
			tgame->AddPair("Gametype", demoScript->modName);
			tgame->AddPair("Demofile", demoName);
			tgame->AddPair("OnlyLocal", 1);

			for (std::map<std::string, TdfParser::TdfSection*>::iterator it = tgame->sections.begin(); it != tgame->sections.end(); ++it)
			{
				if (it->first.size() > 6 && it->first.substr(0, 6) == "player")
				{
					it->second->AddPair("isfromdemo", 1);
				}
			}

			// add local spectator (and assert we didn't already have MAX_PLAYERS players)
			int myPlayerNum;
			string playerStr;
			for (myPlayerNum = MAX_PLAYERS - 1; myPlayerNum >= 0; --myPlayerNum) {
				char section[50];
				sprintf(section, "game\\player%i", myPlayerNum);
				string s(section);

				if (script.SectionExist(s)) {
					++myPlayerNum;
					sprintf(section, "player%i", myPlayerNum);
					playerStr = std::string(section);
					break;
				}
			}

			assert(!playerStr.empty());


			TdfParser::TdfSection* me = tgame->construct_subsection(playerStr);
			me->AddPair("name", settings->myPlayerName);
			me->AddPair("spectator", 1);
			tgame->AddPair("myplayername", settings->myPlayerName);

			TdfParser::TdfSection* modopts = tgame->construct_subsection("MODOPTIONS");
			modopts->AddPair("MaxSpeed", 20);

			std::ostringstream buf;
			script.print(buf);

			data->SetSetup(buf.str());
			CGameSetup* tempSetup = new CGameSetup();

			if (!tempSetup->Init(buf.str()))
			{
				throw content_error("Demo contains incorrect script");
			}
			LOG("Starting GameServer");
			good_fpu_control_registers("before CGameServer creation");

			gameServer = new CGameServer(settings->hostIP, settings->hostPort, data, tempSetup);
			gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
			delete data;

			good_fpu_control_registers("after CGameServer creation");
			LOG("GameServer started");
			break;
		}

		if (scanner.ReachedEnd())
		{
			throw content_error("End of demo reached and no game data found");
		}
		buf.reset(scanner.GetData(FLT_MAX));
	}

	assert(gameServer);
}

void CPreGame::GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	ScopedOnceTimer startserver("PreGame::GameDataReceived");

	try {
		gameData.reset(new GameData(packet));
	} catch (const netcode::UnpackPacketException& ex) {
		throw content_error(std::string("Server sent us invalid GameData: ") + ex.what());
	}

	CGameSetup* temp = new CGameSetup();

	if (temp->Init(gameData->GetSetup())) {
		if (settings->isHost) {
			const std::string& setupTextStr = gameData->GetSetup();
			std::fstream setupTextFile("_script.txt", std::ios::out);

			setupTextFile.write(setupTextStr.c_str(), setupTextStr.size());
			setupTextFile.close();
		}
		gameSetup = temp;
		gu->LoadFromSetup(gameSetup);
		gs->LoadFromSetup(gameSetup);
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

	gs->SetRandSeed(gameData->GetRandomSeed(), true);
	LOG("Using map: %s", gameSetup->mapName.c_str());

	vfsHandler->AddArchiveWithDeps(gameSetup->mapName, false);
	try {
		archiveScanner->CheckArchive(gameSetup->mapName, gameData->GetMapChecksum());
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "Incompatible map-checksum: %s", ex.what());
	}

	LOG("Using game: %s", gameSetup->modName.c_str());
	vfsHandler->AddArchiveWithDeps(gameSetup->modName, false);
	modArchive = archiveScanner->ArchiveFromName(gameSetup->modName);
	LOG("Using game archive: %s", modArchive.c_str());

	try {
		archiveScanner->CheckArchive(modArchive, gameData->GetModChecksum());
	} catch (const content_error& ex) {
		LOG_L(L_WARNING, "Incompatible game-checksum: %s", ex.what());
	}


	if (net != NULL && wantDemo) {
		assert(net->GetDemoRecorder() == NULL);

		CDemoRecorder* recorder = new CDemoRecorder(gameSetup->mapName, gameSetup->modName);
		recorder->WriteSetupText(gameData->GetSetup());
		recorder->SaveToDemo(packet->data, packet->length, net->GetPacketTime(gs->frameNum));
		net->SetDemoRecorder(recorder);

		LOG("recording demo: %s", (recorder->GetName()).c_str());
	}
}

