#include "StdAfx.h"
#include "PreGame.h"
#include "MouseHandler.h"
#include "myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include "glFont.h"
#include "glList.h"
#include "Game.h"
#include "ScriptHandler.h"
#include "RegHandler.h"
#include "InfoConsole.h"
#include "Net.h"
#include "GameSetup.h"
#include "Team.h"
#include "TAPalette.h"
#include "VFSHandler.h"
#include "FileHandler.h"
#include "ArchiveScanner.h"
//#include "mmgr.h"
#include "filefunctions.h"

CPreGame* pregame=0;
extern bool keys[256];
extern bool globalQuit;
string stupidGlobalModName;

CPreGame::CPreGame(bool server, const string& demo)
: server(server),
	waitOnAddress(false),
	waitOnScript(false),
	waitOnMap(false),
	saveAddress(true)
{
	showList=0;
	mapName="";

	info=new CInfoConsole();
	net=new CNet();

	//hpiHandler=new CHpiHandler();

	activeController=this;
	allReady=false;

	inbufpos=0;
	inbuflength=0;

	if(!gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			gs->teams[a]->colorNum=a;
			for(int b=0;b<4;++b)
				gs->teams[a]->color[b]=palette.teamColor[a][b];
		}
	}

	if(server){
		if(gameSetup){
			CScriptHandler::SelectScript("Commanders");
			mapName=gameSetup->mapname;
			allReady=true;
			return;
		} else {
			showList=CScriptHandler::Instance()->list;
			waitOnScript=true;
		}
	} else {
		if(gameSetup){
			PrintLoadMsg("Connecting to server");
			if(net->InitClient(gameSetup->hostip.c_str(),gameSetup->hostport,gameSetup->sourceport)==-1){
				MessageBox(0,"Client couldnt connect","PreGame error",0);
				exit(-1);
			}
			CScriptHandler::SelectScript("Commanders");
			mapName=gameSetup->mapname;
			allReady=true;
		} else {
			if (demo != "") {
				userInput = demo;
				waitOnAddress = true;
				userWriting = false;
				saveAddress = false;
			}
			else {
				userInput=regHandler.GetString("address","");
				userPrompt="Enter server address: ";
				waitOnAddress=true;
				userWriting=true;
			}
		}
	}
}

CPreGame::~CPreGame(void)
{
}

int CPreGame::KeyPressed(unsigned char k,bool isRepeat)
{
	if (k==27){
		if(keys[VK_SHIFT]){
			info->AddLine("User exited");
			globalQuit=true;
		} else
			info->AddLine("Use shift-esc to quit");
	}
	if(showList){					//are we currently showing a list?
		if(k==VK_UP)
			showList->UpOne();
		if(k==VK_DOWN)
			showList->DownOne();
		if(k==VK_RETURN){
			showList->Select();
			showList=0;
		}
		return 0;
	}

	if (userWriting){
		keys[k] = TRUE;
#ifndef NO_CLIPBOARD
		if ((k=='V') && keys[VK_CONTROL]){
			OpenClipboard(0);
			void* p;
			if((p=GetClipboardData(CF_TEXT))!=0){
				userInput+=(char*)p;
			}
			CloseClipboard();
			return 0;
		}
#endif
		if(k==8){ //backspace
			if(userInput.size()!=0)
				userInput.erase(userInput.size()-1,1);
			return 0;
		}
		if(k==VK_RETURN){
			userWriting=false;
			return 0;
		}
		return 0;
	}

	return 0;
}

bool CPreGame::Draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();			
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLoadIdentity();

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


bool CPreGame::Update(void)
{
	if(waitOnAddress && !userWriting){		//användaren har skrivit klart addressen
		waitOnAddress=false;
		if (saveAddress)
			regHandler.SetString("address",userInput);
		if(net->InitClient(userInput.c_str(),8452,0)==-1){
			info->AddLine("Client couldnt connect");
			return false;
		}
		userWriting=false;
	}

	if(!server && !waitOnAddress){
		net->Update();
		UpdateClientNet();
	}

	if(waitOnScript && !showList){
		waitOnScript=false;

		mapName=CScriptHandler::Instance()->chosenScript->GetMapName();

		if(mapName==""){
			ShowMapList();
			waitOnMap=true;
		} else {
			allReady=true;
		}
	}

	if(allReady){
		ENTER_MIXED;

		// Map all required archives depending on selected mod(s)
		stupidGlobalModName = "xta_se_v061.sdz";
		if (gameSetup)
			stupidGlobalModName = gameSetup->baseMod;
		vector<string> ars = archiveScanner->GetArchives(stupidGlobalModName);
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			hpiHandler->AddArchive(*i, false);
		}

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
	return true;
}

void CPreGame::UpdateClientNet(void)
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
			inbufpos+=1;
			break;

		case NETMSG_MAPNAME:
			mapId=*(int*)(&inbuf[inbufpos+6]);
			mapName=(char*)(&inbuf[inbufpos+6]);
			info->AddLine("Got mapname %s",&inbuf[inbufpos+6]);
			allReady=true;
			inbufpos+=inbuf[inbufpos+1];
			if(inbufpos!=inbuflength){
				net->connections[0].readyLength=inbuflength-inbufpos;
				memcpy(net->connections[0].readyData,&inbuf[inbufpos],inbuflength-inbufpos);
			}
			break;

		case NETMSG_MAPDRAW:
			inbufpos+=inbuf[inbufpos+1];	
			break;

		case NETMSG_SYSTEMMSG:
		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
			string s=(char*)(&inbuf[inbufpos+3]);
			info->AddLine(s);
			inbufpos+=inbuf[inbufpos+1];	
			break;}

		case NETMSG_STARTPOS:{
			inbufpos+=15;
			break;}

		case NETMSG_SCRIPT:
			CScriptHandler::SelectScript((char*)(&inbuf[inbufpos+2]));
			info->AddLine("Using script %s",(char*)(&inbuf[inbufpos+2]));
			inbufpos+=inbuf[inbufpos+1];				
			break;

		case NETMSG_SETPLAYERNUM:
			gu->myPlayerNum=inbuf[inbufpos+1];
			info->AddLine("Became player %i",gu->myPlayerNum);
			inbufpos+=2;
			break;

		case NETMSG_PLAYERNAME:
			gs->players[inbuf[inbufpos+2]]->playerName=(char*)(&inbuf[inbufpos+3]);
			gs->players[inbuf[inbufpos+2]]->readyToStart=true;
			gs->players[inbuf[inbufpos+2]]->active=true;
			inbufpos+=inbuf[inbufpos+1];				
			break;

		case NETMSG_QUIT:
			net->connected=false;
			globalQuit=true;
			return;

		default:
			char txt[200];
			sprintf(txt,"Unknown net msg in client %d",(int)inbuf[inbufpos]);
			MessageBox(0,txt,"Network error in CPreGame",0);
			inbufpos++;
			break;
		}
	}
}

void CPreGame::SelectMap(std::string s)
{
	pregame->mapName=s;
	delete pregame->showList;
	pregame->showList=0;
	pregame->waitOnMap=false;
	pregame->allReady=true;
}

void CPreGame::ShowMapList(void)
{
	CglList* list=new CglList("Select map",SelectMap);
	fs::path fn("maps/");
	std::vector<fs::path> found = find_files(fn,"*.smf");
	std::vector<std::string> arFound = archiveScanner->GetMaps();
	if (found.begin() == found.end() && arFound.begin() == arFound.end()
			) {
		MessageBox(0,"Couldnt find any map files","PreGame error",0);
		return;
	}
	for (std::vector<fs::path>::iterator it = found.begin(); it != found.end(); it++)
		list->AddItem(it->leaf().c_str(),it->leaf().c_str());
	for (std::vector<std::string>::iterator it = arFound.begin(); it != arFound.end(); it++)
		list->AddItem((*it).c_str(), (*it).c_str());

	showList=list;
}
