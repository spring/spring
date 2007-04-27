#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <SDL.h>
#include "CameraController.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "LogOutput.h"
#include "NetProtocol.h"
#include "Player.h"
#include "TdfParser.h"
#include "Team.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/TAPalette.h"
#include "UI/MouseHandler.h"
#include "UI/StartPosSelecter.h"

CGameSetup* gameSetup=0;

using namespace std;
extern Uint8 *keys;

extern string stupidGlobalMapname;

CGameSetup::CGameSetup()
{
	readyTime=0;
	gameSetupText=0;
	startPosType=0;
	numDemoPlayers=0;
	hostDemo=false;
	forceReady=false;
}

CGameSetup::~CGameSetup()
{
	delete[] gameSetupText;
}

bool CGameSetup::Init(std::string setupFile)
{
	setupFileName = setupFile;
	if(setupFile.empty())
		return false;
	CFileHandler fh(setupFile);
	if (!fh.FileExists())
		return false;
	char* c=SAFE_NEW char[fh.FileSize()];
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

	gameSetupText=SAFE_NEW char[size];
	memcpy(gameSetupText,buf,size);
	gameSetupTextLength=size;

	file.LoadBuffer(buf,size);

	if(!file.SectionExist("GAME"))
		return false;

	mapname=file.SGetValueDef("","GAME\\mapname");
	scriptName=file.SGetValueDef("Commanders","GAME\\scriptname");
	baseMod=archiveScanner->ModArchiveToModName(file.SGetValueDef(MOD_FILE,"GAME\\Gametype"));
	file.GetDef(hostip,          "0",   "GAME\\HostIP");
	file.GetDef(hostport,        "0",   "GAME\\HostPort");
	file.GetDef(maxUnits,        "500", "GAME\\MaxUnits");
	file.GetDef(gs->gameMode,    "0",   "GAME\\GameMode");
	file.GetDef(gs->noHelperAIs, "0",   "GAME\\NoHelperAIs");
	file.GetDef(sourceport,      "0",   "GAME\\SourcePort");
	file.GetDef(limitDgun,       "0",   "GAME\\LimitDgun");
	file.GetDef(diminishingMMs,  "0",   "GAME\\DiminishingMMs");
	file.GetDef(disableMapDamage,"0",   "GAME\\DisableMapDamage");

	const string luaGaiaStr  = file.SGetValueDef("GAME\\LuaGaia",  "1");
	const string luaRulesStr = file.SGetValueDef("GAME\\LuaRules", "1");
	gs->useLuaGaia  = (luaGaiaStr  != "0");
	gs->useLuaRules = (luaRulesStr != "0");

	demoName = file.SGetValueDef("","GAME\\Demofile");
	if (!demoName.empty()) {
		hostDemo = true;
	}
	file.GetDef(ghostedBuildings,"1","GAME\\GhostedBuildings");

	file.GetDef(maxSpeed, "3.0", "GAME\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\MinSpeed");

	// Determine if the map is inside an archive, and possibly map needed archives
	CFileHandler* f = SAFE_NEW CFileHandler("maps/" + mapname);
	if (!f->FileExists()) {
		vector<string> ars = archiveScanner->GetArchivesForMap(mapname);
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			if (!hpiHandler->AddArchive(*i, false))
				logOutput.Print("Warning: Couldn't load archive '%s'.", i->c_str());
		}
	}
	delete f;

	file.GetDef(myPlayer,"0","GAME\\MyPlayerNum");
	gu->myPlayerNum=myPlayer;
	file.GetDef(numPlayers,"2","GAME\\NumPlayers");
	file.GetDef(gs->activeTeams,"2","GAME\\NumTeams");
	file.GetDef(gs->activeAllyTeams,"2","GAME\\NumAllyTeams");
	
	// gaia adjustments
	if (gs->useLuaGaia) {
		gs->gaiaTeamID = gs->activeTeams;
		gs->gaiaAllyTeamID = gs->activeAllyTeams;
		gs->activeTeams++;
		gs->activeAllyTeams++;
	}

	file.GetDef(startPosType,"0","GAME\\StartPosType");
	if(startPosType==2){
		for(int a=0;a<gs->activeTeams;++a)
			readyTeams[a]=false;
		for(int a=0;a<gs->activeTeams;++a)
			teamStartNum[a]=a;
		SAFE_NEW CStartPosSelecter();
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

		gs->players[a]->team=atoi(file.SGetValueDef("0", s + "team").c_str());
		gs->players[a]->spectator=!!atoi(file.SGetValueDef("0", s + "spectator").c_str());
		gs->players[a]->playerName=file.SGetValueDef("0", s + "name");
		gs->players[a]->countryCode=file.SGetValueDef("", s + "countryCode");

		int fromDemo;
		file.GetDef(fromDemo,"0",s+"IsFromDemo");
		if(fromDemo)
			numDemoPlayers++;
	}
	gu->spectating = gs->players[myPlayer]->spectator;
	gu->spectatingFullView = gu->spectating;
	gu->spectatingFullSelect = false;

	TdfParser p2;
	CReadMap::OpenTDF (mapname, p2);

	for(int a=0;a<gs->activeTeams;++a){
		char section[50];
		sprintf(section,"GAME\\TEAM%i\\",a);
		string s(section);

		int colorNum=atoi(file.SGetValueDef("0",s+"color").c_str());
		colorNum%=palette.NumTeamColors();
		float3 defaultCol(palette.teamColor[colorNum][0]/255.0f,palette.teamColor[colorNum][1]/255.0f,palette.teamColor[colorNum][2]/255.0f);
		float3 color=file.GetFloat3(defaultCol,s+"rgbcolor");
		for(int b=0;b<3;++b){
			gs->Team(a)->color[b]=int(color[b]*255);
		}
		gs->Team(a)->color[3]=255;

 		gs->Team(a)->handicap=atof(file.SGetValueDef("0",s+"handicap").c_str())/100+1;
 		gs->Team(a)->leader=atoi(file.SGetValueDef("0",s+"teamleader").c_str());
 		gs->Team(a)->side = StringToLower(file.SGetValueDef("arm",s+"side").c_str());
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

	// setup the gaia team
	if (gs->useLuaGaia) {
		CTeam* team = gs->Team(gs->gaiaTeamID);
		team->color[0] = 255;
		team->color[1] = 255;
		team->color[2] = 255;
		team->color[3] = 255;
		team->gaia = true;
		readyTeams[gs->gaiaTeamID] = true;
	}

	assert(gs->activeTeams <= MAX_TEAMS);
	assert(gs->activeAllyTeams <= MAX_TEAMS);

	return true;
}

void CGameSetup::Draw()
{
	glColor4f(1,1,1,1);
	glPushMatrix();
	glTranslatef(0.3f,0.7f,0.0f);
	glScalef(0.03f,0.04f,0.1f);
	if(!serverNet && net->inInitialConnect){
		font->glPrint("Connecting to server %i",40-(int)(net->curTime-net->connections[0].lastReceiveTime));
	}else if(readyTime>0)
		font->glPrint("Starting in %i", 3 - (SDL_GetTicks() - readyTime) / 1000);
	else if(!readyTeams[gu->myTeam])
		font->glPrint("Choose start pos");
	else if(gu->myPlayerNum==0){
		glTranslatef(-8,0.0f,0.0f);
		font->glPrint("Waiting for players, Ctrl+Return to force start");
	}else
		font->glPrint("Waiting for players");

	glPopMatrix();
	glPushMatrix();
	for(int a=0;a<numPlayers;a++){
		if(!gs->players[a]->readyToStart){
			glColor4f(1,0.2f,0.2f,1);
		} else if(!readyTeams[gs->players[a]->team]){
			glColor4f(0.8f,0.8f,0.2f,1);
		} else {
			glColor4f(0.2f,1,0.2f,1);
		}
		glPushMatrix();
		glTranslatef(0.3f,0.6f-a*0.05f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		font->glPrintRaw(gs->players[a]->playerName.c_str());
		glPopMatrix();
	}
	glPopMatrix();
}

bool CGameSetup::Update()
{
	bool allReady=true;
	if(!serverNet && net->inInitialConnect)
		return false;
	for(int a=0;a<numPlayers;a++){
		if(!gs->players[a]->readyToStart){
			allReady=false;
			break;
		} else if(!readyTeams[gs->players[a]->team]){
			allReady=false;
			break;
		}
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
			mouse->CameraTransition(1.0f);
			readyTime = SDL_GetTicks();
		}
	}
	return readyTime && (SDL_GetTicks() - readyTime) > 3000;
}
