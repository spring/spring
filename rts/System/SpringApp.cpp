/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Input/InputHandler.h"

#include <functional>
#include <iostream>

#include <SDL.h>
#include <gflags/gflags.h>

#ifdef WIN32
//windows workarrounds
#undef KeyPress
#undef KeyRelease
#endif

#include "Rendering/GL/myGL.h"
#include "System/SpringApp.h"

#include "aGui/Gui.h"
#include "ExternalAI/IAILibraryManager.h"
#include "Game/Benchmark.h"
#include "Game/ClientSetup.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/GameController.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/PreGame.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaVFSDownload.h"
#include "Menu/LuaMenuController.h"
#include "Menu/SelectMenu.h"
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "System/bitops.h"
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/myMath.h"
#include "System/MsgStrings.h"
#include "System/StartScriptGen.h"
#include "System/TimeProfiler.h"
#include "System/UriParser.h"
#include "System/Util.h"
#include "System/Config/ConfigHandler.h"
#include "System/creg/creg_runtime_tests.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"
#include "System/Input/Joystick.h"
#include "System/Log/ConsoleSink.h"
#include "System/Log/ILog.h"
#include "System/Log/DefaultFilter.h"
#include "System/LogOutput.h"
#include "System/Platform/Battery.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Sound/ISound.h"
#include "System/Sync/FPUCheck.h"
#include "System/Threading/ThreadPool.h"

#include "lib/luasocket/src/restrictions.h"

#ifdef WIN32
	#include "System/Platform/Win/WinVersion.h"
#else
	#include <unistd.h>
#endif



CONFIG(unsigned, SetCoreAffinity).defaultValue(0).safemodeValue(1).description("Defines a bitmask indicating which CPU cores the main-thread should use.");
CONFIG(unsigned, SetCoreAffinitySim).defaultValue(0).safemodeValue(1).description("Defines a bitmask indicating which CPU cores the sim-thread should use.");
CONFIG(bool, UseHighResTimer).defaultValue(false).description("On Windows, sets whether Spring will use low- or high-resolution timer functions for tasks like graphical interpolation between game frames.");
CONFIG(int, PathingThreadCount).defaultValue(0).safemodeValue(1).minimumValue(0);

CONFIG(int, FSAALevel).defaultValue(0).minimumValue(0).maximumValue(8).description("If >0 enables FullScreen AntiAliasing.");

CONFIG(std::string, FontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine text.");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine small text.");
CONFIG(int, FontSize).defaultValue(23).description("Sets the font size (in pixels) of the MainMenu and more.");
CONFIG(int, SmallFontSize).defaultValue(14).description("Sets the font size (in pixels) of the engine GUIs and more.");
CONFIG(int, FontOutlineWidth).defaultValue(3).description("Sets the width of the black outline around Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(float, FontOutlineWeight).defaultValue(25.0f).description("Sets the opacity of Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(int, SmallFontOutlineWidth).defaultValue(2).description("see FontOutlineWidth");
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f).description("see FontOutlineWeight");

CONFIG(bool, Fullscreen).defaultValue(true).headlessValue(false).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(int, XResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, XResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See XResolution, just for windowed.");
CONFIG(int, YResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See YResolution, just for windowed.");
CONFIG(int, WindowPosX).defaultValue(32).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowState).defaultValue(CGlobalRendering::WINSTATE_MAXIMIZED);
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(bool, BlockCompositing).defaultValue(false).safemodeValue(true).description("Disables kwin compositing to fix tearing, possible fixes low FPS in windowed mode, too.");

CONFIG(std::string, name).defaultValue(UnnamedPlayerName).description("Sets your name in the game. Since this is overridden by lobbies with your lobby username when playing, it usually only comes up when viewing replays or starting the engine directly for testing purposes.");
CONFIG(std::string, DefaultStartScript).defaultValue("").description("filename of script.txt to use when no command line parameters are specified.");



DEFINE_bool_EX  (sync_version,       "sync-version",       false, "Display program sync version (for online gaming)");
DEFINE_bool     (fullscreen,                               false, "Run in fullscreen mode");
DEFINE_bool     (window,                                   false, "Run in windowed mode");
DEFINE_bool     (minimise,                                 false, "Start in background (minimised)");
DEFINE_bool     (nocolor,                                  false, "Disables colorized stdout");
DEFINE_string   (server,                                   "",    "Set listening IP for server");
DEFINE_bool     (textureatlas,                             false, "Dump each finalized textureatlas in textureatlasN.tga");
DEFINE_int32    (benchmark,                                -1,    "Enable benchmark mode (writes a benchmark.data file). The given number specifies the timespan to test.");
DEFINE_int32    (benchmarkstart,                           -1,    "Benchmark start time in minutes.");

DEFINE_bool_EX  (list_ai_interfaces, "list-ai-interfaces", false, "Dump a list of available AI Interfaces to stdout");
DEFINE_bool_EX  (list_skirmish_ais,  "list-skirmish-ais",  false, "Dump a list of available Skirmish AIs to stdout");
DEFINE_bool_EX  (list_config_vars,   "list-config-vars",   false, "Dump a list of config vars and meta data to stdout");
DEFINE_bool_EX  (list_def_tags,      "list-def-tags",      false, "Dump a list of all unitdef-, weapondef-, ... tags and meta data to stdout");
DEFINE_bool_EX  (list_ceg_classes,   "list-ceg-classes",   false, "Dump a list of available projectile classes to stdout");
DEFINE_bool_EX  (test_creg,          "test-creg",          false, "Test if all CREG classes are completed");

DEFINE_bool     (safemode,                                 false, "Turns off many things that are known to cause problems (i.e. on PC/Mac's with lower-end graphic cards)");

DEFINE_string   (config,                                   "",    "Exclusive configuration file");
DEFINE_bool     (isolation,                                false, "Limit the data-dir (games & maps) scanner to one directory");
DEFINE_string_EX(isolation_dir,      "isolation-dir",      "",    "Specify the isolation-mode data-dir (see --isolation)");
DEFINE_string_EX(write_dir,          "write-dir",          "",    "Specify where Spring writes to.");
DEFINE_string   (game,                                     "",    "Specify the game that will be instantly loaded");
DEFINE_string   (map,                                      "",    "Specify the map that will be instantly loaded");
DEFINE_string   (menu,                                     "",    "Specify a lua menu archive to be used by spring");
DEFINE_string   (name,                                     "",    "Set your player name");
DEFINE_bool     (oldmenu,                                  false, "Start the old menu");



static bool inShutDown = false;



/**
 * Initializes SpringApp variables
 *
 * @param argc argument count
 * @param argv array of argument strings
 */
SpringApp::SpringApp(int argc, char** argv)
{
	// initializes configHandler which we need
	std::string binaryName = argv[0];

	gflags::SetUsageMessage("Usage: " + binaryName + " [options] [path_to_script.txt or demo.sdfz]");
	gflags::SetVersionString(SpringVersion::GetFull());
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	ParseCmdLine(argc, argv);

	spring_clock::PushTickRate(configHandler->GetBool("UseHighResTimer"));
	// set the Spring "epoch" to be whatever value the first
	// call to gettime() returns, should not be 0 (can safely
	// be done before SDL_Init, we are not using SDL_GetTicks
	// as our clock anymore)
	spring_time::setstarttime(spring_time::gettime(true));
}

/**
 * Destroys SpringApp variables
 */
SpringApp::~SpringApp()
{
	spring_clock::PopTickRate();
}

/**
 * @brief Initializes the SpringApp instance
 * @return whether initialization was successful
 */
bool SpringApp::Initialize()
{
	assert(configHandler != nullptr);
	FileSystemInitializer::InitializeLogOutput();

	CLogOutput::LogConfigInfo();
	CLogOutput::LogSystemInfo();

	CMyMath::Init();

	globalRendering = new CGlobalRendering();
	globalRendering->SetFullScreen(configHandler->GetBool("Fullscreen"), FLAGS_window, FLAGS_fullscreen);

#if !(defined(WIN32) || defined(__APPLE__) || defined(HEADLESS))
	// this MUST run before any other X11 call (esp. those by SDL!)
	// we need it to make calls to X11 threadsafe
	if (!XInitThreads()) {
		LOG_L(L_FATAL, "Xlib is not thread safe");
		return false;
	}
#endif

#if defined(WIN32) && defined(__GNUC__)
	// load QTCreator's gdb helper dll; a variant of this should also work on other OSes
	{
		// don't display a dialog box if gdb helpers aren't found
		UINT olderrors = SetErrorMode(SEM_FAILCRITICALERRORS);
		if (LoadLibrary("gdbmacros.dll"))
			LOG_L(L_DEBUG, "QT Creator's gdbmacros.dll loaded");

		SetErrorMode(olderrors);
	}
#endif
	// Initialize crash reporting
	CrashHandler::Install();
	good_fpu_control_registers(__func__);

	// GlobalConfig
	GlobalConfig::Instantiate();


	// Create Window
	if (!InitWindow(("Spring " + SpringVersion::GetSync()).c_str())) {
		SDL_Quit();
		return false;
	}

	// Init OpenGL
	globalRendering->PostInit();
	InitOpenGL();

	// Install Watchdog (must happen after time epoch is set)
	Watchdog::Install();
	Watchdog::RegisterThread(WDT_MAIN, true);

	// ArchiveScanner uses for_mt --> needs thread-count set
	// (employ all available threads, then switch to default)
	ThreadPool::SetMaximumThreadCount();
	FileSystemInitializer::Initialize();
	ThreadPool::SetDefaultThreadCount();

	// Multithreading & Affinity
	Threading::SetThreadName("spring-main"); // set default threadname for pstree
	Threading::SetThreadScheduler();

	mouseInput = IMouseInput::GetInstance();
	input.AddHandler(std::bind(&SpringApp::MainEventHandler, this, std::placeholders::_1));

	// Global structures
	gs = new CGlobalSynced();
	gu = new CGlobalUnsynced();

	// GUIs
	agui::InitGui();
	LoadFonts();
	keyCodes = new CKeyCodes();

	CNamedTextures::Init();
	LuaOpenGL::Init();
	ISound::Initialize();
	InitJoystick();

	// Lua socket restrictions
	luaSocketRestrictions = new CLuaSocketRestrictions();

	battery = new CBattery();
	LuaVFSDownload::Init();

	// Create CGameSetup and CPreGame objects
	Startup();

	return true;
}


/**
 * @return whether window initialization succeeded
 * @param title char* string with window title
 *
 * Initializes the game window
 */
bool SpringApp::InitWindow(const char* title)
{
	// SDL will cause a creation of gpu-driver thread that will clone its name from the starting threads (= this one = mainthread)
	Threading::SetThreadName("gpu-driver");

	// the crash reporter should be catching errors, not SDL
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1)) {
		LOG_L(L_FATAL, "Could not initialize SDL: %s", SDL_GetError());
		return false;
	}

	PrintAvailableResolutions();
	SDL_DisableScreenSaver();

	if (!globalRendering->CreateSDLWindow(title)) {
		LOG_L(L_FATAL, "Failed to set SDL video mode: %s", SDL_GetError());
		return false;
	}

	// Something in SDL_SetVideoMode (OpenGL drivers?) messes with the FPU control word.
	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();

	if (FLAGS_minimise) {
		SDL_HideWindow(globalRendering->window);
	}

	// any other thread spawned from the main-process should be `unknown`
	Threading::SetThreadName("unknown");
	return true;
}


// origin for our coordinates is the bottom left corner
void SpringApp::GetDisplayGeometry()
{
#ifdef HEADLESS
	globalRendering->screenSizeX = 8;
	globalRendering->screenSizeY = 8;
	globalRendering->winSizeX = 8;
	globalRendering->winSizeY = 8;
	globalRendering->winPosX = 0;
	globalRendering->winPosY = 0;
	globalRendering->UpdateViewPortGeometry();
	return;

#else
	SDL_Rect screenSize;
	SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(globalRendering->window), &screenSize);
	globalRendering->screenSizeX = screenSize.w;
	globalRendering->screenSizeY = screenSize.h;

	SDL_GetWindowSize(globalRendering->window, &globalRendering->winSizeX, &globalRendering->winSizeY);
	SDL_GetWindowPosition(globalRendering->window, &globalRendering->winPosX, &globalRendering->winPosY);

	globalRendering->UpdateViewPortGeometry();

	//XXX SDL2 is crap ...
	// Reading window state fails if it is changed via the window manager, like clicking on the titlebar (2013)
	// https://bugzilla.libsdl.org/show_bug.cgi?id=1508 & https://bugzilla.libsdl.org/show_bug.cgi?id=2282
	// happens on linux too!
	const int state = WindowManagerHelper::GetWindowState(globalRendering->window);

	globalRendering->winState = CGlobalRendering::WINSTATE_DEFAULT;
	if (state & SDL_WINDOW_MAXIMIZED) {
		globalRendering->winState = CGlobalRendering::WINSTATE_MAXIMIZED;
	} else
	if (state & SDL_WINDOW_MINIMIZED) {
		globalRendering->winState = CGlobalRendering::WINSTATE_MINIMIZED;
	}
#endif
}


/**
 * Saves position of the window, if we are not in full-screen mode
 */
void SpringApp::SaveWindowPosition()
{
#ifndef HEADLESS
	if (globalRendering->fullScreen)
		return;

	GetDisplayGeometry();
	if (globalRendering->winState == CGlobalRendering::WINSTATE_DEFAULT) {
		configHandler->Set("WindowPosX",  globalRendering->winPosX);
		configHandler->Set("WindowPosY",  globalRendering->winPosY);
		configHandler->Set("WindowState", globalRendering->winState);
		configHandler->Set("XResolutionWindowed", globalRendering->winSizeX);
		configHandler->Set("YResolutionWindowed", globalRendering->winSizeY);
	} else
	if (globalRendering->winState == CGlobalRendering::WINSTATE_MINIMIZED) {
		// don't automatically save minimized states
	} else {
		configHandler->Set("WindowState", globalRendering->winState);
	}
#endif
}


void SpringApp::SetupViewportGeometry()
{
	GetDisplayGeometry();

	globalRendering->SetDualScreenParams();
	globalRendering->UpdateViewPortGeometry();
	globalRendering->UpdatePixelGeometry();

	const int vpx = globalRendering->viewPosX;
	const int vpy = globalRendering->winSizeY - globalRendering->viewSizeY - globalRendering->viewPosY;

	if (agui::gui != nullptr) {
		agui::gui->UpdateScreenGeometry(globalRendering->viewSizeX, globalRendering->viewSizeY, vpx, vpy);
	}

	glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	gluPerspective(45.0f, globalRendering->aspectRatio, 2.8f, CGlobalRendering::MAX_VIEW_RANGE);
}


/**
 * Initializes OpenGL
 */
void SpringApp::InitOpenGL()
{
	globalRendering->UpdateGLConfigs();

	// setup viewport
	SetupViewportGeometry();

	// Initialize some GL states
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	#ifdef GL_ARB_clip_control
	// avoid precision loss with default DR transform
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	#endif

	if (globalRendering->EnableFSAA()) {
		glEnable(GL_MULTISAMPLE);
	} else {
		glDisable(GL_MULTISAMPLE);
	}

	// FFP model lighting
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	// Clear Window
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	globalRendering->SwapBuffers(true);
	globalRendering->LogDisplayMode();
}


void SpringApp::LoadFonts()
{
	// Initialize font
	const std::string fontFile = configHandler->GetString("FontFile");
	const std::string smallFontFile = configHandler->GetString("SmallFontFile");
	const int fontSize = configHandler->GetInt("FontSize");
	const int smallFontSize = configHandler->GetInt("SmallFontSize");
	const int outlineWidth = configHandler->GetInt("FontOutlineWidth");
	const float outlineWeight = configHandler->GetFloat("FontOutlineWeight");
	const int smallOutlineWidth = configHandler->GetInt("SmallFontOutlineWidth");
	const float smallOutlineWeight = configHandler->GetFloat("SmallFontOutlineWeight");

	spring::SafeDelete(font);
	spring::SafeDelete(smallFont);
	font = CglFont::LoadFont(fontFile, fontSize, outlineWidth, outlineWeight);
	smallFont = CglFont::LoadFont(smallFontFile, smallFontSize, smallOutlineWidth, smallOutlineWeight);

	const static std::string installBroken = ", installation of spring broken? did you run make install?";
	if (!font) {
		throw content_error(std::string("Failed to load FontFile: ") + fontFile + installBroken);
	}
	if (!smallFont) {
		throw content_error(std::string("Failed to load SmallFontFile: ") + smallFontFile + installBroken);
	}
}

// initialize basic systems for command line help / output
static void ConsolePrintInitialize(const std::string& configSource, bool safemode)
{
	spring_clock::PushTickRate(false);
	spring_time::setstarttime(spring_time::gettime(true));
	LOG_DISABLE();
	FileSystemInitializer::PreInitializeConfigHandler(configSource, safemode);
	FileSystemInitializer::InitializeLogOutput();
	LOG_ENABLE();
}


/**
 * @return whether commandline parsing was successful
 *
 * Parse command line arguments
 */
void SpringApp::ParseCmdLine(int argc, char* argv[])
{
#ifndef WIN32
	if (!FLAGS_nocolor && (getenv("SPRING_NOCOLOR") == NULL)) {
		// don't colorize, if our output is piped to a diff tool or file
		if (isatty(fileno(stdout)))
			log_console_colorizedOutput(true);
	}
#endif

	if (FLAGS_sync_version) {
		// Note, the missing "Spring " is intentionally to make it compatible with `spring-dedicated --sync-version`
		std::cout << SpringVersion::GetSync() << std::endl;
		exit(EXIT_SUCCESS);
	}

	if (FLAGS_isolation)
		dataDirLocater.SetIsolationMode(true);


	if (!FLAGS_isolation_dir.empty()) {
		dataDirLocater.SetIsolationMode(true);
		dataDirLocater.SetIsolationModeDir(FLAGS_isolation_dir);
	}

	if (!FLAGS_write_dir.empty())
		dataDirLocater.SetWriteDir(FLAGS_write_dir);


	// Interface Documentations in JSON-Format
	if (FLAGS_list_config_vars) {
		ConfigVariable::OutputMetaDataMap();
		exit(EXIT_SUCCESS);
	}
	if (FLAGS_list_def_tags) {
		DefType::OutputTagMap();
		exit(EXIT_SUCCESS);
	}
	if (FLAGS_list_ceg_classes) {
		const int res = CCustomExplosionGenerator::OutputProjectileClassInfo() ? EXIT_SUCCESS : EXIT_FAILURE;
		exit(res);
	}

	// Runtime Tests
	if (FLAGS_test_creg) {
#ifdef USING_CREG
		exit(creg::RuntimeTest() ? EXIT_SUCCESS : EXIT_FAILURE);
#else
		LOG_L(L_ERROR, "Creg is not enabled!\n");
		exit(EXIT_FAILURE); //Do not fail tests
#endif
	}

	// mutually exclusive options that cause spring to quit immediately
	if (FLAGS_list_ai_interfaces) {
		ConsolePrintInitialize(FLAGS_config, FLAGS_safemode);
		IAILibraryManager::OutputAIInterfacesInfo();
		exit(EXIT_SUCCESS);
	}
	else if (FLAGS_list_skirmish_ais) {
		ConsolePrintInitialize(FLAGS_config, FLAGS_safemode);
		IAILibraryManager::OutputSkirmishAIInfo();
		exit(EXIT_SUCCESS);
	}

	//LOG("[%s] command-line args: \"%s\"", __func__, cmdline->GetCmdLine().c_str());
	FileSystemInitializer::PreInitializeConfigHandler(FLAGS_config, FLAGS_safemode);

	if (FLAGS_textureatlas)
		CTextureAtlas::SetDebug(true);

	if (!FLAGS_name.empty())
		configHandler->SetString("name", StringReplace(FLAGS_name, " ", "_"));

	if (FLAGS_benchmark > 0) {
		CBenchmark::enabled = true;
		if (FLAGS_benchmarkstart > 0) {
			CBenchmark::startFrame = FLAGS_benchmarkstart * 60 * GAME_SPEED;
		}
		CBenchmark::endFrame = CBenchmark::startFrame + FLAGS_benchmark * 60 * GAME_SPEED;
	}

	if (argc >= 2) {
		inputFile = argv[1];
	}
}


CGameController* SpringApp::LoadSaveFile(const std::string& saveFile)
{
	const std::string& ext = FileSystem::GetExtension(saveFile);

	if (ext != "ssf" && ext != "slsf")
		throw content_error(std::string("Unknown save extension: ") + ext);

	clientSetup->isHost = true;

	pregame = new CPreGame(clientSetup);
	pregame->LoadSavefile(saveFile, ext == "ssf");
	return pregame;
}


CGameController* SpringApp::LoadDemoFile(const std::string& demoFile)
{
	const std::string& ext = FileSystem::GetExtension(demoFile);

	if (ext != "sdfz")
		throw content_error(std::string("Unknown demo extension: ") + ext);

	clientSetup->isHost = true;
	clientSetup->myPlayerName += " (spec)";

	pregame = new CPreGame(clientSetup);
	pregame->LoadDemo(demoFile);
	return pregame;
}


CGameController* SpringApp::RunScript(const std::string& buf)
{
	try {
		clientSetup.reset(new ClientSetup());
		clientSetup->LoadFromStartScript(buf);
	} catch (const content_error& err) {
		throw content_error(std::string("Invalid script file\n") + err.what());
	}

	if (!clientSetup->demoFile.empty())
		return LoadDemoFile(clientSetup->demoFile);

	if (!clientSetup->saveFile.empty())
		return LoadSaveFile(clientSetup->saveFile);

	// LoadFromStartScript overrides all values so reset cmdline defined ones
	if (!FLAGS_server.empty()) {
		clientSetup->hostIP = FLAGS_server;
		clientSetup->isHost = true;
	}

	clientSetup->SanityCheck();

	pregame = new CPreGame(clientSetup);

	if (clientSetup->isHost)
		pregame->LoadSetupscript(buf);

	return pregame;
}

void SpringApp::StartScript(const std::string& script)
{
	// startscript
	LOG("[%s] Loading StartScript from: %s", __func__, script.c_str());
	CFileHandler fh(script, SPRING_VFS_PWD_ALL);
	if (!fh.FileExists())
		throw content_error("Setup-script does not exist in given location: " + script);

	std::string buf;
	if (!fh.LoadStringData(buf))
		throw content_error("Setup-script cannot be read: " + script);

	activeController = RunScript(buf);
}

void SpringApp::LoadSpringMenu()
{
	const std::string  vfsScript = "defaultstartscript.txt";
	const std::string& cfgScript = configHandler->GetString("DefaultStartScript");

	const std::string& startScript = (cfgScript.empty() && CFileHandler::FileExists(vfsScript, SPRING_VFS_PWD_ALL))? vfsScript: cfgScript;

	// bypass default menu if we have a valid LuaMenu handler
	if (CLuaMenuController::ActivateInstance(""))
		return;

	if (FLAGS_oldmenu || startScript.empty()) {
		// old menu
	#ifdef HEADLESS
		handleerror(nullptr,
			"The headless version of the engine can not be run in interactive mode.\n"
			"Please supply a start-script, save- or demo-file.", "ERROR", MBF_OK|MBF_EXCL);
	#endif
		// not a memory-leak: SelectMenu deletes itself on start
		activeController = new SelectMenu(clientSetup);
	} else {
		// run custom menu from game and map
		StartScript(startScript);
	}
}

/**
 * Initializes instance of GameSetup
 */
void SpringApp::Startup()
{
	// bash input
	const std::string extension = FileSystem::GetExtension(inputFile);

	// note: avoid any .get() leaks between here and GameServer!
	clientSetup.reset(new ClientSetup());

	// create base client-setup
	if (!FLAGS_server.empty()) {
		clientSetup->hostIP = FLAGS_server;
		clientSetup->isHost = true;
	}

	clientSetup->myPlayerName = configHandler->GetString("name");
	clientSetup->SanityCheck();

	luaMenuController = new CLuaMenuController(FLAGS_menu);

	// no argument (either game is given or show selectmenu)
	if (inputFile.empty()) {
		clientSetup->isHost = true;

		if ((!FLAGS_game.empty()) && (!FLAGS_map.empty())) {
			// --game and --map directly specified, try to run them
			activeController = RunScript(StartScriptGen::CreateMinimalSetup(FLAGS_game, FLAGS_map));
			return;
		}

		LoadSpringMenu();
		return;
	}

	// process given argument
	if (inputFile.find("spring://") == 0) {
		// url (syntax: spring://username:password@host:port)
		if (!ParseSpringUri(inputFile, clientSetup->myPlayerName, clientSetup->myPasswd, clientSetup->hostIP, clientSetup->hostPort))
			throw content_error("invalid url specified: " + inputFile);

		clientSetup->isHost = false;
		pregame = new CPreGame(clientSetup);
	} else if (extension == "sdfz") {
		LoadDemoFile(inputFile);
	} else if (extension == "slsf" || extension == "ssf") {
		LoadSaveFile(inputFile);
	} else {
		StartScript(inputFile);
	}
}



void SpringApp::Reload(const std::string script)
{
	LOG("[SpringApp::%s][1]", __func__);

	// get rid of any running worker threads
	ThreadPool::SetThreadCount(0);
	ThreadPool::SetDefaultThreadCount();

	LOG("[SpringApp::%s][2]", __func__);

	if (gameServer != nullptr)
		gameServer->SetReloading(true);

	// Lua shutdown functions need to access 'game' but spring::SafeDelete sets it to NULL.
	// ~CGame also calls this, which does not matter because handlers are gone by then
	game->KillLua(false);

	LOG("[SpringApp::%s][3]", __func__);

	// kill sound here; thread might access readMap which is deleted by ~CGame
	ISound::Shutdown();

	LOG("[SpringApp::%s][4]", __func__);

	spring::SafeDelete(game);
	spring::SafeDelete(pregame);

	LOG("[SpringApp::%s][5]", __func__);

	// no-op if we are not the server
	spring::SafeDelete(gameServer);
	// PreGame allocates clientNet, so we need to delete our old connection
	spring::SafeDelete(clientNet);

	LOG("[SpringApp::%s][6]", __func__);

	LuaVFSDownload::Free();
	spring::SafeDelete(battery);

	LOG("[SpringApp::%s][7]", __func__);

	// note: technically we only need to use RemoveArchive
	FileSystemInitializer::Cleanup(false);
	FileSystemInitializer::Initialize();

	LOG("[SpringApp::%s][8]", __func__);

	CNamedTextures::Kill();
	CNamedTextures::Init();

	LuaOpenGL::Free();
	LuaOpenGL::Init();

	// normally not needed, but would allow switching fonts
	// LoadFonts();

	LOG("[SpringApp::%s][9]", __func__);

	// reload sounds.lua in case we switched to a different game
	ISound::Initialize();

	// make sure all old EventClients are really gone (safety)
	eventHandler.ResetState();

	battery = new CBattery();
	LuaVFSDownload::Init();

	LOG("[SpringApp::%s][10]", __func__);

	gu->ResetState();
	gs->ResetState();

	profiler.ResetState();
	modInfo.ResetState();

	LOG("[SpringApp::%s][11]", __func__);

	// must hold or we would loop forever
	assert(!gu->globalReload);

	luaMenuController->Reset();
	// clean changed configs
	configHandler->Update();

	LOG("[SpringApp::%s][12] #script=%lu", __func__, (unsigned long) script.size());

	if (script.empty()) {
		// if no script, drop back to menu
		LoadSpringMenu();
	} else {
		activeController = RunScript(script);
	}

	LOG("[SpringApp::%s][13]", __func__);
}

/**
 * @return return code of ActiveController::Update
 */
bool SpringApp::Update()
{
	bool retc = true;
	bool swap = true;

	configHandler->Update();

	#if 0
	if (activeController == nullptr)
		return true;
	if (!activeController->Update())
		return false;
	if (!activeController->Draw())
		return true;
	#else
	// sic; Update can set the controller to null
	retc = (        activeController == nullptr || activeController->Update());
	swap = (retc && activeController != nullptr && activeController->Draw());
	#endif

	// always swap by default, seemingly upsets some drivers not to
	globalRendering->SwapBuffers(swap);
	return retc;
}


/**
 * Executes the application
 * (contains main game loop)
 */
int SpringApp::Run()
{
	Threading::Error* thrErr = nullptr;

	// note: exceptions thrown by other threads are *not* caught here
	try {
		if (!Initialize())
			return -1;

		while (!gu->globalQuit) {
			Watchdog::ClearTimer(WDT_MAIN);
			input.PushEvents();

			if (gu->globalReload) {
				// copy; reloadScript is cleared by ResetState
				Reload(gu->reloadScript);
			} else {
				if (!Update()) {
					break;
				}
			}
		}
	} CATCH_SPRING_ERRORS

	// no exception from main, check if some other thread interrupted our regular loop
	// in case one did, ErrorMessageBox will call ShutDown and forcibly exit the process
	if ((thrErr = Threading::GetThreadError()) != nullptr)
		ErrorMessageBox("  [thread] " + thrErr->message, thrErr->caption, thrErr->flags);

	try {
		if (globalRendering != nullptr) {
			SaveWindowPosition();
			ShutDown(true);
		}
	} CATCH_SPRING_ERRORS

	// no exception from main, but a thread might have thrown *during* ShutDown
	if ((thrErr = Threading::GetThreadError()) != nullptr)
		ErrorMessageBox("  [thread] " + thrErr->message, thrErr->caption, thrErr->flags);

	return spring::exitCode;
}



/**
 * Deallocates and shuts down engine
 */
void SpringApp::ShutDown(bool fromRun)
{
	assert(Threading::IsMainThread());

	if (inShutDown) {
		assert(!fromRun);
		return;
	}

	inShutDown = true;

	LOG("[SpringApp::%s][1] fromRun=%d", __func__, fromRun);
	ThreadPool::SetThreadCount(0);
	LOG("[SpringApp::%s][2]", __func__);
	LuaVFSDownload::Free(true);

	// see ::Reload
	if (game != nullptr)
		game->KillLua(false);

	// see ::Reload
	ISound::Shutdown();

	spring::SafeDelete(game);
	spring::SafeDelete(pregame);

	spring::SafeDelete(luaMenuController);

	LOG("[SpringApp::%s][3]", __func__);
	spring::SafeDelete(clientNet);
	spring::SafeDelete(gameServer);
	spring::SafeDelete(gameSetup);

	LOG("[SpringApp::%s][4]", __func__);
	spring::SafeDelete(keyCodes);
	agui::FreeGui();
	spring::SafeDelete(font);
	spring::SafeDelete(smallFont);

	LOG("[SpringApp::%s][5]", __func__);
	CNamedTextures::Kill(true);
	GlobalConfig::Deallocate();

	FreeJoystick();
	IMouseInput::FreeInstance(mouseInput);

	LOG("[SpringApp::%s][6]", __func__);
	WindowManagerHelper::SetIconSurface();
	globalRendering->DestroySDLWindow();

	LOG("[SpringApp::%s][7]", __func__);
	spring::SafeDelete(gs);
	spring::SafeDelete(gu);
	spring::SafeDelete(globalRendering);
	spring::SafeDelete(luaSocketRestrictions);

	// also gets rid of configHandler
	FileSystemInitializer::Cleanup();
	DataDirLocater::FreeInstance();

	LOG("[SpringApp::%s][8]", __func__);
	Watchdog::DeregisterThread(WDT_MAIN);
	Watchdog::Uninstall();
	LOG("[SpringApp::%s][9]", __func__);

	inShutDown = false;
}


bool SpringApp::MainEventHandler(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_WINDOWEVENT: {
			switch (event.window.event) {
				case SDL_WINDOWEVENT_MOVED: {
					SaveWindowPosition();
				} break;
				//case SDL_WINDOWEVENT_RESIZED: //this is event is always preceded by:
				case SDL_WINDOWEVENT_SIZE_CHANGED: {
					Watchdog::ClearTimer(WDT_MAIN, true);
					SaveWindowPosition();
					InitOpenGL();
					activeController->ResizeEvent();
					mouseInput->InstallWndCallback();
				} break;
				case SDL_WINDOWEVENT_MAXIMIZED:
				case SDL_WINDOWEVENT_RESTORED:
				case SDL_WINDOWEVENT_SHOWN: {
					// reactivate sounds and other
					globalRendering->active = true;
					if (ISound::IsInitialized()) {
						sound->Iconified(false);
					}
					if (globalRendering->fullScreen) {
						FBO::GLContextReinit();
					}
				} break;
				case SDL_WINDOWEVENT_MINIMIZED:
				case SDL_WINDOWEVENT_HIDDEN: {
					// deactivate sounds and other
					globalRendering->active = false;
					if (ISound::IsInitialized()) {
						sound->Iconified(true);
					}
					if (globalRendering->fullScreen) {
						FBO::GLContextLost();
					}
				} break;
				case SDL_WINDOWEVENT_FOCUS_GAINED: {
					// update keydown table
					KeyInput::Update(0, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));
				} break;
				case SDL_WINDOWEVENT_FOCUS_LOST: {
					Watchdog::ClearTimer(WDT_MAIN, true);

					// SDL has some bug and does not update modstate on alt+tab/minimize etc.
					//FIXME check if still happens with SDL2 (2013)
					SDL_SetModState((SDL_Keymod)(SDL_GetModState() & (KMOD_NUM | KMOD_CAPS | KMOD_MODE)));

					// release all keyboard keys
					KeyInput::ReleaseAllKeys();

					// simulate mouse release to prevent hung buttons
					for (int i = 1; i <= NUM_BUTTONS; ++i) {
						if (!mouse)
							continue;

						if (!mouse->buttons[i].pressed)
							continue;

						SDL_Event event;
						event.type = event.button.type = SDL_MOUSEBUTTONUP;
						event.button.state = SDL_RELEASED;
						event.button.which = 0;
						event.button.button = i;
						event.button.x = -1;
						event.button.y = -1;
						SDL_PushEvent(&event);
					}

					// unlock mouse
					if (mouse && mouse->locked) {
						mouse->ToggleMiddleClickScroll();
					}

					// and make sure to un-capture mouse
					if (SDL_GetWindowGrab(globalRendering->window))
						SDL_SetWindowGrab(globalRendering->window, SDL_FALSE);

					break;
				}
				case SDL_WINDOWEVENT_CLOSE: {
					gu->globalQuit = true;
					break;
				}
			};
		} break;
		case SDL_QUIT: {
			gu->globalQuit = true;
		} break;
		case SDL_TEXTEDITING: {
			//FIXME don't known when this is called
		} break;
		case SDL_TEXTINPUT: {
			if (activeController) {
				std::string utf8Text = event.text.text;
				activeController->TextInput(utf8Text);
			}
		} break;
		case SDL_KEYDOWN: {
			KeyInput::Update(event.key.keysym.sym, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				activeController->KeyPressed(KeyInput::GetNormalizedKeySymbol(event.key.keysym.sym), event.key.repeat);
			}
		} break;
		case SDL_KEYUP: {
			KeyInput::Update(event.key.keysym.sym, ((keyBindings != NULL)? keyBindings->GetFakeMetaKey(): -1));

			if (activeController) {
				if (activeController->ignoreNextChar) {
					activeController->ignoreNextChar = false;
				}
				activeController->KeyReleased(KeyInput::GetNormalizedKeySymbol(event.key.keysym.sym));
			}
		} break;
	};

	return false;
}
