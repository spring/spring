#include "StdAfx.h"
#include "GameSetup.h"
#include "TdfParser.h"
#include "Player.h"
#include "Team.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include <algorithm>
#include <cctype>
#include "UI/MouseHandler.h"
#include "CameraController.h"
#include "Rendering/Textures/TAPalette.h"
#include "Net.h"
#include "FileSystem/FileHandler.h"
#include "UI/StartPosSelecter.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "GameVersion.h"
#include "SDL_types.h"
#include "SDL_keysym.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"

CGameSetup* gameSetup=0;

using namespace std;
extern Uint8 *keys;

extern string stupidGlobalMapname;

CGameSetup::CGameSetup(void)
{
	readyTime=0;
	gameSetupText=0;
	startPosType=0;
	numDemoPlayers=0;
	hostDemo=false;
	forceReady=false;
}

CGameSetup::~CGameSetup(void)
{
	delete[] gameSetupText;
}

bool CGameSetup::Init(std::string setupFile)
{
	if(setupFile.empty())
		return false;
	CFileHandler fh(setupFile);
	if (!fh.FileExists())
		return false;
	char* c=new char[fh.FileSize()];
	fh.Read(c,fh.FileSize());

	bool ret=Init(c,fh.FileSize());

	delete[] c;

	return ret;
}

bool CGameSetup::Init(char* buf, int size)
{
	for(int a=0;a<MAX_PLAYERS;a++){
		gs->players[a]->team=0;					//needed in case one tries to spec a game with only one team
	}

	gameSetupText=new char[size];
	memcpy(gameSetupText,buf,size);
	gameSetupTextLength=size;

	file.LoadBuffer(buf,size);

	if(!file.SectionExist("GAME"))
		return false;

	mapname=file.SGetValueDef("","GAME\\mapname");
	baseMod=file.SGetValueDef(MOD_FILE,"GAME\\Gametype");
	file.GetDef(hostip,"0","GAME\\HostIP");
	file.GetDef(hostport,"0","GAME\\HostPort");
	file.GetDef(maxUnits,"500","GAME\\MaxUnits");
	file.GetDef(gs->gameMode,"0","GAME\\GameMode");
	file.GetDef(sourceport,"0","GAME\\SourcePort");
	file.GetDef(limitDgun,"0","GAME\\LimitDgun");
	file.GetDef(diminishingMMs,"0","GAME\\DiminishingMMs");
	file.GetDef(disableMapDamage,"0","GAME\\DisableMapDamage");
	demoName=file.SGetValueDef("","GAME\\Demofile");
	if(!demoName.empty())
		hostDemo=true;
	file.GetDef(ghostedBuildings,"1","GAME\\GhostedBuildings");

	file.GetDef(maxSpeed, "3", "GAME\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\MinSpeed");

	// Determine if the map is inside an archive, and possibly map needed archives
	CFileHandler* f = new CFileHandler("maps/" + mapname);
	if (!f->FileExists()) {
		vector<string> ars = archiveScanner->GetArchivesForMap(mapname);
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			hpiHandler->AddArchive(*i, false);
		}
	}
	delete f;

	file.GetDef(myPlayer,"0","GAME\\MyPlayerNum");
	gu->myPlayerNum=myPlayer;
	file.GetDef(numPlayers,"2","GAME\\NumPlayers");
	file.GetDef(gs->activeTeams,"2","GAME\\NumTeams");
	file.GetDef(gs->activeAllyTeams,"2","GAME\\NumAllyTeams");

	file.GetDef(startPosType,"0","GAME\\StartPosType");
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

		gs->players[a]->team=atoi(file.SGetValueDef("0",s+"team").c_str());
		gs->players[a]->spectator=!!atoi(file.SGetValueDef("0",s+"spectator").c_str());
		gs->players[a]->playerName=file.SGetValueDef("0",s+"name");

		int fromDemo;
		file.GetDef(fromDemo,"0",s+"IsFromDemo");
		if(fromDemo)
			numDemoPlayers++;
	}
	gu->spectating=gs->players[myPlayer]->spectator;

	TdfParser p2;
	CReadMap::OpenTDF (mapname, p2);

	for(int a=0;a<gs->activeTeams;++a){
		char section[50];
		sprintf(section,"GAME\\TEAM%i\\",a);
		string s(section);

		int colorNum=atoi(file.SGetValueDef("0",s+"color").c_str());
		colorNum%=palette.NumTeamColors();
		float3 defaultCol(palette.teamColor[colorNum][0]/255.0,palette.teamColor[colorNum][1]/255.0,palette.teamColor[colorNum][2]/255.0);
		float3 color=file.GetFloat3(defaultCol,s+"rgbcolor");
		for(int b=0;b<3;++b)
			gs->Team(a)->color[b]=color[b]*255;

 		gs->Team(a)->handicap=atof(file.SGetValueDef("0",s+"handicap").c_str())/100+1;
 		gs->Team(a)->leader=atoi(file.SGetValueDef("0",s+"teamleader").c_str());
 		gs->Team(a)->side=file.SGetValueDef("arm",s+"side").c_str();
 		std::transform(gs->Team(a)->side.begin(), gs->Team(a)->side.end(), gs->Team(a)->side.begin(), (int (*)(int))std::tolower);
 		gs->SetAllyTeam(a, atoi(file.SGetValueDef("0",s+"allyteam").c_str()));

		if (demoName.empty())
			aiDlls[a]=file.SGetValueDef("",s+"aidll");
		else
			aiDlls[a]="";

		float x,z;
		char teamName[50];
		sprintf(teamName,"TEAM%i",teamStartNum[a]);
		p2.GetDef(x,"1000",string("MAP\\")+teamName+"\\StartPosX");
		p2.GetDef(z,"1000",string("MAP\\")+teamName+"\\StartPosZ");
		gs->Team(a)->startPos=float3(x,100,z);

		if(startPosType==2)
			gs->Team(a)->startPos.y=-500;	//show that we havent selected start pos yet
	}
	gu->myTeam=gs->players[myPlayer]->team;
	gu->myAllyTeam=gs->AllyTeam(gu->myTeam);

	for(int a=0;a<gs->activeAllyTeams;++a){
		char section[50];
		sprintf(section,"GAME\\ALLYTEAM%i\\",a);
		string s(section);

		startRectTop[a]=atof(file.SGetValueDef("0",s+"StartRectTop").c_str());
		startRectBottom[a]=atof(file.SGetValueDef("1",s+"StartRectBottom").c_str());
		startRectLeft[a]=atof(file.SGetValueDef("0",s+"StartRectLeft").c_str());
		startRectRight[a]=atof(file.SGetValueDef("1",s+"StartRectRight").c_str());

		int numAllies=atoi(file.SGetValueDef("0",s+"NumAllies").c_str());
		for(int b=0;b<numAllies;++b){
			char key[100];
			sprintf(key,"GAME\\ALLYTEAM%i\\Ally%i",a,b);
			int other=atoi(file.SGetValueDef("0",key).c_str());
			gs->SetAlly(a,other, true);
		}
	}
	if(startPosType==2){
		for(int a=0;a<gs->activeTeams;++a)
			gs->Team(a)->startPos=float3(startRectLeft[gs->AllyTeam(a)]*gs->mapx*8,-500,startRectTop[gs->AllyTeam(a)]*gs->mapy*8);
	}

	int metal,energy;
	file.GetDef(metal,"1000","GAME\\StartMetal");
	file.GetDef(energy,"1000","GAME\\StartEnergy");
	for(int a=0;a<gs->activeTeams;++a){
		gs->Team(a)->metal=metal;
		gs->Team(a)->metalIncome=metal;	//for the endgame statistics
		gs->Team(a)->metalStorage=metal;

		gs->Team(a)->energy=energy;
		gs->Team(a)->energyIncome=energy;
		gs->Team(a)->energyStorage=energy;
	}

	// Read the unit restrictions
	int numRestrictions;
	file.GetDef(numRestrictions, "0", "GAME\\NumRestrictions");

	for (int i = 0; i < numRestrictions; ++i) {
		char key[100];
		sprintf(key, "GAME\\RESTRICT\\Unit%d", i);
		string resName = file.SGetValueDef("", key);
		sprintf(key, "GAME\\RESTRICT\\Limit%d", i);
		int resLimit;
		file.GetDef(resLimit, "0", key);

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
		font->glPrint("Connecting to server %i",40-(int)(net->curTime-net->connections[0].lastReceiveTime));
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
	if(gu->myPlayerNum==0 && keys[SDLK_RETURN] && keys[SDLK_LCTRL]){

		forceReady=true;
	}
	if(forceReady)
		allReady=true;
	if(allReady){
		if(readyTime==0 && !net->playbackDemo){
			int mode=configHandler.GetInt("CamMode",1);
			mouse->currentCamController=mouse->camControllers[mode];
			mouse->currentCamControllerNum=mode;
			mouse->currentCamController->SetPos(gs->Team(gu->myTeam)->startPos);
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

