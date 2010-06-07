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

#include <boost/thread/barrier.hpp>

#include <SDL_keyboard.h>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <SDL_events.h>

#include "mmgr.h"

#include "Game.h"
#include "float.h"
#include "Camera.h"
#include "CameraHandler.h"
#include "ClientSetup.h"
#include "ConsoleHistory.h"
#include "FPUCheck.h"
#include "GameHelper.h"
#include "GameServer.h"
#include "GameVersion.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "PlayerRoster.h"
#include "Sync/SyncTracer.h"
#include "ChatMessage.h"
#include "TimeProfiler.h"
#include "WaitCommandsAI.h"
#include "WordCompletion.h"
#include "OSCStatsSender.h"
#include "IVideoCapturing.h"
#ifdef _WIN32
#  include "winerror.h"
#endif
#include "NetProtocol.h"
#include "ConfigHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileSystem.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/HeightMapTexture.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "LoadSave/LoadSaveHandler.h"
#include "LoadSave/DemoRecorder.h"
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
#include "Rendering/PathDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/Models/ModelDrawer.hpp"
#include "Rendering/Models/IModelParser.h"
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
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/TeamHighlight.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/COB/CobEngine.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Units/COB/UnitScriptEngine.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTracker.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sync/SyncedPrimitiveIO.h"
#include "Util.h"
#include "Exceptions.h"
#include "EventHandler.h"
#include "Sound/ISound.h"
#include "Sound/AudioChannel.h"
#include "Sound/Music.h"
#include "FileSystem/SimpleParser.h"
#include "Net/RawPacket.h"
#include "Net/PackPacket.h"
#include "Net/UnpackPacket.h"
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
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Misc/SmoothHeightMesh.h"

#include <boost/cstdint.hpp>

#include "myMath.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"

#undef CreateDirectory

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

extern boost::uint8_t *keys;
extern bool globalQuit;

CGame* game = NULL;


CR_BIND(CGame, (std::string(""), std::string(""), NULL));

CR_REG_METADATA(CGame,(
//	CR_MEMBER(drawMode),
	CR_MEMBER(oldframenum),
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
//	CR_MEMBER(lastCpuUsageTime),
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


CGame::CGame(std::string mapname, std::string modName, ILoadSaveHandler *saveFile):
	gameDrawMode(gameNotDrawing),
	defsParser(NULL),
	oldframenum(0),
	fps(0),
	thisFps(0),
	lastSimFrame(-1),

	totalGameTime(0),

	hideInterface(false),

	gameOver(false),

	noSpectatorChat(false),

	skipping(false),
	playing(false),
	chatting(false),
	lastFrameTime(0),

	leastQue(0),
	timeLeft(0.0f),
	consumeSpeed(1.0f),

	saveFile(saveFile)
{
	game = this;
	boost::thread loadThread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

	memset(gameID, 0, sizeof(gameID));

	time(&starttime);
	lastTick = clock();

	for (int a = 0; a < 8; ++a) { camMove[a] = false; }
	for (int a = 0; a < 4; ++a) { camRot[a] = false; }

	windowedEdgeMove   = !!configHandler->Get("WindowedEdgeMove",   1);
	fullscreenEdgeMove = !!configHandler->Get("FullscreenEdgeMove", 1);

	showFPS   = !!configHandler->Get("ShowFPS",   0);
	showClock = !!configHandler->Get("ShowClock", 1);
	showSpeed = !!configHandler->Get("ShowSpeed", 0);

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

	{
		ScopedOnceTimer timer("Loading sounds");

		sound->LoadSoundDefs("gamedata/sounds.lua");
		chatSound = sound->GetSoundId("IncomingChat", false);
	}

	{
		ScopedOnceTimer timer("Camera and mouse");
		camera = new CCamera();
		cam2 = new CCamera();
		mouse = new CMouseHandler();
		camHandler = new CCameraHandler();
	}

	iconHandler = new CIconHandler();

	selectedUnits.Init(playerHandler->ActivePlayers());
	modInfo.Init(modName.c_str());

	if (!sideParser.Load()) {
		throw content_error(sideParser.GetErrorLog());
	}

	LoadDefs();
	LoadSimulation(mapname);
	LoadRendering();
	LoadInterface();
	LoadLua();
	LoadFinalize();

	loadThread.join();

	// sending your playername to the server indicates that you are finished loading
	const CPlayer* p = playerHandler->Player(gu->myPlayerNum);
	net->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, p->name));

	#ifdef SYNCCHECK
	net->Send(CBaseNetProtocol::Get().SendPathCheckSum(gu->myPlayerNum, pathManager->GetPathCheckSum()));
	#endif

	mouse->ShowMouse();
}

CGame::~CGame()
{
	SafeDelete(guihandler);

	if (videoCapturing->IsCapturing()) {
		videoCapturing->StopCapturing();
	}
	IVideoCapturing::FreeInstance();

#ifdef TRACE_SYNC
	tracefile << "End game\n";
#endif

	CLuaGaia::FreeHandler();
	CLuaRules::FreeHandler();
	LuaOpenGL::Free();
	heightMapTexture.Kill();

	SafeDelete(gameServer);

	eoh->PreDestroy();
	CEngineOutHandler::Destroy();

	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		delete grouphandlers[t];
		grouphandlers[t] = NULL;
	}
	grouphandlers.clear();

	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(resourceBar);

	SafeDelete(featureHandler);
	SafeDelete(featureDrawer);
	SafeDelete(uh);
	SafeDelete(unitDrawer);
	SafeDelete(modelDrawer);
	SafeDelete(projectileDrawer);
	SafeDelete(geometricObjects);
	SafeDelete(ph);
	SafeDelete(minimap);
	SafeDelete(pathManager);
	SafeDelete(groundDecals);
	SafeDelete(ground);
	SafeDelete(smoothGround);
	SafeDelete(luaInputReceiver);
	SafeDelete(inMapDrawer);
	SafeDelete(net);
	SafeDelete(radarhandler);
	SafeDelete(loshandler);
	SafeDelete(mapDamage);
	SafeDelete(qf);
	SafeDelete(tooltip);
	SafeDelete(keyBindings);
	SafeDelete(keyCodes);
	ISound::Shutdown();
	SafeDelete(selectionKeys);
	SafeDelete(mouse);
	SafeDelete(camHandler);
	SafeDelete(helper);
	SafeDelete(shadowHandler);
	SafeDelete(moveinfo);
	SafeDelete(unitDefHandler);
	SafeDelete(weaponDefHandler);
	SafeDelete(damageArrayHandler);
	SafeDelete(vfsHandler);
	SafeDelete(archiveScanner);
	SafeDelete(modelParser);
	SafeDelete(iconHandler);
	SafeDelete(farTextureHandler);
	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);
	SafeDelete(camera);
	SafeDelete(cam2);
	SafeDelete(infoConsole);
	SafeDelete(consoleHistory);
	SafeDelete(wordCompletion);
	SafeDelete(explGenHandler);
	SafeDelete(saveFile);

	delete const_cast<CMapInfo*>(mapInfo);
	mapInfo = NULL;
	SafeDelete(groundBlockingObjectMap);

	CCategoryHandler::RemoveInstance();
	CColorMap::DeleteColormaps();
}

void CGame::LoadDefs()
{
	ScopedOnceTimer timer("Loading GameData Definitions");
	PrintLoadMsg("Loading GameData Definitions");

	defsParser = new LuaParser("gamedata/defs.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	defsParser->GetTable("Spring");
	defsParser->AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	defsParser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
	defsParser->EndTable();

	// run the parser
	if (!defsParser->Execute()) {
		throw content_error(defsParser->GetErrorLog());
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

void CGame::LoadSimulation(const std::string& mapname)
{
	// simulation components
	helper = new CGameHelper();
	ground = new CGround();

	PrintLoadMsg("Parsing Map Information");

	const_cast<CMapInfo*>(mapInfo)->Load();
	readmap = CReadMap::LoadMap(mapname);
	groundBlockingObjectMap = new CGroundBlockingObjectMap(gs->mapSquares);

	PrintLoadMsg("Calculating smooth height mesh");
	smoothGround = new SmoothHeightMesh(ground, float3::maxxpos, float3::maxzpos, SQUARE_SIZE * 2, SQUARE_SIZE * 40);

	moveinfo = new CMoveInfo();
	qf = new CQuadField();

	damageArrayHandler = new CDamageArrayHandler();
	explGenHandler = new CExplosionGeneratorHandler();

	{
		//! FIXME: these five need to be loaded before featureHandler
		//! (maps with features have their models loaded at startup)
		modelParser = new C3DModelLoader();
		texturehandler3DO = new C3DOTextureHandler;
		texturehandlerS3O = new CS3OTextureHandler;
		farTextureHandler = new CFarTextureHandler();
		featureDrawer = new CFeatureDrawer();
	}

	weaponDefHandler = new CWeaponDefHandler();
	unitDefHandler = new CUnitDefHandler();

	uh = new CUnitHandler();
	ph = new CProjectileHandler();

	featureHandler = new CFeatureHandler();
	featureHandler->LoadFeaturesFromMap(saveFile != NULL);

	mapDamage = IMapDamage::GetMapDamage();
	loshandler = new CLosHandler();
	radarhandler = new CRadarHandler(false);

	pathManager = new CPathManager();

	wind.LoadWind(mapInfo->atmosphere.minWind, mapInfo->atmosphere.maxWind);

	CCobInstance::InitVars(teamHandler->ActiveTeams(), teamHandler->ActiveAllyTeams());
	CEngineOutHandler::Initialize();
}

void CGame::LoadRendering()
{
	// rendering components
	shadowHandler = new CShadowHandler();
	groundDecals = new CGroundDecalHandler();

	readmap->NewGroundDrawer();
	treeDrawer = CBaseTreeDrawer::GetTreeDrawer();
	inMapDrawer = new CInMapDraw();

	geometricObjects = new CGeometricObjects();

	projectileDrawer = new CProjectileDrawer();
	projectileDrawer->LoadWeaponTextures();
	unitDrawer = new CUnitDrawer();
	modelDrawer = IModelDrawer::GetInstance();

	sky = CBaseSky::GetSky();
	water = CBaseWater::GetWater(NULL);

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
		const CSkirmishAIHandler::id_ai_t& ais  = skirmishAIHandler.GetAllSkirmishAIs();
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
	PrintLoadMsg("Loading LuaRules");
	CLuaRules::LoadHandler();

	if (gs->useLuaGaia) {
		PrintLoadMsg("Loading LuaGaia");
		CLuaGaia::LoadHandler();
	}
	{
		PrintLoadMsg("Loading LuaUI");
		CLuaUI::LoadHandler();
	}

	// last in, first served
	luaInputReceiver = new LuaInputReceiver();

	delete defsParser;
	defsParser = NULL;
}

void CGame::LoadFinalize()
{
	PrintLoadMsg("Finalizing...");
	eventHandler.GamePreload();

	lastframe = SDL_GetTicks();
	lastModGameTimeMeasure = lastframe;
	lastUpdate = lastframe;
	lastMoveUpdate = lastframe;
	lastUpdateRaw = lastframe;
	lastCpuUsageTime = gu->gameTime + 10;
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

	activeController = this;
	net->loading = false;
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
	water = CBaseWater::GetWater(water);

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
				vector<string> partials = wordCompletion->Complete(head);
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
	//	keys[k] = false;

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

static std::vector<std::string> _local_strSpaceTokenize(const std::string& text) {

	std::vector<std::string> tokens;

#define SPACE_DELIMS " \t"
	// Skip delimiters at beginning.
	std::string::size_type lastPos = text.find_first_not_of(SPACE_DELIMS, 0);
	// Find first "non-delimiter".
	std::string::size_type pos     = text.find_first_of(SPACE_DELIMS, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(text.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = text.find_first_not_of(SPACE_DELIMS, pos);
		// Find next "non-delimiter"
		pos = text.find_first_of(SPACE_DELIMS, lastPos);
	}
#undef SPACE_DELIMS

	return tokens;
}

// FOR UNSYNCED MESSAGES
bool CGame::ActionPressed(const Action& action,
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
		const int current = configHandler->Get("Shadows", 0);
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
				configHandler->Set("ShadowMapSize", mapsize);
			}
		} else {
			next = (current+1)%3;
		}
		configHandler->Set("Shadows", next);
		logOutput.Print("Set Shadows to %i", next);
		shadowHandler = new CShadowHandler();
	}
	else if (cmd == "water") {
		static char rmodes[5][32] = {"basic", "reflective", "dynamic", "reflective&refractive", "bumpmapped"};
		int next = 0;

		if (!action.extra.empty()) {
			next = std::max(0, atoi(action.extra.c_str()) % 5);
		} else {
			const int current = configHandler->Get("ReflectiveWater", 1);
			next = (std::max(0, current) + 1) % 5;
		}
		configHandler->Set("ReflectiveWater", next);
		logOutput.Print("Set water rendering mode to %i (%s)", next, rmodes[next]);
		water = CBaseWater::GetWater(water);
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
	else if (cmd == "w") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const int playernum = playerHandler->Player(action.extra.substr(0, pos));
			if (playernum >= 0) {
				SendNetChat(action.extra.substr(pos+1), playernum);
			} else {
				logOutput.Print("Player not found: %s", action.extra.substr(0, pos).c_str());
			}
		}
	}
	else if (cmd == "wbynum") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			std::istringstream buf(action.extra.substr(0, pos));
			int playernum;
			buf >> playernum;
			if (playernum >= 0) {
				SendNetChat(action.extra.substr(pos+1), playernum);
			} else {
				logOutput.Print("Playernumber invalid: %i", playernum);
			}
		}
	}
	else if (cmd == "echo") {
		logOutput.Print(action.extra);
	}
	else if (cmd == "set") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.extra.substr(0, pos);
			configHandler->SetString(varName, action.extra.substr(pos+1));
		}
	}
	else if (cmd == "tset") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.extra.substr(0, pos);
			configHandler->SetOverlay(varName, action.extra.substr(pos+1));
		}
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
	else if (cmd == "viewselection") {

		GML_RECMUTEX_LOCK(sel); // ActionPressed

		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (!selUnits.empty()) {
			float3 pos(0.0f, 0.0f, 0.0f);
			CUnitSet::const_iterator it;
			for (it = selUnits.begin(); it != selUnits.end(); ++it) {
				pos += (*it)->midPos;
			}
			pos /= (float)selUnits.size();
			camHandler->GetCurrentController().SetPos(pos);
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
	else if ((cmd == "aikill") || (cmd == "aireload")) {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.extra.size() > 0);
		std::vector<std::string> args = _local_strSpaceTokenize(action.extra);
		size_t skirmishAIId           = 0; // will only be used if !badArgs
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		const std::string actionName  = cmd.substr(2);

		if (hasArgs) {
			bool share = false;
			int teamToKillId         = -1;
			int teamToReceiveUnitsId = -1;

			if (args.size() >= 1) {
				teamToKillId = atoi(args[0].c_str());
			}
			if ((args.size() >= 2) && (actionName == "kill")) {
				teamToReceiveUnitsId = atoi(args[1].c_str());
				share = true;
			}

			CTeam* teamToKill = teamHandler->IsActiveTeam(teamToKillId) ?
			                          teamHandler->Team(teamToKillId) : NULL;
			const CTeam* teamToReceiveUnits = teamHandler->Team(teamToReceiveUnitsId) ?
			                                  teamHandler->Team(teamToReceiveUnitsId) : NULL;

			if (teamToKill == NULL) {
				logOutput.Print("Team to %s: not a valid team number: \"%s\"", actionName.c_str(), args[0].c_str());
				badArgs = true;
			}
			if (share && teamToReceiveUnits == NULL) {
				logOutput.Print("Team to receive units: not a valid team number: \"%s\"", args[1].c_str());
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId).size() == 0)) {
				logOutput.Print("Team to %s: not a Skirmish AI team: %i", actionName.c_str(), teamToKillId);
				badArgs = true;
			} else {
				const CSkirmishAIHandler::ids_t skirmishAIIds = skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId, gu->myPlayerNum);
				if (skirmishAIIds.size() > 0) {
					skirmishAIId = skirmishAIIds[0];
				} else {
					logOutput.Print("Team to %s: not a local Skirmish AI team: %i", actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && skirmishAIHandler.GetSkirmishAI(skirmishAIId)->isLuaAI) {
				logOutput.Print("Team to %s: it is not yet supported to %s Lua AIs", actionName.c_str(), actionName.c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToKillId);
				const bool weAreAIHost  = (skirmishAIHandler.GetSkirmishAI(skirmishAIId)->hostPlayer == gu->myPlayerNum);
				const bool weAreLeader  = (teamToKill->leader == gu->myPlayerNum);
				if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
					logOutput.Print("Team to %s: player %s is not allowed to %s Skirmish AI controlling team %i (try with /cheat)",
							actionName.c_str(), fromPlayer->name.c_str(), actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToKill->isDead) {
				logOutput.Print("Team to %s: is a dead team already: %i", actionName.c_str(), teamToKillId);
				badArgs = true;
			}

			if (!badArgs) {
				if (actionName == "kill") {
					if (share) {
						net->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, teamToReceiveUnitsId, teamToKillId));
						// when the AIs team has no units left,
						// the AI will be destroyed automatically
					} else {
						const SkirmishAIData* sai = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
						const bool isLocalSkirmishAI = (sai->hostPlayer == gu->myPlayerNum);
						if (isLocalSkirmishAI) {
							skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 3 /* = AI killed */);
						}
					}
				} else if (actionName == "reload") {
					net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_RELOADING));
				}

				logOutput.Print("Skirmish AI controlling team %i is beeing %sed ...", teamToKillId, actionName.c_str());
			}
		} else {
			logOutput.Print("/%s: missing mandatory argument \"teamTo%s\"", action.command.c_str(), actionName.c_str());
			badArgs = true;
		}

		if (badArgs) {
			if (actionName == "kill") {
				logOutput.Print("description: "
				                "Kill a Skirmish AI controlling a team. The team itsself will remain alive, "
				                "unless a second argument is given, which specifies an active team "
				                "that will receive all the units of the AI team.");
				logOutput.Print("usage:   /%s teamToKill [teamToReceiveUnits]", action.command.c_str());
			} else if (actionName == "reload") {
				logOutput.Print("description: "
				                "Reload a Skirmish AI controlling a team."
				                "The team itsself will remain alive during the process.");
				logOutput.Print("usage:   /%s teamToReload", action.command.c_str());
			}
		}
	}
	else if (cmd == "aicontrol") {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.extra.size() > 0);
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		std::vector<std::string> args = _local_strSpaceTokenize(action.extra);

		if (hasArgs) {
			int         teamToControlId = -1;
			std::string aiShortName     = "";
			std::string aiVersion       = "";
			std::string aiName          = "";
			std::map<std::string, std::string> aiOptions;

			std::vector<std::string> args = _local_strSpaceTokenize(action.extra);
			if (args.size() >= 1) {
				teamToControlId = atoi(args[0].c_str());
			}
			if (args.size() >= 2) {
				aiShortName = args[1];
			} else {
				logOutput.Print("/%s: missing mandatory argument \"aiShortName\"", action.command.c_str());
			}
			if (args.size() >= 3) {
				aiVersion = args[2];
			}
			if (args.size() >= 4) {
				aiName = args[3];
			}

			CTeam* teamToControl = teamHandler->IsActiveTeam(teamToControlId) ?
			                             teamHandler->Team(teamToControlId) : NULL;

			if (teamToControl == NULL) {
				logOutput.Print("Team to control: not a valid team number: \"%s\"", args[0].c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToControlId);
				const bool weAreLeader  = (teamToControl->leader == gu->myPlayerNum);
				const bool noLeader     = (teamToControl->leader == -1);
				if (!(weAreLeader || singlePlayer || (weAreAllied && (cheating || noLeader)))) {
					logOutput.Print("Team to control: player %s is not allowed to let a Skirmish AI take over control of team %i (try with /cheat)",
							fromPlayer->name.c_str(), teamToControlId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToControl->isDead) {
				logOutput.Print("Team to control: is a dead team: %i", teamToControlId);
				badArgs = true;
			}
			// TODO: FIXME: remove this, if support for multiple Skirmish AIs per team is in place
			if (!badArgs && (skirmishAIHandler.GetSkirmishAIsInTeam(teamToControlId).size() > 0)) {
				logOutput.Print("Team to control: there is already an AI controlling this team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetLocalSkirmishAIInCreation(teamToControlId) != NULL)) {
				logOutput.Print("Team to control: there is already an AI beeing created for team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs) {
				const std::set<std::string>& luaAIImplShortNames = skirmishAIHandler.GetLuaAIImplShortNames();
				if (luaAIImplShortNames.find(aiShortName) != luaAIImplShortNames.end()) {
					logOutput.Print("Team to control: it is currently not supported to initialize Lua AIs mid-game");
					badArgs = true;
				}
			}

			if (!badArgs) {
				SkirmishAIKey aiKey(aiShortName, aiVersion);
				aiKey = aiLibManager->ResolveSkirmishAIKey(aiKey);
				if (aiKey.IsUnspecified()) {
					logOutput.Print("Skirmish AI: not a valid Skirmish AI: %s %s",
							aiShortName.c_str(), aiVersion.c_str());
					badArgs = true;
				} else {
					const CSkirmishAILibraryInfo* aiLibInfo = aiLibManager->GetSkirmishAIInfos().find(aiKey)->second;

					SkirmishAIData aiData;
					aiData.name       = (aiName != "") ? aiName : aiShortName;
					aiData.team       = teamToControlId;
					aiData.hostPlayer = gu->myPlayerNum;
					aiData.shortName  = aiShortName;
					aiData.version    = aiVersion;
					std::map<std::string, std::string>::const_iterator o;
					for (o = aiOptions.begin(); o != aiOptions.end(); ++o) {
						aiData.optionKeys.push_back(o->first);
					}
					aiData.options    = aiOptions;
					aiData.isLuaAI    = aiLibInfo->IsLuaAI();

					skirmishAIHandler.CreateLocalSkirmishAI(aiData);
				}
			}
		} else {
			logOutput.Print("/%s: missing mandatory arguments \"teamToControl\" and \"aiShortName\"", action.command.c_str());
			badArgs = true;
		}

		if (badArgs) {
			logOutput.Print("description: Let a Skirmish AI take over control of a team.");
			logOutput.Print("usage:   /%s teamToControl aiShortName [aiVersion] [name] [options...]", action.command.c_str());
			logOutput.Print("example: /%s 1 RAI 0.601 my_RAI_Friend difficulty=2 aggressiveness=3", action.command.c_str());
		}
	}
	else if (cmd == "ailist") {
		const CSkirmishAIHandler::id_ai_t& ais  = skirmishAIHandler.GetAllSkirmishAIs();
		if (ais.size() > 0) {
			CSkirmishAIHandler::id_ai_t::const_iterator ai;
			logOutput.Print("ID | Team | Local | Lua | Name | (Hosting player name) or (Short name & Version)");
			for (ai = ais.begin(); ai != ais.end(); ++ai) {
				const bool isLocal = (ai->second.hostPlayer == gu->myPlayerNum);
				std::string lastPart;
				if (isLocal) {
					lastPart = "(Key:)  " + ai->second.shortName + " " + ai->second.version;
				} else {
					lastPart = "(Host:) " + playerHandler->Player(gu->myPlayerNum)->name;
				}
				LogObject() << ai->first << " | " <<  ai->second.team << " | " << (isLocal ? "yes" : "no ") << " | " << (ai->second.isLuaAI ? "yes" : "no ") << " | " << ai->second.name << " | " << lastPart;
			}
		} else {
			logOutput.Print("<There are no active Skirmish AIs in this game>");
		}
	}
	else if (cmd == "team"){
		if (gs->cheatEnabled)
		{
			int team=atoi(action.extra.c_str());
			if ((team >= 0) && (team < teamHandler->ActiveTeams())) {
				net->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, team));
			}
		}
	}
	else if (cmd == "spectator"){
		if (!gu->spectating)
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
	}
	else if ((cmd == "specteam") && gu->spectating) {
		const int team = atoi(action.extra.c_str());
		if ((team >= 0) && (team < teamHandler->ActiveTeams())) {
			gu->myTeam = team;
			gu->myAllyTeam = teamHandler->AllyTeam(team);
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
	else if (cmd == "ally" && !gu->spectating) {
		if (action.extra.size() > 0) {
			if (!gameSetup->fixedAllies) {
				std::istringstream is(action.extra);
				int otherAllyTeam = -1;
				is >> otherAllyTeam;
				int state = -1;
				is >> state;
				if (state >= 0 && state < 2 && otherAllyTeam >= 0 && otherAllyTeam != gu->myAllyTeam)
					net->Send(CBaseNetProtocol::Get().SendSetAllied(gu->myPlayerNum, otherAllyTeam, state));
				else
					logOutput.Print("/ally: wrong parameters (usage: /ally <other team> [0|1])");
			}
			else {
				logOutput.Print("No ingame alliances are allowed");
			}
		}
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
		// cycle through the positions
		camHandler->GetCurrentController().SetPos(infoConsole->GetMsgPos());
		camHandler->CameraTransition(0.6f);
	}
	else if (((cmd == "chat")     || (cmd == "chatall") ||
	         (cmd == "chatally") || (cmd == "chatspec")) &&
	         // if chat is bound to enter and we're waiting for user to press enter to start game, ignore.
				  (ks.Key() != SDLK_RETURN || playing || !keys[SDLK_LCTRL] )) {

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
	else if (cmd == "showhealthbars") {
		if (action.extra.empty()) {
			unitDrawer->showHealthBars = !unitDrawer->showHealthBars;
		} else {
			unitDrawer->showHealthBars = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "showrezbars") {
		if (action.extra.empty()) {
			featureDrawer->SetShowRezBars(!featureDrawer->GetShowRezBars());
		} else {
			featureDrawer->SetShowRezBars(!!atoi(action.extra.c_str()));
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
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, newPause));
			lastframe = SDL_GetTicks(); // this required here?
		}
	}
	else if (cmd == "debug") {
		if (globalRendering->drawdebug)
		{
			ProfileDrawer::Disable();
			globalRendering->drawdebug = false;
		}
		else
		{
			ProfileDrawer::Enable();
			globalRendering->drawdebug = true;
		}
	}
	else if (cmd == "nosound") {
		if (sound->Mute()) {
			logOutput.Print("Sound disabled");
		} else {
			logOutput.Print("Sound enabled");
		}
	}
	else if (cmd == "soundchannelenable") {
		std::string channel;
		int enableInt, enable;
		std::istringstream buf(action.extra);
		buf >> channel;
		buf >> enableInt;
		if (enableInt == 0)
			enable = false;
		else
			enable = true;

		if (channel == "UnitReply")
			Channels::UnitReply.Enable(enable);
		else if (channel == "General")
			Channels::General.Enable(enable);
		else if (channel == "Battle")
			Channels::Battle.Enable(enable);
		else if (channel == "UserInterface")
			Channels::UserInterface.Enable(enable);
		else if (channel == "Music")
			Channels::BGMusic.Enable(enable);
	}

	else if (cmd == "createvideo") {
		if (videoCapturing->IsCapturing()) {
			videoCapturing->StopCapturing();
		} else {
			videoCapturing->StartCapturing();
		}
	}
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
#ifdef USE_GML
	else if (cmd == "multithreaddrawground") {
		if (action.extra.empty()) {
			gd->multiThreadDrawGround = !gd->multiThreadDrawGround;
		} else {
			gd->multiThreadDrawGround = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded ground rendering is %s", gd->multiThreadDrawGround?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawgroundshadow") {
		if (action.extra.empty()) {
			gd->multiThreadDrawGroundShadow = !gd->multiThreadDrawGroundShadow;
		} else {
			gd->multiThreadDrawGroundShadow = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded ground shadow rendering is %s", gd->multiThreadDrawGroundShadow?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawunit") {
		if (action.extra.empty()) {
			unitDrawer->multiThreadDrawUnit = !unitDrawer->multiThreadDrawUnit;
		} else {
			unitDrawer->multiThreadDrawUnit = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded unit rendering is %s", unitDrawer->multiThreadDrawUnit?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawunitshadow") {
		if (action.extra.empty()) {
			unitDrawer->multiThreadDrawUnitShadow = !unitDrawer->multiThreadDrawUnitShadow;
		} else {
			unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded unit shadow rendering is %s", unitDrawer->multiThreadDrawUnitShadow?"enabled":"disabled");
	}
	else if (cmd == "multithread" || cmd == "multithreaddraw" || cmd == "multithreadsim") {
		int mtenabled=gd->multiThreadDrawGround + unitDrawer->multiThreadDrawUnit + unitDrawer->multiThreadDrawUnitShadow > 1;
		if (cmd == "multithread" || cmd == "multithreaddraw") {
			if (action.extra.empty()) {
				gd->multiThreadDrawGround = !mtenabled;
				unitDrawer->multiThreadDrawUnit = !mtenabled;
				unitDrawer->multiThreadDrawUnitShadow = !mtenabled;
			} else {
				gd->multiThreadDrawGround = !!atoi(action.extra.c_str());
				unitDrawer->multiThreadDrawUnit = !!atoi(action.extra.c_str());
				unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.extra.c_str());
			}
			if(!gd->multiThreadDrawGround)
				gd->multiThreadDrawGroundShadow=0;
			logOutput.Print("Multithreaded rendering is %s", gd->multiThreadDrawGround?"enabled":"disabled");
		}
#	if GML_ENABLE_SIM
		if (cmd == "multithread" || cmd == "multithreadsim") {
			extern volatile int gmlMultiThreadSim;
			extern volatile int gmlStartSim;
			if (action.extra.empty()) {
				gmlMultiThreadSim = (cmd == "multithread") ? !mtenabled : !gmlMultiThreadSim;
			} else {
				gmlMultiThreadSim = !!atoi(action.extra.c_str());
			}
			gmlStartSim=1;
			logOutput.Print("Simulation threading is %s", gmlMultiThreadSim?"enabled":"disabled");
		}
#	endif
	}
#endif
	else if (cmd == "speedcontrol") {
		if (action.extra.empty()) {
			++speedControl;
			if(speedControl > 2)
				speedControl = -2;
		}
		else {
			speedControl = atoi(action.extra.c_str());
		}
		speedControl = std::max(-2, std::min(speedControl, 2));
		net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, speedControl));
		logOutput.Print("Speed Control: %s%s",
			(speedControl == 0) ? "Default" : ((speedControl == 1 || speedControl == -1) ? "Average CPU" : "Maximum CPU"),
			(speedControl < 0) ? " (server voting disabled)" : "");
		configHandler->Set("SpeedControl", speedControl);
		if (gameServer)
			gameServer->UpdateSpeedControl(speedControl);
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
	else if (cmd == "hardwarecursor") {
		if (action.extra.empty()) {
			mouse->hardwareCursor = !mouse->hardwareCursor;
		} else {
			mouse->hardwareCursor = !!atoi(action.extra.c_str());
		}
		mouse->UpdateHwCursor();
		configHandler->Set("HardwareCursor", (int)mouse->hardwareCursor);
	}
	else if (cmd == "increaseviewradius") {
		gd->IncreaseDetail();
	}
	else if (cmd == "decreaseviewradius") {
		gd->DecreaseDetail();
	}
	else if (cmd == "moretrees") {
		treeDrawer->baseTreeDistance += 0.2f;
		LogObject() << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "lesstrees") {
		treeDrawer->baseTreeDistance -= 0.2f;
		LogObject() << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "moreclouds") {
		sky->cloudDensity*=0.95f;
		LogObject() << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (cmd == "lessclouds") {
		sky->cloudDensity*=1.05f;
		LogObject() << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	// Break up the if/else chain to workaround MSVC compiler limit
	// "fatal error C1061: compiler limit : blocks nested too deeply"
	else { notfound1=true; }
	if (notfound1) { // BEGINN: MSVC limit workaround

	if (cmd == "speedup") {
		float speed = gs->userSpeedFactor;
		if(speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if(fpart < 0.01f || fpart > 0.99f)
				speed = round(speed);
		} else if (speed < 10) {
			speed += 0.5f;
		} else {
			speed += 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
	}
	else if (cmd == "slowdown") {
		float speed = gs->userSpeedFactor;
		if (speed <= 5) {
			speed -= (speed <= 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if(fpart < 0.01f || fpart > 0.99f)
				speed = round(speed);
			if (speed < 0.1f)
				speed = 0.1f;
		} else if (speed <= 10) {
			speed -= 0.5f;
		} else {
			speed -= 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
	}

	else if (cmd == "controlunit") {
		Command c;
		c.id=CMD_STOP;
		c.options=0;
		selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que
		net->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
	}
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
	else if (cmd == "togglelos") {
		gd->ToggleLosTexture();
	}
	else if (cmd == "showheat") {
		if (gs->cheatEnabled) {
			gd->ToggleHeatMapTexture();
		}
	}
	else if (cmd == "sharedialog") {
		if(!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front())==0 && !gu->spectating)
			new CShareBox();
	}
	else if (cmd == "quitmenu") {
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == 0)
			new CQuitBox();
	}
	else if (cmd == "quitforce" || cmd == "quit") {
		logOutput.Print("User exited");
		globalQuit = true;
	}
	else if (cmd == "incguiopacity") {
		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha+0.1f,1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
	else if (cmd == "decguiopacity") {
		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha-0.1f,0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}

	else if (cmd == "screenshot") {
		TakeScreenshot(action.extra);
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
	else if (cmd == "clock") {
		if (action.extra.empty()) {
			showClock = !showClock;
		} else {
			showClock = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowClock", showClock ? 1 : 0);
	}
	else if (cmd == "cross") {
		if (action.extra.empty()) {
			if (mouse->crossSize > 0.0f) {
				mouse->crossSize = -mouse->crossSize;
			} else {
				mouse->crossSize = std::max(1.0f, -mouse->crossSize);
			}
		} else {
			mouse->crossSize = atof(action.extra.c_str());
		}
		configHandler->Set("CrossSize", mouse->crossSize);
	}
	else if (cmd == "fps") {
		if (action.extra.empty()) {
			showFPS = !showFPS;
		} else {
			showFPS = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowFPS", showFPS ? 1 : 0);
	}
	else if (cmd == "speed") {
		if (action.extra.empty()) {
			showSpeed = !showSpeed;
		} else {
			showSpeed = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowSpeed", showSpeed ? 1 : 0);
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
		configHandler->Set("ShowPlayerInfo", (int)playerRoster.GetSortType());
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
		CglFont *newFont = NULL, *newSmallFont = NULL;
		try {
			const int fontSize = configHandler->Get("FontSize", 23);
			const int smallFontSize = configHandler->Get("SmallFontSize", 14);
			const int outlineWidth = configHandler->Get("FontOutlineWidth", 3);
			const float outlineWeight = configHandler->Get("FontOutlineWeight", 25.0f);
			const int smallOutlineWidth = configHandler->Get("SmallFontOutlineWidth", 2);
			const float smallOutlineWeight = configHandler->Get("SmallFontOutlineWeight", 10.0f);

			newFont = CglFont::LoadFont(action.extra, fontSize, outlineWidth, outlineWeight);
			newSmallFont = CglFont::LoadFont(action.extra, smallFontSize, smallOutlineWidth, smallOutlineWeight);
		} catch (std::exception e) {
			if (newFont) delete newFont;
			if (newSmallFont) delete newSmallFont;
			newFont = newSmallFont = NULL;
			logOutput.Print(string("font error: ") + e.what());
		}
		if (newFont != NULL && newSmallFont != NULL) {
			delete font;
			delete smallFont;
			font = newFont;
			smallFont = newSmallFont;
			logOutput.Print("Loaded font: %s\n", action.extra.c_str());
			configHandler->SetString("FontFile", action.extra);
			configHandler->SetString("SmallFontFile", action.extra);
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
			hudDrawer->SetDraw(!hudDrawer->GetDraw());
		} else {
			hudDrawer->SetDraw(!!atoi(action.extra.c_str()));
		}
	}
	else if (cmd == "debugdrawai") {
		if (action.extra.empty()) {
			debugDrawerAI->SetDraw(!debugDrawerAI->GetDraw());
		} else {
			debugDrawerAI->SetDraw(!!atoi(action.extra.c_str()));
		}

		logOutput.Print("SkirmishAI debug drawing %s", (debugDrawerAI->GetDraw()? "enabled": "disabled"));
	}

	else if (cmd == "movewarnings") {
		if (action.extra.empty()) {
			gu->moveWarnings = !gu->moveWarnings;
		} else {
			gu->moveWarnings = !!atoi(action.extra.c_str());
		}

		configHandler->Set("MoveWarnings", gu->moveWarnings? 1: 0);
		logOutput.Print(string("movewarnings ") +
		                (gu->moveWarnings ? "enabled" : "disabled"));
	}
	else if (cmd == "buildwarnings") {
		if (action.extra.empty()) {
			gu->buildWarnings = !gu->buildWarnings;
		} else {
			gu->buildWarnings = !!atoi(action.extra.c_str());
		}

		configHandler->Set("BuildWarnings", gu->buildWarnings? 1: 0);
		logOutput.Print(string("buildwarnings ") +
		                (gu->buildWarnings ? "enabled" : "disabled"));
	}

	else if (cmd == "mapmarks") {
		if (action.extra.empty()) {
			globalRendering->drawMapMarks = !globalRendering->drawMapMarks;
		} else {
			globalRendering->drawMapMarks = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "allmapmarks") {
		if (gs->cheatEnabled) {
			if (action.extra.empty()) {
				inMapDrawer->ToggleAllVisible();
			} else {
				inMapDrawer->SetAllVisible(!!atoi(action.extra.c_str()));
			}
		}
	}
	else if (cmd == "noluadraw") {
		if (action.extra.empty()) {
			inMapDrawer->SetLuaMapDrawingAllowed(!inMapDrawer->GetLuaMapDrawingAllowed());
		} else {
			inMapDrawer->SetLuaMapDrawingAllowed(!!atoi(action.extra.c_str()));
		}
	}

	else if (cmd == "luaui") {
		if (guihandler != NULL) {

			GML_RECMUTEX_LOCK(sim); // ActionPressed

			guihandler->RunLayoutCommand(action.extra);
		}
	}
	else if (cmd == "luamoduictrl") {
		bool modUICtrl;
		if (action.extra.empty()) {
			modUICtrl = !CLuaHandle::GetModUICtrl();
		} else {
			modUICtrl = !!atoi(action.extra.c_str());
		}
		CLuaHandle::SetModUICtrl(modUICtrl);
		configHandler->Set("LuaModUICtrl", modUICtrl ? 1 : 0);
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
			const int value = std::max(1, atoi(action.extra.c_str()));
			ph->SetMaxParticles(value);
			logOutput.Print("Set maximum particles to: %i", value);
		}
	}
	else if (cmd == "maxnanoparticles") {
		if (ph && !action.extra.empty()) {
			const int value = std::max(1, atoi(action.extra.c_str()));
			ph->SetMaxNanoParticles(value);
			logOutput.Print("Set maximum nano-particles to: %i", value);
		}
	}
	else if (cmd == "gathermode") {
		if (guihandler != NULL) {
			bool gatherMode;
			if (action.extra.empty()) {
				gatherMode = !guihandler->GetGatherMode();
			} else {
				gatherMode = !!atoi(action.extra.c_str());
			}
			guihandler->SetGatherMode(gatherMode);
			logOutput.Print("gathermode %s", gatherMode ? "enabled" : "disabled");
		}
	}
	else if (cmd == "pastetext") {
		if (userWriting){
			if (!action.extra.empty()) {
				userInput.insert(writingPos, action.extra);
				writingPos += action.extra.length();
			} else {
				PasteClipboard();
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
	else if (cmd == "disticon") {
		if (!action.extra.empty()) {
			const int iconDist = atoi(action.extra.c_str());
			unitDrawer->SetUnitIconDist((float)iconDist);
			configHandler->Set("UnitIconDist", iconDist);
			logOutput.Print("Set UnitIconDist to %i", iconDist);
		}
	}
	else if (cmd == "distdraw") {
		if (!action.extra.empty()) {
			const int drawDist = atoi(action.extra.c_str());
			unitDrawer->SetUnitDrawDist((float)drawDist);
			configHandler->Set("UnitLodDist", drawDist);
			logOutput.Print("Set UnitLodDist to %i", drawDist);
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
			gd->wireframe  = !gd->wireframe;
			sky->wireframe = gd->wireframe;
		} else {
			gd->wireframe  = !!atoi(action.extra.c_str());
			sky->wireframe = gd->wireframe;
		}
	}
	else if (cmd == "airmesh") {
		if (action.extra.empty()) {
			smoothGround->drawEnabled = !smoothGround->drawEnabled;
		} else {
			smoothGround->drawEnabled = !!atoi(action.extra.c_str());
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
	else if (cmd == "crash" && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}
	else if (cmd == "exception" && gs->cheatEnabled) {
		throw std::runtime_error("Exception test");
	}
	else if (cmd == "divbyzero" && gs->cheatEnabled) {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}
	else if (cmd == "give" && gs->cheatEnabled) {
		if (action.extra.find('@') == string::npos) {
			std::string msg = "give "; //FIXME lazyness
			msg += action.extra;
			float3 p;
			CInputReceiver* ir = NULL;
			if (!hideInterface)
				ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);
			if (ir == minimap)
				p = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			else {
				const float3& pos = camera->pos;
				const float3& dir = mouse->dir;
				const float dist = ground->LineGroundCol(pos, pos + (dir * 30000.0f));
				p = pos + (dir * dist);
			}
			char buf[128];
			SNPRINTF(buf, sizeof(buf), " @%.0f,%.0f,%.0f", p.x, p.y, p.z);
			msg += buf;
			CommandMessage pckt(msg, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}
		else {
			CommandMessage pckt(action, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}
	}
	else if (cmd == "destroy" && gs->cheatEnabled) {
		// kill selected units
		std::stringstream ss;
		ss << "destroy";
		for (CUnitSet::iterator it = selectedUnits.selectedUnits.begin();
				it != selectedUnits.selectedUnits.end();
				++it) {
			ss << " " << (*it)->id;
		}
		CommandMessage pckt(ss.str(), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else if (cmd == "send") {
		CommandMessage pckt(Action(action.extra), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else if (cmd == "savegame"){
		SaveGame("Saves/QuickSave.ssf", true);
	}
	else if (cmd == "save") {// /save [-y ]<savename>
		bool saveoverride = action.extra.find("-y ") == 0;
		std::string savename(action.extra.c_str() + (saveoverride ? 3 : 0));
		savename = "Saves/" + savename + ".ssf";
		SaveGame(savename, saveoverride);
	}
	else if (cmd == "reloadgame") {
		ReloadGame();
	}
	else if (cmd == "debuginfo") {
		if (action.extra == "sound") {
			sound->PrintDebugInfo();
		} else if (action.extra == "profiling") {
			profiler.PrintProfilingInfo();
		}
	}
	else if (cmd == "benchmark-script") {
		CUnitScript::BenchmarkScript(action.extra);
	}
	else if (cmd == "atm" ||
#ifdef DEBUG
			cmd == "desync" ||
#endif
			cmd == "resync" ||
			cmd == "take" ||
			cmd == "luarules") {
		//these are synced commands, forward only
		CommandMessage pckt(action, gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else {
		static std::set<std::string> serverCommands = std::set<std::string>(commands, commands+numCommands);
		if (serverCommands.find(cmd) != serverCommands.end())
		{
			CommandMessage pckt(action, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}

		if (!Console::Instance().ExecuteAction(action))
		{
			if (guihandler != NULL) // maybe a widget is interested?
				guihandler->RunLayoutCommand(action.rawline);
			return false;
		}
	}

	} // END: MSVC limit workaround

	return true;
}


bool CGame::ActionReleased(const Action& action)
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


void SetBoolArg(bool& value, const std::string& str)
{
	if (str.empty()) // toggle
	{
		value = !value;
	}
	else // set
	{
		const int num = atoi(str.c_str());
		value = (num != 0);
	}
}


// FOR SYNCED MESSAGES
void CGame::ActionReceived(const Action& action, int playernum)
{
	if (action.command == "cheat") {
		SetBoolArg(gs->cheatEnabled, action.extra);
		if (gs->cheatEnabled)
			logOutput.Print("Cheating!");
		else
			logOutput.Print("No more cheating");
	}
	else if (action.command == "nohelp") {
		SetBoolArg(gs->noHelperAIs, action.extra);
		selectedUnits.PossibleCommandChange(NULL);
		logOutput.Print("LuaUI control is %s", gs->noHelperAIs ? "disabled" : "enabled");
	}
	else if (action.command == "nospecdraw") {
		bool buf;
		SetBoolArg(buf, action.extra);
		inMapDrawer->SetSpecMapDrawingAllowed(buf);
	}
	else if (action.command == "godmode") {
		if (!gs->cheatEnabled)
			logOutput.Print("godmode requires /cheat");
		else {
			SetBoolArg(gs->godMode, action.extra);
			CLuaUI::UpdateTeams();
			if (gs->godMode) {
				logOutput.Print("God Mode Enabled");
			} else {
				logOutput.Print("God Mode Disabled");
			}
			CPlayer::UpdateControlledTeams();
		}
	}
	else if (action.command == "globallos") {
		if (!gs->cheatEnabled) {
			logOutput.Print("globallos requires /cheat");
		} else {
			SetBoolArg(gs->globalLOS, action.extra);
			if (gs->globalLOS) {
				logOutput.Print("Global LOS Enabled");
			} else {
				logOutput.Print("Global LOS Disabled");
			}
		}
	}
	else if (action.command == "nocost" && gs->cheatEnabled) {
		if (unitDefHandler->ToggleNoCost()) {
			logOutput.Print("Everything is for free!");
		} else {
			logOutput.Print("Everything costs resources again!");
		}
	}
	else if (action.command == "give" && gs->cheatEnabled) {
		std::string s = "give "; //FIXME lazyness
		s += action.extra;

		// .give [amount] <unitName> [team] <@x,y,z>
		vector<string> args = CSimpleParser::Tokenize(s, 0);

		if (args.size() < 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		float3 pos;
		if (sscanf(args[args.size() - 1].c_str(), "@%f,%f,%f", &pos.x, &pos.y, &pos.z) != 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		int amount = 1;
		int team = playerHandler->Player(playernum)->team;

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
			if ((amount < 0) || (amountStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad give amount: %s", amountStr.c_str());
				return;
			}
		}

		if (teamArg >= 0) {
			const string& teamStr = args[teamArg];
			team = atoi(teamStr.c_str());
			if ((team < 0) || (team >= teamHandler->ActiveTeams()) || (teamStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad give team: %s", teamStr.c_str());
				return;
			}
		}

		const string unitName = (amountArg >= 0) ? args[2] : args[1];

		if (unitName == "all") {
			// player entered ".give all"
			int sqSize = (int) streflop::ceil(streflop::sqrt((float) unitDefHandler->numUnitDefs));
			int currentNumUnits = teamHandler->Team(team)->units.size();
			int numRequestedUnits = unitDefHandler->numUnitDefs;

			// make sure team unit-limit not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->MaxUnitsPerTeam()) {
				numRequestedUnits = uh->MaxUnitsPerTeam() - currentNumUnits;
			}

			// make sure square is entirely on the map
			float sqHalfMapSize = sqSize / 2 * 10 * SQUARE_SIZE;
			pos.x = std::max(sqHalfMapSize, std::min(pos.x, float3::maxxpos - sqHalfMapSize - 1));
			pos.z = std::max(sqHalfMapSize, std::min(pos.z, float3::maxzpos - sqHalfMapSize - 1));

			for (int a = 1; a <= numRequestedUnits; ++a) {
				float posx = pos.x + (a % sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float posz = pos.z + (a / sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float3 pos2 = float3(posx, pos.y, posz);
				const UnitDef* ud = &unitDefHandler->unitDefs[a];
				if (ud->valid) {
					const CUnit* unit =
						unitLoader.LoadUnit(ud, pos2, team, false, 0, NULL);
					if (unit) {
						unitLoader.FlattenGround(unit);
					}
				}
			}
		}
		else if (!unitName.empty()) {
			int numRequestedUnits = amount;
			int currentNumUnits = teamHandler->Team(team)->units.size();

			if (currentNumUnits >= uh->MaxUnitsPerTeam()) {
				LogObject() << "Unable to give any more units to team " << team << "(current: " << currentNumUnits << ", max: " << uh->MaxUnits() << ")";
				return;
			}

			// make sure team unit-limit is not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->MaxUnitsPerTeam()) {
				numRequestedUnits = uh->MaxUnitsPerTeam() - currentNumUnits;
			}

			const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(unitName);

			if (unitDef != NULL) {
				int xsize = unitDef->xsize;
				int zsize = unitDef->zsize;
				int squareSize = (int) streflop::ceil(streflop::sqrt((float) numRequestedUnits));
				int total = numRequestedUnits;

				float3 minpos = pos;
				minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
				minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

				for (int z = 0; z < squareSize; ++z) {
					for (int x = 0; x < squareSize && total > 0; ++x) {
						float minposx = minpos.x + x * xsize * SQUARE_SIZE;
						float minposz = minpos.z + z * zsize * SQUARE_SIZE;
						const float3 upos(minposx, minpos.y, minposz);
						const CUnit* unit = unitLoader.LoadUnit(unitDef, upos, team, false, 0, NULL);

						if (unit) {
							unitLoader.FlattenGround(unit);
						}
						--total;
					}
				}

				logOutput.Print("Giving %i %s to team %i", numRequestedUnits, unitName.c_str(), team);
			}
			else {
				if (teamArg < 0) {
					team = -1; // default to world features
				}

				const FeatureDef* featureDef = featureHandler->GetFeatureDef(unitName);
				if (featureDef) {
					int xsize = featureDef->xsize;
					int zsize = featureDef->zsize;
					int squareSize = (int) streflop::ceil(streflop::sqrt((float) numRequestedUnits));
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
							CFeature* feature = new CFeature();
							feature->Initialize(upos, featureDef, 0, 0, team, teamHandler->AllyTeam(team), "");
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
	else if (action.command == "destroy" && gs->cheatEnabled) {
		std::stringstream ss(action.extra);
		logOutput.Print("Killing units: %s", action.extra.c_str());
		do {
			unsigned id;
			ss >> id;
			if (!ss)
				break;
			if (id >= uh->units.size())
				continue;
			if (uh->units[id] == NULL)
				continue;
			uh->units[id]->KillUnit(false, false, 0);
		} while (true);
	}
	else if (action.command == "nospectatorchat") {
		SetBoolArg(noSpectatorChat, action.extra);
		logOutput.Print("Spectators %s chat", noSpectatorChat ? "can not" : "can");
	}
	else if (action.command == "reloadcob" && gs->cheatEnabled) {
		ReloadCOB(action.extra, playernum);
	}
	else if (action.command == "devlua" && gs->cheatEnabled) {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, action.extra);
		CLuaHandle::SetDevMode(devMode);
		if (devMode) {
			logOutput.Print("Lua devmode enabled, this can cause desyncs");
		} else {
			logOutput.Print("Lua devmode disabled");
		}
	}
	else if (action.command == "editdefs" && gs->cheatEnabled) {
		SetBoolArg(gs->editDefsEnabled, action.extra);
		if (gs->editDefsEnabled)
			logOutput.Print("Definition Editing!");
		else
			logOutput.Print("No definition Editing");
	}
	else if ((action.command == "luarules") && (gs->frameNum > 1)) {
		if ((action.extra == "reload") && (playernum == 0)) {
			if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to reload synced scripts");
			} else {
				CLuaRules::FreeHandler();
				CLuaRules::LoadHandler();
				if (luaRules) {
					logOutput.Print("LuaRules reloaded");
				} else {
					logOutput.Print("LuaRules reload failed");
				}
			}
		}
		else if ((action.extra == "disable") && (playernum == 0)) {
			if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to disable synced scripts");
			} else {
				CLuaRules::FreeHandler();
				logOutput.Print("LuaRules disabled");
			}
		}
		else {
			if (luaRules) luaRules->GotChatMsg(action.extra, playernum);
		}
	}
	else if ((action.command == "luagaia") && (gs->frameNum > 1)) {
		if (gs->useLuaGaia) {
			if ((action.extra == "reload") && (playernum == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to reload synced scripts");
				} else {
					CLuaGaia::FreeHandler();
					CLuaGaia::LoadHandler();
					if (luaGaia) {
						logOutput.Print("LuaGaia reloaded");
					} else {
						logOutput.Print("LuaGaia reload failed");
					}
				}
			}
			else if ((action.extra == "disable") && (playernum == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts");
				} else {
					CLuaGaia::FreeHandler();
					logOutput.Print("LuaGaia disabled");
				}
			}
			else if (luaGaia) {
				luaGaia->GotChatMsg(action.extra, playernum);
			}
			else {
				logOutput.Print("LuaGaia is not enabled");
			}
		}
	}
#ifdef DEBUG
	else if (action.command == "desync" && gs->cheatEnabled) {
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123.0f);
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123);
		ASSERT_SYNCED_PRIMITIVE((short)(gu->myPlayerNum * 123 + 123));
		ASSERT_SYNCED_FLOAT3(float3(gu->myPlayerNum, gu->myPlayerNum, gu->myPlayerNum));

		for (size_t i = uh->MaxUnits() - 1; i >= 0; --i) {
			if (uh->units[i]) {
				if (playernum == gu->myPlayerNum) {
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
#endif // defined DEBUG
	else if (action.command == "atm" && gs->cheatEnabled) {
		int team = playerHandler->Player(playernum)->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
	}
	else if (action.command == "take" && (!playerHandler->Player(playernum)->spectator || gs->cheatEnabled)) {
		const int sendTeam = playerHandler->Player(playernum)->team;
		for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
			if (teamHandler->AlliedTeams(a, sendTeam)) {
				bool hasPlayer = false;
				for (int b = 0; b < playerHandler->ActivePlayers(); ++b) {
					if (playerHandler->Player(b)->active && playerHandler->Player(b)->team==a && !playerHandler->Player(b)->spectator) {
						hasPlayer = true;
						break;
					}
				}
				if (!hasPlayer) {
					teamHandler->Team(a)->GiveEverythingTo(sendTeam);
				}
			}
		}
	}
	else if (action.command == "skip") {
		if (action.extra.find_first_of("start") == 0) {
			std::istringstream buf(action.extra.substr(6));
			int targetframe;
			buf >> targetframe;
			StartSkip(targetframe);
		}
		else if (action.extra == "end") {
			EndSkip();
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, false));
		}
	}
	else if (gs->frameNum > 1) {
		if (luaRules) luaRules->SyncedActionFallback(action.rawline, playernum);
		if (luaGaia)  luaGaia->SyncedActionFallback(action.rawline, playernum);
	}
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
		oldframenum = gs->frameNum;

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
		GameEnd();
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
	}

	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("Water");
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

	if (globalRendering->drawGround) {
		gd->DrawTrees();
	}

	pathDrawer->Draw();

	//! transparent stuff
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	bool noAdvShading = shadowHandler->drawShadows;

	const double plane_below[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	glClipPlane(GL_CLIP_PLANE3, plane_below);
	glEnable(GL_CLIP_PLANE3);

		//! draw cloaked part below surface
		unitDrawer->DrawCloakedUnits(noAdvShading);
		featureDrawer->DrawFadeFeatures(noAdvShading);

	glDisable(GL_CLIP_PLANE3);

	//! draw water
	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("Water");
		if (!water->drawSolid) {
			water->UpdateWater(this);
			water->Draw();
		}
	}

	const double plane_above[4] = {0.0f, 1.0f, 0.0f, 0.0f};
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

	mouse->Draw();

	guihandler->DrawMapStuff(0);

	if (globalRendering->drawMapMarks) {
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
			water->Update();
			sky->Update();
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

		GML_RECMUTEX_LOCK(sim); // Draw

		guihandler->RunLayoutCommand("disable");
		LogObject() << "Type '/luaui reload' in the chat to re-enable LuaUI.\n";
		LogObject() << "===>>>  Please report this error to the forum or mantis with your infolog.txt\n";
	}


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
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);

		//reset fov
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLoadIdentity();
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
			const int seconds = (gs->frameNum / 30);
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
	playing = true;
	GameSetupDrawer::Disable();
	lastTick = clock();
	lastframe = SDL_GetTicks();

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

	if (luaUI)    { luaUI->GameFrame(gs->frameNum); }
	if (luaGaia)  { luaGaia->GameFrame(gs->frameNum); }
	if (luaRules) { luaRules->GameFrame(gs->frameNum); }

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

void CGame::ClientReadNet()
{
	if (gu->gameTime - lastCpuUsageTime >= 1) {
		lastCpuUsageTime = gu->gameTime;
		net->Send(CBaseNetProtocol::Get().SendCPUUsage(profiler.GetPercent("CPU load")));
	}

	boost::shared_ptr<const netcode::RawPacket> packet;

	// compute new timeLeft to "smooth" out SimFrame() calls
	if(!gameServer){
		const unsigned int currentFrame = SDL_GetTicks();

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
		while ((packet = net->Peek(ahead))) {
			if (packet->data[0] == NETMSG_NEWFRAME || packet->data[0] == NETMSG_KEYFRAME)
				++que;
			++ahead;
		}

		if(que < leastQue)
			leastQue = que;
	}
	else
	{
		// make sure ClientReadNet returns at least every 15 game frames
		// so CGame can process keyboard input, and render etc.
		timeLeft = (float)MAX_CONSECUTIVE_SIMFRAMES * gs->userSpeedFactor;
	}

	// always render at least 2FPS (will otherwise be highly unresponsive when catching up after a reconnection)
	unsigned procstarttime = SDL_GetTicks();
	// really process the messages
	while (timeLeft > 0.0f && (SDL_GetTicks() - procstarttime) < 500 && (packet = net->GetData()))
	{
		const unsigned char* inbuf = packet->data;
		const unsigned dataLength = packet->length;
		const unsigned char packetCode = inbuf[0];

		switch (packetCode) {
			case NETMSG_QUIT: {
				const std::string message = (char*)(&inbuf[3]);
				logOutput.Print(message);
				if (!gameOver)
				{
					GameEnd();
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERLEFT: {
				int player = inbuf[1];
				if (player >= playerHandler->ActivePlayers() || player < 0) {
					logOutput.Print("Got invalid player num (%i) in NETMSG_PLAYERLEFT", player);
				} else {
					playerHandler->PlayerLeft(player, inbuf[2]);
					eventHandler.PlayerRemoved(player, (int) inbuf[2]);
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
				unsigned timeToStart = *(unsigned*)(inbuf+1);
				if (timeToStart > 0) {
					GameSetupDrawer::StartCountdown(timeToStart);
				} else {
					StartPlaying();
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_GAMEOVER: {
				GameEnd();
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_SENDPLAYERSTAT: {
				//logOutput.Print("Game over");
			// Warning: using CPlayer::Statistics here may cause endianness problems
			// once net->SendData is endian aware!
				net->Send(CBaseNetProtocol::Get().SendPlayerStat(gu->myPlayerNum, playerHandler->Player(gu->myPlayerNum)->currentStats));
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERSTAT: {
				int player=inbuf[1];
				if(player >= playerHandler->ActivePlayers() || player<0){
					logOutput.Print("Got invalid player num %i in playerstat msg",player);
					break;
				}
				playerHandler->Player(player)->currentStats = *(CPlayer::Statistics*)&inbuf[2];
				if (gameOver) {
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL) {
						record->SetPlayerStats(player, playerHandler->Player(player)->currentStats);
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PAUSE: {
				int player=inbuf[1];
				if(player>=playerHandler->ActivePlayers() || player<0){
					logOutput.Print("Got invalid player num %i in pause msg",player);
				}
				else if (!skipping) {
					gs->paused=!!inbuf[2];
					if(gs->paused){
						logOutput.Print("%s paused the game",playerHandler->Player(player)->name.c_str());
					} else {
						logOutput.Print("%s unpaused the game",playerHandler->Player(player)->name.c_str());
					}
					lastframe = SDL_GetTicks();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_INTERNAL_SPEED: {
				gs->speedFactor = *((float*) &inbuf[1]);
				sound->PitchAdjust(sqrt(gs->speedFactor));
				//	logOutput.Print("Internal speed set to %.2f",gs->speedFactor);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_USER_SPEED: {
				gs->userSpeedFactor = *((float*) &inbuf[2]);

				unsigned char pNum = *(unsigned char*) &inbuf[1];
				const char* pName = (pNum == SERVER_PLAYER)? "server": playerHandler->Player(pNum)->name.c_str();

				logOutput.Print("Speed set to %.1f [%s]", gs->userSpeedFactor, pName);
				AddTraffic(pNum, packetCode, dataLength);
				break;
			}

			case NETMSG_CPU_USAGE: {
				logOutput.Print("Game clients shouldn't get cpu usage msgs?");
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERINFO: {
				int player = inbuf[1];
				if (player >= playerHandler->ActivePlayers() || player < 0) {
					logOutput.Print("Got invalid player num %i in playerinfo msg", player);
				} else {
					playerHandler->Player(player)->cpuUsage = *(float*) &inbuf[2];
					playerHandler->Player(player)->ping = *(boost::uint32_t*) &inbuf[6];
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERNAME: {
				int player = inbuf[2];
				playerHandler->Player(player)->name=(char*)(&inbuf[3]);
				playerHandler->Player(player)->readyToStart=(gameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
				playerHandler->Player(player)->active=true;
				wordCompletion->AddWord(playerHandler->Player(player)->name, false, false, false); // required?
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_CHAT: {
				try {
					ChatMessage msg(packet);

					HandleChatMsg(msg);
					AddTraffic(msg.fromPlayer, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid ChatMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_SYSTEMMSG:{
				string s=(char*)(&inbuf[4]);
				logOutput.Print(s);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_STARTPOS:{
				unsigned player = inbuf[1];
				int team = inbuf[2];
				if ((team >= teamHandler->ActiveTeams()) || (team < 0)) {
					logOutput.Print("Got invalid team num %i in startpos msg",team);
				} else {
					float3 pos(*(float*)&inbuf[4],
					           *(float*)&inbuf[8],
					           *(float*)&inbuf[12]);
					if (!luaRules || luaRules->AllowStartPosition(player, pos)) {
						teamHandler->Team(team)->StartposMessage(pos);
						if (inbuf[3] != 2 && player != SERVER_PLAYER)
						{
							playerHandler->Player(player)->readyToStart = !!inbuf[3];
						}
						if (pos.y != -500) // no marker marker when no pos set yet
						{
							char label[128];
							SNPRINTF(label, sizeof(label), "Start %i", team);
							inMapDrawer->LocalPoint(pos, label, player);
							// FIXME - erase old pos ?
						}
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_RANDSEED: {
				gs->SetRandSeed(*((unsigned int*)&inbuf[1]), true);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_GAMEID: {
				const unsigned char* p = &inbuf[1];
				CDemoRecorder* record = net->GetDemoRecorder();
				if (record != NULL) {
					record->SetGameID(p);
				}
				memcpy(gameID, p, sizeof(gameID));
				logOutput.Print(
				  "GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
				  p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PATH_CHECKSUM: {
				const unsigned char playerNum = inbuf[1];
				const boost::uint32_t playerCheckSum = *(boost::uint32_t*) &inbuf[2];
				const boost::uint32_t localCheckSum = pathManager->GetPathCheckSum();
				const CPlayer* player = playerHandler->Player(playerNum);

				if (playerCheckSum == 0) {
					logOutput.Print(
						"[DESYNC WARNING] path-checksum for player %d (%s) is 0; non-writable cache?",
						playerNum, player->name.c_str()
					);
				} else {
					if (playerCheckSum != localCheckSum) {
						logOutput.Print(
							"[DESYNC WARNING] path-checksum %08x for player %d (%s) does not match local checksum %08x",
							playerCheckSum, playerNum, player->name.c_str(), localCheckSum
						);
					}
				}
			} break;

			case NETMSG_KEYFRAME: {
				int serverframenum = *(int*)(inbuf+1);
				net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				if (gs->frameNum == (serverframenum - 1)) {
				} else {
					// error
					LogObject() << "Error: Keyframe difference: " << gs->frameNum - (serverframenum - 1);
				}
				/* Fall through */
			}
			case NETMSG_NEWFRAME: {
				timeLeft -= 1.0f;
				SimFrame();
				// both NETMSG_SYNCRESPONSE and NETMSG_NEWFRAME are used for ping calculation by server
#ifdef SYNCCHECK
				net->Send(CBaseNetProtocol::Get().SendSyncResponse(gs->frameNum, CSyncChecker::GetChecksum()));
				if ((gs->frameNum & 4095) == 0) {// reset checksum every ~2.5 minute gametime
					CSyncChecker::NewFrame();
				}
#endif
				AddTraffic(-1, packetCode, dataLength);

				if (videoCapturing->IsCapturing()) {
					return;
				}
				break;
			}

			case NETMSG_COMMAND: {
				int player = inbuf[3];
				if ((player >= playerHandler->ActivePlayers()) || (player < 0)) {
					logOutput.Print("Got invalid player num %i in command msg",player);
				} else {
					Command c;
					c.id=*((int*)&inbuf[4]);
					c.options=inbuf[8];
					for(int a = 0; a < ((*((short int*)&inbuf[1])-9)/4); ++a) {
						c.params.push_back(*((float*)&inbuf[9+a*4]));
					}
					selectedUnits.NetOrder(c,player);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SELECT: {
				int player=inbuf[3];
				if ((player >= playerHandler->ActivePlayers()) || (player < 0)) {
					logOutput.Print("Got invalid player num %i in netselect msg",player);
				} else {
					vector<int> selected;
					for (int a = 0; a < ((*((short int*)&inbuf[1])-4)/2); ++a) {
						int unitid=*((short int*)&inbuf[4+a*2]);
						if(unitid < 0 || static_cast<size_t>(unitid) >= uh->MaxUnits()){
							logOutput.Print("Got invalid unitid %i in netselect msg",unitid);
							break;
						}
						if ((uh->units[unitid] &&
						    (uh->units[unitid]->team == playerHandler->Player(player)->team)) ||
						    gs->godMode) {
							selected.push_back(unitid);
						}
					}
					selectedUnits.NetSelect(selected, player);
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_AICOMMAND: {
				const int player = inbuf[3];
				if (player >= playerHandler->ActivePlayers() || player < 0) {
					logOutput.Print("Got invalid player number (%i) in NETMSG_AICOMMAND", player);
					break;
				}

				int unitid = *((short int*) &inbuf[4]);
				if (unitid < 0 || static_cast<size_t>(unitid) >= uh->MaxUnits()) {
					logOutput.Print("Got invalid unitID (%i) in NETMSG_AICOMMAND", unitid);
					break;
				}

				Command c;
				c.id = *((int*) &inbuf[6]);
				c.options = inbuf[10];

				// insert the command parameters
				for (int a = 0; a < ((*((short int*) &inbuf[1]) - 11) / 4); ++a) {
					c.params.push_back(*((float*) &inbuf[11 + a * 4]));
				}

				selectedUnits.AiOrder(unitid, c, player);
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_AICOMMANDS: {
				const int player = inbuf[3];
				if (player >= playerHandler->ActivePlayers() || player < 0) {
					logOutput.Print("Got invalid player number (%i) in NETMSG_AICOMMANDS", player);
					break;
				}

				int u, c;
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
						selectedUnits.AiOrder(unitIDs[u], commands[c], player);
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_AISHARE: {
				const int player = inbuf[3];
				if (player >= playerHandler->ActivePlayers() || player < 0) {
					logOutput.Print("Got invalid player number (%i) in NETMSG_AISHARE", player);
					break;
				}

				// total message length
				const short numBytes = *(short*) &inbuf[1];
				const int fixedLen = (1 + sizeof(short) + 3 + (2 * sizeof(float)));
				const int variableLen = numBytes - fixedLen;
				const int numUnitIDs = variableLen / sizeof(short); // each unitID is two bytes
				const int srcTeam = inbuf[4];
				const int dstTeam = inbuf[5];
				const float metalShare = *(float*) &inbuf[6];
				const float energyShare = *(float*) &inbuf[10];

				if (metalShare > 0.0f) {
					if (!luaRules || luaRules->AllowResourceTransfer(srcTeam, dstTeam, "m", metalShare)) {
						teamHandler->Team(srcTeam)->metal -= metalShare;
						teamHandler->Team(dstTeam)->metal += metalShare;
					}
				}
				if (energyShare > 0.0f) {
					if (!luaRules || luaRules->AllowResourceTransfer(srcTeam, dstTeam, "e", energyShare)) {
						teamHandler->Team(srcTeam)->energy -= energyShare;
						teamHandler->Team(dstTeam)->energy += energyShare;
					}
				}

				for (int i = 0, j = fixedLen;  i < numUnitIDs;  i++, j += sizeof(short)) {
					short int unitID = *(short int*) &inbuf[j];
					CUnit* u = uh->units[unitID];

					// ChangeTeam() handles the AllowUnitTransfer() LuaRule
					if (u && u->team == srcTeam && !u->beingBuilt) {
						u->ChangeTeam(dstTeam, CUnit::ChangeGiven);
					}
				}
				break;
			}

			case NETMSG_LUAMSG: {
				const int player = inbuf[3];
				if ((player < 0) || (player >= playerHandler->ActivePlayers())) {
					logOutput.Print("Got invalid player num %i in LuaMsg", player);
					break;
				}
				try {
					netcode::UnpackPacket unpack(packet, 1);
					boost::uint16_t size;
					unpack >> size;
					if(size != packet->length)
						throw netcode::UnpackPacketException("Invalid size");
					boost::uint8_t playerNum;
					unpack >> playerNum;
					if(player != playerNum)
						throw netcode::UnpackPacketException("Invalid player number");
					boost::uint16_t script;
					unpack >> script;
					boost::uint8_t mode;
					unpack >> mode;
					std::vector<boost::uint8_t> data(size - 7);
					unpack >> data;

					CLuaHandle::HandleLuaMsg(player, script, mode, data);
					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid LuaMsg: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_SHARE: {
				int player = inbuf[1];
				if ((player >= playerHandler->ActivePlayers()) || (player < 0)){
					logOutput.Print("Got invalid player num %i in share msg", player);
					break;
				}
				int teamID1 = playerHandler->Player(player)->team;
				int teamID2 = inbuf[2];
				bool shareUnits = !!inbuf[3];
				CTeam* team1 = teamHandler->Team(teamID1);
				CTeam* team2 = teamHandler->Team(teamID2);
				float metalShare  = std::min(*(float*)&inbuf[4], (float)team1->metal);
				float energyShare = std::min(*(float*)&inbuf[8], (float)team1->energy);

				if (metalShare != 0.0f) {
					metalShare = std::min(metalShare, team1->metal);
					if (!luaRules || luaRules->AllowResourceTransfer(teamID1, teamID2, "m", metalShare)) {
						team1->metal                       -= metalShare;
						team1->metalSent                   += metalShare;
						team1->currentStats->metalSent     += metalShare;
						team2->metal                       += metalShare;
						team2->metalReceived               += metalShare;
						team2->currentStats->metalReceived += metalShare;
					}
				}
				if (energyShare != 0.0f) {
					energyShare = std::min(energyShare, team1->energy);
					if (!luaRules || luaRules->AllowResourceTransfer(teamID1, teamID2, "e", energyShare)) {
						team1->energy                       -= energyShare;
						team1->energySent                   += energyShare;
						team1->currentStats->energySent     += energyShare;
						team2->energy                       += energyShare;
						team2->energyReceived               += energyShare;
						team2->currentStats->energyReceived += energyShare;
					}
				}

				if (shareUnits) {
					vector<int>& netSelUnits = selectedUnits.netSelected[player];
					vector<int>::const_iterator ui;
					for (ui = netSelUnits.begin(); ui != netSelUnits.end(); ++ui){
						CUnit* unit = uh->units[*ui];
						if (unit && unit->team == teamID1 && !unit->beingBuilt) {
							if (!unit->directControl)
								unit->ChangeTeam(teamID2, CUnit::ChangeGiven);
						}
					}
					netSelUnits.clear();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SETSHARE: {
				int player=inbuf[1];
				int team=inbuf[2];
				if ((team >= teamHandler->ActiveTeams()) || (team < 0)) {
					logOutput.Print("Got invalid team num %i in setshare msg",team);
				} else {
					float metalShare=*(float*)&inbuf[3];
					float energyShare=*(float*)&inbuf[7];

					if (!luaRules || luaRules->AllowResourceLevel(team, "m", metalShare)) {
						teamHandler->Team(team)->metalShare = metalShare;
					}
					if (!luaRules || luaRules->AllowResourceLevel(team, "e", energyShare)) {
						teamHandler->Team(team)->energyShare = energyShare;
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_MAPDRAW: {
				inMapDrawer->GotNetMsg(inbuf);
				AddTraffic(inbuf[2], packetCode, dataLength);
				break;
			}
			case NETMSG_TEAM: {
				const int player = (int)inbuf[1];
				const unsigned char action = inbuf[2];
				const int fromTeam = playerHandler->Player(player)->team;

				switch (action)
				{
					case TEAMMSG_GIVEAWAY: {
						const int toTeam                    = inbuf[3];
						const int fromTeam_g                = inbuf[4];
						const int numPlayersInTeam_g        = playerHandler->ActivePlayersInTeam(fromTeam_g).size();
						const size_t numTotAIsInTeam_g      = skirmishAIHandler.GetSkirmishAIsInTeam(fromTeam_g).size();
						const size_t numControllersInTeam_g = numPlayersInTeam_g + numTotAIsInTeam_g;
						const bool isOwnTeam_g              = (fromTeam_g == fromTeam);

						bool giveAwayOk = false;
						if (isOwnTeam_g) {
							// player is giving stuff from his own team
							giveAwayOk = true;
							if (numPlayersInTeam_g == 1) {
								teamHandler->Team(fromTeam_g)->GiveEverythingTo(toTeam);
							} else {
								playerHandler->Player(player)->StartSpectating();
							}
							selectedUnits.ClearNetSelect(player);
						} else {
							// player is giving stuff from one of his AI teams
							if (numPlayersInTeam_g == 0) {
								giveAwayOk = true;
							}
						}
						if (giveAwayOk && (numControllersInTeam_g == 1)) {
							// team has no controller left now
							teamHandler->Team(fromTeam_g)->GiveEverythingTo(toTeam);
							teamHandler->Team(fromTeam_g)->leader = -1;
						}
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_RESIGN: {
						playerHandler->Player(player)->StartSpectating();
						if (player == gu->myPlayerNum) {
							selectedUnits.ClearSelected();
							unitTracker.Disable();
							CLuaUI::UpdateTeams();
						}
						// actualize all teams of which the player is leader
						for (size_t t = 0; t < teamHandler->ActiveTeams(); ++t) {
							CTeam* team = teamHandler->Team(t);
							if (team->leader == player) {
								const std::vector<int> teamPlayers = playerHandler->ActivePlayersInTeam(t);
								const std::vector<size_t> teamAIs  = skirmishAIHandler.GetSkirmishAIsInTeam(t);
								if ((teamPlayers.size() + teamAIs.size()) == 0) {
									// no controllers left in team
									//team.active = false;
									team->leader = -1;
								} else if (teamPlayers.size() == 0) {
									// no human player left in team
									team->leader = skirmishAIHandler.GetSkirmishAI(teamAIs[0])->hostPlayer;
								} else {
									// still human controllers left in team
									team->leader = teamPlayers[0];
								}
							}
						}
						logOutput.Print("Player %i resigned and is now spectating!", player);
						selectedUnits.ClearNetSelect(player);
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						const int newTeam = int(inbuf[3]);
						playerHandler->Player(player)->team      = newTeam;
						playerHandler->Player(player)->spectator = false;
						if (player == gu->myPlayerNum) {
							gu->myTeam = newTeam;
							gu->myAllyTeam = teamHandler->AllyTeam(gu->myTeam);
							gu->spectating           = false;
							gu->spectatingFullView   = false;
							gu->spectatingFullSelect = false;
							selectedUnits.ClearSelected();
							unitTracker.Disable();
							CLuaUI::UpdateTeams();
						}
						if (teamHandler->Team(newTeam)->leader == -1) {
							teamHandler->Team(newTeam)->leader = player;
						}
						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(player);
						break;
					}
					default: {
						logOutput.Print("Unknown action in NETMSG_TEAM (%i) from player %i", action, player);
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_AI_CREATED: {
				// inbuf[1] contains the message size
				const unsigned char playerId = inbuf[2];
				const unsigned skirmishAIId  = *((unsigned int*)&inbuf[3]); // 4 bytes
				const unsigned char aiTeamId = inbuf[7];
				const char* aiName           = (const char*) (&inbuf[8]);
				CTeam* tai                   = teamHandler->Team(aiTeamId);
				const unsigned isLocal       = (playerId == gu->myPlayerNum);

				if (isLocal) {
					const SkirmishAIData& aiData = *(skirmishAIHandler.GetLocalSkirmishAIInCreation(aiTeamId));
					if (skirmishAIHandler.IsActiveSkirmishAI(skirmishAIId)) {
						// we will end up here for AIs defined in the start script
						const SkirmishAIData* curAIData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
						assert((aiData.team == curAIData->team) && (aiData.name == curAIData->name) && (aiData.hostPlayer == curAIData->hostPlayer));
					} else {
						// we will end up here for local AIs defined mid-game,
						// eg. with /aicontrol
						skirmishAIHandler.AddSkirmishAI(aiData, skirmishAIId);
						wordCompletion->AddWord(aiData.name + " ", false, false, false);
					}
				} else {
					SkirmishAIData aiData;
					aiData.team       = aiTeamId;
					aiData.name       = aiName;
					aiData.hostPlayer = playerId;
					skirmishAIHandler.AddSkirmishAI(aiData, skirmishAIId);
					wordCompletion->AddWord(aiData.name + " ", false, false, false);
				}

				if (tai->leader == -1) {
					tai->leader = playerId;
				}
				CPlayer::UpdateControlledTeams();
				eventHandler.PlayerChanged(playerId);
				if (isLocal) {
					logOutput.Print("Skirmish AI being created for team %i ...", aiTeamId);
					eoh->CreateSkirmishAI(skirmishAIId);
				}
				break;
			}
			case NETMSG_AI_STATE_CHANGED: {
				const unsigned char playerId     = inbuf[1];
				const unsigned int skirmishAIId  = *((unsigned int*)&inbuf[2]); // 4 bytes
				const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[6];
				SkirmishAIData* aiData           = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
				const ESkirmishAIStatus oldState = aiData->status;
				const unsigned aiTeamId          = aiData->team;
				const bool isLuaAI               = aiData->isLuaAI;
				const unsigned isLocal           = (aiData->hostPlayer == gu->myPlayerNum);
				const size_t numPlayersInAITeam  = playerHandler->ActivePlayersInTeam(aiTeamId).size();
				const size_t numAIsInAITeam      = skirmishAIHandler.GetSkirmishAIsInTeam(aiTeamId).size();
				CTeam* tai                       = teamHandler->Team(aiTeamId);

				aiData->status = newState;

				if (isLocal && !isLuaAI && ((newState == SKIRMAISTATE_DIEING) || (newState == SKIRMAISTATE_RELOADING))) {
					eoh->DestroySkirmishAI(skirmishAIId);
				} else if (newState == SKIRMAISTATE_DEAD) {
					if (oldState == SKIRMAISTATE_RELOADING) {
						if (isLocal) {
							logOutput.Print("Skirmish AI \"%s\" being reloaded for team %i ...", aiData->name.c_str(), aiTeamId);
							eoh->CreateSkirmishAI(skirmishAIId);
						}
					} else {
						const std::string aiInstanceName = aiData->name;
						wordCompletion->RemoveWord(aiData->name + " ");
						skirmishAIHandler.RemoveSkirmishAI(skirmishAIId);
						aiData = NULL; // not valid anymore after RemoveSkirmishAI()
						// this could be done in the above function as well
						if ((numPlayersInAITeam + numAIsInAITeam) == 1) {
							// team has no controller left now
							tai->leader = -1;
						}
						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(playerId);
						logOutput.Print("Skirmish AI \"%s\", which controlled team %i is now dead", aiInstanceName.c_str(), aiTeamId);
					}
				} else if (newState == SKIRMAISTATE_ALIVE) {
					logOutput.Print("Skirmish AI \"%s\" took over control of team %i", aiData->name.c_str(), aiTeamId);
				}
				break;
			}
			case NETMSG_ALLIANCE: {
				const int player = inbuf[1];
				const int whichAllyTeam = inbuf[2];
				const bool allied = static_cast<bool>(inbuf[3]);
				const int fromAllyTeam = teamHandler->AllyTeam(playerHandler->Player(player)->team);
				if (whichAllyTeam < teamHandler->ActiveAllyTeams() && whichAllyTeam >= 0 && fromAllyTeam != whichAllyTeam) {
					// FIXME - need to reset unit allyTeams
					//       - need a call-in for AIs
					teamHandler->SetAlly(fromAllyTeam, whichAllyTeam, allied);

					// inform the players
					std::ostringstream msg;
					if (fromAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: you have " << (allied ? "allied" : "unallied")
							<< " allyteam " << whichAllyTeam << ".";
					} else if (whichAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: allyteam " << whichAllyTeam << " has "
							<< (allied ? "allied" : "unallied") <<  " with you.";
					} else {
						msg << "Alliance: allyteam " << whichAllyTeam << " has "
							<< (allied ? "allied" : "unallied")
							<<  " with allyteam " << fromAllyTeam << ".";
					}
					logOutput.Print(msg.str());

					// stop attacks against former foe
					if (allied) {
						for (std::list<CUnit*>::iterator it = uh->activeUnits.begin();
								it != uh->activeUnits.end();
								++it) {
							if (teamHandler->Ally((*it)->allyteam, whichAllyTeam)) {
								(*it)->StopAttackingAllyTeam(whichAllyTeam);
							}
						}
					}
					eventHandler.TeamChanged(playerHandler->Player(player)->team);
				} else {
					logOutput.Print("Alliance: Player %i sent out wrong allyTeam index in alliance message", player);
				}
				break;
			}
			case NETMSG_CCOMMAND: {
				try {
					CommandMessage msg(packet);

					ActionReceived(msg.action, msg.player);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid CommandMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_DIRECT_CONTROL: {
				const int player = inbuf[1];

				if ((player >= playerHandler->ActivePlayers()) || (player < 0)) {
					logOutput.Print("Invalid player number (%i) in NETMSG_DIRECT_CONTROL", player);
					break;
				}

				CPlayer* sender = playerHandler->Player(player);
				if (sender->spectator || !sender->active) {
					break;
				}

				CUnit* ctrlUnit = (sender->dccs).playerControlledUnit;
				if (ctrlUnit) {
					// player released control
					sender->StopControllingUnit();
				} else {
					// player took control
					if (
						!selectedUnits.netSelected[player].empty() &&
						uh->units[selectedUnits.netSelected[player][0]] != NULL &&
						!uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()
					) {
						CUnit* unit = uh->units[selectedUnits.netSelected[player][0]];

						if (unit->directControl && unit->directControl->myController) {
							if (player == gu->myPlayerNum) {
								logOutput.Print(
									"player %d is already controlling unit %d, try later",
									unit->directControl->myController->playerNum, unit->id
								);
							}
						}
						else if (!luaRules || luaRules->AllowDirectUnitControl(player, unit)) {
							unit->directControl = &sender->myControl;
							(sender->dccs).playerControlledUnit = unit;

							if (player == gu->myPlayerNum) {
								gu->directControl = unit;
								mouse->wasLocked = mouse->locked;
								if (!mouse->locked) {
									mouse->locked = true;
									mouse->HideMouse();
								}
								camHandler->PushMode();
								camHandler->SetCameraMode(0);
								selectedUnits.ClearSelected();
							}
						}
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_DC_UPDATE: {
				int player = inbuf[1];
				if ((player >= playerHandler->ActivePlayers()) || (player < 0)) {
					logOutput.Print("Invalid player number (%i) in NETMSG_DC_UPDATE", player);
					break;
				}

				DirectControlStruct* dc = &playerHandler->Player(player)->myControl;
				DirectControlClientState& dccs = playerHandler->Player(player)->dccs;
				CUnit* unit = dccs.playerControlledUnit;

				dc->forward    = !!(inbuf[2] & (1 << 0));
				dc->back       = !!(inbuf[2] & (1 << 1));
				dc->left       = !!(inbuf[2] & (1 << 2));
				dc->right      = !!(inbuf[2] & (1 << 3));
				dc->mouse1     = !!(inbuf[2] & (1 << 4));
				bool newMouse2 = !!(inbuf[2] & (1 << 5));

				if (!dc->mouse2 && newMouse2 && unit) {
					unit->AttackUnit(0, true);
				}
				dc->mouse2 = newMouse2;

				short int h = *((short int*) &inbuf[3]);
				short int p = *((short int*) &inbuf[5]);
				dc->viewDir = GetVectorFromHAndPExact(h, p);

				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SETPLAYERNUM:
			case NETMSG_ATTEMPTCONNECT: {
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
			default: {
#ifdef SYNCDEBUG
				if (!CSyncDebugger::GetInstance()->ClientReceived(inbuf))
#endif
				{
					logOutput.Print("Unknown net msg received, packet code is %d."
							" A likely cause of this is network instability,"
							" which may happen in a WLAN, for example.",
							packetCode);
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
		}
	}

	return;
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
	for (std::list<CUnit*>::iterator usi = uh->activeUnits.begin(); usi != uh->activeUnits.end(); usi++) {
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



void CGame::GameEnd()
{
	if (!gameOver)
	{
		gameOver=true;
		eventHandler.GameOver();
		new CEndGameBox();
#ifdef    HEADLESS
		profiler.PrintProfilingInfo();
#endif // HEADLESS
		CDemoRecorder* record = net->GetDemoRecorder();
		if (record != NULL) {
			// Write CPlayer::Statistics and CTeam::Statistics to demo
			const int numPlayers = playerHandler->ActivePlayers();

			// TODO: move this to a method in CTeamHandler
			// Figure out who won the game.
			int numTeams = teamHandler->ActiveTeams();
			if (gs->useLuaGaia) {
				--numTeams;
			}
			int winner = -1;
			for (int i = 0; i < numTeams; ++i) {
				if (!teamHandler->Team(i)->isDead) {
					winner = teamHandler->AllyTeam(i);
					break;
				}
			}
			// Finally pass it on to the CDemoRecorder.
			record->SetTime(gs->frameNum / 30, (int)gu->gameTime);
			record->InitializeStats(numPlayers, numTeams, winner);
			for (int i = 0; i < numPlayers; ++i) {
				record->SetPlayerStats(i, playerHandler->Player(i)->currentStats);
			}
			for (int i = 0; i < numTeams; ++i) {
				record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
			}
		}
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
		logOutput.Print("Unknown unit name");
		return;
	}
	const CCobFile* oldScript = GCobFileHandler.GetScriptAddr(udef->scriptPath);
	if (oldScript == NULL) {
		logOutput.Print("Unknown cob script: %s", udef->scriptPath.c_str());
		return;
	}
	CCobFile* newScript = GCobFileHandler.ReloadCobFile(udef->scriptPath);
	if (newScript == NULL) {
		logOutput.Print("Could not load COB script from: %s", udef->scriptPath.c_str());
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


static unsigned char GetLuaColor(const LuaTable& tbl, int channel, unsigned char orig)
{
	const float fOrig = (float)orig / 255.0f;
	float luaVal = tbl.GetFloat(channel, fOrig);
	luaVal = std::max(0.0f, std::min(1.0f, luaVal));
	return (unsigned char)(luaVal * 255.0f);
}


void CGame::ReColorTeams()
{
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		CTeam* team = teamHandler->Team(t);
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

	luaParser.AddInt("myPlayer", gu->myPlayerNum);

	luaParser.AddString("modName",      modInfo.humanName);
	luaParser.AddString("modShortName", modInfo.shortName);
	luaParser.AddString("modVersion",   modInfo.version);

	luaParser.AddString("mapName",      mapInfo->map.name);
	luaParser.AddString("mapHumanName", mapInfo->map.humanName);

	luaParser.AddInt("gameMode", gameSetup->gameMode);

	luaParser.GetTable("teams");
	for(int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		luaParser.GetTable(t); {
			const CTeam* team = teamHandler->Team(t);
			const unsigned char* color = teamHandler->Team(t)->color;
			luaParser.AddInt("allyTeam", teamHandler->AllyTeam(t));
			luaParser.AddBool("gaia",    team->gaia);
			luaParser.AddInt("leader",   team->leader);
			luaParser.AddString("side",  team->side);
			luaParser.GetTable("color"); {
				luaParser.AddFloat(1, float(color[0]) / 255.0f);
				luaParser.AddFloat(2, float(color[1]) / 255.0f);
				luaParser.AddFloat(3, float(color[2]) / 255.0f);
				luaParser.AddFloat(4, float(color[3]) / 255.0f);
			}
			luaParser.EndTable(); // color
		}
		luaParser.EndTable(); // team#
	}
	luaParser.EndTable(); // teams

	luaParser.GetTable("players");
	for(int p = 0; p < playerHandler->ActivePlayers(); ++p) {
		luaParser.GetTable(p); {
			const CPlayer* player = playerHandler->Player(p);
			luaParser.AddString("name",     player->name);
			luaParser.AddInt("team",        player->team);
			luaParser.AddBool("active",     player->active);
			luaParser.AddBool("spectating", player->spectator);
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

	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		LuaTable teamTable = root.SubTable(t);
		if (teamTable.IsValid()) {
			unsigned char* color = teamHandler->Team(t)->color;
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
	if (!gs->paused && (timeNow > (lastFrameTime + (500.0f / gs->speedFactor)))) {
		return true;
	} else {
		return false;
	}
}

void CGame::SaveGame(const std::string& filename, bool overwrite)
{
	if (filesystem.CreateDirectory("Saves")) {
		if (overwrite || filesystem.GetFilesize(filename) == 0) {
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
