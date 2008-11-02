#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <map>
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <SDL_types.h>
#include <set>

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
#include "Platform/Clipboard.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Rendering/Textures/TAPalette.h"
#include "StartScripts/ScriptHandler.h"
#include "UI/InfoConsole.h"
#include "UI/MouseHandler.h"
#include "Exceptions.h"

// msvc behaves really strange
#if _MSC_VER
namespace std {
	using ::sin;
}
#endif

const int springDefaultPort = 8452;

CPreGame* pregame = NULL;
using netcode::RawPacket;

extern Uint8* keys;
extern bool globalQuit;
std::string stupidGlobalMapname;

CglList* CPreGame::showList = NULL;
std::string CPreGame::userScript;
std::string CPreGame::userMap;
std::string CPreGame::userMod;


CPreGame::CPreGame(const LocalSetup* setup, const string& demo, const std::string& save)
: settings(setup),
  state(UNKNOWN),
  hasDemo(!demo.empty()),
  hasSave(!save.empty()),
  gameData(0),
  savefile(NULL)
{
	demoFile = gameSetup? gameSetup->demoName : demo;

	if (!gameSetup && demoFile.empty()) {
		gs->noHelperAIs = !!configHandler.GetInt("NoHelperAIs", 0);
		LoadLua();
	}

	infoConsole = SAFE_NEW CInfoConsole;

	pregame = this; // prevent crashes if Select* is called from ctor
	net = SAFE_NEW CNetProtocol();

	//vfsHandler=SAFE_NEW CHpiHandler();

	activeController=this;

	if(!gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			for(int b=0;b<4;++b){
				gs->Team(a)->color[b]=palette.teamColor[a][b];
			}
		}
	}

	if(settings->isHost){
		net->InitLocalClient();
		if (!demoFile.empty())
		{
			ReadDataFromDemo(demoFile);
			state = WAIT_CONNECTING;
		}
		else if(gameSetup){
			StartServer(gameSetup->mapName, gameSetup->baseMod, gameSetup->scriptName);
			state = WAIT_CONNECTING;
		} else if (hasSave) {
			savefile = new CLoadSaveHandler();
			savefile->LoadGameStartInfo(savefile->FindSaveFile(save.c_str()));
			StartServer(gameSetup->mapName, gameSetup->baseMod, gameSetup->scriptName);
			state = WAIT_CONNECTING;
		} else {
			ShowModList();
			state = WAIT_ON_USERINPUT;
		}
	} else {
		if(gameSetup){
			PrintLoadMsg("Connecting to server");
			net->InitClient(settings->hostip.c_str(), settings->hostport, settings->sourceport, settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
			state = WAIT_CONNECTING;
		} else {
			if (hasDemo) {
				net->localDemoPlayback = true;
				state = WAIT_CONNECTING;
				ReadDataFromDemo(demoFile); // scan for GameData
				net->InitLocalClient();
				if (gameSetup) {	// we read a gameSetup from the demofiles
					logOutput.Print("Read GameSetup from Demofile");
				}
				else { // we dont read a GameSetup from demofile (this code was copied from CDemoReader)
					logOutput.Print("Demo file does not contain a setupscript");
					LoadLua();
				}
			}
			else {
				PrintLoadMsg("Connecting to server");
				net->InitClient(settings->hostip.c_str(), settings->hostport, settings->sourceport, settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
				state = WAIT_CONNECTING;
			}
		}
	}
	assert(state != UNKNOWN);
}


CPreGame::~CPreGame()
{
	delete gameData;
	delete infoConsole;
	infoConsole = 0;
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
	if (showList) { //are we currently showing a list?
		showList->KeyPressed(k, isRepeat);
		return 0;
	}

	return 0;
}


bool CPreGame::Draw()
{
	SDL_Delay(10); // milliseconds
	if (!gu->active) {
		return true;
	}

	if (showList) {
		PrintLoadMsg("", false); // just clear screen and set up matrices etc.
	}
	else {
		switch (state) {
			case WAIT_ON_GAMEDATA:
				PrintLoadMsg("Waiting on game data", false);
				break;
			case WAIT_CONNECTING:
				if ( ((SDL_GetTicks()/1000) % 2) == 0 )
					PrintLoadMsg("Connecting to server .", false);
				else
					PrintLoadMsg("Connecting to server  ", false);
				break;
			case UNKNOWN:
			case WAIT_ON_USERINPUT:
			case ALL_READY:
			default:
				PrintLoadMsg("", false); // just clear screen and set up matrices etc.
				break;
		}
	}

	infoConsole->Draw();

	if (showList) {
		showList->Draw();
	}

	return true;
}


bool CPreGame::Update()
{
	good_fpu_control_registers("CPreGame::Update");

	switch (state) {

		case UNKNOWN:
			logOutput.Print("Internal error in CPreGame");
			return false;

		case WAIT_ON_USERINPUT: {
			break;
		}

		case WAIT_CONNECTING:
			if (net->Connected())
				state = WAIT_ON_GAMEDATA; // fall through
			else
				break; // abort

		case WAIT_ON_GAMEDATA: {
			if (gameData) // we recieved tha gameData
				state = ALL_READY;
			else
				break;
		}

		case ALL_READY: {
			ENTER_MIXED;
			const int teamID = gs->players[gu->myPlayerNum]->team;
			const CTeam* team = gs->Team(teamID);
			assert(team);
			if (net->localDemoPlayback) {
				gs->players[gu->myPlayerNum]->StartSpectating();
			}
			LoadStartPicture(team->side);

			game = SAFE_NEW CGame(gameData->GetMap(), modArchive, infoConsole, savefile);

			if (savefile) {
				savefile->LoadGame();
			}

			infoConsole = 0;

			ENTER_UNSYNCED;
			pregame=0;
			delete this;
			return true;
		}
		default:
			assert(false);
			break;
	}

	if(state >= WAIT_CONNECTING){
		net->Update();
		UpdateClientNet();
	}

	return true;
}


void CPreGame::StartServer(std::string map, std::string mod, std::string script)
{
	assert(!gameServer);
	GameData* startupData = new GameData();
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

	if (gameSetup) {
		const_cast<CGameSetup*>(gameSetup)->LoadStartPositions(); // only host needs to do this, because
		                                 // client will receive startpos msg from server
	}
	
	good_fpu_control_registers("before CGameServer creation");
	int myPort = settings->hostport;
	gameServer = new CGameServer(myPort, false, startupData, gameSetup, demoFile);
	if (settings->autohostport > 0) {
		gameServer->AddAutohostInterface(settings->autohostport);
	}
	gameServer->AddLocalClient(settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
	good_fpu_control_registers("after CGameServer creation");
}


void CPreGame::UpdateClientNet()
{
	if (gameData)
	{
		logOutput.Print("Warning: game should have started before");
		return;
	}
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
			} break;
			case NETMSG_GAMEDATA: {
				GameDataReceived(packet);
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
	bool hasSetup = static_cast<bool>(gameSetup);
	CDemoReader scanner(demoName, 0);
	bool demoSetup = static_cast<bool>(gameSetup);
	if (demoSetup && ! hasSetup)
	{
		//HACK: make gs read the setup if we just read it out of the demofile
		gs->LoadFromSetup(gameSetup);
		gu->SetMyPlayer(0); //HACK load data for player 0
	}

	if (!hasSetup)
		gu->SetMyPlayer(scanner.GetFileHeader().maxPlayerNum + 1); //HACK pt. #2: set real player num

	boost::shared_ptr<const RawPacket> buf(scanner.GetData(static_cast<float>(INT_MAX)));
	while ( buf )
	{
		if (buf->data[0] == NETMSG_GAMEDATA)
		{
			GameData *data = new GameData(boost::shared_ptr<const RawPacket>(buf));
			good_fpu_control_registers("before CGameServer creation");
			gameServer = new CGameServer(settings->hostport, false, data, gameSetup, demoName);
			gameServer->AddLocalClient(settings->myPlayerName, std::string(VERSION_STRING_DETAILED));
			good_fpu_control_registers("after CGameServer creation");
			break;
		}
		
		if (scanner.ReachedEnd())
		{
			throw content_error("End of demo reached and no game data found");
		}
		buf.reset(scanner.GetData(static_cast<float>(INT_MAX)));
	}
	assert(gameServer);
}


/** Create a CglList for selecting the map. */
void CPreGame::ShowMapList()
{
	CglList* list = SAFE_NEW CglList("Select map", SelectMap, 2);
	std::vector<std::string> found = filesystem.FindFiles("maps/","{*.sm3,*.smf}");
	std::vector<std::string> arFound = archiveScanner->GetMaps();
	if (found.begin() == found.end() && arFound.begin() == arFound.end()) {
		throw content_error("PreGame couldn't find any map files");
		return;
	}

	std::set<std::string> mapSet; // use a set to sort them
	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); it++) {
		std::string fn(filesystem.GetFilename(*it));
		mapSet.insert(fn.c_str());
	}
	for (std::vector<std::string>::iterator it = arFound.begin(); it != arFound.end(); it++) {
		mapSet.insert((*it).c_str());
	}

	list->AddItem("Random map", "Random map"); // always first
	for (std::set<std::string>::iterator sit = mapSet.begin(); sit != mapSet.end(); ++sit) {
		list->AddItem(sit->c_str(), sit->c_str());
	}
	showList = list;
}


/** Create a CglList for selecting the script. */
void CPreGame::ShowScriptList()
{
	CglList* list = CScriptHandler::Instance().GenList(SelectScript);
	showList = list;
}


/** Create a CglList for selecting the mod. */
void CPreGame::ShowModList()
{
	CglList* list = SAFE_NEW CglList("Select mod", SelectMod, 3);
	std::vector<CArchiveScanner::ModData> found = archiveScanner->GetPrimaryMods();
	if (found.empty()) {
		throw content_error("PreGame couldn't find any mod files");
		return;
	}

	std::map<std::string, std::string> modMap; // name, desc  (using a map to sort)
	for (std::vector<CArchiveScanner::ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		modMap[it->name] = it->description;
	}

	list->AddItem("Random mod", "Random mod"); // always first
	std::map<std::string, std::string>::iterator mit;
	for (mit = modMap.begin(); mit != modMap.end(); ++mit) {
		list->AddItem(mit->first.c_str(), mit->second.c_str());
	}
	showList = list;
}


void CPreGame::SelectMap(std::string s)
{
	if (s == "Random map") {
		s = pregame->showList->items[1 + gu->usRandInt() % (showList->items.size() - 1)];
	}

	delete showList;
	showList = NULL;

	userMap = s;
	pregame->StartServer(userMap, userMod, userScript);
	pregame->state = WAIT_CONNECTING;
}


void CPreGame::SelectScript(std::string s)
{
	delete showList;
	showList = NULL;

	userScript = s;
	pregame->ShowMapList();
}


void CPreGame::SelectMod(std::string s)
{
	if (s == "Random mod") {
		const int index = 1 + (gu->usRandInt() % (showList->items.size() - 1));
		s = showList->items[index];
	}

	delete showList;
	showList = NULL;

	userMod = s;
	pregame->ShowScriptList();
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
	gameData = new GameData(packet);
	
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
	
	if (gameSetup) {
		const_cast<CGameSetup*>(gameSetup)->scriptName = gameData->GetScript();
		const_cast<CGameSetup*>(gameSetup)->mapName = gameData->GetMap();
		const_cast<CGameSetup*>(gameSetup)->baseMod = gameData->GetMod();
	}
}
