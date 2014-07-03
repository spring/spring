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
#include "Game/UI/UnitTracker.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Fonts/CFontTexture.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/Fonts/glFont.h"
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
#include "Rendering/UnitDrawer.h"
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
#include "Lua/LuaUtils.h"
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
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/SpringApp.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
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
#include <boost/thread.hpp>

#undef CreateDirectory

CONFIG(bool, WindowedEdgeMove).defaultValue(true).description("Sets whether moving the mouse cursor to the screen edge will move the camera across the map.");
CONFIG(bool, FullscreenEdgeMove).defaultValue(true).description("see WindowedEdgeMove, just for fullscreen mode");
CONFIG(bool, ShowFPS).defaultValue(false).description("Displays current framerate.");
CONFIG(bool, ShowClock).defaultValue(true).description("Displays a clock on the top-right corner of the screen showing the elapsed time of the current game.");
CONFIG(bool, ShowSpeed).defaultValue(false).description("Displays current game speed.");
CONFIG(bool, ShowMTInfo).defaultValue(true);
CONFIG(int, ShowPlayerInfo).defaultValue(1);
CONFIG(float, GuiOpacity).defaultValue(0.8f).minimumValue(0.0f).maximumValue(1.0f).description("Sets the opacity of the built-in Spring UI. Generally has no effect on LuaUI widgets. Can be set in-game using shift+, to decrease and shift+. to increase.");
CONFIG(std::string, InputTextGeo).defaultValue("");
CONFIG(bool, LuaModUICtrl).defaultValue(true);


CGame* game = NULL;


CR_BIND(CGame, (std::string(""), std::string(""), NULL));

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
	CR_MEMBER(showMTInfo),
	CR_MEMBER(noSpectatorChat),
	CR_MEMBER(gameID),
	//CR_MEMBER(infoConsole),
	//CR_MEMBER(consoleHistory),
	CR_IGNORED(inputTextPosX),
	CR_IGNORED(inputTextPosY),
	CR_IGNORED(inputTextSizeX),
	CR_IGNORED(inputTextSizeY),
	CR_IGNORED(skipping),
	CR_MEMBER(playing),
	CR_IGNORED(msgProcTimeLeft),
	CR_IGNORED(consumeSpeedMult),

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
		, name("")
		{}

		std::function<bool()> f; // allows us to use lambdas

		float freq;
		//bool inSim;
		//bool inDraw;
		bool startDirect;
		bool catchUp;

		const char* name;
	};

public:
	static void AddJob(Job j, const spring_time t) {
		spring_time jobTime = t;

		// never overwrite one job by another (!)
		while (jobs.find(jobTime) != jobs.end())
			jobTime += spring_time(1);

		jobs[jobTime] = j;
	}

	static void Update() {
		const spring_time now = spring_gettime();

		std::map<spring_time, Job>::iterator it = jobs.begin();

		while (it != jobs.end()) {
			if (it->first > now) {
				++it; continue;
			}

			Job* j = &it->second;

			if (j->f()) {
				AddJob(*j, (j->catchUp ? it->first : spring_gettime()) + spring_time(1000.0f / j->freq));
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
	j.freq = GAME_SPEED;
	j.name = "EventHandler::CollectGarbage";
	// static initialization is done BEFORE Spring's time-epoch is set
	JobDispatcher::AddJob(j, spring_notime + spring_time(j.startDirect ? 0 : (1000.0f / j.freq)));
);

DO_ONCE_FNC(
	JobDispatcher::Job j;
	j.f = []() -> bool {
		profiler.Update();
		return true;
	};
	j.freq = 1;
	j.name = "Profiler::Update";
	JobDispatcher::AddJob(j, spring_notime + spring_time(j.startDirect ? 0 : (1000.0f / j.freq)));
);



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
	, skipLastDrawTime(spring_gettime())
	, speedControl(-1)
	, infoConsole(NULL)
	, consoleHistory(NULL)
	, worldDrawer(NULL)
	, defsParser(NULL)
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
	showMTInfo = configHandler->GetBool("ShowMTInfo");

	speedControl = configHandler->GetInt("SpeedControl");

	playerRoster.SetSortTypeByCode((PlayerRoster::SortType)configHandler->GetInt("ShowPlayerInfo"));

	CInputReceiver::guiAlpha = configHandler->GetFloat("GuiOpacity");

	ParseInputTextGeometry("default");
	ParseInputTextGeometry(configHandler->GetString("InputTextGeo"));

	CLuaHandle::SetModUICtrl(configHandler->GetBool("LuaModUICtrl"));

	modInfo.Init(modName.c_str());

	// FIXME: THIS HAS ALREADY BEEN CALLED! (SpringApp::Initialize)
	// Threading::InitThreadPool();
	Threading::SetThreadScheduler();

	if (!mapInfo) {
		mapInfo = new CMapInfo(gameSetup->MapFile(), gameSetup->mapName);
	}

	showMTInfo = -1;

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
	LOG("[%s][1]", __FUNCTION__);

	CEndGameBox::Destroy();
	CLoadScreen::DeleteInstance(); // make sure to halt loading, otherwise crash :)
	CColorMap::DeleteColormaps();

	IVideoCapturing::FreeInstance();

	LOG("[%s][2]", __FUNCTION__);
	CLuaGaia::FreeHandler();
	LOG("[%s][3]", __FUNCTION__);
	CLuaRules::FreeHandler();
	LOG("[%s][4]", __FUNCTION__);
	LuaOpenGL::Free();
	LOG("[%s][5]", __FUNCTION__);

	// Kill all teams that are still alive, in
	// case the game did not do so through Lua.
	//
	// must happen after Lua (cause CGame is already
	// null'ed and Died() causes a Lua event, which
	// could issue Lua code that tries to access it)
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		teamHandler->Team(t)->Died(false);
	}

	CEngineOutHandler::Destroy();
	CResourceHandler::FreeInstance();

	// TODO move these to the end of this dtor, once all action-executors are registered by their respective engine sub-parts
	UnsyncedGameCommands::DestroyInstance();
	SyncedGameCommands::DestroyInstance();
	CWordCompletion::DestroyInstance();

	LOG("[%s][6]", __FUNCTION__);
	SafeDelete(worldDrawer);
	SafeDelete(guihandler); // frees LuaUI
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

	LOG("[%s][7]", __FUNCTION__);
	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(camHandler);
	SafeDelete(camera);
	SafeDelete(cam2);
	SafeDelete(icon::iconHandler);
	SafeDelete(geometricObjects);
	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	LOG("[%s][8]", __FUNCTION__);
	SafeDelete(featureHandler); // depends on unitHandler (via ~CFeature)
	SafeDelete(unitHandler); // depends on modelParser (via ~CUnit)
	SafeDelete(projectileHandler);

	LOG("[%s][9]", __FUNCTION__);
	SafeDelete(cubeMapHandler);
	SafeDelete(modelParser);

	LOG("[%s][10]", __FUNCTION__);
	SafeDelete(pathManager);
	SafeDelete(readMap);
	SafeDelete(smoothGround);
	SafeDelete(groundBlockingObjectMap);
	SafeDelete(radarHandler);
	SafeDelete(losHandler);
	SafeDelete(mapDamage);
	SafeDelete(quadField);
	SafeDelete(moveDefHandler);
	SafeDelete(unitDefHandler);
	SafeDelete(weaponDefHandler);
	SafeDelete(damageArrayHandler);
	SafeDelete(explGenHandler);
	SafeDelete(helper);
	SafeDelete((mapInfo = const_cast<CMapInfo*>(mapInfo)));

	LOG("[%s][11]", __FUNCTION__);
	CClassicGroundMoveType::DeleteLineTable();
	CCategoryHandler::RemoveInstance();

	for (unsigned int i = 0; i < grouphandlers.size(); i++) {
		SafeDelete(grouphandlers[i]);
	}
	grouphandlers.clear();

	LOG("[%s][12]", __FUNCTION__);
	SafeDelete(saveFile); // ILoadSaveHandler, depends on vfsHandler via ~IArchive
	LOG("[%s][13]", __FUNCTION__);
	SafeDelete(vfsHandler);
	LOG("[%s][14]", __FUNCTION__);
	SafeDelete(archiveScanner);

	LOG("[%s][15]", __FUNCTION__);
	SafeDelete(gameServer);
	LOG("[%s][16]", __FUNCTION__);
	ISound::Shutdown();

	LOG("[%s][17]", __FUNCTION__);
	LEAVE_SYNCED_CODE();
}


void CGame::LoadGame(const std::string& mapName, bool threaded)
{
	// NOTE:
	//   this is needed for LuaHandle::CallOut*UpdateCallIn
	//   the main-thread is NOT the same as the load-thread
	//   when LoadingMT=1 (!!!)
	Threading::SetGameLoadThread();
	Watchdog::RegisterThread(WDT_LOAD);

	if (!gu->globalQuit) LoadMap(mapName);
	if (!gu->globalQuit) LoadDefs();
	if (!gu->globalQuit) PreLoadSimulation();
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

	Watchdog::DeregisterThread(WDT_LOAD);
}


void CGame::LoadMap(const std::string& mapName)
{
	ENTER_SYNCED_CODE();

	{
		// after this, other components are able to register chat action-executors
		SyncedGameCommands::CreateInstance();
		UnsyncedGameCommands::CreateInstance();

		CWordCompletion::CreateInstance();

		loadscreen->SetLoadMessage("Parsing Map Information");

		// simulation components
		helper = new CGameHelper();

		readMap = CReadMap::LoadMap(mapName);
		groundBlockingObjectMap = new CGroundBlockingObjectMap(gs->mapSquares);
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

void CGame::PreLoadSimulation()
{
	ENTER_SYNCED_CODE();

	loadscreen->SetLoadMessage("Creating Smooth Height Mesh");
	smoothGround = new SmoothHeightMesh(float3::maxxpos, float3::maxzpos, SQUARE_SIZE * 2, SQUARE_SIZE * 40);

	loadscreen->SetLoadMessage("Creating QuadField & CEGs");
	moveDefHandler = new MoveDefHandler(defsParser);
	quadField = new CQuadField((gs->mapx * SQUARE_SIZE) / CQuadField::BASE_QUAD_SIZE, (gs->mapy * SQUARE_SIZE) / CQuadField::BASE_QUAD_SIZE);
	damageArrayHandler = new CDamageArrayHandler(defsParser);
	explGenHandler = new CExplosionGeneratorHandler();
}

void CGame::PostLoadSimulation()
{
	loadscreen->SetLoadMessage("Loading Weapon Definitions");
	weaponDefHandler = new CWeaponDefHandler(defsParser);
	loadscreen->SetLoadMessage("Loading Unit Definitions");
	unitDefHandler = new CUnitDefHandler(defsParser);

	CClassicGroundMoveType::CreateLineTable();

	unitHandler = new CUnitHandler();
	projectileHandler = new CProjectileHandler();

	loadscreen->SetLoadMessage("Loading Feature Definitions");
	featureHandler = new CFeatureHandler(defsParser);

	losHandler = new CLosHandler();
	radarHandler = new CRadarHandler(false);

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
	eventHandler.GamePreload();

	{
		loadscreen->SetLoadMessage("[" + std::string(__FUNCTION__) + "] finalizing PFS");

		ENTER_SYNCED_CODE();
		const boost::uint64_t dt = pathManager->Finalize();
		const boost::uint32_t cs = pathManager->GetPathCheckSum();
		LEAVE_SYNCED_CODE();

		loadscreen->SetLoadMessage(
			"[" + std::string(__FUNCTION__) + "] finalized PFS " +
			"(" + IntToString(dt, "%ld") + "ms, checksum " + IntToString(cs, "%08x") + ")"
		);
	}

	if (CBenchmark::enabled) {
		static CBenchmark benchmark;
	}

	lastReadNetTime = spring_gettime();
	lastSimFrameTime = lastReadNetTime;
	lastDrawFrameTime = lastReadNetTime;
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
				ignoreNextChar = true;
				break;
			}
		}

		return 0;
	}

	// try the input receivers
	for (CInputReceiver* recv: GetInputReceivers()) {
		if (recv && recv->KeyPressed(key, isRepeat)) {
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
	if (luaUI != NULL) {
		for (const Action& action: actionList) {
			luaUI->GotChatMsg(action.rawline, false);
		}
	}

	return 0;
}


int CGame::KeyReleased(int k)
{
	if (userWriting && keyCodes->IsPrintable(k)) {
		return 0;
	}

	// try the input receivers
	for (CInputReceiver* recv: GetInputReceivers()) {
		if (recv && recv->KeyReleased(k)) {
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



bool CGame::Update()
{
	//TODO move as much as possible unsynced code out of this function

	good_fpu_control_registers("CGame::Update");

	JobDispatcher::Update();
	net->Update();

	// When video recording do step by step simulation, so each simframe gets a corresponding videoframe
	// FIXME: SERVER ALREADY DOES THIS BY ITSELF
	if (videoCapturing->IsCapturing() && playing && gameServer != NULL) {
		gameServer->CreateNewFrame(false, true);
	}

	ENTER_SYNCED_CODE();
	SendClientProcUsage();
	ClientReadNet(); // this can issue new SimFrame()s

	if (!gameOver) {
		if (net->NeedsReconnect()) {
			net->AttemptReconnect(SpringVersion::GetFull());
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


bool CGame::UpdateUnsynced(const spring_time currentTime)
{
	// timings and frame interpolation
	const spring_time deltaDrawFrameTime = currentTime - lastDrawFrameTime;
	const float modGameDeltaTimeSecs = mix(deltaDrawFrameTime.toMilliSecsf() * 0.001f, 0.01f, skipping);

	lastDrawFrameTime = currentTime;

	globalRendering->lastFrameTime = deltaDrawFrameTime.toMilliSecsf();
	gu->avgFrameTime = mix(gu->avgFrameTime, deltaDrawFrameTime.toMilliSecsf(), 0.05f);

	{
		// update game timings
		gu->gameTime += modGameDeltaTimeSecs;
		gu->modGameTime += (modGameDeltaTimeSecs * gs->speedFactor * (1 - gs->paused));

		if (playing && !gameOver) {
			totalGameTime += modGameDeltaTimeSecs;
		}

		updateDeltaSeconds = modGameDeltaTimeSecs;
	}

	{
		// update simFPS counter (once per second)
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

	// Update the interpolation coefficient (globalRendering->timeOffset)
	if (!gs->paused && !IsLagging() && gs->frameNum > 1 && !videoCapturing->IsCapturing()) {
		globalRendering->lastFrameStart = currentTime;
		globalRendering->weightedSpeedFactor = 0.001f * gu->simFPS;
		globalRendering->timeOffset = (currentTime - lastSimFrameTime).toMilliSecsf() * globalRendering->weightedSpeedFactor;
	} else {
		globalRendering->timeOffset = 0;
		lastSimFrameTime = currentTime;
	}

	if ((currentTime - frameStartTime).toMilliSecsf() >= 1000.0f) {
		globalRendering->FPS = (numDrawFrames * 1000.0f) / (currentTime - frameStartTime).toMilliSecsf();

		// update FPS counter once every second
		frameStartTime = currentTime;
		numDrawFrames = 0;

	}

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const bool newSimFrame = (lastSimFrame != gs->frameNum);
	lastSimFrame = gs->frameNum;

	// set camera
	UpdateCam();
	camHandler->UpdateCam();
	camera->Update();

	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
	unitDrawer->Update();
	lineDrawer.UpdateLineStipple();
	if (doDrawWorld) {
		worldDrawer->Update();
		CNamedTextures::Update();
		modelParser->Update();
		CFontTexture::Update();

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

		if (gd->GetDrawMode() != CBaseGroundDrawer::drawNormal) {
			SCOPED_TIMER("GroundDrawer::UpdateExtraTex");
			gd->UpdateExtraTexture(gd->GetDrawMode());
		}

		// TODO call only when camera changed
		sound->UpdateListener(camera->GetPos(), camera->forward, camera->up, unsyncedUpdateDeltaTime);
	}

	SetDrawMode(gameNormalDraw); //TODO move to ::Draw()?

	if (luaUI)    { luaUI->CheckStack(); }
	if (luaGaia)  { luaGaia->CheckStack(); }
	if (luaRules) { luaRules->CheckStack(); }

	// XXX ugly hack to minimize luaUI errors
	if (luaUI && luaUI->GetCallInErrors() >= 5) {
		for (int annoy = 0; annoy < 8; annoy++) {
			LOG_L(L_ERROR, "5 errors deep in LuaUI, disabling...");
		}

		CLuaUI::FreeHandler();
		LOG_L(L_ERROR, "Type '/luaui reload' in the chat to re-enable LuaUI.");
		LOG_L(L_ERROR, "===>>>  Please report this error to the forum or mantis with your infolog.txt");
	}

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

	configHandler->Update();
	mouse->Update();
	mouse->UpdateCursors();
	guihandler->Update();
	LuaUnsyncedCtrl::ClearUnitCommandQueues();
	eventHandler.Update();

	eventHandler.DbgTimingInfo(TIMING_UNSYNCED, currentTime, spring_now());
	return false;
}


bool CGame::Draw() {
	const spring_time currentTimePreUpdate = spring_gettime();

	if (UpdateUnsynced(currentTimePreUpdate))
		return true;

	const bool doDrawWorld = hideInterface || !minimap->GetMaximized() || minimap->GetMinimized();
	const spring_time currentTimePreDraw = spring_gettime();

	eventHandler.DrawGenesis();

	if (!globalRendering->active) {
		spring_sleep(spring_msecs(10));

		// return early if and only if less than 30K milliseconds have passed since last draw-frame
		// so we force render two frames per minute when minimized to clear batches and free memory
		// don't need to mess with globalRendering->active since only mouse-input code depends on it
		if ((currentTimePreDraw - lastDrawFrameTime).toSecsi() < 30)
			return true;
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

		if (currTimeOffset < 0.0f) LOG_L(L_ERROR, minFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, net->GetNumWaitingServerPackets());
		if (currTimeOffset > 1.3f) LOG_L(L_ERROR, maxFmtStr, gs->frameNum, globalRendering->drawFrame, currTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime, deltaNetPacketProcTime, deltaReceivedPacketTime, deltaSimFramePacketTime, net->GetNumWaitingServerPackets());

		// test for monotonicity, normally should only fail
		// when SimFrame() advances time or if simframe rate
		// changes
		if (lastGameFrame == gs->frameNum && currTimeOffset < lastTimeOffset)
			LOG_L(L_ERROR, "assert(CTO >= LTO) failed (SF=%u : DF=%u : CTO=%f : LTO=%f : WSF=%f : DT=%fms)", gs->frameNum, globalRendering->drawFrame, currTimeOffset, lastTimeOffset, globalRendering->weightedSpeedFactor, deltaFrameTime);

		lastTimeOffset = currTimeOffset;
		lastGameFrame = gs->frameNum;
	}

	//FIXME move both to UpdateUnsynced?
	CTeamHighlight::Enable(spring_tomsecs(currentTimePreDraw));
	if (unitTracker.Enabled()) {
		unitTracker.SetCam();
	}

	{
		minimap->Update();

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
					readMap->UpdateShadingTexture();
				}
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
			for (auto ri = inputReceivers.rbegin(); ri != inputReceivers.rend(); ++ri) {
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

	#define KEY_FONT_FLAGS (FONT_SCALE | FONT_CENTER | FONT_NORM)
	#define INF_FONT_FLAGS (FONT_RIGHT | FONT_SCALE | FONT_NORM | (FONT_OUTLINE * guihandler->GetOutlineFonts()))

	if (userWriting) {
		DrawInputText();
	}

	if (!hideInterface) {
		smallFont->Begin();

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

	mouse->DrawCursor();

	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	videoCapturing->RenderFrame();

	SetDrawMode(gameNotDrawing);
	CTeamHighlight::Disable();

	const spring_time currentTimePostDraw = spring_gettime();
	gu->avgDrawFrameTime = mix(gu->avgDrawFrameTime, (currentTimePostDraw - currentTimePreDraw).toMilliSecsf(), 0.05f);

	eventHandler.DbgTimingInfo(TIMING_VIDEO, currentTimePreDraw, currentTimePostDraw);

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
	{
		const int caretPosStr = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPosStr);
		const float caretPos    = fontSize * font->GetTextWidth(caretStr) * globalRendering->pixelX;
		const float caretHeight = fontSize * font->GetLineHeight() * globalRendering->pixelY;
		int cpos = writingPos;
		char32_t c = Utf8GetNextChar(userInput, cpos);
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
	GameSetupDrawer::Disable();
	lastReadNetTime = spring_gettime();

	gu->startTime = gu->gameTime;
	gu->myTeam = playerHandler->Player(gu->myPlayerNum)->team;
	gu->myAllyTeam = teamHandler->AllyTeam(gu->myTeam);
//	grouphandler->team = gu->myTeam;
	CLuaUI::UpdateTeams();

	{
		// keep connection to server alive in case we need to instantiate AI's
		// (which can individually trigger massive thread-blocking operations)
		net->KeepUpdating(true);
		boost::thread pingThread = boost::thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

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

			if (gameSetup->hostDemo)
				continue;

			// create a Skirmish AI if required
			const CSkirmishAIHandler::ids_t& localAIs = skirmishAIHandler.GetSkirmishAIsInTeam(a, gu->myPlayerNum);

			for (auto ai = localAIs.begin(); ai != localAIs.end(); ++ai) {
				skirmishAIHandler.CreateLocalSkirmishAI(*ai);
			}
		}

		net->KeepUpdating(false);
		pingThread.join();
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

}



void CGame::SimFrame() {
	ENTER_SYNCED_CODE();

	good_fpu_control_registers("CGame::SimFrame");
	lastFrameTime = spring_gettime();

	// clear allocator statistics periodically
	// note: allocator itself should do this (so that
	// stats are reliable when paused) but see LuaUser
	spring_lua_alloc_update_stats(((++gs->frameNum) % GAME_SPEED) == 0);

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

		(playerHandler->Player(gu->myPlayerNum)->fpsController).SendStateUpdate(/*camera->GetMovState(), mouse->buttons*/);

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
	losHandler->Update();
	interceptHandler.Update(false);

	teamHandler->GameFrame(gs->frameNum);
	playerHandler->GameFrame(gs->frameNum);

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

	// usefull for desync-debugging enter (enter instead of -1 start & end frame of the range you want to debug)
	DumpState(-1, -1, 1);

	LEAVE_SYNCED_CODE();
}


void CGame::UpdateCam()
{
	//FIXME move to camHandler

	CCameraController& cc = camHandler->GetCurrentController();
	FPSUnitController& fpsCon = playerHandler->Player(gu->myPlayerNum)->fpsController;
	if (fpsCon.oldDCpos != ZeroVector) {
		cc.SetPos(fpsCon.oldDCpos);
		fpsCon.oldDCpos = ZeroVector;
	}

	if (!gu->fpsMode) {
		// Note: GetMoveVectorFromState doesn't return a vec3, instead it returns a xy-vector and in its
		//       z-component it returns a speed scaling factor!

		// key scrolling
		const float3 camMoveVector = camera->GetMoveVectorFromState(true);
		const bool moved = ((camMoveVector * XYVector).SqLength() > 0.0f);
		if (moved && cc.DisableTrackingByKey()) unitTracker.Disable();
		if (moved) cc.KeyMove(camMoveVector);

		// screen edge scrolling
		if ((globalRendering->fullScreen && fullscreenEdgeMove) || (!globalRendering->fullScreen && windowedEdgeMove)) {
			const float3 camMoveVector = camera->GetMoveVectorFromState(false);
			const bool moved = ((camMoveVector * XYVector).SqLength() > 0.0f);
			if (moved) unitTracker.Disable();
			if (moved) cc.ScreenEdgeMove(camMoveVector);
		}

		// mouse wheel zoom
		float mouseWheelDir  = camera->GetMoveDistance(NULL, NULL, CCamera::MOVE_STATE_UP);
		      mouseWheelDir += camera->GetMoveDistance(NULL, NULL, CCamera::MOVE_STATE_DWN);
		if (math::fabsf(mouseWheelDir) > 0.0f) cc.MouseWheelMove(mouseWheelDir);
	}

	cc.Update();
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
			*buf << *(team->currentStats);
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
	if (luaUI != NULL) {
		luaUI->GotChatMsg(action.rawline, false); //FIXME add return argument!
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
		eventHandler.SyncedActionFallback(action.rawline, playerID);
		//FIXME add unsynced one?
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

	const std::set<std::string>& serverCommands = CGameServer::GetCommandBlackList();

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
