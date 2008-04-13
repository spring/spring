#include "StdAfx.h"
#include "PreGame.h"
#include <map>
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <SDL_types.h>
#include <set>
#include <cmath>
#include "Game.h"
#include "Team.h"
#include "FPUCheck.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "GameData.h"
#include "NetProtocol.h"
#include "DemoRecorder.h"
#include "DemoReader.h"
#include "LoadSaveHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Platform/Clipboard.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/TAPalette.h"
#include "StartScripts/ScriptHandler.h"
#include "UI/InfoConsole.h"
#include "UI/MouseHandler.h"
#include "mmgr.h"


CPreGame* pregame=0;

extern Uint8 *keys;
extern bool globalQuit;
std::string stupidGlobalMapname;

CglList* CPreGame::showList = 0;

CPreGame::CPreGame(bool server, const string& demo, const std::string& save)
: server(server),
  state(UNKNOWN),
  hasDemo(!demo.empty()),
  hasSave(!save.empty()),
  savefile(NULL),
  gameData(0),
  serverStartupData(0)
{
	demoFile = gameSetup? gameSetup->demoName : demo;

	infoConsole = SAFE_NEW CInfoConsole;

	pregame = this; // prevent crashes if Select* is called from ctor
	net = SAFE_NEW CNetProtocol();

	//hpiHandler=SAFE_NEW CHpiHandler();

	activeController=this;

	if(!gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			for(int b=0;b<4;++b){
				gs->Team(a)->color[b]=palette.teamColor[a][b];
			}
		}
	}

	if(server){
		net->InitLocalClient(gameSetup ? gameSetup->myPlayerNum : 0);
		serverStartupData = new GameData();
		if(gameSetup){
			CScriptHandler::SelectScript(gameSetup->scriptName);
			SelectScript(gameSetup->scriptName);
			if (!gameSetup->saveName.empty()) {
				savefile = new CLoadSaveHandler();
				savefile->LoadGameStartInfo(savefile->FindSaveFile(gameSetup->saveName.c_str()));
			}
			SelectMap(gameSetup->mapName);
			SelectMod(gameSetup->baseMod);
			state = WAIT_CONNECTING;
		} else if (hasSave) {
			savefile = new CLoadSaveHandler();
			savefile->LoadGameStartInfo(savefile->FindSaveFile(save.c_str()));
			CScriptHandler::SelectScript("Commanders");
			SelectScript("Commanders");
			SelectMap(gameSetup->mapName);
			SelectMod(gameSetup->baseMod);
			state = WAIT_CONNECTING;
		} else {
			ShowScriptList();
			state = WAIT_ON_USERINPUT;
		}
	} else {
		if(gameSetup){
			PrintLoadMsg("Connecting to server");
			net->InitClient(gameSetup->hostip.c_str(),gameSetup->hostport,gameSetup->sourceport, gameSetup->myPlayerNum);
			state = WAIT_CONNECTING;
		} else {
			if (hasDemo) {
				net->localDemoPlayback = true;
				state = WAIT_CONNECTING;
				ReadDataFromDemo(demoFile); // scan for GameData
				net->InitLocalClient(0);
				if (gameSetup) {	// we read a gameSetup from the demofiles
					logOutput.Print("Read GameSetup from Demofile");
				}
				else	// we dont read a GameSetup from demofile (this code was copied from CDemoReader)
				{
					logOutput.Print("Demo file does not contain a setupscript");
					// Didn't get a CGameSetup script
					// FIXME: duplicated in Main.cpp
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
			}
			else {
				userInput=configHandler.GetString("address","");
				writingPos = userInput.length();
				userPrompt = "Enter server address: ";
				state = WAIT_ON_ADDRESS;
				userWriting = true;
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
	if(showList){					//are we currently showing a list?
		showList->KeyPressed(k, isRepeat);
		return 0;
	}

	if (userWriting){
		keys[k] = true;
		if (k == SDLK_v && keys[SDLK_LCTRL]){
			CClipboard clipboard;
			const string text = clipboard.GetContents();
			userInput.insert(writingPos, text);
			writingPos += text.length();
			return 0;
		}
		if(k == SDLK_BACKSPACE){
			if (!userInput.empty() && (writingPos > 0)) {
				userInput.erase(writingPos - 1, 1);
				writingPos--;
			}
			return 0;
		}
		if(k == SDLK_DELETE){
			if (!userInput.empty() && (writingPos < (int)userInput.size())) {
				userInput.erase(writingPos, 1);
			}
			return 0;
		}
		else if(k==SDLK_LEFT) {
			writingPos = max(0, min((int)userInput.length(), writingPos - 1));
		}
		else if(k==SDLK_RIGHT) {
			writingPos = max(0, min((int)userInput.length(), writingPos + 1));
		}
		else if(k==SDLK_HOME) {
			writingPos = 0;
		}
		else if(k==SDLK_END) {
			writingPos = (int)userInput.length();
		}
		if(k == SDLK_RETURN){
			userWriting=false;
			return 0;
		}
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

	if (!showList) {
		switch (state) {
			case WAIT_ON_GAMEDATA:
				PrintLoadMsg("Waiting on game data", false);
				break;
			case WAIT_CONNECTING:
				if ( ((SDL_GetTicks()/1000) % 2) == 0 )
					PrintLoadMsg("Connecting to server .");
				else
					PrintLoadMsg("Connecting to server  ");
				break;
			case UNKNOWN:
			case WAIT_ON_ADDRESS:
			case WAIT_ON_USERINPUT:
			case ALL_READY:
			default:
				PrintLoadMsg("", false); // just clear screen and set up matrices etc.
				break;
		}
	} else {
		PrintLoadMsg("", false); // just clear screen and set up matrices etc.
	}

	infoConsole->Draw();

	if(userWriting){
		const std::string tempstring = userPrompt + userInput;

		const float xStart = 0.10f;
		const float yStart = 0.75f;

		const float fontScale = 1.0f;

		// draw the caret
		const int caretPos = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPos);
		const float caretWidth = font->CalcTextWidth(caretStr.c_str());
		char c = userInput[writingPos];
		if (c == 0) { c = ' '; }
		const float cw = fontScale * font->CalcCharWidth(c);
		const float csx = xStart + (fontScale * caretWidth);
		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + std::sin((float)SDL_GetTicks() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, yStart, csx + cw, yStart + fontScale * font->GetHeight());
		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		font->glPrintAt(xStart, yStart, fontScale, tempstring.c_str());
	}

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

		case WAIT_ON_ADDRESS: {
			if (userWriting)
				break;

			configHandler.SetString("address",userInput);
			net->InitClient(userInput.c_str(),8452,0, 0);
			state = WAIT_CONNECTING;
			// fall trough
		}

		case WAIT_ON_USERINPUT: {
			break;
		}

		case WAIT_CONNECTING:
			if ((server || hasDemo) && !gameServer) {
				good_fpu_control_registers("before CGameServer creation");
				int myPort = gameSetup? gameSetup->hostport : 8452;
				gameServer = new CGameServer(myPort, serverStartupData, gameSetup, demoFile);
				if (gameSetup && gameSetup->autohostport > 0)
					gameServer->AddAutohostInterface(gameSetup->autohostport);
				gameServer->AddLocalClient();
				good_fpu_control_registers("after CGameServer creation");
			}

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
			if (net->localDemoPlayback)
				gs->players[gu->myPlayerNum]->StartSpectating();
			LoadStartPicture(team->side);

			game = SAFE_NEW CGame(gameData->GetMap(), modArchive, infoConsole);

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

	if(state > WAIT_ON_ADDRESS){
		net->Update();
		UpdateClientNet();
	}

	return true;
}

void CPreGame::UpdateClientNet()
{
	if (!net->IsActiveConnection())
	{
		logOutput.Print("Server not reachable");
		globalQuit = true;
		return;
	}

	RawPacket* packet = 0;
	while ( (packet = net->GetData()) )
	{
		const unsigned char* inbuf = packet->data;
		switch (inbuf[0]) {

			case NETMSG_GAMEDATA: {
				GameDataRecieved(packet);
				break;
			}

			case NETMSG_MAPDRAW: {
			} break;

			case NETMSG_RANDSEED: {
				gs->SetRandSeed(*((unsigned int*)&inbuf[1]));
				break;
			}

			case NETMSG_SYSTEMMSG:
			case NETMSG_CHAT: {
				// int player = inbuf[inbufpos + 2];
				string s = (char*) (inbuf + 3);
				logOutput.Print(s);
			} break;

			case NETMSG_STARTPOS: {
				// copied from CGame
				// unsigned player = inbuf[1];
				int team = inbuf[2];
				if(team>=gs->activeTeams || team<0 || !gameSetup){
					logOutput.Print("Got invalid team num %i in startpos msg",team);
				} else {
					if(inbuf[3]!=2)
						gameSetup->readyTeams[team]=!!inbuf[3];
					gs->Team(team)->startPos.x=*(float*)&inbuf[4];
					gs->Team(team)->startPos.y=*(float*)&inbuf[8];
					gs->Team(team)->startPos.z=*(float*)&inbuf[12];
				}
				break;
			}

			case NETMSG_SETPLAYERNUM: {
				gu->myPlayerNum = inbuf[1];
				logOutput.Print("Became player %i", gu->myPlayerNum);
			} break;

			case NETMSG_PLAYERNAME: {
				gs->players[inbuf[2]]->playerName = (char*) (inbuf + 3);
				gs->players[inbuf[2]]->readyToStart = true;
				gs->players[inbuf[2]]->active = true;
				if (net->GetDemoRecorder())
					net->GetDemoRecorder()->SetMaxPlayerNum(inbuf[2]);
			} break;

			case NETMSG_QUIT: {
				globalQuit = true;
				break;
			}

			case NETMSG_USER_SPEED: {
			} break;
			case NETMSG_INTERNAL_SPEED: {
			} break;

			case NETMSG_SENDPLAYERSTAT: {
			} break;
			
			case NETMSG_TEAM: {
				const int player = (int)inbuf[1];
				const unsigned char action = inbuf[2];

				switch (action)
				{
					case TEAMMSG_SELFD: {
						break;
					}
					case TEAMMSG_GIVEAWAY: {
						break;
					}
					case TEAMMSG_RESIGN: {
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						//TODO is this enought?
						int newTeam = int(inbuf[3]);
						gs->players[player]->team = newTeam;
						if (gs->Team(newTeam)->leader == -1)
							gs->Team(newTeam)->leader = player;
						break;
					}
					default: {
						logOutput.Print("Unknown action in NETMSG_TEAM (%i) from player %i", action, player);
					}
				}
				break;
			}

			case NETMSG_PAUSE: {
				// these can get into the network stream here -- Kloot
				int playerNum = (int) inbuf[1];
				bool paused = !!inbuf[2];
				logOutput.Print(paused? "player %i paused the game": "player %i unpaused the game", playerNum);
			} break;

			case NETMSG_PLAYERINFO:
				break;

			default: {
				logOutput.Print("Unknown net-msg recieved from CPreGame: %i", int(inbuf[0]));
				break;
			}
		}
		delete packet;
	}
}

void CPreGame::ReadDataFromDemo(const std::string& demoName)
{
	logOutput.Print("Pre-scanning demo file for game data...");
	CDemoReader scanner(demoName, 0);

	gu->myPlayerNum = scanner.GetFileHeader().maxPlayerNum + 1;

	RawPacket* buf = 0;
	while ( (buf = scanner.GetData(static_cast<float>(INT_MAX))) ) {
		if (buf->data[0] == NETMSG_GAMEDATA)
		{
			serverStartupData = new GameData(*buf);
			delete buf;
			break;
		}
		delete buf;
		if (scanner.ReachedEnd())
		{
			throw content_error("End of demo reached and no game data found");
		}
	}
}

/** Create a CglList for selecting the script. */
void CPreGame::ShowScriptList()
{
	CglList* list = CScriptHandler::Instance().GenList(SelectScript);
	showList = list;
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

void CPreGame::SelectScript(std::string s)
{
	delete showList;
	showList = 0;
	assert(pregame->serverStartupData);
	pregame->serverStartupData->SetScript(s);
	
	// if the script specify a map, use this and don't ask the user
	std::string map = CScriptHandler::Instance().chosenScript->GetMapName();
	if (map == "")
		pregame->ShowMapList();
	else
		SelectMap(map);
}

void CPreGame::SelectMap(std::string s)
{
	if (s == "Random map") {
		s = pregame->showList->items[1 + gu->usRandInt() % (showList->items.size() - 1)];
	}
	delete showList;
	showList = 0;
	LoadMap(s);
	if (gameSetup)
		gameSetup->LoadStartPositions(); // only host needs to do this, because client will recieve startpos msg from server
	pregame->serverStartupData->SetMap(s, archiveScanner->GetMapChecksum(s));
	
	std::string mod = CScriptHandler::Instance().chosenScript->GetModName();
	if (mod == "")
		pregame->ShowModList();
	else
		SelectMod(mod);
}

void CPreGame::SelectMod(std::string s)
{
	if (s == "Random mod") {
		const int index = 1 + (gu->usRandInt() % (showList->items.size() - 1));
		s = showList->items[index];
	}
	delete showList;
	showList = 0;
	LoadMod(s);
	// make sure s is a modname (because the same mod can be in different archives on different computers)
	s = archiveScanner->ModArchiveToModName(s);
	std::string modArchive = archiveScanner->ModNameToModArchive(s);
	pregame->serverStartupData->SetMod(s, archiveScanner->GetModChecksum(modArchive));
	pregame->state = WAIT_CONNECTING; // last thing to set, so start server now
}

void CPreGame::LoadMap(const std::string& mapName)
{
	static bool alreadyLoaded = false;
	
	if (!alreadyLoaded)
	{
		CFileHandler* f = SAFE_NEW CFileHandler("maps/" + mapName);
		if (!f->FileExists()) {
			vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
			if (ars.empty())
				throw content_error("Couldn't find any archives for map '" + mapName + "'.");
			for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
				if (!hpiHandler->AddArchive(*i, false))
					throw content_error("Couldn't load archive '" + *i + "' for map '" + mapName + "'.");
			}
		}
		delete f;
		alreadyLoaded = true;
	}
}

void CPreGame::LoadMod(const std::string& modName)
{
	static bool alreadyLoaded = false;
	
	if (!alreadyLoaded)
	{
		std::string modArchive = archiveScanner->ModNameToModArchive(modName);
		// Map all required archives depending on selected mod(s)
		vector<string> ars = archiveScanner->GetArchives(modArchive);
		if (ars.empty())
			throw content_error("Couldn't find any archives for mod '" + modName + "'");
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
			if (!hpiHandler->AddArchive(*i, false))
				throw content_error("Couldn't load archive '" + *i + "' for mod '" + modName + "'.");
	
		// always load springcontent.sdz
		hpiHandler->AddArchive("base/springcontent.sdz", false);
		alreadyLoaded = true;
	}
}

void CPreGame::GameDataRecieved(RawPacket* packet)
{
	gameData = new GameData(*packet);
	logOutput << "Using script " << gameData->GetScript() << "\n";
	CScriptHandler::SelectScript(gameData->GetScript());
	
	logOutput << "Using map " << gameData->GetMap() << "\n";
	stupidGlobalMapname = gameData->GetMap();
	
	if (net && net->GetDemoRecorder())
		net->GetDemoRecorder()->SetName(gameData->GetMap());
	LoadMap(gameData->GetMap());
	archiveScanner->CheckMap(gameData->GetMap(), gameData->GetMapChecksum());
	
	logOutput << "Using mod " << gameData->GetMod() << "\n";
	LoadMod(gameData->GetMod());
	modArchive = archiveScanner->ModNameToModArchive(gameData->GetMod());
	archiveScanner->CheckMod(modArchive, gameData->GetModChecksum());
	
	if (gameSetup)
	{
		gameSetup->scriptName = gameData->GetScript();
		gameSetup->mapName = gameData->GetMap();
		gameSetup->baseMod = gameData->GetMod();
	}
}
