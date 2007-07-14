#include "StdAfx.h"
#include "PreGame.h"
#include <map>
#include <set>
#include "UI/MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Game.h"
#include "StartScripts/ScriptHandler.h"
#include "Platform/ConfigHandler.h"
#include "UI/InfoConsole.h"
#include "NetProtocol.h"
#include "GameSetup.h"
#include "command.h"
#include "Team.h"
#include "Rendering/Textures/TAPalette.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "GameVersion.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include <SDL_timer.h>
#include <SDL_types.h>
#include <SDL_keysym.h>
#include "GameServer.h"
#include "FPUCheck.h"
#include "Platform/Clipboard.h"
#include "mmgr.h"
#include "LoadSaveHandler.h"

CPreGame* pregame=0;
std::string CPreGame::mapName;
std::string CPreGame::modName;
std::string CPreGame::modArchive;
std::string CPreGame::scriptName;

extern Uint8 *keys;
extern bool globalQuit;
std::string stupidGlobalMapname;

CPreGame::CPreGame(bool server, const string& demo, const std::string& save)
: showList(0),
  server(server),
  state(UNKNOWN),
  saveAddress(true),
  hasDemo(!demo.empty()),
  hasSave(!save.empty()),
  savefile(NULL)
{
	demoFile = gameSetup? gameSetup->demoName : demo;
	CommandDescription::Init();

	infoConsole = SAFE_NEW CInfoConsole;

	pregame = this; // prevent crashes if Select* is called from ctor
	net = SAFE_NEW CNetProtocol();

	//hpiHandler=SAFE_NEW CHpiHandler();

	activeController=this;

	inbufpos=0;
	inbuflength=0;

	if(!gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			for(int b=0;b<4;++b){
				gs->Team(a)->color[b]=palette.teamColor[a][b];
			}
		}
	}

	if(server){
		if(gameSetup){
			CScriptHandler::SelectScript(gameSetup->scriptName);
			SelectScript(gameSetup->scriptName);
			if (!gameSetup->saveName.empty()) {
				savefile = new CLoadSaveHandler();
				savefile->LoadGameStartInfo(savefile->FindSaveFile(gameSetup->saveName.c_str()));
			}
			SelectMap(gameSetup->mapname);
			SelectMod(gameSetup->baseMod);
			state = ALL_READY;
		} else if (hasSave) {
			savefile = new CLoadSaveHandler();
			savefile->LoadGameStartInfo(savefile->FindSaveFile(save.c_str()));
			CScriptHandler::SelectScript("Commanders");
			SelectScript("Commanders");
			SelectMap(savefile->mapName);
			SelectMod(savefile->modName);
			state = ALL_READY;
		} else {
			ShowScriptList();
			state = WAIT_ON_SCRIPT;
		}
	} else {
		if(gameSetup){
			server = false;
			PrintLoadMsg("Connecting to server");
			if(net->InitClient(gameSetup->hostip.c_str(),gameSetup->hostport,gameSetup->sourceport)==-1){
				handleerror(0,"Client couldn't connect","PreGame error",0);
				exit(-1);
			}
			CScriptHandler::SelectScript(gameSetup->scriptName);
			SelectScript(gameSetup->scriptName);
			SelectMap(gameSetup->mapname);
			SelectMod(gameSetup->baseMod);
			state = ALL_READY;
		} else {
			if (hasDemo) {
				/*
				We want to watch a demo local, so we dont know script, map and mod yet and we have to start a server which should send us the required data
				*/
				net->localDemoPlayback = true;
				server = false;
				state = WAIT_ON_SCRIPT;
				good_fpu_control_registers("before CGameServer creation");
				gameServer = SAFE_NEW CGameServer(8452, pregame->mapName, pregame->modArchive, pregame->scriptName, demoFile);
				good_fpu_control_registers("after CGameServer creation");
				if (gameSetup) {	// we read a gameSetup from the demofiles
					SelectMap(gameSetup->mapname);
					SelectMod(gameSetup->baseMod);
					CScriptHandler::SelectScript(gameSetup->scriptName);
					SelectScript(gameSetup->scriptName);
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
			case WAIT_ON_SCRIPT:
				PrintLoadMsg("Waiting on script", false);
				break;
			case WAIT_ON_MAP:
				PrintLoadMsg("Waiting on map", false);
				break;
			case WAIT_ON_MOD:
				PrintLoadMsg("Waiting on mod", false);
				break;
			case UNKNOWN:
			case WAIT_ON_ADDRESS:
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

		const float xScale = 0.03f;
		const float yScale = 0.04f;

		// draw the caret
		const int caretPos = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPos);
		const float caretWidth = font->CalcTextWidth(caretStr.c_str());
		char c = userInput[writingPos];
		if (c == 0) { c = ' '; }
		const float cw = xScale * font->CalcCharWidth(c);
		const float csx = xStart + (xScale * caretWidth);
		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + sin((float)SDL_GetTicks() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, yStart, csx + cw, yStart + yScale);
		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTranslatef(xStart, yStart, 0.0f);
		glScalef(xScale, yScale, 1.0f);
		font->glPrintRaw(tempstring.c_str());
		glLoadIdentity();
	}

	if(showList)
		showList->Draw();

	if(net->inInitialConnect){
		char text[400];
		sprintf(text,"Connecting to server %i",40-(int)(static_cast<float>(SDL_GetTicks())/1000.0f - net->connections[0]->lastSendTime));
		glColor4f(1,1,1,1);
		glTranslatef(0.5f-0.01f*strlen(text),0.48f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		font->glPrintRaw(text);
		glLoadIdentity();
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

		case WAIT_ON_ADDRESS:
			if (userWriting)
				break;

			if (saveAddress)
				configHandler.SetString("address",userInput);
			if(net->InitClient(userInput.c_str(),8452,0)==-1){
				logOutput.Print("Client couldn't connect");
				return false;
			}
			{
				state = WAIT_ON_SCRIPT;
				// fall trough
			}

		case WAIT_ON_SCRIPT:
			if (showList || !server)
				break;

			mapName = CScriptHandler::Instance().chosenScript->GetMapName();
			if (mapName == "")
				ShowMapList();
			else
				SelectMap(mapName);
			state = WAIT_ON_MAP;
			// fall through

		case WAIT_ON_MAP:
			if (showList || !server)
				break;

			pregame->modName = CScriptHandler::Instance().chosenScript->GetModName();
			if (pregame->modName == "")
				ShowModList();
			else
				SelectMod(pregame->modName);
			state = WAIT_ON_MOD;
			// fall through

		case WAIT_ON_MOD:
			if (showList || !server)
				break;

			state = ALL_READY;
			// fall through

		case ALL_READY: {
			ENTER_MIXED;

			// Map all required archives depending on selected mod(s)
			vector<string> ars = archiveScanner->GetArchives(pregame->modArchive);
			if (ars.empty())
				logOutput.Print("Warning: mod archive \"%s\" is missing?\n", pregame->modArchive.c_str());
			for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
				if (!hpiHandler->AddArchive(*i, false))
					logOutput.Print("Warning: Couldn't load archive '%s'.", i->c_str());

			// Determine if the map is inside an archive, and possibly map needed archives
			CFileHandler* f = SAFE_NEW CFileHandler("maps/" + pregame->mapName);
			if (!f->FileExists()) {
				vector<string> ars = archiveScanner->GetArchivesForMap(pregame->mapName);
				if (ars.empty())
					logOutput.Print("Warning: map archive \"%s\" is missing?\n", pregame->mapName.c_str());
				for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
					if (!hpiHandler->AddArchive(*i, false))
						logOutput.Print("Warning: Couldn't load archive '%s'.", i->c_str());
				}
			}
			delete f;

			// always load springcontent.sdz
			hpiHandler->AddArchive("base/springcontent.sdz", false);

			const int teamID = gs->players[gu->myPlayerNum]->team;
			const CTeam* team = gs->Team(teamID);
			LoadStartPicture(team->side);

			// create GameServer here where we can ensure to know which map and mod we are using
			if (server) {
				good_fpu_control_registers("before CGameServer creation");
				gameServer = SAFE_NEW CGameServer(gameSetup? gameSetup->hostport : 8452, pregame->mapName, pregame->modArchive, pregame->scriptName, demoFile);
				good_fpu_control_registers("after CGameServer creation");
			}

			game = SAFE_NEW CGame(server, pregame->mapName, pregame->modArchive, infoConsole);

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

	if(!server && state != WAIT_ON_ADDRESS){
		net->Update();
		UpdateClientNet();
	}

	return true;
}

void CPreGame::UpdateClientNet()
{
	int a;
	if((a=net->GetData(&inbuf[inbuflength],15000-inbuflength,gameSetup ? gameSetup->myPlayer : 0))==-1){
		globalQuit=true;
		return;
	}
	inbuflength+=a;

	while(inbufpos<inbuflength){
		switch (inbuf[inbufpos]){
		case NETMSG_SCRIPT:
			if (!gameSetup) {
				CScriptHandler::SelectScript((char*)(&inbuf[inbufpos+2]));
				SelectScript((char*)(&inbuf[inbufpos+2]));
			}
			if (mapName.empty()) state = WAIT_ON_MAP;
			else if (pregame->modName.empty()) state = WAIT_ON_MOD;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MAPNAME:
			if (!gameSetup)
				SelectMap((char*)(&inbuf[inbufpos+6]));
			archiveScanner->CheckMap(mapName, *(unsigned*)(&inbuf[inbufpos+2]));
			if (!CScriptHandler::Instance().chosenScript) state = WAIT_ON_SCRIPT;
			else if (pregame->modName.empty()) state = WAIT_ON_MOD;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MODNAME:
			if (!gameSetup)
				SelectMod((char*)(&inbuf[inbufpos+6]));
			archiveScanner->CheckMod(pregame->modArchive, *(unsigned*)(&inbuf[inbufpos+2]));
			if (!CScriptHandler::Instance().chosenScript) state = WAIT_ON_SCRIPT;
			else if (mapName.empty()) state = WAIT_ON_MAP;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];

		case NETMSG_MAPDRAW:
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_SYSTEMMSG:
		case NETMSG_CHAT:{
			//int player=inbuf[inbufpos+2];
			string s=(char*)(&inbuf[inbufpos+3]);
			logOutput.Print(s);
			inbufpos += inbuf[inbufpos+1];
			break;}

		case NETMSG_STARTPOS:{
			inbufpos += 15;
			break;}

		case NETMSG_SETPLAYERNUM:
			gu->myPlayerNum=inbuf[inbufpos+1];
			logOutput.Print("Became player %i",gu->myPlayerNum);
			inbufpos += 2;
			break;

		case NETMSG_PLAYERNAME:
			gs->players[inbuf[inbufpos+2]]->playerName=(char*)(&inbuf[inbufpos+3]);
			gs->players[inbuf[inbufpos+2]]->readyToStart=true;
			gs->players[inbuf[inbufpos+2]]->active=true;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_QUIT:
			net->connected=false;
			globalQuit=true;
			inbufpos += 1;
			return;

		case NETMSG_USER_SPEED:
		case NETMSG_INTERNAL_SPEED:
			inbufpos += 5;
			break;

		case NETMSG_HELLO:
		case NETMSG_SENDPLAYERSTAT:
			inbufpos += 1;
			break;

		default:
			char txt[200];
			sprintf(txt,"Unknown net msg in client %d",(int)inbuf[inbufpos]);
			handleerror(0,txt,"Network error in CPreGame",0);
			inbufpos++;
			break;
		}
	}
}

/** Called by the script-selecting CglList. */
void CPreGame::SelectScript(std::string s)
{
	delete pregame->showList;
	pregame->showList = 0;
	logOutput << "Using script " << s.c_str() << "\n";
	if (pregame->server)
		pregame->scriptName = s;
}

/** Create a CglList for selecting the script. */
void CPreGame::ShowScriptList()
{
	CglList* list = CScriptHandler::Instance().GenList(SelectScript);
	showList = list;
}

/** Called by the map-selecting CglList. */
void CPreGame::SelectMap(std::string s)
{
	if (s == "Random map") {
		s = pregame->showList->items[1 + gu->usRandInt() % (pregame->showList->items.size() - 1)];
	}
	stupidGlobalMapname = pregame->mapName = s;
	delete pregame->showList;
	pregame->showList = 0;
	logOutput << "Map: " << s.c_str() << "\n";
	if (net)
		// inform CNetProtocol about our map (for demo writing)
		net->SendMapName(archiveScanner->GetMapChecksum(pregame->mapName), pregame->mapName);
}

/** Create a CglList for selecting the map. */
void CPreGame::ShowMapList()
{
	CglList* list = SAFE_NEW CglList("Select map", SelectMap, 2);
	std::vector<std::string> found = filesystem.FindFiles("maps/","{*.sm3,*.smf}");
	std::vector<std::string> arFound = archiveScanner->GetMaps();
	if (found.begin() == found.end() && arFound.begin() == arFound.end()) {
		handleerror(0, "Couldn't find any map files", "PreGame error", 0);
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

/** Called by the mod-selecting CglList. */
void CPreGame::SelectMod(std::string s)
{
	if (s == "Random mod") {
		const int index = 1 + (gu->usRandInt() % (pregame->showList->items.size() - 1));
		const string& modName = pregame->showList->items[index];
		pregame->modArchive = archiveScanner->ModNameToModArchive(modName);
	} else {
		pregame->modArchive = archiveScanner->ModNameToModArchive(s);
	}
	delete pregame->showList;
	pregame->showList = 0;
	logOutput << "Mod: \"" << s.c_str() << "\" from " << pregame->modArchive.c_str() << "\n";
	pregame->modName = s;
}

/** Create a CglList for selecting the mod. */
void CPreGame::ShowModList()
{
	CglList* list = SAFE_NEW CglList("Select mod", SelectMod, 3);
	std::vector<CArchiveScanner::ModData> found = archiveScanner->GetPrimaryMods();
	if (found.empty()) {
		handleerror(0, "Couldn't find any mod files", "PreGame error", 0);
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
