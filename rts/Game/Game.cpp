// Game.cpp: implementation of the CGame class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

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
#include <SDL_events.h>

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
#include "ExternalAI/Group.h"
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
#include "NetProtocol.h"
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
#include "Rendering/VerticalSync.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaCob.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaOpenGL.h"
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
#include "System/Sound.h"
#include "System/Platform/NullSound.h"
#include "Platform/Clipboard.h"
#include "UI/CommandColors.h"
#include "UI/CursorIcons.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/KeyBindings.h"
#include "UI/KeyCodes.h"
#include "UI/LuaUI.h"
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
#include "Sim/Weapons/WeaponDefHandler.h"
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




CGame::CGame(bool server,std::string mapname, std::string modName, CInfoConsole *ic)
{
	infoConsole = ic;

	script = NULL;

	time(&starttime);
	lastTick=clock();
	consumeSpeed=1;
	leastQue=0;
	que=0;
	timeLeft=0.0f;

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
	drawFpsHUD = true;

	debugging=false;

	bOneStep=false;
	creatingVideo=false;

	skipping=0;
	playing=false;
	allReady=false;
	hideInterface=false;
	gameOver=false;
	windowedEdgeMove=!!configHandler.GetInt("WindowedEdgeMove", 1);
	showClock=!!configHandler.GetInt("ShowClock", 1);
	playerRoster.SetSortTypeByCode(
	  (PlayerRoster::SortType)configHandler.GetInt("ShowPlayerInfo", 1));

	const string inputTextGeo = configHandler.GetString("InputTextGeo", "");
	ParseInputTextGeometry("default");
	ParseInputTextGeometry(inputTextGeo);

	gamePausable=true;
	noSpectatorChat=false;

	inbufpos=0;
	inbuflength=0;

	chatting=false;
	userInput="";
	writingPos = 0;
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

	ENTER_UNSYNCED;

	guihandler=SAFE_NEW CGuiHandler();
	minimap=SAFE_NEW CMiniMap();

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

	ENTER_MIXED;
	uh=SAFE_NEW CUnitHandler();
	iconHandler=SAFE_NEW CIconHandler();
	unitDrawer=SAFE_NEW CUnitDrawer();
	fartextureHandler = SAFE_NEW CFartextureHandler();
	modelParser = SAFE_NEW C3DModelParser();

 	ENTER_SYNCED;
 	featureHandler->LoadFeaturesFromMap(CScriptHandler::Instance().chosenScript->loadGame);
 	pathManager = SAFE_NEW CPathManager();

	ENTER_UNSYNCED;
	sky=CBaseSky::GetSky();

	resourceBar=SAFE_NEW CResourceBar();
	keyCodes=SAFE_NEW CKeyCodes();
	keyBindings=SAFE_NEW CKeyBindings();
	keyBindings->Load("uikeys.txt");

	water=CBaseWater::GetWater();
	grouphandler=SAFE_NEW CGroupHandler(gu->myTeam);
	globalAI=SAFE_NEW CGlobalAIHandler();

	CLuaCob::LoadHandler();
	if (gs->useLuaRules) {
		CLuaRules::LoadHandler();
	}
	if (gs->useLuaGaia) {
		CLuaGaia::LoadHandler();
	}

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

	logOutput.Print("Spring %s",VERSION_STRING);

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
	net->SendPlayerName(gu->myPlayerNum, p->playerName);

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
	if (treeDrawer) {
		configHandler.SetInt("TreeRadius",
		                     (unsigned int)(treeDrawer->baseTreeDistance * 256));
	}

	ENTER_MIXED;

	delete guihandler;
	guihandler = NULL;

#ifndef NO_AVI
	if(creatingVideo){
		creatingVideo=false;
		aviGenerator->ReleaseEngine();
		delete aviGenerator;
		aviGenerator = NULL;
	}
#endif
#ifdef TRACE_SYNC
	tracefile << "End game\n";
#endif

	CLuaCob::FreeHandler();
	CLuaGaia::FreeHandler();
	CLuaRules::FreeHandler();
	LuaOpenGL::Free();

	delete gameServer;         gameServer         = NULL;

	globalAI->PreDestroy ();
	delete water;              water              = NULL;
	delete sky;                sky                = NULL;

	delete resourceBar;        resourceBar        = NULL;
	delete uh;                 uh                 = NULL;
	delete unitDrawer;         unitDrawer         = NULL;
	delete featureHandler;     featureHandler     = NULL;
	delete geometricObjects;   geometricObjects   = NULL;
	delete ph;                 ph                 = NULL;
	delete globalAI;           globalAI           = NULL;
	delete grouphandler;       grouphandler       = NULL;
	delete minimap;            minimap            = NULL;
	delete pathManager;        pathManager        = NULL;
	delete groundDecals;       groundDecals       = NULL;
	delete ground;             ground             = NULL;
	delete inMapDrawer;        inMapDrawer        = NULL;
	delete net;                net                = NULL;
	delete radarhandler;       radarhandler       = NULL;
	delete loshandler;         loshandler         = NULL;
	delete mapDamage;          mapDamage          = NULL;
	delete qf;                 qf                 = NULL;
	delete tooltip;            tooltip            = NULL;
	delete keyBindings;        keyBindings        = NULL;
	delete keyCodes;           keyCodes           = NULL;
	delete sound;              sound              = NULL;
	delete selectionKeys;      selectionKeys      = NULL;
	delete mouse;              mouse              = NULL;
	delete helper;             helper             = NULL;
	delete shadowHandler;      shadowHandler      = NULL;
	delete moveinfo;           moveinfo           = NULL;
	delete unitDefHandler;     unitDefHandler     = NULL;
	delete damageArrayHandler; damageArrayHandler = NULL;
	delete hpiHandler;         hpiHandler         = NULL;
	delete archiveScanner;     archiveScanner     = NULL;
	delete modelParser;        modelParser        = NULL;
	delete fartextureHandler;  fartextureHandler  = NULL;
	delete modInfo;            modInfo            = NULL;
	delete camera;             camera             = NULL;
	delete cam2;               cam2               = NULL;
	delete infoConsole;        infoConsole        = NULL;
	delete consoleHistory;     consoleHistory     = NULL;
	delete wordCompletion;     wordCompletion     = NULL;
	delete explGenHandler;     explGenHandler     = NULL;
	CCategoryHandler::RemoveInstance();
	CColorMap::DeleteColormaps();
}


void CGame::ResizeEvent()
{
	if (minimap != NULL) {
		minimap->UpdateGeometry();
	}

	// Fix water renderer, they depend on screen resolution...
	delete water;
	water = CBaseWater::GetWater();
}


//called when the key is pressed by the user (can be called several times due to key repeat)
int CGame::KeyPressed(unsigned short k, bool isRepeat)
{
	if (!gameOver && !isRepeat) {
		gs->players[gu->myPlayerNum]->currentStats->keyPresses++;
	}

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
					writingPos = max(0, writingPos - 2);
				}
				userInputPrefix = "";
				return 0;
			}
			else if (action.command == "chatswitchally") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 'a';
				} else {
					userInput = "a:" + userInput;
					writingPos += 2;
				}
				userInputPrefix = "a:";
				return 0;
			}
			else if (action.command == "chatswitchspec") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 's';
				} else {
					userInput = "s:" + userInput;
					writingPos += 2;
				}
				userInputPrefix = "s:";
				return 0;
			}
			else if (action.command == "pastetext") {
				if (!action.extra.empty()) {
					userInput.insert(writingPos, action.extra);
					writingPos += action.extra.length();
				} else {
					CClipboard clipboard;
					const string text = clipboard.GetContents();
					userInput.insert(writingPos, text);
					writingPos += text.length();
				}
				return 0;
			}
		}

		if (k == SDLK_BACKSPACE) {
			if (!userInput.empty() && (writingPos > 0)) {
				userInput.erase(writingPos - 1, 1);
				writingPos--;
			}
		}
		else if (k == SDLK_DELETE) {
			if (!userInput.empty() && (writingPos < (int)userInput.size())) {
				userInput.erase(writingPos, 1);
			}
		}
		else if (k == SDLK_LEFT) {
			if (keys[SDLK_LALT]) {
				// home
				writingPos = 0;
			}
			else if (keys[SDLK_LCTRL]) {
				// prev word
				const char* s = userInput.c_str();
				int p = writingPos;
				while ((p > 0) && !isalnum(s[p - 1])) { p--; }
				while ((p > 0) &&  isalnum(s[p - 1])) { p--; }
				writingPos = p;
			}
			else {
				// prev character
				writingPos = max(0, min((int)userInput.length(), writingPos - 1));
			}
		}
		else if (k == SDLK_RIGHT) {
			if (keys[SDLK_LALT]) {
				// end
				writingPos = (int)userInput.length();
			}
			else if (keys[SDLK_LCTRL]) {
				// next word
				const int len = (int)userInput.length();
				const char* s = userInput.c_str();
				int p = writingPos;
				while ((p < len) && !isalnum(s[p])) { p++; }
				while ((p < len) &&  isalnum(s[p])) { p++; }
				writingPos = p;
			}
			else {
				// next character
				writingPos = max(0, min((int)userInput.length(), writingPos + 1));
			}
		}
		else if (k == SDLK_HOME) {
			writingPos = 0;
		}
		else if (k == SDLK_END) {
			writingPos = (int)userInput.length();
		}
		else if (k == SDLK_RETURN){
			userWriting=false;
			writingPos = 0;
			keys[k] = false;		//prevent game start when server chats
			if (chatting) {
				string command;
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					command = userInput.substr(2);
				} else {
					command = userInput;
				}
				if ((command[0] == '/') && (command[1] != '/')) {
					// execute an action
					consoleHistory->AddLine(command);
					const string actionLine = command.substr(1); // strip the '/'
					chatting = false;
					userInput = "";
					writingPos = 0;
					logOutput.Print(command);
					CKeySet ks(k, false);
					CKeyBindings::Action fakeAction(actionLine);
					ActionPressed(fakeAction, ks, isRepeat);
				}
			}
		}
		else if ((k == 27) && (chatting || inMapDrawer->wantLabel)){ // escape
			if (chatting) {
				consoleHistory->AddLine(userInput);
			}
			userWriting=false;
			chatting=false;
			inMapDrawer->wantLabel=false;
			userInput="";
			writingPos = 0;
		}
		else if ((k == SDLK_UP) && chatting) {
			userInput = consoleHistory->PrevLine(userInput);
			writingPos = (int)userInput.length();
		}
		else if ((k == SDLK_DOWN) && chatting) {
			userInput = consoleHistory->NextLine(userInput);
			writingPos = (int)userInput.length();
		}
		else if (k == SDLK_TAB) {
			string head = userInput.substr(0, writingPos);
			string tail = userInput.substr(writingPos);
			vector<string> partials = wordCompletion->Complete(head);
			userInput = head + tail;
			writingPos = (int)head.length();
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
			int mapsize = 2048;
			const char* args = action.extra.c_str();
			const int argcount = sscanf(args, "%i %i", &next, &mapsize);
			if (argcount > 1) {
				configHandler.SetInt("ShadowMapSize", mapsize);
			}
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
		inMapDrawer->keyPressed = true;
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
	else if (cmd == "viewfree") {
		mouse->SetCameraMode(4);
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
			mouse->CameraTransition(0.6f);
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
	else if ((cmd == "specteam") && gu->spectating) {
		const int team = atoi(action.extra.c_str());
		if ((team >= 0) && (team < gs->activeTeams)) {
			gu->myTeam = team;
			gu->myAllyTeam = gs->AllyTeam(team);
		}
		CLuaUI::UpdateTeams();
	}
	else if ((cmd == "specfullview") && gu->spectating) {
		if (!action.extra.empty()) {
			const int mode = atoi(action.extra.c_str());
			gu->spectatingFullView   = (mode >= 1);
			gu->spectatingFullSelect = (mode >= 2);
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
			gu->spectatingFullSelect = false;
		}
		CLuaUI::UpdateTeams();
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
		writingPos = (int)userInput.length();
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
		if (action.extra.empty()) {
			unitDrawer->showHealthBars = !unitDrawer->showHealthBars;
		} else {
			unitDrawer->showHealthBars = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "pause") {
		bool newPause;
		if (action.extra.empty()) {
			newPause = !gs->paused;
		} else {
			newPause = !!atoi(action.extra.c_str());
		}
		if (net->playbackDemo) {
			gs->paused = newPause;
		} else {
			net->SendPause(gu->myPlayerNum, newPause);
			lastframe = SDL_GetTicks(); // this required here?
		}
	}
	else if (cmd == "singlestep") {
		bOneStep = true;
	}
	else if (cmd == "debug") {
		gu->drawdebug = !gu->drawdebug;
	}
	else if (cmd == "nosound") {
		soundEnabled = !soundEnabled;
		sound->SetVolume (soundEnabled ? gameSoundVolume : 0.0f);
		if (soundEnabled) {
			logOutput.Print("Sound enabled");
		} else {
			logOutput.Print("Sound disabled");
		}
	}
	else if (cmd == "volume") {
		char* endPtr;
		const char* startPtr = action.extra.c_str();
		float volume = (float)strtod(startPtr, &endPtr);
		if (endPtr != startPtr) {
			gameSoundVolume = max(0.0f, min(1.0f, volume));
			sound->SetVolume(gameSoundVolume);
			configHandler.SetInt("SoundVolume", (int)(gameSoundVolume * 100.0f));
		}
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
			for(int a=0;a<999;++a){
				char t[50];
				itoa(a,t,10);
				name=string("video")+t+".avi";
				CFileHandler ifs(name);
				if(!ifs.FileExists())
					break;
			}

			BITMAPINFOHEADER bih;
			memset(&bih,0, sizeof(BITMAPINFOHEADER));

			// filling bitmap info structure.
			bih.biSize=sizeof(BITMAPINFOHEADER);
			bih.biWidth=(gu->viewSizeX/4)*4;
			bih.biHeight=(gu->viewSizeY/4)*4;
			bih.biPlanes=1;
			bih.biBitCount=24;
			bih.biSizeImage=bih.biWidth*bih.biHeight*3;
			bih.biCompression=BI_RGB;


			aviGenerator = SAFE_NEW CAVIGenerator(name, &bih, 30);
			int savedCursorMode = SDL_ShowCursor(SDL_QUERY);
			SDL_ShowCursor(SDL_ENABLE);
			HRESULT hr=aviGenerator->InitEngine();
			SDL_ShowCursor(savedCursorMode);
			//aviGenerator->InitEngine() (avicap32.dll)? modifies the FPU control word.
			//Setting it back to 'normal'.
			streflop_init<streflop::Simple>();

			if(hr!=AVIERR_OK){
				creatingVideo=false;
				logOutput.Print(aviGenerator->GetLastErrorMessage());
				delete aviGenerator;
				aviGenerator=0;
			} else {
				logOutput.Print("Recording avi to %s size %i %i",name.c_str(), bih.biWidth, bih.biHeight);
			}
		}
	}
#endif

	else if (cmd == "updatefov") {
		if (action.extra.empty()) {
			gd->updateFov = !gd->updateFov;
		} else {
			gd->updateFov = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "drawtrees") {
		if (action.extra.empty()) {
			treeDrawer->drawTrees = !treeDrawer->drawTrees;
		} else {
			treeDrawer->drawTrees = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "dynamicsky") {
		if (action.extra.empty()) {
			sky->dynamicSky = !sky->dynamicSky;
		} else {
			sky->dynamicSky = !!atoi(action.extra.c_str());
		}
	}
	else if (!isRepeat && (cmd == "gameinfo")) {
		if (!CGameInfo::IsActive()) {
			CGameInfo::Enable();
		} else {
			CGameInfo::Disable();
		}
	}
	else if (cmd == "hideinterface") {
		if (action.extra.empty()) {
			hideInterface = !hideInterface;
		} else {
			hideInterface = !!atoi(action.extra.c_str());
		}
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
			net->SendUserSpeed(speed);
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
			net->SendUserSpeed(speed);
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
		net->SendDirectControl(gu->myPlayerNum);
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
	else if (cmd == "ctrlpanel") {
		const string name = action.extra.empty() ? "ctrlpanel.txt" : action.extra;
		guihandler->ReloadConfig(name);
		logOutput.Print("Reloaded ctrlpanel with: " + name);
	}
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
			LuaOpenGL::CalcFontHeight();
		}
	}
	else if (cmd == "aiselect") {
		if (gs->noHelperAIs) {
			logOutput.Print("GroupAI and LuaUI control is disabled");
			return true;
		}
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
	else if (cmd == "vsync") {
		if (action.extra.empty()) {
			VSync.SetFrames((VSync.GetFrames() <= 0) ? 1 : 0);
		} else {
			VSync.SetFrames(atoi(action.extra.c_str()));
		}
	}
	else if (cmd == "safegl") {
		if (action.extra.empty()) {
			LuaOpenGL::SetSafeMode(!LuaOpenGL::GetSafeMode());
		} else {
			LuaOpenGL::SetSafeMode(atoi(action.extra.c_str()));
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
	else if (cmd == "endgraph") {
		if (action.extra.empty()) {
			CEndGameBox::disabled = !CEndGameBox::disabled;
		} else {
			CEndGameBox::disabled = !atoi(action.extra.c_str());
		}
	}
	else if (cmd == "fpshud") {
		if (action.extra.empty()) {
			drawFpsHUD = !drawFpsHUD;
		} else {
			drawFpsHUD = !atoi(action.extra.c_str());
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
	else if (cmd == "pastetext") {
		if (userWriting){
			if (!action.extra.empty()) {
				userInput.insert(writingPos, action.extra);
				writingPos += action.extra.length();
			} else {
				CClipboard clipboard;
				const string text = clipboard.GetContents();
				userInput.insert(writingPos, text);
				writingPos += text.length();
			}
			return 0;
		}
	}
	else if (cmd == "buffertext") {
		if (!action.extra.empty()) {
			consoleHistory->AddLine(action.extra);
		}
	}
	else if (cmd == "inputtextgeo") {
		if (!action.extra.empty()) {
			ParseInputTextGeometry(action.extra);
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

	const Uint64 difTime = (timeNow - lastModGameTimeMeasure);
	const float dif = skipping ? 0.010f : (float)difTime * 0.001f;

	if (!gs->paused) {
		gu->modGameTime += dif * gs->speedFactor;
	}
	gu->gameTime += dif;
	if (playing && !gameOver) {
		totalGameTime += dif;
	}
	lastModGameTimeMeasure = timeNow;

	if (gameServer && gu->autoQuit && (gu->gameTime > gu->quitTime)) {
		logOutput.Print("Automatical quit enforced from commandline");
		return false;
	}

	time(&fpstimer);

	if (difftime(fpstimer, starttime) != 0) {		//do once every second
		fps=thisFps;
		thisFps=0;

		consumeSpeed=((float)(GAME_SPEED+leastQue-2));
		leastQue=10000;
		if (!gameServer) {
			timeLeft = 0.0f;
		}

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
	if (gameServer)
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

	if (gameServer && serverNet->waitOnCon && allReady &&
	    (keys[SDLK_RETURN] || script->onlySinglePlayer || gameSetup)) {
		chatting = false;
		userWriting = false;
		writingPos = 0;
		gameServer->StartGame();
	}

	return true;
}

bool CGame::Draw()
{
	ASSERT_UNSYNCED_MODE;

	luaCallIns.Update();

	if (!gu->active) {
		// update LuaUI
		guihandler->Update();
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

	luaCallIns.DrawWorld();

	lineDrawer.UpdateLineStipple();
	if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
		selectedUnits.DrawCommands();
		cursorIcons.Draw();
	}
	cursorIcons.Clear();

	mouse->Draw();

	guihandler->DrawMapStuff(0);

	inMapDrawer->Draw();

	if (camera->pos.y < 0.0f) {
		const float3& cpos = camera->pos;
		const float vr = gu->viewRange * 0.5f;
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.5f, 0.3f, 0.50f);
		glBegin(GL_QUADS);
		glVertex3f(cpos.x - vr, 0.0f, cpos.z - vr);
		glVertex3f(cpos.x - vr, 0.0f, cpos.z + vr);
		glVertex3f(cpos.x + vr, 0.0f, cpos.z + vr);
		glVertex3f(cpos.x + vr, 0.0f, cpos.z - vr);
		glEnd();
		glBegin(GL_QUAD_STRIP);
		glVertex3f(cpos.x - vr, 0.0f, cpos.z - vr);
		glVertex3f(cpos.x - vr,  -vr, cpos.z - vr);
		glVertex3f(cpos.x - vr, 0.0f, cpos.z + vr);
		glVertex3f(cpos.x - vr,  -vr, cpos.z + vr);
		glVertex3f(cpos.x + vr, 0.0f, cpos.z + vr);
		glVertex3f(cpos.x + vr,  -vr, cpos.z + vr);
		glVertex3f(cpos.x + vr, 0.0f, cpos.z - vr);
		glVertex3f(cpos.x + vr,  -vr, cpos.z - vr);
		glVertex3f(cpos.x - vr, 0.0f, cpos.z - vr);
		glVertex3f(cpos.x - vr,  -vr, cpos.z - vr);
		glEnd();
	}

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

	if (camera->pos.y < 0.0f) {
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);
		glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	}
	luaCallIns.DrawScreen();

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
	if(gu->directControl){
		DrawDirectControlHud();
	}
#endif

	glEnable(GL_TEXTURE_2D);

	if(!hideInterface)
		infoConsole->Draw();

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

	if (userWriting) {
		DrawInputText();
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
	lastMoveUpdate = start;

#ifndef NO_AVI
	if(creatingVideo){
		gu->lastFrameTime=1.0f/GAME_SPEED;
		LPBITMAPINFOHEADER ih;
		ih = aviGenerator->GetBitmapHeader();
		unsigned char* buf = aviGenerator->GetPixelBuf();
		glReadPixels(0,0,ih->biWidth, ih->biHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, buf);

		aviGenerator->AddFrame(buf);

//		logOutput.Print("Saved avi frame size %i %i",ih->biWidth,ih->biHeight);
	}
#endif

	return true;
}


void CGame::ParseInputTextGeometry(const string& geo)
{
	if (geo == "default") { // safety
		ParseInputTextGeometry("0.26 0.73 0.02 0.028");
		return;
	}
	float px, py, sx, sy;
	if (sscanf(geo.c_str(), "%f %f %f %f", &px, &py, &sx, &sy) == 4) {
		inputTextPosX  = px;
		inputTextPosY  = py;
		inputTextSizeX = sx;
		inputTextSizeY = sy;
		configHandler.SetString("InputTextGeo", geo);
	}
}


void CGame::DrawInputText()
{
	const string tempstring = userPrompt + userInput;

	// draw the caret
	const int caretPos = userPrompt.length() + writingPos;
	const string caretStr = tempstring.substr(0, caretPos);
	const float caretWidth = font->CalcTextWidth(caretStr.c_str());
	char c = userInput[writingPos];
	if (c == 0) { c = ' '; }
	const float cw = inputTextSizeX * font->CalcCharWidth(c);
	const float csx = inputTextPosX + (inputTextSizeX * caretWidth);
	glDisable(GL_TEXTURE_2D);
	const float f = 0.5f * (1.0f + sin((float)SDL_GetTicks() * 0.015f));
	glColor4f(f, f, f, 0.75f);
	glRectf(csx, inputTextPosY, csx + cw, inputTextPosY + inputTextSizeY);
	glEnable(GL_TEXTURE_2D);

	// setup the color
	const float defColor[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float allyColor[4] = { 0.5f, 1.0f, 0.5f, 1.0f };
	const float specColor[4] = { 1.0f, 1.0f, 0.5f, 1.0f };
	const float* textColor = defColor;
	if (userInput.length() < 2) {
		textColor = defColor;
	} else if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
		textColor = allyColor;
	} else if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
		textColor = specColor;
	} else {
		textColor = defColor;
	}

	// draw the text
	glTranslatef(inputTextPosX, inputTextPosY, 0.0f);
	glScalef(inputTextSizeX, inputTextSizeY, 1.0f);
	if (!outlineFont.IsEnabled()) {
		glColor4fv(textColor);
		font->glPrintRaw(tempstring.c_str());
	} else {
		const float xPixel  = 1.0f / (inputTextSizeX * (float)gu->viewSizeX);
		const float yPixel  = 1.0f / (inputTextSizeY * (float)gu->viewSizeY);
		outlineFont.print(xPixel, yPixel, textColor, tempstring.c_str());
	}
	glLoadIdentity();
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
	CLuaUI::UpdateTeams();
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

	if (luaCob)   { luaCob->GameFrame(gs->frameNum); }
	if (luaGaia)  { luaGaia->GameFrame(gs->frameNum); }
	if (luaRules) { luaRules->GameFrame(gs->frameNum); }

	gs->frameNum++;

	ENTER_UNSYNCED;

	if (!skipping) {
		infoConsole->Update();
		waitCommandsAI.Update();
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
				net->SendDirectControlUpdate(gu->myPlayerNum, status, hp.x, hp.y);
			}
		}
#endif
	}

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

	if (!(gs->frameNum & 31)) {
		for (int a = 0; a < gs->activeTeams; ++a) {
			gs->Team(a)->SlowUpdate();
		}
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
	if (!skipping) {
		water->Update();
	}
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
		logOutput.Print("Too much data read");
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

		if (timeLeft > 1.0f) {
			timeLeft = timeLeft - 1.0f;
		}
		timeLeft+=consumeSpeed*((float)(currentFrame - lastframe)/1000.f);
		if (skipping) {
			timeLeft = 0.01f;
		}
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
			luaCallIns.GameOver();
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
			net->SendPlayerStat(gu->myPlayerNum, *gs->players[gu->myPlayerNum]->currentStats);
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
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				logOutput.Print("Got invalid player num %i in pause msg",player);
			}
			else if (!skipping) {
				gs->paused=!!inbuf[inbufpos+2];
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
				HandleChatMsg(s, player, false);
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
			net->SendNewFrame(gs->frameNum);
			SimFrame();
#ifdef SYNCCHECK
			net->SendSyncResponse(gu->myPlayerNum, gs->frameNum, CSyncChecker::GetChecksum());
			CSyncChecker::NewFrame();
#endif
			if(gameServer && serverNet && serverNet->playbackDemo)	//server doesnt update framenums automatically while playing demo
				gameServer->serverframenum=gs->frameNum;

			if(creatingVideo && net->playbackDemo){
				POP_CODE_MODE;
				return true;
			}
			lastLength=5;
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

		case NETMSG_AICOMMANDS:{
			int u, c;
			const int player = inbuf[inbufpos+3];
			if (player>=MAX_PLAYERS || player<0) {
				logOutput.Print("Got invalid player num %i in aicommands msg",player);
				lastLength = *((short int*)&inbuf[inbufpos+1]);
				break;
			}

			unsigned char* ptr = &inbuf[inbufpos + 4];

// FIXME -- hackish
#define UNPACK(type)  *((type*)ptr); ptr = ptr + sizeof(type);

			// parse the unit list
			vector<int> unitIDs;
			const int unitCount = UNPACK(short);
			for (u = 0; u < unitCount; u++) {
				const int unitID = UNPACK(short);
				unitIDs.push_back(unitID);
			}
			// parse the command list
			vector<Command> commands;
			const int commandCount = UNPACK(short);
			for (c = 0; c < commandCount; c++) {
				Command cmd;
				cmd.id               = UNPACK(int);
				cmd.options          = UNPACK(unsigned char);
				const int paramCount = UNPACK(short);
				for (int p = 0; p < paramCount; p++) {
					const float param = UNPACK(float);
					cmd.params.push_back(param);
				}
				commands.push_back(cmd);
			}
			// apply the commands
			for (c = 0; c < commandCount; c++) {
				for (u = 0; u < unitCount; u++) {
					selectedUnits.AiOrder(unitIDs[u], commands[c]);
				}
			}
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_SYNCREQUEST:{
			// TODO rename this net message, change error msg, etc.
			ENTER_MIXED;
			int frame=*((int*)&inbuf[inbufpos+1]);
			if(frame!=gs->frameNum){
				logOutput.Print("Sync request for wrong frame (%i instead of %i)", frame, gs->frameNum);
			}
#ifdef PROFILE_TIME
			net->SendCPUUsage(profiler.profile["Sim time"].percent);
#else
			net->SendCPUUsage(0.30f);
#endif
			lastLength=5;
			ENTER_SYNCED;
			break;}

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
							mouse->wasLocked = mouse->locked;
							if(!mouse->locked){
								mouse->locked = true;
								mouse->HideMouse();
							}
							mouse->preControlCamNum = mouse->currentCamControllerNum;
							mouse->currentCamControllerNum = 0;
							mouse->currentCamController = mouse->camControllers[0];	//set fps mode
							mouse->CameraTransition(1.0f);
							((CFPSController*)mouse->camControllers[0])->pos = unit->midPos;
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
	if (skipping) {
		return;
	}
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

		bool disableTracker = false;
		if (camMove[0]) {
			movement.y += gu->lastFrameTime;
			disableTracker = true;
		}
		if (camMove[1]) {
			movement.y -= gu->lastFrameTime;
			disableTracker = true;
		}
		if (camMove[3]) {
			movement.x += gu->lastFrameTime;
			disableTracker = true;
		}
		if (camMove[2]) {
			movement.x -= gu->lastFrameTime;
			disableTracker = true;
		}

		CCameraController* camCtrl = mouse->currentCamController;
		if (disableTracker && camCtrl->DisableTrackerByKey()) {
			unitTracker.Disable();
		}
		movement.z = cameraSpeed;
		camCtrl->KeyMove(movement);

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

	mouse->currentCamController->Update();

	if(chatting && !userWriting){
		consoleHistory->AddLine(userInput);
		string msg = userInput;
		string pfx = "";
		if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
			pfx = userInput.substr(0, 2);
			msg = userInput.substr(2);
		}
		if ((msg[0] == '/') && (msg[1] == '/')) {
			msg = msg.substr(1);
		}
		userInput = pfx + msg;
		SendNetChat(userInput);
		chatting=false;
		userInput="";
		writingPos = 0;
	}

	if(inMapDrawer->wantLabel && !userWriting){
		if(userInput.size()>200){	//avoid troubles with to long lines
			userInput=userInput.substr(0,200);
			writingPos = (int)userInput.length();
		}
		inMapDrawer->CreatePoint(inMapDrawer->waitingPoint,userInput);
		inMapDrawer->wantLabel=false;
		userInput="";
		writingPos = 0;
		ignoreChar=0;
	}
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

	if (drawFpsHUD) {

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
		glTranslatef3(-unit->pos - (unit->speed * gu->timeOffset));
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
		glTranslatef(0.02f, 0.65f, 0);
		glScalef(0.03f,0.03f,0.03f);
		glColor4d(0.2f,0.8f,0.2f,0.8f);
		font->glPrint("Health %.0f",unit->health);
		glPopMatrix();

		if(gs->players[gu->myPlayerNum]->myControl.mouse2){
			glPushMatrix();
			glTranslatef(0.02f,0.7f,0);
			glScalef(0.03f,0.03f,0.03f);
			glColor4d(0.2f,0.8f,0.2f,0.8f);
			font->glPrint("Free fire mode");
			glPopMatrix();
		}
		for(int a=0;a<unit->weapons.size();++a){
			glPushMatrix();
			glTranslatef(0.02f,0.32f-a*0.04f,0);
			glScalef(0.0225f,0.03f,0.03f);
			CWeapon* w=unit->weapons[a];
			WeaponDef* wd=w->weaponDef;
			if(!wd->isShield){
				if(w->reloadStatus>gs->frameNum){
					glColor4d(0.8f,0.2f,0.2f,0.8f);
					font->glPrint("%s: Reloading",wd->description.c_str());
				} else if(!w->angleGood){
					glColor4d(0.6f,0.6f,0.2f,0.8f);
					font->glPrint("%s: Aiming",wd->description.c_str());
				} else {
					glColor4d(0.2f,0.8f,0.2f,0.8f);
					font->glPrint("%s: Ready",wd->description.c_str());
				}
			}
			glPopMatrix();
		}
	} // end IF drawFpsHUD

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
	net->SendChat(gu->myPlayerNum, msg);
	if (net->playbackDemo) {
		HandleChatMsg(msg, gu->myPlayerNum, true);
	}
}


static void SetBoolArg(bool& value, const std::string& str, const char* cmd)
{
	char* end;
	const char* start = str.c_str() + strlen(cmd);
	const int num = strtol(start, &end, 10);
	if (end != start) {
		value = (num != 0);
	} else {
		value = !value;
	}
}


void CGame::HandleChatMsg(std::string s, int player, bool demoPlayer)
{
	// Player chat messages
	if (!noSpectatorChat || !gs->players[player]->spectator || !gu->spectating) {
		LogNetMsg(s, player);
	}

	globalAI->GotChatMsg(s.c_str(),player);
	CScriptHandler::Instance().chosenScript->GotChatMsg(s, player);

	if(s.find(".cheat")==0 && player==0){
		SetBoolArg(gs->cheatEnabled, s, ".cheat");
		if (gs->cheatEnabled) {
			logOutput.Print("Cheating!");
		} else {
			logOutput.Print("No more cheating");
		}
	}
	else if(s.find(".nocost")==0 && player==0 && gs->cheatEnabled){
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
	else if (s.find(".crash") == 0 && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}
	else if (s.find(".exception") == 0 && gs->cheatEnabled) {
		throw std::runtime_error("Exception test");
	}
	else if (s.find(".divbyzero") == 0 && gs->cheatEnabled) {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}
	else if (s.find(".resync") == 0 && gs->cheatEnabled) {
		CObject* o = CObject::GetSyncedObjects();
		for (; o; o = o->GetNext()) {
			creg::Class* c = o->GetClass();
			logOutput.Print("%s\n", c->name.c_str());
			for (std::vector<creg::Class::Member*>::const_iterator m = c->members.begin(); m != c->members.end(); ++m) {
				logOutput.Print("  %s\n", (*m)->name);
			}
		}
	}
	else if (s.find(".desync") == 0 && gs->cheatEnabled) {
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
	else if (s.find(".fakedesync") == 0 && gs->cheatEnabled && gameServer && serverNet) {
		gameServer->fakeDesync = true;
		logOutput.Print("Fake desyncing.");
	}
	else if (s.find(".reset") == 0 && gs->cheatEnabled) {
		CSyncDebugger::GetInstance()->Reset();
		logOutput.Print("Resetting sync debugger.");
	}
#endif
	else if(s==".spectator" && (gs->cheatEnabled || net->playbackDemo)){
		gs->players[player]->spectator=true;
		if(player==gu->myPlayerNum){
			gu->spectating = true;
			gu->spectatingFullView = gu->spectating;
			gu->spectatingFullSelect = false;
			CLuaUI::UpdateTeams();
		}
	}
	else if(s.find(".team")==0 && (gs->cheatEnabled || net->playbackDemo)){
		int team=atoi(&s.c_str()[s.find(" ")]);
		if(team>=0 && team<gs->activeTeams){
			gs->players[player]->team=team;
			gs->players[player]->spectator=false;
			if(player==gu->myPlayerNum){
				selectedUnits.ClearSelected();
				gu->spectating = false;
				gu->spectatingFullView = gu->spectating;
				gu->spectatingFullSelect = false;
				gu->myTeam = team;
				gu->myAllyTeam = gs->AllyTeam(gu->myTeam);
				grouphandler->team = gu->myTeam;
				CLuaUI::UpdateTeams();
			}
		}
	}
	else if(s.find(".atm")==0 && gs->cheatEnabled){
		int team=gs->players[player]->team;
		gs->Team(team)->AddMetal(1000);
		gs->Team(team)->AddEnergy(1000);
	}

	else if (s.find(".give") == 0 && gs->cheatEnabled) {
		int team = gs->players[player]->team;
		int p1 = s.rfind(" @"), p2 = s.find(",", p1 + 1), p3 = s.find(",", p2 + 1);

		if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
			logOutput.Print("Someone is spoofing invalid .give messages!");

		float3 pos(atof(&s.c_str()[p1 + 2]), atof(&s.c_str()[p2 + 1]), atof(&s.c_str()[p3 + 1]));
		s = s.substr(0, p1);

		if (s.find(" all") != string::npos) {
			// player entered ".give all"
			int sqSize = (int) ceil(sqrt((float) unitDefHandler->numUnits));
			int currentNumUnits = gs->Team(team)->units.size();
			int numRequestedUnits = unitDefHandler->numUnits;

			// make sure team unit-limit not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->maxUnits)
				numRequestedUnits = uh->maxUnits - currentNumUnits;

			for (int a = 1; a <= numRequestedUnits; ++a) {
				float posx = pos.x + (a % sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float posz = pos.z + (a / sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float3 pos2 = float3(posx, pos.y, posz);
				const CUnit* unit = unitLoader.LoadUnit(unitDefHandler->unitDefs[a].name, pos2, team, false, 0, NULL);

				if (unit) {
					unitLoader.FlattenGround(unit);
				}
			}
		} else if (((s.rfind(" ")) != string::npos) && ((s.length() - s.rfind(" ") - 1) > 0)) {
			// player entered ".give <unitname>" or ".give <amount> <unitname>"
			string unitName = s.substr(s.rfind(" ")+1,s.length() - s.rfind(" ") -1);
			string tempUnitName = s.substr(s.find(" "), s.rfind(" ") + 1 - s.find(" "));
			int i = tempUnitName.find_first_not_of(" ");

			if (i != string::npos) {
				tempUnitName = tempUnitName.substr(i, tempUnitName.find_last_not_of(" ") + 1 - i);
			}

			bool createNano = (tempUnitName.find("nano") != string::npos);
			int numRequestedUnits = 1;
			int currentNumUnits = gs->Team(team)->units.size();

			if (currentNumUnits < uh->maxUnits) {
				int j = tempUnitName.find_first_of("0123456789");

				if (j != string::npos) {
					tempUnitName = tempUnitName.substr(j, tempUnitName.find_last_of("0123456789") + 1 - j);
					numRequestedUnits = atoi(tempUnitName.c_str());

					// make sure team unit-limit not exceeded
					if ((currentNumUnits + numRequestedUnits) > uh->maxUnits)
						numRequestedUnits = uh->maxUnits - currentNumUnits;
				}

				UnitDef* unitDef = unitDefHandler->GetUnitByName(unitName);

				if (unitDef != NULL) {
					int xsize = unitDef->xsize;
					int zsize = unitDef->ysize;
					int squareSize = (int) ceil(sqrt((float) numRequestedUnits));
					int total = numRequestedUnits;

					float3 minpos = pos;
					minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
					minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

					for (int z = 0; z < squareSize; ++z) {
						for (int x = 0; x < squareSize && total > 0; ++x) {
							float minposx = minpos.x + x * xsize * SQUARE_SIZE;
							float minposz = minpos.z + z * zsize * SQUARE_SIZE;
							const float3 upos(minposx, minpos.y, minposz);
							const CUnit* unit = unitLoader.LoadUnit(unitName, upos, team, createNano, 0, NULL);

							if (unit) {
								unitLoader.FlattenGround(unit);
							}
							--total;
						}
					}

					logOutput.Print("Giving %i %s to team %i", numRequestedUnits, unitName.c_str(), team);
				}
				else {
					FeatureDef* featureDef = featureHandler->GetFeatureDef(unitName);
					if (featureDef) {
						int xsize = featureDef->xsize;
						int zsize = featureDef->ysize;
						int squareSize = (int) ceil(sqrt((float) numRequestedUnits));
						int total = numRequestedUnits;

						float3 minpos = pos;
						minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
						minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

						for (int z = 0; z < squareSize; ++z) {
							for (int x = 0; x < squareSize && total > 0; ++x) {
								float minposx = minpos.x + x * xsize * SQUARE_SIZE;
								float minposz = minpos.z + z * zsize * SQUARE_SIZE;
								const float3 upos(minposx, minpos.y, minposz);
								CFeature* feature = SAFE_NEW CFeature();
								feature->Initialize(upos, featureDef, 0, 0, team, "");
								--total;
							}
						}

						logOutput.Print("Giving %i %s (feature) to team %i", numRequestedUnits, unitName.c_str(), team);
					}
					else {
						logOutput.Print(unitName + " is not a valid unitname");
					}
				}
			}
			else {
				logOutput.Print("Unable to give any more units to team %i", team);
			}
		}
	}

	else if(s.find(".take")==0 && (!gu->spectating || gs->cheatEnabled)){
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
	else if ((s.find(".kickbynum") == 0) && (player == 0) && gameServer && serverNet) {
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
	else if ((s.find(".kick") == 0) && (player == 0) && gameServer && serverNet) {
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
	else if ((s.find(".nospectatorchat") == 0) && (player == 0)) {
		SetBoolArg(noSpectatorChat, s, ".nospectatorchat");
		logOutput.Print("Spectators %s chat", noSpectatorChat ? "can" : "can not");
	}
	else if ((s.find(".nopause") == 0) && (player == 0)) {
		SetBoolArg(gamePausable, s, ".nopause");
	}
	else if ((s.find(".nohelp") == 0) && (player == 0)) {
		SetBoolArg(gs->noHelperAIs, s, ".nohelp");
		selectedUnits.PossibleCommandChange(NULL);
		if (gs->noHelperAIs) {
			// remove any current GroupAIs
			set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
			set<CUnit*>::iterator it;
      for(it = teamUnits.begin(); it != teamUnits.end(); ++it) {
      	CUnit* unit = *it;
				if (unit->group && (unit->group->id > 9)) {
					unit->SetGroup(NULL);
				}
			}
		}
		logOutput.Print("GroupAI and LuaUI control is %s",
		                gs->noHelperAIs ? "disabled" : "enabled");
	}
	else if ((s.find(".setmaxspeed") == 0) && (player == 0)) {
		maxUserSpeed = atof(s.substr(12).c_str());
		if (gs->userSpeedFactor > maxUserSpeed) {
			gs->userSpeedFactor = maxUserSpeed;
		}
	}
	else if ((s.find(".setminspeed") == 0) && (player == 0)) {
		minUserSpeed = atof(s.substr(12).c_str());
		if (gs->userSpeedFactor < minUserSpeed) {
			gs->userSpeedFactor = minUserSpeed;
		}
	}
	else if (s.find(".skip") == 0) {
		if (!demoPlayer && ((player != 0) || !gs->cheatEnabled)) {
			logOutput.Print(".skip only works in replay, and when cheating\n");
		} else {
			Skip(s, demoPlayer);
		}
	}
	else if ((s.find(".reloadcob") == 0) && gs->cheatEnabled && (player == 0)) {
		ReloadCOB(s, player);
	}
	else if ((s.find(".devlua") == 0) && (player == 0) && gs->cheatEnabled) {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, s, ".devlua");
		CLuaHandle::SetDevMode(devMode);
		if (devMode) {
			logOutput.Print("Lua devmode enabled, this can cause desyncs");
		} else {
			logOutput.Print("Lua devmode disabled");
		}
	}
	else if (s.find(".editdefs")==0 && player==0 && gs->cheatEnabled){
		SetBoolArg(gs->editDefsEnabled, s, ".editdefs");
		if (gs->editDefsEnabled) {
			logOutput.Print("Definition Editing!");
		} else {
			logOutput.Print("No definition Editing");
		}
	}
	else if ((s.find(".luarules") == 0) && (gs->frameNum > 1)) {
		if (gs->useLuaRules) {
			if (s.find(" reload", 9) == 9) {
				if (player != 0) {
					logOutput.Print("Only the host player can reload synced scripts\n");
				} else if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to reload synced scripts\n");
				} else {
					CLuaRules::FreeHandler();
					CLuaRules::LoadHandler();
					if (luaRules) {
						logOutput.Print("LuaRules reloaded\n");
					} else {
						logOutput.Print("LuaRules reload failed\n");
					}
				}
			}
			else if (s.find(" disable", 9) == 9) {
				if (player != 0) {
					logOutput.Print("Only the host player can disable synced scripts\n");
				} else if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts\n");
				} else {
					CLuaRules::FreeHandler();
					logOutput.Print("LuaRules disabled\n");
				}
			}
			else if (luaRules) {
				luaRules->GotChatMsg(s, player);
			}
			else {
				logOutput.Print("LuaRules is not enabled\n");
			}
		}
	}
	else if ((s.find(".luagaia") == 0) && (gs->frameNum > 1)) {
		if (gs->useLuaGaia) {
			if (s.find(" reload", 8) == 8) {
				if (player != 0) {
					logOutput.Print("Only the host player can reload synced scripts\n");
				} else if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to reload synced scripts\n");
				} else {
					CLuaGaia::FreeHandler();
					CLuaGaia::LoadHandler();
					if (luaGaia) {
						logOutput.Print("LuaGaia reloaded\n");
					} else {
						logOutput.Print("LuaGaia reload failed\n");
					}
				}
			}
			else if (s.find(" disable", 8) == 8) {
				if (player != 0) {
					logOutput.Print("Only the host player can disable synced scripts\n");
				} else if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts\n");
				} else {
					CLuaGaia::FreeHandler();
					logOutput.Print("LuaGaia disabled\n");
				}
			}
			else if (luaGaia) {
				luaGaia->GotChatMsg(s, player);
			}
			else {
				logOutput.Print("LuaGaia is not enabled\n");
			}
		}
	}
	else if ((s.find(".luacob") == 0) && (gs->frameNum > 1)) {
		if (s.find(" reload", 7) == 7) {
			if (player != 0) {
				logOutput.Print("Only the host player can reload synced scripts\n");
			} else if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to reload synced scripts\n");
			} else {
				CLuaCob::FreeHandler();
				CLuaCob::LoadHandler();
				if (luaCob) {
					logOutput.Print("LuaCob reloaded\n");
				} else {
					logOutput.Print("LuaCob reload failed\n");
				}
			}
		}
		else if (s.find(" disable", 7) == 7) {
			if (player != 0) {
				logOutput.Print("Only the host player can disable synced scripts\n");
			} else if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to disable synced scripts\n");
			} else {
				CLuaCob::FreeHandler();
				logOutput.Print("LuaCob disabled\n");
			}
		}
		else if (luaCob) {
			luaCob->GotChatMsg(s, player);
		}
		else {
			logOutput.Print("LuaCob is not enabled\n");
		}
	}
}


void CGame::LogNetMsg(const string& msg, int playerID)
{
	string s = msg;

	CPlayer* player = gs->players[playerID];
	const bool myMsg = (playerID == gu->myPlayerNum);

	bool allyMsg = false;
	bool specMsg = false;
	userInputPrefix = "";

	if ((s.length() >= 2) && (s[1] == ':')) {
		const char lower = tolower(s[0]);
		if (lower == 'a') {
			allyMsg = true;
			s = s.substr(2);
			userInputPrefix = "a:";
		}
		else if (lower == 's') {
			specMsg = true;
			s = s.substr(2);
			userInputPrefix = "s:";
		}
	}

	string label;
	if (player->spectator) {
		label = "[" + player->playerName + "] ";
	} else {
		label = "<" + player->playerName + "> ";
	}

	s.substr(0, 255);

	if (!s.empty()) {
		if (allyMsg) {
			const int msgAllyTeam = gs->AllyTeam(player->team);
			const bool allied = gs->Ally(msgAllyTeam, gu->myAllyTeam);
			if ((allied && !player->spectator) || gu->spectating) {
				logOutput.Print(label + "Allies: " + s);
				sound->PlaySample(chatSound, 5);
			}
		}
		else if (specMsg) {
			if (gu->spectating || myMsg) {
				logOutput.Print(label + "Spectators: " + s);
				sound->PlaySample(chatSound, 5);
			}
		}
		else {
			logOutput.Print(label + s);
			sound->PlaySample(chatSound, 5);
		}
	}
}


void CGame::Skip(const std::string& msg, bool demoPlayer)
{
	if ((skipping < 0) || (skipping > 2)) {
		logOutput.Print("ERROR: skipping appears to be busted (%i)\n", skipping);
		skipping = 0;
	}
	const string timeStr = msg.substr(6);
	const int startFrame = gs->frameNum;
	int endFrame;

	// parse the skip time
	if (timeStr[0] == 'f') {        // skip to frame
		endFrame = atoi(timeStr.c_str() + 1);
	} else if (timeStr[0] == '+') { // relative time
		endFrame = startFrame + (GAME_SPEED * atoi(timeStr.c_str() + 1));
	} else {                        // absolute time
		endFrame = GAME_SPEED * atoi(timeStr.c_str());
	}

	if (endFrame <= startFrame) {
		logOutput.Print("Already passed %i (%i)\n", endFrame / GAME_SPEED, endFrame);
		return;
	}
	if (gs->paused) {
		logOutput.Print("Can not skip while paused\n");
		return;
	}

	const int totalFrames = endFrame - startFrame;
	const float seconds = (float)(totalFrames) / (float)GAME_SPEED;

	CSound* tmpSound = sound;
	sound = SAFE_NEW CNullSound;

	skipping++;
	{
		const float oldSpeed     = gs->speedFactor;
		const float oldUserSpeed = gs->userSpeedFactor;
		const float speed = 1.0f;
		gs->speedFactor     = speed;
		gs->userSpeedFactor = speed;

		Uint32 gfxLastTime = SDL_GetTicks() - 10000; // force the first draw

		while (endFrame > gs->frameNum) {
			if (demoPlayer) {
				Update();
			} else {
				SimFrame();
				if (gameServer) {
					gameServer->serverframenum = gs->frameNum;
				}
			}
			// draw something so that users don't file bug reports
			const Uint32 gfxTime = SDL_GetTicks();
			if ((gfxTime - gfxLastTime) > 100) { // 10fps
				gfxLastTime = gfxTime;

				const int framesLeft = (endFrame - gs->frameNum);
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor3f(0.5f, 1.0f, 0.5f);
				font->glPrintCentered(0.5f, 0.55f, 2.5f,
					"Skipping %.1f game seconds", seconds);
				glColor3f(1.0f, 1.0f, 1.0f);
				font->glPrintCentered(0.5f, 0.45f, 2.0f,
					"(%i frames left)", framesLeft);

				const float ff = (float)framesLeft / (float)totalFrames;
				glDisable(GL_TEXTURE_2D);
				const float b = 0.004f; // border
				const float yn = 0.35f;
				const float yp = 0.38f;
				glColor3f(0.2f, 0.2f, 1.0f);
				glRectf(0.25f - b, yn - b, 0.75f + b, yp + b);
				glColor3f(0.25f + (0.75f * ff), 1.0f - (0.75f * ff), 0.0f);
				glRectf(0.5 - (0.25f * ff), yn, 0.5f + (0.25f * ff), yp);

				SDL_GL_SwapBuffers();
			}
		}

		if (!demoPlayer) {
			gu->gameTime    += seconds;
			gu->modGameTime += seconds;
		}

		gs->speedFactor     = oldSpeed;
		gs->userSpeedFactor = oldUserSpeed;
	}
	skipping--;

	delete sound;
	sound = tmpSound;

	logOutput.Print("Skipped %.1f seconds\n", seconds);
}


void CGame::ReloadCOB(const string& msg, int player)
{
	if (!gs->cheatEnabled) {
		logOutput.Print("reloadcob can only be used if cheating is enabled");
		return;
	}
	const string unitName = msg.substr(11);
	if (unitName.empty()) {
		logOutput.Print("Missing unit name");
		return;
	}
	const string name = "scripts/" + unitName + ".cob";
	const CCobFile* oldScript = GCobEngine.GetScriptAddr(name);
	if (oldScript == NULL) {
		logOutput.Print("Unknown cob script: %s", name.c_str());
		return;
	}
	CCobFile* newScript = &GCobEngine.ReloadCobFile(name);
	int count = 0;
	for (int i = 0; i < MAX_UNITS; i++) {
		CUnit* unit = uh->units[i];
		if (unit != NULL) {
			if (unit->cob->GetScriptAddr() == oldScript) {
				count++;
				delete unit->cob;
				unit->cob = SAFE_NEW CCobInstance(*newScript, unit);
				delete unit->localmodel;
				unit->localmodel =
					modelParser->CreateLocalModel(unit->model, &unit->cob->pieces);
			}
		}
	}
	logOutput.Print("Update cob script for %i units", count);
}


void CGame::SelectUnits(const string& line)
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
			if (!gu->spectatingFullSelect) {
				const set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
				if (teamUnits.find(unit) == teamUnits.end()) {
					continue; // not mine to select
				}
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


void CGame::SelectCycle(const string& command)
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
