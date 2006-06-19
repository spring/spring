#include "StdAfx.h"
#include "PreGame.h"
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
#include "Team.h"
#include "Rendering/Textures/TAPalette.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "GameVersion.h"
#include "Platform/filefunctions.h"
#include "Platform/errorhandler.h"
#include <SDL_types.h>
#include <SDL_keysym.h>
#include "GameServer.h"
#include "mmgr.h"

CPreGame* pregame=0;
extern Uint8 *keys;
extern bool globalQuit;
std::string stupidGlobalModname;
std::string stupidGlobalMapname;

CPreGame::CPreGame(bool server, const string& demo):
		showList(0),
		server(server),
		state(UNKNOWN),
		saveAddress(true)
{
	pregame = this; // prevent crashes if Select* is called from ctor
	info = new CInfoConsole;
	net = new CNet;
	if (server)
		gameServer = new CGameServer;

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
			info->AddLine("User exited");
			globalQuit=true;
		} else
			info->AddLine("Use shift-esc to quit");
	}
	if(showList){					//are we currently showing a list?
		if(k == SDLK_UP)
			showList->UpOne();
		if(k == SDLK_DOWN)
			showList->DownOne();
		if(k == SDLK_RETURN)
			showList->Select();
		showList->KeyPress(k);
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

	info->Draw();

	if(userWriting){
		glColor4f(1,1,1,1);
		glTranslatef(0.1f,0.75f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		std::string tempstring=userPrompt;
		tempstring+=userInput;
		font->glPrint("%s",tempstring.c_str());
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
		font->glPrint("%s",text);
		glLoadIdentity();
	}
	return true;
}


bool CPreGame::Update()
{
	switch (state) {

		case UNKNOWN:
			info->AddLine("Internal error in CPreGame");
			return false;

		case WAIT_ON_ADDRESS:
			if (userWriting)
				break;

			if (saveAddress)
				configHandler.SetString("address",userInput);
			if(net->InitClient(userInput.c_str(),8452,0)==-1){
				info->AddLine("Client couldn't connect");
				return false;
			}
			state = WAIT_ON_SCRIPT;
			// fall trough

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
				fprintf(stderr, "Warning: mod archive \"%s\" is missing?\n", modName.c_str());
			for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
				hpiHandler->AddArchive(*i, false);

			// Determine if the map is inside an archive, and possibly map needed archives
			CFileHandler* f = new CFileHandler("maps/" + mapName);
			if (!f->FileExists()) {
				vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
				for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
					hpiHandler->AddArchive(*i, false);
				}
			}
			delete f;

			LoadStartPicture();

			game=new CGame(server,mapName);
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
			assert(state == WAIT_ON_SCRIPT);
			state = WAIT_ON_MAP;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MAPNAME:
			SelectMap((char*)(&inbuf[inbufpos+6]));
			if (GetMapChecksum() != *(unsigned*)(&inbuf[inbufpos+2])) {
				// shit happens
				throw content_error("Local map archive(s) are not binary equal to host map archive(s).\n"
						"Consider redownloading the map.");
			}
			assert(state == WAIT_ON_MAP);
			state = WAIT_ON_MOD;
			inbufpos += inbuf[inbufpos+1];
			break;

		case NETMSG_MODNAME:
			SelectMod((char*)(&inbuf[inbufpos+6]));
			if (GetModChecksum() != *(unsigned*)(&inbuf[inbufpos+2])) {
				// bad stuff too
				throw content_error("Local mod archive(s) are not binary equal to host mod archive(s).\n"
						"Consider redownloading the mod.");
			}
			assert(state == WAIT_ON_MOD);
			state = ALL_READY;
			inbufpos += inbuf[inbufpos+1];

		case NETMSG_MAPDRAW:
			inbufpos += inbuf[inbufpos+1];	
			break;

		case NETMSG_SYSTEMMSG:
		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
			string s=(char*)(&inbuf[inbufpos+3]);
			info->AddLine(s);
			inbufpos += inbuf[inbufpos+1];	
			break;}

		case NETMSG_STARTPOS:{
			inbufpos += 15;
			break;}

		case NETMSG_SETPLAYERNUM:
			gu->myPlayerNum=inbuf[inbufpos+1];
			info->AddLine("Became player %i",gu->myPlayerNum);
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
	(*info) << "Using script " << s.c_str() << "\n";
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
	(*info) << "Map: " << s.c_str() << "\n";
	if (pregame->server)
		serverNet->SetMap(pregame->GetMapChecksum(), pregame->mapName);
}

/** Create a CglList for selecting the map. */
void CPreGame::ShowMapList()
{
	CglList* list = new CglList("Select map", SelectMap, 2);
	fs::path fn("maps/");
	std::vector<fs::path> found = find_files(fn,"{*.sm3,*.smf}");
	std::vector<std::string> arFound = archiveScanner->GetMaps();
	if (found.begin() == found.end() && arFound.begin() == arFound.end()) {
		handleerror(0, "Couldn't find any map files", "PreGame error", 0);
		return;
	}
	list->AddItem("Random map", "Random map");
	for (std::vector<fs::path>::iterator it = found.begin(); it != found.end(); it++)
		list->AddItem(it->leaf().c_str(),it->leaf().c_str());
	for (std::vector<std::string>::iterator it = arFound.begin(); it != arFound.end(); it++)
		list->AddItem((*it).c_str(), (*it).c_str());
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
	stupidGlobalModname = pregame->modName = s;
	delete pregame->showList;
	pregame->showList = 0;
	(*info) << "Mod: " << s.c_str() << "\n";
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
	list->AddItem("Random mod", "Random mod");
	for (std::vector<CArchiveScanner::ModData>::iterator it = found.begin(); it != found.end(); ++it)
		list->AddItem(it->name.c_str(), it->description.c_str());
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
