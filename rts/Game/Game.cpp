/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>
#include <locale>
#include <sstream>
#include <stdexcept>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <SDL_keyboard.h>

#include "mmgr.h"

#include "Game.h"
#include "Camera.h"
#include "CameraHandler.h"
#include "ChatMessage.h"
#include "ClientSetup.h"
#include "CommandMessage.h"
#include "ConsoleHistory.h"
#include "GameHelper.h"
#include "GameServer.h"
#include "GameVersion.h"
#include "GameSetup.h"
#include "GlobalUnsynced.h"
#include "LoadScreen.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "PlayerRoster.h"
#include "WaitCommandsAI.h"
#include "WordCompletion.h"
#include "OSCStatsSender.h"
#include "IVideoCapturing.h"
#include "InMapDraw.h"
#include "InMapDrawModel.h"
#include "SyncedActionExecutor.h"
#include "SyncedGameCommands.h"
#include "UnsyncedActionExecutor.h"
#include "UnsyncedGameCommands.h"
#include "Game/UI/UnitTracker.h"
#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/glFont.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/Screenshot.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDrawView.h"
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
#include "Sim/Units/Scripts/CobEngine.h"
#include "Sim/Units/Scripts/UnitScriptEngine.h"
#include "Sim/Units/UnitDefHandler.h"
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
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FPUCheck.h"
#include "System/GlobalConfig.h"
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
#include "System/Platform/Watchdog.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/SyncedPrimitiveIO.h"
#include "System/Sync/SyncTracer.h"
#include "System/TimeProfiler.h"

#include <boost/cstdint.hpp>

#undef CreateDirectory

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif


CONFIG(bool, WindowedEdgeMove).defaultValue(true);
CONFIG(bool, FullscreenEdgeMove).defaultValue(true);
CONFIG(bool, ShowFPS).defaultValue(false);
CONFIG(bool, ShowClock).defaultValue(true);
CONFIG(bool, ShowSpeed).defaultValue(false);
CONFIG(bool, ShowMTInfo).defaultValue(true);
CONFIG(int, SpeedControl).defaultValue(0);
CONFIG(int, ShowPlayerInfo).defaultValue(1);
CONFIG(float, GuiOpacity).defaultValue(0.8f);
CONFIG(std::string, InputTextGeo).defaultValue("");
CONFIG(bool, LuaModUICtrl).defaultValue(true);


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
	CR_MEMBER(mtInfoCtrl),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(gameID),
//	CR_MEMBER(script),
//	CR_MEMBER(infoConsole),
//	CR_MEMBER(consoleHistory),
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



CGame::CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile) :
	gameDrawMode(gameNotDrawing),
	defsParser(NULL),
	fps(0),
	thisFps(0),
	lastSimFrame(-1),

	totalGameTime(0),

	hideInterface(false),

	gameOver(false),

	noSpectatorChat(false),
	finishedLoading(false),

	infoConsole(NULL),
	consoleHistory(NULL),

	skipping(false),
	playing(false),
	chatting(false),
	lastFrameTime(0),

	leastQue(0),
	timeLeft(0.0f),
	consumeSpeed(1.0f),
	luaLockTime(0),
	luaExportSize(0),

	saveFile(saveFile),

	worldDrawer(NULL)
{
	game = this;

	memset(gameID, 0, sizeof(gameID));

	time(&starttime);
	lastTick = clock();

	for (int a = 0; a < 8; ++a) { camMove[a] = false; }
	for (int a = 0; a < 4; ++a) { camRot[a] = false; }

	// set "Headless" in config overlay (not persisted)
	const bool isHeadless =
#ifdef HEADLESS
			true;
#else
			false;
#endif
	configHandler->Set("Headless", isHeadless ? 1 : 0, true);

	//FIXME move to MouseHandler!
	windowedEdgeMove   = configHandler->GetBool("WindowedEdgeMove");
	fullscreenEdgeMove = configHandler->GetBool("FullscreenEdgeMove");

	showFPS   = configHandler->GetBool("ShowFPS");
	showClock = configHandler->GetBool("ShowClock");
	showSpeed = configHandler->GetBool("ShowSpeed");
	showMTInfo = configHandler->GetBool("ShowMTInfo");
	mtInfoCtrl = 0;

	speedControl = configHandler->GetInt("SpeedControl");

	playerRoster.SetSortTypeByCode((PlayerRoster::SortType)configHandler->GetInt("ShowPlayerInfo"));

	CInputReceiver::guiAlpha = configHandler->GetFloat("GuiOpacity");

	const string inputTextGeo = configHandler->GetString("InputTextGeo");
	ParseInputTextGeometry("default");
	ParseInputTextGeometry(inputTextGeo);

	userInput  = "";
	writingPos = 0;
	userPrompt = "";

	CLuaHandle::SetModUICtrl(configHandler->GetBool("LuaModUICtrl"));

	modInfo.Init(modName.c_str());

	if (!mapInfo) {
		mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);
	}

	showMTInfo = (showMTInfo && globalConfig->GetMultiThreadLua() <= 3) ? globalConfig->GetMultiThreadLua() : 0;

	if (!sideParser.Load()) {
		throw content_error(sideParser.GetErrorLog());
	}
}

CGame::~CGame()
{
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "]";
#endif

	// TODO move these to the end of this dtor, once all action-executors are registered by their respective engine sub-parts
	UnsyncedGameCommands::DestroyInstance();
	SyncedGameCommands::DestroyInstance();

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

	CWordCompletion::DestroyInstance();

	SafeDelete(worldDrawer);
	SafeDelete(guihandler);
	SafeDelete(minimap);
	SafeDelete(resourceBar);
	SafeDelete(tooltip); // CTooltipConsole*
	SafeDelete(infoConsole);
	SafeDelete(consoleHistory);
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
	SafeDelete(icon::iconHandler);
	SafeDelete(geometricObjects);
	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	SafeDelete(featureHandler); // depends on unitHandler (via ~CFeature)
	SafeDelete(uh); // CUnitHandler*, depends on modelParser (via ~CUnit)
	SafeDelete(ph); // CProjectileHandler*

	SafeDelete(cubeMapHandler);
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
	SafeDelete(gCEG);
	SafeDelete(helper);
	SafeDelete((mapInfo = const_cast<CMapInfo*>(mapInfo)));

	CGroundMoveType::DeleteLineTable();
	CCategoryHandler::RemoveInstance();

	for (unsigned int i = 0; i < grouphandlers.size(); i++) {
		SafeDelete(grouphandlers[i]);
	}
	grouphandlers.clear();

	SafeDelete(saveFile); // ILoadSaveHandler, depends on vfsHandler via ~IArchive
	SafeDelete(vfsHandler);
	SafeDelete(archiveScanner);

	SafeDelete(gameServer);
	SafeDelete(net);

	game = NULL;
}


void CGame::LoadGame(const std::string& mapName)
{
#ifdef USE_GML
	set_threadnum(GML_LOAD_THREAD_NUM);
#endif

	Watchdog::RegisterThread(WDT_LOAD);

	if (!gu->globalQuit) LoadDefs();
	if (!gu->globalQuit) LoadSimulation(mapName);
	if (!gu->globalQuit) LoadRendering();
	if (!gu->globalQuit) LoadInterface();
	if (!gu->globalQuit) LoadLua();
	if (!gu->globalQuit) LoadFinalize();

	if (!gu->globalQuit && saveFile) {
		loadscreen->SetLoadMessage("Loading game");
		saveFile->LoadGame();
	}

	Watchdog::DeregisterThread(WDT_LOAD);
}


void CGame::LoadDefs()
{
	{
		loadscreen->SetLoadMessage("Loading Radar Icons");
		icon::iconHandler = new icon::CIconHandler();
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

void CGame::LoadSimulation(const std::string& mapName)
{
	// after this, other components are able to register chat action-executors
	SyncedGameCommands::CreateInstance();
	UnsyncedGameCommands::CreateInstance();

	CWordCompletion::CreateInstance();

	// simulation components
	helper = new CGameHelper();
	ground = new CGround();

	loadscreen->SetLoadMessage("Parsing Map Information");

	readmap = CReadMap::LoadMap(mapName);
	groundBlockingObjectMap = new CGroundBlockingObjectMap(gs->mapSquares);

	loadscreen->SetLoadMessage("Creating Smooth Height Mesh");
	smoothGround = new SmoothHeightMesh(ground, float3::maxxpos, float3::maxzpos, SQUARE_SIZE * 2, SQUARE_SIZE * 40);

	loadscreen->SetLoadMessage("Creating QuadField & CEGs");
	moveinfo = new CMoveInfo();
	qf = new CQuadField();
	damageArrayHandler = new CDamageArrayHandler();
	explGenHandler = new CExplosionGeneratorHandler();
	gCEG = new CCustomExplosionGenerator();

	{
		//! FIXME: these five need to be loaded before featureHandler
		//! (maps with features have their models loaded at startup)
		modelParser = new C3DModelLoader();

		loadscreen->SetLoadMessage("Creating Unit Textures");
		texturehandler3DO = new C3DOTextureHandler();
		texturehandlerS3O = new CS3OTextureHandler();

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

	geometricObjects = new CGeometricObjects();

	inMapDrawerModel = new CInMapDrawModel();
	inMapDrawer = new CInMapDraw();

	syncedGameCommands->AddDefaultActionExecutors();
	unsyncedGameCommands->AddDefaultActionExecutors();
}

void CGame::LoadRendering()
{
	worldDrawer = new CWorldDrawer();
}

void CGame::SetupRenderingParams()
{
	glLightfv(GL_LIGHT1, GL_AMBIENT, mapInfo->light.unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, mapInfo->light.unitSunColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, mapInfo->light.unitAmbientColor);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	IBaseSky::SetFog();

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

		const std::map<std::string, int>& unitDefs = unitDefHandler->unitDefIDsByName;
		const std::map<std::string, const FeatureDef*>& featureDefs = featureHandler->GetFeatureDefs();

		std::map<std::string, int>::const_iterator uit;
		std::map<std::string, const FeatureDef*>::const_iterator fit;

		for (uit = unitDefs.begin(); uit != unitDefs.end(); ++uit) {
			wordCompletion->AddWord(uit->first + " ", false, true, false);
		}
		for (fit = featureDefs.begin(); fit != featureDefs.end(); ++fit) {
			wordCompletion->AddWord(fit->first + " ", false, true, false);
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


int CGame::KeyPressed(unsigned short key, bool isRepeat)
{
	if (!gameOver && !isRepeat) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.keyPresses++;
	}

	// Get the list of possible key actions
	const CKeySet ks(key, false);
	const CKeyBindings::ActionList& actionList = keyBindings->GetActionList(ks);

	if (!hotBinding.empty()) {
		if (key == SDLK_ESCAPE) {
			hotBinding.clear();
		}
		else if (!keyCodes->IsModifier(key) && (key != keyBindings->GetFakeMetaKey())) {
			string cmd = "bind";
			cmd += " " + ks.GetString(false);
			cmd += " " + hotBinding;
			keyBindings->Command(cmd);
			hotBinding.clear();
			logOutput.Print("%s", cmd.c_str());
		}
		return 0;
	}

	if (userWriting) {
		for (unsigned int actionIndex = 0; actionIndex < actionList.size(); actionIndex++) {
			if (ProcessKeyPressAction(key, actionList[actionIndex])) {
				// the key was used, ignore it (ex: alt+a)
				ignoreNextChar = (actionIndex < (actionList.size() - 1));
				break;
			}
		}

		return 0;
	}

	// try the input receivers
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv = *ri;
		if (recv && recv->KeyPressed(key, isRepeat)) {
			return 0;
		}
	}

	// try our list of actions
	for (unsigned int i = 0; i < actionList.size(); ++i) {
		if (ActionPressed(key, actionList[i], isRepeat)) {
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
		CInputReceiver* recv = *ri;
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
#if defined(USE_GML) && GML_ENABLE_SIM
		extern int backupSize;
		luaExportSize = backupSize;
		backupSize = 0;
#endif
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

	if (gs->frameNum == 0 || gs->paused)
		eventHandler.UpdateObjects(); // we must add new rendering objects even if the game has not started yet

	ClientReadNet();

	if (net->NeedsReconnect() && !gameOver) {
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

	if (skipping) {
		if (skipLastDraw + 500 > currentTime) // render at 2 FPS
			return true;
		skipLastDraw = currentTime;
#if defined(USE_GML) && GML_ENABLE_SIM
		extern volatile int gmlMultiThreadSim;
		if (!gmlMultiThreadSim)
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
			sky->GetLight()->Update();
			water->Update();
		}
	}


	if (lastSimFrame != gs->frameNum) {
		CInputReceiver::CollectGarbage();
	}

	//! always update ExtraTexture & SoundListener with <=30Hz (even when paused)
	if (!skipping) {
		bool newSimFrame = (lastSimFrame != gs->frameNum);
		if (newSimFrame || gs->paused) {
			static unsigned lastUpdate = SDL_GetTicks();
			unsigned deltaMSec = currentTime - lastUpdate;

			if (!gs->paused || deltaMSec >= 1000/30.f) {
				lastUpdate = currentTime;

				{
					SCOPED_TIMER("ExtraTexture");
					gd->UpdateExtraTexture();
				}

				// TODO call only when camera changed
				sound->UpdateListener(camera->pos, camera->forward, camera->up, deltaMSec / 1000.f);
			}
		}
	}

	lastSimFrame = gs->frameNum;

	if (!skipping)
		UpdateUI(true);

	SetDrawMode(gameNormalDraw);

 	if (luaUI)    { luaUI->CheckStack(); luaUI->ExecuteDelayedXCalls(); luaUI->ExecuteUIEventBatch(); }
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
	worldDrawer->Update();
	mouse->UpdateCursors();
	mouse->EmptyMsgQueUpdate();
	guihandler->Update();

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
		SCOPED_TIMER("Shadows/Reflections");

		if (shadowHandler->shadowsLoaded && (gd->drawMode != CBaseGroundDrawer::drawLos)) {
			// NOTE: shadows don't work in LOS mode, gain a few fps (until it's fixed)
			SetDrawMode(gameShadowDraw);
			shadowHandler->CreateShadows();
			SetDrawMode(gameNormalDraw);
		}

		cubeMapHandler->UpdateReflectionTexture();

		if (sky->GetLight()->IsDynamic()) {
			cubeMapHandler->UpdateSpecularTexture();
			sky->UpdateSkyTexture();
			readmap->UpdateShadingTexture();
		}

		if (FBO::IsSupported())
			FBO::Unbind();

		glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);	// Clear Screen And Depth&Stencil Buffer
	camera->Update(false);

	if (doDrawWorld) {
		worldDrawer->Draw();
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

	hudDrawer->Draw((gu->GetMyPlayer())->fpsController.GetControllee());
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
		int cit = (int)GML_LOCK_TIME() * (int)fps;
		if (cit > luaLockTime)
			++luaLockTime;
		else if (cit < luaLockTime)
			--luaLockTime;

		if (showMTInfo == 1 || showMTInfo == 2) {
			float lockPercent = (float)luaLockTime / 10.0f;
			if (lockPercent >= 0.1f) {
				if((mtInfoCtrl = std::min(mtInfoCtrl + 1, 5)) == 3) mtInfoCtrl = 5;
			}
			else if((mtInfoCtrl = std::max(mtInfoCtrl - 1, 0)) == 2) mtInfoCtrl = 0;
			if (mtInfoCtrl >= 3) {
				char buf[40];
				SNPRINTF(buf, sizeof(buf), "LUA-SYNC-CPU(MT): %2.1f%%", lockPercent);
				float4 warncol(lockPercent >= 10.0f && (currentTime & 128) ?
					0.5f : std::max(0.0f, std::min(lockPercent / 5.0f, 1.0f)), std::max(0.0f, std::min(2.0f - lockPercent / 5.0f, 1.0f)), 0.0f, 1.0f);
				smallFont->SetColors(&warncol, NULL);
				smallFont->glPrint(0.99f, 0.88f, 1.0f, font_options, buf);
			}
		}
		else if (showMTInfo == 3) {
			float ek = (float)luaExportSize / 1000.0f;
			if (ek >= 0.1f) {
				if((mtInfoCtrl = std::min(mtInfoCtrl + 1, 5)) == 3) mtInfoCtrl = 5;
			}
			else if ((mtInfoCtrl = std::max(mtInfoCtrl - 1, 0)) == 2) mtInfoCtrl = 0;
			if (mtInfoCtrl >= 3) {
				char buf[40];
				SNPRINTF(buf, sizeof(buf), "LUA-EXP-SIZE(MT): %2.1fK", ek);
				float4 warncol(ek >= 10.0f && (currentTime & 128) ?
					0.5f : std::max(0.0f, std::min(ek / 5.0f, 1.0f)), std::max(0.0f, std::min(2.0f - ek / 5.0f, 1.0f)), 0.0f, 1.0f);
				smallFont->SetColors(&warncol, NULL);
				smallFont->glPrint(0.99f, 0.88f, 1.0f, font_options, buf);
			}
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
				if (p->ping != PATHING_FLAG || gs->frameNum != 0) {
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
					float4 pingcolor(!p->spectator && globalConfig->reconnectTimeout > 0 && ping > 1000 * globalConfig->reconnectTimeout &&
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
	if (skipping)
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
			infoConsole->SetLastMsgPos(team->startPos);
		}
	}

	eventHandler.GameStart();
	net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, speedControl));
#if defined(USE_GML) && GML_ENABLE_SIM
	if (showMTInfo >= 1) {
		logOutput.Print("\n************** SPRING MULTITHREADING VERSION IMPORTANT NOTICE **************");
		logOutput.Print("Engine or game settings have forced Spring MT to use compatibility mode %d", showMTInfo);
		if(showMTInfo == 1) {
			CKeyBindings::HotkeyList lslist = keyBindings->GetHotkeys("luaui selector");
			std::string lskey = lslist.empty() ? "<none>" : lslist.front();
			logOutput.Print("If your game uses lua based rendering, it may run very slow with Spring MT");
			logOutput.Print("A high LUA-SYNC-CPU(MT) value in the upper right corner could indicate a problem");
			logOutput.Print("Consider changing the engine setting 'MultiThreadLua' to 2 to improve performance,");
			logOutput.Print("or try to disable LuaShaders and all rendering widgets (press " + lskey + ")\n");
		}
		else if(showMTInfo == 2) {
			logOutput.Print("If your game uses lua gadget based rendering, it may run very slow with Spring MT");
			logOutput.Print("A high LUA-SYNC-CPU(MT) value in the upper right corner could indicate a problem\n");
		}
		else if(showMTInfo == 3) {
			logOutput.Print("If your game uses lua gadgets that export data, it may run very slow with Spring MT");
			logOutput.Print("A high LUA-EXP-SIZE(MT) value in the upper right corner could indicate a problem\n");
		}
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
	if (!(gs->frameNum & 31))
		m_validateAllAllocUnits();
#endif

	// Important: gs->frameNum must be updated *before* GameFrame is called,
	// or any call-outs called by Lua will see a stale gs->frameNum.
	// (e.g. effective TTL of CEGs emitted in GameFrame will be reduced...)
	// It must still be passed the old frameNum because Lua may depend on it.
	// (e.g. initialization in frame 0...)
	eventHandler.GameFrame(gs->frameNum++);

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

		(playerHandler->Player(gu->myPlayerNum)->fpsController).SendStateUpdate(camMove);

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
		FPSUnitController& fpsCon = player->fpsController;

		if (fpsCon.oldDCpos != ZeroVector) {
			GML_STDMUTEX_LOCK(pos); // UpdateUI

			camHandler->GetCurrentController().SetPos(fpsCon.oldDCpos);
			fpsCon.oldDCpos = ZeroVector;
		}
	}

	if (!gu->fpsMode) {
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

		if (inMapDrawer->IsWantLabel() && !userWriting) {
			if (userInput.size() > 200) {
				// avoid troubles with long lines
				userInput = userInput.substr(0, 200);
				writingPos = (int)userInput.length();
			}
			inMapDrawer->SendWaitingInput(userInput);
			userInput = "";
			writingPos = 0;
			ignoreChar = 0;
		}
	}
}




void CGame::MakeMemDump()
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
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		file << "LOS-map for team " << a << "\n";
		for (int y = 0; y < gs->mapy>>modInfo.losMipLevel; ++y) {
			file << " ";
			for (int x = 0; x < gs->mapx >> modInfo.losMipLevel; ++x) {
				file << loshandler->losMaps[a][y * (gs->mapx >> modInfo.losMipLevel) + x] << " ";
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

	eoh->SendChatMessage(msg.msg.c_str(), msg.fromPlayer);
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
	if (blackscreen) {
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

void CGame::ReloadCEGs(const std::string& tag) {
	gCEG->RefreshCache(tag);
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
			++fit;
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



void CGame::ActionReceived(const Action& action, int playerID)
{
	const ISyncedActionExecutor* executor = syncedGameCommands->GetActionExecutor(action.command);

	if (executor != NULL) {
		// an executor for that action was found
		SyncedAction syncedAction(action, playerID);
		executor->ExecuteAction(syncedAction);
	} else if (gs->frameNum > 1) {
		if (luaRules) luaRules->SyncedActionFallback(action.rawline, playerID);
		if (luaGaia) luaGaia->SyncedActionFallback(action.rawline, playerID);
	}
}


bool CGame::ActionPressed(unsigned int key, const Action& action, bool isRepeat)
{
	const IUnsyncedActionExecutor* executor = unsyncedGameCommands->GetActionExecutor(action.command);

	if (executor != NULL) {
		// an executor for that action was found
		UnsyncedAction unsyncedAction(action, key, isRepeat);
		executor->ExecuteAction(unsyncedAction);
	} else {
		static std::set<std::string> serverCommands = std::set<std::string>(commands, commands+numCommands);
		if (serverCommands.find(action.command) != serverCommands.end())
		{
			CommandMessage pckt(action, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}

		if (!Console::Instance().ExecuteAction(action))
		{
			if (guihandler != NULL) // maybe a widget is interested?
				guihandler->PushLayoutCommand(action.rawline, false);
			return false;
		}
	}

	return false;
}
