/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>
#include <locale>
#include <fstream>
#include <stdexcept>
#include <functional> // C++11

#include <SDL_keyboard.h>

#include "Game.h"
#include "Benchmark.h"
#include "Camera.h"
#include "CameraHandler.h"
#include "ChatMessage.h"
#include "ClientSetup.h"
#include "CommandMessage.h"
#include "ConsoleHistory.h"
#include "GameHelper.h"
#include "GameVersion.h"
#include "GameSetup.h"
#include "GlobalUnsynced.h"
#include "LoadScreen.h"
#include "SelectedUnitsHandler.h"
#include "WaitCommandsAI.h"
#include "WordCompletion.h"
#include "IVideoCapturing.h"
#include "InMapDraw.h"
#include "InMapDrawModel.h"
#include "SyncedActionExecutor.h"
#include "SyncedGameCommands.h"
#include "UnsyncedActionExecutor.h"
#include "UnsyncedGameCommands.h"
#include "Game/GUI/PlayerRoster.h"
#include "Game/GUI/PlayerRosterDrawer.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/GameServer.h"
#include "Game/UI/UnitTracker.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/glFont.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/Screenshot.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDrawView.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Models/ModelDrawer.h"
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
#include "Lua/LuaUI.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
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
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/ClassicGroundMoveType.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/CobEngine.h"
#include "Sim/Units/Scripts/UnitScriptEngine.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Groups/GroupHandler.h"
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
#include "System/Sync/FPUCheck.h"
#include "System/GlobalConfig.h"
#include "System/myMath.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/SpringApp.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Log/ILog.h"
#include "System/Net/PackPacket.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Watchdog.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/DumpState.h"
#include "System/Sync/SyncedPrimitiveIO.h"
#include "System/Sync/SyncTracer.h"
#include "System/TimeProfiler.h"

#include <boost/cstdint.hpp>
#include "lib/lua/include/LuaUser.h"

#undef CreateDirectory

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*>* gmlProcessor;
#endif


CONFIG(bool, WindowedEdgeMove).defaultValue(true).description("Sets whether moving the mouse cursor to the screen edge will move the camera across the map.");
CONFIG(bool, FullscreenEdgeMove).defaultValue(true).description("see [[#WindowedEdgeMove]], just for fullscreen mode");
CONFIG(bool, ShowFPS).defaultValue(false).description("Displays current framerate.");
CONFIG(bool, ShowClock).defaultValue(true).description("Displays a clock on the top-right corner of the screen showing the elapsed time of the current game.");
CONFIG(bool, ShowSpeed).defaultValue(false).description("Displays current game speed.");
CONFIG(bool, ShowMTInfo).defaultValue(true);
CONFIG(float, MTInfoThreshold).defaultValue(1.0f);
CONFIG(int, ShowPlayerInfo).defaultValue(1);
CONFIG(float, GuiOpacity).defaultValue(0.8f).minimumValue(0.0f).maximumValue(1.0f).description("Sets the opacity of the built-in Spring UI. Generally has no effect on LuaUI widgets. Can be set in-game using shift+, to decrease and shift+. to increase.");
CONFIG(std::string, InputTextGeo).defaultValue("");
CONFIG(bool, LuaModUICtrl).defaultValue(true);


CGame* game = NULL;


CR_BIND(CGame, (std::string(""), std::string(""), NULL));

CR_REG_METADATA(CGame, (
	CR_IGNORED(finishedLoading),
	CR_IGNORED(thisFps),
	CR_MEMBER(lastSimFrame),
	CR_IGNORED(frameStartTime),
	CR_IGNORED(lastUpdateTime),
	CR_IGNORED(lastSimFrameTime),
	CR_IGNORED(lastDrawFrameTime),
	CR_IGNORED(lastFrameTime),
	CR_IGNORED(updateDeltaSeconds),
	CR_MEMBER(totalGameTime),
	CR_MEMBER(userInputPrefix),
	CR_IGNORED(chatSound),
	CR_IGNORED(camMove),
	CR_IGNORED(camRot),
	CR_MEMBER(hideInterface),
	CR_MEMBER(gameOver),
	CR_IGNORED(gameDrawMode),
	CR_IGNORED(windowedEdgeMove),
	CR_IGNORED(fullscreenEdgeMove),
	CR_MEMBER(showFPS),
	CR_MEMBER(showClock),
	CR_MEMBER(showSpeed),
	CR_MEMBER(showMTInfo),
	CR_MEMBER(mtInfoThreshold),
	CR_MEMBER(mtInfoCtrl),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(gameID),
	//CR_MEMBER(infoConsole),
	//CR_MEMBER(consoleHistory),
	CR_IGNORED(hotBinding),
	CR_IGNORED(inputTextPosX),
	CR_IGNORED(inputTextPosY),
	CR_IGNORED(inputTextSizeX),
	CR_IGNORED(inputTextSizeY),
	CR_IGNORED(skipping),
	CR_MEMBER(playing),
	CR_IGNORED(unconsumedFrames),
	CR_IGNORED(msgProcTimeLeft),
	CR_IGNORED(consumeSpeed),
	CR_IGNORED(lastframe),

	CR_POSTLOAD(PostLoad)
));



class JobDispatcher
{
public:
	struct Job {
		Job()
		: freq(0)
		//, inSim(false)
		//, inDraw(false)
		, startDirect(true)
		, catchUp(false)
		{}

		std::function<bool()> f; // allows us to use lambdas
		float freq;
		//bool inSim;
		//bool inDraw;
		bool startDirect;
		bool catchUp;
	};

public:
	static void AddJob(Job j) {
		jobs[spring_gettime() + spring_time(j.startDirect ? 0 : 1000.0f/j.freq)] = j;
	}

	static void Update() {
		const spring_time now = spring_gettime();
		std::map<spring_time, Job>::iterator it = jobs.begin();
		while (it != jobs.end() && it->first <= now) {
			Job* j = &it->second;
			if (j->f()) {
				spring_time nextCallTime = j->catchUp ? it->first : spring_gettime();
				nextCallTime += spring_time(1000.0f / j->freq);
				jobs[nextCallTime] = *j;
			}
			jobs.erase(it); //FIXME remove by range? (faster!)
			it = jobs.begin();
		}
	}

private:
	static std::map<spring_time, Job> jobs;
};

std::map<spring_time, JobDispatcher::Job> JobDispatcher::jobs;


DO_ONCE_FNC(
	JobDispatcher::Job j;
	j.f = []() -> bool {
		SCOPED_TIMER("CollectGarbage");
		eventHandler.CollectGarbage();
		CInputReceiver::CollectGarbage();
		return true;
	};
	j.freq = 30;
	JobDispatcher::AddJob(j);
);

DO_ONCE_FNC(
	JobDispatcher::Job j;
	j.f = []() -> bool {
		profiler.Update();
		return true;
	};
	j.freq = 1;
	JobDispatcher::AddJob(j);
);



CGame::CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile)
	: finishedLoading(false)
	, gameOver(false)
	, gameDrawMode(gameNotDrawing)
	, thisFps(0)
	, lastSimFrame(-1)
	, frameStartTime(spring_gettime())
	, lastUpdateTime(spring_gettime())
	, lastSimFrameTime(spring_gettime())
	, lastDrawFrameTime(spring_gettime())
	, lastFrameTime(spring_gettime())
	, updateDeltaSeconds(0.0f)
	, totalGameTime(0)
	, hideInterface(false)
	, noSpectatorChat(false)
	, skipping(false)
	, playing(false)
	, chatting(false)
	, unconsumedFrames(0)
	, msgProcTimeLeft(0.0f)
	, consumeSpeed(1.0f)
	, lastframe(spring_gettime())
	, skipStartFrame(0)
	, skipEndFrame(0)
	, skipTotalFrames(0)
	, skipSeconds(0.0f)
	, skipSoundmute(false)
	, skipOldSpeed(0.0f)
	, skipOldUserSpeed(0.0f)
	, skipLastDrawTime(spring_gettime())
	, speedControl(-1)
	, defsParser(NULL)
	, saveFile(saveFile)
	, infoConsole(NULL)
	, consoleHistory(NULL)
	, worldDrawer(NULL)
{
	game = this;

	memset(gameID, 0, sizeof(gameID));

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
	GML::EnableCallChainWarnings(!!showMTInfo);
	mtInfoThreshold = configHandler->GetFloat("MTInfoThreshold");
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
	GML::Init(); // modinfo plays key part in MT enable/disable
	Threading::InitOMP(!GML::Enabled());
	Threading::SetThreadScheduler();

	if (!mapInfo) {
		mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);
	}

	int mtl = globalConfig->GetMultiThreadLua();
	showMTInfo = showMTInfo ? mtl : -1;

	if (!sideParser.Load()) {
		throw content_error(sideParser.GetErrorLog());
	}
}

CGame::~CGame()
{
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "]";
#endif

	ENTER_SYNCED_CODE();

	CEndGameBox::Destroy();
	CLoadScreen::DeleteInstance(); // make sure to halt loading, otherwise crash :)
	CColorMap::DeleteColormaps();

	IVideoCapturing::FreeInstance();

	CLuaGaia::FreeHandler();
	CLuaRules::FreeHandler();
	LuaOpenGL::Free();

	// Kill all teams that are still alive, in
	// case the game did not do so through Lua.
	// must happen after Lua (cause CGame is
	// already null'ed and it causes a Lua event,
	// which could issue Lua code that tries to access it)
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		teamHandler->Team(t)->Died(false);
	}

	CEngineOutHandler::Destroy();
	CResourceHandler::FreeInstance();

	// TODO move these to the end of this dtor, once all action-executors are registered by their respective engine sub-parts
	UnsyncedGameCommands::DestroyInstance();
	SyncedGameCommands::DestroyInstance();

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
	SafeDelete(inMapDrawerModel);
	SafeDelete(inMapDrawer);

	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(camHandler);
	SafeDelete(camera);
	SafeDelete(cam2);
	SafeDelete(icon::iconHandler);
	SafeDelete(geometricObjects);
	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	SafeDelete(featureHandler); // depends on unitHandler (via ~CFeature)
	SafeDelete(unitHandler); // depends on modelParser (via ~CUnit)
	SafeDelete(projectileHandler);

	SafeDelete(cubeMapHandler);
	SafeDelete(modelParser);

	SafeDelete(pathManager);
	SafeDelete(ground);
	SafeDelete(smoothGround);
	SafeDelete(groundBlockingObjectMap);
	SafeDelete(radarhandler);
	SafeDelete(loshandler);
	SafeDelete(mapDamage);
	SafeDelete(quadField);
	SafeDelete(moveDefHandler);
	SafeDelete(unitDefHandler);
	SafeDelete(weaponDefHandler);
	SafeDelete(damageArrayHandler);
	SafeDelete(explGenHandler);
	SafeDelete(helper);
	SafeDelete((mapInfo = const_cast<CMapInfo*>(mapInfo)));

	CClassicGroundMoveType::DeleteLineTable();
	CCategoryHandler::RemoveInstance();

	for (unsigned int i = 0; i < grouphandlers.size(); i++) {
		SafeDelete(grouphandlers[i]);
	}
	grouphandlers.clear();

	SafeDelete(saveFile); // ILoadSaveHandler, depends on vfsHandler via ~IArchive
	SafeDelete(vfsHandler);
	SafeDelete(archiveScanner);

	SafeDelete(gameServer);
	ISound::Shutdown();

	game = NULL;

	LEAVE_SYNCED_CODE();
}


void CGame::LoadGame(const std::string& mapName)
{
	GML::ThreadNumber(GML_LOAD_THREAD_NUM);
	Threading::SetThreadName("loading");

	Watchdog::RegisterThread(WDT_LOAD);
	if (!gu->globalQuit) LoadDefs();
	if (!gu->globalQuit) PreLoadSimulation(mapName);
	if (!gu->globalQuit) PreLoadRendering();
	if (!gu->globalQuit) PostLoadSimulation();
	if (!gu->globalQuit) PostLoadRendering();
	if (!gu->globalQuit) LoadInterface();
	if (!gu->globalQuit) LoadLua();
	if (!gu->globalQuit) LoadFinalize();

	if (!gu->globalQuit && saveFile) {
		loadscreen->SetLoadMessage("Loading game");
		saveFile->LoadGame();
	}

	Threading::SetThreadName("unknown");
	Watchdog::DeregisterThread(WDT_LOAD);
}


void CGame::LoadDefs()
{
	ENTER_SYNCED_CODE();

	{
		loadscreen->SetLoadMessage("Loading Radar Icons");
		icon::iconHandler = new icon::CIconHandler();
	}

	{
		ScopedOnceTimer timer("Game::LoadDefs (GameData)");
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
		ScopedOnceTimer timer("Game::LoadDefs (Sound)");
		loadscreen->SetLoadMessage("Loading Sound Definitions");

		sound->LoadSoundDefs("gamedata/sounds.lua");
		chatSound = sound->GetSoundId("IncomingChat");
	}

	LEAVE_SYNCED_CODE();
}

void CGame::PreLoadSimulation(const std::string& mapName)
{
	ENTER_SYNCED_CODE();

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
	moveDefHandler = new MoveDefHandler();
	quadField = new CQuadField();
	damageArrayHandler = new CDamageArrayHandler();
	explGenHandler = new CExplosionGeneratorHandler();
}

void CGame::PostLoadSimulation()
{
	loadscreen->SetLoadMessage("Loading Weapon Definitions");
	weaponDefHandler = new CWeaponDefHandler();
	loadscreen->SetLoadMessage("Loading Unit Definitions");
	unitDefHandler = new CUnitDefHandler();

	CClassicGroundMoveType::CreateLineTable();

	unitHandler = new CUnitHandler();
	projectileHandler = new CProjectileHandler();

	loadscreen->SetLoadMessage("Loading Feature Definitions");
	featureHandler = new CFeatureHandler();

	loshandler = new CLosHandler();
	radarhandler = new CRadarHandler(false);

	mapDamage = IMapDamage::GetMapDamage();
	pathManager = IPathManager::GetInstance(modInfo.pathFinderSystem);

	// load map-specific features after pathManager so it knows about them (via TerrainChange)
	loadscreen->SetLoadMessage("Initializing Map Features");
	featureHandler->LoadFeaturesFromMap(saveFile != NULL);

	wind.LoadWind(mapInfo->atmosphere.minWind, mapInfo->atmosphere.maxWind);

	CCobInstance::InitVars(teamHandler->ActiveTeams(), teamHandler->ActiveAllyTeams());

	geometricObjects = new CGeometricObjects();

	inMapDrawerModel = new CInMapDrawModel();
	inMapDrawer = new CInMapDraw();

	syncedGameCommands->AddDefaultActionExecutors();
	unsyncedGameCommands->AddDefaultActionExecutors();

	LEAVE_SYNCED_CODE();
}

void CGame::PreLoadRendering()
{
	//! these need to be loaded before featureHandler
	//! (maps with features have their models loaded at startup)
	modelParser = new C3DModelLoader();

	loadscreen->SetLoadMessage("Creating Unit Textures");
	texturehandler3DO = new C3DOTextureHandler();
	texturehandlerS3O = new CS3OTextureHandler();

	featureDrawer = new CFeatureDrawer();
	loadscreen->SetLoadMessage("Creating Sky");
	sky = ISky::GetSky();
}

void CGame::PostLoadRendering() {
	worldDrawer = new CWorldDrawer();
}

void CGame::SetupRenderingParams()
{
	glLightfv(GL_LIGHT1, GL_AMBIENT, mapInfo->light.unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, mapInfo->light.unitSunColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, mapInfo->light.unitAmbientColor);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	ISky::SetupFog();

	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0.0f);
}

void CGame::LoadInterface()
{
	{
		ScopedOnceTimer timer("Game::LoadInterface (Camera&Mouse)");
		camera = new CCamera();
		cam2 = new CCamera();
		mouse = new CMouseHandler();
		camHandler = new CCameraHandler();
	}

	selectedUnitsHandler.Init(playerHandler->ActivePlayers());

	// interface components
	ReColorTeams();
	cmdColors.LoadConfigFromFile("cmdcolors.txt");

	{
		ScopedOnceTimer timer("Game::LoadInterface (Console)");
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
	ENTER_SYNCED_CODE();
	loadscreen->SetLoadMessage("Loading LuaRules");
	CLuaRules::LoadHandler();

	if (gs->useLuaGaia) {
		loadscreen->SetLoadMessage("Loading LuaGaia");
		CLuaGaia::LoadHandler();
	}
	LEAVE_SYNCED_CODE();

	loadscreen->SetLoadMessage("Loading LuaUI");
	CLuaUI::LoadHandler();

	// last in, first served
	luaInputReceiver = new LuaInputReceiver();

	delete defsParser;
	defsParser = NULL;
}

void CGame::LoadFinalize()
{
	loadscreen->SetLoadMessage("Initializing PathCache");
	eventHandler.GamePreload();
	pathManager->UpdateFull(); // mapfeatures are not in written pathcaches, so we need to repath those & other stuff done by Lua

	loadscreen->SetLoadMessage("Finalizing");

	if (CBenchmark::enabled) {
		static CBenchmark benchmark;
	}

	lastframe = spring_gettime();
	lastSimFrameTime = lastframe;
	lastDrawFrameTime = lastframe;
	updateDeltaSeconds = 0.0f;

	finishedLoading = true;
}



void CGame::PostLoad()
{
	GameSetupDrawer::Disable();
	if (gameServer) {
		gameServer->PostLoad(gs->frameNum);
	}
}


void CGame::ResizeEvent()
{
	if (minimap != NULL) {
		minimap->UpdateGeometry();
	}

	// reload water renderer (it may depend on screen resolution)
	water = IWater::GetWater(water, water->GetID());

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
			keyBindings->ExecuteCommand(cmd);
			hotBinding.clear();
			LOG("%s", cmd.c_str());
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
	std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::list<CInputReceiver*>::iterator ri;
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

	// maybe a widget is interested?
	if (guihandler != NULL) {
		for (unsigned int i = 0; i < actionList.size(); ++i) {
			guihandler->PushLayoutCommand(actionList[i].rawline, false);
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
	std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::list<CInputReceiver*>::iterator ri;
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
	//TODO move as much as possible unsynced code out of this function

	good_fpu_control_registers("CGame::Update");

	JobDispatcher::Update();

	const spring_time timeNow = spring_gettime();
	const float diffsecs = spring_difftime(timeNow, lastUpdateTime).toSecsf();

	if (diffsecs >= 1.0f) { // do once every second
		lastUpdateTime = timeNow;

		// Some netcode stuff
		if (!gameServer) {
			const int consumeFrameSpeed = (unconsumedFrames < 0) ? 10000 : unconsumedFrames;
			consumeSpeed = GAME_SPEED * gs->speedFactor + consumeFrameSpeed - 2;
			unconsumedFrames = -1;
			msgProcTimeLeft = 0.0f;
		}
	}

	//TODO: why? it already gets called with `true` in ::Draw()?
	if (!skipping)
	{
		UpdateUI(false);
	}

	net->Update();

	// When video recording do step by step simulation, so each simframe gets a corresponding videoframe
	if (videoCapturing->IsCapturing() && playing && gameServer) {
		gameServer->CreateNewFrame(false, true);
	}

	ENTER_SYNCED_CODE();
	SendClientProcUsage();
	ClientReadNet(); // this can issue new SimFrame()s

	if (!gameOver) {
		if (net->NeedsReconnect()) {
			extern ClientSetup* startsetup;
			net->AttemptReconnect(startsetup->myPlayerName, startsetup->myPasswd, SpringVersion::GetFull());
		}

		if (net->CheckTimeout(0, gs->frameNum == 0)) {
			GameEnd(std::vector<unsigned char>(), true);
		}
	}
	LEAVE_SYNCED_CODE();

	//TODO move this to ::Draw()?
	if (gs->frameNum == 0 || gs->paused)
		eventHandler.UpdateObjects(); // we must add new rendering objects even if the game has not started yet

	return true;
}


bool CGame::UpdateUnsynced()
{
	// timings and frame interpolation
	const spring_time currentTime = spring_gettime();
	const spring_time diffLastCall = (currentTime - lastDrawFrameTime);
	lastDrawFrameTime = currentTime;

	const float modGameDeltaTime = skipping ? 0.01f : diffLastCall.toSecsf();
	globalRendering->lastFrameTime = diffLastCall.toSecsf();
	gu->avgFrameTime = mix(gu->avgFrameTime, diffLastCall.toMilliSecsf(), 0.05f);

	{
		// update game timings
		gu->gameTime += modGameDeltaTime;
		gu->modGameTime += (modGameDeltaTime * gs->speedFactor * (1 - gs->paused));

		if (playing && !gameOver) {
			totalGameTime += modGameDeltaTime;
		}

		updateDeltaSeconds = modGameDeltaTime;
	}

	{
		// update simFPS counter
		static int lsf = gs->frameNum;
		static spring_time lsft = currentTime;

		const float diffsecs_ = (currentTime - lsft).toSecsf();

		if (diffsecs_ >= 1.0f) {
			gu->simFPS = (gs->frameNum - lsf) / diffsecs_;
			lsft = currentTime;
			lsf = gs->frameNum;
		}
	}

	if (skipping) {
		// when fast-forwarding, maintain a draw-rate of 2Hz
		if (spring_tomsecs(currentTime - skipLastDrawTime) < 500.0f)
			return true;

		skipLastDrawTime = currentTime;

		if (!GML::SimEnabled() || !GML::MultiThreadSim()) {
			DrawSkip();
			return true;
		}
	}

	thisFps++;
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);

	// Update the interpolation coefficient (globalRendering->timeOffset)
	if (!gs->paused && !HasLag() && gs->frameNum>1 && !videoCapturing->IsCapturing()) {
		globalRendering->lastFrameStart = currentTime;
		globalRendering->weightedSpeedFactor = 0.001f * gu->simFPS;
		globalRendering->timeOffset = (currentTime - lastSimFrameTime).toMilliSecsf() * globalRendering->weightedSpeedFactor;
	} else  {
		globalRendering->timeOffset = 0;
		lastSimFrameTime = currentTime;
	}

	const float frameDeltaTime = spring_diffsecs(currentTime, frameStartTime);

	if (frameDeltaTime >= 1.0f) {
		// update FPS counter once every second
		frameStartTime = currentTime;

		globalRendering->FPS = thisFps / frameDeltaTime;
		thisFps = 0;

		if (GML::SimEnabled()) GML_RESET_LOCK_TIME(); //FIXME move to a GML update place?
	}

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const bool newSimFrame = (lastSimFrame != gs->frameNum);
	lastSimFrame = gs->frameNum;

	// set camera
	camHandler->UpdateCam();
	camera->Update();

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	if (doDrawWorld) {
		worldDrawer->Update();
		CNamedTextures::Update();
		modelParser->Update();

		if (newSimFrame) {
			projectileDrawer->UpdateTextures();
			sky->Update();
			sky->GetLight()->Update();
			water->Update();
		}
	}

	static spring_time lastUnsyncedUpdateTime = spring_gettime();
	const float unsyncedUpdateDeltaTime = spring_diffsecs(currentTime, lastUnsyncedUpdateTime);

	// always update ExtraTexture & SoundListener with <= 30Hz (even when paused)
	// garbage collection event must also run regularly because of unsynced code
	if (newSimFrame || unsyncedUpdateDeltaTime >= (1.0f / GAME_SPEED)) {
		lastUnsyncedUpdateTime = currentTime;

		{
			SCOPED_TIMER("GroundDrawer::UpdateExtraTex");
			gd->UpdateExtraTexture();
		}

		// TODO call only when camera changed
		sound->UpdateListener(camera->GetPos(), camera->forward, camera->up, unsyncedUpdateDeltaTime);
	}

	UpdateUI(true);

	SetDrawMode(gameNormalDraw); //TODO move to ::Draw()?

	if (luaUI)    { luaUI->CheckStack(); luaUI->ExecuteUIEventBatch(); }
	if (luaGaia)  { luaGaia->CheckStack(); }
	if (luaRules) { luaRules->CheckStack(); }

	// XXX ugly hack to minimize luaUI errors
	if (luaUI && luaUI->GetCallInErrors() >= 5) {
		for (int annoy = 0; annoy < 8; annoy++) {
			LOG_L(L_ERROR, "5 errors deep in LuaUI, disabling...");
		}

		guihandler->PushLayoutCommand("disable");
		LOG_L(L_ERROR, "Type '/luaui reload' in the chat to re-enable LuaUI.");
		LOG_L(L_ERROR, "===>>>  Please report this error to the forum or mantis with your infolog.txt");
	}

	assert(infoConsole);
	infoConsole->PushNewLinesToEventHandler();

	configHandler->Update();
	mouse->Update();
	mouse->UpdateCursors();
	guihandler->Update();

	LuaUnsyncedCtrl::ClearUnitCommandQueues();
	eventHandler.Update();

	return false;
}


#if defined(USE_GML) && GML_ENABLE_DRAW
bool CGame::Draw() {
	gmlProcessor->Work(&CGame::DrawMTcb,NULL,NULL,this,GML::ThreadCount(),TRUE,NULL,1,2,2,FALSE);
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

	if (UpdateUnsynced())
		return true;

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const spring_time currentTimePreDraw = spring_gettime();

	eventHandler.DrawGenesis();

	if (!globalRendering->active) {
		spring_sleep(spring_msecs(10));

		// return early if and only if less than 30K milliseconds have passed since last draw-frame
		// so we force render two frames per minute when minimized to clear batches and free memory
		// don't need to mess with globalRendering->active since only mouse-input code depends on it
		if ((currentTimePreDraw - lastDrawFrameTime).toSecs() < 30)
			return true;
	}

	CTeamHighlight::Enable(spring_tomsecs(currentTimePreDraw));

	if (unitTracker.Enabled()) {
		unitTracker.SetCam();
	}

	if (doDrawWorld) {
		if (shadowHandler->shadowsLoaded) {
			SCOPED_TIMER("ShadowHandler::CreateShadows");
			SetDrawMode(gameShadowDraw);
			shadowHandler->CreateShadows();
			SetDrawMode(gameNormalDraw);
		}

		{
			SCOPED_TIMER("CubeMapHandler::UpdateReflTex");
			cubeMapHandler->UpdateReflectionTexture();
		}

		if (sky->GetLight()->IsDynamic()) {
			{
				SCOPED_TIMER("CubeMapHandler::UpdateSpecTex");
				cubeMapHandler->UpdateSpecularTexture();
			}
			{
				SCOPED_TIMER("Sky::UpdateSkyTex");
				sky->UpdateSkyTexture();
			}
			{
				SCOPED_TIMER("ReadMap::UpdateShadingTex");
				readmap->UpdateShadingTexture();
			}
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
	camera->Update();

	if (doDrawWorld) {
		worldDrawer->Draw();
	} else {
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

	SCOPED_TIMER("Game::DrawScreen");

	if (doDrawWorld) {
		eventHandler.DrawScreenEffects();
	}

	hudDrawer->Draw((gu->GetMyPlayer())->fpsController.GetControllee());
	debugDrawerAI->Draw();

	glEnable(GL_TEXTURE_2D);

	{
		SCOPED_TIMER("CInputReceiver::DrawScreen");

		if (!hideInterface) {
			std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
			std::list<CInputReceiver*>::reverse_iterator ri;
			for (ri = inputReceivers.rbegin(); ri != inputReceivers.rend(); ++ri) {
				CInputReceiver* rcvr = *ri;
				if (rcvr) {
					rcvr->Draw();
				}
			}
		} else {
			if (globalRendering->dualScreenMode) {
				// minimap is on its own screen, so always draw it
				minimap->Draw();
			}
		}
	}

	glEnable(GL_TEXTURE_2D);

	#define DBG_FONT_FLAGS (FONT_SCALE | FONT_NORM | FONT_SHADOW)
	#define KEY_FONT_FLAGS (FONT_SCALE | FONT_CENTER | FONT_NORM)
	#define INF_FONT_FLAGS (FONT_RIGHT | FONT_SCALE | FONT_NORM | (FONT_OUTLINE * guihandler->GetOutlineFonts()))

	if (globalRendering->drawdebug) {
		//print some infos (fps,gameframe,particles)
		font->Begin();
		font->SetTextColor(1,1,0.5f,0.8f);

		font->glFormat(0.03f, 0.02f, 1.0f, DBG_FONT_FLAGS, "FPS: %0.1f SimFPS: %0.1f SimFrame: %d Speed: %2.2f (%2.2f) Particles: %d (%d)",
		    globalRendering->FPS, gu->simFPS, gs->frameNum, gs->speedFactor, gs->wantedSpeedFactor, projectileHandler->syncedProjectiles.size() + projectileHandler->unsyncedProjectiles.size(), projectileHandler->currentParticles);

		// 16ms := 60fps := 30simFPS + 30drawFPS
		font->glFormat(0.03f, 0.07f, 0.7f, DBG_FONT_FLAGS, "avgFrame: %s%2.1fms\b avgDrawFrame: %s%2.1fms\b avgSimFrame: %s%2.1fms\b",
		   (gu->avgFrameTime     > 30) ? "\xff\xff\x01\x01" : "", gu->avgFrameTime,
		   (gu->avgDrawFrameTime > 16) ? "\xff\xff\x01\x01" : "", gu->avgDrawFrameTime,
		   (gu->avgSimFrameTime  > 16) ? "\xff\xff\x01\x01" : "", gu->avgSimFrameTime
		);

		const int2 pfsUpdates = pathManager->GetNumQueuedUpdates();
		const char* fmtString = "[%s-PFS] queued updates: %i %i";

		switch (pathManager->GetPathFinderType()) {
			case PFS_TYPE_DEFAULT: {
				font->glFormat(0.03f, 0.12f, 0.7f, DBG_FONT_FLAGS, fmtString, "DEFAULT", pfsUpdates.x, pfsUpdates.y);
			} break;
			case PFS_TYPE_QTPFS: {
				font->glFormat(0.03f, 0.12f, 0.7f, DBG_FONT_FLAGS, fmtString, "QT", pfsUpdates.x, pfsUpdates.y);
			} break;
		}

		SLuaInfo luaInfo;
		spring_lua_alloc_get_stats(&luaInfo);
		font->glFormat(0.03f, 0.15f, 0.7f, DBG_FONT_FLAGS, "Lua allocated memory: %.1fMB in %i LuaStates", luaInfo.allocedBytes/1024.f/1024.f, luaInfo.numStates);

		font->End();
	}

	if (userWriting) {
		DrawInputText();
	}

	if (!hotBinding.empty()) {
		glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
		font->glPrint(0.5f, 0.6f, 3.0f, KEY_FONT_FLAGS, "Hit keyset for:");
		glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
		font->glFormat(0.5f, 0.5f, 3.0f, KEY_FONT_FLAGS, "%s", hotBinding.c_str());
		glColor4f(0.3f, 0.3f, 1.0f, 1.0f);
		font->glPrint(0.5f, 0.4f, 3.0f, KEY_FONT_FLAGS, "(or Escape)");
	}

	if (!hideInterface) {
		smallFont->Begin();

		if (showClock) {
			char buf[32];
			const int seconds = (gs->frameNum / GAME_SPEED);
			if (seconds < 3600) {
				SNPRINTF(buf, sizeof(buf), "%02i:%02i", seconds / 60, seconds % 60);
			} else {
				SNPRINTF(buf, sizeof(buf), "%02i:%02i:%02i", seconds / 3600, (seconds / 60) % 60, seconds % 60);
			}

			smallFont->glPrint(0.99f, 0.94f, 1.0f, INF_FONT_FLAGS, buf);
		}

		if (showFPS) {
			char buf[32];
			SNPRINTF(buf, sizeof(buf), "%.0f", globalRendering->FPS);

			const float4 yellow(1.0f, 1.0f, 0.25f, 1.0f);
			smallFont->SetColors(&yellow,NULL);
			smallFont->glPrint(0.99f, 0.92f, 1.0f, INF_FONT_FLAGS, buf);
		}

		if (showSpeed) {
			char buf[32];
			SNPRINTF(buf, sizeof(buf), "%2.2f", gs->speedFactor);

			const float4 speedcol(1.0f, gs->speedFactor < gs->wantedSpeedFactor * 0.99f ? 0.25f : 1.0f, 0.25f, 1.0f);
			smallFont->SetColors(&speedcol, NULL);
			smallFont->glPrint(0.99f, 0.90f, 1.0f, INF_FONT_FLAGS, buf);
		}

		if (GML::SimEnabled() && showMTInfo != -1) {
			const char* pstr = "LUA-EXP-SIZE(MT): %2.1fK LUA-SYNC-CPU(MT): %2.1fms";
			char buf[80];
			SNPRINTF(buf, sizeof(buf), pstr, LuaUtils::exportedDataSize / 1000.0f, GML_LOCK_TIME());
			const float warnMix = std::max(LuaUtils::exportedDataSize / 5000.0f, (GML_LOCK_TIME() - 1.0f) / 50.0f);
			const float4 warncol(float(spring_tomsecs(currentTimePreDraw) & 128) * warnMix, 1.0f - warnMix, 0.0f,1.0f);
			smallFont->SetColors(&warncol, NULL);
			smallFont->glPrint(0.99f, 0.88f, 1.0f, INF_FONT_FLAGS, buf);
		}

		CPlayerRosterDrawer::Draw();

		smallFont->End();
	}

	if (GML::SimEnabled() && skipping)
		DrawSkip(false);

	mouse->DrawCursor();

	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	videoCapturing->RenderFrame();

	SetDrawMode(gameNotDrawing);
	CTeamHighlight::Disable();

	const spring_time currentTimePostDraw = spring_gettime();
	gu->avgDrawFrameTime = mix(gu->avgDrawFrameTime, (currentTimePostDraw - currentTimePreDraw).toMilliSecsf(), 0.05f);

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

	char c = (writingPos >= userInput.size()) ? '\0' : userInput[writingPos];
	if (c == 0) { c = ' '; }

	const float cw = fontSize * font->GetCharacterWidth(c) * globalRendering->pixelX;
	const float csx = inputTextPosX + caretWidth;
	glDisable(GL_TEXTURE_2D);
	const float f = 0.5f * (1.0f + fastmath::sin(spring_now().toMilliSecsf() * 0.015f));
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
	lastframe = spring_gettime();

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
	}

	eventHandler.GameStart();

	// This is a hack!!!
	// Before 0.83 Lua had its GameFrame callin before gs->frameNum got updated,
	// what caused it to have a `gameframe0` while the engine started with 1.
	// This became a problem when LUS was added, and so we were forced to switch
	// Lua to the 1-indexed system, too.
	// To keep backward compability Lua now gets 2 GameFrames at start (0 & 1)
	// and both share the same SimFrame!
	eventHandler.GameFrame(0);

	GML::PrintStartupMessage(showMTInfo);
}



void CGame::SimFrame() {
	ENTER_SYNCED_CODE();

	good_fpu_control_registers("CGame::SimFrame");
	lastFrameTime = spring_gettime();

	gs->frameNum++;

#ifdef TRACE_SYNC
	tracefile << "New frame:" << gs->frameNum << " " << gs->GetRandSeed() << "\n";
#endif

	if (!skipping) {
		// everything here is unsynced and should ideally moved to Game::Update()
		infoConsole->Update();
		waitCommandsAI.Update();
		geometricObjects->Update();
		sound->NewFrame();
		eoh->Update();
		for (size_t a = 0; a < grouphandlers.size(); a++) {
			grouphandlers[a]->Update();
		}

		(playerHandler->Player(gu->myPlayerNum)->fpsController).SendStateUpdate(camMove);

		CTeamHighlight::Update(gs->frameNum);
	}

	// everything from here is simulation
	{
		SCOPED_TIMER("EventHandler::GameFrame");
		eventHandler.GameFrame(gs->frameNum);
	}
	SCOPED_TIMER("SimFrame");
	helper->Update();
	mapDamage->Update();
	pathManager->Update();
	unitHandler->Update();
	projectileHandler->Update();
	featureHandler->Update();
	GCobEngine.Tick(33);
	GUnitScriptEngine.Tick(33);
	wind.Update();
	loshandler->Update();
	interceptHandler.Update(false);

	teamHandler->GameFrame(gs->frameNum);
	playerHandler->GameFrame(gs->frameNum);

	lastSimFrameTime = spring_gettime();
	gu->avgSimFrameTime = mix(gu->avgSimFrameTime, (lastSimFrameTime - lastFrameTime).toMilliSecsf(), 0.05f);
	gu->avgSimFrameTime = std::max(gu->avgSimFrameTime, 0.001f);

	#ifdef HEADLESS
	{
		const float msecMaxSimFrameTime = 1000.0f / (GAME_SPEED * gs->wantedSpeedFactor);
		const float msecDifSimFrameTime = (lastSimFrameTime - lastFrameTime).toMilliSecsf();
		// multiply by 0.5 to give unsynced code some execution time (50% of our sleep-budget)
		const float msecSleepTime = (msecMaxSimFrameTime - msecDifSimFrameTime) * 0.5f;

		if (msecSleepTime > 0.0f) {
			spring_sleep(spring_msecs(msecSleepTime));
		}
	}
	#endif

	// usefull for desync-debugging enter (enter instead of -1 start & end frame of the range you want to debug)
	DumpState(-1, -1, 1);

	LEAVE_SYNCED_CODE();
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


void CGame::GameEnd(const std::vector<unsigned char>& winningAllyTeams, bool timeout)
{
	if (gameOver)
		return;

	gameOver = true;
	eventHandler.GameOver(winningAllyTeams);

	CEndGameBox::Create(winningAllyTeams);
#ifdef    HEADLESS
	profiler.PrintProfilingInfo();
#endif // HEADLESS

	CDemoRecorder* record = net->GetDemoRecorder();

	if (record != NULL) {
		// Write CPlayer::Statistics and CTeam::Statistics to demo
		// TODO: move this to a method in CTeamHandler
		const int numPlayers = playerHandler->ActivePlayers();
		const int numTeams = teamHandler->ActiveTeams() - int(gs->useLuaGaia);

		record->SetTime(gs->frameNum / GAME_SPEED, (int)gu->gameTime);
		record->InitializeStats(numPlayers, numTeams);
		// pass the list of winners
		record->SetWinningAllyTeams(winningAllyTeams);

		// tell everybody about our APM, it's the most important statistic
		net->Send(CBaseNetProtocol::Get().SendPlayerStat(gu->myPlayerNum, playerHandler->Player(gu->myPlayerNum)->currentStats));

		for (int i = 0; i < numPlayers; ++i) {
			record->SetPlayerStats(i, playerHandler->Player(i)->currentStats);
		}
		for (int i = 0; i < numTeams; ++i) {
			const CTeam* team = teamHandler->Team(i);
			record->SetTeamStats(i, team->statHistory);
			netcode::PackPacket* buf = new netcode::PackPacket(2 + sizeof(CTeam::Statistics), NETMSG_TEAMSTAT);
			*buf << static_cast<uint8_t>(team->teamNum);
			*buf << team->currentStats;
			net->Send(buf);
		}
	}

	if (!timeout) {
		// pass the winner info to the host in the case it's a dedicated server
		net->Send(CBaseNetProtocol::Get().SendGameOver(gu->myPlayerNum, winningAllyTeams));
	} else {
		// client timed out, don't send anything (in theory the GAMEOVER
		// message not be able to reach the server if connection is lost,
		// but in practice it can get through --> timeout check needs work)
		LOG_L(L_ERROR, "[%s] lost connection to gameserver", __FUNCTION__);
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

	const std::string& s = msg.msg;

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
				LOG("%sAllies: %s", label.c_str(), s.c_str());
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_SPECTATORS) {
			if (gu->spectating || myMsg) {
				LOG("%sSpectators: %s", label.c_str(), s.c_str());
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_EVERYONE) {
			const bool specsOnly = noSpectatorChat && (player && player->spectator);
			if (gu->spectating || !specsOnly) {
				LOG("%s%s", label.c_str(), s.c_str());
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
		}
		else if ((msg.destination < playerHandler->ActivePlayers()) && player)
		{	// player -> spectators and spectator -> player PMs should be forbidden
			// player <-> player and spectator <-> spectator are allowed
			if (msg.destination == gu->myPlayerNum && player->spectator == gu->spectating) {
				LOG("%sPrivate: %s", label.c_str(), s.c_str());
				Channels::UserInterface.PlaySample(chatSound, 5);
			}
			else if (player->playerNum == gu->myPlayerNum)
			{
				LOG("You whispered %s: %s", playerHandler->Player(msg.destination)->name.c_str(), s.c_str());
			}
		}
	}

	eoh->SendChatMessage(msg.msg.c_str(), msg.fromPlayer);
}



void CGame::StartSkip(int toFrame) {
	return; // FIXME: desyncs

	if (skipping) {
		LOG_L(L_ERROR, "skipping appears to be busted (%i)", skipping);
	}

	skipStartFrame = gs->frameNum;
	skipEndFrame = toFrame;

	if (skipEndFrame <= skipStartFrame) {
		LOG_L(L_WARNING, "Already passed %i (%i)", skipEndFrame / GAME_SPEED, skipEndFrame);
		return;
	}

	skipTotalFrames = skipEndFrame - skipStartFrame;
	skipSeconds = skipTotalFrames / float(GAME_SPEED);

	skipSoundmute = sound->IsMuted();
	if (!skipSoundmute)
		sound->Mute(); // no sounds

	//FIXME not smart to change SYNCED values in demo playbacks etc.
	skipOldSpeed     = gs->speedFactor;
	skipOldUserSpeed = gs->wantedSpeedFactor;
	const float speed = 1.0f;
	gs->speedFactor     = speed;
	gs->wantedSpeedFactor = speed;

	skipLastDrawTime = spring_gettime();

	skipping = true;
}

void CGame::EndSkip() {
	return; // FIXME
/*
	skipping = false;

	gu->gameTime    += skipSeconds;
	gu->modGameTime += skipSeconds;

	gs->speedFactor     = skipOldSpeed;
	gs->wantedSpeedFactor = skipOldUserSpeed;

	if (!skipSoundmute) {
		sound->Mute(); // sounds back on
	}

	LOG("Skipped %.1f seconds", skipSeconds);
*/
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
		LOG_L(L_WARNING, "reloadcob can only be used if cheating is enabled");
		return;
	}
	const string unitName = msg;
	if (unitName.empty()) {
		LOG_L(L_WARNING, "Missing unit name");
		return;
	}
	const UnitDef* udef = unitDefHandler->GetUnitDefByName(unitName);
	if (udef==NULL) {
		LOG_L(L_WARNING, "Unknown unit name: \"%s\"", unitName.c_str());
		return;
	}
	const CCobFile* oldScript = GCobFileHandler.GetScriptAddr("scripts/" + udef->scriptName);
	if (oldScript == NULL) {
		LOG_L(L_WARNING, "Unknown COB script for unit \"%s\": %s", unitName.c_str(), udef->scriptName.c_str());
		return;
	}
	CCobFile* newScript = GCobFileHandler.ReloadCobFile("scripts/" + udef->scriptName);
	if (newScript == NULL) {
		LOG_L(L_WARNING, "Could not load COB script for unit \"%s\" from: %s", unitName.c_str(), udef->scriptName.c_str());
		return;
	}
	int count = 0;
	for (size_t i = 0; i < unitHandler->MaxUnits(); i++) {
		CUnit* unit = unitHandler->units[i];
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
	LOG("Reloaded cob script for %i units", count);
}



void CGame::SelectUnits(const string& line)
{
	const vector<string> &args = CSimpleParser::Tokenize(line, 0);
	for (int i = 0; i < (int)args.size(); i++) {
		const string& arg = args[i];
		if (arg == "clear") {
			selectedUnitsHandler.ClearSelected();
		}
		else if ((arg[0] == '+') || (arg[0] == '-')) {
			char* endPtr;
			const char* startPtr = arg.c_str() + 1;
			const int unitIndex = strtol(startPtr, &endPtr, 10);
			if (endPtr == startPtr) {
				continue; // bad number
			}
			if ((unitIndex < 0) || (static_cast<unsigned int>(unitIndex) >= unitHandler->MaxUnits())) {
				continue; // bad index
			}
			CUnit* unit = unitHandler->units[unitIndex];
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
				selectedUnitsHandler.AddUnit(unit);
			} else {
				selectedUnitsHandler.RemoveUnit(unit);
			}
		}
	}
}


void CGame::SelectCycle(const string& command)
{
	static set<int> unitIDs;
	static int lastID = -1;

	GML_RECMUTEX_LOCK(sel); // SelectCycle

	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;

	if (command == "restore") {
		selectedUnitsHandler.ClearSelected();
		set<int>::const_iterator it;
		for (it = unitIDs.begin(); it != unitIDs.end(); ++it) {
			CUnit* unit = unitHandler->units[*it];
			if (unit != NULL) {
				selectedUnitsHandler.AddUnit(unit);
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
		selectedUnitsHandler.ClearSelected();
		lastID = *unitIDs.begin();
		selectedUnitsHandler.AddUnit(unitHandler->units[lastID]);
		return;
	}

	// clean the list
	set<int> tmpSet;
	set<int>::const_iterator it;
	for (it = unitIDs.begin(); it != unitIDs.end(); ++it) {
		if (unitHandler->units[*it] != NULL) {
			tmpSet.insert(*it);
		}
	}
	unitIDs = tmpSet;
	if ((lastID >= 0) && (unitHandler->units[lastID] == NULL)) {
		lastID = -1;
	}

	// selectedUnits size is 0 or 1
	selectedUnitsHandler.ClearSelected();
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
		selectedUnitsHandler.AddUnit(unitHandler->units[lastID]);
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
	const spring_time timeNow = spring_gettime();
	const float diffTime = spring_tomsecs(timeNow - lastFrameTime);

	if (!gs->paused && (diffTime > (500.0f / gs->speedFactor))) {
		return true;
	} else {
		return false;
	}
}


void CGame::SaveGame(const std::string& filename, bool overwrite)
{
	if (FileSystem::CreateDirectory("Saves")) {
		if (overwrite || !FileSystem::FileExists(filename)) {
			LOG("Saving game to %s", filename.c_str());
			ILoadSaveHandler* ls = ILoadSaveHandler::Create();
			ls->mapName = gameSetup->mapName;
			ls->modName = gameSetup->modName;
			ls->SaveGame(filename);
			delete ls;
		}
		else {
			LOG_L(L_WARNING, "File %s already exists(use /save -y to override)", filename.c_str());
		}
	}
}


void CGame::ReloadGame()
{
	if (saveFile) {
		// This reloads heightmap, triggers Load call-in, etc.
		// Inside the Load call-in, Lua can ensure old units are wiped before new ones are placed.
		saveFile->LoadGame();
	} else {
		LOG_L(L_WARNING, "We can only reload the game when it has been started from a savegame");
	}
}


bool CGame::ProcessAction(const Action& action, unsigned int key, bool isRepeat)
{
	if (ActionPressed(key, action, isRepeat)) {
		return true;
	}

	// maybe a widget is interested?
	if (guihandler != NULL) {
		guihandler->PushLayoutCommand(action.rawline, false); //FIXME add return argument!
	}

	return false;
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
		if (executor->ExecuteAction(unsyncedAction)) {
			return true;
		}
	}

	static std::set<std::string> serverCommands = std::set<std::string>(commands, commands+numCommands);
	if (serverCommands.find(action.command) != serverCommands.end()) {
		CommandMessage pckt(action, gu->myPlayerNum);
		net->Send(pckt.Pack());
		return true;
	}

	if (Console::Instance().ExecuteAction(action)) {
		return true;
	}

	return false;
}
