#include "StdAfx.h"
#include "GameSetup.h"
#include "SunParser.h"
#include "Player.h"
#include "Team.h"
#include "myGL.h"
#include "glFont.h"
#include <algorithm>
#include <cctype>
#include "MouseHandler.h"
#include "CameraController.h"
#include "TAPalette.h"
#include "Net.h"
#include "FileHandler.h"
#include "StartPosSelecter.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"

CGameSetup* gameSetup=0;

using namespace std;
extern bool keys[256];

extern string stupidGlobalMapname;

CGameSetup::CGameSetup(void)
{
	file=0;
	readyTime=0;
	gameSetupText=0;
	startPosType=0;
	forceReady=false;
}

CGameSetup::~CGameSetup(void)
{
	delete file;
	delete[] gameSetupText;
}

bool CGameSetup::Init(std::string setupFile)
{
	if(setupFile.empty())
		return false;
	CFileHandler fh(setupFile);
	char* c=new char[fh.FileSize()];
	fh.Read(c,fh.FileSize());

	bool ret=Init(c,fh.FileSize());

	delete[] c;

	return ret;
}

bool CGameSetup::Init(char* buf, int size)
{
	gameSetupText=new char[size];
	memcpy(gameSetupText,buf,size);
	gameSetupTextLength=size;

	file=new CSunParser;
	file->LoadBuffer(buf,size);

	if(!file->SectionExist("GAME"))
		return false;

	mapname=file->SGetValueDef("","GAME\\mapname");
	baseMod=file->SGetValueDef("xta_se_060.sdz","GAME\\Gametype");
	file->GetDef(hostip,"0","GAME\\HostIP");
	file->GetDef(hostport,"0","GAME\\HostPort");
	file->GetDef(maxUnits,"500","GAME\\MaxUnits");
	file->GetDef(gs->gameMode,"0","GAME\\GameMode");
	file->GetDef(sourceport,"0","GAME\\SourcePort");

	// Determine if the map is inside an archive, and possibly map needed archives
	CFileHandler* f = new CFileHandler("maps/" + mapname);
	if (!f->FileExists()) {
		vector<string> ars = archiveScanner->GetArchivesForMap(mapname);
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			hpiHandler->AddArchive(*i, false);
		}
	}
	delete f;

	file->GetDef(myPlayer,"0","GAME\\MyPlayerNum");
	gu->myPlayerNum=myPlayer;
	file->GetDef(numPlayers,"2","GAME\\NumPlayers");
	file->GetDef(gs->activeTeams,"2","GAME\\NumTeams");
	file->GetDef(gs->activeAllyTeams,"2","GAME\\NumAllyTeams");

	file->GetDef(startPosType,"0","GAME\\StartPosType");
	if(startPosType==2){
		for(int a=0;a<gs->activeTeams;++a)
			readyTeams[a]=false;
		for(int a=0;a<gs->activeTeams;++a)
			teamStartNum[a]=a;
		new CStartPosSelecter();
	} else {
		for(int a=0;a<gs->activeTeams;++a)
			readyTeams[a]=true;
		if(startPosType==0){		//in order
			for(int a=0;a<gs->activeTeams;++a)
				teamStartNum[a]=a;
		} else {								//random order
			std::multimap<int,int> startNums;
			for(int a=0;a<gs->activeTeams;++a)
				startNums.insert(pair<int,int>(gu->usRandInt(),a));	//server syncs these later
			int b=0;
			for(std::multimap<int,int>::iterator si=startNums.begin();si!=startNums.end();++si){
				teamStartNum[si->second]=b;
				++b;
			}
		}
	}
	for(int a=0;a<numPlayers;++a){
		char section[50];
		sprintf(section,"GAME\\PLAYER%i\\",a);
		string s(section);

		gs->players[a]->team=atoi(file->SGetValueDef("0",s+"team").c_str());
		gs->players[a]->spectator=!!atoi(file->SGetValueDef("0",s+"spectator").c_str());
		gs->players[a]->playerName=file->SGetValueDef("0",s+"name");
	}
	gu->spectating=gs->players[myPlayer]->spectator;

	CSunParser p2;
	p2.LoadFile(string("maps/")+mapname.substr(0,mapname.find_last_of('.'))+".smd");

	for(int a=0;a<gs->activeTeams;++a){
		char section[50];
		sprintf(section,"GAME\\TEAM%i\\",a);
		string s(section);

		gs->teams[a]->colorNum=atoi(file->SGetValueDef("0",s+"color").c_str());
		for(int b=0;b<4;++b)
			gs->teams[a]->color[b]=palette.teamColor[gs->teams[a]->colorNum][b];

		gs->teams[a]->handicap=atof(file->SGetValueDef("0",s+"handicap").c_str())/100+1;
		gs->teams[a]->leader=atoi(file->SGetValueDef("0",s+"teamleader").c_str());
		gs->teams[a]->side=file->SGetValueDef("arm",s+"side").c_str();
		std::transform(gs->teams[a]->side.begin(), gs->teams[a]->side.end(), gs->teams[a]->side.begin(), (int (*)(int))std::tolower);
		gs->team2allyteam[a]=atoi(file->SGetValueDef("0",s+"allyteam").c_str());
		aiDlls[a]=file->SGetValueDef("",s+"aidll");

		float x,z;
		char teamName[50];
		sprintf(teamName,"TEAM%i",teamStartNum[a]);
		p2.GetDef(x,"1000",string("MAP\\")+teamName+"\\StartPosX");
		p2.GetDef(z,"1000",string("MAP\\")+teamName+"\\StartPosZ");
		gs->teams[a]->startPos=float3(x,100,z);

		if(startPosType==2)
			gs->teams[a]->startPos.y=-500;	//show that we havent selected start pos yet
	}
	gu->myTeam=gs->players[myPlayer]->team;
	gu->myAllyTeam=gs->team2allyteam[gu->myTeam];

	for(int a=0;a<gs->activeAllyTeams;++a){
		char section[50];
		sprintf(section,"GAME\\ALLYTEAM%i\\",a);
		string s(section);

		int numAllies=atoi(file->SGetValueDef("0",s+"NumAllies").c_str());
		for(int b=0;b<numAllies;++b){
			char key[100];
			sprintf(key,"GAME\\ALLYTEAM%i\\Ally%i",a,b);
			int other=atoi(file->SGetValueDef("0",key).c_str());
			gs->allies[a][other]=true;
		}
	}

	int metal,energy;
	file->GetDef(metal,"1000","GAME\\StartMetal");
	file->GetDef(energy,"1000","GAME\\StartEnergy");
	for(int a=0;a<gs->activeTeams;++a){
		gs->teams[a]->metal=metal;
		gs->teams[a]->metalStorage=metal;

		gs->teams[a]->energy=energy;
		gs->teams[a]->energyStorage=energy;
	}

	// Read the unit restrictions
	int numRestrictions;
	file->GetDef(numRestrictions, "0", "GAME\\NumRestrictions");

	for (int i = 0; i < numRestrictions; ++i) {
		char key[100];
		sprintf(key, "GAME\\RESTRICT\\Unit%d", i);
		string resName = file->SGetValueDef("", key);
		sprintf(key, "GAME\\RESTRICT\\Limit%d", i);
		int resLimit;
		file->GetDef(resLimit, "0", key);

		restrictedUnits[resName] = resLimit;
	}

	return true;
}

bool CGameSetup::Draw(void)
{

	glColor4f(1,1,1,1);
	glPushMatrix();
	glTranslatef(0.3f,0.7f,0.0f);
	glScalef(0.03f,0.04f,0.1f);
	if(!serverNet && net->inInitialConnect){
#ifndef NO_NET
		font->glPrint("Connecting to server %i",40-(int)(net->curTime-net->connections[0].lastReceiveTime));
#endif
		return false;
	}else if(readyTime>0)
		font->glPrint("Starting in %i",3-(int)readyTime);
	else if(!readyTeams[gu->myTeam])
		font->glPrint("Choose start pos");
	else if(gu->myPlayerNum==0){
		glTranslatef(-8,0.0f,0.0f);
		font->glPrint("Waiting for players, Ctrl+Return to force start");
	}else
		font->glPrint("Waiting for players");

	glPopMatrix();
	glPushMatrix();
	bool allReady=true;
	for(int a=0;a<numPlayers;a++){
		if(!gs->players[a]->readyToStart){
			glColor4f(1,0.2,0.2,1);
			allReady=false;
		} else if(!readyTeams[gs->players[a]->team]){
			glColor4f(0.8,0.8,0.2,1);
			allReady=false;
		} else {
			glColor4f(0.2,1,0.2,1);
		}
		glPushMatrix();
		glTranslatef(0.3f,0.6f-a*0.05,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		font->glPrint("%s",gs->players[a]->playerName.c_str());
		glPopMatrix();
	}
	if(gu->myPlayerNum==0 && keys[VK_RETURN] && keys[VK_CONTROL]){

		forceReady=true;
	}
	if(forceReady)
		allReady=true;
	if(allReady){
		if(readyTime==0 && !net->playbackDemo){
			mouse->currentCamController=mouse->camControllers[1];
			mouse->currentCamControllerNum=1;
			mouse->currentCamController->SetPos(gs->teams[gu->myTeam]->startPos);
			mouse->inStateTransit=true;
			mouse->transitSpeed=1;
		}
		readyTime+=gu->lastFrameTime;
	} else {
		readyTime=0;
	}
	glPopMatrix();
	return readyTime>3;
}

