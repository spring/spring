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
#include "Net.h"
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
#include "mmgr.h"

CPreGame* pregame=0;
extern Uint8 *keys;
extern bool globalQuit;
std::string stupidGlobalMapname;

CPreGame::CPreGame(bool server, const string& demo):
		showList(0),
		server(server),
		state(UNKNOWN),
		saveAddress(true)
{
	CommandDescription::Init();
	infoConsole = new CInfoConsole;

	pregame = this; // prevent crashes if Select* is called from ctor
	net = new CNet;

	if (server) {
		assert(good_fpu_control_registers());
		gameServer = new CGameServer;
		assert(good_fpu_control_registers());
	}

	//hpiHandler=new CHpiHandler();

	activeController=this;

	inbufpos=0;
	inbuflength=0;

	if(!gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			for(int b=0;b<4;++b)
				gs->Team(a)->color[b]=palette.teamColor[a][b];
		}
	}

	if(server){
		if(gameSetup){
			CScriptHandler::SelectScript("Commanders");
			SelectMap(gameSetup->mapname);
			SelectMod(gameSetup->baseMod);
			state = ALL_READY;
		} else {
			ShowScriptList();
			state = WAIT_ON_SCRIPT;
		}
	} else {
		if(gameSetup){
			PrintLoadMsg("Connecting to server");
			if(net->InitClient(gameSetup->hostip.c_str(),gameSetup->hostport,gameSetup->sourceport)==-1){
				handleerror(0,"Client couldn't connect","PreGame error",0);
				exit(-1);
			}
			CScriptHandler::SelectScript("Commanders");
			SelectMap(gameSetup->mapname);
			SelectMod(gameSetup->baseMod);
			state = ALL_READY;
		} else {
			if (demo != "") {
				userInput = demo;
				state = WAIT_ON_ADDRESS;
				userWriting = false;
				saveAddress = false;
			}
			else {
				userInput=configHandler.GetString("address","");
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
		showList->KeyPressed(k);
		return 0;
	}

	if (userWriting){
		keys[k] = true;
#ifndef NO_CLIPBOARD
		if (k == SDLK_v && keys[SDLK_LCTRL]){
			OpenClipboard(0);
			void* p;
			if((p=GetClipboardData(CF_TEXT))!=0){
				userInput+=(char*)p;
			}
			CloseClipboard();
			return 0;
		}
#endif
		if(k == SDLK_BACKSPACE){ //backspace
			if(userInput.size()!=0)
				userInput.erase(userInput.size()-1,1);
			return 0;
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
			default:
				PrintLoadMsg("", false); // just clear screen and set up matrices etc.
				break;
		}
	} else
		PrintLoadMsg("", false); // just clear screen and set up matrices etc.

	infoConsole->Draw();

	if(userWriting){
		glColor4f(1,1,1,1);
		glTranslatef(0.1f,0.75f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		std::string tempstring=userPrompt;
		tempstring+=userInput;
		font->glPrintRaw(tempstring.c_str());
		glLoadIdentity();
	}	

	if(showList)
		showList->Draw();

	if(net->inInitialConnect){
		char text[400];
		sprintf(text,"Connecting to server %i",40-(int)(net->curTime-net->connections[0].lastSendTime));
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
	assert(good_fpu_control_registers("CPreGame::Update"));

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

			// State is never WAIT_ON_ADDRESS if gameSetup was true in our constructor,
			// so if it's true here, it means net->InitClient() just loaded a demo
			// with gameSetup.
			// If so, don't wait indefinitely on a script/map/mod name, but load
			// everything from gameSetup and switch to ALL_READY state.
			if(gameSetup) {
				CScriptHandler::SelectScript("Commanders");
				SelectMap(gameSetup->mapname);
				SelectMod(gameSetup->baseMod);
				state = ALL_READY;
				break;
			} else {
				state = WAIT_ON_SCRIPT;
				// fall trough
			}

		case WAIT_ON_SCRIPT:
			if (showList || !server)
				break;

			mapName = CScriptHandler::Instance().chosenScript->GetMapName();
			if (mapName == "")
				ShowMapList();
			state = WAIT_ON_MAP;
			// fall through

		case WAIT_ON_MAP:
			if (showList || !server)
				break;

			modName = CScriptHandler::Instance().chosenScript->GetModName();
			if (modName == "")
				ShowModList();
			state = WAIT_ON_MOD;
			// fall through

		case WAIT_ON_MOD:
			if (showList || !server)
				break;

			state = ALL_READY;
			// fall through

		case ALL_READY:
			ENTER_MIXED;

			// Map all required archives depending on selected mod(s)
			vector<string> ars = archiveScanner->GetArchives(modName);
			if (ars.empty())
				logOutput.Print("Warning: mod archive \"%s\" is missing?\n", modName.c_str());
			for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
				hpiHandler->AddArchive(*i, false);

			// Determine if the map is inside an archive, and possibly map needed archives
			CFileHandler* f = new CFileHandler("maps/" + mapName);
			if (!f->FileExists()) {
				vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
				if (ars.empty())
					logOutput.Print("Warning: map archive \"%s\" is missing?\n", mapName.c_str());
				for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
					hpiHandler->AddArchive(*i, false);
				}
			}
			delete f;

			LoadStartPicture();

			game=new CGame(server,mapName,modName,infoConsole);
			infoConsole = 0;

			ENTER_UNSYNCED;
			game->Update();
			pregame=0;
			delete this;
			return true;
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
	if((a=net->GetData(&inbuf[inbuflength],15000-inbuflength,0))==-1){
		globalQuit=true;
		return;
	}
	inbuflength+=a;

	while(inbufpos<inbuflength){
		switch (inbuf[inbufpos]){
		case NETMSG_HELLO:
			inbufpos += 1;
			break;

		case NETMSG_SCRIPT:
			CScriptHandler::SelectScript((char*)(&inbuf[inbufpos+2]));
			if (mapName.empty()) state = WAIT_ON_MAP;
			else if (modName.empty()) state = WAIT_ON_MOD;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MAPNAME:
			SelectMap((char*)(&inbuf[inbufpos+6]));
			if (GetMapChecksum() != *(unsigned*)(&inbuf[inbufpos+2])) {
				char buf[256];
				sprintf(buf, "Local map archive(s) are not binary equal to host map archive(s).\n"
						"Make sure you installed all map dependencies & consider redownloading the map."
						"\n\nLocal checksum = %u\nRemote checksum = %u", GetMapChecksum(), *(unsigned*)(&inbuf[inbufpos+2]));
				throw content_error(buf);
			}
			if (!CScriptHandler::Instance().chosenScript) state = WAIT_ON_SCRIPT;
			else if (modName.empty()) state = WAIT_ON_MOD;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MODNAME:
			SelectMod((char*)(&inbuf[inbufpos+6]));
			if (GetModChecksum() != *(unsigned*)(&inbuf[inbufpos+2])) {
				char buf[256];
				sprintf(buf, "Local mod archive(s) are not binary equal to host mod archive(s).\n"
						"Make sure you installed all mod dependencies & consider redownloading the mod."
						"\n\nLocal checksum = %u\nRemote checksum = %u", GetModChecksum(), *(unsigned*)(&inbuf[inbufpos+2]));
				throw content_error(buf);
			}
			if (!CScriptHandler::Instance().chosenScript) state = WAIT_ON_SCRIPT;
			else if (mapName.empty()) state = WAIT_ON_MAP;
			else state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];

		case NETMSG_MAPDRAW:
			inbufpos += inbuf[inbufpos+1];	
			break;

		case NETMSG_SYSTEMMSG:
		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
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
			return;

		case NETMSG_USER_SPEED:
		case NETMSG_INTERNAL_SPEED:
			inbufpos += 5;
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
		serverNet->SetScript(s);
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
	if (pregame->server)
		serverNet->SetMap(pregame->GetMapChecksum(), pregame->mapName);
}

/** Create a CglList for selecting the map. */
void CPreGame::ShowMapList()
{
	CglList* list = new CglList("Select map", SelectMap, 2);
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
		s = pregame->showList->items[1 + gu->usRandInt() % (pregame->showList->items.size() - 1)];
	}
	// Convert mod name to mod archive
	std::vector<CArchiveScanner::ModData> found = archiveScanner->GetPrimaryMods();
	for (std::vector<CArchiveScanner::ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		if (it->name == s) {
			s = it->dependencies.front();
			break;
		}
	}
	pregame->modName = s;
	delete pregame->showList;
	pregame->showList = 0;
	logOutput << "Mod: " << s.c_str() << "\n";
	if (pregame->server)
		serverNet->SetMod(pregame->GetModChecksum(), pregame->modName);
}

/** Create a CglList for selecting the mod. */
void CPreGame::ShowModList()
{
	CglList* list = new CglList("Select mod", SelectMod, 3);
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

/** Determine if the map is inside an archive, if so get checksum of all required archives. */
unsigned CPreGame::GetMapChecksum() const
{
	CFileHandler f("maps/" + mapName);
	if (!f.FileExists())
		return archiveScanner->GetChecksumForMap(mapName);
	return 0;
}

/** Get checksum of all required archives depending on selected mod(s). */
unsigned CPreGame::GetModChecksum() const
{
	return archiveScanner->GetChecksum(modName);
}
