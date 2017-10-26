/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include <SDL_keyboard.h>

#include "Game.h"
#include "Benchmark.h"
#include "Camera.h"
#include "CameraHandler.h"
#include "ChatMessage.h"
#include "CommandMessage.h"
#include "ConsoleHistory.h"
#include "GameHelper.h"
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
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/PlayerRoster.h"
#include "Game/UI/PlayerRosterDrawer.h"
#include "Game/UI/UnitTracker.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Fonts/CFontTexture.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaHandle.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUI.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/BuildingMaskMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveTypeFactory.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/UnitScriptFactory.h"
#include "Sim/Units/Scripts/UnitScriptEngine.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponLoader.h"
#include "UI/CommandColors.h"
#include "UI/EndGameBox.h"
#include "UI/GameSetupDrawer.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/KeyBindings.h"
#include "UI/KeyCodes.h"
#include "UI/MiniMap.h"
#include "UI/MouseHandler.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/TooltipConsole.h"
#include "UI/ProfileDrawer.h"
#include "UI/Groups/GroupHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/myMath.h"
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/SafeUtil.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Log/ILog.h"
#include "System/Platform/Watchdog.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/DumpState.h"
#include "System/TimeProfiler.h"


#undef CreateDirectory

CONFIG(bool, GameEndOnConnectionLoss).defaultValue(true);
CONFIG(bool, WindowedEdgeMove).defaultValue(true).description("Sets whether moving the mouse cursor to the screen edge will move the camera across the map.");
CONFIG(bool, FullscreenEdgeMove).defaultValue(true).description("see WindowedEdgeMove, just for fullscreen mode");
CONFIG(bool, ShowFPS).defaultValue(false).description("Displays current framerate.");
CONFIG(bool, ShowClock).defaultValue(true).headlessValue(false).description("Displays a clock on the top-right corner of the screen showing the elapsed time of the current game.");
CONFIG(bool, ShowSpeed).defaultValue(false).description("Displays current game speed.");
CONFIG(int, ShowPlayerInfo).defaultValue(1).headlessValue(0);
CONFIG(float, GuiOpacity).defaultValue(0.8f).minimumValue(0.0f).maximumValue(1.0f).description("Sets the opacity of the built-in Spring UI. Generally has no effect on LuaUI widgets. Can be set in-game using shift+, to decrease and shift+. to increase.");
CONFIG(std::string, InputTextGeo).defaultValue("");


CGame* game = nullptr;


CR_BIND(CGame, (std::string(""), std::string(""), nullptr))

CR_REG_METADATA(CGame, (
	CR_IGNORED(finishedLoading),
	CR_IGNORED(numDrawFrames),
	CR_MEMBER(lastSimFrame),
	CR_IGNORED(lastNumQueuedSimFrames),

	CR_IGNORED(frameStartTime),
	CR_IGNORED(lastSimFrameTime),
	CR_IGNORED(lastDrawFrameTime),
	CR_IGNORED(lastFrameTime),
	CR_IGNORED(lastReadNetTime),
	CR_IGNORED(lastNetPacketProcessTime),
	CR_IGNORED(lastReceivedNetPacketTime),
	CR_IGNORED(lastSimFrameNetPacketTime),
	CR_IGNORED(lastUnsyncedUpdateTime),
	CR_IGNORED(skipLastDrawTime),

	CR_IGNORED(updateDeltaSeconds),
	CR_MEMBER(totalGameTime),
	CR_MEMBER(userInputPrefix),
	CR_IGNORED(chatSound),
	CR_MEMBER(hideInterface),
	CR_MEMBER(gameOver),
	CR_IGNORED(gameDrawMode),
	CR_IGNORED(windowedEdgeMove),
	CR_IGNORED(fullscreenEdgeMove),
	CR_MEMBER(showFPS),
	CR_MEMBER(showClock),
	CR_MEMBER(showSpeed),
	CR_IGNORED(chatting),
	CR_IGNORED(curKeyChain),
	CR_IGNORED(playerTraffic),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(gameID),
	//CR_MEMBER(consoleHistory),
	CR_IGNORED(inputTextPosX),
	CR_IGNORED(inputTextPosY),
	CR_IGNORED(inputTextSizeX),
	CR_IGNORED(inputTextSizeY),
	CR_IGNORED(skipping),
	CR_MEMBER(playing),
	CR_IGNORED(msgProcTimeLeft),
	CR_IGNORED(consumeSpeedMult),

	CR_IGNORED(skipStartFrame),
	CR_IGNORED(skipEndFrame),
	CR_IGNORED(skipTotalFrames),
	CR_IGNORED(skipSeconds),
	CR_IGNORED(skipSoundmute),
	CR_IGNORED(skipOldSpeed),
	CR_IGNORED(skipOldUserSpeed),

	CR_MEMBER(speedControl),

	CR_IGNORED(consoleHistory),
	CR_IGNORED(jobDispatcher),
	CR_IGNORED(worldDrawer),
	CR_IGNORED(defsParser),
	CR_IGNORED(saveFile),

	// from CGameController
	CR_IGNORED(writingPos),
	CR_IGNORED(ignoreNextChar),
	CR_IGNORED(userInput),
	CR_IGNORED(userPrompt),
	CR_IGNORED(userWriting),

	// Post Load
	CR_POSTLOAD(PostLoad)
))



CGame::CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile)
	: gameDrawMode(gameNotDrawing)
	, lastSimFrame(-1)
	, lastNumQueuedSimFrames(-1)
	, numDrawFrames(0)

	, frameStartTime(spring_gettime())
	, lastSimFrameTime(spring_gettime())
	, lastDrawFrameTime(spring_gettime())
	, lastFrameTime(spring_gettime())
	, lastReadNetTime(spring_gettime())
	, lastNetPacketProcessTime(spring_gettime())
	, lastReceivedNetPacketTime(spring_gettime())
	, lastSimFrameNetPacketTime(spring_gettime())
	, lastUnsyncedUpdateTime(spring_gettime())
	, skipLastDrawTime(spring_gettime())

	, updateDeltaSeconds(0.0f)
	, totalGameTime(0)
	, hideInterface(false)
	, skipping(false)
	, playing(false)
	, chatting(false)
	, noSpectatorChat(false)
	, msgProcTimeLeft(0.0f)
	, consumeSpeedMult(1.0f)
	, skipStartFrame(0)
	, skipEndFrame(0)
	, skipTotalFrames(0)
	, skipSeconds(0.0f)
	, skipSoundmute(false)
	, skipOldSpeed(0.0f)
	, skipOldUserSpeed(0.0f)
	, speedControl(-1)
	, consoleHistory(nullptr)
	, worldDrawer(nullptr)
	, defsParser(nullptr)
	, saveFile(saveFile)
	, finishedLoading(false)
	, gameOver(false)
{
	game = this;

	memset(gameID, 0, sizeof(gameID));

	// set "Headless" in config overlay (not persisted)
	configHandler->Set("Headless", (SpringVersion::IsHeadless()) ? 1 : 0, true);

	//FIXME move to MouseHandler!
	windowedEdgeMove   = configHandler->GetBool("WindowedEdgeMove");
	fullscreenEdgeMove = configHandler->GetBool("FullscreenEdgeMove");

	showFPS   = configHandler->GetBool("ShowFPS");
	showClock = configHandler->GetBool("ShowClock");
	showSpeed = configHandler->GetBool("ShowSpeed");

	speedControl = configHandler->GetInt("SpeedControl");

	playerRoster.SetSortTypeByCode((PlayerRoster::SortType)configHandler->GetInt("ShowPlayerInfo"));

	CInputReceiver::guiAlpha = configHandler->GetFloat("GuiOpacity");

	ParseInputTextGeometry("default");
	ParseInputTextGeometry(configHandler->GetString("InputTextGeo"));

	// clear left-over receivers in case we reloaded
	commandConsole.ResetState();

	wind.ResetState();

	modInfo.Init(modName.c_str());

	// needed for LuaIntro (pushes LuaConstGame)
	assert(mapInfo == nullptr);
	mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);

	if (!sideParser.Load())
		throw content_error(sideParser.GetErrorLog());


	// after this, other components are able to register chat action-executors
	SyncedGameCommands::CreateInstance();
	UnsyncedGameCommands::CreateInstance();

	// note: makes no sense to create this unless we have AI's
	// (events will just go into the void otherwise) but it is
	// unconditionally deref'ed in too many places
	CEngineOutHandler::Create();

	CResourceHandler::CreateInstance();
	CCategoryHandler::CreateInstance();
	CWordCompletion::CreateInstance();
}

CGame::~CGame()
{
#ifdef TRACE_SYNC
	tracefile << "[" << __func__ << "]";
#endif

	ENTER_SYNCED_CODE();
	LOG("[Game::%s][1]", __func__);

	KillLua(true);
	KillMisc();
	KillRendering();
	KillInterface();
	KillSimulation();

	LOG("[Game::%s][2]", __func__);
	spring::SafeDelete(saveFile); // ILoadSaveHandler, depends on vfsHandler via ~IArchive

	LOG("[Game::%s][3]", __func__);
	CWordCompletion::DestroyInstance();
	CCategoryHandler::RemoveInstance();
	CResourceHandler::FreeInstance();

	LEAVE_SYNCED_CODE();
}


void CGame::AddTimedJobs()
{
	{
		JobDispatcher::Job j;

		j.f = []() -> bool {
			SCOPED_TIMER("Misc::CollectGarbage");
			eventHandler.CollectGarbage();
			CInputReceiver::CollectGarbage();
			return true;
		};

		j.freq = GAME_SPEED;
		j.time = (1000.0f / j.freq) * (1 - j.startDirect);
		j.name = "EventHandler::CollectGarbage";

		jobDispatcher.AddTimedJob(j);
	}

	{
		JobDispatcher::Job j;

		j.f = []() -> bool {
			profiler.Update();
			return true;
		};

		j.freq = 1.0f;
		j.time = (1000.0f / j.freq) * (1 - j.startDirect);
		j.name = "Profiler::Update";

		jobDispatcher.AddTimedJob(j);
	}
}

void CGame::LoadGame(const std::string& mapName)
{
	// NOTE:
	//   this is needed for LuaHandle::CallOut*UpdateCallIn
	//   the main-thread is NOT the same as the load-thread
	//   when LoadingMT=1 (!!!)
	Threading::SetGameLoadThread();
	Watchdog::RegisterThread(WDT_LOAD);

	auto& globalQuit = gu->globalQuit;
	bool  forcedQuit = false;

	try {
		LOG("[Game::%s][1] globalQuit=%d threaded=%d", __func__, globalQuit, !Threading::IsMainThread());

		LoadMap(mapName);
		LoadDefs();
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][1] forced quit with exception \"%s\"", __func__, e.what());

		// we can not (yet) do a clean early exit here because the dtor assumes
		// all loading stages proceeded normally; just force automatic shutdown
		forcedQuit = true;
	}

	try {
		LOG("[Game::%s][2] globalQuit=%d forcedQuit=%d", __func__, globalQuit, forcedQuit);

		PreLoadSimulation();
		PreLoadRendering();
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][2] forced quit with exception \"%s\"", __func__, e.what());
		forcedQuit = true;
	}

	try {
		LOG("[Game::%s][3] globalQuit=%d forcedQuit=%d", __func__, globalQuit, forcedQuit);

		PostLoadSimulation();
		PostLoadRendering();
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][3] forced quit with exception \"%s\"", __func__, e.what());
		forcedQuit = true;
	}

	try {
		LOG("[Game::%s][4] globalQuit=%d forcedQuit=%d", __func__, globalQuit, forcedQuit);

		LoadInterface();
		LoadLua();
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][4] forced quit with exception \"%s\"", __func__, e.what());
		forcedQuit = true;
	}

	try {
		LOG("[Game::%s][5] globalQuit=%d forcedQuit=%d", __func__, globalQuit, forcedQuit);

		LoadFinalize();
		LoadSkirmishAIs();
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][5] forced quit with exception \"%s\"", __func__, e.what());
		forcedQuit = true;
	}

	try {
		LOG("[Game::%s][6] globalQuit=%d forcedQuit=%d", __func__, globalQuit, forcedQuit);

		if (!globalQuit && saveFile != nullptr) {
			loadscreen->SetLoadMessage("Loading Saved Game");
			saveFile->LoadGame();
		}
	} catch (const content_error& e) {
		LOG_L(L_WARNING, "[Game::%s][6] forced quit with exception \"%s\"", __func__, e.what());
		forcedQuit = true;
	}

	Watchdog::DeregisterThread(WDT_LOAD);
	AddTimedJobs();

	finishedLoading = true;
	globalQuit |= forcedQuit;
}


void CGame::LoadMap(const std::string& mapName)
{
	ENTER_SYNCED_CODE();

	{
		loadscreen->SetLoadMessage("Parsing Map Information");

		waterRendering->Init();
		mapRendering->Init();

		// simulation components
		helper->Init();
		readMap = CReadMap::LoadMap(mapName);
		groundBlockingObjectMap = new CGroundBlockingObjectMap(mapDims.mapSquares);
		buildingMaskMap = new BuildingMaskMap();
	}

	LEAVE_SYNCED_CODE();
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

		defsParser = new LuaParser("gamedata/defs.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP, {true});
		// customize the defs environment
		defsParser->GetTable("Spring");
		defsParser->AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
		defsParser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
		defsParser->EndTable();

		// run the parser
		if (!defsParser->Execute())
			throw content_error("Defs-Parser: " + defsParser->GetErrorLog());

		const LuaTable& root = defsParser->GetRoot();

		if (!root.IsValid())
			throw content_error("Error loading gamedata definitions");

		// bail now if any of these tables are invalid
		// makes searching for errors that much easier
		if (!root.SubTable("UnitDefs").IsValid())
			throw content_error("Error loading UnitDefs");

		if (!root.SubTable("FeatureDefs").IsValid())
			throw content_error("Error loading FeatureDefs");

		if (!root.SubTable("WeaponDefs").IsValid())
			throw content_error("Error loading WeaponDefs");

		if (!root.SubTable("ArmorDefs").IsValid())
			throw content_error("Error loading ArmorDefs");

		if (!root.SubTable("MoveDefs").IsValid())
			throw content_error("Error loading MoveDefs");

	}

	{
		ScopedOnceTimer timer("Game::LoadDefs (Sound)");
		loadscreen->SetLoadMessage("Loading Sound Definitions");

		sound->LoadSoundDefs("gamedata/sounds.lua", SPRING_VFS_MOD_BASE);
		chatSound = sound->GetSoundId("IncomingChat");
	}

	LEAVE_SYNCED_CODE();
}

void CGame::PreLoadSimulation()
{
	ENTER_SYNCED_CODE();

	loadscreen->SetLoadMessage("Creating Smooth Height Mesh");
	smoothGround = new SmoothHeightMesh(float3::maxxpos, float3::maxzpos, SQUARE_SIZE * 2, SQUARE_SIZE * 40);

	loadscreen->SetLoadMessage("Creating QuadField & CEGs");
	moveDefHandler = new MoveDefHandler(defsParser);
	quadField = new CQuadField(int2(mapDims.mapx, mapDims.mapy), CQuadField::BASE_QUAD_SIZE);
	damageArrayHandler = new CDamageArrayHandler(defsParser);
	explGenHandler = new CExplosionGeneratorHandler();
}

void CGame::PostLoadSimulation()
{
	{
		ScopedOnceTimer timer("Game::PostLoadSim (WeaponDefs)");
		loadscreen->SetLoadMessage("Loading Weapon Definitions");
		weaponDefHandler = new CWeaponDefHandler(defsParser);
	}
	{
		ScopedOnceTimer timer("Game::PostLoadSim (UnitDefs)");
		loadscreen->SetLoadMessage("Loading Unit Definitions");
		unitDefHandler = new CUnitDefHandler(defsParser);
	}
	{
		ScopedOnceTimer timer("Game::PostLoadSim (FeatureDefs)");
		loadscreen->SetLoadMessage("Loading Feature Definitions");
		featureDefHandler = new CFeatureDefHandler(defsParser);
	}

	CUnit::InitStatic();
	CCommandAI::InitCommandDescriptionCache();
	CUnitScriptFactory::InitStatic();
	CUnitScriptEngine::InitStatic();
	MoveTypeFactory::InitStatic();
	CWeaponLoader::InitStatic();

	unitHandler = new CUnitHandler();
	featureHandler = new CFeatureHandler();
	projectileHandler = new CProjectileHandler();
	losHandler = new CLosHandler();

	readMap->InitHeightMapDigestVectors(losHandler->los.size);

	// pre-load the PFS, gets finalized after Lua
	//
	// features loaded from the map (and any terrain changes
	// made by Lua while loading) would otherwise generate a
	// queue of pending PFS updates, which should be consumed
	// to avoid blocking regular updates from being processed
	// but doing so was impossible without stalling the loading
	// thread for *minutes* (in the worst-case scenario)
	//
	// the only disadvantage is that LuaPathFinder can not be
	// used during Lua initialization anymore (not a concern)
	//
	// NOTE:
	//   the cache written to disk will reflect changes made by
	//   Lua which can vary each run with {mod,map}options, etc
	//   --> need a way to let Lua flush it or re-calculate map
	//   checksum (over heightmap + blockmap, not raw archive)
	mapDamage = IMapDamage::GetMapDamage();
	pathManager = IPathManager::GetInstance(modInfo.pathFinderSystem);

	// load map-specific features
	loadscreen->SetLoadMessage("Initializing Map Features");
	featureDefHandler->LoadFeatureDefsFromMap();
	if (saveFile == nullptr)
		featureHandler->LoadFeaturesFromMap();

	wind.LoadWind(mapInfo->atmosphere.minWind, mapInfo->atmosphere.maxWind);


	inMapDrawerModel = new CInMapDrawModel();
	inMapDrawer = new CInMapDraw();

	syncedGameCommands->AddDefaultActionExecutors();
	unsyncedGameCommands->AddDefaultActionExecutors();

	LEAVE_SYNCED_CODE();
}

void CGame::PreLoadRendering()
{
	geometricObjects = new CGeometricObjects();
	worldDrawer = new CWorldDrawer();

	// load components that need to exist before PostLoadSimulation
	worldDrawer->LoadPre();
}

void CGame::PostLoadRendering() {
	worldDrawer->LoadPost();
}


void CGame::LoadInterface()
{
	{
		ScopedOnceTimer timer("Game::LoadInterface (Camera&Mouse)");

		camHandler = new CCameraHandler();
		mouse = CMouseHandler::GetOrReloadInstance();
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
		// add the available Skirmish AI libraries to word completion, for /aicontrol
		const IAILibraryManager::T_skirmishAIKeys& aiLibs = aiLibManager->GetSkirmishAIKeys();
		// add the available Lua AI implementations to word completion, for /aicontrol
		const std::set<std::string>& luaAIShortNames = skirmishAIHandler.GetLuaAIImplShortNames();

		for (const auto& ai: ais) {
			wordCompletion->AddWord(ai.second.name + " ", false, false, false);
		}

		for (const auto& aiLib: aiLibs) {
			wordCompletion->AddWord(aiLib.GetShortName() + " " + aiLib.GetVersion() + " ", false, false, false);
		}

		for (const std::string& sn: luaAIShortNames) {
			wordCompletion->AddWord(sn + " ", false, false, false);
		}

		const auto& unitDefs = unitDefHandler->unitDefIDsByName;
		const auto& featureDefs = featureDefHandler->GetFeatureDefs();

		for (auto uit = unitDefs.cbegin(); uit != unitDefs.cend(); ++uit) {
			wordCompletion->AddWord(uit->first + " ", false, true, false);
		}
		for (auto fit = featureDefs.cbegin(); fit != featureDefs.cend(); ++fit) {
			wordCompletion->AddWord(fit->first + " ", false, true, false);
		}
	}

	if (infoConsole == nullptr)
		infoConsole = new CInfoConsole();

	tooltip = new CTooltipConsole();
	guihandler = new CGuiHandler();
	minimap = new CMiniMap();
	resourceBar = new CResourceBar();
	keyBindings = new CKeyBindings();
	keyBindings->Load("uikeys.txt");
	selectionKeys = new CSelectionKeyHandler();

	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		grouphandlers.push_back(new CGroupHandler(t));
	}

	// note: disable is needed in case user reloads before StartPlaying
	GameSetupDrawer::Disable();
	GameSetupDrawer::Enable();
}

void CGame::LoadLua()
{
	// Lua components
	ENTER_SYNCED_CODE();

	loadscreen->SetLoadMessage("Loading LuaRules");
	CLuaRules::LoadFreeHandler();

	loadscreen->SetLoadMessage("Loading LuaGaia");
	CLuaGaia::LoadFreeHandler();

	LEAVE_SYNCED_CODE();

	loadscreen->SetLoadMessage("Loading LuaUI");
	CLuaUI::LoadFreeHandler();

	spring::SafeDelete(defsParser);
}

void CGame::LoadSkirmishAIs()
{
	if (gameSetup->hostDemo)
		return;

	// create a Skirmish AI if required
	const CSkirmishAIHandler::ids_t& localAIs = skirmishAIHandler.GetSkirmishAIsByPlayer(gu->myPlayerNum);

	if (!localAIs.empty()) {
		loadscreen->SetLoadMessage("Loading Skirmish AIs");

		for (auto ai = localAIs.begin(); ai != localAIs.end(); ++ai) {
			skirmishAIHandler.CreateLocalSkirmishAI(*ai);
		}
	}
}

void CGame::LoadFinalize()
{
	if (saveFile == nullptr) {
		ENTER_SYNCED_CODE();
		eventHandler.GamePreload();
		LEAVE_SYNCED_CODE();
	}

	{
		loadscreen->SetLoadMessage("[" + std::string(__func__) + "] finalizing PFS");

		ENTER_SYNCED_CODE();
		const std::uint64_t dt = pathManager->Finalize();
		const std::uint32_t cs = pathManager->GetPathCheckSum();
		LEAVE_SYNCED_CODE();

		loadscreen->SetLoadMessage(
			"[" + std::string(__func__) + "] finalized PFS " +
			"(" + IntToString(dt, "%ld") + "ms, checksum " + IntToString(cs, "%08x") + ")"
		);
	}

	if (CBenchmark::enabled) {
		static CBenchmark benchmark;

		// in case we reloaded (benchmark does not get deleted
		// until program exit, so only data of the most recent
		// run counts)
		benchmark.ResetState();
	}

	lastReadNetTime = spring_gettime();
	lastSimFrameTime = lastReadNetTime;
	lastDrawFrameTime = lastReadNetTime;
	updateDeltaSeconds = 0.0f;
}


void CGame::PostLoad()
{
	GameSetupDrawer::Disable();

	if (gameServer != nullptr) {
		gameServer->PostLoad(gs->frameNum);
	}
}


void CGame::KillLua(bool dtor)
{
	// belongs here; destructs LuaIntro (which might access sound, etc)
	// if LoadingMT=1, a reload-request might be seen by SpringApp::Run
	// while the loading thread is still alive so this must go first
	assert((!dtor) || (loadscreen == nullptr));

	LOG("[Game::%s][0] dtor=%d loadscreen=%p", __func__, dtor, loadscreen);
	CLoadScreen::DeleteInstance();

	ENTER_SYNCED_CODE();
	LOG("[Game::%s][1] dtor=%d luaGaia=%p", __func__, dtor, luaGaia);
	CLuaGaia::FreeHandler();

	LOG("[Game::%s][2] dtor=%d luaRules=%p", __func__, dtor, luaRules);
	CLuaRules::FreeHandler();

	CLuaHandleSynced::ClearGameParams();
	LEAVE_SYNCED_CODE();

	// kill LuaUI here, various handler pointers are invalid in ~GuiHandler
	LOG("[Game::%s][3] dtor=%d luaUI=%p", __func__, dtor, luaUI);
	CLuaUI::FreeHandler();

	LOG("[Game::%s][4] dtor=%d", __func__, dtor);
	LuaOpenGL::Free();
}

void CGame::KillMisc()
{
	LOG("[Game::%s][1]", __func__);
	CEndGameBox::Destroy();
	IVideoCapturing::FreeInstance();

	LOG("[Game::%s][2]", __func__);
	// delete this first since AI's might call back into sim-components in their dtors
	// this means the simulation *should not* assume the EOH still exists on game exit
	CEngineOutHandler::Destroy();

	LOG("[Game::%s][3]", __func__);
	// TODO move these to the end of this dtor, once all action-executors are registered by their respective engine sub-parts
	UnsyncedGameCommands::DestroyInstance();
	SyncedGameCommands::DestroyInstance();
}

void CGame::KillRendering()
{
	LOG("[Game::%s][1]", __func__);
	spring::SafeDelete(icon::iconHandler);
	spring::SafeDelete(geometricObjects);
	spring::SafeDelete(worldDrawer);
}

void CGame::KillInterface()
{
	LOG("[Game::%s][1]", __func__);
	ProfileDrawer::SetEnabled(false);
	spring::SafeDelete(guihandler);
	spring::SafeDelete(minimap);
	spring::SafeDelete(resourceBar);
	spring::SafeDelete(tooltip); // CTooltipConsole*
	spring::SafeDelete(infoConsole);
	spring::SafeDelete(consoleHistory);
	spring::SafeDelete(keyBindings);
	spring::SafeDelete(selectionKeys); // CSelectionKeyHandler*
	spring::SafeDelete(mouse); // CMouseHandler*
	spring::SafeDelete(inMapDrawerModel);
	spring::SafeDelete(inMapDrawer);

	LOG("[Game::%s][2]", __func__);
	spring::SafeDelete(camHandler);

	for (unsigned int i = 0; i < grouphandlers.size(); i++) {
		spring::SafeDelete(grouphandlers[i]);
	}

	grouphandlers.clear();
}

void CGame::KillSimulation()
{
	LOG("[Game::%s][1]", __func__);

	// Kill all teams that are still alive, in
	// case the game did not do so through Lua.
	//
	// must happen after Lua (cause CGame is already
	// null'ed and Died() causes a Lua event, which
	// could issue Lua code that tries to access it)
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		teamHandler->Team(t)->Died(false);
	}

	LOG("[Game::%s][2] unitHandler=%p", __func__, unitHandler);
	if (unitHandler != nullptr)
		unitHandler->DeleteScripts();

	spring::SafeDelete(featureHandler); // depends on unitHandler (via ~CFeature)
	spring::SafeDelete(unitHandler);
	spring::SafeDelete(projectileHandler);

	LOG("[Game::%s][3]", __func__);
	IPathManager::FreeInstance(pathManager);

	spring::SafeDelete(readMap);
	spring::SafeDelete(smoothGround);
	spring::SafeDelete(groundBlockingObjectMap);
	spring::SafeDelete(buildingMaskMap);
	spring::SafeDelete(losHandler);
	spring::SafeDelete(mapDamage);
	spring::SafeDelete(quadField);
	spring::SafeDelete(moveDefHandler);
	spring::SafeDelete(unitDefHandler);
	spring::SafeDelete(featureDefHandler);
	spring::SafeDelete(weaponDefHandler);
	spring::SafeDelete(damageArrayHandler);
	spring::SafeDelete(explGenHandler);
	spring::SafeDelete((mapInfo = const_cast<CMapInfo*>(mapInfo)));

	LOG("[Game::%s][4]", __func__);
	CCommandAI::KillCommandDescriptionCache();
	CUnitScriptEngine::KillStatic();
	CWeaponLoader::KillStatic();
}





void CGame::ResizeEvent()
{
	if (minimap != nullptr)
		minimap->UpdateGeometry();

	// reload water renderer (it may depend on screen resolution)
	water = IWater::GetWater(water, water->GetID());

	eventHandler.ViewResize();
}


int CGame::KeyPressed(int key, bool isRepeat)
{
	if (!gameOver && !isRepeat) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.keyPresses++;
	}

	const CKeySet ks(key, false);
	curKeyChain.push_back(key, spring_gettime(), isRepeat);

	// Get the list of possible key actions
	//LOG_L(L_DEBUG, "curKeyChain: %s", curKeyChain.GetString().c_str());
	const CKeyBindings::ActionList& actionList = keyBindings->GetActionList(curKeyChain);

	if (userWriting) {
		for (const Action& action: actionList) {
			if (ProcessKeyPressAction(key, action)) {
				// the key was used, ignore it (ex: alt+a)
				ignoreNextChar = keyCodes->IsPrintable(key);
				break;
			}
		}

		return 0;
	}

	if (luaInputReceiver->KeyPressed(key, isRepeat))
		return 0;

	// try the input receivers
	for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
		if (recv != nullptr && recv->KeyPressed(key, isRepeat)) {
			return 0;
		}
	}


	// try our list of actions
	for (const Action& action: actionList) {
		if (ActionPressed(key, action, isRepeat)) {
			return 0;
		}
	}

	// maybe a widget is interested?
	if (luaUI != nullptr) {
		for (const Action& action: actionList) {
			luaUI->GotChatMsg(action.rawline, false);
		}
	}
	if (luaMenu != nullptr) {
		for (const Action& action: actionList) {
			luaMenu->GotChatMsg(action.rawline, false);
		}
	}

	return 0;
}


int CGame::KeyReleased(int k)
{
	if (userWriting) {
		if (
			   keyCodes->IsPrintable(k)
			|| k == SDLK_RETURN || k == SDLK_BACKSPACE || k == SDLK_DELETE
			|| k == SDLK_HOME || k == SDLK_END || k == SDLK_RIGHT || k == SDLK_LEFT
		) {
			return 0;
		}
	}

	if (luaInputReceiver->KeyReleased(k))
		return 0;

	// try the input receivers
	for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
		if (recv != nullptr && recv->KeyReleased(k)) {
			return 0;
		}
	}

	// try our list of actions
	CKeySet ks(k, true);
	const CKeyBindings::ActionList& al = keyBindings->GetActionList(ks);
	for (const Action& action: al) {
		if (ActionReleased(action)) {
			return 0;
		}
	}

	return 0;
}


int CGame::TextInput(const std::string& utf8Text)
{
	if (eventHandler.TextInput(utf8Text))
		return 0;
	if (!userWriting)
		return 0;

	std::string text = ignoreNextChar ? utf8Text.substr(utf8::NextChar(utf8Text, 0)) : utf8Text;

	writingPos = Clamp<int>(writingPos, 0, userInput.length());
	userInput.insert(writingPos, text);
	writingPos += text.length();
	return 0;
}


bool CGame::Update()
{
	good_fpu_control_registers("CGame::Update");

	jobDispatcher.Update();
	clientNet->Update();

	// When video recording do step by step simulation, so each simframe gets a corresponding videoframe
	// FIXME: SERVER ALREADY DOES THIS BY ITSELF
	if (playing && globalRendering->isVideoCapturing && gameServer != nullptr)
		gameServer->CreateNewFrame(false, true);

	ENTER_SYNCED_CODE();
	SendClientProcUsage();
	ClientReadNet(); // issues new SimFrame()s

	if (!gameOver) {
		if (clientNet->NeedsReconnect())
			clientNet->AttemptReconnect(SpringVersion::GetFull());

		if (clientNet->CheckTimeout(0, gs->PreSimFrame()))
			GameEnd({}, true);
	}

	LEAVE_SYNCED_CODE();

	{
		SLuaAllocError error = {};

		if (spring_lua_alloc_get_error(&error)) {
			// convert the "abc\ndef\n..." buffer into "abc", "def", ... chunks
			for (char *ptr = &error.msgBuf[0], *tmp = nullptr; (tmp = strstr(ptr, "\n")) != nullptr; ptr = tmp + 1) {
				*tmp = 0;

				LOG_L(L_FATAL, "%s", error.msgBuf);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_FATAL, error.msgBuf);
			}
		}
	}

	return true;
}


bool CGame::UpdateUnsynced(const spring_time currentTime)
{
	SCOPED_TIMER("Update");

	// timings and frame interpolation
	const spring_time deltaDrawFrameTime = currentTime - lastDrawFrameTime;

	const float modGameDeltaTimeSecs = mix(deltaDrawFrameTime.toMilliSecsf() * 0.001f, 0.01f, skipping);
	const float unsyncedUpdateDeltaTime = (currentTime - lastUnsyncedUpdateTime).toSecsf();

	lastDrawFrameTime = currentTime;

	{
		// update game timings
		globalRendering->lastFrameTime = deltaDrawFrameTime.toMilliSecsf();

		gu->avgFrameTime = mix(gu->avgFrameTime, deltaDrawFrameTime.toMilliSecsf(), 0.05f);
		gu->gameTime += modGameDeltaTimeSecs;
		gu->modGameTime += (modGameDeltaTimeSecs * gs->speedFactor * (1 - gs->paused));

		if (playing && !gameOver) {
			totalGameTime += modGameDeltaTimeSecs;
		}

		updateDeltaSeconds = modGameDeltaTimeSecs;
	}

	{
		// update sim-FPS counter once per second
		static int lsf = gs->frameNum;
		static spring_time lsft = currentTime;

		// toSecsf throws away too much precision
		const float diffMilliSecs = (currentTime - lsft).toMilliSecsf();

		if (diffMilliSecs >= 1000.0f) {
			gu->simFPS = (gs->frameNum - lsf) / (diffMilliSecs * 0.001f);
			lsft = currentTime;
			lsf = gs->frameNum;
		}
	}

	if (skipping) {
		// when fast-forwarding, maintain a draw-rate of 2Hz
		if (spring_tomsecs(currentTime - skipLastDrawTime) < 500.0f)
			return true;

		skipLastDrawTime = currentTime;

		DrawSkip();
		return true;
	}

	numDrawFrames++;
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
	globalRendering->lastFrameStart = currentTime;
	// Update the interpolation coefficient (globalRendering->timeOffset)
	if (!gs->paused && !IsLagging() && !gs->PreSimFrame() && !globalRendering->isVideoCapturing) {
		globalRendering->weightedSpeedFactor = 0.001f * gu->simFPS;
		globalRendering->timeOffset = (currentTime - lastFrameTime).toMilliSecsf() * globalRendering->weightedSpeedFactor;
	} else {
		globalRendering->timeOffset = globalRendering->videoCapturingTimeOffset;
		lastSimFrameTime = currentTime;
		lastFrameTime = currentTime;
	}

	if ((currentTime - frameStartTime).toMilliSecsf() >= 1000.0f) {
		globalRendering->FPS = (numDrawFrames * 1000.0f) / std::max(0.01f, (currentTime - frameStartTime).toMilliSecsf());

		// update draw-FPS counter once every second
		frameStartTime = currentTime;
		numDrawFrames = 0;

	}

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const bool newSimFrame = (lastSimFrame != gs->frameNum);
	const bool forceUpdate = (unsyncedUpdateDeltaTime >= (1.0f / GAME_SPEED));

	lastSimFrame = gs->frameNum;

	// set camera
	camHandler->UpdateController(playerHandler->Player(gu->myPlayerNum), gu->fpsMode, fullscreenEdgeMove, windowedEdgeMove);

	{
		SCOPED_TIMER("Update::UnitDrawer");
		unitDrawer->Update();
	}
	lineDrawer.UpdateLineStipple();


	if (doDrawWorld) {
		worldDrawer->Update(newSimFrame);

		CNamedTextures::Update();
		CFontTexture::Update();
	}
	// always update InfoTexture and SoundListener at <= 30Hz (even when paused)
	// garbage collection event must also run regularly because of unsynced code
	if (newSimFrame || forceUpdate) {
		lastUnsyncedUpdateTime = currentTime;

		{
			// TODO: should be moved to WorldDrawer::Update
			SCOPED_TIMER("Update::InfoTexture");
			infoTextureHandler->Update();
		}

		// TODO call only when camera changed
		sound->UpdateListener(camera->GetPos(), camera->GetDir(), camera->GetUp());
	}

	SetDrawMode(gameNormalDraw); //TODO move to ::Draw()?

	if (luaUI != nullptr) { luaUI->CheckStack(); luaUI->CheckAction(); }
	if (luaGaia != nullptr) { luaGaia->CheckStack(); }
	if (luaRules != nullptr) { luaRules->CheckStack(); }

	if (chatting && !userWriting) {
		consoleHistory->AddLine(userInput);

		std::string msg = userInput;
		std::string pfx = "";

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
	}

	assert(infoConsole);
	infoConsole->PushNewLinesToEventHandler();
	infoConsole->Update();

	mouse->Update();
	mouse->UpdateCursors();
	guihandler->Update();
	commandDrawer->Update();

	{
		SCOPED_TIMER("Update::Update");
		eventHandler.Update();
	}
	eventHandler.DbgTimingInfo(TIMING_UNSYNCED, currentTime, spring_now());
	return false;
}


bool CGame::Draw() {
	const spring_time currentTimePreUpdate = spring_gettime();

	if (UpdateUnsynced(currentTimePreUpdate))
		return false;

	SCOPED_SPECIAL_TIMER("Draw");

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const spring_time currentTimePreDraw = spring_gettime();

	{
		SCOPED_TIMER("Draw::DrawGenesis");
		eventHandler.DrawGenesis();
	}

	if (!globalRendering->active) {
		spring_sleep(spring_msecs(10));

		// return early if and only if less than 30K milliseconds have passed since last draw-frame
		// so we force render two frames per minute when minimized to clear batches and free memory
		// don't need to mess with globalRendering->active since only mouse-input code depends on it
		if ((currentTimePreDraw - lastDrawFrameTime).toSecsi() < 30)
			return false;
	}

	if (globalRendering->drawdebug) {
		const float deltaFrameTime = (currentTimePreUpdate - lastSimFrameTime).toMilliSecsf();
		const float deltaNetPacketProcTime  = (currentTimePreUpdate - lastNetPacketProcessTime ).toMilliSecsf();
		const float deltaReceivedPacketTime = (currentTimePreUpdate - lastReceivedNetPacketTime).toMilliSecsf();
		const float deltaSimFramePacketTime = (currentTimePreUpdate - lastSimFrameNetPacketTime).toMilliSecsf();

		const float currTimeOffset = globalRendering->timeOffset;
		static float lastTimeOffset = globalRendering->timeOffset;
		static int lastGameFrame = gs->frameNum;

		static const char* minFmtStr = "assert(CTO >= 0.0f) failed (SF=%u : DF=%u : CTO=%f : WSF=%f : DT=%fms : DLNPPT=%fms | DLRPT=%fms | DSFPT=%fms : NP=%u)";
		static const char* maxFmtStr = "assert(CTO <= 1.3f) failed (SF=%u : DF=%u : CTO=%f : WSF=%f : DT=%fms : DLNPPT=%fms | DLRPT=%fms | DSFPT=%fms : NP=%u)";

		// CTO = MILLISECSF(CT - LSFT) * WSF = MILLISECSF(CT - LSFT) * (SFPS * 0.001)
		// AT 30Hz LHS (MILLISECSF(CT - LSFT)) SHOULD BE ~33ms, RHS SHOULD BE ~0.03
		assert(currTimeOffset >= 0.0f);

		if (currTimeOffset < 0.0f) LOG_L(L_DEBUG, minFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, clientNet->GetNumWaitingServerPackets());
		if (currTimeOffset > 1.3f) LOG_L(L_DEBUG, maxFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, clientNet->GetNumWaitingServerPackets());

		// test for monotonicity, normally should only fail
		// when SimFrame() advances time or if simframe rate
		// changes
		if (lastGameFrame == gs->frameNum && currTimeOffset < lastTimeOffset)
			LOG_L(L_DEBUG, "assert(CTO >= LTO) failed (SF=%u : DF=%u : CTO=%f : LTO=%f : WSF=%f : DT=%fms)", gs->frameNum, globalRendering->drawFrame, currTimeOffset, lastTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime);

		lastTimeOffset = currTimeOffset;
		lastGameFrame = gs->frameNum;
	}

	//FIXME move both to UpdateUnsynced?
	CTeamHighlight::Enable(spring_tomsecs(currentTimePreDraw));
	if (unitTracker.Enabled())
		unitTracker.SetCam();

	{
		minimap->Update();

		if (doDrawWorld)
			worldDrawer->GenerateIBLTextures();

		camera->Update();

		if (doDrawWorld)
			worldDrawer->Draw();

		worldDrawer->ResetMVPMatrices();
	}

	{
		SCOPED_TIMER("Draw::Screen");

		if (doDrawWorld)
			eventHandler.DrawScreenEffects();

		hudDrawer->Draw((gu->GetMyPlayer())->fpsController.GetControllee());
		debugDrawerAI->Draw();

		DrawInputReceivers();
		DrawInputText();
		DrawInterfaceWidgets();
		mouse->DrawCursor();

		if (doDrawWorld)
			eventHandler.DrawScreenPost();
	}

	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	if (globalRendering->isVideoCapturing) {
		globalRendering->lastFrameTime = 1000.0f / GAME_SPEED;
		if (videoCapturing->IsCapturing())
			videoCapturing->RenderFrame();
	}

	SetDrawMode(gameNotDrawing);
	CTeamHighlight::Disable();

	const spring_time currentTimePostDraw = spring_gettime();
	const spring_time currentFrameDrawTime = currentTimePostDraw - currentTimePreDraw;
	gu->avgDrawFrameTime = mix(gu->avgDrawFrameTime, currentFrameDrawTime.toMilliSecsf(), 0.05f);

	eventHandler.DbgTimingInfo(TIMING_VIDEO, currentTimePreDraw, currentTimePostDraw);

	return true;
}


void CGame::DrawInputReceivers()
{

	glEnable(GL_TEXTURE_2D);

	if (!hideInterface) {
		{
			SCOPED_TIMER("Draw::Screen::InputReceivers");
			CInputReceiver::DrawReceivers();
		}
		{
			// this has MANUAL ordering, draw it last (front-most)
			SCOPED_TIMER("Draw::Screen::DrawScreen");
			luaInputReceiver->Draw();
		}
	} else {
		SCOPED_TIMER("Draw::Screen::Minimap");

		if (globalRendering->dualScreenMode) {
			// minimap is on its own screen, so always draw it
			minimap->Draw();
		}
	}

	glEnable(GL_TEXTURE_2D);
}

void CGame::DrawInterfaceWidgets()
{
	if (hideInterface)
		return;

	smallFont->Begin();

	#define KEY_FONT_FLAGS (FONT_SCALE | FONT_CENTER | FONT_NORM)
	#define INF_FONT_FLAGS (FONT_RIGHT | FONT_SCALE | FONT_NORM | (FONT_OUTLINE * guihandler->GetOutlineFonts()))

	if (showClock) {
		const int seconds = (gs->frameNum / GAME_SPEED);
		if (seconds < 3600) {
			smallFont->glFormat(0.99f, 0.94f, 1.0f, INF_FONT_FLAGS, "%02i:%02i", seconds / 60, seconds % 60);
		} else {
			smallFont->glFormat(0.99f, 0.94f, 1.0f, INF_FONT_FLAGS, "%02i:%02i:%02i", seconds / 3600, (seconds / 60) % 60, seconds % 60);
		}
	}

	if (showFPS) {
		const float4 yellow(1.0f, 1.0f, 0.25f, 1.0f);
		smallFont->SetColors(&yellow,NULL);
		smallFont->glFormat(0.99f, 0.92f, 1.0f, INF_FONT_FLAGS, "%.0f", globalRendering->FPS);
	}

	if (showSpeed) {
		const float4 speedcol(1.0f, gs->speedFactor < gs->wantedSpeedFactor * 0.99f ? 0.25f : 1.0f, 0.25f, 1.0f);
		smallFont->SetColors(&speedcol, NULL);
		smallFont->glFormat(0.99f, 0.90f, 1.0f, INF_FONT_FLAGS, "%2.2f", gs->speedFactor);
	}

	CPlayerRosterDrawer::Draw();
	smallFont->End();
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
	if (!userWriting)
		return;

	const float fontSale = 1.0f;                       // TODO: make configurable again
	const float fontSize = fontSale * font->GetSize();

	const string tempstring = userPrompt + userInput;

	// draw the caret
	{
		const int caretPosStr = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPosStr);
		const float caretPos    = fontSize * font->GetTextWidth(caretStr) * globalRendering->pixelX;
		const float caretHeight = fontSize * font->GetLineHeight() * globalRendering->pixelY;
		int cpos = writingPos;
		char32_t c = utf8::GetNextChar(userInput, cpos);
		if (c == 0) c = ' '; // make caret always visible
		const float cw = fontSize * font->GetCharacterWidth(c) * globalRendering->pixelX;
		const float csx = inputTextPosX + caretPos;

		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + fastmath::sin(spring_now().toMilliSecsf() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, inputTextPosY, csx + cw, inputTextPosY + caretHeight);
		glEnable(GL_TEXTURE_2D);
	}

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
	lastReadNetTime = spring_gettime();

	gu->startTime = gu->gameTime;
	gu->myTeam = playerHandler->Player(gu->myPlayerNum)->team;
	gu->myAllyTeam = teamHandler->AllyTeam(gu->myTeam);
//	grouphandler->team = gu->myTeam;

	GameSetupDrawer::Disable();
	CLuaUI::UpdateTeams();

	// setup the teams
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		CTeam* team = teamHandler->Team(a);

		if (team->gaia)
			continue;

		if (!team->HasValidStartPos() && gameSetup->startPosType == CGameSetup::StartPos_ChooseInGame) {
			// if the player did not choose a start position (eg. if
			// the game was force-started by the host before sending
			// any), silently generate one for him
			// TODO: notify Lua of this also?
			team->SetDefaultStartPos();
		}
	}

	if (saveFile == nullptr)
		eventHandler.GameStart();
}



void CGame::SimFrame() {
	ENTER_SYNCED_CODE();
	ASSERT_SYNCED(gsRNG.GetGenState());

	good_fpu_control_registers("CGame::SimFrame");

	// note: starts at -1, first actual frame is 0
	gs->frameNum += 1;
	lastFrameTime = spring_gettime();

	// clear allocator statistics periodically
	// note: allocator itself should do this (so that
	// stats are reliable when paused) but see LuaUser
	spring_lua_alloc_update_stats((gs->frameNum % GAME_SPEED) == 0);

#ifdef TRACE_SYNC
	tracefile << "New frame:" << gs->frameNum << " " << gsRNG.GetLastSeed() << "\n";
#endif

	if (!skipping) {
		// everything here is unsynced and should ideally moved to Game::Update()
		waitCommandsAI.Update();
		geometricObjects->Update();
		sound->NewFrame();
		eoh->Update();

		for (size_t a = 0; a < grouphandlers.size(); a++)
			grouphandlers[a]->Update();

		CPlayer* p = playerHandler->Player(gu->myPlayerNum);
		FPSUnitController& c = p->fpsController;

		c.SendStateUpdate(/*camera->GetMovState(), mouse->buttons*/);

		CTeamHighlight::Update(gs->frameNum);
	}

	// everything from here is simulation
	{
		SCOPED_SPECIAL_TIMER("Sim");
		{
			SCOPED_TIMER("Sim::GameFrame");
			eventHandler.GameFrame(gs->frameNum);
		}
		helper->Update();
		mapDamage->Update();
		pathManager->Update();
		unitHandler->Update();
		projectileHandler->Update();
		featureHandler->Update();
		{
			SCOPED_TIMER("Sim::Script");
			unitScriptEngine->Tick(33);
		}
		wind.Update();
		losHandler->Update();
		// dead ghosts have to be updated in sim, after los,
		// to make sure they represent the current knowledge correctly.
		// should probably be split from drawer
		unitDrawer->UpdateGhostedBuildings();
		interceptHandler.Update(false);

		teamHandler->GameFrame(gs->frameNum);
		playerHandler->GameFrame(gs->frameNum);
	}

	lastSimFrameTime = spring_gettime();
	gu->avgSimFrameTime = mix(gu->avgSimFrameTime, (lastSimFrameTime - lastFrameTime).toMilliSecsf(), 0.05f);
	gu->avgSimFrameTime = std::max(gu->avgSimFrameTime, 0.001f);

	eventHandler.DbgTimingInfo(TIMING_SIM, lastFrameTime, lastSimFrameTime);

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

	// useful for desync-debugging (enter instead of -1 start & end frame of the range you want to debug)
	DumpState(-1, -1, 1);

	ASSERT_SYNCED(gsRNG.GetGenState());
	LEAVE_SYNCED_CODE();
}


void CGame::GameEnd(const std::vector<unsigned char>& winningAllyTeams, bool timeout)
{
	if (gameOver)
		return;

	if (timeout) {
		// client timed out, don't send anything (in theory the GAMEOVER
		// message should not be able to reach the server if connection
		// is lost, but in practice it can get through --> timeout check
		// needs work)
		if (!configHandler->GetBool("GameEndOnConnectionLoss")) {
			LOG_L(L_WARNING, "[%s] lost connection to server; continuing game", __func__);
			clientNet->InitLocalClient();
			return;
		}

		LOG_L(L_ERROR, "[%s] lost connection to server; terminating game", __func__);
	} else {
		// pass the winner info to the host in the case it's a dedicated server
		clientNet->Send(CBaseNetProtocol::Get().SendGameOver(gu->myPlayerNum, winningAllyTeams));
	}


	gameOver = true;
	eventHandler.GameOver(winningAllyTeams);

	CEndGameBox::Create(winningAllyTeams);
#ifdef    HEADLESS
	profiler.PrintProfilingInfo();
#endif // HEADLESS

	CDemoRecorder* record = clientNet->GetDemoRecorder();

	if (record == nullptr)
		return;

	// Write CPlayer::Statistics and CTeam::Statistics to demo
	// TODO: move this to a method in CTeamHandler
	const int numPlayers = playerHandler->ActivePlayers();
	const int numTeams = teamHandler->ActiveTeams() - int(gs->useLuaGaia);

	record->SetTime(gs->frameNum / GAME_SPEED, (int)gu->gameTime);
	record->InitializeStats(numPlayers, numTeams);
	// pass the list of winners
	record->SetWinningAllyTeams(winningAllyTeams);

	// tell everybody about our APM, it's the most important statistic
	clientNet->Send(CBaseNetProtocol::Get().SendPlayerStat(gu->myPlayerNum, playerHandler->Player(gu->myPlayerNum)->currentStats));

	for (int i = 0; i < numPlayers; ++i) {
		record->SetPlayerStats(i, playerHandler->Player(i)->currentStats);
	}
	for (int i = 0; i < numTeams; ++i) {
		const CTeam* team = teamHandler->Team(i);
		record->SetTeamStats(i, team->statHistory);
		clientNet->Send(CBaseNetProtocol::Get().SendTeamStat(team->teamNum, team->GetCurrentStats()));
	}
}

void CGame::SendNetChat(std::string message, int destination)
{
	if (message.empty())
		return;

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
	clientNet->Send(buf.Pack());
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
				Channels::UserInterface->PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_SPECTATORS) {
			if (gu->spectating || myMsg) {
				LOG("%sSpectators: %s", label.c_str(), s.c_str());
				Channels::UserInterface->PlaySample(chatSound, 5);
			}
		}
		else if (msg.destination == ChatMessage::TO_EVERYONE) {
			const bool specsOnly = noSpectatorChat && (player && player->spectator);
			if (gu->spectating || !specsOnly) {
				if (specsOnly) {
					LOG("%sSpectators: %s", label.c_str(), s.c_str());
				} else {
					LOG("%s%s", label.c_str(), s.c_str());
				}
				Channels::UserInterface->PlaySample(chatSound, 5);
			}
		}
		else if ((msg.destination < playerHandler->ActivePlayers()) && player)
		{	// player -> spectators and spectator -> player PMs should be forbidden
			// player <-> player and spectator <-> spectator are allowed
			if (msg.destination == gu->myPlayerNum && player->spectator == gu->spectating) {
				LOG("%sPrivate: %s", label.c_str(), s.c_str());
				Channels::UserInterface->PlaySample(chatSound, 5);
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
/*
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
*/
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
		LOG_L(L_WARNING, "[Game::%s] can only be used if cheating is enabled", __func__);
		return;
	}

	if (msg.empty()) {
		LOG_L(L_WARNING, "[Game::%s] missing UnitDef name", __func__);
		return;
	}

	const UnitDef* udef = unitDefHandler->GetUnitDefByName(msg);

	if (udef == nullptr) {
		LOG_L(L_WARNING, "[Game::%s] unknown UnitDef name: \"%s\"", __func__, msg.c_str());
		return;
	}

	unitScriptEngine->ReloadScripts(udef);
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

bool CGame::IsLagging(float maxLatency) const
{
	const spring_time timeNow = spring_gettime();
	const float diffTime = spring_tomsecs(timeNow - lastFrameTime);

	return (!gs->paused && (diffTime > (maxLatency / gs->speedFactor)));
}


void CGame::SaveGame(const std::string& filename, bool overwrite, bool usecreg)
{
	if (!FileSystem::CreateDirectory("Saves"))
		return;

	if (overwrite || !FileSystem::FileExists(filename)) {
		LOG("[Game::%s] saving game to %s", __func__, filename.c_str());
		ILoadSaveHandler* ls = ILoadSaveHandler::Create(usecreg);
		ls->mapName = gameSetup->mapName;
		ls->modName = gameSetup->modName;
		ls->SaveGame(filename);
		delete ls;
	} else {
		LOG_L(L_WARNING, "File %s already exists(use /save -y to override)", filename.c_str());
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




bool CGame::ProcessCommandText(unsigned int key, const std::string& command) {
	if (command.size() <= 2)
		return false;

	if ((command[0] == '/') && (command[1] != '/')) {
		// strip the '/'
		const Action fakeAction(command.substr(1));

		ProcessAction(fakeAction, key, false);
		return true;
	}

	return false;
}

bool CGame::ProcessKeyPressAction(unsigned int key, const Action& action) {
	if (!userWriting)
		return false;

	if (action.command == "pastetext") {
		//we cannot use extra commands because tokenization strips multiple spaces
		//or even trailing spaces, the text should be copied verbatim
		const std::string pastecommand = "pastetext ";

		if (action.rawline.length() > pastecommand.length()) {
			userInput.insert(writingPos, action.rawline.substr(pastecommand.length(), action.rawline.length() - pastecommand.length()));
			writingPos += (action.rawline.length() - pastecommand.length());
		} else {
			PasteClipboard();
		}

		return true;
	}

	if (action.command.find("edit_") == 0) {
		if (action.command == "edit_return") {
			userWriting = false;
			writingPos = 0;

			if (chatting) {
				string command;

				if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
					command = userInput.substr(2);
				} else {
					command = userInput;
				}

				if (ProcessCommandText(key, command)) {
					// execute an action
					consoleHistory->AddLine(command);
					LOG_L(L_DEBUG, "%s", command.c_str());

					chatting = false;
					userInput = "";
					writingPos = 0;
				}
			}

			return true;
		}

		if (action.command == "edit_escape") {
			if (chatting || inMapDrawer->IsWantLabel()) {
				if (chatting) {
					consoleHistory->AddLine(userInput);
				}
				userWriting = false;
				chatting = false;
				inMapDrawer->SetWantLabel(false);
				userInput = "";
				writingPos = 0;
			}

			return true;
		}

		if (action.command == "edit_complete") {
			string head = userInput.substr(0, writingPos);
			string tail = userInput.substr(writingPos);

			// fills head with the first partial match
			const vector<string>& partials = wordCompletion->Complete(head);

			userInput = head + tail;
			writingPos = head.length();

			if (!partials.empty()) {
				string msg;
				for (unsigned int i = 0; i < partials.size(); i++) {
					msg += "  ";
					msg += partials[i];
				}
				LOG("%s", msg.c_str());
			}

			return true;
		}


		if (action.command == "edit_backspace") {
			if (!userInput.empty() && (writingPos > 0)) {
				const int prev = utf8::PrevChar(userInput, writingPos);
				userInput.erase(prev, writingPos - prev);
				writingPos = prev;
			}

			return true;
		}

		if (action.command == "edit_delete") {
			if (!userInput.empty() && (writingPos < (int)userInput.size())) {
				userInput.erase(writingPos, utf8::CharLen(userInput, writingPos));
			}

			return true;
		}


		if (action.command == "edit_home") {
			writingPos = 0;
			return true;
		}

		if (action.command == "edit_end") {
			writingPos = userInput.length();
			return true;
		}


		if (action.command == "edit_prev_char") {
			writingPos = utf8::PrevChar(userInput, writingPos);
			return true;
		}

		if (action.command == "edit_next_char") {
			writingPos = utf8::NextChar(userInput, writingPos);
			return true;
		}


		if (action.command == "edit_prev_word") { //TODO don't seems to work correctly with utf-8
			// prev word
			const char* s = userInput.c_str();
			int p = writingPos;
			while ((p > 0) && !isalnum(s[p - 1])) { p--; }
			while ((p > 0) &&  isalnum(s[p - 1])) { p--; }
			writingPos = p;
			return true;
		}

		if (action.command == "edit_next_word") { //TODO don't seems to work correctly with utf-8
			const size_t len = userInput.length();
			const char* s = userInput.c_str();
			size_t p = writingPos;
			while ((p < len) && !isalnum(s[p])) { p++; }
			while ((p < len) &&  isalnum(s[p])) { p++; }
			writingPos = p;
			return true;
		}


		if ((action.command == "edit_prev_line") && chatting) {
			userInput = consoleHistory->PrevLine(userInput);
			writingPos = (int)userInput.length();
			return true;
		}

		if ((action.command == "edit_next_line") && chatting) {
			userInput = consoleHistory->NextLine(userInput);
			writingPos = (int)userInput.length();
			return true;
		}

		// unknown edit-command
		return false;
	}


	if (action.command.find("chatswitch") == 0) {
		if (action.command == "chatswitchall") {
			if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
				userInput = userInput.substr(2);
				writingPos = std::max(0, writingPos - 2);
			}

			userInputPrefix = "";
			return true;
		}

		if (action.command == "chatswitchally") {
			if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
				// already are in ally chat -> toggle it off
				userInput = userInput.substr(2);
				writingPos = std::max(0, writingPos - 2);
				userInputPrefix = "";
			}
			else if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
				// are in spec chat -> switch it to ally chat
				userInput[0] = 'a';
				userInputPrefix = "a:";
			} else {
				userInput = "a:" + userInput;
				writingPos += 2;
				userInputPrefix = "a:";
			}

			return true;
		}

		if (action.command == "chatswitchspec") {
			if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
				// already are in spec chat -> toggle it off
				userInput = userInput.substr(2);
				writingPos = std::max(0, writingPos - 2);
				userInputPrefix = "";
			}
			else if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
				// are in ally chat -> switch it to spec chat
				userInput[0] = 's';
				userInputPrefix = "s:";
			} else {
				userInput = "s:" + userInput;
				writingPos += 2;
				userInputPrefix = "s:";
			}

			return true;
		}

		// unknown chat-command
		return false;
	}

	return false;
}


bool CGame::ProcessAction(const Action& action, unsigned int key, bool isRepeat)
{
	if (ActionPressed(key, action, isRepeat))
		return true;

	// maybe a widget is interested?
	if (luaUI != nullptr)
		luaUI->GotChatMsg(action.rawline, false); //FIXME add return argument!

	if (luaMenu != nullptr)
		luaMenu->GotChatMsg(action.rawline, false); //FIXME add return argument!

	return false;
}

void CGame::ActionReceived(const Action& action, int playerID)
{
	const ISyncedActionExecutor* executor = syncedGameCommands->GetActionExecutor(action.command);

	if (executor != nullptr) {
		// an executor for that action was found
		executor->ExecuteAction(SyncedAction(action, playerID));
		return;
	}

	if (!gs->PreSimFrame()) {
		eventHandler.SyncedActionFallback(action.rawline, playerID);
		//FIXME add unsynced one?
	}
}

bool CGame::ActionPressed(unsigned int key, const Action& action, bool isRepeat)
{
	const IUnsyncedActionExecutor* executor = unsyncedGameCommands->GetActionExecutor(action.command);

	if (executor != NULL) {
		// an executor for that action was found
		if (executor->ExecuteAction(UnsyncedAction(action, key, isRepeat))) {
			return true;
		}
	}

	const std::set<std::string>& serverCommands = CGameServer::GetCommandBlackList();

	if (serverCommands.find(action.command) != serverCommands.end()) {
		CommandMessage pckt(action, gu->myPlayerNum);
		clientNet->Send(pckt.Pack());
		return true;
	}

	return (commandConsole.ExecuteAction(action));
}
