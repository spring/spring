// // Game.cpp: implementation of the CGame class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>
#include <locale>

#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include <SDL_keyboard.h>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <SDL_types.h>
#include <SDL_events.h>
#include <SDL_video.h>

#include "Game.h"
#include "float.h"
#include "Camera.h"
#include "Camera/CameraController.h"
#include "Camera/FPSController.h"
#include "Camera/OverheadController.h"
#include "CameraHandler.h"
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
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "NetProtocol.h"
#include "DemoRecorder.h"
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
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUnsyncedCtrl.h"
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
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/COB/CobEngine.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTracker.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "StartScripts/Script.h"
#include "StartScripts/ScriptHandler.h"
#include "Sync/SyncedPrimitiveIO.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound.h"
#include "System/Platform/NullSound.h"
#include "Platform/Clipboard.h"
#include "UI/CommandColors.h"
#include "UI/CursorIcons.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GameSetupDrawer.h"
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
#include "UI/TooltipConsole.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ExplosionGenerator.h"

#ifndef NO_AVI
#include "Platform/Win/AVIGenerator.h"
#endif

#ifdef DIRECT_CONTROL_ALLOWED
#include "myMath.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#endif

#include "mmgr.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

GLfloat LightDiffuseLand[] = { 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat LightAmbientLand[] = { 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat FogLand[] =          { 0.7f, 0.7f, 0.8f, 0.0f };
GLfloat FogWhite[] =         { 1.0f, 1.0f, 1.0f, 0.0f };
GLfloat FogBlack[] =         { 0.0f, 0.0f, 0.0f, 0.0f };

extern Uint8 *keys;
extern bool globalQuit;
extern bool fullscreen;
extern string stupidGlobalMapname;


CGame* game = NULL;


CR_BIND(CGame, (std::string(""), std::string(""), NULL));

CR_REG_METADATA(CGame,(
	CR_RESERVED(4),//r3927
	CR_MEMBER(oldframenum),
//	CR_MEMBER(fps),
//	CR_MEMBER(thisFps),

//	CR_MEMBER(lastUpdate),
//	CR_MEMBER(lastMoveUpdate),
//	CR_MEMBER(lastModGameTimeMeasure),
//	CR_MEMBER(lastframe),

	CR_MEMBER(totalGameTime),

	CR_MEMBER(userInputPrefix),

	CR_MEMBER(lastTick),
	CR_MEMBER(chatSound),

	CR_MEMBER(hideInterface),
	CR_MEMBER(showFPS),
	CR_MEMBER(showClock),
	CR_MEMBER(crossSize),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(drawFpsHUD),

	CR_MEMBER(soundEnabled),
	CR_MEMBER(gameSoundVolume),

//	CR_MEMBER(script),
	CR_RESERVED(64),
	CR_POSTLOAD(PostLoad)
));


CGame::CGame(std::string mapname, std::string modName, CInfoConsole *ic)
: lastFrameTime(0),
  drawMode(notDrawing),
  drawSky(true),
  drawWater(true),
  drawGround(true)
{
	game = this;
	boost::thread thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

	leastQue = 0;
	timeLeft = 0.0f;
	consumeSpeed = 1.0f;

	infoConsole = ic;

	script = NULL;

	defsParser = NULL;

	time(&starttime);
	lastTick = clock();

	oldframenum = 0;

	for(int a = 0; a < 8; ++a) {
		camMove[a] = false;
	}
	for(int a = 0; a < 4; ++a) {
		camRot[a] = false;
	}

	fps = 0;
	thisFps = 0;
	totalGameTime = 0;

	creatingVideo = false;

	playing  = false;
	gameOver = false;
	allReady = false;
	skipping = 0;

	drawFpsHUD = true;
	drawMapMarks = true;
	hideInterface = false;

	windowedEdgeMove   = !!configHandler.GetInt("WindowedEdgeMove",   1);
	fullscreenEdgeMove = !!configHandler.GetInt("FullscreenEdgeMove", 1);

	showFPS   = !!configHandler.GetInt("ShowFPS",   0);
	showClock = !!configHandler.GetInt("ShowClock", 1);

	crossSize = configHandler.GetFloat("CrossSize", 10.0f);

	playerRoster.SetSortTypeByCode(
	  (PlayerRoster::SortType)configHandler.GetInt("ShowPlayerInfo", 1));

	const string inputTextGeo = configHandler.GetString("InputTextGeo", "");
	ParseInputTextGeometry("default");
	ParseInputTextGeometry(inputTextGeo);

	noSpectatorChat = false;

	chatting   = false;
	userInput  = "";
	writingPos = 0;
	userPrompt = "";

	consoleHistory = SAFE_NEW CConsoleHistory;
	wordCompletion = SAFE_NEW CWordCompletion;
	for (int pp = 0; pp < MAX_PLAYERS; pp++) {
	  wordCompletion->AddWord(gs->players[pp]->playerName, false, false, false);
	}

#ifdef DIRECT_CONTROL_ALLOWED
	oldPitch   = 0;
	oldHeading = 0;
	oldStatus  = 255;
#endif

	ENTER_UNSYNCED;
	sound = CSound::GetSoundSystem();
	gameSoundVolume = configHandler.GetInt("SoundVolume", 60) * 0.01f;
	soundEnabled = true;
	sound->SetVolume(gameSoundVolume);
	sound->SetUnitReplyVolume(configHandler.GetInt ("UnitReplySoundVolume", 80) * 0.01f);

	camera = SAFE_NEW CCamera();
	cam2 = SAFE_NEW CCamera();
	mouse = SAFE_NEW CMouseHandler();
	camHandler = SAFE_NEW CCameraHandler();
	selectionKeys = SAFE_NEW CSelectionKeyHandler();
	tooltip = SAFE_NEW CTooltipConsole();

	ENTER_MIXED;

	helper = SAFE_NEW CGameHelper(this);

	ENTER_SYNCED;

	modInfo.Init(modName.c_str());

	defsParser = SAFE_NEW LuaParser("gamedata/defs.lua",
	                                 SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	defsParser->GetTable("Spring");
	defsParser->AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	defsParser->EndTable();
	// run the parser
	if (!defsParser->Execute()) {
		throw content_error(defsParser->GetErrorLog());
	}
	const LuaTable root = defsParser->GetRoot();
	if (!root.IsValid()) {
		throw content_error("Error loading definitions");
	}
	// bail now if any of these tables in invalid
	// (makes searching for errors that much easier
	if (!root.SubTable("UnitDefs").IsValid()) {
		throw content_error("Error loading UnitDefs");
	}
	if (!root.SubTable("FeatureDefs").IsValid()) {
		throw content_error("Error loading FeatureDefs");
	}
	if (!root.SubTable("WeaponDefs").IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}
	if (!root.SubTable("ArmorDefs").IsValid()) {
		throw content_error("Error loading ArmorDefs");
	}
	if (!root.SubTable("MoveDefs").IsValid()) {
		throw content_error("Error loading MoveDefs");
	}

	explGenHandler = SAFE_NEW CExplosionGeneratorHandler();

	ENTER_UNSYNCED;
	shadowHandler = SAFE_NEW CShadowHandler();

	ENTER_SYNCED;
	ground = SAFE_NEW CGround();
	readmap = CReadMap::LoadMap (mapname);
	wind.LoadWind();
	moveinfo = SAFE_NEW CMoveInfo();
	groundDecals = SAFE_NEW CGroundDecalHandler();
	ReColorTeams();

	ENTER_UNSYNCED;

	guihandler = SAFE_NEW CGuiHandler();
	minimap = SAFE_NEW CMiniMap();

	ENTER_MIXED;
	ph = SAFE_NEW CProjectileHandler();

	ENTER_SYNCED;
	sensorHandler = SAFE_NEW CSensorHandler();
	damageArrayHandler = SAFE_NEW CDamageArrayHandler();
	unitDefHandler = SAFE_NEW CUnitDefHandler();

	ENTER_UNSYNCED;
	inMapDrawer = SAFE_NEW CInMapDraw();
	cmdColors.LoadConfig("cmdcolors.txt");

	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
	  wordCompletion->AddWord(uit->first + " ", false, true, false);
	}

	geometricObjects = SAFE_NEW CGeometricObjects();

	ENTER_SYNCED;
	qf = SAFE_NEW CQuadField();

	ENTER_MIXED;
	featureHandler = SAFE_NEW CFeatureHandler();

	ENTER_SYNCED;
	mapDamage = IMapDamage::GetMapDamage();
	loshandler = SAFE_NEW CLosHandler();
	radarhandler = SAFE_NEW CRadarHandler(false);

	ENTER_MIXED;
	uh = SAFE_NEW CUnitHandler();
	iconHandler = SAFE_NEW CIconHandler();
	unitDrawer = SAFE_NEW CUnitDrawer();
	fartextureHandler = SAFE_NEW CFartextureHandler();
	modelParser = SAFE_NEW C3DModelParser();

 	ENTER_SYNCED;
 	featureHandler->LoadFeaturesFromMap(CScriptHandler::Instance().chosenScript->loadGame);
 	pathManager = SAFE_NEW CPathManager();

 	delete defsParser;
	defsParser = NULL;

	ENTER_UNSYNCED;
	sky=CBaseSky::GetSky();

	resourceBar = SAFE_NEW CResourceBar();
	keyCodes = SAFE_NEW CKeyCodes();
	keyBindings = SAFE_NEW CKeyBindings();
	keyBindings->Load("uikeys.txt");

	water=CBaseWater::GetWater();
	for(int a=0;a<MAX_TEAMS;a++)
		grouphandlers[a] = SAFE_NEW CGroupHandler(a);

	globalAI = SAFE_NEW CGlobalAIHandler();

	PrintLoadMsg("Loading LuaCOB");
	CLuaCob::LoadHandler();
	if (gs->useLuaRules) {
		PrintLoadMsg("Loading LuaRules");
		CLuaRules::LoadHandler();
	}
	if (gs->useLuaGaia) {
		PrintLoadMsg("Loading LuaGaia");
		CLuaGaia::LoadHandler();
	}

	PrintLoadMsg("Finalizing...");

	ENTER_MIXED;
	if (true || !shadowHandler->drawShadows) { // FIXME ?
		for(int a = 0; a < 3; ++a) {
			LightAmbientLand[a] = unitDrawer->unitAmbientColor[a];
			LightDiffuseLand[a] = unitDrawer->unitSunColor[a];
		}
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand);		// Setup The Ambient Light
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand);		// Setup The Diffuse Light
		glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand);		// Setup The Diffuse Light
		glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);
	}

	logOutput.Print("Spring %s",VERSION_STRING);

	CPlayer* p = gs->players[gu->myPlayerNum];
	if(!gameSetup || net->localDemoPlayback) {
		p->playerName = configHandler.GetString("name", "");
	}
	else
	{
		GameSetupDrawer::Enable();
	}

	lastframe = SDL_GetTicks();
	lastModGameTimeMeasure = lastframe;
	lastUpdate = lastframe;
	lastMoveUpdate = lastframe;
	lastUpdateRaw = lastframe;
	updateDeltaSeconds = 0.0f;

	glFogfv(GL_FOG_COLOR,FogLand);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, gu->viewRange * 0.98f);
	glFogf(GL_FOG_DENSITY, 1.0f);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glEnable(GL_FOG);
	glClearColor(FogLand[0], FogLand[1], FogLand[2], 0.0f);
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

	net->loading = false;
	thread.join();

	activeController = this;

	chatSound = sound->GetWaveId("sounds/beep4.wav");

	UnloadStartPicture();
	//sending your playername to the server indicates that you are finished loading
	net->SendPlayerName(gu->myPlayerNum, p->playerName);

	if (net->localDemoPlayback) // auto-start when playing a demo in local mode
		net->SendStartPlaying();

	lastCpuUsageTime = gu->gameTime + 10;
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

	if (gameServer)delete gameServer;gameServer         = NULL;

	globalAI->PreDestroy ();
	delete globalAI;           globalAI           = NULL;
//	delete grouphandler;       grouphandler       = NULL;
	for(int a=0;a<MAX_TEAMS;a++) {
		delete grouphandlers[a];grouphandlers[a]   = NULL;}

	delete water;              water              = NULL;
	delete sky;                sky                = NULL;
	delete resourceBar;        resourceBar        = NULL;

	delete uh;                 uh                 = NULL;
	delete unitDrawer;         unitDrawer         = NULL;
	delete featureHandler;     featureHandler     = NULL;
	delete geometricObjects;   geometricObjects   = NULL;
	delete ph;                 ph                 = NULL;
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
	delete camHandler;         camHandler         = NULL;
	delete helper;             helper             = NULL;
	delete shadowHandler;      shadowHandler      = NULL;
	delete moveinfo;           moveinfo           = NULL;
	delete unitDefHandler;     unitDefHandler     = NULL;
	delete damageArrayHandler; damageArrayHandler = NULL;
	delete hpiHandler;         hpiHandler         = NULL;
	delete archiveScanner;     archiveScanner     = NULL;
	delete modelParser;        modelParser        = NULL;
	delete fartextureHandler;  fartextureHandler  = NULL;
	delete camera;             camera             = NULL;
	delete cam2;               cam2               = NULL;
	delete infoConsole;        infoConsole        = NULL;
	delete consoleHistory;     consoleHistory     = NULL;
	delete wordCompletion;     wordCompletion     = NULL;
	delete explGenHandler;     explGenHandler     = NULL;
	CCategoryHandler::RemoveInstance();
	CColorMap::DeleteColormaps();
}

void CGame::PostLoad()
{
	if (gameServer) {
		gameServer->PostLoad(lastTick, gs->frameNum);
	}
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
		if (k == SDLK_ESCAPE) {
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

	if (userWriting) {
		int actionIndex;
		for (actionIndex = 0; actionIndex < (int)actionList.size(); actionIndex++) {
			const CKeyBindings::Action& action = actionList[actionIndex];

			if (action.command == "edit_return") {
				userWriting=false;
				writingPos = 0;
				if (k == SDLK_RETURN) {
					keys[k] = false; //prevent game start when server chats
				}
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
				break;
			}
			else if ((action.command == "edit_escape") &&
			         (chatting || inMapDrawer->wantLabel)) {
				if (chatting) {
					consoleHistory->AddLine(userInput);
				}
				userWriting=false;
				chatting=false;
				inMapDrawer->wantLabel=false;
				userInput="";
				writingPos = 0;
				break;
			}
			else if (action.command == "edit_complete") {
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
				break;
			}
			else if (action.command == "chatswitchall") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput = userInput.substr(2);
					writingPos = max(0, writingPos - 2);
				}
				userInputPrefix = "";
				break;
			}
			else if (action.command == "chatswitchally") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 'a';
				} else {
					userInput = "a:" + userInput;
					writingPos += 2;
				}
				userInputPrefix = "a:";
				break;
			}
			else if (action.command == "chatswitchspec") {
				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					userInput[0] = 's';
				} else {
					userInput = "s:" + userInput;
					writingPos += 2;
				}
				userInputPrefix = "s:";
				break;
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
				break;
			}

			else if (action.command == "edit_backspace") {
				if (!userInput.empty() && (writingPos > 0)) {
					userInput.erase(writingPos - 1, 1);
					writingPos--;
				}
				break;
			}
			else if (action.command == "edit_delete") {
				if (!userInput.empty() && (writingPos < (int)userInput.size())) {
					userInput.erase(writingPos, 1);
				}
				break;
			}
			else if (action.command == "edit_home") {
				writingPos = 0;
				break;
			}
			else if (action.command == "edit_end") {
				writingPos = (int)userInput.length();
				break;
			}
			else if (action.command == "edit_prev_char") {
				writingPos = max(0, min((int)userInput.length(), writingPos - 1));
				break;
			}
			else if (action.command == "edit_next_char") {
				writingPos = max(0, min((int)userInput.length(), writingPos + 1));
				break;
			}
			else if (action.command == "edit_prev_word") {
				// prev word
				const char* s = userInput.c_str();
				int p = writingPos;
				while ((p > 0) && !isalnum(s[p - 1])) { p--; }
				while ((p > 0) &&  isalnum(s[p - 1])) { p--; }
				writingPos = p;
				break;
			}
			else if (action.command == "edit_next_word") {
				const int len = (int)userInput.length();
				const char* s = userInput.c_str();
				int p = writingPos;
				while ((p < len) && !isalnum(s[p])) { p++; }
				while ((p < len) &&  isalnum(s[p])) { p++; }
				writingPos = p;
				break;
			}
			else if ((action.command == "edit_prev_line") && chatting) {
				userInput = consoleHistory->PrevLine(userInput);
				writingPos = (int)userInput.length();
				break;
			}
			else if ((action.command == "edit_next_line") && chatting) {
				userInput = consoleHistory->NextLine(userInput);
				writingPos = (int)userInput.length();
				break;
			}
		}

		if (actionIndex != actionList.size()) {
			ignoreNextChar = true; // the key was used, ignore it  (ex: alt+a)
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
	bool notfound1=false;

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
	else if (cmd == "advshading") {
		static bool canUse = unitDrawer->advShading;
		if (canUse) {
			if (!action.extra.empty()) {
				unitDrawer->advShading = !!atoi(action.extra.c_str());
			} else {
				unitDrawer->advShading = !unitDrawer->advShading;
			}
			logOutput.Print("Advanced shading %s",
			                unitDrawer->advShading ? "enabled" : "disabled");
		}
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
		camHandler->SetCameraMode(0);
	}
	else if (cmd == "viewta") {
		camHandler->SetCameraMode(1);
	}
	else if (cmd == "viewtw") {
		camHandler->SetCameraMode(2);
	}
	else if (cmd == "viewrot") {
		camHandler->SetCameraMode(3);
	}
	else if (cmd == "viewfree") {
		camHandler->SetCameraMode(4);
	}
	else if (cmd == "viewov") {
		camHandler->SetCameraMode(5);
	}
	else if (cmd == "viewlua") {
		camHandler->SetCameraMode(6);
	}
	else if (cmd == "viewtaflip") {
		COverheadController* taCam =
			dynamic_cast<COverheadController*>(camHandler->camControllers[1]);
		if (taCam) {
			if (!action.extra.empty()) {
				taCam->flipped = !!atoi(action.extra.c_str());
			} else {
				taCam->flipped = !taCam->flipped;
			}
		}
	}
	else if (cmd == "viewsave") {
		camHandler->SaveView(action.extra);
	}
	else if (cmd == "viewload") {
		camHandler->LoadView(action.extra);
	}
	else if (cmd == "viewselection") {
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (!selUnits.empty()) {
			float3 pos(0.0f, 0.0f, 0.0f);
			CUnitSet::const_iterator it;
			for (it = selUnits.begin(); it != selUnits.end(); ++it) {
				pos += (*it)->midPos;
			}
			pos /= (float)selUnits.size();
			camCtrl->SetPos(pos);
			camHandler->CameraTransition(0.6f);
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
			gu->spectatingFullView   = !!(mode & 1);
			gu->spectatingFullSelect = !!(mode & 2);
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
			gu->spectatingFullSelect = gu->spectatingFullView;
		}
		CLuaUI::UpdateTeams();
	}
	else if (cmd == "group") {
		const char* c = action.extra.c_str();
		const int t = c[0];
		if ((t >= '0') && (t <= '9')) {
			const int team = (t - '0');
			do { c++; } while ((c[0] != 0) && isspace(c[0]));
			grouphandlers[gu->myTeam]->GroupCommand(team, c);
		}
	}
	else if (cmd == "group0") {
		grouphandlers[gu->myTeam]->GroupCommand(0);
	}
	else if (cmd == "group1") {
		grouphandlers[gu->myTeam]->GroupCommand(1);
	}
	else if (cmd == "group2") {
		grouphandlers[gu->myTeam]->GroupCommand(2);
	}
	else if (cmd == "group3") {
		grouphandlers[gu->myTeam]->GroupCommand(3);
	}
	else if (cmd == "group4") {
		grouphandlers[gu->myTeam]->GroupCommand(4);
	}
	else if (cmd == "group5") {
		grouphandlers[gu->myTeam]->GroupCommand(5);
	}
	else if (cmd == "group6") {
		grouphandlers[gu->myTeam]->GroupCommand(6);
	}
	else if (cmd == "group7") {
		grouphandlers[gu->myTeam]->GroupCommand(7);
	}
	else if (cmd == "group8") {
		grouphandlers[gu->myTeam]->GroupCommand(8);
	}
	else if (cmd == "group9") {
		grouphandlers[gu->myTeam]->GroupCommand(9);
	}
	else if (cmd == "lastmsgpos") {
		camCtrl->SetPos(infoConsole->lastMsgPos);
		camHandler->CameraTransition(0.6f);
	}
	else if (((cmd == "chat")     || (cmd == "chatall") ||
	         (cmd == "chatally") || (cmd == "chatspec")) &&
	         // if chat is bound to enter and we're waiting for user to press enter to start game, ignore.
				  (ks.Key() != SDLK_RETURN || !(gameServer && gameServer->WaitsOnCon() && allReady))) {

		if (cmd == "chatall")  { userInputPrefix = ""; }
		if (cmd == "chatally") { userInputPrefix = "a:"; }
		if (cmd == "chatspec") { userInputPrefix = "s:"; }
		userWriting = true;
		userPrompt = "Say: ";
		userInput = userInputPrefix;
		writingPos = (int)userInput.length();
		chatting = true;

		if (ks.Key() != SDLK_RETURN) {
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
		camHandler->ToggleOverviewCamera();
	}
	else if (cmd == "showhealthbars") {
		if (action.extra.empty()) {
			unitDrawer->showHealthBars = !unitDrawer->showHealthBars;
		} else {
			unitDrawer->showHealthBars = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "pause") {
		// disallow pausing prior to start of game proper
		if (playing) {
			bool newPause;
			if (action.extra.empty()) {
				newPause = !gs->paused;
			} else {
				newPause = !!atoi(action.extra.c_str());
			}
			net->SendPause(gu->myPlayerNum, newPause);
			lastframe = SDL_GetTicks(); // this required here?
		}
	}
	else if (cmd == "singlestep") {
		if (gameServer && gs->paused)
			gameServer->CreateNewFrame(false);
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
		if (filesystem.CreateDirectory("Saves")) {
			CLoadSaveHandler ls;
			ls.mapName = stupidGlobalMapname;
			ls.modName = modInfo.filename;
			ls.SaveGame("Saves/QuickSave.ssf");
		}
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
				logOutput.Print("Recording avi to %s size %li %li", name.c_str(),
				                bih.biWidth, bih.biHeight);
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
		float speed = gs->userSpeedFactor;
		if (speed < 1) {
			speed /= 0.8f;
			if (speed > 0.99f) {
				speed = 1;
			}
		} else {
			speed += 0.5f;
		}
		net->SendUserSpeed(gu->myPlayerNum, speed);
	}
	else if (cmd == "slowdown") {
		float speed = gs->userSpeedFactor;
		if (speed <= 1) {
			speed *= 0.8f;
			if (speed < 0.1f) {
				speed = 0.1f;
			}
		} else {
			speed -= 0.5f;
		}
		net->SendUserSpeed(gu->myPlayerNum, speed);
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
		shadowHandler->showShadowMap = !shadowHandler->showShadowMap;
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
		gd->SetMetalTexture(readmap->metalMap->metalMap,&readmap->metalMap->extractionMap.front(),readmap->metalMap->metalPal,false);
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
		if (!playing || gameOver || gs->Team(gu->myTeam)->isDead || gu->spectating) {
			userMayQuit = true;
		}
		else {
			// Check if there are more active players on his team.
			bool onlyActive = true;
			for (int a = 0;a < MAX_PLAYERS; ++a) {
				if (a != gu->myPlayerNum) {
					if (gs->players[a]->active) {
						onlyActive = false;
						if (gs->players[a]->team == gu->myTeam) {
							userMayQuit = true;
							break;
						}
					}
				}
			}
			// If you are the only remaining active player, you can quit immediately
			if (onlyActive) {
				userMayQuit = true;
			}
		}

		// User may not quit if he is the only player in his still active team.
		// Present him with the options given in CQuitBox.
		if (!userMayQuit) {
			if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == 0) {
				SAFE_NEW CQuitBox();
			}
		} else {
			logOutput.Print("User exited");
			globalQuit = true;
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
			int x = gu->dualScreenMode? gu->viewSizeX << 1: gu->viewSizeX;

			if (x % 4)
				x += (4 - x % 4);

			unsigned char* buf = SAFE_NEW unsigned char[x * gu->viewSizeY * 4];
			glReadPixels(0, 0, x, gu->viewSizeY, GL_RGBA, GL_UNSIGNED_BYTE, buf);

			CBitmap b(buf, x, gu->viewSizeY);
			b.ReverseYAxis();

			char t[50];
			for (int a = 0; a < 9999; ++a) {
				sprintf(t, "screenshots/screen%03i.%s", a, ext);
				CFileHandler ifs(t);
				if (!ifs.FileExists())
					break;
			}
			b.Save(t);
			logOutput.Print("Saved: %s", t);
			delete[] buf;
		}
	}

	else if (cmd == "grabinput") {
		SDL_GrabMode newMode;
		if (action.extra.empty()) {
			const SDL_GrabMode curMode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
			switch (curMode) {
				default: // make compiler happy
				case SDL_GRAB_OFF: newMode = SDL_GRAB_ON;  break;
				case SDL_GRAB_ON:  newMode = SDL_GRAB_OFF; break;
			}
		} else {
			if (atoi(action.extra.c_str())) {
				newMode = SDL_GRAB_ON;
			} else {
				newMode = SDL_GRAB_OFF;
			}
		}
		SDL_WM_GrabInput(newMode);
		logOutput.Print("Input grabbing %s",
		                (newMode == SDL_GRAB_ON) ? "enabled" : "disabled");
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
	else if (cmd == "cross") {
		if (action.extra.empty()) {
			if (crossSize > 0.0f) {
				crossSize = -crossSize;
			} else {
				crossSize = max(1.0f, -crossSize);
			}
		} else {
			crossSize = atof(action.extra.c_str());
		}
		configHandler.SetFloat("CrossSize", crossSize);
	}
	else if (cmd == "fps") {
		if (action.extra.empty()) {
			showFPS = !showFPS;
		} else {
			showFPS = !!atoi(action.extra.c_str());
		}
		configHandler.SetInt("ShowFPS", showFPS ? 1 : 0);
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
		unitDefHandler->SaveTechLevels("", modInfo.filename); // stdout
		unitDefHandler->SaveTechLevels("techlevels.txt", modInfo.filename);
		logOutput.Print("saved techlevels.txt");
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
			grouphandlers[gu->myTeam]->GetSuitedAis(selectedUnits.selectedUnits);
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
			LuaOpenGL::SetSafeMode(!!atoi(action.extra.c_str()));
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
			drawFpsHUD = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "mapmarks") {
		if (action.extra.empty()) {
			drawMapMarks = !drawMapMarks;
		} else {
			drawMapMarks = !!atoi(action.extra.c_str());
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
	else if (cmd == "grounddecals") {
		if (groundDecals) {
			if (action.extra.empty()) {
				groundDecals->SetDrawDecals(!groundDecals->GetDrawDecals());
			} else {
				groundDecals->SetDrawDecals(!!atoi(action.extra.c_str()));
			}
		}
		logOutput.Print("Ground decals are %s",
		                groundDecals->GetDrawDecals() ? "enabled" : "disabled");
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
	// Break up the if/else chain to workaround MSVC compiler limit
	// "fatal error C1061: compiler limit : blocks nested too deeply"
	else notfound1=true;
	if (notfound1)
	if (cmd == "pastetext") {
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
	else if (cmd == "lodscale") {
		if (!action.extra.empty()) {
			vector<string> args = CSimpleParser::Tokenize(action.extra, 0);
			if (args.size() == 1) {
				const float value = (float)atof(args[0].c_str());
				unitDrawer->LODScale = value;
			}
			else if (args.size() == 2) {
				const float value = (float)atof(args[1].c_str());
				if (args[0] == "shadow") {
					unitDrawer->LODScaleShadow = value;
				} else if (args[0] == "reflection") {
					unitDrawer->LODScaleReflection = value;
				} else if (args[0] == "refraction") {
					unitDrawer->LODScaleRefraction = value;
				}
			}
		}
	}
	else if (cmd == "wiremap") {
		if (action.extra.empty()) {
			gd->wireframe = !gd->wireframe;
		} else {
			gd->wireframe = !atoi(action.extra.c_str());
		}
	}
	else if (cmd == "setgamma") {
		float r, g, b;
		const int count = sscanf(action.extra.c_str(), "%f %f %f", &r, &g, &b);
		if (count == 1) {
			SDL_SetGamma(r, r, r);
			logOutput.Print("Set gamma value");
		}
		else if (count == 3) {
			SDL_SetGamma(r, g, b);
			logOutput.Print("Set gamma values");
		}
		else {
			logOutput.Print("Unknown gamma format");
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
		if (keys[SDLK_LSHIFT] || keys[SDLK_LCTRL])
			camHandler->ToggleState();
		else
			mouse->ToggleState();
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

	unsigned timeNow = SDL_GetTicks();

	const unsigned difTime = (timeNow - lastModGameTimeMeasure);
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

		starttime=fpstimer;
		oldframenum=gs->frameNum;

		if (!gameServer) {
			consumeSpeed = ((float)(GAME_SPEED * gs->speedFactor + leastQue - 2));
			leastQue = 10000;
			timeLeft = 0.0f;
		}

#ifdef TRACE_SYNC
		tracefile.DeleteInterval();
		tracefile.NewInterval();
#endif
		CInputReceiver::CollectGarbage();
	}

	UpdateUI();
	net->Update();

#ifdef DEBUG
	if(gameServer)
		gameServer->gameClientUpdated=true;
#endif

#ifdef SYNCIFY		//syncify doesnt support multithreading ...
	if (gameServer)
		gameServer->Update();
#endif

	if(creatingVideo && playing && gameServer){
		gameServer->CreateNewFrame();
	}

	if(!ClientReadNet()){
		logOutput.Print("Client read net wanted quit");
		return false;
	}

	if(gameSetup && !playing) {
		allReady=gameSetup->Update();
	} else if( gameServer && gameServer->WaitsOnCon()) {
		allReady=true;
		for(int a=0;a<gs->activePlayers;a++) {
			if(gs->players[a]->active && !gs->players[a]->readyToStart) {
				allReady=false;
				break;
			}
		}
	}

	if (gameServer && gameServer->WaitsOnCon() && allReady && (keys[SDLK_RETURN] || script->onlySinglePlayer || gameSetup))
	{
		chatting = false;
		userWriting = false;
		writingPos = 0;
		net->SendRandSeed(gs->randSeed);
		net->SendStartPlaying();
	}

	return true;
}

bool CGame::DrawWorld()
{
	SCOPED_TIMER("Draw world");

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

	if (drawSky) {
		sky->Draw();
	}
	if (drawGround) {
		gd->UpdateExtraTexture();
		gd->Draw();
	}
	if (drawWater) {
		if (!readmap->voidWater && water->drawSolid) {
			water->Draw();
		}
	}

	selectedUnits.Draw();
	luaCallIns.DrawWorldPreUnit();
	unitDrawer->Draw(false);
	featureHandler->Draw();

	if (gu->drawdebug && gs->cheatEnabled) {
		pathManager->Draw();
	}

	//transparent stuff
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	if (drawGround) {
		if (treeDrawer->drawTrees) {
			treeDrawer->DrawGrass();
		}
	}
	if (drawWater) {
		if (!readmap->voidWater && !water->drawSolid) {
			water->Draw();
		}
	}
	unitDrawer->DrawCloakedUnits();
	ph->Draw(false);
	sky->DrawSun();

	luaCallIns.DrawWorld();

	LuaUnsyncedCtrl::DrawUnitCommandQueues();
	if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
		selectedUnits.DrawCommands();
	}
	cursorIcons.Draw();
	cursorIcons.Clear();

	mouse->Draw();

	guihandler->DrawMapStuff(0);

	if (drawMapMarks) {
		inMapDrawer->Draw();
	}

	// underwater overlay
	if (camera->pos.y < 0.0f) {
		const float3& cpos = camera->pos;
		const float vr = gu->viewRange * 0.5f;
		glDepthMask(GL_FALSE);
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
		glDepthMask(GL_TRUE);
	}

	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	//reset fov
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);

	if (shadowHandler->drawShadows && shadowHandler->showShadowMap) {
		shadowHandler->DrawShadowTex();
	}

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLoadIdentity();

	// underwater overlay, part 2
	if (camera->pos.y < 0.0f) {
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);
		glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	}

	return true;
}

bool CGame::Draw()
{
	ASSERT_UNSYNCED_MODE;

	SetDrawMode(normalDraw);

	const unsigned currentTime = SDL_GetTicks();
	updateDeltaSeconds = 0.001f * float(currentTime - lastUpdateRaw);
	lastUpdateRaw = SDL_GetTicks();

 	if (luaUI)    { luaUI->CheckStack(); }
	if (luaCob)   { luaCob->CheckStack(); }
	if (luaGaia)  { luaGaia->CheckStack(); }
	if (luaRules) { luaRules->CheckStack(); }

	LuaUnsyncedCtrl::ClearUnitCommandQueues();

	if (luaUI) {
		luaUI->AddConsoleLines();
	}

	luaCallIns.Update();
	// XXX ugly hack to minimize luaUI errors
	if (luaUI && luaUI->GetCallInErrors() >= 5) {
		// FIXME: trepan -- I want error reports, make this verbose and annoying
		for (int annoy = 0; annoy < 32; annoy++) {
			logOutput << "5 errors deep in LuaUI, disabling...\n";
		}
		guihandler->RunLayoutCommand("disable");
		logOutput << "Type '/luaui reload' in the chat to reenable LuaUI.\n";
		logOutput << "===>>>  Please report this error to the forum or mantis with your infolog.txt\n";
	}

	if (!gu->active) {
		guihandler->Update();
		SDL_Delay(10); // milliseconds
		return true;
	}

	lineDrawer.UpdateLineStipple();

//	logOutput << mouse->lastx << "\n";
	if(!gs->paused && !HasLag() && gs->frameNum>1 && !creatingVideo){
		unsigned startDraw = SDL_GetTicks();
		gu->timeOffset = ((float)(startDraw - lastUpdate) * 0.001f)
		                 * (float)(GAME_SPEED * gs->speedFactor);
	} else  {
		gu->timeOffset=0;
		lastUpdate = SDL_GetTicks();
	}

	ph->UpdateTextures();

	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);

	sky->Update();

//	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	//set camera
	camHandler->UpdateCam();
	mouse->UpdateCursors();

	if(unitTracker.Enabled())
		unitTracker.SetCam();

	if(playing && (hideInterface || script->wantCameraControl))
		script->SetCamera();

	CBaseGroundDrawer* gd;
	{
		SCOPED_TIMER("Ground");
		gd = readmap->GetGroundDrawer();
		gd->Update(); // let it update before shadows have to be drawn
	}

	const bool doDrawWorld =
		hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();

	if (doDrawWorld) {
		SCOPED_TIMER("Shadows/Reflect");
		if (shadowHandler->drawShadows &&
		    (gd->drawMode != CBaseGroundDrawer::drawLos)) {
			// NOTE: shadows don't work in LOS mode, gain a few fps (until it's fixed)
			SetDrawMode(shadowDraw);
			shadowHandler->CreateShadows();
			SetDrawMode(normalDraw);
		}
		if (unitDrawer->advShading) {
			unitDrawer->UpdateReflectTex();
		}
	}

	camera->Update(false);

	{
		SCOPED_TIMER("Water");
		water->UpdateWater(this);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	if (doDrawWorld) {
		DrawWorld();
	}
	else {
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);

		//reset fov
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLoadIdentity();
	}

	glDisable(GL_FOG);

	luaCallIns.DrawScreenEffects();

	if (mouse->locked && (crossSize > 0.0f)) {
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		glLineWidth(1.49f);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glVertex2f(0.5f - (crossSize / gu->viewSizeX), 0.5f);
		glVertex2f(0.5f + (crossSize / gu->viewSizeX), 0.5f);
		glVertex2f(0.5f, 0.5f - (crossSize / gu->viewSizeY));
		glVertex2f(0.5f, 0.5f + (crossSize / gu->viewSizeY));
		glLineWidth(1.0f);
		glEnd();
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		DrawDirectControlHud();
	}
#endif

	glEnable(GL_TEXTURE_2D);

	{
		SCOPED_TIMER("Interface draw");
		if (hideInterface) {
			guihandler->Update();
		}
		else {
			std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
			if (!inputReceivers.empty()) {
				std::deque<CInputReceiver*>::reverse_iterator ri;
				for(ri = inputReceivers.rbegin(); ri != inputReceivers.rend(); ++ri) {
					CInputReceiver* rcvr = *ri;
					if (rcvr) {
						rcvr->Draw();
					}
				}
			}
		}

		if (!hideInterface) {
			luaCallIns.DrawScreen();
		}
	}

	glEnable(GL_TEXTURE_2D);

	if(gu->drawdebug){
		glPushMatrix();
		glColor4f(1,1,0.5f,0.8f);
		glTranslatef(0.03f,0.02f,0.0f);
		glScalef(0.03f,0.04f,0.1f);

		//skriv ut fps etc
		font->glPrint("FPS %d Frame %d Part %d(%d)",fps,gs->frameNum,ph->ps.size(),ph->currentParticles);
		glPopMatrix();

		if(playing){
			glPushMatrix();
			glTranslatef(0.03f,0.08f,0.0f);
			glScalef(0.02f,0.025f,0.1f);
			font->glPrint("xpos: %5.0f ypos: %5.0f zpos: %5.0f speed %2.2f",camera->pos.x,camera->pos.y,camera->pos.z,gs->speedFactor);
			glPopMatrix();
		}
	}

	if( gameServer && gameServer->WaitsOnCon()){
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
		glTranslatef(0.99f - tWidth, 0.94f, 1.0f);
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

	if (showFPS) {
		char buf[32];
		SNPRINTF(buf, sizeof(buf), "%i", fps);
		const float xScale = 0.015f;
		const float yScale = 0.020f;
		const float tWidth = font->CalcTextWidth(buf) * xScale;
		glTranslatef(0.99f - tWidth, 0.92f, 1.0f);
		glScalef(xScale, yScale, 1.0f);
		glColor4f(1.0f, 1.0f, 0.25f, 1.0f);
		if (!outlineFont.IsEnabled()) {
			font->glPrintRaw(buf);
		} else {
			const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
			const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
			const float yellow[4] = { 1.0f, 1.0f, 0.25f, 1.0f };
			outlineFont.print(xPixel, yPixel, yellow, buf);
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
			SNPRINTF(buf, sizeof(buf), "%c%i:%s %s %3.0f%% Ping:%d ms",
						(gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ',
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

	unsigned start = SDL_GetTicks();
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

	SetDrawMode(notDrawing);

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
	GameSetupDrawer::Disable();
	lastTick=clock();
	lastframe = SDL_GetTicks();
	ENTER_MIXED;
	gu->myTeam=gs->players[gu->myPlayerNum]->team;
	gu->myAllyTeam=gs->AllyTeam(gu->myTeam);
//	grouphandler->team=gu->myTeam;
	CLuaUI::UpdateTeams();
	ENTER_SYNCED;

}

void CGame::SimFrame()
{
	good_fpu_control_registers("CGame::SimFrame");
	lastFrameTime = SDL_GetTicks();
	ASSERT_SYNCED_MODE;

#ifdef TRACE_SYNC
	uh->CreateChecksum();
	tracefile << "New frame:" << gs->frameNum << " " << gs->randSeed << "\n";
#endif

#ifdef USE_MMGR
	if(!(gs->frameNum & 31))
		m_validateAllAllocUnits();
#endif

	script->Update();

	if (luaUI)    { luaUI->GameFrame(gs->frameNum); }
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
		for(int a=0;a<MAX_TEAMS;a++)
			grouphandlers[a]->Update();

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
	START_TIME_PROFILE("Sim time")

	helper->Update();
	mapDamage->Update();
	pathManager->Update();
	uh->Update();

	START_TIME_PROFILE("Collisions");
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

	END_TIME_PROFILE("Sim time")

	lastUpdate = SDL_GetTicks();

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
	//feclearexcept(FPU_Exceptions(FE_INVALID | FE_DIVBYZERO));
#endif
}


void CGame::AddTraffic(int playerID, int packetCode, int length)
{
	std::map<int, PlayerTrafficInfo>::iterator it = playerTraffic.find(playerID);
	if (it == playerTraffic.end()) {
		playerTraffic[playerID] = PlayerTrafficInfo();
		it = playerTraffic.find(playerID);
	}
	PlayerTrafficInfo& pti = it->second;
	pti.total += length;

	std::map<int, int>::iterator cit = pti.packets.find(packetCode);
	if (cit == pti.packets.end()) {
		pti.packets[packetCode] = length;
	} else {
		cit->second += length;
	}
}


bool CGame::ClientReadNet()
{
	// quick hack until better netcode is in place
#ifdef PROFILE_TIME
	if (gu->gameTime - lastCpuUsageTime >= 1) {
		lastCpuUsageTime = gu->gameTime;
		net->SendCPUUsage(profiler.profile["Sim time"].percent);
	}
#endif

	PUSH_CODE_MODE;
	ENTER_SYNCED;

	if (!net->IsActiveConnection())
		return gameOver;

	const RawPacket* packet = NULL;

	// compute new timeLeft to "smooth" out SimFrame() calls
	if(!gameServer){
		unsigned int currentFrame = SDL_GetTicks();

		if (timeLeft > 1.0f)
			timeLeft -= 1.0f;
		timeLeft += consumeSpeed * ((float)(currentFrame - lastframe) / 1000.f);
		if (skipping)
			timeLeft = 0.01f;
		lastframe = currentFrame;

		// read ahead to calculate the number of NETMSG_NEWFRAMES
		// we still have to process (in variable "que")
		int que = 0; // Number of NETMSG_NEWFRAMEs waiting to be processed.
		unsigned ahead = 0;
		while ((packet = net->Peek(ahead)) != NULL)
		{
			if (packet->data[0] == NETMSG_NEWFRAME)
				++que;
			++ahead;
		}

		if(que < leastQue)
			leastQue = que;
	}

	// make sure ClientReadNet returns at least every 15 game frames
	// so CGame can process keyboard input, and render etc.
	if (gameServer || net->localDemoPlayback)
		timeLeft = 15.0f;

	// really process the messages
	while (timeLeft > 0.0f && (packet = net->GetData()) != NULL)
	{
		const unsigned char* inbuf = packet->data;
		const unsigned dataLength = packet->length;
		const unsigned char packetCode = inbuf[0];

		switch (packetCode) {
			case NETMSG_QUIT: {
				logOutput.Print("Server exited");
				POP_CODE_MODE;
				AddTraffic(-1, packetCode, dataLength);
				return gameOver;
			}

			case NETMSG_PLAYERLEFT: {
				int player = inbuf[1];
				if (player >= MAX_PLAYERS || player < 0) {
					logOutput.Print("Got invalid player num (%i) in NETMSG_PLAYERLEFT", player);
				} else {
					switch (inbuf[2]) {
						case 1: {
							if (gs->players[player]->spectator) {
								logOutput.Print("Spectator %s left", gs->players[player]->playerName.c_str());
							} else {
								logOutput.Print("Player %s left", gs->players[player]->playerName.c_str());
							}
							break;
						}
						case 2:
							logOutput.Print("Player %s has been kicked", gs->players[player]->playerName.c_str());
							break;
						case 0:
							logOutput.Print("Lost connection to %s", gs->players[player]->playerName.c_str());
							break;
						default:
							logOutput.Print("Player %s left the game (reason unknown: %i)", gs->players[player]->playerName.c_str(), inbuf[2]);
					}
					gs->players[player]->active = false;
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_MEMDUMP: {
				MakeMemDump();
#ifdef TRACE_SYNC
				tracefile.Commit();
#endif
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_STARTPLAYING: {
				StartPlaying();
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_GAMEOVER: {
				ENTER_MIXED;
				gameOver=true;
				luaCallIns.GameOver();
				if (gu->autoQuit) {
					logOutput.Print("Automatical quit enforced from commandline");
					globalQuit = true;
				} else {
					SAFE_NEW CEndGameBox();
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL) {
					// Write CPlayer::Statistics and CTeam::Statistics to demo
						int numPlayers;
					// FIXME: ugh, there should be a better way to figure out number of players ...
						if (gameSetup != NULL) {
							numPlayers = gameSetup->numPlayers;
						} else {
							numPlayers = 0;
							while (gs->players[numPlayers]->currentStats->mousePixels != 0)
								++numPlayers;
						}
						int numTeams = gs->activeAllyTeams;
						if (gs->useLuaGaia)
							--numTeams;
					// Figure out who won the game.
						int winner = -1;
						for (int i = 0; i < numTeams; ++i) {
							if (!gs->Team(i)->isDead) {
								winner = gs->AllyTeam(i);
								break;
							}
						}
					// Finally pass it on to the CDemoRecorder.
						record->SetTime(gs->frameNum / 30, (int)gu->gameTime);
						record->InitializeStats(numPlayers, numTeams, winner);
						for (int i = 0; i < numPlayers; ++i)
							record->SetPlayerStats(i, *gs->players[i]->currentStats);
						for (int i = 0; i < numTeams; ++i)
							record->SetTeamStats(i, gs->Team(i)->statHistory);
					}
				}
				ENTER_SYNCED;
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_SENDPLAYERSTAT: {
				ENTER_MIXED;
				logOutput.Print("Game over");
			// Warning: using CPlayer::Statistics here may cause endianness problems
			// once net->SendData is endian aware!
				net->SendPlayerStat(gu->myPlayerNum, *gs->players[gu->myPlayerNum]->currentStats);
				ENTER_SYNCED;
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERSTAT: {
				int player=inbuf[1];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in playerstat msg",player);
					break;
				}
				*gs->players[player]->currentStats = *(CPlayer::Statistics*)&inbuf[2];
				if (gameOver) {
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL)
						record->SetPlayerStats(player, *gs->players[player]->currentStats);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PAUSE: {
				int player=inbuf[1];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in pause msg",player);
				}
				else if (!skipping) {
					gs->paused=!!inbuf[2];
					if(gs->paused){
						logOutput.Print("%s paused the game",gs->players[player]->playerName.c_str());
					} else {
						logOutput.Print("%s unpaused the game",gs->players[player]->playerName.c_str());
					}
					lastframe = SDL_GetTicks();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_INTERNAL_SPEED: {
				gs->speedFactor = *((float*) &inbuf[1]);
				//	logOutput.Print("Internal speed set to %.2f",gs->speedFactor);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_USER_SPEED: {
				gs->userSpeedFactor = *((float*) &inbuf[2]);

				unsigned char playerNum = *(unsigned char*) &inbuf[1];
				const char* playerName = gs->players[playerNum]->playerName.c_str();
				logOutput.Print("Speed set to %.1f [%s]", gs->userSpeedFactor, playerName);
				AddTraffic(playerNum, packetCode, dataLength);
				break;
			}

			case NETMSG_CPU_USAGE: {
				logOutput.Print("Game clients shouldn't get cpu usage msgs?");
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

		// header (1); uchar myPlayerNum (1); float cpuUsage (4); int ping (4): 10
			case NETMSG_PLAYERINFO: {
				int player = inbuf[1];
				if (player >= MAX_PLAYERS || player < 0) {
					logOutput.Print("Got invalid player num %i in playerinfo msg", player);
				} else {
					gs->players[player]->cpuUsage = *(float*) &inbuf[2];
					gs->players[player]->ping = *(int*) &inbuf[6];
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERNAME: {
				int player = inbuf[2];
				gs->players[player]->playerName=(char*)(&inbuf[3]);
				gs->players[player]->readyToStart=true;
				gs->players[player]->active=true;
				if (net->GetDemoRecorder())
					net->GetDemoRecorder()->SetMaxPlayerNum(inbuf[2]);
				wordCompletion->AddWord(gs->players[player]->playerName, false, false, false); // required?
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_CHAT: {
				int player=inbuf[2];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in chat msg",player);
				} else {
					string s=(char*)(&inbuf[3]);
					HandleChatMsg(s, player);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SYSTEMMSG:{
				string s=(char*)(&inbuf[3]);
				logOutput.Print(s);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_STARTPOS:{
				unsigned player = inbuf[1];
				int team = inbuf[2];
				if(team>=gs->activeTeams || team<0 || !gameSetup){
					logOutput.Print("Got invalid team num %i in startpos msg",team);
				} else {
					if(inbuf[3]!=2)
						gameSetup->readyTeams[team]=!!inbuf[3];
					gs->Team(team)->startPos.x=*(float*)&inbuf[4];
					gs->Team(team)->startPos.y=*(float*)&inbuf[8];
					gs->Team(team)->startPos.z=*(float*)&inbuf[12];
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_RANDSEED: {
				gs->randSeed = (*((unsigned int*)&inbuf[1]));
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_GAMEID: {
				const unsigned char* p = &inbuf[1];
				CDemoRecorder* record = net->GetDemoRecorder();
				if (record != NULL)
					record->SetGameID(p);
				logOutput.Print(
				  "GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
				  p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_NEWFRAME: {
				timeLeft -= 1.0f;
				SimFrame();
				// both NETMSG_SYNCRESPONSE and NETMSG_NEWFRAME are used for ping calculation by server
#ifdef SYNCCHECK
				net->SendSyncResponse(gu->myPlayerNum, gs->frameNum, CSyncChecker::GetChecksum());
				if ((gs->frameNum & 4095) == 0) // reset checksum every ~2.5 minute gametime
					CSyncChecker::NewFrame();
#else
				net->SendNewFrame(gs->frameNum);
#endif
				AddTraffic(-1, packetCode, dataLength);

				if(creatingVideo && net->localDemoPlayback){
					POP_CODE_MODE;
					return true;
				}
				break;
			}

			case NETMSG_COMMAND: {
				int player=inbuf[3];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in command msg",player);
				} else {
					Command c;
					c.id=*((int*)&inbuf[4]);
					c.options=inbuf[8];
					for(int a=0;a<((*((short int*)&inbuf[1])-9)/4);++a)
						c.params.push_back(*((float*)&inbuf[9+a*4]));
					selectedUnits.NetOrder(c,player);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SELECT: {
				int player=inbuf[3];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in netselect msg",player);
				} else {
					vector<int> selected;
					for(int a=0;a<((*((short int*)&inbuf[1])-4)/2);++a){
						int unitid=*((short int*)&inbuf[4+a*2]);
						if(unitid>=MAX_UNITS || unitid<0){
							logOutput.Print("Got invalid unitid %i in netselect msg",unitid);
							break;
						}
						if (uh->units[unitid] &&
													(uh->units[unitid]->team == gs->players[player]->team) || gs->godMode) {
							selected.push_back(unitid);
													}
					}
					selectedUnits.NetSelect(selected, player);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_AICOMMAND:{
				Command c;
				int player=inbuf[3];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in aicommand msg",player);
					break;
				}
				int unitid=*((short int*)&inbuf[4]);
				if(unitid>=MAX_UNITS || unitid<0){
					logOutput.Print("Got invalid unitid %i in aicommand msg",unitid);
					break;
				}
				//if(uh->units[unitid] && uh->units[unitid]->team!=gs->players[player]->team)		//msgs from global ais can be for other teams
				//	logOutput.Print("Warning player %i of team %i tried to send aiorder to unit from team %i",a,gs->players[player]->team,uh->units[unitid]->team);
				c.id=*((int*)&inbuf[6]);
				c.options=inbuf[10];
				for(int a=0;a<((*((short int*)&inbuf[1])-11)/4);++a)
					c.params.push_back(*((float*)&inbuf[11+a*4]));
				selectedUnits.AiOrder(unitid,c);
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_AICOMMANDS:{
				int u, c;
				const int player = inbuf[3];
				if (player>=MAX_PLAYERS || player<0) {
					logOutput.Print("Got invalid player num %i in aicommands msg",player);
					break;
				}

				const unsigned char* ptr = &inbuf[4];

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
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_LUAMSG: {
				const int player = inbuf[3];
				if ((player < 0) || (player >= MAX_PLAYERS)) {
					logOutput.Print("Got invalid player num %i in LuaMsg", player);
				}
				const int script = inbuf[4];
				const int mode = inbuf[5];
				const int msglen = *((short*)(inbuf + 1)) - 6;
				const string msg((char*)&inbuf[6], msglen); // allow embedded 0's
				CLuaHandle::HandleLuaMsg(player, script, mode, msg);
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SYNCREQUEST: {
				// TODO rename this net message, change error msg, etc.
				ENTER_MIXED;
				int frame=*((int*)&inbuf[1]);
				if(frame!=gs->frameNum){
					logOutput.Print("Sync request for wrong frame (%i instead of %i)", frame, gs->frameNum);
				}
#ifdef PROFILE_TIME
				net->SendCPUUsage(profiler.profile["Sim time"].percent);
#else
				net->SendCPUUsage(0.30f);
#endif
				ENTER_SYNCED;
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_SHARE: {
				int player=inbuf[1];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in share msg",player);
					break;
				}
				int team1=gs->players[player]->team;
				int team2=inbuf[2];
				bool shareUnits=!!inbuf[3];
				float metalShare=min(*(float*)&inbuf[4],(float)gs->Team(team1)->metal);
				float energyShare=min(*(float*)&inbuf[8],(float)gs->Team(team1)->energy);

				if (metalShare != 0.0f) {
					if (!luaRules ||
														luaRules->AllowResourceTransfer(team1, team2, "m", metalShare)) {
						gs->Team(team1)->metal -= metalShare;
						gs->Team(team2)->metal += metalShare;
														}
				}
				if (energyShare != 0.0f) {
					if (!luaRules ||
														luaRules->AllowResourceTransfer(team1, team2, "e", energyShare)) {
						gs->Team(team1)->energy -= energyShare;
						gs->Team(team2)->energy += energyShare;
														}
				}

				if(shareUnits){
					for(vector<int>::iterator ui=selectedUnits.netSelected[player].begin();ui!=selectedUnits.netSelected[player].end();++ui){
						if(uh->units[*ui] && uh->units[*ui]->team==team1 && !uh->units[*ui]->beingBuilt){
							uh->units[*ui]->ChangeTeam(team2,CUnit::ChangeGiven);
						}
					}
					selectedUnits.netSelected[player].clear();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SETSHARE: {
				int player=inbuf[1];
				int team=inbuf[2];
				if(team>=gs->activeTeams || team<0){
					logOutput.Print("Got invalid team num %i in setshare msg",team);
				} else {
					float metalShare=*(float*)&inbuf[3];
					float energyShare=*(float*)&inbuf[7];

					if (!luaRules || luaRules->AllowResourceLevel(team, "m", metalShare)) {
						gs->Team(team)->metalShare  = metalShare;
					}
					if (!luaRules || luaRules->AllowResourceLevel(team, "e", energyShare)) {
						gs->Team(team)->energyShare = energyShare;
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_MAPDRAW:{
				inMapDrawer->GotNetMsg(inbuf);
				AddTraffic(inbuf[2], packetCode, dataLength);
				break;
			}

#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL: {
				int player=inbuf[1];

				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in direct control msg",player);
					break;
				}

				CUnit* ctrlUnit = gs->players[player]->playerControlledUnit;
				if (ctrlUnit) {
					CUnit* unit=gs->players[player]->playerControlledUnit;
				//logOutput.Print("Player %s released control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());

					unit->directControl=0;
					unit->AttackUnit(0,true);
					gs->players[player]->StopControllingUnit();
				}
				else {
					if(!selectedUnits.netSelected[player].empty() && uh->units[selectedUnits.netSelected[player][0]] && !uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()){
						CUnit* unit = uh->units[selectedUnits.netSelected[player][0]];
						//logOutput.Print("Player %s took control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());
						if (unit->directControl) {
							if (player == gu->myPlayerNum) {
								logOutput.Print("Sorry someone already controls that unit");
							}
						}
						else if (!luaRules || luaRules->AllowDirectUnitControl(player, unit)) {
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
								camHandler->PushMode();
								camHandler->SetCameraMode(0);
								((CFPSController*)camCtrl)->SetPos(unit->midPos);
								selectedUnits.ClearSelected();
							}
							ENTER_SYNCED;
						}
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_DC_UPDATE: {
				int player=inbuf[1];
				if(player>=MAX_PLAYERS || player<0){
					logOutput.Print("Got invalid player num %i in dc update msg",player);
					break;
				}
				DirectControlStruct* dc=&gs->players[player]->myControl;
				CUnit* unit=gs->players[player]->playerControlledUnit;

				dc->forward    = !!(inbuf[2] & (1 << 0));
				dc->back       = !!(inbuf[2] & (1 << 1));
				dc->left       = !!(inbuf[2] & (1 << 2));
				dc->right      = !!(inbuf[2] & (1 << 3));
				dc->mouse1     = !!(inbuf[2] & (1 << 4));
				bool newMouse2 = !!(inbuf[2] & (1 << 5));
				if (!dc->mouse2 && newMouse2 && unit) {
					unit->AttackUnit(0, true);
					//	for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi)
					//	(*wi)->HoldFire();
				}
				dc->mouse2 = newMouse2;

				short int h = *((short int*)&inbuf[3]);
				short int p = *((short int*)&inbuf[5]);
				dc->viewDir = GetVectorFromHAndPExact(h, p);
				AddTraffic(player, packetCode, dataLength);
				break;
			}
#endif // DIRECT_CONTROL_ALLOWED

			//TODO CGame should not recieve this (handle in CPreGame)
			case NETMSG_MAPNAME: {
				const std::string mapname = std::string((char*) (inbuf + 6));
				if (mapname != stupidGlobalMapname)
				{
					logOutput.Print("Warning: mapname differs from host's");
				}
				archiveScanner->CheckMap(mapname, *(unsigned*)(&inbuf[2]));
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			//TODO same here
			case NETMSG_MODNAME: {
				std::string modArchive = archiveScanner->ModNameToModArchive(modInfo.filename);
				archiveScanner->CheckMod(modArchive, *(unsigned*)(&inbuf[2]));
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_SETPLAYERNUM:
			case NETMSG_ATTEMPTCONNECT:
			case NETMSG_SCRIPT: {
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
			default: {
#ifdef SYNCDEBUG
				lastLength = CSyncDebugger::GetInstance()->ClientReceived(&inbuf[inbufpos]);
				if (!lastLength)
#endif
				{
					logOutput.Print("Unknown net msg in client %d", (int) inbuf[0]);
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
		}
		delete packet;
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

		((CFPSController*)camHandler->camControllers[0])->SetPos(pos);
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

		if (disableTracker && camCtrl->DisableTrackingByKey()) {
			unitTracker.Disable();
		}
		movement.z = cameraSpeed;
		camCtrl->KeyMove(movement);

		movement=float3(0,0,0);

		if (( fullscreen && fullscreenEdgeMove) ||
		    (!fullscreen && windowedEdgeMove)) {
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
		camCtrl->ScreenEdgeMove(movement);

		if(camMove[4])
			camCtrl->MouseWheelMove(gu->lastFrameTime*200*cameraSpeed);
		if(camMove[5])
			camCtrl->MouseWheelMove(-gu->lastFrameTime*200*cameraSpeed);
	}

	camCtrl->Update();

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
		unit->currentLOD = 0;
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
			const WeaponDef* wd = w->weaponDef;
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
		float3 p;
		CInputReceiver* ir = NULL;
		if (!hideInterface) {
			ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);
		}
		if (ir == minimap) {
			p = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
		}
		else {
			const float3& pos = camera->pos;
			const float3& dir = mouse->dir;
			const float dist = ground->LineGroundCol(pos, pos + (dir * 9000.0f));
			p = pos + (dir * dist);
		}

		char buf[128];
		SNPRINTF(buf, sizeof(buf), " @%.0f,%.0f,%.0f", p.x, p.y, p.z);
		msg += buf;
	}
	if (msg.size() > 128) {
		msg.resize(128); // safety
	}
	net->SendChat(gu->myPlayerNum, msg);
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


void CGame::HandleChatMsg(std::string s, int player)
{
	// Player chat messages
	if (!noSpectatorChat || !gs->players[player]->spectator || !gu->spectating) {
		LogNetMsg(s, player);
	}

	globalAI->GotChatMsg(s.c_str(),player);
	CScriptHandler::Instance().chosenScript->GotChatMsg(s, player);

	if ((s.find(".cheat") == 0) && (player == 0 || net->localDemoPlayback)) {
		SetBoolArg(gs->cheatEnabled, s, ".cheat");
		if (gs->cheatEnabled) {
			logOutput.Print("Cheating!");
		} else {
			logOutput.Print("No more cheating");
		}
	}
	else if ((s.find(".godmode") == 0) && (player == 0)) {
		if (!gs->cheatEnabled) {
			logOutput.Print("godmode requires .cheat");
		} else {
			SetBoolArg(gs->godMode, s, ".godmode");
			if (gs->godMode) {
				logOutput.Print("God Mode Enabled");
			} else {
				logOutput.Print("God Mode Disabled");
			}
			CLuaUI::UpdateTeams();
		}
	}
	else if ((s.find(".nocost") == 0) && (player == 0) && gs->cheatEnabled) {
		for(unsigned i=0; i<unitDefHandler->numUnitDefs; i++)
		{
			unitDefHandler->unitDefs[i].metalCost = 1;
			unitDefHandler->unitDefs[i].energyCost = 1;
			unitDefHandler->unitDefs[i].buildTime = 10;
			unitDefHandler->unitDefs[i].metalUpkeep = 0;
			unitDefHandler->unitDefs[i].energyUpkeep = 0;
		}
		unitDefHandler->noCost=true;
		logOutput.Print("Everything is for free!");
	}
#ifdef DEBUG
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
	else if ((s.find(".desync") == 0) && gs->cheatEnabled) {
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
#endif
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
#ifdef SYNCDEBUG
	else if ((s.find(".fakedesync") == 0) && gs->cheatEnabled && gameServer) {
		gameServer->fakeDesync = true;
		logOutput.Print("Fake desyncing.");
	}
	else if ((s.find(".reset") == 0) && gs->cheatEnabled) {
		CSyncDebugger::GetInstance()->Reset();
		logOutput.Print("Resetting sync debugger.");
	}
#endif
	else if ((s == ".spectator") && (gs->cheatEnabled || net->localDemoPlayback)) {
		gs->players[player]->spectator=true;
		if (player == gu->myPlayerNum) {
			gu->spectating           = true;
			gu->spectatingFullView   = true;
			gu->spectatingFullSelect = true;
			selectedUnits.ClearSelected();
			unitTracker.Disable();
			CLuaUI::UpdateTeams();
		}
	}
	else if ((s.find(".team") == 0) && (gs->cheatEnabled || net->localDemoPlayback)) {
		int team=atoi(&s.c_str()[s.find(" ")]);
		if ((team >= 0) && (team <gs->activeTeams)) {
			gs->players[player]->team = team;
			gs->players[player]->spectator = net->localDemoPlayback;
			if (player == gu->myPlayerNum) {
				gu->spectating           = net->localDemoPlayback;
				gu->spectatingFullView   = net->localDemoPlayback;
				gu->spectatingFullSelect = net->localDemoPlayback;
				gu->myTeam = team;
				gu->myAllyTeam = gs->AllyTeam(gu->myTeam);
				selectedUnits.ClearSelected();
				unitTracker.Disable();
				CLuaUI::UpdateTeams();
			}
		}
	}
	else if ((s.find(".atm") == 0) && gs->cheatEnabled) {
		int team = gs->players[player]->team;
		gs->Team(team)->AddMetal(1000);
		gs->Team(team)->AddEnergy(1000);
	}

	else if ((s.find(".give") == 0) && gs->cheatEnabled) {
		// .give [amount] <unitName> [team] <@x,y,z>
		vector<string> args = CSimpleParser::Tokenize(s, 0);


		if (args.size() < 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		float3 pos;
		if (sscanf(args[args.size() - 1].c_str(),
		           "@%f,%f,%f", &pos.x, &pos.y, &pos.z) != 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		int amount = 1;
		int team = gs->players[player]->team;

		int amountArg = -1;
		int teamArg = -1;

		if (args.size() == 5) {
			amountArg = 1;
			teamArg = 3;
		}
		else if (args.size() == 4) {
			if (args[1].find_first_not_of("0123456789") == string::npos) {
				amountArg = 1;
			} else {
				teamArg = 2;
			}
		}

		if (amountArg >= 0) {
			const string& amountStr = args[amountArg];
			amount = atoi(amountStr.c_str());
			if ((amount < 0) ||
			    (amountStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad .give amount: %s", amountStr.c_str());
				return;
			}
		}

		if (teamArg >= 0) {
			const string& teamStr = args[teamArg];
			team = atoi(teamStr.c_str());
			if ((team < 0) || (team >= gs->activeTeams) ||
			    (teamStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad .give team: %s", teamStr.c_str());
				return;
			}
		}

		const string unitName = (amountArg >= 0) ? args[2] : args[1];

		if (unitName == "all") {
			// player entered ".give all"
			int sqSize = (int) ceil(sqrt((float) unitDefHandler->numUnitDefs));
			int currentNumUnits = gs->Team(team)->units.size();
			int numRequestedUnits = unitDefHandler->numUnitDefs;

			// make sure team unit-limit not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->maxUnits) {
				numRequestedUnits = uh->maxUnits - currentNumUnits;
			}

			for (int a = 1; a <= numRequestedUnits; ++a) {
				float posx = pos.x + (a % sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float posz = pos.z + (a / sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float3 pos2 = float3(posx, pos.y, posz);
				const string& defName = unitDefHandler->unitDefs[a].name;
				const CUnit* unit =
					unitLoader.LoadUnit(defName, pos2, team, false, 0, NULL);

				if (unit) {
					unitLoader.FlattenGround(unit);
				}
			}
		}
		else if (!unitName.empty()) {
			int numRequestedUnits = amount;
			int currentNumUnits = gs->Team(team)->units.size();

			if (currentNumUnits >= uh->maxUnits) {
				logOutput.Print("Unable to give any more units to team %i", team);
				return;
			}

			// make sure team unit-limit not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->maxUnits) {
				numRequestedUnits = uh->maxUnits - currentNumUnits;
			}

			const UnitDef* unitDef = unitDefHandler->GetUnitByName(unitName);

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
						const CUnit* unit = unitLoader.LoadUnit(unitName, upos, team, false, 0, NULL);

						if (unit) {
							unitLoader.FlattenGround(unit);
						}
						--total;
					}
				}

				logOutput.Print("Giving %i %s to team %i",
				                numRequestedUnits, unitName.c_str(), team);
			}
			else {
				if (teamArg < 0) {
					team = -1; // default to world features
				}

				const FeatureDef* featureDef = featureHandler->GetFeatureDef(unitName);
				if (featureDef) {
					int xsize = featureDef->xsize;
					int zsize = featureDef->ysize;
					int squareSize = (int) ceil(sqrt((float) numRequestedUnits));
					int total = amount; // FIXME -- feature count limit?

					float3 minpos = pos;
					minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
					minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

					for (int z = 0; z < squareSize; ++z) {
						for (int x = 0; x < squareSize && total > 0; ++x) {
							float minposx = minpos.x + x * xsize * SQUARE_SIZE;
							float minposz = minpos.z + z * zsize * SQUARE_SIZE;
							float minposy = ground->GetHeight2(minposx, minposz);
							const float3 upos(minposx, minposy, minposz);
							CFeature* feature = SAFE_NEW CFeature();
							feature->Initialize(upos, featureDef, 0, 0, team, "");
							--total;
						}
					}

					logOutput.Print("Giving %i %s (feature) to team %i",
					                numRequestedUnits, unitName.c_str(), team);
				}
				else {
					logOutput.Print(unitName + " is not a valid unitname");
				}
			}
		}
	}

	else if ((s.find(".take") == 0) && (!gs->players[player]->spectator || gs->cheatEnabled)) {
		int sendTeam = gs->players[player]->team;
		for (int a = 0; a < gs->activeTeams; ++a) {
			if (gs->AlliedTeams(a, sendTeam)) {
				bool hasPlayer = false;
				for (int b = 0; b < gs->activePlayers; ++b) {
					if (gs->players[b]->active && gs->players[b]->team==a && !gs->players[b]->spectator) {
						hasPlayer = true;
					}
				}
				if (!hasPlayer) {
					for (std::list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui) {
						CUnit* unit = *ui;
						if ((unit->team == a) && (unit->selfDCountdown == 0)) {
							unit->ChangeTeam(sendTeam, CUnit::ChangeGiven);
						}
					}
				}
			}
		}
	}
	// .kick, .kickbynum and .nopause moved to CGameServer
	else if ((s.find(".nospectatorchat") == 0) && (player == 0)) {
		SetBoolArg(noSpectatorChat, s, ".nospectatorchat");
		logOutput.Print("Spectators %s chat", noSpectatorChat ? "can" : "can not");
	}
	else if ((s.find(".nohelp") == 0) && (player == 0)) {
		SetBoolArg(gs->noHelperAIs, s, ".nohelp");
		selectedUnits.PossibleCommandChange(NULL);
		if (gs->noHelperAIs) {
			// remove any current GroupAIs
			CUnitSet& teamUnits = gs->Team(gu->myTeam)->units;
			CUnitSet::iterator it;
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
	else if (s.find(".skip") == 0) {
		if (((player != 0) || !gs->cheatEnabled) && !net->localDemoPlayback) {
			logOutput.Print(".skip only works in replay, and when cheating\n");
		} else if (gs->frameNum < 1) {
			logOutput.Print(".skip only works after every demo-player has readied\n");
		} else if (s.size() <= 6) {
			logOutput.Print("missing argument to .skip\n");
		} else {
			Skip(s);
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
	else if ((s.find(".editdefs") == 0) && (player == 0) && gs->cheatEnabled) {
		SetBoolArg(gs->editDefsEnabled, s, ".editdefs");
		if (gs->editDefsEnabled) {
			logOutput.Print("Definition Editing!");
		} else {
			logOutput.Print("No definition Editing");
		}
	}
	else if ((s.find(".luarules") == 0) && (gs->frameNum > 1)) {
		if (gs->useLuaRules) {
			if (s == ".luarules reload") {
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
			else if (s == ".luarules disable") {
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
				luaRules->GotChatMsg(s.substr(strlen(".luarules")), player);
			}
			else {
				logOutput.Print("LuaRules is not enabled\n");
			}
		}
	}
	else if ((s.find(".luagaia") == 0) && (gs->frameNum > 1)) {
		if (gs->useLuaGaia) {
			if (s == ".luagaia reload") {
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
			else if (s == ".luagaia disable") {
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
				luaGaia->GotChatMsg(s.substr(strlen(".luagaia")), player);
			}
			else {
				logOutput.Print("LuaGaia is not enabled\n");
			}
		}
	}
	else if ((s.find(".luacob") == 0) && (gs->frameNum > 1)) {
		if (s ==  ".luacob reload") {
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
		else if (s == ".luacob disable") {
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
			luaCob->GotChatMsg(s.substr(strlen(".luacob")), player);
		}
		else {
			logOutput.Print("LuaCob is not enabled\n");
		}
	}
	else if (s.find(".save ") == 0) {//.save [-y ]<savename>
		if (filesystem.CreateDirectory("Saves")) {
			bool saveoverride = s.find(" -y ") == 0;
			std::string savename(s.c_str()+(6+(saveoverride?3:0)));
			savename="Saves/"+savename+".ssf";
			if (filesystem.GetFilesize(savename)==0 || saveoverride) {
				logOutput.Print("Saving game to %s\n",savename.c_str());
				CLoadSaveHandler ls;
				ls.mapName = stupidGlobalMapname;
				ls.modName = modInfo.filename;
				ls.SaveGame(savename);
			} else {
				logOutput.Print("File %s allready exists(use .save -y to override)\n",savename.c_str());
			}
		}
	}
	else if ((s[0] == '.') && (gs->frameNum > 1)) {
		if (luaRules && luaRules->SyncedActionFallback(s, player)) { return; }
		if (luaGaia  && luaGaia->SyncedActionFallback(s, player))  { return; }
		if (luaCob   && luaCob->SyncedActionFallback(s, player))   { return; }
	}
}


void CGame::LogNetMsg(const string& msg, int playerID)
{
	string s = msg;

	CPlayer* player = gs->players[playerID];
	const bool myMsg = (playerID == gu->myPlayerNum);

	bool allyMsg = false;
	bool specMsg = false;

	if ((s.length() >= 2) && (s[1] == ':')) {
		const char lower = tolower(s[0]);
		if (lower == 'a') {
			allyMsg = true;
			s = s.substr(2);
		}
		else if (lower == 's') {
			specMsg = true;
			s = s.substr(2);
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


void CGame::Skip(const std::string& msg)
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
			// FIXME: messes up the how-many-frames-are-left bar
			Update();

			if (gameServer) {
				gameServer->SkipTo(gs->frameNum);
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

		gu->gameTime    += seconds;
		gu->modGameTime += seconds;

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
	const string unitName = (msg.size() >= 12)? msg.substr(11): "";
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
				unit->cob->Call("Create");
			}
		}
	}
	logOutput.Print("Reloaded cob script for %i units", count);
}


void CGame::SelectUnits(const string& line)
{
	vector<string> args = CSimpleParser::Tokenize(line, 0);
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
				const CUnitSet& teamUnits = gs->Team(gu->myTeam)->units;
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

	const CUnitSet& selUnits = selectedUnits.selectedUnits;

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
		CUnitSet::const_iterator it;
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


static unsigned char GetLuaColor(const LuaTable& tbl, int channel, unsigned char orig)
{
	const float fOrig = (float)orig / 255.0f;
	float luaVal = tbl.GetFloat(channel, fOrig);
	luaVal = max(0.0f, min(1.0f, luaVal));
	return (unsigned char)(luaVal * 255.0f);
}


void CGame::ReColorTeams()
{
	for (int t = 0; t < MAX_TEAMS; t++) {
		CTeam* team = gs->Team(t);
		team->origColor[0] = team->color[0];
		team->origColor[1] = team->color[1];
		team->origColor[2] = team->color[2];
		team->origColor[3] = team->color[3];
	}

	{
		// scoped so that 'fh' disappears before luaParser uses the file
		CFileHandler fh("teamcolors.lua", SPRING_VFS_RAW);
		if (!fh.FileExists()) {
			return;
		}
	}

	LuaParser luaParser("teamcolors.lua", SPRING_VFS_RAW, SPRING_VFS_RAW_FIRST);

	luaParser.AddParam("myPlayer", gu->myPlayerNum);

	luaParser.AddParam("modName",      modInfo.humanName);
	luaParser.AddParam("modShortName", modInfo.shortName);
	luaParser.AddParam("modVersion",   modInfo.version);

	luaParser.AddParam("mapName",      readmap->mapName);
	luaParser.AddParam("mapHumanName", readmap->mapHumanName);

	luaParser.AddParam("gameMode",     gs->gameMode);

	luaParser.GetTable("teams");
	for(int t = 0; t < gs->activeTeams; ++t) {
		luaParser.GetTable(t); {
			const CTeam* team = gs->Team(t);
			const unsigned char* color = gs->Team(t)->color;
			luaParser.AddParam("allyTeam", gs->AllyTeam(t));
			luaParser.AddParam("gaia",     team->gaia);
			luaParser.AddParam("leader",   team->leader);
			luaParser.AddParam("side",     team->side);
			luaParser.GetTable("color"); {
				luaParser.AddParam(1, float(color[0]) / 255.0f);
				luaParser.AddParam(2, float(color[1]) / 255.0f);
				luaParser.AddParam(3, float(color[2]) / 255.0f);
				luaParser.AddParam(4, float(color[3]) / 255.0f);
			}
			luaParser.EndTable(); // color
		}
		luaParser.EndTable(); // team#
	}
	luaParser.EndTable(); // teams

	luaParser.GetTable("players");
	for(int p = 0; p < gs->activePlayers; ++p) {
		luaParser.GetTable(p); {
			const CPlayer* player = gs->players[p];
			luaParser.AddParam("name",       player->playerName);
			luaParser.AddParam("team",       player->team);
			luaParser.AddParam("active",     player->active);
			luaParser.AddParam("spectating", player->spectator);
		}
		luaParser.EndTable(); // player#
	}
	luaParser.EndTable(); // players

	if (!luaParser.Execute()) {
		logOutput.Print("teamcolors.lua: luaParser.Execute() failed\n");
		return;
	}
	LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		logOutput.Print("teamcolors.lua: root table is not valid\n");
	}

	for (int t = 0; t < gs->activeTeams; ++t) {
		LuaTable teamTable = root.SubTable(t);
		if (teamTable.IsValid()) {
			unsigned char* color = gs->Team(t)->color;
			color[0] = GetLuaColor(teamTable, 1, color[0]);
			color[1] = GetLuaColor(teamTable, 2, color[1]);
			color[2] = GetLuaColor(teamTable, 3, color[2]);
			// do not adjust color[3] -- alpha
		}
	}
}

bool CGame::HasLag() const
{
	unsigned timeNow = SDL_GetTicks();
	if (!gs->paused && timeNow > lastFrameTime + 500.0f / gs->speedFactor)
	{
		return true;
	}
	else
	{
		return false;
	}
}
