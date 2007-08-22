#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <SDL.h>
#include "CameraController.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "LogOutput.h"
#include "GameServer.h"
#include "Player.h"
#include "TdfParser.h"
#include "Team.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
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
	startPosType=StartPos_Fixed;
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

/** @brief random number generator function object for use with std::random_shuffle */
struct UnsyncedRandomNumberGenerator {
	/** @brief returns a random number in the range [0, n) */
	int operator()(int n) {
		// a simple gu->usRandInt() % n isn't random enough
		return gu->usRandInt() * n / (RANDINT_MAX + 1);
	}
};

bool CGameSetup::Init(const char* buf, int size)
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
	baseMod=archiveScanner->ModArchiveToModName(file.SGetValueDef("","GAME\\Gametype"));
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
	gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
	gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);

	demoName = file.SGetValueDef("","GAME\\Demofile");
	if (!demoName.empty()) {
		hostDemo = true;
	}
	saveName = file.SGetValueDef("","GAME\\Savefile");
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

	int startPosTypeInt;
	file.GetDef(startPosTypeInt,"0","GAME\\StartPosType");
	if (startPosTypeInt < 0 || startPosTypeInt > StartPos_Last)
		startPosTypeInt = 0;
	startPosType = (StartPosType)startPosTypeInt;

	for (int a = 0; a < gs->activeTeams; ++a) {
		// ready up automatically unless startPosType is choose in game
		readyTeams[a] = (startPosType != StartPos_ChooseInGame);
		teamStartNum[a] = a;
	}

	if (startPosType == StartPos_Random) {
		//server syncs these later, so we can use unsynced rng
		UnsyncedRandomNumberGenerator rng;
		std::random_shuffle(&teamStartNum[0], &teamStartNum[gs->activeTeams], rng);
	}

	if (startPosType == StartPos_ChooseInGame)
		SAFE_NEW CStartPosSelecter();

	// gaia adjustments
	if (gs->useLuaGaia) {
		gs->gaiaTeamID = gs->activeTeams;
		gs->gaiaAllyTeamID = gs->activeAllyTeams;
		gs->activeTeams++;
		gs->activeAllyTeams++;
		teamStartNum[gs->gaiaTeamID] = gs->gaiaTeamID;
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
	gu->spectatingFullView   = gu->spectating;
	gu->spectatingFullSelect = gu->spectating;

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
		sprintf(teamName, "TEAM%i", teamStartNum[a]);
		p2.GetDef(x, "1000", string("MAP\\") + teamName + "\\StartPosX");
		p2.GetDef(z, "1000", string("MAP\\") + teamName + "\\StartPosZ");
		gs->Team(a)->startPos = float3(x, 100, z);

		if (startPosType == StartPos_ChooseInGame)
			gs->Team(a)->startPos.y=-500;	//show that we havent selected start pos yet

		if (startPosType == StartPos_ChooseBeforeGame) {
			std::string xpos = file.SGetValueDef("", s + "StartPosX");
			std::string zpos = file.SGetValueDef("", s + "StartPosZ");
			if (!xpos.empty()) gs->Team(a)->startPos.x = atoi(xpos.c_str());
			if (!zpos.empty()) gs->Team(a)->startPos.z = atoi(zpos.c_str());
		}
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
		gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
	}

	assert(gs->activeTeams <= MAX_TEAMS);
	assert(gs->activeAllyTeams <= MAX_TEAMS);

	return true;
}

void CGameSetup::Draw()
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(0.3f, 0.7f, 0.0f);
	glScalef(0.03f, 0.04f, 0.1f);
	if (!gameServer && !net->Connected()) {
		if ( ((SDL_GetTicks()/1000) % 2) == 0 )
			font->glPrint("Connecting to server .");
		else
			font->glPrint("Connecting to server  ");
	} else if (readyTime > 0) {
		font->glPrint("Starting in %i", 3 - (SDL_GetTicks() - readyTime) / 1000);
	} else if (!readyTeams[gu->myTeam]) {
		font->glPrint("Choose start pos");
	} else if (gu->myPlayerNum==0) {
		glTranslatef(-8.0f, 0.0f, 0.0f);
		font->glPrint("Waiting for players, Ctrl+Return to force start");
	} else {
		font->glPrint("Waiting for players");
	}
	glPopMatrix();

	glPushMatrix();
	for(int a=0;a<numPlayers;a++){
		const float* color;
		const float red[4]    = { 1.0f ,0.2f, 0.2f, 1.0f };
		const float green[4]  = { 0.2f, 1.0f, 0.2f, 1.0f };
		const float yellow[4] = { 0.8f, 0.8f, 0.2f, 1.0f };
		const float dark[4]   = { 0.2f, 0.2f, 0.2f, 0.8f };
		if (!gs->players[a]->readyToStart) {
			color = red;
		} else if (!readyTeams[gs->players[a]->team]) {
			color = yellow;
		} else {
			color = green;
		}
		const float yScale = 0.028f;
		const float yShift = yScale * 1.25f;
		const float xScale = yScale / gu->aspectRatio;
		const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
		const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
		glPushMatrix();
		glTranslatef(0.3f, 0.64f - (a * yShift), 0.0f);
		glScalef(xScale, yScale, 1.0f);
		font->glPrintOutlined(gs->players[a]->playerName.c_str(),
		                      xPixel, yPixel, color, dark);
		glPopMatrix();
	}
	glPopMatrix();
}

bool CGameSetup::Update()
{
	bool allReady=true;
	if(!gameServer && net->Connected())
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
		if(readyTime==0 && !net->IsDemoServer()){
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
