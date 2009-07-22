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
#include "Sim/Misc/TeamHandler.h"
#include "FPUCheck.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "ClientSetup.h"
#include "GameData.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "NetProtocol.h"
#include "Net/RawPacket.h"
#include "DemoRecorder.h"
#include "DemoReader.h"
#include "LoadSaveHandler.h"
#include "TdfParser.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Sound/Sound.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystem.h"
#include "Rendering/glFont.h"
#include "Rendering/Textures/TAPalette.h"
#include "StartScripts/ScriptHandler.h"
#include "UI/InfoConsole.h"
#include "Exceptions.h"


CPreGame* pregame = NULL;
using netcode::RawPacket;

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
		net->InitClient(settings->hostip.c_str(), settings->hostport, settings->sourceport, settings->myPlayerName, SpringVersion::GetFull());
		timer = SDL_GetTicks();
	}
	else
	{
		net->InitLocalClient();
	}
	sound = new CSound(); // should have finished until server response arrives
}


CPreGame::~CPreGame()
{
	// don't delete infoconsole, its beeing reused by CGame
}

void CPreGame::LoadSetupscript(const std::string& script)
{
	assert(settings->isHost);
	StartServer(script);
}

void CPreGame::LoadDemo(const std::string& demo)
{
	assert(settings->isHost);
	ReadDataFromDemo(demo);
}

void CPreGame::LoadSavefile(const std::string& save)
{
	assert(settings->isHost);
	savefile = new CLoadSaveHandler();
	savefile->LoadGameStartInfo(savefile->FindSaveFile(save.c_str()));
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

	font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "Using playername: %s", settings->myPlayerName.c_str());

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
	GameData* startupData = new GameData();
	CGameSetup* setup = new CGameSetup();
	setup->Init(setupscript);

	startupData->SetRandomSeed(static_cast<unsigned>(gu->usRandInt()));
	if (! setup->mapName.empty())
	{
		// would be better to use MapInfo here, but this doesn't work
		LoadMap(setup->mapName); // map into VFS
		std::string mapDefFile;
		const std::string extension = setup->mapName.substr(setup->mapName.length()-3);
		if (extension == "smf")
			mapDefFile = std::string("maps/")+setup->mapName.substr(0,setup->mapName.find_last_of('.'))+".smd";
		else if(extension == "sm3")
			mapDefFile = string("maps/")+setup->mapName;
		else
			throw std::runtime_error("CPreGame::StartServer(): Unknown extension: " + extension);

		MapParser mp(setup->mapName);
		LuaTable mapRoot = mp.GetRoot();
		const std::string mapWantedScript = mapRoot.GetString("script",     "");
		const std::string scriptFile      = mapRoot.GetString("scriptFile", "");

		if (!mapWantedScript.empty()) {
			setup->scriptName = mapWantedScript;
		}
	}
	// here we now the name of the script to use

	CScriptHandler::SelectScript(setup->scriptName);
	std::string scriptWantedMod;
	scriptWantedMod = CScriptHandler::Instance().chosenScript->GetModName();
	if (!scriptWantedMod.empty()) {
		setup->modName = archiveScanner->ModArchiveToModName(scriptWantedMod);
	}
	LoadMod(setup->modName);

	std::string modArchive = archiveScanner->ModNameToModArchive(setup->modName);
	startupData->SetModChecksum(archiveScanner->GetModChecksum(modArchive));

	std::string mapFromScript = CScriptHandler::Instance().chosenScript->GetMapName();
	if (!mapFromScript.empty() &&  setup->mapName != mapFromScript) {
		//TODO unload old map
		LoadMap(mapFromScript, true);
	}

	startupData->SetMapChecksum(archiveScanner->GetMapChecksum(setup->mapName));
	setup->LoadStartPositions();

	good_fpu_control_registers("before CGameServer creation");
	startupData->SetSetup(setup->gameSetupText);
	gameServer = new CGameServer(settings.get(), false, startupData, setup);
	gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	if (!net->Active())
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
			case NETMSG_GAMEDATA: { // server first sends this to let us know about teams, allyteams etc.
				GameDataReceived(packet);
				break;
			}
			case NETMSG_SETPLAYERNUM: { // this is sent afterwards to let us know which playernum we have
				gu->SetMyPlayer(packet->data[1]);
				logOutput.Print("Became player %i (team %i, allyteam %i)", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				const CTeam* team = teamHandler->Team(gu->myTeam);
				assert(team);
				LoadStartPicture(team->side);

				game = new CGame(gameSetup->mapName, modArchive, savefile);

				if (savefile) {
					savefile->LoadGame();
				}

				pregame=0;
				delete this;
				return;
			}
			default: {
				logOutput.Print("Unknown net-msg recieved from CPreGame: %i", int(packet->data[0]));
				break;
			}
		}
	}
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	assert(!gameServer);
	logOutput.Print("Pre-scanning demo file for game data...");
	CDemoReader scanner(demoName, 0);

	boost::shared_ptr<const RawPacket> buf(scanner.GetData(static_cast<float>(FLT_MAX )));
	while ( buf )
	{
		if (buf->data[0] == NETMSG_GAMEDATA)
		{
			GameData *data = new GameData(boost::shared_ptr<const RawPacket>(buf));

			CGameSetup* demoScript = new CGameSetup();
			if (!demoScript->Init(data->GetSetup()))
			{
				throw content_error("Demo contains incorrect script");
			}

			// modify the startscriptscript so it can be used to watch the demo
			TdfParser script(data->GetSetup().c_str(), data->GetSetup().size());
			TdfParser::TdfSection* tgame = script.GetRootSection()->sections["game"];

			tgame->AddPair("ScriptName", demoScript->scriptName);
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
			gameServer = new CGameServer(settings.get(), true, data, tempSetup);
			gameServer->AddLocalClient(settings->myPlayerName, SpringVersion::GetFull());
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

void CPreGame::LoadMap(const std::string& mapName, const bool forceReload)
{
	static bool alreadyLoaded = false;

	if (!alreadyLoaded || forceReload)
	{
		CFileHandler* f = new CFileHandler("maps/" + mapName);
		if (!f->FileExists()) {
			vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
			if (ars.empty()) {
				throw content_error("Couldn't find any archives for map '" + mapName + "'.");
			}
			for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
				if (!vfsHandler->AddArchive(*i, false)) {
					throw content_error("Couldn't load archive '" + *i + "' for map '" + mapName + "'.");
				}
			}
		}
		delete f;
		alreadyLoaded = true;
	}
}


void CPreGame::LoadMod(const std::string& modName)
{
	static bool alreadyLoaded = false;

	if (!alreadyLoaded) {
		// Map all required archives depending on selected mod(s)
		std::string modArchive = archiveScanner->ModNameToModArchive(modName);
		vector<string> ars = archiveScanner->GetArchives(modArchive);
		if (ars.empty()) {
			throw content_error("Couldn't find any archives for mod '" + modName + "'");
		}
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {

			if (!vfsHandler->AddArchive(*i, false)) {
				throw content_error("Couldn't load archive '" + *i + "' for mod '" + modName + "'.");
			}
		}
		alreadyLoaded = true;
	}
}

void CPreGame::GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	gameData.reset(new GameData(packet));

	CGameSetup* temp = new CGameSetup();
	if (temp->Init(gameData->GetSetup()))
	{
		gameSetup = const_cast<const CGameSetup*>(temp);
		gs->LoadFromSetup(gameSetup);
		CPlayer::UpdateControlledTeams();
	}
	else
	{
		throw content_error("Server sent us incorrect script");
	}

	gs->SetRandSeed(gameData->GetRandomSeed(), true);
	LogObject() << "Using map " << gameSetup->mapName << "\n";

	if (net && net->GetDemoRecorder()) {
		net->GetDemoRecorder()->SetName(gameSetup->mapName);
		LogObject() << "Recording demo " << net->GetDemoRecorder()->GetName() << "\n";
	}
	LoadMap(gameSetup->mapName);
	archiveScanner->CheckMap(gameSetup->mapName, gameData->GetMapChecksum());

	LogObject() << "Using script " << gameSetup->scriptName << "\n";
	CScriptHandler::SelectScript(gameSetup->scriptName);

	LogObject() << "Using mod " << gameSetup->modName << "\n";
	LoadMod(gameSetup->modName);
	modArchive = archiveScanner->ModNameToModArchive(gameSetup->modName);
	LogObject() << "Using mod archive " << modArchive << "\n";
	archiveScanner->CheckMod(modArchive, gameData->GetModChecksum());
}
