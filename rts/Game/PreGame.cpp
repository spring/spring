#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <map>
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <SDL_types.h>
#include <set>
#include <cfloat>

#include "mmgr.h"

#include "PreGame.h"
#include "Game.h"
#include "GameVersion.h"
#include "Team.h"
#include "FPUCheck.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "GameData.h"
#include "GlobalSynced.h"
#include "NetProtocol.h"
#include "Net/RawPacket.h"
#include "DemoRecorder.h"
#include "DemoReader.h"
#include "LoadSaveHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Textures/TAPalette.h"
#include "StartScripts/ScriptHandler.h"
#include "UI/InfoConsole.h"
#include "Exceptions.h"


CPreGame* pregame = NULL;
using netcode::RawPacket;

extern Uint8* keys;
extern bool globalQuit;
std::string stupidGlobalMapname;


CPreGame::CPreGame(const LocalSetup* setup) :
		settings(setup),
		savefile(NULL)
{
	localDemoHack = false;
	infoConsole = SAFE_NEW CInfoConsole;

	net = SAFE_NEW CNetProtocol();

	activeController=this;

	if(!settings->isHost)
	{
		net->InitClient(settings->hostip.c_str(), settings->hostport, settings->sourceport, settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
	}
	else
	{
		net->InitLocalClient();
	}
}


CPreGame::~CPreGame()
{
	// don't delete infoconsole, its beeing reused by CGame
}

void CPreGame::LoadSetupscript(const std::string& script)
{
	StartServer(script);
}

void CPreGame::LoadDemo(const std::string& demo)
{
	ReadDataFromDemo(demo);
}

void CPreGame::LoadSavefile(const std::string& save)
{
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
	
	if (!net->Connected())
	{
		if ( ((SDL_GetTicks()/1000) % 2) == 0 )
			PrintLoadMsg("Connecting to server .", false);
		else
			PrintLoadMsg("Connecting to server  ", false);
	}
	else
	{
		PrintLoadMsg("Awaiting server response", false);
	}

	infoConsole->Draw();

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
	std::string map = setup->mapName;
	std::string mod = setup->baseMod;
	std::string script = setup->scriptName;
	
	startupData->SetRandomSeed(static_cast<unsigned>(gu->usRandInt()));
	bool mapHasStartscript = false;
	if (!map.empty())
	{
		// would be better to use MapInfo here, but this doesn't work
		LoadMap(map); // map into VFS
		std::string mapDefFile;
		const std::string extension = map.substr(map.length()-3);
		if (extension == "smf")
			mapDefFile = std::string("maps/")+map.substr(0,map.find_last_of('.'))+".smd";
		else if(extension == "sm3")
			mapDefFile = string("maps/")+map;
		else
			throw std::runtime_error("CPreGame::StartServer(): Unknown extension: " + extension);

		MapParser mp(map);
		LuaTable mapRoot = mp.GetRoot();
		const std::string mapWantedScript = mapRoot.GetString("script",     "");
		const std::string scriptFile      = mapRoot.GetString("scriptFile", "");
		if (!scriptFile.empty()) {
			CScriptHandler::Instance().LoadScriptFile(scriptFile);
		}
		if (!mapWantedScript.empty()) {
			script = mapWantedScript;
			mapHasStartscript = true;
		}
	}
	startupData->SetScript(script);
	// here we now the name of the script to use

	try { // to load the script 
		CScriptHandler::SelectScript(script);
		std::string scriptWantedMod;
		scriptWantedMod = CScriptHandler::Instance().chosenScript->GetModName();
		if (!scriptWantedMod.empty()) {
			mod = scriptWantedMod;
		}
		LoadMod(mod);
	}
	catch (const std::runtime_error& err) { // script not found, so it may be in the modarchive?
		LoadMod(mod); // new map into VFS
		CScriptHandler::SelectScript(script);
	}
	// make sure s is a modname (because the same mod can be in different archives on different computers)
	mod = archiveScanner->ModArchiveToModName(mod);
	std::string modArchive = archiveScanner->ModNameToModArchive(mod);
	startupData->SetMod(mod, archiveScanner->GetModChecksum(modArchive));
	
	if (!mapHasStartscript) {
		std::string mapFromScript = CScriptHandler::Instance().chosenScript->GetMapName();
		if (!mapFromScript.empty() && map != mapFromScript) {
			//TODO unload old map
			LoadMap(mapFromScript, true);
		}
	}
	startupData->SetMap(map, archiveScanner->GetMapChecksum(map));
	setup->LoadStartPositions();

	if (gameSetup) {
		const_cast<CGameSetup*>(gameSetup)->LoadStartPositions(); // only host needs to do this, because
		                                 // client will receive startpos msg from server
	}
	
	good_fpu_control_registers("before CGameServer creation");
	startupData->SetSetup(setup->gameSetupText);
	gameServer = new CGameServer(settings.get(), false, startupData, setup);
	gameServer->AddLocalClient(settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
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
			case NETMSG_SETPLAYERNUM: {
				gu->SetMyPlayer(packet->data[1]);
				logOutput.Print("Became player %i", gu->myPlayerNum);
				
				const int teamID = gs->players[gu->myPlayerNum]->team;
				const CTeam* team = gs->Team(teamID);
				assert(team);
				LoadStartPicture(team->side);

				game = SAFE_NEW CGame(gameData->GetMap(), modArchive, infoConsole, savefile);

				if (savefile) {
					savefile->LoadGame();
				}

				pregame=0;
				delete this;
				return;
			}
			case NETMSG_GAMEDATA: {
				GameDataReceived(packet);
				break;
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
			CGameSetup* tempSetup = new CGameSetup();
			if (tempSetup->Init(data->GetSetup()))
			{
				tempSetup->scriptName = data->GetScript();
				tempSetup->mapName = data->GetMap();
				tempSetup->baseMod = data->GetMod();
				tempSetup->hostDemo = true;
				tempSetup->demoName = demoName;
				
				// all players in the setupscript are from demo
				for (std::vector<PlayerBase>::iterator it = tempSetup->playerStartingData.begin(); it != tempSetup->playerStartingData.end(); ++it)
					it->isFromDemo = true;
				
				// add myself to the script
				PlayerBase myPlayer;
				myPlayer.name = settings->myPlayerName;
				myPlayer.spectator = true;
				tempSetup->playerStartingData.push_back(myPlayer);
				tempSetup->numDemoPlayers = std::max(scanner.GetFileHeader().maxPlayerNum+1, tempSetup->numPlayers);
				tempSetup->numPlayers = tempSetup->numDemoPlayers+1;
				tempSetup->maxSpeed = 10;
				localDemoHack = true;
			}
			else
			{
				throw content_error("Server sent us incorrect script");
			}
			good_fpu_control_registers("before CGameServer creation");
			gameServer = new CGameServer(settings.get(), false, data, tempSetup);
			gameServer->AddLocalClient(settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
			good_fpu_control_registers("after CGameServer creation");
			break;
		}
		
		if (scanner.ReachedEnd())
		{
			throw content_error("End of demo reached and no game data found");
		}
		buf.reset(scanner.GetData(static_cast<float>(FLT_MAX )));
	}

	assert(gameServer);
}

void CPreGame::LoadMap(const std::string& mapName, const bool forceReload)
{
	static bool alreadyLoaded = false;
	
	if (!alreadyLoaded || forceReload)
	{
		CFileHandler* f = SAFE_NEW CFileHandler("maps/" + mapName);
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

void CPreGame::LoadLua()
{
	const string luaGaiaStr  = configHandler.GetString("LuaGaia",  "1");
	const string luaRulesStr = configHandler.GetString("LuaRules", "1");
	gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
	gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);
	if (gs->useLuaGaia) {
		gs->gaiaTeamID = gs->activeTeams;
		gs->gaiaAllyTeamID = gs->activeAllyTeams;
		gs->activeTeams++;
		gs->activeAllyTeams++;
		CTeam* team = gs->Team(gs->gaiaTeamID);
		team->color[0] = 255;
		team->color[1] = 255;
		team->color[2] = 255;
		team->color[3] = 255;
		team->gaia = true;
		gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
	}
}

void CPreGame::GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	gameData.reset(new GameData(packet));
	
	CGameSetup* temp = new CGameSetup();
	if (temp->Init(gameData->GetSetup()))
	{
		temp->scriptName = gameData->GetScript();
		temp->mapName = gameData->GetMap();
		temp->baseMod = gameData->GetMod();
		
		if (localDemoHack)
		{
			temp->hostDemo = true;
				
				// all players in the setupscript are from demo
			for (std::vector<PlayerBase>::iterator it = temp->playerStartingData.begin(); it != temp->playerStartingData.end(); ++it)
				it->isFromDemo = true;
				
			// add myself to the script
			PlayerBase myPlayer;
			myPlayer.name = settings->myPlayerName;
			myPlayer.spectator = true;
			temp->playerStartingData.push_back(myPlayer);
			temp->numDemoPlayers = temp->numPlayers;
			temp->numPlayers = temp->numDemoPlayers+1;
			temp->maxSpeed = 10;
		}
		gameSetup = const_cast<const CGameSetup*>(temp);
		gs->LoadFromSetup(gameSetup);
		LoadLua();
	}
	else
	{
		throw content_error("Server sent us incorrect script");
	}
	
	gs->SetRandSeed(gameData->GetRandomSeed(), true);
	logOutput << "Using map " << gameData->GetMap() << "\n";
	stupidGlobalMapname = gameData->GetMap();
	
	if (net && net->GetDemoRecorder()) {
		net->GetDemoRecorder()->SetName(gameData->GetMap());
		logOutput << "Recording demo " << net->GetDemoRecorder()->GetName() << "\n";
	}
	LoadMap(gameData->GetMap());
	archiveScanner->CheckMap(gameData->GetMap(), gameData->GetMapChecksum());

	logOutput << "Using script " << gameData->GetScript() << "\n";
	CScriptHandler::SelectScript(gameData->GetScript());
	
	logOutput << "Using mod " << gameData->GetMod() << "\n";
	LoadMod(gameData->GetMod());
	modArchive = archiveScanner->ModNameToModArchive(gameData->GetMod());
	archiveScanner->CheckMod(modArchive, gameData->GetModChecksum());
	
}
