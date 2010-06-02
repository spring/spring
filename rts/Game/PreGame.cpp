/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <map>
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <set>
#include <cfloat>

#include "mmgr.h"

#include "PreGame.h"
#include "Game.h"
#include "GameVersion.h"
#include "Player.h"
#include "Sim/Misc/TeamHandler.h"
#include "FPUCheck.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "ClientSetup.h"
#include "GameData.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "NetProtocol.h"
#include "Net/RawPacket.h"
#include "LoadSave/DemoRecorder.h"
#include "LoadSave/DemoReader.h"
#include "LoadSave/LoadSaveHandler.h"
#include "TdfParser.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Sound/ISound.h"
#include "Sound/Music.h"
#include "Map/MapInfo.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystem.h"
#include "Rendering/glFont.h"
#include "UI/InfoConsole.h"
#include "aGui/Gui.h"
#include "Exceptions.h"
#include "TimeProfiler.h"
#include "Net/UnpackPacket.h"

CPreGame* pregame = NULL;
using netcode::RawPacket;
using std::string;

extern boost::uint8_t* keys;
extern bool globalQuit;

CPreGame::CPreGame(const ClientSetup* setup) :
		settings(setup),
		savefile(NULL)
{
	net = new CNetProtocol();
	activeController=this;

	if(!settings->isHost)
	{
		net->InitClient(settings->hostip.c_str(), settings->hostport, settings->myPlayerName, settings->myPasswd, SpringVersion::GetFull());
		timer = SDL_GetTicks();
	}
	else
	{
		net->InitLocalClient();
	}
	ISound::Initialize(); // should have finished until server response arrives
}


CPreGame::~CPreGame()
{
	// don't delete infoconsole, its beeing reused by CGame
	agui::gui->Draw(); // delete leftover gui elements (remove once the gui is drawn ingame)
}

void CPreGame::LoadSetupscript(const std::string& script)
{
	assert(settings->isHost);
	StartServer(script);
}

void CPreGame::LoadDemo(const std::string& demo)
{
	assert(settings->isHost);
	if (!configHandler->Get("DemoFromDemo", false))
		net->DisableDemoRecording();
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
	if (k == SDLK_ESCAPE){
		if(keys[SDLK_LSHIFT]){
			logOutput.Print("User exited");
			globalQuit=true;
		} else
			logOutput.Print("Use shift-esc to quit");
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
	good_fpu_control_registers("CPreGame::Update");
	net->Update();
	UpdateClientNet();

	return true;
}


void CPreGame::StartServer(const std::string& setupscript)
{
	assert(!gameServer);
	ScopedOnceTimer startserver("Starting GameServer");
	GameData* startupData = new GameData();
	CGameSetup* setup = new CGameSetup();
	setup->Init(setupscript);

	startupData->SetRandomSeed(static_cast<unsigned>(gu->usRandInt()));

	if (setup->mapName.empty()) {
		throw content_error("No map selected in startscript");
	}

	// We must map the map into VFS this early, because server needs the start positions.
	// Take care that MapInfo isn't loaded here, as map options aren't available to it yet.
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);

	// Loading the start positions executes the map's Lua.
	// This means start positions can NOT be influenced by map options.
	// (Which is OK, since unitsync does not have map options available either.)
	setup->LoadStartPositions();

	const std::string modArchive = archiveScanner->ArchiveFromName(setup->modName);
	startupData->SetModChecksum(archiveScanner->GetArchiveCompleteChecksum(modArchive));
	const std::string mapArchive = archiveScanner->ArchiveFromName(setup->mapName);
	startupData->SetMapChecksum(archiveScanner->GetArchiveCompleteChecksum(mapArchive));

	good_fpu_control_registers("before CGameServer creation");
	startupData->SetSetup(setup->gameSetupText);
	gameServer = new CGameServer(settings->hostport, (setup->playerStartingData.size() == 1), startupData, setup);
	delete startupData;
	gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	if (net->CheckTimeout(0, true))
	{
		logOutput.Print("Server not reachable");
		globalQuit = true;
		return;
	}

	boost::shared_ptr<const RawPacket> packet;
	while ((packet = net->GetData()))
	{
		const unsigned char* inbuf = packet->data;
		switch (inbuf[0]) {
			case NETMSG_QUIT: {
				const std::string message((char*)(inbuf+3));
				logOutput.Print(message);
				handleerror(NULL, message, "Quit message", MBF_OK | MBF_EXCL);
				break;
			}
			case NETMSG_GAMEDATA: { // server first sends this to let us know about teams, allyteams etc.
				GameDataReceived(packet);
				break;
			}
			case NETMSG_SETPLAYERNUM: { // this is sent afterwards to let us know which playernum we have
				gu->SetMyPlayer(packet->data[1]);
				logOutput.Print("User number %i (team %i, allyteam %i)", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				// When calling this function, mod archives have to be loaded
				// and gu->myPlayerNum has to be set.
				skirmishAIHandler.LoadPreGame();

				const CTeam* team = teamHandler->Team(gu->myTeam);
				assert(team);
				std::string mapStartPic(mapInfo->GetStringValue("Startpic"));
				if (mapStartPic.empty())
					RandomStartPicture(team->side);
				else
					LoadStartPicture(mapStartPic);

				std::string mapStartMusic(mapInfo->GetStringValue("Startmusic"));
				if (!mapStartMusic.empty())
					Channels::BGMusic.Play(mapStartMusic);

				game = new CGame(gameSetup->MapFile(), modArchive, savefile);

				if (savefile) {
					PrintLoadMsg("Loading game");
					savefile->LoadGame();
				}

				UnloadStartPicture();

				pregame=0;
				delete this;
				return;
			}
			default: {
				logOutput.Print("Unknown net-msg received from CPreGame: %i", int(packet->data[0]));
				break;
			}
		}
	}
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	ScopedOnceTimer startserver("Reading demo data");
	assert(!gameServer);
	logOutput.Print("Pre-scanning demo file for game data...");
	CDemoReader scanner(demoName, 0);

	boost::shared_ptr<const RawPacket> buf(scanner.GetData(static_cast<float>(FLT_MAX )));
	while ( buf )
	{
		if (buf->data[0] == NETMSG_GAMEDATA)
		{
			GameData *data = NULL;
			try {
				data = new GameData(boost::shared_ptr<const RawPacket>(buf));
			} catch (netcode::UnpackPacketException &e) {
				throw content_error("Demo contains invalid GameData");
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
			logOutput.Print("Starting GameServer");
			good_fpu_control_registers("before CGameServer creation");
			gameServer = new CGameServer(settings->hostport, true, data, tempSetup);
			gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
			delete data;
			good_fpu_control_registers("after CGameServer creation");
			logOutput.Print("GameServer started");
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
	ScopedOnceTimer startserver("Loading client data");

	try {
		GameData *data = new GameData(packet);

		gameData.reset(data);
	} catch (netcode::UnpackPacketException &e) {
		throw content_error("Server sent us invalid GameData");
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
		gs->LoadFromSetup(gameSetup);
		CPlayer::UpdateControlledTeams();
	} else {
		throw content_error("Server sent us incorrect script");
	}

	gs->SetRandSeed(gameData->GetRandomSeed(), true);
	LogObject() << "Using map " << gameSetup->mapName << "\n";

	if (net && net->GetDemoRecorder()) {
		net->GetDemoRecorder()->SetName(gameSetup->mapName, gameSetup->modName);
		LogObject() << "Recording demo " << net->GetDemoRecorder()->GetName() << "\n";
	}
	vfsHandler->AddArchiveWithDeps(gameSetup->mapName, false);
	archiveScanner->CheckArchive(gameSetup->mapName, gameData->GetMapChecksum());

	// This MUST be loaded this late, since this executes map Lua code which
	// may call Spring.GetMapOptions(), which NEEDS gameSetup to be set!
	if (!mapInfo) {
		mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);
	}

	LogObject() << "Using mod " << gameSetup->modName << "\n";
	vfsHandler->AddArchiveWithDeps(gameSetup->modName, false);
	modArchive = archiveScanner->ArchiveFromName(gameSetup->modName);
	LogObject() << "Using mod archive " << modArchive << "\n";
	archiveScanner->CheckArchive(modArchive, gameData->GetModChecksum());
}
