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
#include "PlayerRoster.h"
#include "Sync/SyncTracer.h"
#include "Team.h"
#include "TimeProfiler.h"
#include "WaitCommandsAI.h"
#include "WordCompletion.h"
#ifdef _WIN32
#include "winerror.h"
#endif
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/GroupHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
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
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "StartScripts/Script.h"
#include "StartScripts/ScriptHandler.h"
#include "Sync/SyncedPrimitiveIO.h"
#include "Sound.h"
#include "Platform/Clipboard.h"
#include "UI/CommandColors.h"
#include "UI/CursorIcons.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/KeyBindings.h"
#include "UI/KeyCodes.h"
#include "UI/MiniMap.h"
#include "UI/MouseHandler.h"
#include "UI/OutlineFont.h"
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/SimpleParser.h"
#include "UI/TooltipConsole.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ExplosionGenerator.h"

#ifndef NO_AVI
#include "Platform/Win/AVIGenerator.h"
#endif

#ifdef DIRECT_CONTROL_ALLOWED
#include "myMath.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Weapons/Weapon.h"
#endif

#include "mmgr.h"

GLfloat LightDiffuseLand[] = { 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat LightAmbientLand[] = { 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat FogLand[] =          { 0.7f, 0.7f, 0.8f, 0.0f };
GLfloat FogWhite[] =         { 1.0f, 1.0f, 1.0f, 0.0f };
GLfloat FogBlack[] =         { 0.0f, 0.0f, 0.0f, 0.0f };

extern Uint8 *keys;
extern bool globalQuit;
extern bool fullscreen;
extern string stupidGlobalMapname;


CGame* game = 0;


static void SelectUnits(const string& line);    // FIXME -- move into class
static void SelectCycle(const string& command); // FIXME -- move into class


CGame::CGame(bool server,std::string mapname, std::string modName, CInfoConsole *ic)
{
	infoConsole = ic;

	script = NULL;

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
	windowedEdgeMove=!!configHandler.GetInt("WindowedEdgeMove", 1);
	showClock=!!configHandler.GetInt("ShowClock", 1);
	playerRoster.SetSortTypeByCode(
	  (PlayerRoster::SortType)configHandler.GetInt("ShowPlayerInfo", 1));

	gamePausable=true;
	noSpectatorChat=false;

	inbufpos=0;
	inbuflength=0;

	chatting=false;
	userInput="";
	userPrompt="";

	consoleHistory = SAFE_NEW CConsoleHistory;
	wordCompletion = SAFE_NEW CWordCompletion;
	for (int pp = 0; pp < MAX_PLAYERS; pp++) {
	  wordCompletion->AddWord(gs->players[pp]->playerName, false, false, false);
	}

#ifdef DIRECT_CONTROL_ALLOWED
	oldPitch=0;
	oldHeading=0;
	oldStatus=255;
#endif

	ENTER_UNSYNCED;
	sound=CSound::GetSoundSystem();
	gameSoundVolume=configHandler.GetInt("SoundVolume", 60)*0.01f;
	soundEnabled=true;
	sound->SetVolume(gameSoundVolume);
	sound->SetUnitReplyVolume (configHandler.GetInt ("UnitReplySoundVolume",80)*0.01f);

	camera=SAFE_NEW CCamera();
	cam2=SAFE_NEW CCamera();
	mouse=SAFE_NEW CMouseHandler();
	selectionKeys=SAFE_NEW CSelectionKeyHandler();
	tooltip=SAFE_NEW CTooltipConsole();

	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load
	helper=SAFE_NEW CGameHelper(this);
	//	physicsEngine = SAFE_NEW CPhysicsEngine();
	ENTER_SYNCED;
	explGenHandler = SAFE_NEW CExplosionGeneratorHandler();
	ENTER_UNSYNCED;
	shadowHandler=SAFE_NEW CShadowHandler();

	modInfo=SAFE_NEW CModInfo(modName.c_str());

	ENTER_SYNCED;
	ground=SAFE_NEW CGround();
	readmap = CReadMap::LoadMap (mapname);
	wind.LoadWind();
	moveinfo=SAFE_NEW CMoveInfo();
	groundDecals=SAFE_NEW CGroundDecalHandler();

	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load

	ENTER_UNSYNCED;
#ifndef NEW_GUI
	guihandler=SAFE_NEW CGuiHandler();
	minimap=SAFE_NEW CMiniMap();
#endif

	ENTER_MIXED;
	ph=SAFE_NEW CProjectileHandler();

	ENTER_SYNCED;
	sensorHandler=SAFE_NEW CSensorHandler();
	damageArrayHandler=SAFE_NEW CDamageArrayHandler();
	unitDefHandler=SAFE_NEW CUnitDefHandler();

	ENTER_UNSYNCED;
	inMapDrawer=SAFE_NEW CInMapDraw();
	cmdColors.LoadConfig("cmdcolors.txt");

	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
	  wordCompletion->AddWord(uit->first, false, true, false);
	}

	geometricObjects=SAFE_NEW CGeometricObjects();

	ENTER_SYNCED;
	qf=SAFE_NEW CQuadField();

	ENTER_MIXED;
	featureHandler=SAFE_NEW CFeatureHandler();

	ENTER_SYNCED;
	mapDamage=IMapDamage::GetMapDamage();
	loshandler=SAFE_NEW CLosHandler();
	radarhandler=SAFE_NEW CRadarHandler(false);
	if(!server) net->Update();	//prevent timing out during load

	ENTER_MIXED;
	uh=SAFE_NEW CUnitHandler();
	iconHandler=SAFE_NEW CIconHandler();
	unitDrawer=SAFE_NEW CUnitDrawer();
	fartextureHandler = SAFE_NEW CFartextureHandler();
	if(!server) net->Update();	//prevent timing out during load
	modelParser = SAFE_NEW C3DModelParser();

 	ENTER_SYNCED;
 	if(!server) net->Update();	//prevent timing out during load
 	featureHandler->LoadFeaturesFromMap(CScriptHandler::Instance().chosenScript->loadGame);
 	if(!server) net->Update();	//prevent timing out during load
 	pathManager = SAFE_NEW CPathManager();
 	if(!server) net->Update();	//prevent timing out during load

	ENTER_UNSYNCED;
	sky=CBaseSky::GetSky();
#ifndef NEW_GUI
	resourceBar=SAFE_NEW CResourceBar();
	keyCodes=SAFE_NEW CKeyCodes();
	keyBindings=SAFE_NEW CKeyBindings();
	keyBindings->Load("uikeys.txt");
#endif
	if(!server) net->Update();	//prevent timing out during load

	water=CBaseWater::GetWater();
	grouphandler=SAFE_NEW CGroupHandler(gu->myTeam);
	globalAI=SAFE_NEW CGlobalAIHandler();

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

	logOutput.Print("TA Spring %s",VERSION_STRING);

	if(!server)
		net->SendData<unsigned int>(NETMSG_EXECHECKSUM, CreateExeChecksum());

	if (gameSetup) {
		maxUserSpeed = gameSetup->maxSpeed;
		minUserSpeed = gameSetup->minSpeed;
	}
	else {
		maxUserSpeed=3;
		minUserSpeed=0.3f;
	}

	// make sure initial game speed is within allowed range
	if(gs->userSpeedFactor>maxUserSpeed)
		gs->userSpeedFactor=maxUserSpeed;
	if(gs->userSpeedFactor<minUserSpeed)
		gs->userSpeedFactor=minUserSpeed;

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
	glFogf(GL_FOG_END,gu->viewRange*0.98f);
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

	chatSound=sound->GetWaveId("sounds/beep4.wav");

	if(gameServer)
		gameServer->gameLoading=false;

	UnloadStartPicture();

	if(serverNet && serverNet->playbackDemo)
		serverNet->StartDemoServer();
}


CGame::~CGame()
{
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
	delete infoConsole;
	delete consoleHistory;
	delete wordCompletion;
	delete explGenHandler;
	CColorMap::DeleteColormaps();
}


void CGame::ResizeEvent()
{
	if (minimap != NULL) {
		minimap->UpdateGeometry();
	}
}


//called when the key is pressed by the user (can be called several times due to key repeat)
int CGame::KeyPressed(unsigned short k, bool isRepeat)
{
	if(!gameOver && !isRepeat)
		gs->players[gu->myPlayerNum]->currentStats->keyPresses++;
	//	logOutput.Print("%i",(int)k);

	if (!hotBinding.empty()) {
		if (k == 27) {
			hotBinding.clear();
		}
		else if (!keyCodes->IsModifier(k) && (k != keyBindings->GetFakeMetaKey())) {
			CKeySet ks(k, false);
			string cmd = "bind";
			cmd += " " + ks.GetString(false) ;
			cmd += " " + hotBinding;
			keyBindings->Command(cmd);
			hotBinding.clear();
			logOutput.Print("%s", cmd.c_str());
		}
		return 0;
	}

	// Get the list of possible key actions
	CKeySet ks(k, false);
	const CKeyBindings::ActionList& actionList = keyBindings->GetActionList(ks);
	
	if (userWriting){

		// convert to the appropriate prefix if we receive a chat switch action
		for (int i = 0; i < (int)actionList.size(); i++) {
			const CKeyBindings::Action& action = actionList[i];
			if (action.command == "chatswitchall") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput = userInput.substr(2);
				}
				userInputPrefix = "";
				return 0;
			}
			else if (action.command == "chatswitchally") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 'a';
				} else {
					userInput = "a:" + userInput;
				}
				userInputPrefix = "a:";
				return 0;
			}
			else if (action.command == "chatswitchspec") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 's';
				} else {
					userInput = "s:" + userInput;
				}
				userInputPrefix = "s:";
				return 0;
			}
			else if (action.command == "pastetext") {
				if (!action.extra.empty()) {
					userInput += action.extra;
				} else {
					CClipboard clipboard;
					userInput += clipboard.GetContents();
				}
				return 0;
			}
		}

		if(k==8){ //backspace
			if(userInput.size()!=0){
				userInput.erase(userInput.size()-1,1);
			}
		}
		else if(k==SDLK_RETURN){
			userWriting=false;
			keys[k] = false;		//prevent game start when server chats
			if (chatting) {
				string command;
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					command = userInput.substr(2);
				} else {
					command = userInput;
				}
				if (command[0] == '/') {
					consoleHistory->AddLine(command);
					const string actionLine = command.substr(1); // strip the '/' or '.'
					chatting = false;
					userInput = "";
					logOutput.Print(command);
					CKeySet ks(k, false);
					CKeyBindings::Action fakeAction(actionLine);
					ActionPressed(fakeAction, ks, isRepeat);
				}
			}
		}
		else if(k==27 && (chatting || inMapDrawer->wantLabel)){ // escape
			if (chatting) {
				consoleHistory->AddLine(userInput);
			}
			userWriting=false;
			chatting=false;
			inMapDrawer->wantLabel=false;
			userInput="";
		}
		else if(k==SDLK_UP && chatting) {
			userInput = consoleHistory->PrevLine(userInput);
		}
		else if(k==SDLK_DOWN && chatting) {
			userInput = consoleHistory->NextLine(userInput);
		}
		else if(k==SDLK_TAB) {
			vector<string> partials = wordCompletion->Complete(userInput);
			if (!partials.empty()) {
				string msg;
				for (int i = 0; i < partials.size(); i++) {
					msg += "  ";
					msg += partials[i];
				}
				logOutput.Print(msg);
			}
		}
		return 0;
	}

	// try the input receivers
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri) && (*ri)->KeyPressed(k, isRepeat)) {
			return 0;
		}
	}

	// try our list of actions
	for (int i = 0; i < (int)actionList.size(); ++i) {
		if (ActionPressed(actionList[i], ks, isRepeat)) {
			return 0;
		}
	}

	return 0;
}


//Called when a key is released by the user
int CGame::KeyReleased(unsigned short k)
{
	//	keys[k] = false;

	if ((userWriting) && (((k>=' ') && (k<='Z')) || (k==8) || (k==190) )){
		return 0;
	}

	// try the input receivers
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri) && (*ri)->KeyReleased(k))
			return 0;
	}

	// try our list of actions
	CKeySet ks(k, true);
	const CKeyBindings::ActionList& al = keyBindings->GetActionList(ks);
	for (int i = 0; i < (int)al.size(); ++i) {
		if (ActionReleased(al[i])) {
			return 0;
		}
	}


	return 0;
}


bool CGame::ActionPressed(const CKeyBindings::Action& action,
                          const CKeySet& ks, bool isRepeat)
{
	// we may need these later
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();

	const string& cmd = action.command;
	
	// process the action
	if (cmd == "select") {
		selectionKeys->DoSelection(action.extra);
	}
	else if (cmd == "selectunits") {
		SelectUnits(action.extra);
	}
	else if (cmd == "selectcycle") {
		SelectCycle(action.extra);
	}
	else if (cmd == "deselect") {
		selectedUnits.ClearSelected();
	}
	else if (cmd == "shadows") {
		const int current = configHandler.GetInt("Shadows", 0);
		if (current < 0) {
			logOutput.Print("Shadows have been disabled with %i", current);
			logOutput.Print("Change your configuration and restart to use them");
			return true;
		}
		else if (!shadowHandler->canUseShadows) {
			logOutput.Print("Your hardware/driver setup does not support shadows");
			return true;
		}

		delete shadowHandler;
		int next = 0;
		if (!action.extra.empty()) {
			next = atoi(action.extra.c_str());
		} else {
			next = (current == 0) ? 1 : 0;
		}
		configHandler.SetInt("Shadows", next);
		logOutput.Print("Set Shadows to %i", next);
		shadowHandler = SAFE_NEW CShadowHandler();
	}
	else if (cmd == "water") {
		delete water;
		int next = 0;
		if (!action.extra.empty()) {
			next = atoi(action.extra.c_str());
		} else {
			const int current = configHandler.GetInt("ReflectiveWater", 1);
			next = (max(0, current) + 1) % 4;
		}
		configHandler.SetInt("ReflectiveWater", next);
		logOutput.Print("Set ReflectiveWater to %i", next);
		water = CBaseWater::GetWater();
	}
	else if (cmd == "say") {
		SendNetChat(action.extra);
	}
	else if (cmd == "echo") {
		logOutput.Print(action.extra);
	}
	else if (cmd == "drawinmap") {
		inMapDrawer->keyPressed=true;
	}
	else if (cmd == "drawlabel") {
		float3 pos = inMapDrawer->GetMouseMapPos();
		if (pos.x >= 0) {
			inMapDrawer->keyPressed = false;
			inMapDrawer->PromptLabel(pos);
			if ((ks.Key() >= SDLK_SPACE) && (ks.Key() <= SDLK_DELETE)) {
				ignoreNextChar=true;
			}
		}
	}
	else if (!isRepeat && cmd == "mouse1") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 1);
	}
	else if (!isRepeat && cmd == "mouse2") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 2);
	}
	else if (!isRepeat && cmd == "mouse3") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 3);
	}
	else if (!isRepeat && cmd == "mouse4") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 4);
	}
	else if (!isRepeat && cmd == "mouse5") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 5);
	}
	else if (cmd == "viewfps") {
		mouse->SetCameraMode(0);
	}
	else if (cmd == "viewta") {
		mouse->SetCameraMode(1);
	}
	else if (cmd == "viewtw") {
		mouse->SetCameraMode(2);
	}
	else if (cmd == "viewrot") {
		mouse->SetCameraMode(3);
	}
	else if (cmd == "viewsave") {
		mouse->SaveView(action.extra);
	}
	else if (cmd == "viewload") {
		mouse->LoadView(action.extra);
	}
	else if (cmd == "viewselection") {
		const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
		if (!selUnits.empty()) {
			float3 pos(0.0f, 0.0f, 0.0f);
			set<CUnit*>::const_iterator it;
			for (it = selUnits.begin(); it != selUnits.end(); it++) {
				pos += (*it)->midPos;
			}
			pos /= (float)selUnits.size();
			mouse->currentCamController->SetPos(pos);
		}
	}
	else if (cmd == "moveforward") {
		camMove[0]=true;
	}
	else if (cmd == "moveback") {
		camMove[1]=true;
	}
	else if (cmd == "moveleft") {
		camMove[2]=true;
	}
	else if (cmd == "moveright") {
		camMove[3]=true;
	}
	else if (cmd == "moveup") {
		camMove[4]=true;
	}
	else if (cmd == "movedown") {
		camMove[5]=true;
	}
	else if (cmd == "movefast") {
		camMove[6]=true;
	}
	else if (cmd == "moveslow") {
		camMove[7]=true;
	}
	else if ((cmd == "specteam") && (gu->spectating)) {
		const int team = atoi(action.extra.c_str());
		if ((team >= 0) && (team < gs->activeTeams)) {
			gu->myTeam = team;
			gu->myAllyTeam = gs->AllyTeam(team);
		}
	}
	else if ((cmd == "specfullview") && (gu->spectating)) {
		if (!action.extra.empty()) {
			gu->spectatingFullView = atoi(action.extra.c_str());
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
		}
	}
	else if (cmd == "group") {
		const char* c = action.extra.c_str();
		const int t = c[0];
		if ((t >= '0') && (t <= '9')) {
			const int team = (t - '0');
			do { c++; } while ((c[0] != 0) && isspace(c[0]));
			grouphandler->GroupCommand(team, c);
		}
	}
	else if (cmd == "group0") {
		grouphandler->GroupCommand(0);
	}
	else if (cmd == "group1") {
		grouphandler->GroupCommand(1);
	}
	else if (cmd == "group2") {
		grouphandler->GroupCommand(2);
	}
	else if (cmd == "group3") {
		grouphandler->GroupCommand(3);
	}
	else if (cmd == "group4") {
		grouphandler->GroupCommand(4);
	}
	else if (cmd == "group5") {
		grouphandler->GroupCommand(5);
	}
	else if (cmd == "group6") {
		grouphandler->GroupCommand(6);
	}
	else if (cmd == "group7") {
		grouphandler->GroupCommand(7);
	}
	else if (cmd == "group8") {
		grouphandler->GroupCommand(8);
	}
	else if (cmd == "group9") {
		grouphandler->GroupCommand(9);
	}
	else if (cmd == "lastmsgpos") {
		mouse->currentCamController->SetPos(infoConsole->lastMsgPos);
		mouse->CameraTransition(0.6f);
	}
	else if (((cmd == "chat")     || (cmd == "chatall") ||
	         (cmd == "chatally") || (cmd == "chatspec")) &&
	         // if chat is bound to enter and we're waiting for user to press enter to start game, ignore.
	         (ks.Key() != SDLK_RETURN || !(gameServer && serverNet->waitOnCon && allReady))) {
		if (cmd == "chatall")  { userInputPrefix = ""; }
		if (cmd == "chatally") { userInputPrefix = "a:"; }
		if (cmd == "chatspec") { userInputPrefix = "s:"; }
		userWriting = true;
		userPrompt = "Say: ";
		userInput = userInputPrefix;
		chatting = true;
		if (ks.Key()!=SDLK_RETURN) {
			ignoreNextChar = true;
		}
		consoleHistory->ResetPosition();
	}
	else if (cmd == "track") {
		unitTracker.Track();
	}
	else if (cmd == "trackoff") {
		unitTracker.Disable();
	}
	else if (cmd == "trackmode") {
		unitTracker.IncMode();
	}
	else if (cmd == "toggleoverview") {
		mouse->ToggleOverviewCamera();
	}
	else if (cmd == "showhealthbars") {
		unitDrawer->showHealthBars=!unitDrawer->showHealthBars;
	}
	else if (cmd == "pause") {
		if(net->playbackDemo){
			gs->paused=!gs->paused;
		} else {
			net->SendData<unsigned char, unsigned char>(
					NETMSG_PAUSE, !gs->paused, gu->myPlayerNum);
			lastframe = SDL_GetTicks();
		}
	}
	else if (cmd == "singlestep") {
		bOneStep=true;
	}
	else if (cmd == "debug") {
		gu->drawdebug=!gu->drawdebug;
	}
	else if (cmd == "nosound") {
		soundEnabled=!soundEnabled;
		sound->SetVolume (soundEnabled ? gameSoundVolume : 0.0f);
	}
	else if (cmd == "savegame"){
		CLoadSaveHandler ls;
		ls.mapName = stupidGlobalMapname;
		ls.modName = modInfo->name;
		ls.SaveGame("Test.ssf");
	}

#ifndef NO_AVI
	else if (cmd == "createvideo") {
		if(creatingVideo){
			creatingVideo=false;
			aviGenerator->ReleaseEngine();
			delete aviGenerator;
			aviGenerator=0;
			//			logOutput.Print("Finished avi");
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
			int x=gu->viewSizeX;
			x-=gu->viewSizeX%4;

			int y=gu->viewSizeY;
			y-=gu->viewSizeY%4;

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

			aviGenerator=SAFE_NEW CAVIGenerator();
			aviGenerator->SetFileName(name.c_str());
			aviGenerator->SetRate(30);
			aviGenerator->SetBitmapHeader(&bih);
			Sint32 hr=aviGenerator->InitEngine();
			if(hr!=AVIERR_OK){
				creatingVideo=false;
			} else {
				logOutput.Print("Recording avi to %s size %i %i",name.c_str(),x,y);
			}
		}
	}
#endif

	else if (cmd == "updatefov") {
		gd->updateFov=!gd->updateFov;
	}
	else if (cmd == "drawtrees") {
		treeDrawer->drawTrees=!treeDrawer->drawTrees;
	}
	else if (cmd == "dynamicsky") {
		sky->dynamicSky=!sky->dynamicSky;
	}
	else if (!isRepeat && (cmd == "gameinfo")) {
		if (!CGameInfo::IsActive()) {
			CGameInfo::Enable();
		} else {
			CGameInfo::Disable();
		}
	}
	else if (cmd == "hideinterface") {
		hideInterface=!hideInterface;
	}
	else if (cmd == "increaseviewradius") {
		gd->IncreaseDetail();
	}
	else if (cmd == "decreaseviewradius") {
		gd->DecreaseDetail();
	}
	else if (cmd == "moretrees") {
		treeDrawer->baseTreeDistance+=0.2f;
		logOutput << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "lesstrees") {
		treeDrawer->baseTreeDistance-=0.2f;
		logOutput << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "moreclouds") {
		sky->cloudDensity*=0.95f;
		logOutput << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (cmd == "lessclouds") {
		sky->cloudDensity*=1.05f;
		logOutput << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (cmd == "speedup") {
		float speed=gs->userSpeedFactor;
		if (speed<1) {
			speed/=0.8f;
			if (speed>0.99f) {
				speed=1;
			}
		} else {
			speed+=0.5f;
		}
		if (!net->playbackDemo) {
			net->SendData<float>(NETMSG_USER_SPEED, speed);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			logOutput << "Speed " << gs->speedFactor << "\n";
		}
	}
	else if (cmd == "slowdown") {
		float speed=gs->userSpeedFactor;
		if (speed<=1) {
			speed*=0.8f;
			if (speed<0.1f) {
				speed=0.1f;
			}
		} else {
			speed-=0.5f;
		}
		if (!net->playbackDemo) {
			net->SendData<float>(NETMSG_USER_SPEED, speed);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			logOutput << "Speed " << gs->speedFactor << "\n";
		}
	}

#ifdef DIRECT_CONTROL_ALLOWED
	else if (cmd == "controlunit") {
		Command c;
		c.id=CMD_STOP;
		c.options=0;
		selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que
		net->SendData<unsigned char>(NETMSG_DIRECT_CONTROL, gu->myPlayerNum);
	}
#endif

	else if (cmd == "showshadowmap") {
		shadowHandler->showShadowMap=!shadowHandler->showShadowMap;
	}
	else if (cmd == "showstandard") {
		gd->DisableExtraTexture();
	}
	else if (cmd == "showelevation") {
		gd->SetHeightTexture();
	}
	else if (cmd == "toggleradarandjammer"){
		gd->ToggleRadarAndJammer();
	}
	else if (cmd == "showmetalmap") {
		gd->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
	}
	else if (cmd == "showpathmap") {
		gd->SetPathMapTexture();
	}
	else if (cmd == "yardmap4") {
		//		groundDrawer->SetExtraTexture(readmap->yardmapLevels[3],readmap->yardmapPal,true);
	}
	/*	if (cmd == "showsupply"){
		groundDrawer->SetExtraTexture(supplyhandler->supplyLevel[gu->myTeam],supplyhandler->supplyPal);
		}*/
	/*	if (cmd == "showteam"){
		groundDrawer->SetExtraTexture(readmap->teammap,cityhandler->teampal);
		}*/
	else if (cmd == "togglelos") {
		gd->ToggleLosTexture();
	}
	else if (cmd == "sharedialog") {
		if(!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front())==0 && !gu->spectating)
			SAFE_NEW CShareBox();
	}
	else if (cmd == "quitwarn") {
		const CKeyBindings::HotkeyList hkl = keyBindings->GetHotkeys("quit");
		if (hkl.empty()) {
			logOutput.Print("How odd, you appear to be lacking a \"quit\" binding");
		} else {
			logOutput.Print("Use %s to quit", hkl[0].c_str());
		}
	}
	else if (cmd == "quit") {
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
				SAFE_NEW CQuitBox();
			}
		} else {
			logOutput.Print("User exited");
			globalQuit=true;
		}
	}
	else if (cmd == "incguiopacity") {
		CInputReceiver::guiAlpha = min(CInputReceiver::guiAlpha+0.1f,1.0f);
	}
	else if (cmd == "decguiopacity") {
		CInputReceiver::guiAlpha = max(CInputReceiver::guiAlpha-0.1f,0.0f);
	}
	else if (cmd == "screenshot") {
		const char* ext = "jpg";
		if (action.extra == "png") {
			ext = "png";
		}
		if (filesystem.CreateDirectory("screenshots")) {
			int x=gu->viewSizeX;
			if(x%4)
				x+=4-x%4;
			unsigned char* buf=SAFE_NEW unsigned char[x*gu->viewSizeY*4];
			glReadPixels(0,0,x,gu->viewSizeY,GL_RGBA,GL_UNSIGNED_BYTE,buf);
			CBitmap b(buf,x,gu->viewSizeY);
			b.ReverseYAxis();
			char t[50];
			for(int a=0;a<9999;++a){
				sprintf(t,"screenshots/screen%03i.%s", a, ext);
				CFileHandler ifs(t);
				if(!ifs.FileExists())
					break;
			}
			b.Save(t);
			logOutput.Print("Saved: %s", t);
			delete[] buf;
		}
	}
	else if (cmd == "grabinput") {
		SDL_GrabMode mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
		switch (mode) {
			case SDL_GRAB_ON: mode = SDL_GRAB_OFF; break;
			case SDL_GRAB_OFF: mode = SDL_GRAB_ON; break;
		}
		SDL_WM_GrabInput(mode);
	}
	else if ((cmd == "bind")         ||
	         (cmd == "unbind")       ||
	         (cmd == "unbindall")    ||
	         (cmd == "unbindkeyset") ||
	         (cmd == "unbindaction") ||
	         (cmd == "keydebug")     ||
	         (cmd == "fakemeta")) {
		keyBindings->Command(action.rawline);
	}
	else if (cmd == "keyload") {
		keyBindings->Load("uikeys.txt");
	}
	else if (cmd == "keyreload") {
		keyBindings->Command("unbindall");
		keyBindings->Command("unbind enter chat");
		keyBindings->Load("uikeys.txt");
	}
	else if (cmd == "keysave") {
		if (keyBindings->Save("uikeys.tmp")) {  // tmp, not txt
			logOutput.Print("Saved uikeys.tmp");
		} else {
			logOutput.Print("Could not save uikeys.tmp");
		}
	}
	else if (cmd == "keyprint") {
		keyBindings->Print();
	}
	else if (cmd == "keysyms") {
		keyCodes->PrintNameToCode();
	}
	else if (cmd == "keycodes") {
		keyCodes->PrintCodeToName();
	}
	else if (cmd == "clock") {
		if (action.extra.empty()) {
			showClock = !showClock;
		} else {
			showClock = !!atoi(action.extra.c_str());
		}
		configHandler.SetInt("ShowClock", showClock ? 1 : 0);
	}
	else if (cmd == "info") {
		if (action.extra.empty()) {
			if (playerRoster.GetSortType() == PlayerRoster::Disabled) {
				playerRoster.SetSortTypeByCode(PlayerRoster::Allies);
			} else {
				playerRoster.SetSortTypeByCode(PlayerRoster::Disabled);
			}
		} else {
			playerRoster.SetSortTypeByName(action.extra);
		}
		if (playerRoster.GetSortType() != PlayerRoster::Disabled) {
			logOutput.Print("Sorting roster by %s", playerRoster.GetSortName());
		}
		configHandler.SetInt("ShowPlayerInfo", (int)playerRoster.GetSortType());
	}
	else if (cmd == "techlevels") {
		unitDefHandler->SaveTechLevels("", modInfo->name); // stdout
		unitDefHandler->SaveTechLevels("techlevels.txt", modInfo->name);
	}
	else if (cmd == "cmdcolors") {
		const string name = action.extra.empty() ? "cmdcolors.txt" : action.extra;
		cmdColors.LoadConfig(name);
		logOutput.Print("Reloaded cmdcolors with: " + name);
	}
#ifndef NEW_GUI
	else if (cmd == "ctrlpanel") {
		const string name = action.extra.empty() ? "ctrlpanel.txt" : action.extra;
		guihandler->ReloadConfig(name);
		logOutput.Print("Reloaded ctrlpanel with: " + name);
	}
#endif
	else if (cmd == "font") {
		CglFont* newFont = NULL;
		try {
			newFont = SAFE_NEW CglFont(font->GetCharStart(), font->GetCharEnd(),
			                      action.extra.c_str());
		} catch (std::exception e) {
			delete newFont;
			newFont = NULL;
			logOutput.Print(string("font error: ") + e.what());
		}
		if (newFont != NULL) {
			delete font;
			font = newFont;
			logOutput.Print("Loaded font: %s\n", action.extra.c_str());
			configHandler.SetString("FontFile", action.extra);
		}
	}
	else if (cmd == "aiselect") {
		map<AIKey, string>::iterator aai;
		map<AIKey, string> suitedAis =
			grouphandler->GetSuitedAis(selectedUnits.selectedUnits);
		int i = 0;
		for (aai = suitedAis.begin(); aai != suitedAis.end(); ++aai) {
			i++;
			if (action.extra == aai->second) {
				Command cmd;
				cmd.id = CMD_AISELECT;
				cmd.params.push_back((float)i);
				selectedUnits.GiveCommand(cmd);
				break;
			}
		}
	}
	else if (cmd == "resbar") {
		if (resourceBar) {
			if (action.extra.empty()) {
				resourceBar->disabled = !resourceBar->disabled;
			} else {
				resourceBar->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "tooltip") {
		if (tooltip) {
			if (action.extra.empty()) {
				tooltip->disabled = !tooltip->disabled;
			} else {
				tooltip->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "console") {
		if (infoConsole) {
			if (action.extra.empty()) {
				infoConsole->disabled = !infoConsole->disabled;
			} else {
				infoConsole->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "luaui") {
		if (guihandler != NULL) {
			guihandler->RunLayoutCommand(action.extra);
		}
	}
	else if (cmd == "minimap") {
		if (minimap != NULL) {
			minimap->ConfigCommand(action.extra);
		}
	}
	else if (cmd == "maxparticles") {
		if (ph && !action.extra.empty()) {
			const int value = max(1, atoi(action.extra.c_str()));
			ph->SetMaxParticles(value);
			logOutput.Print("Set maximum particles to: %i", value);
		}
	}
	else if (cmd == "gathermode") {
		if (guihandler != NULL) {
			if (action.extra.empty()) {
				guihandler->SetGatherMode(!guihandler->GetGatherMode());
			} else {
				guihandler->SetGatherMode(!!atoi(action.extra.c_str()));
			}
		}
	}
	else {
		return false;
	}

	return true;
}


bool CGame::ActionReleased(const CKeyBindings::Action& action)
{
	const string& cmd = action.command;

	if (cmd == "drawinmap"){
		inMapDrawer->keyPressed=false;
	}
	else if (cmd == "moveforward") {
		camMove[0]=false;
	}
	else if (cmd == "moveback") {
		camMove[1]=false;
	}
	else if (cmd == "moveleft") {
		camMove[2]=false;
	}
	else if (cmd == "moveright") {
		camMove[3]=false;
	}
	else if (cmd == "moveup") {
		camMove[4]=false;
	}
	else if (cmd == "movedown") {
		camMove[5]=false;
	}
	else if (cmd == "movefast") {
		camMove[6]=false;
	}
	else if (cmd == "moveslow") {
		camMove[7]=false;
	}
	else if (cmd == "mouse1") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 1);
	}
	else if (cmd == "mouse2") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 2);
	}
	else if (cmd == "mouse3") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 3);
	}
	else if (cmd == "mousestate") {
		mouse->ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
	}
	else if (cmd == "gameinfoclose") {
		CGameInfo::Disable();
	}
	// HACK   somehow weird things happen when MouseRelease is called for button 4 and 5.
	// Note that SYS_WMEVENT on windows also only sends MousePress events for these buttons.
// 	else if (cmd == "mouse4") {
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 4);
//	}
// 	else if (cmd == "mouse5") {
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 5);
//	}

	return 0;
}

bool CGame::Update()
{
	good_fpu_control_registers("CGame::Update");

	mouse->EmptyMsgQueUpdate();
	script = CScriptHandler::Instance().chosenScript;
	assert(script);
	thisFps++;

	Uint64 timeNow;
	timeNow = SDL_GetTicks();
	Uint64 difTime;
	difTime=timeNow-lastModGameTimeMeasure;
	float dif=float(difTime)/1000.f;
	if(!gs->paused)
		gu->modGameTime+=dif*gs->speedFactor;
	gu->gameTime+=dif;
	if(playing && !gameOver)
		totalGameTime+=dif;
	lastModGameTimeMeasure=timeNow;

	if (gameServer && gu->autoQuit && gu->gameTime > gu->quitTime) {
		logOutput.Print("Automatical quit enforced from commandline");
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
		CInputReceiver::CollectGarbage();
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
			logOutput.Print("Client read net wanted quit");
			return false;
		}
	}

	if(gameSetup && !playing) {
		allReady=gameSetup->Update();
	} else if( gameServer && serverNet->waitOnCon) {
		allReady=true;
		for(int a=0;a<gs->activePlayers;a++) {
			if(gs->players[a]->active && !gs->players[a]->readyToStart) {
				allReady=false;
				break;
			}
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

//	logOutput << mouse->lastx << "\n";
	if(!gs->paused && gs->frameNum>1 && !creatingVideo){
		Uint64 startDraw;
		startDraw = SDL_GetTicks();
		gu->timeOffset = ((float)(startDraw - lastUpdate) * 0.001f)
		                 * (GAME_SPEED * gs->speedFactor);
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

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
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

	lineDrawer.UpdateLineStipple();
	if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
		selectedUnits.DrawCommands();
		cursorIcons.Draw();
	}

	mouse->Draw();

#ifndef NEW_GUI
	guihandler->DrawMapStuff(0);
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
		glColor4f(1,1,1,0.5f);
		glLineWidth(1.49f);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glVertex2f(0.5f-10.0f/gu->viewSizeX,0.5f);
		glVertex2f(0.5f+10.0f/gu->viewSizeX,0.5f);
		glVertex2f(0.5f,0.5f-10.0f/gu->viewSizeY);
		glVertex2f(0.5f,0.5f+10.0f/gu->viewSizeY);
		glLineWidth(1.0f);
		glEnd();
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl)
		DrawDirectControlHud();
#endif

	glEnable(GL_TEXTURE_2D);

	if(!hideInterface)
		infoConsole->Draw();

#ifndef NEW_GUI
	if(!hideInterface) {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty()){
			std::deque<CInputReceiver*>::iterator ri;
			for(ri=--inputReceivers.end();ri!=inputReceivers.begin();--ri){
				if (*ri)
					(*ri)->Draw();
			}
			if (*ri)
				(*ri)->Draw();
		}
	} else {
		guihandler->Update();
	}
#endif

	glEnable(GL_TEXTURE_2D);

	if(gu->drawdebug){
		glPushMatrix();
		glColor4f(1,1,0.5f,0.8f);
		glTranslatef(0.03f,0.02f,0.0f);
		glScalef(0.03f,0.04f,0.1f);

		//skriv ut fps etc
		font->glPrint("FPS %d Frame %d Units %d Part %d(%d)",fps,gs->frameNum,uh->activeUnits.size(),ph->ps.size(),ph->currentParticles);
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
		gameSetup->Draw();
	} else if( gameServer && serverNet->waitOnCon){
		if (allReady) {
			glColor3f(1.0f, 1.0f, 1.0f);
			font->glPrintCentered (0.5f, 0.5f, 1.5f, "Waiting for connections. Press return to start");
		}
	}

	if(userWriting){
		const string tempstring = userPrompt + userInput;

		const float xScale = 0.020f;
		const float yScale = 0.028f;
		
		glColor4f(1,1,1,1);
		glTranslatef(0.26f,0.73f,0.0f);
		glScalef(xScale, yScale, 1.0f);

		if (!outlineFont.IsEnabled()) {
			font->glPrintRaw(tempstring.c_str());
		}
		else {
			const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
			const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
			const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			outlineFont.print(xPixel, yPixel, white, tempstring.c_str());
		}
		
		glLoadIdentity();
	}

	if (!hotBinding.empty()) {
		glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
		font->glPrintCentered(0.5f, 0.6f, 3.0f, "Hit keyset for:");
		glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
		font->glPrintCentered(0.5f, 0.5f, 3.0f, "%s", hotBinding.c_str());
		glColor4f(0.3f, 0.3f, 1.0f, 1.0f);
		font->glPrintCentered(0.5f, 0.4f, 3.0f, "(or Escape)");
	}

	if (showClock) {
		char buf[32];
		const int seconds = (gs->frameNum / 30);
		if (seconds < 3600) {
			SNPRINTF(buf, sizeof(buf), "%02i:%02i", seconds / 60, seconds % 60);
		} else {
			SNPRINTF(buf, sizeof(buf), "%02i:%02i:%02i", seconds / 3600,
			                                 (seconds / 60) % 60, seconds % 60);
		}
		const float xScale = 0.015f;
		const float yScale = 0.020f;
		const float tWidth = font->CalcTextWidth(buf) * xScale;
		glTranslatef(0.99f - tWidth, 0.01f, 1.0f);
		glScalef(xScale, yScale, 1.0f);
		glColor4f(1,1,1,1);
		if (!outlineFont.IsEnabled()) {
			font->glPrintRaw(buf);
		} else {
			const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
			const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
			const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			outlineFont.print(xPixel, yPixel, white, buf);
		}
		glLoadIdentity();
	}

	if (playerRoster.GetSortType() != PlayerRoster::Disabled){
		char buf[128];
		const float xScale = 0.015f;
		const float yScale = 0.020f;
		const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
		const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
		int count;
		const int* indices = playerRoster.GetIndices(&count);
		for (int a = 0; a < count; ++a) {
			const CPlayer* p = gs->players[indices[a]];
			float color[4];
			const char* prefix;
			if (p->spectator) {
				color[0] = 1.0f;
				color[1] = 1.0f;
				color[2] = 1.0f;
				color[3] = 1.0f;
				prefix = "S|";
			} else {
				const unsigned char* bColor = gs->Team(p->team)->color;
				color[0] = (float)bColor[0] / 255.0f;
				color[1] = (float)bColor[1] / 255.0f;
				color[2] = (float)bColor[2] / 255.0f;
				color[3] = (float)bColor[3] / 255.0f;
				prefix = gu->myAllyTeam == gs->AllyTeam(p->team) ? "A|" : "E|";
			}
			const char format1[] = " %i:%s %s %3.0f%% Ping:%d ms";
			const char format2[] = "-%i:%s %s %3.0f%% Ping:%d ms";
			const char* format;
			if (gu->spectating && !p->spectator && (gu->myTeam == p->team)) {
				format = format2;
			} else {
				format = format1;
			}
			SNPRINTF(buf, sizeof(buf), format,
							 p->team, prefix, p->playerName.c_str(), p->cpuUsage * 100.0f,
							 (int)(((p->ping-1) * 1000) / (30 * gs->speedFactor)));
			glTranslatef(0.76f, 0.01f + (0.02f * (count - a - 1)), 1.0f);
			glScalef(xScale, yScale, 1.0f);
			glColor4fv(color);
			if (!outlineFont.IsEnabled()) {
				font->glPrintRaw(buf);
			} else {
				outlineFont.print(xPixel, yPixel, color, buf);
			}
			glLoadIdentity();
		}
	}

	mouse->DrawCursor();

//	float tf[]={1,1,1,1,1,1,1,1,1};
//	glVertexPointer(3,GL_FLOAT,0,tf);
//	glDrawArrays(GL_TRIANGLES,0,3);

	glEnable(GL_DEPTH_TEST );
	glLoadIdentity();

	Uint64 start;
	start = SDL_GetTicks();
	gu->lastFrameTime = (float)(start - lastMoveUpdate)/1000.f;
	lastMoveUpdate=start;

#ifndef NO_AVI
	if(creatingVideo){
		gu->lastFrameTime=1.0f/GAME_SPEED;
		LPBITMAPINFOHEADER ih;
		ih=aviGenerator->GetBitmapHeader();
		unsigned char* buf=SAFE_NEW unsigned char[ih->biWidth*ih->biHeight*3];
		glReadPixels(0,0,ih->biWidth,ih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

		aviGenerator->AddFrame(buf);

		delete buf;
//		logOutput.Print("Saved avi frame size %i %i",ih->biWidth,ih->biHeight);
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
}

void CGame::SimFrame()
{
	// Enable trapping of NaNs and divisions by zero to make debugging easier.
#ifdef DEBUG
	feraiseexcept(FPU_Exceptions(FE_INVALID | FE_DIVBYZERO));
#endif
	good_fpu_control_registers("CGame::SimFrame");

	ASSERT_SYNCED_MODE;
//	logOutput.Print("New frame %i %i %i",gs->frameNum,gs->randInt(),uh->CreateChecksum());
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
	waitCommandsAI.Update();
	infoConsole->Update();
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

START_TIME_PROFILE
	ph->CheckUnitCol();
	ground->CheckCol(ph);
END_TIME_PROFILE("Collisions");

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
			if(dist>unit->maxRange*0.95f)
				dist=unit->maxRange*0.95f;

			dc->targetDist=dist;
			dc->targetPos=pos+dc->viewDir*dc->targetDist;

			if(!dc->mouse2){
				unit->AttackGround(dc->targetPos,true);
				for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi){
					float d=dc->targetDist;
					if(d>(*wi)->range*0.95f)
						d=(*wi)->range*0.95f;
					float3 p=pos+dc->viewDir*d;
					(*wi)->AttackGround(p,true);
				}
			}
		}
	}

#endif

	ENTER_UNSYNCED;
	water->Update();
	ENTER_SYNCED;

#ifdef DEBUG
	feclearexcept(FPU_Exceptions(FE_INVALID | FE_DIVBYZERO));
#endif
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
		logOutput.Print("To much data read");
	if(inbufpos==inbuflength){
		inbufpos=0;
		inbuflength=0;
	}
	if(inbufpos>NETWORK_BUFFER_SIZE*0.5f){
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
		timeLeft+=consumeSpeed*((float)(currentFrame - lastframe)/1000.f);
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
					logOutput.Print("Unknown net msg in read ahead %i",(int)inbuf[i2]);
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
			logOutput.Print("Attempted connection to client?");
			break;

		case NETMSG_HELLO:
			lastLength=1;
			break;

		case NETMSG_QUIT:
			logOutput.Print("Server exited");
			net->connected=false;
			POP_CODE_MODE;
			return gameOver;

		case NETMSG_PLAYERLEFT:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in player left msg",player);
			} else {
				if(inbuf[inbufpos+2]==1)
					logOutput.Print("Player %s left",gs->players[player]->playerName.c_str());
				else
					logOutput.Print("Lost connection to %s",gs->players[player]->playerName.c_str());
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
				logOutput.Print("Automatical quit enforced from commandline");
				globalQuit = true;
			} else {
				SAFE_NEW CEndGameBox();
			}
			lastLength=1;
			ENTER_SYNCED;
			break;

		case NETMSG_SENDPLAYERSTAT:
			ENTER_MIXED;
			logOutput.Print("Game over");
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
				logOutput.Print("Got invalid player num %i in playerstat msg",player);
				lastLength=sizeof(CPlayer::Statistics)+2;
				break;
			}
			*gs->players[player]->currentStats=*(CPlayer::Statistics*)&inbuf[inbufpos+2];
			lastLength=sizeof(CPlayer::Statistics)+2;
			break;}

		case NETMSG_PAUSE:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in pause msg",player);
			} else {
				gs->paused=!!inbuf[inbufpos+1];
				if(gs->paused){
					logOutput.Print("%s paused the game",gs->players[player]->playerName.c_str());
				} else {
					logOutput.Print("%s unpaused the game",gs->players[player]->playerName.c_str());
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
//			logOutput.Print("Internal speed set to %.2f",gs->speedFactor);
			break;}

		case NETMSG_USER_SPEED:
			if (!net->playbackDemo) {
				gs->userSpeedFactor=*((float*)&inbuf[inbufpos+1]);
				if(gs->userSpeedFactor>maxUserSpeed)
					gs->userSpeedFactor=maxUserSpeed;
				if(gs->userSpeedFactor<minUserSpeed)
					gs->userSpeedFactor=minUserSpeed;
				logOutput.Print("Speed set to %.1f",gs->userSpeedFactor);
			}
			lastLength=5;
			break;

		case NETMSG_CPU_USAGE:
			logOutput.Print("Game clients shouldnt get cpu usage msgs?");
			lastLength=5;
			break;

		case NETMSG_PLAYERINFO:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in playerinfo msg",player);
			} else {
				gs->players[player]->cpuUsage=*(float*)&inbuf[inbufpos+2];
				gs->players[player]->ping=*(int*)&inbuf[inbufpos+6];
			}
			lastLength=10;
			break;}

		case NETMSG_SETPLAYERNUM:
			//logOutput.Print("Warning shouldnt receive NETMSG_SETPLAYERNUM in CGame");
			lastLength=2;
			break;

		case NETMSG_SCRIPT:
			lastLength = inbuf[inbufpos+1];
			break;

		case NETMSG_MAPNAME:
			archiveScanner->CheckMap(stupidGlobalMapname, *(unsigned*)(&inbuf[inbufpos+2]));
			lastLength = inbuf[inbufpos+1];
			break;

		case NETMSG_MODNAME:
			archiveScanner->CheckMod(modInfo->name, *(unsigned*)(&inbuf[inbufpos+2]));
			lastLength = inbuf[inbufpos+1];
			break;

		case NETMSG_PLAYERNAME:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in playername msg",player);
			} else {
				gs->players[player]->playerName=(char*)(&inbuf[inbufpos+3]);
				gs->players[player]->readyToStart=true;
				gs->players[player]->active=true;
				wordCompletion->AddWord(gs->players[player]->playerName, false, false, false); // required?
			}
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in chat msg",player);
			} else {
				string s=(char*)(&inbuf[inbufpos+3]);
				HandleChatMsg(s,player);
			}
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_SYSTEMMSG:{
			string s=(char*)(&inbuf[inbufpos+3]);
			logOutput.Print(s);
			lastLength=inbuf[inbufpos+1];
			break;}

		case NETMSG_STARTPOS:{
			int team=inbuf[inbufpos+1];
			if(team>=gs->activeTeams || team<0 || !gameSetup){
				logOutput.Print("Got invalid team num %i in startpos msg",team);
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
				logOutput.Print("Got invalid player num %i in command msg",player);
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
				logOutput.Print("Got invalid player num %i in netselect msg",player);
			} else {
				vector<int> selected;
				for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-4)/2);++a){
					int unitid=*((short int*)&inbuf[inbufpos+4+a*2]);
					if(unitid>=MAX_UNITS || unitid<0){
						logOutput.Print("Got invalid unitid %i in netselect msg",unitid);
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
				logOutput.Print("Got invalid player num %i in aicommand msg",player);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			int unitid=*((short int*)&inbuf[inbufpos+4]);
			if(unitid>=MAX_UNITS || unitid<0){
				logOutput.Print("Got invalid unitid %i in aicommand msg",unitid);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			//if(uh->units[unitid] && uh->units[unitid]->team!=gs->players[player]->team)		//msgs from global ais can be for other teams
			//	logOutput.Print("Warning player %i of team %i tried to send aiorder to unit from team %i",a,gs->players[player]->team,uh->units[unitid]->team);
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
				logOutput.Print("Sync request for wrong frame (%i instead of %i)", frame, gs->frameNum);
			}
			net->SendData<unsigned char, int, CChecksum>(
					NETMSG_SYNCRESPONSE, gu->myPlayerNum, gs->frameNum, uh->CreateChecksum());

			net->SendData<float>(NETMSG_CPU_USAGE,
#ifdef PROFILE_TIME
					profiler.profile["Sim time"].percent);
#else
					0.30f);
#endif
			lastLength=5;
			ENTER_SYNCED;
			break;}

		case NETMSG_SYNCERROR:
			logOutput.Print("Sync error");
			lastLength=5;
			break;

		case NETMSG_SHARE:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in share msg",player);
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
				logOutput.Print("Got invalid team num %i in setshare msg",team);
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
				logOutput.Print("Got invalid player num %i in direct control msg",player);
				lastLength=2;
				break;
			}

			if(gs->players[player]->playerControlledUnit){
				CUnit* unit=gs->players[player]->playerControlledUnit;
				//logOutput.Print("Player %s released control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());

				unit->directControl=0;
				unit->AttackUnit(0,true);
				gs->players[player]->StopControllingUnit();
			} else {
				if(!selectedUnits.netSelected[player].empty() && uh->units[selectedUnits.netSelected[player][0]] && !uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()){
					CUnit* unit=uh->units[selectedUnits.netSelected[player][0]];
					//logOutput.Print("Player %s took control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());
					if(unit->directControl){
						if(player==gu->myPlayerNum)
							logOutput.Print("Sorry someone already controls that unit");
					}	else {
						unit->directControl=&gs->players[player]->myControl;
						gs->players[player]->playerControlledUnit=unit;
						ENTER_UNSYNCED;
						if(player==gu->myPlayerNum){
							gu->directControl=unit;
							/* currentCamControllerNum isn't touched; it's used to
							   switch back to the old camera. */
							mouse->currentCamController=mouse->camControllers[0];	//set fps mode
							mouse->CameraTransition(1.0f);
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
				logOutput.Print("Got invalid player num %i in dc update msg",player);
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
				logOutput.Print("Unknown net msg in client %d last %d", (int)inbuf[inbufpos], lastMsg);
				lastLength=1;
			}
			break;
		}
		if(lastLength<=0){
			logOutput.Print("Client readnet got packet type %i length %i pos %i??",thisMsg,lastLength,inbufpos);
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
		int screenW = gu->dualScreenMode ? gu->viewSizeX*2 : gu->viewSizeX;
			if (mouse->lasty < 2){
				movement.y+=gu->lastFrameTime;
				unitTracker.Disable();
			}
			if (mouse->lasty > (gu->viewSizeY - 2)){
				movement.y-=gu->lastFrameTime;
				unitTracker.Disable();
			}
			if (mouse->lastx > (screenW - 2)){
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
		consoleHistory->AddLine(userInput);
		SendNetChat(userInput);
		chatting=false;
		userInput="";
	}
#ifndef NEW_GUI
	if(inMapDrawer->wantLabel && !userWriting){
		if(userInput.size()>200){	//avoid troubles with to long lines
			userInput=userInput.substr(0,200);
		}
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
	std::ofstream file(gameServer ? "memdump.txt" : "memdumpclient.txt");

	if (file.bad() || !file.is_open())
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
	logOutput.Print("Memdump finished");
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
	glTranslatef(0.1f,0.5f,0);
	glScalef(0.25f, 0.25f * gu->aspectRatio, 0.25f);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(unit->heading*180.0f/32768+180,0,0,1);

		glColor4d(0.3f,0.9f,0.3f,0.4f);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2f,-0.3f);
		glVertex2d(-0.2f,0.3f);
		glVertex2d(0,0.4f);
		glVertex2d(0.2f,0.3f);
		glVertex2d(0.2f,-0.3f);
		glVertex2d(-0.2f,-0.3f);
		glEnd();
		glPopMatrix();
	}

	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	if(unit->moveType->useHeading){
		float scale=0.4f/unit->radius;
		glScalef(scale,scale,scale);
		glRotatef(90,1,0,0);
	} else {
		float scale=0.2f/unit->radius;
		glScalef(scale,scale,-scale);
		CMatrix44f m(ZeroVector,float3(camera->right.x,camera->up.x,camera->forward.x),float3(camera->right.y,camera->up.y,camera->forward.y),float3(camera->right.z,camera->up.z,camera->forward.z));
		glMultMatrixf(m.m);
	}
	glTranslatef3(-unit->pos-unit->speed*gu->timeOffset);
	glDisable(GL_BLEND);
	unitDrawer->DrawIndividual(unit); // draw the unit
	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(GetHeadingFromVector(camera->forward.x,camera->forward.z)*180.0f/32768+180,0,0,1);
		glScalef(0.4f,0.4f,0.3f);

		glColor4d(0.4f,0.4f,1.0f,0.6f);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2f,-0.3f);
		glVertex2d(-0.2f,0.3f);
		glVertex2d(0,0.5f);
		glVertex2d(0.2f,0.3f);
		glVertex2d(0.2f,-0.3f);
		glVertex2d(-0.2f,-0.3f);
		glEnd();
		glPopMatrix();
	}
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(0.25f, 0.12f, 0);
	glScalef(0.03f,0.03f,0.03f);
	glColor4d(0.2f,0.8f,0.2f,0.8f);
	font->glPrint("Health %.0f",unit->health);
	glPopMatrix();

	if(gs->players[gu->myPlayerNum]->myControl.mouse2){
		glPushMatrix();
		glTranslatef(0.02f,0.34f,0);
		glScalef(0.03f,0.03f,0.03f);
		glColor4d(0.2f,0.8f,0.2f,0.8f);
		font->glPrint("Free fire mode");
		glPopMatrix();
	}
	for(int a=0;a<unit->weapons.size();++a){
		glPushMatrix();
		glTranslatef(0.02f,0.3f-a*0.04f,0);
		glScalef(0.0225f,0.03f,0.03f);
		CWeapon* w=unit->weapons[a];
		if(w->reloadStatus>gs->frameNum){
			glColor4d(0.8f,0.2f,0.2f,0.8f);
			font->glPrint("Weapon %i: Reloading",a+1);
		} else if(!w->angleGood){
			glColor4d(0.6f,0.6f,0.2f,0.8f);
			font->glPrint("Weapon %i: Aiming",a+1);
		} else {
			glColor4d(0.2f,0.8f,0.2f,0.8f);
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

	for(int a=0;a<unit->weapons.size();++a){
		CWeapon* w=unit->weapons[a];
		if(!w){
			logOutput.Print("Null weapon in vector?");
			return;
		}
		switch(a){
		case 0:
			glColor4d(0,1,0,0.7f);
			break;
		case 1:
			glColor4d(1,0,0,0.7f);
			break;
		default:
			glColor4d(0,0,1,0.7f);
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
			if((w->targetPos-camera->pos).Normalize().dot(camera->forward)<0.7f){
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


void CGame::SendNetChat(const std::string& message)
{
	if (message.empty()) {
		return;
	}
	string msg = message;
	if ((msg.find(".give") == 0) && (msg.find('@') == string::npos)) {
		const float3& pos = camera->pos;
		const float3& dir = mouse->dir;
		const float dist = ground->LineGroundCol(pos, pos + (dir * 9000.0f));
		const float3 p = pos + (dir * dist);
		char buf[128];
		SNPRINTF(buf, sizeof(buf), " @%.0f,%.0f,%.0f", p.x, p.y, p.z);
		msg += buf;
	}
	if (msg.size() > 128) {
		msg.resize(128); // safety
	}
	net->SendSTLData<unsigned char, std::string>(NETMSG_CHAT, gu->myPlayerNum, msg);
	if (net->playbackDemo) {
		HandleChatMsg(msg, gu->myPlayerNum);
	}
}


void CGame::HandleChatMsg(std::string s,int player)
{
	globalAI->GotChatMsg(s.c_str(),player);
	CScriptHandler::Instance().chosenScript->GotChatMsg(s, player);

	if(s.find(".cheat")==0 && player==0){
		char* end;
		const char* start = s.c_str() + strlen(".cheat");
		const int num = strtol(start, &end, 10);
		if (end != start) {
			gs->cheatEnabled = (num != 0);
		} else {
			gs->cheatEnabled = !gs->cheatEnabled; 
		}
		if (gs->cheatEnabled) {
			logOutput.Print("Cheating!");
		} else {
			logOutput.Print("No more cheating");
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
		logOutput.Print("Cheating!");
	}

	if (s.find(".crash") == 0 && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}

	if (s.find(".exception") == 0 && gs->cheatEnabled) {
		throw std::runtime_error("Exception test");
	}

	if (s.find(".divbyzero") == 0 && gs->cheatEnabled) {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}

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
		logOutput.Print("Desyncing in frame %d.", gs->frameNum);
	}
#ifdef SYNCDEBUG
	if (s.find(".fakedesync") == 0 && gs->cheatEnabled && gameServer && serverNet) {
		gameServer->fakeDesync = true;
		logOutput.Print("Fake desyncing.");
	}
	if (s.find(".reset") == 0 && gs->cheatEnabled) {
		CSyncDebugger::GetInstance()->Reset();
		logOutput.Print("Resetting sync debugger.");
	}
#endif

	if(s==".spectator" && (gs->cheatEnabled || net->playbackDemo)){
		gs->players[player]->spectator=true;
		if(player==gu->myPlayerNum){
			gu->spectating = true;
			gu->spectatingFullView = gu->spectating;
		}
	}
	if(s.find(".team")==0 && (gs->cheatEnabled || net->playbackDemo)){
		int team=atoi(&s.c_str()[s.find(" ")]);
		if(team>=0 && team<gs->activeTeams){
			gs->players[player]->team=team;
			gs->players[player]->spectator=false;
			if(player==gu->myPlayerNum){
				gu->spectating = false;
				gu->spectatingFullView = gu->spectating;
				gu->myTeam = team;
				gu->myAllyTeam = gs->AllyTeam(gu->myTeam);
				grouphandler->team = gu->myTeam;
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
			logOutput.Print("Someone is spoofing invalid .give messages!");
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

			UnitDef* unitDef = unitDefHandler->GetUnitByName(unitName);
			if (unitDef != NULL) {
				int xsize=unitDef->xsize;
				int zsize=unitDef->ysize;
				int total=numUnits;
				int squareSize=(int)ceil(sqrt((float)numUnits));
				float3 minpos=pos;
				minpos.x-=((squareSize-1)*xsize*SQUARE_SIZE)/2;
				minpos.z-=((squareSize-1)*zsize*SQUARE_SIZE)/2;
				for(int z=0;z<squareSize;++z){
					for(int x=0;x<squareSize && total>0;++x){
						unitLoader.LoadUnit(unitName,float3(minpos.x + x * xsize*SQUARE_SIZE, minpos.y,
						                                    minpos.z + z * zsize*SQUARE_SIZE), team, createNano);
						--total;
					}
				}
				logOutput.Print("Giving %i %s to team %i", numUnits, unitName.c_str(),team);
			} else {
				logOutput.Print(unitName+" is not a valid unitname");
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

	if (player == 0 && gameServer && serverNet) {
		if (s.find(".kickbynum") == 0) {
			if (s.length() >= 11) {
				int a = atoi(s.substr(11, string::npos).c_str());
				if (a != 0 && gs->players[a]->active) {
					unsigned char c=NETMSG_QUIT;
					serverNet->SendData(&c,1,a);
					serverNet->FlushConnection(a);
					//this will rather ungracefully close the connection from our side
					serverNet->connections[a].readyData[0]=NETMSG_QUIT;
					//so if the above was lost in packetlos etc it will never be resent
					serverNet->connections[a].readyLength=1;
				}
			}
		}
		else if (s.find(".kick") == 0){
			if (s.length() >= 6) {
				string name=s.substr(6,string::npos);
				if (!name.empty()){
					StringToLowerInPlace(name);
					for (int a=1;a<gs->activePlayers;++a){
						if (gs->players[a]->active){
							string p = StringToLower(gs->players[a]->playerName);
							if (p.find(name)==0){               //can kick on substrings of name
								unsigned char c=NETMSG_QUIT;
								serverNet->SendData(&c,1,a);
								serverNet->FlushConnection(a);
								//this will rather ungracefully close the connection from our side
								serverNet->connections[a].readyData[0]=NETMSG_QUIT;
								//so if the above was lost in packetlos etc it will never be resent
								serverNet->connections[a].readyLength=1;
							}
						}
					}
				}
			}
		}
	}

	if (player == 0) {
		if (s.find(".nospectatorchat")==0) {
			noSpectatorChat=!noSpectatorChat;
			logOutput.Print("No spectator chat %i",noSpectatorChat);
		}
		if (s.find(".nopause")==0) {
			gamePausable=!gamePausable;
		}
		if (s.find(".setmaxspeed")==0) {
			maxUserSpeed=atof(s.substr(12).c_str());
			if(gs->userSpeedFactor>maxUserSpeed)
				gs->userSpeedFactor=maxUserSpeed;
		}
		if (s.find(".setminspeed")==0) {
			minUserSpeed=atof(s.substr(12).c_str());
			if(gs->userSpeedFactor<minUserSpeed)
				gs->userSpeedFactor=minUserSpeed;
		}
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
			logOutput.Print(s);
			sound->PlaySample(chatSound,5);
		}
	} else if((s[0]=='s' || s[0]=='S') && s[1]==':'){
		if(player==gu->myPlayerNum)
			userInputPrefix="s:";

		if((gu->spectating || gu->myPlayerNum == player) && s.substr(2,255).length() > 0) {
			if(gs->players[player]->spectator)
				s="["+gs->players[player]->playerName+"] Spectators: "+s.substr(2,255);
			else
				s="<"+gs->players[player]->playerName+"> Spectators: "+s.substr(2,255);
			logOutput.Print(s);
			sound->PlaySample(chatSound,5);
		}
	} else {
		if(player==gu->myPlayerNum)
			userInputPrefix="";

		if(gs->players[player]->spectator)
			s="["+gs->players[player]->playerName+"] "+s;
		else
			s="<"+gs->players[player]->playerName+"> "+s;

		logOutput.Print(s);
		sound->PlaySample(chatSound,5);
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
	unsigned char* buf=SAFE_NEW unsigned char[l];
	f.Read(buf,l);

	for(int a=0;a<l;++a){
		ret+=buf[a];
		ret*=max((unsigned char)1,buf[a]);
	}

	delete[] buf;
	return ret;*/
	return 0;
}


static void SelectUnits(const string& line)
{
	vector<string> args = SimpleParser::Tokenize(line, 0);
	for (int i = 0; i < (int)args.size(); i++) {
		const string& arg = args[i];
		if (arg == "clear") {
			selectedUnits.ClearSelected();
		}
		else if ((arg[0] == '+') || (arg[0] == '-')) {
			char* endPtr;
			const char* startPtr = arg.c_str() + 1;
			const int unitIndex = strtol(startPtr, &endPtr, 10);
			if (endPtr == startPtr) {
				continue; // bad number
			}
			if ((unitIndex < 0) || (unitIndex >= MAX_UNITS)) {
				continue; // bad index
			}
			CUnit* unit = uh->units[unitIndex];
			if (unit == NULL) {
				continue; // bad pointer
			}
			const set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
			if (teamUnits.find(unit) == teamUnits.end()) {
				continue; // not mine to select
			}

			// perform the selection
			if (arg[0] == '+') {
				selectedUnits.AddUnit(unit);
			} else {
				selectedUnits.RemoveUnit(unit);
			}
		}
	}
}


static void SelectCycle(const string& command)
{
	static set<int> unitIDs;
	static int lastID = -1;
		
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	
	if (command == "restore") {
		selectedUnits.ClearSelected();
		set<int>::const_iterator it;
		for (it = unitIDs.begin(); it != unitIDs.end(); ++it) {
			CUnit* unit = uh->units[*it];
			if (unit != NULL) {
				selectedUnits.AddUnit(unit);
			}
		}
		return;
	}
	
	if (selUnits.size() >= 2) {
		// assign the cycle units
		unitIDs.clear();
		set<CUnit*>::const_iterator it;
		for (it = selUnits.begin(); it != selUnits.end(); ++it) {
			unitIDs.insert((*it)->id);
		}
		selectedUnits.ClearSelected();
		lastID = *unitIDs.begin();
		selectedUnits.AddUnit(uh->units[lastID]);
		return;
	}
	
	// clean the list
	set<int> tmpSet;
	set<int>::const_iterator it;
	for (it = unitIDs.begin(); it != unitIDs.end(); ++it) {
		if (uh->units[*it] != NULL) {
			tmpSet.insert(*it);
		}
	}
	unitIDs = tmpSet;
	if ((lastID >= 0) && (uh->units[lastID] == NULL)) {
		lastID = -1;
	}

	// selectedUnits size is 0 or 1
	selectedUnits.ClearSelected();
	if (!unitIDs.empty()) {
		set<int>::const_iterator fit = unitIDs.find(lastID);
		if (fit == unitIDs.end()) {
			lastID = *unitIDs.begin();
		} else {
			fit++;
			if (fit != unitIDs.end()) {
				lastID = *fit;
			} else {
				lastID = *unitIDs.begin();
			}
		}
		selectedUnits.AddUnit(uh->units[lastID]);
	}
}
