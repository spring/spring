/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>
#include <locale>
#include <sstream>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <SDL_keyboard.h>

#include "mmgr.h"

#include "Game.h"
#include "Camera.h"
#include "CameraHandler.h"
#include "ChatMessage.h"
#include "ClientSetup.h"
#include "ConsoleHistory.h"
#include "GameHelper.h"
#include "GameServer.h"
#include "GameVersion.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "LoadScreen.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "PlayerRoster.h"
#include "TimeProfiler.h"
#include "WaitCommandsAI.h"
#include "WordCompletion.h"
#include "OSCStatsSender.h"
#include "IVideoCapturing.h"
#include "Game/UI/UnitTracker.h"
#ifdef _WIN32
#  include "winerror.h"
#endif
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/glFont.h"
#include "Rendering/Screenshot.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/UnitDrawer.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Models/ModelDrawer.hpp"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaHandle.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/COB/CobEngine.h"
#include "Sim/Units/COB/UnitScriptEngine.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
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
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/TooltipConsole.h"
#include "UI/ProfileDrawer.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FPUCheck.h"
#include "System/NetProtocol.h"
#include "System/SpringApp.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Net/PackPacket.h"
#include "System/Platform/CrashHandler.h"
#include "System/Sound/ISound.h"
#include "System/Sound/IEffectChannel.h"
#include "System/Sound/IMusicChannel.h"
#include "System/Sync/SyncedPrimitiveIO.h"
#include "System/Sync/SyncTracer.h"

#include <boost/cstdint.hpp>

#undef CreateDirectory

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

CGame* game = NULL;


CR_BIND(CGame, (std::string(""), std::string(""), NULL));

CR_REG_METADATA(CGame,(
//	CR_MEMBER(finishedLoading),
//	CR_MEMBER(drawMode),
//	CR_MEMBER(fps),
//	CR_MEMBER(thisFps),
	CR_MEMBER(lastSimFrame),
//	CR_MEMBER(fpstimer),
//	CR_MEMBER(starttime),
//	CR_MEMBER(lastUpdate),
//	CR_MEMBER(lastMoveUpdate),
//	CR_MEMBER(lastModGameTimeMeasure),
//	CR_MEMBER(lastUpdateRaw),
//	CR_MEMBER(updateDeltaSeconds),
	CR_MEMBER(totalGameTime),
	CR_MEMBER(userInputPrefix),
	CR_MEMBER(lastTick),
	CR_MEMBER(chatSound),
//	CR_MEMBER(camMove),
//	CR_MEMBER(camRot),
	CR_MEMBER(hideInterface),
	CR_MEMBER(gameOver),
//	CR_MEMBER(windowedEdgeMove),
//	CR_MEMBER(fullscreenEdgeMove),
	CR_MEMBER(showFPS),
	CR_MEMBER(showClock),
	CR_MEMBER(showSpeed),
	CR_MEMBER(showMTInfo),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(gameID),
//	CR_MEMBER(script),
//	CR_MEMBER(infoConsole),
//	CR_MEMBER(consoleHistory),
//	CR_MEMBER(wordCompletion),
//	CR_MEMBER(hotBinding),
//	CR_MEMBER(inputTextPosX),
//	CR_MEMBER(inputTextPosY),
//	CR_MEMBER(inputTextSizeX),
//	CR_MEMBER(inputTextSizeY),
//	CR_MEMBER(skipping),
	CR_MEMBER(playing),
//	CR_MEMBER(lastFrameTime),
//	CR_MEMBER(leastQue),
//	CR_MEMBER(timeLeft),
//	CR_MEMBER(consumeSpeed),
//	CR_MEMBER(lastframe),
//	CR_MEMBER(leastQue),

	CR_POSTLOAD(PostLoad)
));



CGame::CGame(const std::string& mapname, const std::string& modName, ILoadSaveHandler* saveFile) :
	finishedLoading(false),
	gameDrawMode(gameNotDrawing),
	defsParser(NULL),
	fps(0),
	thisFps(0),
	lastSimFrame(-1),

	totalGameTime(0),

	hideInterface(false),

	gameOver(false),

	noSpectatorChat(false),

	infoConsole(NULL),
	consoleHistory(NULL),
	wordCompletion(NULL),

	skipping(false),
	playing(false),
	chatting(false),
	lastFrameTime(0),

	leastQue(0),
	timeLeft(0.0f),
	consumeSpeed(1.0f),
	luaDrawTime(0),

	saveFile(saveFile)
{
	game = this;

	memset(gameID, 0, sizeof(gameID));

	time(&starttime);
	lastTick = clock();

	for (int a = 0; a < 8; ++a) { camMove[a] = false; }
	for (int a = 0; a < 4; ++a) { camRot[a] = false; }


	//FIXME move to MouseHandler!
	windowedEdgeMove   = !!configHandler->Get("WindowedEdgeMove",   1);
	fullscreenEdgeMove = !!configHandler->Get("FullscreenEdgeMove", 1);

	showFPS   = !!configHandler->Get("ShowFPS",   0);
	showClock = !!configHandler->Get("ShowClock", 1);
	showSpeed = !!configHandler->Get("ShowSpeed", 0);
	showMTInfo = !!configHandler->Get("ShowMTInfo", 1);

	speedControl = configHandler->Get("SpeedControl", 0);

	playerRoster.SetSortTypeByCode((PlayerRoster::SortType)configHandler->Get("ShowPlayerInfo", 1));

	CInputReceiver::guiAlpha = configHandler->Get("GuiOpacity",  0.8f);

	const string inputTextGeo = configHandler->GetString("InputTextGeo", "");
	ParseInputTextGeometry("default");
	ParseInputTextGeometry(inputTextGeo);

	userInput  = "";
	writingPos = 0;
	userPrompt = "";

	CLuaHandle::SetModUICtrl(!!configHandler->Get("LuaModUICtrl", 1));

	modInfo.Init(modName.c_str());
	if (!mapInfo) {
		mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);
	}

	if (!sideParser.Load()) {
		throw content_error(sideParser.GetErrorLog());
	}
}

CGame::~CGame()
{
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "]";
#endif

	CLoadScreen::DeleteInstance();
	IVideoCapturing::FreeInstance();
	ISound::Shutdown();

	CLuaGaia::FreeHandler();
	CLuaRules::FreeHandler();
	LuaOpenGL::Free();
	heightMapTexture.Kill();
	CColorMap::DeleteColormaps();
	CEngineOutHandler::Destroy();
	CResourceHandler::FreeInstance();

	SafeDelete(guihandler);
	SafeDelete(minimap);
	SafeDelete(resourceBar);
	SafeDelete(tooltip); // CTooltipConsole*
	SafeDelete(infoConsole);
	SafeDelete(consoleHistory);
	SafeDelete(wordCompletion);
	SafeDelete(keyBindings);
	SafeDelete(keyCodes);
	SafeDelete(selectionKeys); // CSelectionKeyHandler*
	SafeDelete(luaInputReceiver);
	SafeDelete(mouse); // CMouseHandler*

	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(treeDrawer);
	SafeDelete(pathDrawer);
	SafeDelete(modelDrawer);
	SafeDelete(shadowHandler);
	SafeDelete(camHandler);
	SafeDelete(camera);
	SafeDelete(cam2);
	SafeDelete(iconHandler);
	SafeDelete(inMapDrawer);
	SafeDelete(geometricObjects);
	SafeDelete(farTextureHandler);
	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	SafeDelete(featureDrawer);
	SafeDelete(featureHandler); // depends on unitHandler (via ~CFeature)
	SafeDelete(unitDrawer); // depends on unitHandler, cubeMapHandler, groundDecals
	SafeDelete(uh); // CUnitHandler*, depends on modelParser (via ~CUnit)
	SafeDelete(projectileDrawer);
	SafeDelete(ph); // CProjectileHandler*

	SafeDelete(cubeMapHandler);
	SafeDelete(groundDecals);
	SafeDelete(modelParser);

	SafeDelete(unitLoader);
	SafeDelete(pathManager);
	SafeDelete(ground);
	SafeDelete(smoothGround);
	SafeDelete(groundBlockingObjectMap);
	SafeDelete(radarhandler);
	SafeDelete(loshandler);
	SafeDelete(mapDamage);
	SafeDelete(qf);
	SafeDelete(moveinfo);
	SafeDelete(unitDefHandler);
	SafeDelete(weaponDefHandler);
	SafeDelete(damageArrayHandler);
	SafeDelete(explGenHandler);
	SafeDelete(helper);
	SafeDelete((mapInfo = const_cast<CMapInfo*>(mapInfo)));

	CGroundMoveType::DeleteLineTable();
	CCategoryHandler::RemoveInstance();

	for (unsigned int i = 0; i < grouphandlers.size(); i++) {
		SafeDelete(grouphandlers[i]);
	}
	grouphandlers.clear();

	SafeDelete(saveFile); // ILoadSaveHandler, depends on vfsHandler via ~CArchiveBase
	SafeDelete(vfsHandler);
	SafeDelete(archiveScanner);

	SafeDelete(gameServer);
	SafeDelete(net);

	game = NULL;
}


void CGame::LoadGame(const std::string& mapname)
{
	if (!gu->globalQuit) LoadDefs();
	if (!gu->globalQuit) LoadSimulation(mapname);
	if (!gu->globalQuit) LoadRendering();
	if (!gu->globalQuit) LoadInterface();
	if (!gu->globalQuit) LoadLua();
	if (!gu->globalQuit) LoadFinalize();

	if (!gu->globalQuit && saveFile) {
		loadscreen->SetLoadMessage("Loading game");
		saveFile->LoadGame();
	}
}


void CGame::LoadDefs()
{
	{
		loadscreen->SetLoadMessage("Loading Radar Icons");
		iconHandler = new CIconHandler();
	}

	{
		ScopedOnceTimer timer("Loading GameData Definitions");
		loadscreen->SetLoadMessage("Loading GameData Definitions");

		defsParser = new LuaParser("gamedata/defs.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
		// customize the defs environment
		defsParser->GetTable("Spring");
		defsParser->AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
		defsParser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
		defsParser->EndTable();

		// run the parser
		if (!defsParser->Execute()) {
			throw content_error("Defs-Parser: " + defsParser->GetErrorLog());
		}
		const LuaTable root = defsParser->GetRoot();
		if (!root.IsValid()) {
			throw content_error("Error loading gamedata definitions");
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
	}

	{
		ScopedOnceTimer timer("Loading Sound Definitions");
		loadscreen->SetLoadMessage("Loading Sound Definitions");

		sound->LoadSoundDefs("gamedata/sounds.lua");
		chatSound = sound->GetSoundId("IncomingChat", false);
	}
}

void CGame::LoadSimulation(const std::string& mapname)
{
	// simulation components
	helper = new CGameHelper();
	ground = new CGround();

	loadscreen->SetLoadMessage("Parsing Map Information");

	readmap = CReadMap::LoadMap(mapname);
	groundBlockingObjectMap = new CGroundBlockingObjectMap(gs->mapSquares);

	loadscreen->SetLoadMessage("Creating Smooth Height Mesh");
	smoothGround = new SmoothHeightMesh(ground, float3::maxxpos, float3::maxzpos, SQUARE_SIZE * 2, SQUARE_SIZE * 40);

	loadscreen->SetLoadMessage("Creating QuadField & CEGs");
	moveinfo = new CMoveInfo();
	qf = new CQuadField();
	damageArrayHandler = new CDamageArrayHandler();
	explGenHandler = new CExplosionGeneratorHandler();

	{
		//! FIXME: these five need to be loaded before featureHandler
		//! (maps with features have their models loaded at startup)
		modelParser = new C3DModelLoader();

		loadscreen->SetLoadMessage("Creating Unit Textures");
		texturehandler3DO = new C3DOTextureHandler;
		texturehandlerS3O = new CS3OTextureHandler;

		farTextureHandler = new CFarTextureHandler();
		featureDrawer = new CFeatureDrawer();
	}

	loadscreen->SetLoadMessage("Loading Weapon Definitions");
	weaponDefHandler = new CWeaponDefHandler();
	loadscreen->SetLoadMessage("Loading Unit Definitions");
	unitDefHandler = new CUnitDefHandler();
	unitLoader = new CUnitLoader();

	CGroundMoveType::CreateLineTable();

	uh = new CUnitHandler();
	ph = new CProjectileHandler();

	loadscreen->SetLoadMessage("Loading Feature Definitions");
	featureHandler = new CFeatureHandler();
	loadscreen->SetLoadMessage("Initializing Map Features");
	featureHandler->LoadFeaturesFromMap(saveFile != NULL);

	mapDamage = IMapDamage::GetMapDamage();
	loshandler = new CLosHandler();
	radarhandler = new CRadarHandler(false);

	pathManager = IPathManager::GetInstance();

	wind.LoadWind(mapInfo->atmosphere.minWind, mapInfo->atmosphere.maxWind);

	CCobInstance::InitVars(teamHandler->ActiveTeams(), teamHandler->ActiveAllyTeams());
	CEngineOutHandler::Initialize();
}

void CGame::LoadRendering()
{
	// rendering components
	cubeMapHandler = new CubeMapHandler();
	shadowHandler = new CShadowHandler();
	groundDecals = new CGroundDecalHandler();

	loadscreen->SetLoadMessage("Creating GroundDrawer");
	readmap->NewGroundDrawer();

	loadscreen->SetLoadMessage("Creating TreeDrawer");
	treeDrawer = CBaseTreeDrawer::GetTreeDrawer();

	inMapDrawer = new CInMapDraw();
	pathDrawer = IPathDrawer::GetInstance();

	geometricObjects = new CGeometricObjects();

	projectileDrawer = new CProjectileDrawer();
	projectileDrawer->LoadWeaponTextures();
	unitDrawer = new CUnitDrawer();
	modelDrawer = IModelDrawer::GetInstance();

	loadscreen->SetLoadMessage("Creating Sky & Water");
	sky = CBaseSky::GetSky();
	water = CBaseWater::GetWater(NULL, -1);

	glLightfv(GL_LIGHT1, GL_AMBIENT, mapInfo->light.unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, mapInfo->light.unitSunColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, mapInfo->light.unitAmbientColor);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	glFogf(GL_FOG_START, globalRendering->viewRange * mapInfo->atmosphere.fogStart);
	glFogf(GL_FOG_END, globalRendering->viewRange);
	glFogf(GL_FOG_DENSITY, 1.0f);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glEnable(GL_FOG);
	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0.0f);
}

void CGame::LoadInterface()
{
	{
		ScopedOnceTimer timer("Camera and mouse");
		camera = new CCamera();
		cam2 = new CCamera();
		mouse = new CMouseHandler();
		camHandler = new CCameraHandler();
	}

	selectedUnits.Init(playerHandler->ActivePlayers());

	// interface components
	ReColorTeams();
	cmdColors.LoadConfig("cmdcolors.txt");

	{
		ScopedOnceTimer timer("Loading console");
		consoleHistory = new CConsoleHistory;
		wordCompletion = new CWordCompletion;
		for (int pp = 0; pp < playerHandler->ActivePlayers(); pp++) {
			wordCompletion->AddWord(playerHandler->Player(pp)->name, false, false, false);
		}
		// add the Skirmish AIs instance names to word completion,
		// for various things, eg chatting
		const CSkirmishAIHandler::id_ai_t& ais = skirmishAIHandler.GetAllSkirmishAIs();
		CSkirmishAIHandler::id_ai_t::const_iterator ai;
		for (ai = ais.begin(); ai != ais.end(); ++ai) {
			wordCompletion->AddWord(ai->second.name + " ", false, false, false);
		}
		// add the available Skirmish AI libraries to word completion, for /aicontrol
		const IAILibraryManager::T_skirmishAIKeys& aiLibs = aiLibManager->GetSkirmishAIKeys();
		IAILibraryManager::T_skirmishAIKeys::const_iterator aiLib;
		for (aiLib = aiLibs.begin(); aiLib != aiLibs.end(); ++aiLib) {
			wordCompletion->AddWord(aiLib->GetShortName() + " " + aiLib->GetVersion() + " ", false, false, false);
		}
		// add the available Lua AI implementations to word completion, for /aicontrol
		const std::set<std::string>& luaAIShortNames = skirmishAIHandler.GetLuaAIImplShortNames();
		for (std::set<std::string>::const_iterator sn = luaAIShortNames.begin();
				sn != luaAIShortNames.end(); ++sn) {
			wordCompletion->AddWord(*sn + " ", false, false, false);
		}

		const std::map<std::string, int>& unitMap = unitDefHandler->unitDefIDsByName;
		std::map<std::string, int>::const_iterator uit;
		for (uit = unitMap.begin(); uit != unitMap.end(); ++uit) {
			wordCompletion->AddWord(uit->first + " ", false, true, false);
		}
	}

	infoConsole = new CInfoConsole();
	tooltip = new CTooltipConsole();
	guihandler = new CGuiHandler();
	minimap = new CMiniMap();
	resourceBar = new CResourceBar();
	keyCodes = new CKeyCodes();
	keyBindings = new CKeyBindings();
	keyBindings->Load("uikeys.txt");
	selectionKeys = new CSelectionKeyHandler();

	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		grouphandlers.push_back(new CGroupHandler(t));
	}

	GameSetupDrawer::Enable();
}

void CGame::LoadLua()
{
	// Lua components
	loadscreen->SetLoadMessage("Loading LuaRules");
	CLuaRules::LoadHandler();

	if (gs->useLuaGaia) {
		loadscreen->SetLoadMessage("Loading LuaGaia");
		CLuaGaia::LoadHandler();
	}

	loadscreen->SetLoadMessage("Loading LuaUI");
	CLuaUI::LoadHandler();

	// last in, first served
	luaInputReceiver = new LuaInputReceiver();

	delete defsParser;
	defsParser = NULL;
}

void CGame::LoadFinalize()
{
	loadscreen->SetLoadMessage("Finalizing");
	eventHandler.GamePreload();

	lastframe = SDL_GetTicks();
	lastModGameTimeMeasure = lastframe;
	lastUpdate = lastframe;
	lastMoveUpdate = lastframe;
	lastUpdateRaw = lastframe;
	lastCpuUsageTime = gu->gameTime;
	updateDeltaSeconds = 0.0f;

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

	finishedLoading = true;
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

	// reload water renderer (it may depend on screen resolution)
	water = CBaseWater::GetWater(water, water->GetID());

	eventHandler.ViewResize();
}


int CGame::KeyPressed(unsigned short k, bool isRepeat)
{
	if (!gameOver && !isRepeat) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.keyPresses++;
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
		unsigned int actionIndex;
		for (actionIndex = 0; actionIndex < actionList.size(); actionIndex++) {
			const Action& action = actionList[actionIndex];

			if (action.command == "edit_return") {
				userWriting=false;
				writingPos = 0;

				if (k == SDLK_RETURN) {
					keyInput->SetKeyState(k, 0); //prevent game start when server chats
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
						Action fakeAction(actionLine);
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
				const vector<string> &partials = wordCompletion->Complete(head);
				userInput = head + tail;
				writingPos = (int)head.length();
				if (!partials.empty()) {
					string msg;
					for (unsigned int i = 0; i < partials.size(); i++) {
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
					writingPos = std::max(0, writingPos - 2);
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
					PasteClipboard();
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
				writingPos = std::max(0, std::min((int)userInput.length(), writingPos - 1));
				break;
			}
			else if (action.command == "edit_next_char") {
				writingPos = std::max(0, std::min((int)userInput.length(), writingPos + 1));
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
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv=*ri;
		if (recv && recv->KeyPressed(k, isRepeat)) {
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


int CGame::KeyReleased(unsigned short k)
{
	if ((userWriting) && (((k>=' ') && (k<='Z')) || (k==8) || (k==190))) {
		return 0;
	}

	// try the input receivers
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv=*ri;
		if (recv && recv->KeyReleased(k)) {
			return 0;
		}
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



bool CGame::Update()
{
	good_fpu_control_registers("CGame::Update");

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

	time(&fpstimer);

	if (difftime(fpstimer, starttime) != 0) { // do once every second
		fps = thisFps;
		thisFps = 0;

		starttime = fpstimer;

		if (!gameServer) {
			consumeSpeed = ((float)(GAME_SPEED * gs->speedFactor + leastQue - 2));
			leastQue = 10000;
			timeLeft = 0.0f;
		}

#ifdef TRACE_SYNC
		tracefile.DeleteInterval();
		tracefile.NewInterval();
#endif
	}

	if (!skipping)
	{
		UpdateUI(false);
	}

	net->Update();

	if (videoCapturing->IsCapturing() && playing && gameServer) {
		gameServer->CreateNewFrame(false, true);
	}

	if(gs->frameNum == 0 || gs->paused)
		eventHandler.UpdateObjects(); // we must add new rendering objects even if the game has not started yet

	ClientReadNet();

	if(net->NeedsReconnect() && !gameOver) {
		extern ClientSetup* startsetup;
		net->AttemptReconnect(startsetup->myPlayerName, startsetup->myPasswd, SpringVersion::GetFull());
	}

	if (net->CheckTimeout(0, gs->frameNum == 0) && !gameOver) {
		logOutput.Print("Lost connection to gameserver");
		GameEnd(std::vector<unsigned char>());
	}

	// send out new console lines
	if (infoConsole) {
		vector<CInfoConsole::RawLine> lines;
		infoConsole->GetNewRawLines(lines);
		for (unsigned int i = 0; i < lines.size(); i++) {
			const CInfoConsole::RawLine& rawLine = lines[i];
			eventHandler.AddConsoleLine(rawLine.text, *rawLine.subsystem);
		}
	}

	if (!(gs->frameNum & 31)) {
		oscStatsSender->Update(gs->frameNum);
	}

	return true;
}


bool CGame::DrawWorld()
{
	SCOPED_TIMER("Draw world");

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

	if (globalRendering->drawSky) {
		sky->Draw();
	}

	if (globalRendering->drawGround) {
		gd->Draw();
		if (smoothGround->drawEnabled)
			smoothGround->DrawWireframe(1);
		treeDrawer->DrawGrass();
		gd->DrawTrees();
	}

	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("Water");
		GML_STDMUTEX_LOCK(water);

		water->OcclusionQuery();
		if (water->drawSolid) {
			water->UpdateWater(this);
			water->Draw();
		}
	}

	selectedUnits.Draw();
	eventHandler.DrawWorldPreUnit();

	unitDrawer->Draw(false);
	modelDrawer->Draw();
	featureDrawer->Draw();
	pathDrawer->Draw();

	//! transparent stuff
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	bool noAdvShading = shadowHandler->drawShadows;

	static const double plane_below[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	static const double plane_above[4] = {0.0f,  1.0f, 0.0f, 0.0f};

	glClipPlane(GL_CLIP_PLANE3, plane_below);
	glEnable(GL_CLIP_PLANE3);

		//! draw cloaked part below surface
		unitDrawer->DrawCloakedUnits(noAdvShading);
		featureDrawer->DrawFadeFeatures(noAdvShading);

	glDisable(GL_CLIP_PLANE3);

	//! draw water
	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("Water");
		GML_STDMUTEX_LOCK(water);

		if (!water->drawSolid) {
			water->UpdateWater(this);
			water->Draw();
		}
	}

	glClipPlane(GL_CLIP_PLANE3, plane_above);
	glEnable(GL_CLIP_PLANE3);

		//! draw cloaked part above surface
		unitDrawer->DrawCloakedUnits(noAdvShading);
		featureDrawer->DrawFadeFeatures(noAdvShading);

	glDisable(GL_CLIP_PLANE3);

	projectileDrawer->Draw(false);

	if (globalRendering->drawSky) {
		sky->DrawSun();
	}

	eventHandler.DrawWorld();

	LuaUnsyncedCtrl::DrawUnitCommandQueues();
	if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
		selectedUnits.DrawCommands();
	}

	lineDrawer.DrawAll();
	cursorIcons.Draw();
	cursorIcons.Clear();

	mouse->DrawSelectionBox();

	guihandler->DrawMapStuff(0);

	if (globalRendering->drawMapMarks && !hideInterface) {
		inMapDrawer->Draw();
	}


	//! underwater overlay
	if (camera->pos.y < 0.0f) {
		glEnableClientState(GL_VERTEX_ARRAY);
		const float3& cpos = camera->pos;
		const float vr = globalRendering->viewRange * 0.5f;
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.5f, 0.3f, 0.50f);
		{
			float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr)
			};
			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUADS, 0, 4);
		}

		{
			float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x - vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr),
				float3(cpos.x + vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
			};
			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUAD_STRIP, 0, 10);
		}

		glDepthMask(GL_TRUE);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	//reset fov
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// underwater overlay, part 2
	if (camera->pos.y < 0.0f) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);
		float3 verts[] = {
			float3 (0.f, 0.f, -1.f),
			float3 (1.f, 0.f, -1.f),
			float3 (1.f, 1.f, -1.f),
			float3 (0.f, 1.f, -1.f),
		};
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	return true;
}



#if defined(USE_GML) && GML_ENABLE_DRAW
bool CGame::Draw() {
	gmlProcessor->Work(&CGame::DrawMTcb,NULL,NULL,this,gmlThreadCount,TRUE,NULL,1,2,2,FALSE);
	return true;
}
#else
bool CGame::DrawMT() {
	return true;
}
#endif


#if defined(USE_GML) && GML_ENABLE_DRAW
bool CGame::DrawMT() {
#else
bool CGame::Draw() {
#endif
	GML_STDMUTEX_LOCK(draw); //Draw

	//! timings and frame interpolation
	const unsigned currentTime = SDL_GetTicks();

	if(skipping) {
		if(skipLastDraw + 500 > currentTime) // render at 2 FPS
			return true;
		skipLastDraw = currentTime;
#if defined(USE_GML) && GML_ENABLE_SIM
		extern volatile int gmlMultiThreadSim;
		if(!gmlMultiThreadSim)
#endif
		{
			DrawSkip();
			return true;
		}
	}

	thisFps++;

	updateDeltaSeconds = 0.001f * float(currentTime - lastUpdateRaw);
	lastUpdateRaw = SDL_GetTicks();
	if (!gs->paused && !HasLag() && gs->frameNum>1 && !videoCapturing->IsCapturing()) {
		globalRendering->lastFrameStart = SDL_GetTicks();
		globalRendering->weightedSpeedFactor = 0.001f * GAME_SPEED * gs->speedFactor;
		globalRendering->timeOffset = (float)(globalRendering->lastFrameStart - lastUpdate) * globalRendering->weightedSpeedFactor;
	} else  {
		globalRendering->timeOffset=0;
		lastUpdate = SDL_GetTicks();
	}


	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();

	//! set camera
	camHandler->UpdateCam();
	camera->Update(false);

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	if (doDrawWorld) {
		{
			SCOPED_TIMER("Ground Update");
			gd->Update();
		}

		if (lastSimFrame != gs->frameNum && !skipping) {
			projectileDrawer->UpdateTextures();
			sky->Update();

			GML_STDMUTEX_LOCK(water);
			water->Update();
		}
	}

	if (lastSimFrame != gs->frameNum) {
		CInputReceiver::CollectGarbage();
		if (!skipping) {
			// TODO call only when camera changed
			sound->UpdateListener(camera->pos, camera->forward, camera->up, globalRendering->lastFrameTime);
		}
	}

	//! update extra texture even if paused (you can still give orders)
	if (!skipping && (lastSimFrame != gs->frameNum || gs->paused)) {
		static unsigned next_upd = lastUpdate + 1000/30;

		if (!gs->paused || next_upd <= lastUpdate) {
			next_upd = lastUpdate + 1000/30;

			SCOPED_TIMER("ExtraTexture");
			gd->UpdateExtraTexture();
		}
	}

	lastSimFrame = gs->frameNum;

	if(!skipping)
		UpdateUI(true);

	SetDrawMode(gameNormalDraw);

 	if (luaUI)    { luaUI->CheckStack(); }
	if (luaGaia)  { luaGaia->CheckStack(); }
	if (luaRules) { luaRules->CheckStack(); }

	// XXX ugly hack to minimize luaUI errors
	if (luaUI && luaUI->GetCallInErrors() >= 5) {
		for (int annoy = 0; annoy < 8; annoy++) {
			LogObject() << "5 errors deep in LuaUI, disabling...\n";
		}

		guihandler->PushLayoutCommand("disable");
		LogObject() << "Type '/luaui reload' in the chat to re-enable LuaUI.\n";
		LogObject() << "===>>>  Please report this error to the forum or mantis with your infolog.txt\n";
	}

	configHandler->Update();
	CNamedTextures::Update();
	texturehandlerS3O->Update();
	modelParser->Update();
	treeDrawer->Update();
	readmap->UpdateDraw();
	unitDrawer->Update();
	featureDrawer->Update();
	mouse->UpdateCursors();
	mouse->EmptyMsgQueUpdate();
	guihandler->Update();
	lineDrawer.UpdateLineStipple();

	LuaUnsyncedCtrl::ClearUnitCommandQueues();
	eventHandler.Update();
	eventHandler.DrawGenesis();

	if (!globalRendering->active) {
		SDL_Delay(10); // milliseconds
		return true;
	}

	CTeamHighlight::Enable(currentTime);

	if (unitTracker.Enabled()) {
		unitTracker.SetCam();
	}

	if (doDrawWorld) {
		{
			SCOPED_TIMER("Shadows/Reflections");
			if (shadowHandler->drawShadows &&
				(gd->drawMode != CBaseGroundDrawer::drawLos)) {
				// NOTE: shadows don't work in LOS mode, gain a few fps (until it's fixed)
				SetDrawMode(gameShadowDraw);
				shadowHandler->CreateShadows();
				SetDrawMode(gameNormalDraw);
			}

			cubeMapHandler->UpdateReflectionTexture();

			if (FBO::IsSupported())
				FBO::Unbind();

			glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
		}
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);	// Clear Screen And Depth&Stencil Buffer
	camera->Update(false);

	if (doDrawWorld) {
		DrawWorld();
	}
	else {
		//reset fov
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_FOG);

	if (doDrawWorld) {
		eventHandler.DrawScreenEffects();
	}

	hudDrawer->Draw(gu->directControl);
	debugDrawerAI->Draw();

	glEnable(GL_TEXTURE_2D);

	{
		SCOPED_TIMER("Draw interface");
		if (hideInterface) {
			//luaInputReceiver->Draw();
		}
		else {
			std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
			std::deque<CInputReceiver*>::reverse_iterator ri;
			for (ri = inputReceivers.rbegin(); ri != inputReceivers.rend(); ++ri) {
				CInputReceiver* rcvr = *ri;
				if (rcvr) {
					rcvr->Draw();
				}
			}
		}
	}

	glEnable(GL_TEXTURE_2D);

	if (globalRendering->drawdebug) {
		//print some infos (fps,gameframe,particles)
		glColor4f(1,1,0.5f,0.8f);
		font->glFormat(0.03f, 0.02f, 1.0f, FONT_SCALE | FONT_NORM, "FPS: %d Frame: %d Particles: %d (%d)",
		    fps, gs->frameNum, ph->syncedProjectiles.size() + ph->unsyncedProjectiles.size(), ph->currentParticles);

		if (playing) {
			font->glFormat(0.03f, 0.07f, 0.7f, FONT_SCALE | FONT_NORM, "xpos: %5.0f ypos: %5.0f zpos: %5.0f speed %2.2f",
			    camera->pos.x, camera->pos.y, camera->pos.z, gs->speedFactor);
		}
	}

	if (userWriting) {
		DrawInputText();
	}

	if (!hotBinding.empty()) {
		glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
		font->glPrint(0.5f, 0.6f, 3.0f, FONT_SCALE | FONT_CENTER | FONT_NORM, "Hit keyset for:");
		glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
		font->glFormat(0.5f, 0.5f, 3.0f, FONT_SCALE | FONT_CENTER | FONT_NORM, "%s", hotBinding.c_str());
		glColor4f(0.3f, 0.3f, 1.0f, 1.0f);
		font->glPrint(0.5f, 0.4f, 3.0f, FONT_SCALE | FONT_CENTER | FONT_NORM, "(or Escape)");
	}

	if (!hideInterface) {
		smallFont->Begin();

		int font_options = FONT_RIGHT | FONT_SCALE | FONT_NORM;
		if (guihandler->GetOutlineFonts())
			font_options |= FONT_OUTLINE;

		if (showClock) {
			char buf[32];
			const int seconds = (gs->frameNum / GAME_SPEED);
			if (seconds < 3600) {
				SNPRINTF(buf, sizeof(buf), "%02i:%02i", seconds / 60, seconds % 60);
			} else {
				SNPRINTF(buf, sizeof(buf), "%02i:%02i:%02i", seconds / 3600, (seconds / 60) % 60, seconds % 60);
			}

			smallFont->glPrint(0.99f, 0.94f, 1.0f, font_options, buf);
		}

		if (showFPS) {
			char buf[32];
			SNPRINTF(buf, sizeof(buf), "%i", fps);

			const float4 yellow(1.0f, 1.0f, 0.25f, 1.0f);
			smallFont->SetColors(&yellow,NULL);
			smallFont->glPrint(0.99f, 0.92f, 1.0f, font_options, buf);
		}

		if (showSpeed) {
			char buf[32];
			SNPRINTF(buf, sizeof(buf), "%2.2f", gs->speedFactor);

			const float4 speedcol(1.0f, gs->speedFactor < gs->userSpeedFactor * 0.99f ? 0.25f : 1.0f, 0.25f, 1.0f);
			smallFont->SetColors(&speedcol, NULL);
			smallFont->glPrint(0.99f, 0.90f, 1.0f, font_options, buf);
		}

#if defined(USE_GML) && GML_ENABLE_SIM
		int cit = (int)GML_DRAW_CALLIN_TIME() * (int)fps;
		if(cit > luaDrawTime)
			++luaDrawTime;
		else if(cit < luaDrawTime)
			--luaDrawTime;

		if(showMTInfo) {
			float drawPercent = (float)luaDrawTime / 10.0f;
			char buf[32];
			SNPRINTF(buf, sizeof(buf), "LUA-DRAW(MT): %2.0f%%", drawPercent);
			float4 warncol(drawPercent >= 10.0f && (currentTime & 128) ?
				0.5f : std::max(0.0f, std::min(drawPercent / 5.0f, 1.0f)), std::max(0.0f, std::min(2.0f - drawPercent / 5.0f, 1.0f)), 0.0f, 1.0f);
			smallFont->SetColors(&warncol, NULL);
			smallFont->glPrint(0.99f, 0.88f, 1.0f, font_options, buf);
		}
#endif

		if (playerRoster.GetSortType() != PlayerRoster::Disabled) {
			static std::string chart; chart = "";
			static std::string prefix;
			char buf[128];

			int count;
			const std::vector<int>& indices = playerRoster.GetIndices(&count, true);

			SNPRINTF(buf, sizeof(buf), "\xff%c%c%c \tNu\tm   \tUser name   \tCPU  \tPing", 255, 255, 63);
			chart += buf;

			for (int a = 0; a < count; ++a) {
				const CPlayer* p = playerHandler->Player(indices[a]);
				unsigned char color[3] = {255, 255, 255};
				unsigned char allycolor[3] = {255, 255, 255};
				if(p->ping != PATHING_FLAG || gs->frameNum != 0) {
					if (p->spectator)
						prefix = "S";
					else {
						const unsigned char* bColor = teamHandler->Team(p->team)->color;
						color[0] = std::max((unsigned char)1, bColor[0]);
						color[1] = std::max((unsigned char)1, bColor[1]);
						color[2] = std::max((unsigned char)1, bColor[2]);
						if (gu->myAllyTeam == teamHandler->AllyTeam(p->team)) {
							allycolor[0] = allycolor[2] = 1;
							prefix = "A";	// same AllyTeam
						}
						else if (teamHandler->AlliedTeams(gu->myTeam, p->team)) {
							allycolor[0] = allycolor[1] = 1;
							prefix = "E+";	// different AllyTeams, but are allied
						}
						else {
							allycolor[1] = allycolor[2] = 1;
							prefix = "E";	//no alliance at all
						}
					}
					float4 cpucolor(!p->spectator && p->cpuUsage > 0.75f && gs->speedFactor < gs->userSpeedFactor * 0.99f &&
						(currentTime & 128) ? 0.5f : std::max(0.01f, std::min(1.0f, p->cpuUsage * 2.0f / 0.75f)),
							std::min(1.0f, std::max(0.01f, (1.0f - p->cpuUsage / 0.75f) * 2.0f)), 0.01f, 1.0f);
					int ping = (int)(((p->ping) * 1000) / (GAME_SPEED * gs->speedFactor));
					float4 pingcolor(!p->spectator && gc->reconnectTimeout > 0 && ping > 1000 * gc->reconnectTimeout &&
							(currentTime & 128) ? 0.5f : std::max(0.01f, std::min(1.0f, (ping - 250) / 375.0f)),
							std::min(1.0f, std::max(0.01f, (1000 - ping) / 375.0f)), 0.01f, 1.0f);
					SNPRINTF(buf, sizeof(buf), "\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t\xff%c%c%c%.0f%%  \t\xff%c%c%c%dms",
							allycolor[0], allycolor[1], allycolor[2], (gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ',
							p->team, prefix.c_str(), color[0], color[1], color[2], p->name.c_str(),
							(unsigned char)(cpucolor[0] * 255.0f), (unsigned char)(cpucolor[1] * 255.0f), (unsigned char)(cpucolor[2] * 255.0f),
							p->cpuUsage * 100.0f,
							(unsigned char)(pingcolor[0] * 255.0f), (unsigned char)(pingcolor[1] * 255.0f), (unsigned char)(pingcolor[2] * 255.0f),
							ping);
				}
				else {
					prefix = "";
					SNPRINTF(buf, sizeof(buf), "\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t%s-%d  \t%d",
							allycolor[0], allycolor[1], allycolor[2], (gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ',
							p->team, prefix.c_str(), color[0], color[1], color[2], p->name.c_str(), (((int)p->cpuUsage) & 0x1)?"PC":"BO",
							((int)p->cpuUsage) & 0xFE, (((int)p->cpuUsage)>>8)*1000);
				}
				chart += "\n";
				chart += buf;
			}

			font_options |= FONT_BOTTOM;
			smallFont->SetColors();
			smallFont->glPrintTable(1.0f - 5 * globalRendering->pixelX, 0.00f + 5 * globalRendering->pixelY, 1.0f, font_options, chart);
		}

		smallFont->End();
	}

#if defined(USE_GML) && GML_ENABLE_SIM
	if(skipping)
		DrawSkip(false);
#endif

	mouse->DrawCursor();

	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	unsigned start = SDL_GetTicks();
	globalRendering->lastFrameTime = (float)(start - lastMoveUpdate)/1000.f;
	lastMoveUpdate = start;

	videoCapturing->RenderFrame();

	SetDrawMode(gameNotDrawing);

	CTeamHighlight::Disable();

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
		configHandler->SetString("InputTextGeo", geo);
	}
}


void CGame::DrawInputText()
{
	const float fontSale = 1.0f;                       // TODO: make configurable again
	const float fontSize = fontSale * font->GetSize();

	const string tempstring = userPrompt + userInput;

	// draw the caret
	const int caretPos = userPrompt.length() + writingPos;
	const string caretStr = tempstring.substr(0, caretPos);
	const float caretWidth = fontSize * font->GetTextWidth(caretStr) * globalRendering->pixelX;

	char c = userInput[writingPos];
	if (c == 0) { c = ' '; }

	const float cw = fontSize * font->GetCharacterWidth(c) * globalRendering->pixelX;
	const float csx = inputTextPosX + caretWidth;
	glDisable(GL_TEXTURE_2D);
	const float f = 0.5f * (1.0f + fastmath::sin((float)SDL_GetTicks() * 0.015f));
	glColor4f(f, f, f, 0.75f);
	glRectf(csx, inputTextPosY, csx + cw, inputTextPosY + fontSize * font->GetLineHeight() * globalRendering->pixelY);
	glEnable(GL_TEXTURE_2D);

	// setup the color
	static float4 const defColor(1.0f, 1.0f, 1.0f, 1.0f);
	static float4 const allyColor(0.5f, 1.0f, 0.5f, 1.0f);
	static float4 const specColor(1.0f, 1.0f, 0.5f, 1.0f);
	const float4* textColor = &defColor;
	if (userInput.length() < 2) {
		textColor = &defColor;
	} else if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
		textColor = &allyColor;
	} else if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
		textColor = &specColor;
	} else {
		textColor = &defColor;
	}

	// draw the text
	font->Begin();
	font->SetColors(textColor, NULL);
	if (!guihandler->GetOutlineFonts()) {
		font->glPrint(inputTextPosX, inputTextPosY, fontSize, FONT_DESCENDER | FONT_NORM, tempstring);
	} else {
		font->glPrint(inputTextPosX, inputTextPosY, fontSize, FONT_DESCENDER | FONT_OUTLINE | FONT_NORM, tempstring);
	}
	font->End();
}


void CGame::StartPlaying()
{
	assert(!playing);
	playing = true;
	GameSetupDrawer::Disable();
	lastTick = clock();
	lastframe = SDL_GetTicks();

	gu->startTime = gu->gameTime;
	gu->myTeam = playerHandler->Player(gu->myPlayerNum)->team;
	gu->myAllyTeam = teamHandler->AllyTeam(gu->myTeam);
//	grouphandler->team = gu->myTeam;
	CLuaUI::UpdateTeams();

	// setup the teams
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		CTeam* team = teamHandler->Team(a);

		if (team->gaia) {
			continue;
		}

		if (gameSetup->startPosType == CGameSetup::StartPos_ChooseInGame
				&& (team->startPos.x < 0 || team->startPos.z < 0
				|| (team->startPos.x <= 0 && team->startPos.z <= 0))) {
			// if the player didn't choose a start position, choose one for him
			// it should be near the center of his startbox
			const int allyTeam = teamHandler->AllyTeam(a);
			const float xmin = (gs->mapx * SQUARE_SIZE) * gameSetup->allyStartingData[allyTeam].startRectLeft;
			const float zmin = (gs->mapy * SQUARE_SIZE) * gameSetup->allyStartingData[allyTeam].startRectTop;
			const float xmax = (gs->mapx * SQUARE_SIZE) * gameSetup->allyStartingData[allyTeam].startRectRight;
			const float zmax = (gs->mapy * SQUARE_SIZE) * gameSetup->allyStartingData[allyTeam].startRectBottom;
			const float xcenter = (xmin + xmax) / 2;
			const float zcenter = (zmin + zmax) / 2;
			assert(xcenter >= 0 && xcenter < gs->mapx*SQUARE_SIZE);
			assert(zcenter >= 0 && zcenter < gs->mapy*SQUARE_SIZE);
			team->startPos.x = (a - teamHandler->ActiveTeams()) * 4 * SQUARE_SIZE + xcenter;
			team->startPos.z = (a - teamHandler->ActiveTeams()) * 4 * SQUARE_SIZE + zcenter;
		}

		// create a Skirmish AI if required
		// TODO: is this needed?
		if (!gameSetup->hostDemo) {
			const CSkirmishAIHandler::ids_t localSkirmAIs =
					skirmishAIHandler.GetSkirmishAIsInTeam(a, gu->myPlayerNum);
			for (CSkirmishAIHandler::ids_t::const_iterator ai =
					localSkirmAIs.begin(); ai != localSkirmAIs.end(); ++ai) {
				skirmishAIHandler.CreateLocalSkirmishAI(*ai);
			}
		}

		if (a == gu->myTeam) {
			minimap->AddNotification(team->startPos, float3(1.0f, 1.0f, 1.0f), 1.0f);
			game->infoConsole->SetLastMsgPos(team->startPos);
		}
	}

	eventHandler.GameStart();
	net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, speedControl));
#if defined(USE_GML) && GML_ENABLE_SIM
	if(showMTInfo) {
		CKeyBindings::HotkeyList lslist = keyBindings->GetHotkeys("luaui selector");
		std::string lskey = lslist.empty() ? "<none>" : lslist.front();
		logOutput.Print("\n************** SPRING MULTITHREADING VERSION IMPORTANT NOTICE **************");
		logOutput.Print("LUA BASED GRAPHICS WILL CAUSE HIGH CPU LOAD AND SEVERE SLOWDOWNS");
		logOutput.Print("For best results disable LuaShaders in SpringSettings or the Edit Settings menu");
		logOutput.Print("Press " + lskey + " to open the widget list, which allows specific widgets to be disabled");
		logOutput.Print("The LUA-DRAW(MT) value in the upper right corner can be used for guidance");
		logOutput.Print("Safe to use: Autoquit, ImmobileBuilder, MetalMakers, MiniMap Start Boxes\n");
	}
#endif
}



void CGame::SimFrame() {
	ScopedTimer cputimer("CPU load"); // SimFrame

	good_fpu_control_registers("CGame::SimFrame");
	lastFrameTime = SDL_GetTicks();

#ifdef TRACE_SYNC
	//uh->CreateChecksum();
	tracefile << "New frame:" << gs->frameNum << " " << gs->GetRandSeed() << "\n";
#endif

#ifdef USE_MMGR
	if(!(gs->frameNum & 31))
		m_validateAllAllocUnits();
#endif

	eventHandler.GameFrame(gs->frameNum);

	gs->frameNum++;

	if (!skipping) {
		infoConsole->Update();
		waitCommandsAI.Update();
		geometricObjects->Update();
		sound->NewFrame();
		eoh->Update();
		for (size_t a = 0; a < grouphandlers.size(); a++) {
			grouphandlers[a]->Update();
		}
		profiler.Update();

		if (gu->directControl) {
			(playerHandler->Player(gu->myPlayerNum)->dccs).SendStateUpdate(camMove);
		}
		CTeamHighlight::Update(gs->frameNum);
	}

	// everything from here is simulation
	ScopedTimer forced("Sim time"); // don't use SCOPED_TIMER here because this is the only timer needed always

	helper->Update();
	mapDamage->Update();
	pathManager->Update();
	uh->Update();
	groundDecals->Update();
	ph->Update();
	featureHandler->Update();
	GCobEngine.Tick(33);
	GUnitScriptEngine.Tick(33);
	wind.Update();
	loshandler->Update();

	teamHandler->GameFrame(gs->frameNum);
	playerHandler->GameFrame(gs->frameNum);

	lastUpdate = SDL_GetTicks();
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



void CGame::UpdateUI(bool updateCam)
{
	if (updateCam) {
		CPlayer* player = playerHandler->Player(gu->myPlayerNum);
		DirectControlClientState& dccs = player->dccs;

		if (dccs.oldDCpos != ZeroVector) {
			GML_STDMUTEX_LOCK(pos); // UpdateUI

			camHandler->GetCurrentController().SetPos(dccs.oldDCpos);
			dccs.oldDCpos = ZeroVector;
		}
	}

	if (!gu->directControl) {
		float cameraSpeed = 1.0f;

		if (camMove[7]) { cameraSpeed *=  0.1f; }
		if (camMove[6]) { cameraSpeed *= 10.0f; }
		float3 movement = ZeroVector;

		bool disableTracker = false;
		if (camMove[0]) { movement.y += globalRendering->lastFrameTime; disableTracker = true; }
		if (camMove[1]) { movement.y -= globalRendering->lastFrameTime; disableTracker = true; }
		if (camMove[3]) { movement.x += globalRendering->lastFrameTime; disableTracker = true; }
		if (camMove[2]) { movement.x -= globalRendering->lastFrameTime; disableTracker = true; }

		if (!updateCam) {
			if (disableTracker && camHandler->GetCurrentController().DisableTrackingByKey()) {
				unitTracker.Disable();
			}
		} else {
			movement.z = cameraSpeed;
			camHandler->GetCurrentController().KeyMove(movement);
		}

		movement = ZeroVector;

		if ((globalRendering->fullScreen && fullscreenEdgeMove) ||
		    (!globalRendering->fullScreen && windowedEdgeMove)) {

			const int screenW = globalRendering->dualScreenMode ? (globalRendering->viewSizeX << 1): globalRendering->viewSizeX;
			disableTracker = false;

			if (mouse->lasty <                  2 ) { movement.y += globalRendering->lastFrameTime; disableTracker = true; }
			if (mouse->lasty > (globalRendering->viewSizeY - 2)) { movement.y -= globalRendering->lastFrameTime; disableTracker = true; }
			if (mouse->lastx >       (screenW - 2)) { movement.x += globalRendering->lastFrameTime; disableTracker = true; }
			if (mouse->lastx <                  2 ) { movement.x -= globalRendering->lastFrameTime; disableTracker = true; }

			if (!updateCam && disableTracker) {
				unitTracker.Disable();
			}
		}
		if (updateCam) {
			movement.z = cameraSpeed;
			camHandler->GetCurrentController().ScreenEdgeMove(movement);

			if (camMove[4]) { camHandler->GetCurrentController().MouseWheelMove( globalRendering->lastFrameTime * 200 * cameraSpeed); }
			if (camMove[5]) { camHandler->GetCurrentController().MouseWheelMove(-globalRendering->lastFrameTime * 200 * cameraSpeed); }
		}
	}

	if (updateCam) {
		camHandler->GetCurrentController().Update();

		if (chatting && !userWriting) {
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
			chatting = false;
			userInput = "";
			writingPos = 0;
		}

		if (inMapDrawer->wantLabel && !userWriting) {
			if (userInput.size() > 200) {
				// avoid troubles with long lines
				userInput = userInput.substr(0, 200);
				writingPos = (int)userInput.length();
			}
			inMapDrawer->SendPoint(inMapDrawer->waitingPoint, userInput, false);
			inMapDrawer->wantLabel = false;
			userInput = "";
			writingPos = 0;
			ignoreChar = 0;
			inMapDrawer->keyPressed = false;
		}
	}
}




void CGame::MakeMemDump(void)
{
	std::ofstream file(gameServer ? "memdump.txt" : "memdumpclient.txt");

	if (file.bad() || !file.is_open()) {
		return;
	}

	file << "Frame " << gs->frameNum <<"\n";
	for (std::list<CUnit*>::iterator usi = uh->activeUnits.begin(); usi != uh->activeUnits.end(); ++usi) {
		CUnit* u = *usi;
		file << "Unit " << u->id << "\n";
		file << "  xpos " << u->pos.x << " ypos " << u->pos.y << " zpos " << u->pos.z << "\n";
		file << "  heading " << u->heading << " power " << u->power << " experience " << u->experience << "\n";
		file << " health " << u->health << "\n";
	}
	//! we only care about the synced projectile data here
	for (ProjectileContainer::iterator psi = ph->syncedProjectiles.begin(); psi != ph->syncedProjectiles.end(); ++psi) {
		CProjectile* p = *psi;
		file << "Projectile " << p->radius << "\n";
		file << "  xpos " << p->pos.x << " ypos " << p->pos.y << " zpos " << p->pos.z << "\n";
		file << "  xspeed " << p->speed.x << " yspeed " << p->speed.y << " zspeed " << p->speed.z << "\n";
	}
	for(int a=0;a<teamHandler->ActiveTeams();++a){
		file << "LOS-map for team " << a << "\n";
		for (int y = 0; y < gs->mapy>>modInfo.losMipLevel; ++y) {
			file << " ";
			for (int x = 0; x < gs->mapx>>modInfo.losMipLevel; ++x) {
				file << loshandler->losMap[a][y * (gs->mapx>>modInfo.losMipLevel) + x] << " ";
			}
			file << "\n";
		}
	}
	file.close();
	logOutput.Print("Memdump finished");
}



void CGame::GameEnd(const std::vector<unsigned char>& winningAllyTeams )
{
	if (!gameOver) {
		gameOver = true;
		eventHandler.GameOver(winningAllyTeams);

		new CEndGameBox(winningAllyTeams);
#ifdef    HEADLESS
		profiler.PrintProfilingInfo();
#endif // HEADLESS
		CDemoRecorder* record = net->GetDemoRecorder();
		if (record != NULL) {
			// Write CPlayer::Statistics and CTeam::Statistics to demo
			const int numPlayers = playerHandler->ActivePlayers();

			// TODO: move this to a method in CTeamHandler
			int numTeams = teamHandler->ActiveTeams();
			if (gs->useLuaGaia) {
				--numTeams;
			}
			record->SetTime(gs->frameNum / GAME_SPEED, (int)gu->gameTime);
			record->InitializeStats(numPlayers, numTeams);
			// pass the list of winners
			record->SetWinningAllyTeams(winningAllyTeams);

			for (int i = 0; i < numPlayers; ++i) {
				record->SetPlayerStats(i, playerHandler->Player(i)->currentStats);
			}
			for (int i = 0; i < numTeams; ++i) {
				record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
				netcode::PackPacket* buf = new netcode::PackPacket(2 + sizeof(CTeam::Statistics), NETMSG_TEAMSTAT);
				*buf << (uint8_t)teamHandler->Team(i)->teamNum << teamHandler->Team(i)->currentStats;
				net->Send(buf);
			}
		}

		// pass the winner info to the host in the case it's a dedicated server
		net->Send(CBaseNetProtocol::Get().SendGameOver(gu->myPlayerNum, winningAllyTeams));
	}
}

void CGame::SendNetChat(std::string message, int destination)
{
	if (message.empty()) {
		return;
	}
	if (destination == -1) // overwrite
	{
		destination = ChatMessage::TO_EVERYONE;
		if ((message.length() >= 2) && (message[1] == ':')) {
			const char lower = tolower(message[0]);
			if (lower == 'a') {
				destination = ChatMessage::TO_ALLIES;
				message = message.substr(2);
			}
			else if (lower == 's') {
				destination = ChatMessage::TO_SPECTATORS;
				message = message.substr(2);
			}
		}
	}
	if (message.size() > 128) {
		message.resize(128); // safety
	}
	ChatMessage buf(gu->myPlayerNum, destination, message);
	net->Send(buf.Pack());
}


void CGame::HandleChatMsg(const ChatMessage& msg)
{
	if ((msg.fromPlayer < 0) ||
		((msg.fromPlayer >= playerHandler->ActivePlayers()) &&
			(static_cast<unsigned int>(msg.fromPlayer) != SERVER_PLAYER))) {
		return;
	}

	string s = msg.msg;

	if (!s.empty()) {
		CPlayer* player = (msg.fromPlayer >= 0 && static_cast<unsigned int>(msg.fromPlayer) == SERVER_PLAYER) ? 0 : playerHandler->Player(msg.fromPlayer);
		const bool myMsg = (msg.fromPlayer == gu->myPlayerNum);

		string label;
		if (!player) {
			label = "> ";
		} else if (player->spectator) {
			if (player->isFromDemo)
				// make clear that the message is from the replay
				label = "[" + player->name + " (replay)" + "] ";
			else
				// its from a spectator not from replay
				label = "[" + player->name + "] ";
		} else {
			// players are always from a replay (if its a replay and not a game)
			label = "<" + player->name + "> ";
		}

		/*
		- If you're spectating you always see all chat messages.
		- If you're playing you see:
		- TO_ALLIES-messages sent by allied players,
		- TO_SPECTATORS-messages sent by yourself,
		- TO_EVERYONE-messages from players,
		- TO_EVERYONE-messages from spectators only if noSpectatorChat is off!
		- private messages if they are for you ;)
		*/

		if (msg.destination == ChatMessage::TO_ALLIES && player) {
			const int msgAllyTeam = teamHandler->AllyTeam(player->team);
			const bool allied = teamHandler->Ally(msgAllyTeam, gu->myAllyTeam);
			if (gu->spectating || (allied && !player->spectator)) {
				logOutput.Print(label + "Allies: " + s);
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_SPECTATORS) {
			if (gu->spectating || myMsg) {
				logOutput.Print(label + "Spectators: " + s);
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_EVERYONE) {
			const bool specsOnly = noSpectatorChat && (player && player->spectator);
			if (gu->spectating || !specsOnly) {
				logOutput.Print(label + s);
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination < playerHandler->ActivePlayers())
		{
			if (msg.destination == gu->myPlayerNum && player && !player->spectator) {
				logOutput.Print(label + "Private: " + s);
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
			else if (player->playerNum == gu->myPlayerNum)
			{
				LogObject() << "You whispered " << player->name << ": " << s;
			}
		}
	}

	eoh->GotChatMsg(msg.msg.c_str(), msg.fromPlayer);
}



void CGame::StartSkip(int toFrame) {
	if (skipping) {
		logOutput.Print("ERROR: skipping appears to be busted (%i)\n", skipping);
	}

	skipStartFrame = gs->frameNum;
	skipEndFrame = toFrame;

	if (skipEndFrame <= skipStartFrame) {
		logOutput.Print("Already passed %i (%i)\n", skipEndFrame / GAME_SPEED, skipEndFrame);
		return;
	}

	skipTotalFrames = skipEndFrame - skipStartFrame;
	skipSeconds = (float)(skipTotalFrames) / (float)GAME_SPEED;

	skipSoundmute = sound->IsMuted();
	if (!skipSoundmute)
		sound->Mute(); // no sounds

	skipOldSpeed     = gs->speedFactor;
	skipOldUserSpeed = gs->userSpeedFactor;
	const float speed = 1.0f;
	gs->speedFactor     = speed;
	gs->userSpeedFactor = speed;

	skipLastDraw = SDL_GetTicks();

	skipping = true;
}

void CGame::EndSkip() {
	skipping = false;

	gu->gameTime    += skipSeconds;
	gu->modGameTime += skipSeconds;

	gs->speedFactor     = skipOldSpeed;
	gs->userSpeedFactor = skipOldUserSpeed;

	if (!skipSoundmute)
		sound->Mute(); // sounds back on

	logOutput.Print("Skipped %.1f seconds\n", skipSeconds);
}



void CGame::DrawSkip(bool blackscreen) {
	const int framesLeft = (skipEndFrame - gs->frameNum);
	if(blackscreen) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glColor3f(0.5f, 1.0f, 0.5f);
	font->glFormat(0.5f, 0.55f, 2.5f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Skipping %.1f game seconds", skipSeconds);
	glColor3f(1.0f, 1.0f, 1.0f);
	font->glFormat(0.5f, 0.45f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "(%i frames left)", framesLeft);

	const float ff = (float)framesLeft / (float)skipTotalFrames;
	glDisable(GL_TEXTURE_2D);
	const float b = 0.004f; // border
	const float yn = 0.35f;
	const float yp = 0.38f;
	glColor3f(0.2f, 0.2f, 1.0f);
	glRectf(0.25f - b, yn - b, 0.75f + b, yp + b);
	glColor3f(0.25f + (0.75f * ff), 1.0f - (0.75f * ff), 0.0f);
	glRectf(0.5 - (0.25f * ff), yn, 0.5f + (0.25f * ff), yp);
}



void CGame::ReloadCOB(const string& msg, int player)
{
	if (!gs->cheatEnabled) {
		logOutput.Print("reloadcob can only be used if cheating is enabled");
		return;
	}
	const string unitName = msg;
	if (unitName.empty()) {
		logOutput.Print("Missing unit name");
		return;
	}
	const UnitDef* udef = unitDefHandler->GetUnitDefByName(unitName);
	if (udef==NULL) {
		logOutput.Print("Unknown unit name: \"%s\"", unitName.c_str());
		return;
	}
	const CCobFile* oldScript = GCobFileHandler.GetScriptAddr(udef->scriptPath);
	if (oldScript == NULL) {
		logOutput.Print("Unknown COB script for unit \"%s\": %s", unitName.c_str(), udef->scriptPath.c_str());
		return;
	}
	CCobFile* newScript = GCobFileHandler.ReloadCobFile(udef->scriptPath);
	if (newScript == NULL) {
		logOutput.Print("Could not load COB script for unit \"%s\" from: %s", unitName.c_str(), udef->scriptPath.c_str());
		return;
	}
	int count = 0;
	for (size_t i = 0; i < uh->MaxUnits(); i++) {
		CUnit* unit = uh->units[i];
		if (unit != NULL) {
			CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);
			if (cob != NULL && cob->GetScriptAddr() == oldScript) {
				count++;
				delete unit->script;
				unit->script = new CCobInstance(*newScript, unit);
				unit->script->Create();
			}
		}
	}
	logOutput.Print("Reloaded cob script for %i units", count);
}


void CGame::SelectUnits(const string& line)
{
	const vector<string> &args = CSimpleParser::Tokenize(line, 0);
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
			if ((unitIndex < 0) || (static_cast<unsigned int>(unitIndex) >= uh->MaxUnits())) {
				continue; // bad index
			}
			CUnit* unit = uh->units[unitIndex];
			if (unit == NULL) {
				continue; // bad pointer
			}
			if (!gu->spectatingFullSelect) {
				const CUnitSet& teamUnits = teamHandler->Team(gu->myTeam)->units;
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

	GML_RECMUTEX_LOCK(sel); // SelectCycle

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


//FIXME remove!
void CGame::ReColorTeams()
{
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		CTeam* team = teamHandler->Team(t);
		team->origColor[0] = team->color[0];
		team->origColor[1] = team->color[1];
		team->origColor[2] = team->color[2];
		team->origColor[3] = team->color[3];
	}
}

bool CGame::HasLag() const
{
	unsigned timeNow = SDL_GetTicks();
	if (!gs->paused && (timeNow > (lastFrameTime + (500.0f / gs->speedFactor)))) {
		return true;
	} else {
		return false;
	}
}


void CGame::SaveGame(const std::string& filename, bool overwrite)
{
	if (filesystem.CreateDirectory("Saves")) {
		if (overwrite || !filesystem.FileExists(filename)) {
			logOutput.Print("Saving game to %s\n", filename.c_str());
			ILoadSaveHandler* ls = ILoadSaveHandler::Create();
			ls->mapName = gameSetup->mapName;
			ls->modName = modInfo.filename;
			ls->SaveGame(filename);
			delete ls;
		}
		else {
			logOutput.Print("File %s already exists(use /save -y to override)\n", filename.c_str());
		}
	}
}


void CGame::ReloadGame()
{
	if (saveFile) {
		// This reloads heightmap, triggers Load call-in, etc.
		// Inside the Load call-in, Lua can ensure old units are wiped before new ones are placed.
		saveFile->LoadGame();
	}
	else {
		logOutput.Print("Can only reload game when game has been started from a savegame");
	}
}
