// Game.cpp: implementation of the CGame class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "StdAfx.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>
#include <locale>

#include "Rendering/GL/myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include <SDL_keyboard.h>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <SDL_types.h>

#include "Game.h"
#include "float.h"
#include "Camera.h"
#include "CameraController.h"
#include "ConsoleHistory.h"
#include "FPUCheck.h"
#include "GameHelper.h"
#include "GameServer.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "LoadSaveHandler.h"
#include "SelectedUnits.h"
#include "SyncTracer.h"
#include "Team.h"
#include "TimeProfiler.h"
#include "WordCompletion.h"
#ifdef _WIN32
#include "winerror.h"
#endif
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/GroupHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Game/UI/GUI/GUIframe.h"
//#include "HpiHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Net.h"
//#include "PhysicsEngine.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/FartextureHandler.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/SensorHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Projectiles/SmokeProjectile.h"
#include "Sim/Units/COB/CobEngine.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTracker.h"
#include "StartScripts/Script.h"
#include "StartScripts/ScriptHandler.h"
#include "Sync/SyncedPrimitiveIO.h"
#include "UI/CursorIcons.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/KeyBindings.h"
#include "UI/KeyCodes.h"
#include "UI/MiniMap.h"
#include "UI/MouseHandler.h"
#include "UI/NewGuiDefine.h"
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/TooltipConsole.h"

#ifndef NO_AVI
#include "Platform/Win/AVIGenerator.h"
#endif

#ifdef _MSC_VER
#include "Platform/Win/DxSound.h"
#else
#include "Platform/Linux/OpenALSound.h"
#endif
#include "Platform/NullSound.h"

#ifdef DIRECT_CONTROL_ALLOWED
#include "myMath.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Weapons/Weapon.h"
#endif

#ifdef NEW_GUI
#include "UI/GUI/GUIcontroller.h"
#endif

#include "mmgr.h"

GLfloat LightDiffuseLand[]=	{ 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat LightAmbientLand[]=	{ 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat FogLand[]=			{ 0.7f,	0.7f, 0.8f, 0	 };
GLfloat FogWhite[]=			{ 1.0f,	1.0f, 1.0f, 0	 };
GLfloat FogBlack[]=			{ 0.0f,	0.0f, 0.0f, 0	 };

extern bool globalQuit;
CGame* game=0;
extern Uint8 *keys;
extern bool fullscreen;
extern string stupidGlobalMapname;

CSound* sound = 0;

static CSound* CreateSoundInterface()
{
  if (!!configHandler.GetInt("NoSound",0)) {
    return new CNullSound ();
  }
#ifdef _MSC_VER
	return new CDxSound ();
#else
	return new COpenALSound ();
#endif
}

CGame::CGame(bool server,std::string mapname, std::string modName)
{
	script = NULL;
	showList = NULL;

	time(&starttime);
	lastTick=clock();
	consumeSpeed=1;
	leastQue=0;
	que=0;
	timeLeft=0;

	oldframenum=0;
	if(server)
		gs->players[0]->readyToStart=true;
	gs->players[0]->active=true;

	if(net->playbackDemo)
		gu->myPlayerNum=MAX_PLAYERS-1;

	for(int a=0;a<8;++a)
		camMove[a]=false;
	for(int a=0;a<4;++a)
		camRot[a]=false;

	fps=0;
	thisFps=0;
	totalGameTime=0;

	debugging=false;

	bOneStep=false;
	creatingVideo=false;

	playing=false;
	allReady=false;
	hideInterface=false;
	gameOver=false;
	showClock=!!configHandler.GetInt("ShowClock",1);
	showPlayerInfo=!!configHandler.GetInt("ShowPlayerInfo",1);
	windowedEdgeMove=!!configHandler.GetInt("WindowedEdgeMove",1);
	gamePausable=true;
	noSpectatorChat=false;


	inbufpos=0;
	inbuflength=0;

	chatting=false;
	userInput="";
	userPrompt="";

	consoleHistory = new CConsoleHistory;
	wordCompletion = new CWordCompletion;
	for (int pp = 0; pp < MAX_PLAYERS; pp++) {
	  wordCompletion->AddWord(gs->players[pp]->playerName, false, false);
	}

#ifdef DIRECT_CONTROL_ALLOWED
	oldPitch=0;
	oldHeading=0;
	oldStatus=255;
#endif

	ENTER_UNSYNCED;
	sound=CreateSoundInterface ();
	gameSoundVolume=configHandler.GetInt("SoundVolume", 60)*0.01f;
	soundEnabled=true;
	sound->SetVolume(gameSoundVolume);
	sound->SetUnitReplyVolume (configHandler.GetInt ("UnitReplySoundVolume",80)*0.01f);

	camera=new CCamera();
	cam2=new CCamera();
	mouse=new CMouseHandler();
	cursorIcons=new CCursorIcons();
	selectionKeys=new CSelectionKeyHandler();
	tooltip=new CTooltipConsole();

	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load
	helper=new CGameHelper(this);
	//	physicsEngine = new CPhysicsEngine();
	ENTER_UNSYNCED;
	shadowHandler=new CShadowHandler();

	modInfo=new CModInfo(modName.c_str());

	ENTER_SYNCED;
	ground=new CGround();
	readmap = CReadMap::LoadMap (mapname);
	moveinfo=new CMoveInfo();
	groundDecals=new CGroundDecalHandler();
	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load
	ENTER_UNSYNCED;
#ifndef NEW_GUI
	guihandler=new CGuiHandler();
	minimap=new CMiniMap();
#endif

#ifdef NEW_GUI
	GUIcontroller::GlobalInitialize();
#endif

	ENTER_MIXED;
	ph=new CProjectileHandler();

	ENTER_SYNCED;
	damageArrayHandler=new CDamageArrayHandler();
	unitDefHandler=new CUnitDefHandler();

	ENTER_UNSYNCED;
#ifndef NEW_GUI
#endif
	inMapDrawer=new CInMapDraw();

	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
	  wordCompletion->AddWord(uit->first, false, true);
	}

	geometricObjects=new CGeometricObjects();
	ENTER_SYNCED;
	qf=new CQuadField();
	ENTER_MIXED;
	featureHandler=new CFeatureHandler();
	ENTER_SYNCED;
	mapDamage=IMapDamage::GetMapDamage();
	sensorHandler=new CSensorHandler();
	loshandler=new CLosHandler();
	radarhandler=new CRadarHandler(false);
	if(!server) net->Update();	//prevent timing out during load
	ENTER_MIXED;
	uh=new CUnitHandler();
	iconHandler=new CIconHandler();
	unitDrawer=new CUnitDrawer();
	fartextureHandler = new CFartextureHandler();
	if(!server) net->Update();	//prevent timing out during load
	modelParser = new C3DModelParser();
 	ENTER_SYNCED;
 	if(!server) net->Update();	//prevent timing out during load
 	featureHandler->LoadFeaturesFromMap(CScriptHandler::Instance().chosenScript->loadGame);
 	if(!server) net->Update();	//prevent timing out during load
 	pathManager = new CPathManager();
 	if(!server) net->Update();	//prevent timing out during load
	ENTER_UNSYNCED;
	sky=CBaseSky::GetSky();
#ifndef NEW_GUI
	resourceBar=new CResourceBar();
	keyCodes=new CKeyCodes();
	keyBindings=new CKeyBindings("uikeys.txt");
#endif
	if(!server) net->Update();	//prevent timing out during load

	water=CBaseWater::GetWater();
	grouphandler=new CGroupHandler(gu->myTeam);
	globalAI=new CGlobalAIHandler();

	ENTER_MIXED;
	if(!shadowHandler->drawShadows){
		for(int a=0;a<3;++a){
			LightAmbientLand[a]=unitDrawer->unitAmbientColor[a];
			LightDiffuseLand[a]=unitDrawer->unitSunColor[a];
		}
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand);		// Setup The Ambient Light
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand);		// Setup The Diffuse Light
		glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand);		// Setup The Diffuse Light
		glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);
	}

	showList=0;

	info->AddLine("TA Spring %s",VERSION_STRING);

	if(!server)
		net->SendData<unsigned int>(NETMSG_EXECHECKSUM, CreateExeChecksum());

	if (gameSetup) {
		maxUserSpeed = gameSetup->maxSpeed;
		minUserSpeed = gameSetup->minSpeed;
	}
	else {
		maxUserSpeed=3;
		minUserSpeed=0.3;
	}

	CPlayer* p=gs->players[gu->myPlayerNum];
	if(!gameSetup)
		p->playerName=configHandler.GetString("name","");
	//sending your playername to the server indicates that you are finished loading
	net->SendSTLData<unsigned char, std::string>(NETMSG_PLAYERNAME, gu->myPlayerNum, p->playerName);

	lastModGameTimeMeasure = SDL_GetTicks();
	lastframe = SDL_GetTicks();
	lastUpdate = SDL_GetTicks();
	lastMoveUpdate = SDL_GetTicks();

	glFogfv(GL_FOG_COLOR,FogLand);
	glFogf(GL_FOG_START,0);
	glFogf(GL_FOG_END,gu->viewRange*0.98);
	glFogf(GL_FOG_DENSITY,1.0f);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glEnable(GL_FOG);
	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);
#ifdef TRACE_SYNC
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
#endif
	activeController=this;

	chatSound=sound->GetWaveId("beep4.wav");

	if(gameServer)
		gameServer->gameLoading=false;

	UnloadStartPicture();

	if(serverNet && serverNet->playbackDemo)
		serverNet->StartDemoServer();
}


CGame::~CGame()
{
	configHandler.SetInt("ShowClock",showClock);
	configHandler.SetInt("ShowPlayerInfo",showPlayerInfo);

	CBaseGroundDrawer *gd = readmap ? readmap->GetGroundDrawer() : 0;
	if (gd) configHandler.SetInt("GroundDetail",gd->viewRadius);
	if (treeDrawer) configHandler.SetInt("TreeRadius",(unsigned int)(treeDrawer->baseTreeDistance*256));

	ENTER_MIXED;
#ifndef NO_AVI
	if(creatingVideo){
		creatingVideo=false;
		aviGenerator->ReleaseEngine();
		delete aviGenerator;
		aviGenerator=0;
	}
#endif
#ifdef TRACE_SYNC
	tracefile << "End game\n";
#endif
	delete gameServer;

	globalAI->PreDestroy ();
	delete water;
	delete sky;

	delete resourceBar;
	delete uh;
	delete unitDrawer;
	delete featureHandler;
	delete geometricObjects;
	delete ph;
	delete globalAI;
	delete grouphandler;
	if ( minimap )
		delete minimap;
	delete pathManager;
	delete groundDecals;
	delete ground;
	delete inMapDrawer;
	delete net;
	delete radarhandler;
	delete loshandler;
	delete mapDamage;
	delete qf;
	delete tooltip;
	delete keyBindings;
	delete keyCodes;
	delete sound;
	if ( guihandler )
		delete guihandler;
	delete selectionKeys;
	delete mouse;
	delete cursorIcons;
	delete helper;
	delete shadowHandler;
	delete moveinfo;
	delete unitDefHandler;
	delete damageArrayHandler;
	delete hpiHandler;
	delete archiveScanner;
	delete modelParser;
	delete fartextureHandler;
	CCategoryHandler::RemoveInstance();
	delete modInfo;
	delete camera;
	delete cam2;
	delete info;
	delete consoleHistory;
	delete wordCompletion;
}


//called when the key is pressed by the user (can be called several times due to key repeat)
int CGame::KeyPressed(unsigned short k, bool isRepeat)
{
	if(!gameOver && !isRepeat)
		gs->players[gu->myPlayerNum]->currentStats->keyPresses++;
	//	info->AddLine("%i",(int)k);

#ifdef NEW_GUI
	GUIcontroller::KeyDown(k);
	// #endif
#else

	if(showList){					//are we currently showing a list?
		if(k==SDLK_UP)
			showList->UpOne();
		if(k==SDLK_DOWN)
			showList->DownOne();
		if(k==SDLK_RETURN){
			showList->Select();
			showList=0;
		}
		if(k==27 && playing){
			showList=0;
		}
		return 0;
	}
	
	string fakeGuiKey("");
	if (userWriting){
#ifndef NO_CLIPBOARD
		if ((k=='V') && keys[SDLK_LCTRL]){
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
		if(k==SDLK_RETURN){
			userWriting=false;
			keys[k] = false;		//prevent game start when server chats
			if (chatting && (userInput.find(".cmd") == 0)) {
				fakeGuiKey = userInput.substr(5);
				consoleHistory->AddLine(userInput);
				chatting = false;
				userInput = "";
			} else {
				return 0;
			}
		}
		if(k==27 && (chatting || inMapDrawer->wantLabel)){ // escape
			userWriting=false;
			chatting=false;
			inMapDrawer->wantLabel=false;
			userInput="";
		}
		if(k==SDLK_UP) {
			userInput = consoleHistory->PrevLine(userInput);
			return 0;
		}
		if(k==SDLK_DOWN) {
			userInput = consoleHistory->NextLine(userInput);
			return 0;
		}
		if(k==SDLK_TAB) {
			vector<string> partials = wordCompletion->Complete(userInput);
			if (!partials.empty()) {
				string msg;
				for (int i = 0; i < partials.size(); i++) {
					msg += "  ";
					msg += partials[i];
				}
				info->AddLine(msg);
			}
		}
		if(fakeGuiKey.size() <= 0) {
			return 0;
		}
	}

	if(k>='0' && k<='9' && gu->spectating){
		int team=k-'0'-1;
		if(team<0)
			team=9;
		if(team<gs->activeTeams){
			gu->myTeam=team;
			gu->myAllyTeam=gs->AllyTeam(team);
		}
		return 0;
	}

  std::string s;
  std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
  if (fakeGuiKey.size() > 0) {
    s=fakeGuiKey;
  } else {
		for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
			if((*ri)->KeyPressed(k)) {
				return 0;
			}
		}
		CKeySet ks(k, false);
		s = keyBindings->GetAction(ks, "");
	}

	// grab a gd pointer
	CBaseGroundDrawer *gd = readmap->GetGroundDrawer ();
	
	static char *buildFaceDirs[] = { "South", "East", "North", "West" };
	

	// process the action
	if (s.find("select:") == 0) {
		string selectCmd(s, 7);
		selectionKeys->DoSelection(selectCmd);
	}
	else if (s=="drawinmap") {
		inMapDrawer->keyPressed=true;
	}
	else if (s=="drawlabel") {
		float3 pos = inMapDrawer->GetMouseMapPos();
		if (pos.x >= 0) {
			inMapDrawer->keyPressed = false;
			inMapDrawer->PromptLabel(pos);
			if ((k >= SDLK_SPACE) && (k <= SDLK_DELETE)) {
				ignoreNextChar=true;
			}
		}
	}
	else if (!isRepeat && s=="mouse1") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 1);
	}
	else if (!isRepeat && s=="mouse2") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 2);
	}
	else if (!isRepeat && s=="mouse3") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 3);
	}
	else if (!isRepeat && s=="mouse4") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 4);
	}
	else if (!isRepeat && s=="mouse5") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 5);
	}
	else if (s=="viewfps") {
		mouse->SetCameraMode(0);
	}
	else if (s=="viewta") {
		mouse->SetCameraMode(1);
	}
	else if (s=="viewtw") {
		mouse->SetCameraMode(2);
	}
	else if (s=="viewrot") {
		mouse->SetCameraMode(3);
	}
	else if (s=="moveforward") {
		camMove[0]=true;
	}
	else if (s=="moveback") {
		camMove[1]=true;
	}
	else if (s=="moveleft") {
		camMove[2]=true;
	}
	else if (s=="moveright") {
		camMove[3]=true;
	}
	else if (s=="moveup") {
		camMove[4]=true;
	}
	else if (s=="movedown") {
		camMove[5]=true;
	}
	else if (s=="movefast") {
		camMove[6]=true;
	}
	else if (s=="moveslow") {
		camMove[7]=true;
	}
	else if (s=="group0") {
		grouphandler->GroupCommand(0);
	}
	else if (s=="group1") {
		grouphandler->GroupCommand(1);
	}
	else if (s=="group2") {
		grouphandler->GroupCommand(2);
	}
	else if (s=="group3") {
		grouphandler->GroupCommand(3);
	}
	else if (s=="group4") {
		grouphandler->GroupCommand(4);
	}
	else if (s=="group5") {
		grouphandler->GroupCommand(5);
	}
	else if (s=="group6") {
		grouphandler->GroupCommand(6);
	}
	else if (s=="group7") {
		grouphandler->GroupCommand(7);
	}
	else if (s=="group8") {
		grouphandler->GroupCommand(8);
	}
	else if (s=="group9") {
		grouphandler->GroupCommand(9);
	}
	else if (s=="lastmsgpos") {
		mouse->currentCamController->SetPos(info->lastMsgPos);
		mouse->inStateTransit=true;
		mouse->transitSpeed=0.5;
	}
	else if ((s=="chat") || (s=="chatall") || (s=="chatally") || (s=="chatspec")) {
		if (s=="chatall")  { userInputPrefix = ""; }
		if (s=="chatally") { userInputPrefix = "a:"; }
		if (s=="chatspec") { userInputPrefix = "s:"; }
		userWriting=true;
		userPrompt="Say: ";
		userInput=userInputPrefix;
		chatting=true;
		if(k!=SDLK_RETURN)
			ignoreNextChar=true;
		consoleHistory->ResetPosition();
	}
	else if (s=="track") {
		unitTracker.Track();
	}
	else if (s=="trackoff") {
		unitTracker.Disable();
	}
	else if (s=="trackmode") {
		unitTracker.IncMode();
	}
	else if (s=="toggleoverview") {
		mouse->ToggleOverviewCamera();
	}
	else if (s=="showhealthbars") {
		unitDrawer->showHealthBars=!unitDrawer->showHealthBars;
	}
	else if (s=="pause") {
		if(net->playbackDemo){
			gs->paused=!gs->paused;
		} else {
			net->SendData<unsigned char, unsigned char>(
					NETMSG_PAUSE, !gs->paused, gu->myPlayerNum);
			lastframe = SDL_GetTicks();
		}
	}
	else if (s=="singlestep") {
		bOneStep=true;
	}
	else if (s=="debug") {
		gu->drawdebug=!gu->drawdebug;
	}
	else if (s=="nosound") {
		soundEnabled=!soundEnabled;
		sound->SetVolume (soundEnabled ? gameSoundVolume : 0.0f);
	}
	else if (s=="savegame"){
		CLoadSaveHandler ls;
		ls.SaveGame("Test.ssf");
	}

#ifndef NO_AVI
	else if (s=="createvideo") {
		if(creatingVideo){
			creatingVideo=false;
			aviGenerator->ReleaseEngine();
			delete aviGenerator;
			aviGenerator=0;
			//			info->AddLine("Finished avi");
		} else {
			creatingVideo=true;
			string name;
			for(int a=0;a<99;++a){
				char t[50];
				itoa(a,t,10);
				name=string("video")+t+".avi";
				CFileHandler ifs(name);
				if(!ifs.FileExists())
					break;
			}
			int x=gu->screenx;
			x-=gu->screenx%4;

			int y=gu->screeny;
			y-=gu->screeny%4;

			BITMAPINFOHEADER bih;
			bih.biSize=sizeof(BITMAPINFOHEADER);
			bih.biWidth=x;
			bih.biHeight=y;
			bih.biPlanes=1;
			bih.biBitCount=24;
			bih.biCompression=BI_RGB;
			bih.biSizeImage=0;
			bih.biXPelsPerMeter=1000;
			bih.biYPelsPerMeter=1000;
			bih.biClrUsed=0;
			bih.biClrImportant=0;

			aviGenerator=new CAVIGenerator();
			aviGenerator->SetFileName(name.c_str());
			aviGenerator->SetRate(30);
			aviGenerator->SetBitmapHeader(&bih);
			Sint32 hr=aviGenerator->InitEngine();
			if(hr!=AVIERR_OK){
				creatingVideo=false;
			} else {
				info->AddLine("Recording avi to %s size %i %i",name.c_str(),x,y);
			}
		}
	}
#endif

	else if (s=="updatefov") {
		gd->updateFov=!gd->updateFov;
	}
	else if (s=="incbuildfacing"){
		guihandler->buildFacing ++;
		if (guihandler->buildFacing > 3)
			guihandler->buildFacing = 0;

		info->AddLine(string("Buildings set to face ")+buildFaceDirs[guihandler->buildFacing]);
	}
	else if (s=="decbuildfacing"){
		guihandler->buildFacing --;
		if (guihandler->buildFacing < 0)
			guihandler->buildFacing = 3;

		info->AddLine(string("Buildings set to face ")+buildFaceDirs[guihandler->buildFacing]);
	}
	else if (s=="drawtrees") {
		treeDrawer->drawTrees=!treeDrawer->drawTrees;
	}
	else if (s=="dynamicsky") {
		sky->dynamicSky=!sky->dynamicSky;
	}
	else if (s=="gameinfo") {
		new CGameInfo;
	}
	else if (s=="hideinterface") {
		hideInterface=!hideInterface;
	}
	else if (s=="increaseviewradius") {
		gd->viewRadius+=2;
		(*info) << "ViewRadius is now " << gd->viewRadius << "\n";
	}
	else if (s=="decreaseviewradius") {
		gd->viewRadius-=2;
		(*info) << "ViewRadius is now " << gd->viewRadius << "\n";
	}
	else if (s=="moretrees") {
		treeDrawer->baseTreeDistance+=0.2f;
		(*info) << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (s=="lesstrees") {
		treeDrawer->baseTreeDistance-=0.2f;
		(*info) << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (s=="moreclouds") {
		sky->cloudDensity*=0.95f;
		(*info) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (s=="lessclouds") {
		sky->cloudDensity*=1.05f;
		(*info) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (s=="speedup") {
		float speed=gs->userSpeedFactor;
		if (speed<1) {
			speed/=0.8;
			if (speed>0.99) {
				speed=1;
			}
		} else {
			speed+=0.5;
		}
		if (!net->playbackDemo) {
			net->SendData<float>(NETMSG_USER_SPEED, speed);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			(*info) << "Speed " << gs->speedFactor << "\n";
		}
	}
	else if (s=="slowdown") {
		float speed=gs->userSpeedFactor;
		if (speed<=1) {
			speed*=0.8;
			if (speed<0.1) {
				speed=0.1;
			}
		} else {
			speed-=0.5;
		}
		if (!net->playbackDemo) {
			net->SendData<float>(NETMSG_USER_SPEED, speed);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			(*info) << "Speed " << gs->speedFactor << "\n";
		}
	}

#ifdef DIRECT_CONTROL_ALLOWED
	else if (s=="controlunit") {
		Command c;
		c.id=CMD_STOP;
		c.options=0;
		selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que
		net->SendData<unsigned char>(NETMSG_DIRECT_CONTROL, gu->myPlayerNum);
	}
#endif

	else if (s=="showshadowmap") {
		shadowHandler->showShadowMap=!shadowHandler->showShadowMap;
	}
	else if (s=="showstandard") {
		gd->DisableExtraTexture();
	}
	else if (s=="showelevation") {
		gd->SetHeightTexture();
	}
	else if (s=="toggleradarandjammer"){
		gd->ToggleRadarAndJammer();
	}
	else if (s=="showmetalmap") {
		gd->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
	}
	else if (s=="showpathmap") {
		gd->SetPathMapTexture();
	}
	else if (s=="yardmap4") {
		//		groundDrawer->SetExtraTexture(readmap->yardmapLevels[3],readmap->yardmapPal,true);
	}
	/*	if (s=="showsupply"){
		groundDrawer->SetExtraTexture(supplyhandler->supplyLevel[gu->myTeam],supplyhandler->supplyPal);
		}*/
	/*	if (s=="showteam"){
		groundDrawer->SetExtraTexture(readmap->teammap,cityhandler->teampal);
		}*/
	else if (s=="togglelos") {
		gd->ToggleLosTexture();
	}
	else if (s=="sharedialog") {
		if(!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front())==0 && !gu->spectating)
			new CShareBox();
	}
	else if (s=="quit") {
		if(!keys[SDLK_LSHIFT]){
			info->AddLine("Use shift-esc to quit");
		}else{
			//The user wants to quit. Do we let him?
			bool userMayQuit=false;
			// Six cases when he may quit:
			//  * If the game isn't started players are free to leave.
			//  * If the game is over
			//  * If his team is dead.
			//  * If he's a spectator.
			//  * If there are other active players on his team.
			//  * If there are no other players
			if(!playing || gameOver || gs->Team(gu->myTeam)->isDead || gu->spectating || (net->onlyLocal && !gameServer)) {
				userMayQuit=true;
			}else{
				// Check if there are more active players on his team.
				for(int a=0;a<MAX_PLAYERS;++a){
					if(gs->players[a]->active && gs->players[a]->team==gu->myTeam && a!=gu->myPlayerNum){
						userMayQuit=true;
						break;
					}
				}
			} // .. if(!playing||isDead||spectating)

			// User may not quit if he is the only player in his still active team.
			// Present him with the options given in CQuitBox.
			if(!userMayQuit){
				if(!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front())==0){
					new CQuitBox();
				}
			} else {
				info->AddLine("User exited");
				globalQuit=true;
			}
		}
	}
	else if (s=="incguiopacity") {
		GUIframe::SetGUIOpacity(min(GUIframe::GetGUIOpacity()+0.1f,1.f));
	}
	else if (s=="decguiopacity") {
		GUIframe::SetGUIOpacity(max(GUIframe::GetGUIOpacity()-0.1f,0.f));
	}
	else if (s=="screenshot") {
		if (filesystem.CreateDirectory("screenshots")) {
			int x=gu->screenx;
			if(x%4)
				x+=4-x%4;
			unsigned char* buf=new unsigned char[x*gu->screeny*4];
			glReadPixels(0,0,x,gu->screeny,GL_RGBA,GL_UNSIGNED_BYTE,buf);
			CBitmap b(buf,x,gu->screeny);
			b.ReverseYAxis();
			char t[50];
			for(int a=0;a<9999;++a){
				sprintf(t,"screenshots/screen%03i.jpg",a);
				CFileHandler ifs(t);
				if(!ifs.FileExists())
					break;
			}
			b.Save(t);
			delete[] buf;
		}
	}
	else if (s=="grabinput") {
		SDL_GrabMode mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
		switch (mode) {
			case SDL_GRAB_ON: mode = SDL_GRAB_OFF; break;
			case SDL_GRAB_OFF: mode = SDL_GRAB_ON; break;
		}
		SDL_WM_GrabInput(mode);
	}
	else if (s=="keyload") {
		keyBindings->Load("uikeys.txt");
	}
	else if (s=="keysave") {
		keyBindings->Save("uikeys.tmp"); // tmp, not txt
	}
	else if (s=="keyprint") {
		keyBindings->Print();
	}
	else if (s=="keysyms") {
		keyCodes->PrintNameToCode();
	}
	else if (s=="keycodes") {
		keyCodes->PrintCodeToName();
	}
	else if (s=="keydebug") {
		keyBindings->debug = !keyBindings->debug;
	}

#endif // NEW_GUI

	return 0;
}


//Called when a key is released by the user
int CGame::KeyReleased(unsigned short k)
{
	//	keys[k] = false;

#ifdef NEW_GUI
	GUIcontroller::KeyUp(k);

#else

	if ((userWriting) && (((k>=' ') && (k<='Z')) || (k==8) || (k==190) )){
		return 0;
	}

	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri)->KeyReleased(k))
			return 0;
	}

	CKeySet ks(k, true);
	const string s = keyBindings->GetAction(ks, "");

	if(s=="drawinmap"){
		inMapDrawer->keyPressed=false;
	}
	else if (s=="buildfaceup") {
		guihandler->buildFacing=0;
		info->AddLine("Buildings set to face South");
	}
	else if (s=="buildfaceright") {
		guihandler->buildFacing=3;
		info->AddLine("Buildings set to face West");
	}
	else if (s=="buildfacedown") {
		guihandler->buildFacing=2;
		info->AddLine("Buildings set to face North");
	}
	else if (s=="buildfaceleft") {
		guihandler->buildFacing=1;
		info->AddLine("Buildings set to face East");
	}
	else if (s=="moveforward") {
		camMove[0]=false;
	}
	else if (s=="moveback") {
		camMove[1]=false;
	}
	else if (s=="moveleft") {
		camMove[2]=false;
	}
	else if (s=="moveright") {
		camMove[3]=false;
	}
	else if (s=="moveup") {
		camMove[4]=false;
	}
	else if (s=="movedown") {
		camMove[5]=false;
	}
	else if (s=="movefast") {
		camMove[6]=false;
	}
	else if (s=="moveslow") {
		camMove[7]=false;
	}
	else if (s=="mouse1") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 1);
	}
	else if (s=="mouse2") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 2);
	}
	else if (s=="mouse3") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 3);
	}
	else if (s=="mousestate") {
		mouse->ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
	}
	// HACK   somehow weird things happen when MouseRelease is called for button 4 and 5.
	// Note that SYS_WMEVENT on windows also only sends MousePress events for these buttons.
// 	if (s=="mouse4")
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 4);

// 	if (s=="mouse5")
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 5);
#endif
	return 0;
}

bool CGame::Update()
{
	mouse->EmptyMsgQueUpdate();
	script = CScriptHandler::Instance().chosenScript;
	assert(script);
	thisFps++;

	Uint64 timeNow;
	timeNow = SDL_GetTicks();
	Uint64 difTime;
	difTime=timeNow-lastModGameTimeMeasure;
	float dif=float(difTime)/1000.;
	if(!gs->paused)
		gu->modGameTime+=dif*gs->speedFactor;
	gu->gameTime+=dif;
	if(playing && !gameOver)
		totalGameTime+=dif;
	lastModGameTimeMeasure=timeNow;

	if (gameServer && gu->autoQuit && gu->gameTime > gu->quitTime) {
		info->AddLine("Automatical quit enforced from commandline");
		return false;
	}

	time(&fpstimer);
	if 	(difftime(fpstimer,starttime)!=0){		//do once every second
		fps=thisFps;
		thisFps=0;

		consumeSpeed=((float)(GAME_SPEED+leastQue-2));
		leastQue=10000;
		if(!gameServer)
			timeLeft=0;

		starttime=fpstimer;
		oldframenum=gs->frameNum;

#ifdef TRACE_SYNC
		tracefile.DeleteInterval();
		tracefile.NewInterval();
#endif
	}

	UpdateUI();
	net->Update();

	if(gameServer)
		gameServer->gameClientUpdated=true;

#ifdef SYNCIFY		//syncify doesnt support multithreading ...
	gameServer->Update();
#endif

	if(creatingVideo && playing && gameServer){
		gameServer->CreateNewFrame();
	}

	//listen to network
	if(net->connected){
		if(!ClientReadNet()){
			info->AddLine("Client read net wanted quit");
			return false;
		}
	}

	if(gameServer && serverNet->waitOnCon && allReady && (keys[SDLK_RETURN] || script->onlySinglePlayer || gameSetup)){
		chatting=false;
		userWriting=false;
		gameServer->StartGame();
	}

	return true;
}

bool CGame::Draw()
{
	ASSERT_UNSYNCED_MODE;

	if (!gu->active) {
		SDL_Delay(10); // milliseconds
		return true;
	}

//	(*info) << mouse->lastx << "\n";
	if(!gs->paused && gs->frameNum>1 && !creatingVideo){
		Uint64 startDraw;
		startDraw = SDL_GetTicks();
		gu->timeOffset = ((float)(startDraw - lastUpdate))/1000.*GAME_SPEED*gs->speedFactor;
	} else  {
		gu->timeOffset=0;
		lastUpdate = SDL_GetTicks();
	}
	int a;
	std::string tempstring;

	ph->UpdateTextures();

	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);

	sky->Update();
//	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	//set camera
	mouse->UpdateCam();
	mouse->UpdateCursors();

	if(unitTracker.Enabled())
		unitTracker.SetCam();

	if(playing && (hideInterface || script->wantCameraControl))
		script->SetCamera();

	CBaseGroundDrawer *gd = readmap->GetGroundDrawer();
	gd->Update(); // let it update before shadows have to be drawn

	if(shadowHandler->drawShadows)
		shadowHandler->CreateShadows();
	if (unitDrawer->advShading)
		unitDrawer->UpdateReflectTex();

	camera->Update(false);

	water->UpdateWater(this);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	sky->Draw();
	gd->UpdateExtraTexture();
	gd->Draw();
	if(!readmap->voidWater && water->drawSolid)
 		water->Draw();

	selectedUnits.Draw();
	unitDrawer->Draw(false);
	featureHandler->Draw();
	if(gu->drawdebug && gs->cheatEnabled)
		pathManager->Draw();
	//transparent stuff
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	if(!readmap->voidWater && !water->drawSolid)
		water->Draw();
	if(treeDrawer->drawTrees)
		treeDrawer->DrawGrass();
	unitDrawer->DrawCloakedUnits();
	ph->Draw(false);
	sky->DrawSun();
	if(keys[SDLK_LSHIFT]) {
		selectedUnits.DrawCommands();
		cursorIcons->Draw();
	}

	mouse->Draw();

#ifndef NEW_GUI
	guihandler->DrawMapStuff();
#endif
	inMapDrawer->Draw();

	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	//reset fov
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);

	if(shadowHandler->drawShadows && shadowHandler->showShadowMap)
		shadowHandler->DrawShadowTex();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLoadIdentity();

	if(mouse->locked){
		glColor4f(1,1,1,0.5);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glVertex2f(0.5-10.0/gu->screenx,0.5);
		glVertex2f(0.5+10.0/gu->screenx,0.5);
		glVertex2f(0.5,0.5-10.0/gu->screeny);
		glVertex2f(0.5,0.5+10.0/gu->screeny);
		glEnd();
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl)
		DrawDirectControlHud();
#endif

	glEnable(GL_TEXTURE_2D);

#ifndef NEW_GUI
	if(!hideInterface) {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty()){
			std::deque<CInputReceiver*>::iterator ri;
			for(ri=--inputReceivers.end();ri!=inputReceivers.begin();--ri){
				(*ri)->Draw();
			}
			(*ri)->Draw();
		}
	}
#endif

	glEnable(GL_TEXTURE_2D);

	if(gu->drawdebug){
		glPushMatrix();
		glColor4f(1,1,0.5f,0.8f);
		glTranslatef(0.03f,0.02f,0.0f);
		glScalef(0.03f,0.04f,0.1f);

		//skriv ut fps etc
		font->glPrint("FPS %3.0d Frame %d Units %d Part %d(%d)",fps,gs->frameNum,uh->activeUnits.size(),ph->ps.size(),ph->currentParticles);
		glPopMatrix();

		if(playing){
			glPushMatrix();
			glTranslatef(0.03f,0.08f,0.0f);
			glScalef(0.02f,0.025f,0.1f);
			font->glPrint("xpos: %5.0f ypos: %5.0f zpos: %5.0f speed %2.2f",camera->pos.x,camera->pos.y,camera->pos.z,gs->speedFactor);
			glPopMatrix();
		}
	}

	if(gameSetup && !playing){
		allReady=gameSetup->Draw();
	} else if( gameServer && serverNet->waitOnCon){
		allReady=true;
		for(a=0;a<gs->activePlayers;a++)
			if(gs->players[a]->active && !gs->players[a]->readyToStart)
				allReady=false;

		if (allReady) {
			glColor3f(1.0f, 1.0f, 1.0f);
			font->glPrintCentered (0.5f, 0.5f, 1.5f, "Waiting for connections. Press return to start");
		}
	}

	if(userWriting){
		glColor4f(1,1,1,1);
		glTranslatef(0.1f,0.75f,0.0f);
		glScalef(0.02f,0.028f,0.1f);
		tempstring=userPrompt;
		tempstring+=userInput;
		font->glPrint("%s",tempstring.c_str());
		glLoadIdentity();
	}

	if(showClock){
		glColor4f(1,1,1,1);
		glTranslatef(0.94f,0.01f,0.0f);
		glScalef(0.015f,0.015f,0.1f);
		font->glPrint("%02i:%02i",gs->frameNum/60/30,(gs->frameNum/30)%60);
		glLoadIdentity();
	}

	if(showPlayerInfo){
		for(int a=0;a<gs->activePlayers;++a){
			if(gs->players[a]->active){
				glColor4ubv(gs->Team(gs->players[a]->team)->color);
				font->glPrintAt (0.76f, 0.01f + 0.02 * a, 0.7f, "(%i) %s %3.0f%% Ping:%d ms",a,gs->players[a]->playerName.c_str(),gs->players[a]->cpuUsage*100,(int)((gs->players[a]->ping-1)*1000/(30*gs->speedFactor)));
			}
		}
	}
	if(!hideInterface)
		info->Draw();

	if(showList)
		showList->Draw();

#ifdef NEW_GUI
	GUIcontroller::Draw();
#endif

	mouse->DrawCursor();

//	float tf[]={1,1,1,1,1,1,1,1,1};
//	glVertexPointer(3,GL_FLOAT,0,tf);
//	glDrawArrays(GL_TRIANGLES,0,3);

	glEnable(GL_DEPTH_TEST );
	glLoadIdentity();

	Uint64 start;
	start = SDL_GetTicks();
	gu->lastFrameTime = (float)(start - lastMoveUpdate)/1000.;
	lastMoveUpdate=start;

#ifndef NO_AVI
	if(creatingVideo){
		gu->lastFrameTime=1.0/GAME_SPEED;
		LPBITMAPINFOHEADER ih;
		ih=aviGenerator->GetBitmapHeader();
		unsigned char* buf=new unsigned char[ih->biWidth*ih->biHeight*3];
		glReadPixels(0,0,ih->biWidth,ih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

		aviGenerator->AddFrame(buf);

		delete buf;
//		info->AddLine("Saved avi frame size %i %i",ih->biWidth,ih->biHeight);
	}
#endif

	return true;
}

void CGame::StartPlaying()
{
	playing=true;
	lastTick=clock();
	lastframe = SDL_GetTicks();
	ENTER_MIXED;
	gu->myTeam=gs->players[gu->myPlayerNum]->team;
	gu->myAllyTeam=gs->AllyTeam(gu->myTeam);
	grouphandler->team=gu->myTeam;
	ENTER_SYNCED;
#ifdef __GNUC__
	// Send gcc version around to all players.
	char version_string_buf[128];
	SNPRINTF(version_string_buf, sizeof(version_string_buf), "gcc version: %i.%i.%i, ", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
	std::string version_string = version_string_buf;
#if defined(i386) || defined(__i386) || defined(__i386__)
	version_string += "i386";
#elif defined(amd64) || defined(__amd64) || defined(__amd64__)
	version_string += "amd64";
#elif defined(powerpc) || defined(__powerpc) || defined(__powerpc__)
	version_string += "powerpc";
#else
	version_string += "unknown architecture";
#endif
	net->SendSTLData<unsigned char, std::string>(NETMSG_CHAT, gu->myPlayerNum, version_string);
#endif // __GNUC__
}

void CGame::SimFrame()
{
	// Enable trapping of NaNs and divisions by zero to make debugging easier.
	feraiseexcept(FPU_Exceptions(FE_INVALID | FE_DIVBYZERO));
	assert(good_fpu_control_registers());

	ASSERT_SYNCED_MODE;
//	info->AddLine("New frame %i %i %i",gs->frameNum,gs->randInt(),uh->CreateChecksum());
#ifdef TRACE_SYNC
	uh->CreateChecksum();
	tracefile << "New frame:" << gs->frameNum << " " << gs->randSeed << "\n";
#endif

#ifdef USE_MMGR
	if(!(gs->frameNum & 31))
		m_validateAllAllocUnits();
#endif

	script->Update();
	gs->frameNum++;

	ENTER_UNSYNCED;
	info->Update();
	geometricObjects->Update();
	if(!(gs->frameNum & 7))
		sound->Update();
	sound->NewFrame();
	treeDrawer->Update();
	globalAI->Update();
	grouphandler->Update();
#ifdef PROFILE_TIME
	profiler.Update();
#endif
	unitDrawer->Update();
#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		unsigned char status=0;
		if(camMove[0]) status|=1;
		if(camMove[1]) status|=2;
		if(camMove[2]) status|=4;
		if(camMove[3]) status|=8;
		if(mouse->buttons[SDL_BUTTON_LEFT].pressed) status|=16;
		if(mouse->buttons[SDL_BUTTON_RIGHT].pressed) status|=32;
		shortint2 hp=GetHAndPFromVector(camera->forward);

		if(hp.x!=oldHeading || hp.y!=oldPitch || oldStatus!=status){
			oldHeading=hp.x;
			oldPitch=hp.y;
			oldStatus=status;
			net->SendData<unsigned char, unsigned char, short int, short int>(
					NETMSG_DC_UPDATE, gu->myPlayerNum, status, hp.x, hp.y);
		}
	}
#endif
	ENTER_SYNCED;
START_TIME_PROFILE

	helper->Update();
	mapDamage->Update();
	pathManager->Update();
	uh->Update();
	ph->Update();
	featureHandler->Update();
	GCobEngine.Tick(33);
	wind.Update();
	loshandler->Update();

	if(!(gs->frameNum&31)){
		for(int a=0;a<gs->activeTeams;++a)
			gs->Team(a)->Update();
	}
//	CPathFinder::Instance()->Update();

START_TIME_PROFILE
	ground->CheckCol(ph);
	ph->CheckUnitCol();
END_TIME_PROFILE("Collisions");

	Uint64 stopPhysics;
	stopPhysics = SDL_GetTicks();

END_TIME_PROFILE("Sim time")

	lastUpdate=stopPhysics;

#ifdef DIRECT_CONTROL_ALLOWED

	for(int a=0;a<gs->activePlayers;++a){
		if(!gs->players[a]->active || !gs->players[a]->playerControlledUnit)
			continue;

		CUnit* unit=gs->players[a]->playerControlledUnit;
		DirectControlStruct* dc=&gs->players[a]->myControl;

		std::vector<int> args;
		args.push_back(0);
		unit->cob->Call(COBFN_AimFromPrimary/*/COBFN_QueryPrimary+weaponNum/**/,args);
		float3 relPos=unit->localmodel->GetPiecePos(args[0]);
		float3 pos=unit->pos+unit->frontdir*relPos.z+unit->updir*relPos.y+unit->rightdir*relPos.x;
		pos+=UpVector*7;

		CUnit* hit;
		float dist=helper->TraceRayTeam(pos,dc->viewDir,unit->maxRange,hit,1,unit,gs->AllyTeam(gs->players[a]->team));
		dc->target=hit;

		if(hit){
			dc->targetDist=dist;
			dc->targetPos=hit->pos;
			if(!dc->mouse2){
				unit->AttackUnit(hit,true);
				/*					for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi)
				if((*wi)->targetType!=Target_Unit || (*wi)->targetUnit!=hit)
				(*wi)->AttackUnit(hit,true);
				*/
			}
		} else {
			if(dist>unit->maxRange*0.95)
				dist=unit->maxRange*0.95;

			dc->targetDist=dist;
			dc->targetPos=pos+dc->viewDir*dc->targetDist;

			if(!dc->mouse2){
				unit->AttackGround(dc->targetPos,true);
				for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi){
					float d=dc->targetDist;
					if(d>(*wi)->range*0.95)
						d=(*wi)->range*0.95;
					float3 p=pos+dc->viewDir*d;
					(*wi)->AttackGround(p,true);
				}
			}
		}
	}

#endif

	water->Update();

	feclearexcept(FPU_Exceptions(FE_INVALID | FE_DIVBYZERO));
}

bool CGame::ClientReadNet()
{
	static int lastMsg=0,thisMsg=0;
	int a;
/*	if(gameServer){
		inbufpos=0;
		inbuflength=0;
	}*/

	if(inbufpos>inbuflength)
		info->AddLine("To much data read");
	if(inbufpos==inbuflength){
		inbufpos=0;
		inbuflength=0;
	}
	if(inbufpos>NETWORK_BUFFER_SIZE*0.5){
		for(a=inbufpos;a<inbuflength;a++)
			inbuf[a-inbufpos]=inbuf[a];
		inbuflength-=inbufpos;
		inbufpos=0;
	}

	if((a=net->GetData(&inbuf[inbuflength],NETWORK_BUFFER_SIZE*2-inbuflength,0))==-1){
		return gameOver;
	}
	inbuflength+=a;

	if(!gameServer/* && !net->onlyLocal*/){
		Uint64 currentFrame;
		currentFrame = SDL_GetTicks();

		if(timeLeft>1)
			timeLeft--;
		timeLeft+=consumeSpeed*((float)(currentFrame - lastframe)/1000.);
		lastframe=currentFrame;

		que=0;
		for(int i2=inbufpos;i2<inbuflength;){
			switch (inbuf[i2]){
			case NETMSG_SETPLAYERNUM:
#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL:
#endif
				i2+=2;
				break;
#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DC_UPDATE:
				i2+=7;
				break;
#endif
			case NETMSG_PLAYERNAME:
			case NETMSG_CHAT:
			case NETMSG_SYSTEMMSG:
			case NETMSG_MAPNAME:
			case NETMSG_MODNAME:
			case NETMSG_SCRIPT:
			case NETMSG_MAPDRAW:
				i2+=inbuf[i2+1];
				break;
			case NETMSG_COMMAND:
			case NETMSG_SELECT:
			case NETMSG_AICOMMAND:
				i2+=(*((short int*)&inbuf[i2+1]));
				break;
			case NETMSG_NEWFRAME:
				que++;
			case NETMSG_RANDSEED:
			case NETMSG_INTERNAL_SPEED:
			case NETMSG_USER_SPEED:
			case NETMSG_CPU_USAGE:
			case NETMSG_SYNCREQUEST:
			case NETMSG_SYNCERROR:
				i2+=5;
				break;
			case NETMSG_HELLO:
			case NETMSG_QUIT:
			case NETMSG_STARTPLAYING:
			case NETMSG_MEMDUMP:
			case NETMSG_SENDPLAYERSTAT:
			case NETMSG_GAMEOVER:
				i2++;
				break;
			case NETMSG_SHARE:
				i2+=12;
				break;
			case NETMSG_SETSHARE:
			case NETMSG_PLAYERINFO:
				i2+=10;
				break;
			case NETMSG_PLAYERSTAT:
				i2+=sizeof(CPlayer::Statistics)+2;
				break;
			case NETMSG_PAUSE:
			case NETMSG_ATTEMPTCONNECT:
			case NETMSG_PLAYERLEFT:
				i2+=3;
				break;
			case NETMSG_STARTPOS:
				i2+=15;
				break;
			default:{
#ifdef SYNCDEBUG
				// maybe something for the sync debugger?
				int x = CSyncDebugger::GetInstance()->GetMessageLength(&inbuf[i2]);
				if (x) i2 += x;
				else
#endif
				{
					info->AddLine("Unknown net msg in read ahead %i",(int)inbuf[i2]);
					i2++;
				}
				break;}
			}
		}
		if(que<leastQue)
			leastQue=que;
	}
	PUSH_CODE_MODE;
	ENTER_SYNCED;
	while((inbufpos<inbuflength) && ((timeLeft>0) || gameServer/* || net->onlyLocal*/)){
		thisMsg=inbuf[inbufpos];
		int lastLength=0;
		switch (inbuf[inbufpos]){
		case NETMSG_ATTEMPTCONNECT:
			lastLength=3;
			info->AddLine("Attempted connection to client?");
			break;

		case NETMSG_HELLO:
			lastLength=1;
			break;

		case NETMSG_QUIT:
			info->AddLine("Server exited");
			net->connected=false;
			POP_CODE_MODE;
			return gameOver;

		case NETMSG_PLAYERLEFT:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in player left msg",player);
			} else {
				if(inbuf[inbufpos+2]==1)
					info->AddLine("Player %s left",gs->players[player]->playerName.c_str());
				else
					info->AddLine("Lost connection to %s",gs->players[player]->playerName.c_str());
				gs->players[player]->active=false;
			}
			lastLength=3;
			break;}

		case NETMSG_MEMDUMP:
			MakeMemDump();
			#ifdef TRACE_SYNC
				tracefile.Commit();
			#endif
			lastLength=1;
			break;

		case NETMSG_STARTPLAYING:
			StartPlaying();
			lastLength=1;
			break;

		case NETMSG_GAMEOVER:
			ENTER_MIXED;
			gameOver=true;
			if (gu->autoQuit) {
				info->AddLine("Automatical quit enforced from commandline");
				globalQuit = true;
			} else {
				#ifdef NEW_GUI
				guicontroller->Event("dialog_endgame_togglehidden");
				#else
				new CEndGameBox();
				#endif
			}
			lastLength=1;
			ENTER_SYNCED;
			break;

		case NETMSG_SENDPLAYERSTAT:
			ENTER_MIXED;
			info->AddLine("Game over");
			// Warning: using CPlayer::Statistics here may cause endianness problems
			// once net->SendData is endian aware!
			net->SendData<unsigned char, CPlayer::Statistics>(
					NETMSG_PLAYERSTAT, gu->myPlayerNum, *gs->players[gu->myPlayerNum]->currentStats);
			lastLength=1;
			ENTER_SYNCED;
			break;

		case NETMSG_PLAYERSTAT:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playerstat msg",player);
				lastLength=sizeof(CPlayer::Statistics)+2;
				break;
			}
			*gs->players[player]->currentStats=*(CPlayer::Statistics*)&inbuf[inbufpos+2];
			lastLength=sizeof(CPlayer::Statistics)+2;
			break;}

		case NETMSG_PAUSE:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in pause msg",player);
			} else {
				gs->paused=!!inbuf[inbufpos+1];
				if(gs->paused){
					info->AddLine("%s paused the game",gs->players[player]->playerName.c_str());
				} else {
					info->AddLine("%s unpaused the game",gs->players[player]->playerName.c_str());
				}
				lastframe = SDL_GetTicks();
				timeLeft=0;
			}
			lastLength=3;
			break;}

		case NETMSG_INTERNAL_SPEED:{
			if(!net->playbackDemo)
				gs->speedFactor=*((float*)&inbuf[inbufpos+1]);
			lastLength=5;
//			info->AddLine("Internal speed set to %.2f",gs->speedFactor);
			break;}

		case NETMSG_USER_SPEED:
			gs->userSpeedFactor=*((float*)&inbuf[inbufpos+1]);
			if(gs->userSpeedFactor>maxUserSpeed)
				gs->userSpeedFactor=maxUserSpeed;
			if(gs->userSpeedFactor<minUserSpeed)
				gs->userSpeedFactor=minUserSpeed;

			lastLength=5;
			info->AddLine("Speed set to %.1f",gs->userSpeedFactor);
			break;

		case NETMSG_CPU_USAGE:
			info->AddLine("Game clients shouldnt get cpu usage msgs?");
			lastLength=5;
			break;

		case NETMSG_PLAYERINFO:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playerinfo msg",player);
			} else {
				gs->players[player]->cpuUsage=*(float*)&inbuf[inbufpos+2];
				gs->players[player]->ping=*(int*)&inbuf[inbufpos+6];
			}
			lastLength=10;
			break;}

		case NETMSG_SETPLAYERNUM:
			//info->AddLine("Warning shouldnt receive NETMSG_SETPLAYERNUM in CGame");
			lastLength=2;
			break;

		case NETMSG_SCRIPT:
		case NETMSG_MAPNAME:
		case NETMSG_MODNAME:
			lastLength = inbuf[inbufpos+1];
			break;

		case NETMSG_PLAYERNAME:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playername msg",player);
			} else {
				gs->players[player]->playerName=(char*)(&inbuf[inbufpos+3]);
				gs->players[player]->readyToStart=true;
				gs->players[player]->active=true;
				wordCompletion->AddWord(gs->players[player]->playerName, false, false); // required?
			}
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in chat msg",player);
			} else {
				string s=(char*)(&inbuf[inbufpos+3]);
				HandleChatMsg(s,player);
			}
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_SYSTEMMSG:{
			string s=(char*)(&inbuf[inbufpos+3]);
			info->AddLine(s);
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_STARTPOS:{
			int team=inbuf[inbufpos+1];
			if(team>=gs->activeTeams || team<0 || !gameSetup){
				info->AddLine("Got invalid team num %i in startpos msg",team);
			} else {
				if(inbuf[inbufpos+2]!=2)
					gameSetup->readyTeams[team]=!!inbuf[inbufpos+2];
				gs->Team(team)->startPos.x=*(float*)&inbuf[inbufpos+3];
				gs->Team(team)->startPos.y=*(float*)&inbuf[inbufpos+7];
				gs->Team(team)->startPos.z=*(float*)&inbuf[inbufpos+11];
			}
			lastLength=15;
			break;}

		case NETMSG_RANDSEED:
			gs->randSeed=(*((unsigned int*)&inbuf[1]));
			lastLength=5;
			break;

		case NETMSG_NEWFRAME:
			if(!gameServer)
				timeLeft-=1;
			net->SendData<int>(NETMSG_NEWFRAME, gs->frameNum);
			SimFrame();
			lastLength=5;

			if(gameServer && serverNet && serverNet->playbackDemo)	//server doesnt update framenums automatically while playing demo
				gameServer->serverframenum=gs->frameNum;

			if(creatingVideo && net->playbackDemo){
				POP_CODE_MODE;
				return true;
			}
			break;

		case NETMSG_COMMAND:{
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in command msg",player);
			} else {
				Command c;
				c.id=*((int*)&inbuf[inbufpos+4]);
				c.options=inbuf[inbufpos+8];
				for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-9)/4);++a)
					c.params.push_back(*((float*)&inbuf[inbufpos+9+a*4]));
				selectedUnits.NetOrder(c,player);
			}
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_SELECT:{
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in netselect msg",player);
			} else {
				vector<int> selected;
				for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-4)/2);++a){
					int unitid=*((short int*)&inbuf[inbufpos+4+a*2]);
					if(unitid>=MAX_UNITS || unitid<0){
						info->AddLine("Got invalid unitid %i in netselect msg",unitid);
						lastLength=*((short int*)&inbuf[inbufpos+1]);
						break;
					}
					if(uh->units[unitid] && uh->units[unitid]->team==gs->players[player]->team)
						selected.push_back(unitid);
				}
				selectedUnits.NetSelect(selected,player);
			}
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_AICOMMAND:{
			Command c;
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in aicommand msg",player);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			int unitid=*((short int*)&inbuf[inbufpos+4]);
			if(unitid>=MAX_UNITS || unitid<0){
				info->AddLine("Got invalid unitid %i in aicommand msg",unitid);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			//if(uh->units[unitid] && uh->units[unitid]->team!=gs->players[player]->team)		//msgs from global ais can be for other teams
			//	info->AddLine("Warning player %i of team %i tried to send aiorder to unit from team %i",a,gs->players[player]->team,uh->units[unitid]->team);
			c.id=*((int*)&inbuf[inbufpos+6]);
			c.options=inbuf[inbufpos+10];
			for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-11)/4);++a)
				c.params.push_back(*((float*)&inbuf[inbufpos+11+a*4]));
			selectedUnits.AiOrder(unitid,c);
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_SYNCREQUEST:{
			ENTER_MIXED;
			int frame=*((int*)&inbuf[inbufpos+1]);
			if(frame!=gs->frameNum){
				info->AddLine("Sync request for wrong frame (%i instead of %i)", frame, gs->frameNum);
			}
			net->SendData<unsigned char, int, CChecksum>(
					NETMSG_SYNCRESPONSE, gu->myPlayerNum, gs->frameNum, uh->CreateChecksum());

			net->SendData<float>(NETMSG_CPU_USAGE,
#ifdef PROFILE_TIME
					profiler.profile["Sim time"].percent);
#else
					0.30);
#endif
			lastLength=5;
			ENTER_SYNCED;
			break;}

		case NETMSG_SYNCERROR:
			info->AddLine("Sync error");
			lastLength=5;
			break;

		case NETMSG_SHARE:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in share msg",player);
				lastLength=12;
				break;
			}
			int team1=gs->players[player]->team;
			int team2=inbuf[inbufpos+2];
			bool shareUnits=!!inbuf[inbufpos+3];
 			float metalShare=min(*(float*)&inbuf[inbufpos+4],(float)gs->Team(team1)->metal);
 			float energyShare=min(*(float*)&inbuf[inbufpos+8],(float)gs->Team(team1)->energy);

			gs->Team(team1)->metal-=metalShare;
			gs->Team(team2)->metal+=metalShare;
			gs->Team(team1)->energy-=energyShare;
			gs->Team(team2)->energy+=energyShare;

			if(shareUnits){
				for(vector<int>::iterator ui=selectedUnits.netSelected[player].begin();ui!=selectedUnits.netSelected[player].end();++ui){
					if(uh->units[*ui] && uh->units[*ui]->team==team1 && !uh->units[*ui]->beingBuilt){
						uh->units[*ui]->ChangeTeam(team2,CUnit::ChangeGiven);
					}
				}
				selectedUnits.netSelected[player].clear();
			}
			lastLength=12;
			break;}

		case NETMSG_SETSHARE:{
			int team=inbuf[inbufpos+1];
			if(team>=gs->activeTeams || team<0){
				info->AddLine("Got invalid team num %i in setshare msg",team);
			} else {
				float metalShare=*(float*)&inbuf[inbufpos+2];
				float energyShare=*(float*)&inbuf[inbufpos+6];

				gs->Team(team)->metalShare=metalShare;
				gs->Team(team)->energyShare=energyShare;
			}
			lastLength=10;
			break;}

		case NETMSG_MAPDRAW:{
			inMapDrawer->GotNetMsg(&inbuf[inbufpos]);
			lastLength=inbuf[inbufpos+1];
			break;}

#ifdef DIRECT_CONTROL_ALLOWED
		case NETMSG_DIRECT_CONTROL:{
			int player=inbuf[inbufpos+1];

			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in direct control msg",player);
				lastLength=2;
				break;
			}

			if(gs->players[player]->playerControlledUnit){
				CUnit* unit=gs->players[player]->playerControlledUnit;
				//info->AddLine("Player %s released control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());

				unit->directControl=0;
				unit->AttackUnit(0,true);
				gs->players[player]->StopControllingUnit();
			} else {
				if(!selectedUnits.netSelected[player].empty() && uh->units[selectedUnits.netSelected[player][0]] && !uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()){
					CUnit* unit=uh->units[selectedUnits.netSelected[player][0]];
					//info->AddLine("Player %s took control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());
					if(unit->directControl){
						if(player==gu->myPlayerNum)
							info->AddLine("Sorry someone already controls that unit");
					}	else {
						unit->directControl=&gs->players[player]->myControl;
						gs->players[player]->playerControlledUnit=unit;
						ENTER_UNSYNCED;
						if(player==gu->myPlayerNum){
							gu->directControl=unit;
							/* currentCamControllerNum isn't touched; it's used to
							   switch back to the old camera. */
							mouse->currentCamController=mouse->camControllers[0];	//set fps mode
							mouse->inStateTransit=true;
							mouse->transitSpeed=1;
							((CFPSController*)mouse->camControllers[0])->pos=unit->midPos;
							mouse->wasLocked = mouse->locked;
							if(!mouse->locked){
								mouse->locked=true;
								mouse->HideMouse();
							}
							selectedUnits.ClearSelected();
						}
						ENTER_SYNCED;
					}
				}
			}
			lastLength=2;
			break;}

		case NETMSG_DC_UPDATE:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in dc update msg",player);
				lastLength=7;
				break;
			}
			DirectControlStruct* dc=&gs->players[player]->myControl;
			CUnit* unit=gs->players[player]->playerControlledUnit;

			dc->forward=!!(inbuf[inbufpos+2] & 1);
			dc->back=!!(inbuf[inbufpos+2] & 2);
			dc->left=!!(inbuf[inbufpos+2] & 4);
			dc->right=!!(inbuf[inbufpos+2] & 8);
			dc->mouse1=!!(inbuf[inbufpos+2] & 16);
			bool newMouse2=!!(inbuf[inbufpos+2] & 32);
			if(!dc->mouse2 && newMouse2 && unit){
				unit->AttackUnit(0,true);
//				for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi)
//					(*wi)->HoldFire();
			}
			dc->mouse2=newMouse2;

			short int h=*((short int*)&inbuf[inbufpos+3]);
			short int p=*((short int*)&inbuf[inbufpos+5]);
			dc->viewDir=GetVectorFromHAndPExact(h,p);

			lastLength=7;
			break;}
#endif

		default:
#ifdef SYNCDEBUG
			lastLength = CSyncDebugger::GetInstance()->ClientReceived(&inbuf[inbufpos]);
			if (!lastLength)
#endif
			{
				info->AddLine("Unknown net msg in client %d last %d", (int)inbuf[inbufpos], lastMsg);
				lastLength=1;
			}
			break;
		}
		if(lastLength<=0){
			info->AddLine("Client readnet got packet type %i length %i pos %i??",thisMsg,lastLength,inbufpos);
			lastLength=0;
		}
		inbufpos+=lastLength;
		lastMsg=thisMsg;
	}
	POP_CODE_MODE;
	return true;
}


void CGame::UpdateUI()
{
	ASSERT_UNSYNCED_MODE;
	//move camera if arrow keys pressed
#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		CUnit* owner=gu->directControl;

		std::vector<int> args;
		args.push_back(0);
		owner->cob->Call(COBFN_AimFromPrimary/*/COBFN_QueryPrimary+weaponNum/**/,args);
		float3 relPos=owner->localmodel->GetPiecePos(args[0]);
		float3 pos=owner->pos+owner->frontdir*relPos.z+owner->updir*relPos.y+owner->rightdir*relPos.x;
		pos+=UpVector*7;

		((CFPSController*)mouse->camControllers[0])->pos=pos;
	} else
#endif
	{
		float cameraSpeed=1;
		if (camMove[7]){
			cameraSpeed*=0.1f;
		}
		if (camMove[6]){
			cameraSpeed*=10;
		}
		float3 movement(0,0,0);

		if (camMove[0]){
			movement.y+=gu->lastFrameTime;
			unitTracker.Disable();
		}
		if (camMove[1]){
			movement.y-=gu->lastFrameTime;
			unitTracker.Disable();
		}
		if (camMove[3]){
			movement.x+=gu->lastFrameTime;
			unitTracker.Disable();
		}
		if (camMove[2]){
			movement.x-=gu->lastFrameTime;
			unitTracker.Disable();
		}
		movement.z=cameraSpeed;
		mouse->currentCamController->KeyMove(movement);

		movement=float3(0,0,0);

    if (fullscreen || windowedEdgeMove) {
			if (mouse->lasty < 2){
				movement.y+=gu->lastFrameTime;
				unitTracker.Disable();
			}
			if (mouse->lasty > (gu->screeny - 2)){
				movement.y-=gu->lastFrameTime;
				unitTracker.Disable();
			}
			if (mouse->lastx > (gu->screenx - 2)){
				movement.x+=gu->lastFrameTime;
				unitTracker.Disable();
			}
			if (mouse->lastx < 2){
				movement.x-=gu->lastFrameTime;
				unitTracker.Disable();
			}
		}
		movement.z=cameraSpeed;
		mouse->currentCamController->ScreenEdgeMove(movement);

		if(camMove[4])
			mouse->currentCamController->MouseWheelMove(gu->lastFrameTime*200*cameraSpeed);
		if(camMove[5])
			mouse->currentCamController->MouseWheelMove(-gu->lastFrameTime*200*cameraSpeed);
	}

	if(chatting && !userWriting){
		if(userInput.size()>0){
			if(userInput.size()>250)	//avoid troubles with long lines
				userInput=userInput.substr(0,250);
			if(userInput.find(".give") == 0 && gs->cheatEnabled) {
				float dist = ground->LineGroundCol(camera->pos, camera->pos + mouse->dir * 9000);
				float3 pos = camera->pos + mouse->dir * dist;
				char buf[100];
				sprintf(buf, " @%.0f,%.0f,%.0f", pos.x, pos.y, pos.z);
				userInput += buf;
			}
			net->SendSTLData<unsigned char, std::string>(NETMSG_CHAT, gu->myPlayerNum, userInput);
			if(net->playbackDemo) {
				HandleChatMsg(userInput,gu->myPlayerNum);
			}
		  consoleHistory->AddLine(userInput);
		}
		chatting=false;
		userInput="";
	}
#ifndef NEW_GUI
	if(inMapDrawer->wantLabel && !userWriting){
		if(userInput.size()>200)	//avoid troubles with to long lines
			userInput=userInput.substr(0,200);

		inMapDrawer->CreatePoint(inMapDrawer->waitingPoint,userInput);
		inMapDrawer->wantLabel=false;
		userInput="";
		ignoreChar=0;
	}
#endif
}


/*
unsigned int crcTable[256];

unsigned int Calculate_CRC(char *first,char *last)
{  int length = ((int) (last - first));  unsigned int crc = 0xFFFFFFFF;
  for(int i=0; i<length; i++)
    {
    crc = (crc>>8) & 0x00FFFFFF ^ crcTable[(crc^first[i]) & 0xFF];
    }  return(crc);}



void InitCRCTable()
{
  unsigned int crc;
  for (int i=0; i<256; i++)
    {
    crc = i;
    for(int j=8; j>0; j--)
      {
      if (crc&1)
        {
        crc = (crc >> 1) ^ 0xEDB88320;
        }
      else
        {
        crc >>= 1;
        }
     }
     crcTable[i] = crc;
   }
  }

  */

void CGame::MakeMemDump(void)
{
	std::auto_ptr<std::ofstream> pfile(filesystem.ofstream(gameServer ? "memdump.txt" : "memdumpclient.txt"));
	std::ofstream& file(*pfile);

	if (!pfile.get())
		return;

	file << "Frame " << gs->frameNum <<"\n";
	list<CUnit*>::iterator usi;
	for(usi=uh->activeUnits.begin();usi!=uh->activeUnits.end();usi++){
		CUnit* u=*usi;
		file << "Unit " << u->id << "\n";
		file << "  xpos " << u->pos.x << " ypos " << u->pos.y << " zpos " << u->pos.z << "\n";
		file << "  heading " << u->heading << " power " << u->power << " experience " << u->experience << "\n";
		file << " health " << u->health << "\n";
	}
	Projectile_List::iterator psi;
	for(psi=ph->ps.begin();psi != ph->ps.end();++psi){
		CProjectile* p=*psi;
		file << "Projectile " << p->radius << "\n";
		file << "  xpos " << p->pos.x << " ypos " << p->pos.y << " zpos " << p->pos.z << "\n";
		file << "  xspeed " << p->speed.x << " yspeed " << p->speed.y << " zspeed " << p->speed.z << "\n";
	}
	for(int a=0;a<gs->activeTeams;++a){
		file << "Losmap for team " << a << "\n";
		for(int y=0;y<gs->mapy>>sensorHandler->losMipLevel;++y){
			file << " ";
			for(int x=0;x<gs->mapx>>sensorHandler->losMipLevel;++x){
				file << loshandler->losMap[a][y*(gs->mapx>>sensorHandler->losMipLevel)+x] << " ";
			}
			file << "\n";
		}
	}
	file.close();
	info->AddLine("Memdump finished");
}

void CGame::DrawDirectControlHud(void)
{
#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* unit=gu->directControl;
	glPushMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(0.1,0.1,0);
	glScalef(0.25,0.25,0.25);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(unit->heading*180.0/32768+180,0,0,1);

		glColor4d(0.3,0.9,0.3,0.4);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2,-0.3);
		glVertex2d(-0.2,0.3);
		glVertex2d(0,0.4);
		glVertex2d(0.2,0.3);
		glVertex2d(0.2,-0.3);
		glVertex2d(-0.2,-0.3);
		glEnd();
		glPopMatrix();
	}

	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	if(unit->moveType->useHeading){
		float scale=0.4/unit->radius;
		glScalef(scale,scale,scale);
		glRotatef(90,1,0,0);
	} else {
		float scale=0.2/unit->radius;
		glScalef(scale,scale,-scale);
		CMatrix44f m(ZeroVector,float3(camera->right.x,camera->up.x,camera->forward.x),float3(camera->right.y,camera->up.y,camera->forward.y),float3(camera->right.z,camera->up.z,camera->forward.z));
		glMultMatrixf(m.m);
	}
	glTranslatef3(-unit->pos-unit->speed*gu->timeOffset);
	glDisable(GL_BLEND);
	unitDrawer->DrawIndividual(unit);
	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(GetHeadingFromVector(camera->forward.x,camera->forward.z)*180.0/32768+180,0,0,1);
		glScalef(0.4,0.4,0.3);

		glColor4d(0.4,0.4,1.0,0.6);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2,-0.3);
		glVertex2d(-0.2,0.3);
		glVertex2d(0,0.5);
		glVertex2d(0.2,0.3);
		glVertex2d(0.2,-0.3);
		glVertex2d(-0.2,-0.3);
		glEnd();
		glPopMatrix();
	}
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(0.22,0.05,0);
	glScalef(0.03,0.03,0.03);
	glColor4d(0.2,0.8,0.2,0.6);
	font->glPrint("Health %.0f",unit->health);
	glPopMatrix();

	if(gs->players[gu->myPlayerNum]->myControl.mouse2){
		glPushMatrix();
		glTranslatef(0.02,0.34,0);
		glScalef(0.03,0.03,0.03);
		glColor4d(0.2,0.8,0.2,0.6);
		font->glPrint("Free fire mode");
		glPopMatrix();
	}
	for(int a=0;a<unit->weapons.size();++a){
		glPushMatrix();
		glTranslatef(0.02,0.3-a*0.04,0);
		glScalef(0.03,0.03,0.03);
		CWeapon* w=unit->weapons[a];
		if(w->reloadStatus>gs->frameNum){
			glColor4d(0.8,0.2,0.2,0.6);
			font->glPrint("Weapon %i: Reloading",a+1);
		} else if(!w->angleGood){
			glColor4d(0.6,0.6,0.2,0.6);
			font->glPrint("Weapon %i: Aiming",a+1);
		} else {
			glColor4d(0.2,0.8,0.2,0.6);
			font->glPrint("Weapon %i: Ready",a+1);
		}
		glPopMatrix();
	}

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	camera->Update(false);		//draw some stuff in world coordinates
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	for(int a=0;a<unit->weapons.size();++a){
		CWeapon* w=unit->weapons[a];
		if(!w){
			info->AddLine("Null weapon in vector?");
			return;
		}
		switch(a){
		case 0:
			glColor4d(0,1,0,0.7);
			break;
		case 1:
			glColor4d(1,0,0,0.7);
			break;
		default:
			glColor4d(0,0,1,0.7);
		}
		if(w->targetType!=Target_None){
			float3 pos=w->targetPos;
			float3 v1=(pos-camera->pos).Normalize();
			float3 v2=(v1.cross(UpVector)).Normalize();
			float3 v3=(v2.cross(v1)).Normalize();
			float radius=10;
			if(w->targetType==Target_Unit)
				radius=w->targetUnit->radius;

			glBegin(GL_LINE_STRIP);
			for(int b=0;b<=80;++b){
				glVertexf3(pos+(v2*sin(b*2*PI/80)+v3*cos(b*2*PI/80))*radius);
			}
			glEnd();

			if(!w->onlyForward){
				float dist=min(w->owner->directControl->targetDist,w->range*0.9f);
				pos=w->weaponPos+w->wantedDir*dist;
				v1=(pos-camera->pos).Normalize();
				v2=(v1.cross(UpVector)).Normalize();
				v3=(v2.cross(v1)).Normalize();
				radius=dist/100;

				glBegin(GL_LINE_STRIP);
				for(int b=0;b<=80;++b){
					glVertexf3(pos+(v2*sin(b*2*PI/80)+v3*cos(b*2*PI/80))*radius);
				}
				glEnd();
			}
			glBegin(GL_LINES);
			if(!w->onlyForward){
				glVertexf3(pos);
				glVertexf3(w->targetPos);

				glVertexf3(pos+(v2*sin(PI*0.25f)+v3*cos(PI*0.25f))*radius);
				glVertexf3(pos+(v2*sin(PI*1.25f)+v3*cos(PI*1.25f))*radius);

				glVertexf3(pos+(v2*sin(PI*-0.25f)+v3*cos(PI*-0.25f))*radius);
				glVertexf3(pos+(v2*sin(PI*-1.25f)+v3*cos(PI*-1.25f))*radius);
			}
			if((w->targetPos-camera->pos).Normalize().dot(camera->forward)<0.7){
				glVertexf3(w->targetPos);
				glVertexf3(camera->pos+camera->forward*100);
			}
			glEnd();
		}
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);

	glPopMatrix();
#endif
}

void CGame::HandleChatMsg(std::string s,int player)
{
	globalAI->GotChatMsg(s.c_str(),player);
	CScriptHandler::Instance().chosenScript->GotChatMsg(s, player);

	if(s.find(".cheat")==0 && player==0){
		if (gs->cheatEnabled){
			gs->cheatEnabled=false;
			info->AddLine("No more cheating");
		}else{
			gs->cheatEnabled=true;
			info->AddLine("Cheating!");
		}
	}

	if(s.find(".nocost")==0 && player==0 && gs->cheatEnabled){
		for(int i=0; i<unitDefHandler->numUnits; i++)
		{
			unitDefHandler->unitDefs[i].metalCost = 1;
			unitDefHandler->unitDefs[i].energyCost = 1;
			unitDefHandler->unitDefs[i].buildTime = 10;
			unitDefHandler->unitDefs[i].metalUpkeep = 0;
			unitDefHandler->unitDefs[i].energyUpkeep = 0;
		}
		unitDefHandler->noCost=true;
		info->AddLine("Cheating!");
	}

	if (s.find(".crash") == 0 && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}

	if (s.find(".divbyzero") == 0 && gs->cheatEnabled) {
		float a = 0;
		info->AddLine("Result: %f", 1.0/a);
	}

#ifdef SYNCDEBUG
	if (s.find(".desync") == 0 && gs->cheatEnabled) {
		for (int i = MAX_UNITS - 1; i >= 0; --i) {
			if (uh->units[i]) {
				if (player == gu->myPlayerNum) {
					++uh->units[i]->midPos.x; // and desync...
					++uh->units[i]->midPos.x;
				} else {
					// execute the same amount of flops on any other player, but don't desync (it's a NOP)...
					++uh->units[i]->midPos.x;
					--uh->units[i]->midPos.x;
				}
				break;
			}
		}
		info->AddLine("Desyncing.");
	}
	if (s.find(".fakedesync") == 0 && gs->cheatEnabled && gameServer && serverNet) {
		gameServer->fakeDesync = true;
		info->AddLine("Fake desyncing.");
	}
	if (s.find(".reset") == 0 && gs->cheatEnabled) {
		CSyncDebugger::GetInstance()->Reset();
		info->AddLine("Resetting sync debugger.");
	}
#endif

	if(s==".spectator" && (gs->cheatEnabled || net->playbackDemo)){
		gs->players[player]->spectator=true;
		if(player==gu->myPlayerNum)
			gu->spectating=true;
	}
	if(s.find(".team")==0 && (gs->cheatEnabled || net->playbackDemo)){
		int team=atoi(&s.c_str()[s.find(" ")]);
		if(team>=0 && team<gs->activeTeams){
			gs->players[player]->team=team;
			gs->players[player]->spectator=false;
			if(player==gu->myPlayerNum){
				gu->spectating=false;
				gu->myTeam=team;
				gu->myAllyTeam=gs->AllyTeam(gu->myTeam);
				grouphandler->team=gu->myTeam;
			}
		}
	}
	if(s.find(".atm")==0 && gs->cheatEnabled){
		int team=gs->players[player]->team;
		gs->Team(team)->AddMetal(1000);
		gs->Team(team)->AddEnergy(1000);
	}

	if(s.find(".give")==0 && gs->cheatEnabled){
		int team=gs->players[player]->team;
		int p1 = s.rfind(" @"), p2 = s.find(",", p1+1), p3 = s.find(",", p2+1);
		if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
			info->AddLine("Someone is spoofing invalid .give messages!");
		float3 pos(atof(&s.c_str()[p1+2]), atof(&s.c_str()[p2+1]), atof(&s.c_str()[p3+1]));
		s = s.substr(0, p1);

		if(s.find(" all")!=string::npos){
			int squareSize=(int)ceil(sqrt((float)unitDefHandler->numUnits));

			for(int a=1;a<=unitDefHandler->numUnits;++a){
				float3 pos2=float3(pos.x + (a%squareSize-squareSize/2) * 10*SQUARE_SIZE, pos.y, pos.z + (a/squareSize-squareSize/2) * 10*SQUARE_SIZE);
				unitLoader.LoadUnit(unitDefHandler->unitDefs[a].name,pos2,team,false);
			}
		} else if (((s.rfind(" "))!=string::npos) && ((s.length() - s.rfind(" ") -1)>0)){
			string unitName=s.substr(s.rfind(" ")+1,s.length() - s.rfind(" ") -1);
			string tempUnitName=s.substr(s.find(" "),s.rfind(" ")+1 - s.find(" "));
			if(tempUnitName.find_first_not_of(" ")!=string::npos)
				tempUnitName=tempUnitName.substr(tempUnitName.find_first_not_of(" "),tempUnitName.find_last_not_of(" ") +1 - tempUnitName.find_first_not_of(" "));
			bool createNano=(tempUnitName.find("nano")!=string::npos);

			int numUnits=1;
			if(tempUnitName.find_first_of("0123456789")!=string::npos){
				tempUnitName=tempUnitName.substr(tempUnitName.find_first_of("0123456789"),tempUnitName.find_last_of("0123456789") +1 -tempUnitName.find_first_of("0123456789"));
				numUnits = atoi(tempUnitName.c_str());
			}
			if (unitDefHandler->GetUnitByName(unitName)!=0)   {

				UnitDef *unitDef= unitDefHandler->GetUnitByName(unitName);
				int xsize=unitDef->xsize;
				int zsize=unitDef->ysize;
				int total=numUnits;
				int squareSize=(int)ceil(sqrt((float)numUnits));
				float3 minpos=pos;
				minpos.x-=((squareSize-1)*xsize*SQUARE_SIZE)/2;
				minpos.z-=((squareSize-1)*zsize*SQUARE_SIZE)/2;
				for(int z=0;z<squareSize;++z){
					for(int x=0;x<squareSize && total>0;++x){
						unitLoader.LoadUnit(unitName,float3(minpos.x + x * xsize*SQUARE_SIZE, minpos.y, minpos.z + z * zsize*SQUARE_SIZE),team,createNano);
						--total;
					}
				}
				info->AddLine("Giving %i %s to team %i",numUnits,unitName.c_str(),team);
			}else{
				info->AddLine(unitName+" is not a valid unitname");
			}
		}
	}

	if(s.find(".take")==0){
		int sendTeam=gs->players[player]->team;
		for(int a=0;a<gs->activeTeams;++a){
			if(gs->AlliedTeams(a,sendTeam)){
				bool hasPlayer=false;
				for(int b=0;b<gs->activePlayers;++b){
					if(gs->players[b]->active && gs->players[b]->team==a && !gs->players[b]->spectator)
						hasPlayer=true;
				}
				if(!hasPlayer){
					for(std::list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
						CUnit* unit=*ui;
						if(unit->team == a && unit->selfDCountdown == 0)
							unit->ChangeTeam(sendTeam,CUnit::ChangeGiven);
					}
				}
			}
		}
	}

	if(s.find(".kick")==0 && player==0 && gameServer && serverNet){
		string name=s.substr(6,string::npos);
		if(!name.empty()){
			StringToLowerInPlace(name);

			for(int a=1;a<gs->activePlayers;++a){
				if(gs->players[a]->active){
					string p = StringToLower(gs->players[a]->playerName);

					if(p.find(name)==0){			//can kick on substrings of name
						unsigned char c=NETMSG_QUIT;
						serverNet->SendData(&c,1,a);
						serverNet->FlushConnection(a);

						serverNet->connections[a].readyData[0]=NETMSG_QUIT;		//this will rather ungracefully close the connection from our side
						serverNet->connections[a].readyLength=1;							//so if the above was lost in packetlos etc it will never be resent
					}
				}
			}
		}
	}

	if(s.find(".nospectatorchat")==0 && player==0){
		noSpectatorChat=!noSpectatorChat;
		info->AddLine("No spectator chat %i",noSpectatorChat);
	}

	if(s.find(".bind")==0 && player==gu->myPlayerNum){
		string::size_type startKey = s.find_first_not_of(' ', 5);
		if (startKey == string::npos) return;
		string::size_type endKey = s.find_first_of(' ', startKey);
		if (endKey == string::npos) return;
		string::size_type startAction = s.find_first_not_of(' ', endKey);
		if (startAction == string::npos) return;
		string::size_type endAction = s.find_first_of(' ', startAction);
		if (endAction != string::npos) return;
		const string key = s.substr(startKey, endKey - startKey);
		const string action = s.substr(startAction, endAction - startAction);
		if (keyBindings->Bind(key, action, "")) {
			info->AddLine("Bound key(%s) to action(%s)", key.c_str(), action.c_str());
		}
	}
	if(s.find(".clock")==0 && player==gu->myPlayerNum){
		showClock=!showClock;
	}
	if(s.find(".info")==0 && player==gu->myPlayerNum){
		showPlayerInfo=!showPlayerInfo;
	}
	if(s.find(".nopause")==0 && player==0){
		gamePausable=!gamePausable;
	}
	if(s.find(".setmaxspeed")==0 && player==0){
		maxUserSpeed=atof(s.substr(12).c_str());
		if(gs->userSpeedFactor>maxUserSpeed)
			gs->userSpeedFactor=maxUserSpeed;
	}
	if(s.find(".setminspeed")==0 && player==0){
		minUserSpeed=atof(s.substr(12).c_str());
		if(gs->userSpeedFactor<minUserSpeed)
			gs->userSpeedFactor=minUserSpeed;
	}

	if(noSpectatorChat && gs->players[player]->spectator && !gu->spectating)
		return;

	if((s[0]=='a' || s[0]=='A') && s[1]==':'){
		if(player==gu->myPlayerNum)
			userInputPrefix="a:";

		if(((gs->Ally(gs->AllyTeam(gs->players[inbuf[inbufpos+2]]->team),gu->myAllyTeam) && !gs->players[player]->spectator) || gu->spectating) && s.substr(2,255).length() > 0) {
			if(gs->players[player]->spectator)
				s="["+gs->players[player]->playerName+"] Allies: "+s.substr(2,255);
			else
				s="<"+gs->players[player]->playerName+"> Allies: "+s.substr(2,255);
			info->AddLine(s);
			sound->PlaySound(chatSound,5);
		}
	} else if((s[0]=='s' || s[0]=='S') && s[1]==':'){
		if(player==gu->myPlayerNum)
			userInputPrefix="s:";

		if((gu->spectating || gu->myPlayerNum == player) && s.substr(2,255).length() > 0) {
			if(gs->players[player]->spectator)
				s="["+gs->players[player]->playerName+"] Spectators: "+s.substr(2,255);
			else
				s="<"+gs->players[player]->playerName+"> Spectators: "+s.substr(2,255);
			info->AddLine(s);
			sound->PlaySound(chatSound,5);
		}
	} else {
		if(player==gu->myPlayerNum)
			userInputPrefix="";

		if(gs->players[player]->spectator)
			s="["+gs->players[player]->playerName+"] "+s;
		else
			s="<"+gs->players[player]->playerName+"> "+s;

		info->AddLine(s);
		sound->PlaySound(chatSound,5);
	}
}

unsigned  int CGame::CreateExeChecksum(void)
{
/*	unsigned int ret=0;
	CFileHandler f(
#ifdef _WIN32
			"spring.exe"
#else
			"spring"
#endif
			);
	int l=f.FileSize();
	unsigned char* buf=new unsigned char[l];
	f.Read(buf,l);

	for(int a=0;a<l;++a){
		ret+=buf[a];
		ret*=max((unsigned char)1,buf[a]);
	}

	delete[] buf;
	return ret;*/
	return 0;
}
