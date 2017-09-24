/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL.h>
#include <functional>

#include "Rendering/GL/myGL.h"
#include "LoadScreen.h"
#include "Game.h"
#include "GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/InputReceiver.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Lua/LuaIntro.h"
#include "Lua/LuaMenu.h"
#include "Map/MapInfo.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/Log/ILog.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/Threading.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <vector>

CONFIG(int, LoadingMT).defaultValue(0).safemodeValue(0);
CONFIG(bool, ShowLoadMessages).defaultValue(true);

CLoadScreen* CLoadScreen::singleton = nullptr;

/******************************************************************************/

CLoadScreen::CLoadScreen(const std::string& _mapName, const std::string& _modName, ILoadSaveHandler* _saveFile) :
	mapName(_mapName),
	modName(_modName),
	saveFile(_saveFile),
	netHeartbeatThread(nullptr),
	gameLoadThread(nullptr),
	mtLoading(true),
	showMessages(true),
	startupTexture(0),
	aspectRatio(1.0f),
	last_draw(0)
{
}

CLoadScreen::~CLoadScreen()
{
	// Kill() must have been called first, such that the loading
	// thread can not access singleton while its dtor is running
	assert(gameLoadThread == nullptr);

	if (clientNet != nullptr)
		clientNet->KeepUpdating(false);
	if (netHeartbeatThread != nullptr)
		netHeartbeatThread->join();

	spring::SafeDelete(netHeartbeatThread);

	if (!gu->globalQuit) {
		activeController = game;

		if (luaMenu != nullptr)
			luaMenu->ActivateGame();
	}

	if (activeController == this)
		activeController = nullptr;

	if (luaIntro != nullptr)
		luaIntro->Shutdown();

	CLuaIntro::FreeHandler();

	if (!gu->globalQuit) {
		// send our playername to the server to indicate we finished loading
		const CPlayer* p = playerHandler->Player(gu->myPlayerNum);
		clientNet->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, p->name));
#ifdef SYNCCHECK
		clientNet->Send(CBaseNetProtocol::Get().SendPathCheckSum(gu->myPlayerNum, pathManager->GetPathCheckSum()));
#endif
		mouse->ShowMouse();

#if !defined(HEADLESS) && !defined(NO_SOUND)
		// sound is initialized at this point,
		// but EFX support is *not* guaranteed
		if (efx != nullptr) {
			efx->sfxProperties = *(mapInfo->efxprops);
			efx->CommitEffects();
		}
#endif
	}

	UnloadStartPicture();
}


bool CLoadScreen::Init()
{
	activeController = this;

	// hide the cursor until we are ingame
	SDL_ShowCursor(SDL_DISABLE);

	// When calling this function, mod archives have to be loaded
	// and gu->myPlayerNum has to be set.
	skirmishAIHandler.LoadPreGame();

#ifdef HEADLESS
	mtLoading = false;
	showMessages = false;
#else
	const int mtCfg = configHandler->GetInt("LoadingMT");
	// user override
	mtLoading = (mtCfg > 0);
	// runtime detect. disable for intel/mesa drivers, they crash at multithreaded OpenGL (date: Nov. 2011)
	mtLoading |= (mtCfg < 0) && !globalRendering->haveMesa && !globalRendering->haveIntel;
	showMessages = configHandler->GetBool("ShowLoadMessages");
#endif

	// Create a thread during the loading that pings the host/server, so it knows that this client is still alive/loading
	clientNet->KeepUpdating(true);

	netHeartbeatThread = new spring::thread(Threading::CreateNewThread(std::bind(&CNetProtocol::UpdateLoop, clientNet)));
	game = new CGame(mapName, modName, saveFile);

	// new stuff
	CLuaIntro::LoadFreeHandler();

	// old stuff
	if (luaIntro == nullptr) {
		const CTeam* team = teamHandler->Team(gu->myTeam);

		const std::string& mapStartPic = mapInfo->GetStringValue("Startpic");
		const std::string& mapStartMusic = mapInfo->GetStringValue("Startmusic");

		assert(team != nullptr);

		if (mapStartPic.empty()) {
			RandomStartPicture(team->GetSide());
		} else {
			LoadStartPicture(mapStartPic);
		}

		if (!mapStartMusic.empty())
			Channels::BGMusic->StreamPlay(mapStartMusic);
	}

	if (mtLoading) {
		try {
			// create the game-loading thread
			CglFont::threadSafety = true;
			gameLoadThread = new COffscreenGLThread(std::bind(&CGame::LoadGame, game, mapName));
			return true;
		} catch (const opengl_error& gle) {
			spring::SafeDelete(gameLoadThread);
			LOG_L(L_WARNING, "[LoadScreen::%s] offscreen GL context creation failed (error: \"%s\")", __func__, gle.what());

			mtLoading = false;
		}
	}

	assert(!mtLoading);
	LOG("[LoadScreen::%s] single-threaded", __func__);
	game->LoadGame(mapName);
	return false;
}

void CLoadScreen::Kill()
{
	if (gameLoadThread == nullptr)
		return;

	// at this point, the thread running CGame::LoadGame
	// has finished and deregistered itself from WatchDog
	gameLoadThread->Join();
	spring::SafeDelete(gameLoadThread);

	CglFont::threadSafety = false;
}


/******************************************************************************/

void CLoadScreen::CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile)
{
	assert(singleton == nullptr);
	singleton = new CLoadScreen(mapName, modName, saveFile);

	// Init() already requires GetInstance() to work.
	if (singleton->Init())
		return;

	DeleteInstance();
}

void CLoadScreen::DeleteInstance()
{
	if (singleton == nullptr)
		return;

	singleton->Kill();
	spring::SafeDelete(singleton);
}


/******************************************************************************/

void CLoadScreen::ResizeEvent()
{
	if (luaIntro != nullptr)
		luaIntro->ViewResize();
}


int CLoadScreen::KeyPressed(int k, bool isRepeat)
{
	//FIXME add mouse events
	if (luaIntro != nullptr)
		luaIntro->KeyPress(k, isRepeat);

	return 0;
}

int CLoadScreen::KeyReleased(int k)
{
	if (luaIntro != nullptr)
		luaIntro->KeyRelease(k);

	return 0;
}


bool CLoadScreen::Update()
{
	{
		// keep checking this while we are the active controller
		std::lock_guard<spring::recursive_mutex> lck(mutex);
		good_fpu_control_registers(curLoadMessage.c_str());
	}

	if (game->IsFinishedLoading()) {
		CLoadScreen::DeleteInstance();
		return true;
	}

	// without this call the window manager would think the window is unresponsive and thus ask for hard kill
	if (!mtLoading)
		SDL_PollEvent(nullptr);

	CNamedTextures::Update();
	return true;
}


bool CLoadScreen::Draw()
{
	// limit FPS via sleep to not lock a singlethreaded CPU from loading the game
	if (mtLoading) {
		const spring_time now = spring_gettime();
		const unsigned diffTime = spring_tomsecs(now - last_draw);

		constexpr unsigned wantedFPS = 50;
		constexpr unsigned minFrameTime = 1000 / wantedFPS;

		if (diffTime < minFrameTime)
			spring_sleep(spring_msecs(minFrameTime - diffTime));

		last_draw = now;
	}

	// cause of `curLoadMessage`
	std::lock_guard<spring::recursive_mutex> lck(mutex);

	// let LuaMenu keep the lobby connection alive
	if (luaMenu != nullptr)
		luaMenu->Update();

	if (luaIntro != nullptr) {
		luaIntro->Update();
		luaIntro->DrawGenesis();
		ClearScreen();
		luaIntro->DrawLoadScreen();
	} else {
		ClearScreen();

		float xDiv = 0.0f;
		float yDiv = 0.0f;
		const float ratioComp = globalRendering->aspectRatio / aspectRatio;
		if (math::fabs(ratioComp - 1.0f) < 0.01f) { // ~= 1
			// show Load-Screen full screen; nothing to do
		} else if (ratioComp > 1.0f) {
			// show Load-Screen on part of the screens X-Axis only
			xDiv = (1.0f - (1.0f / ratioComp)) * 0.5f;
		} else {
			// show Load-Screen on part of the screens Y-Axis only
			yDiv = (1.0f - ratioComp) * 0.5f;
		}

		// Draw loading screen & print load msg.
		if (startupTexture) {
			glBindTexture(GL_TEXTURE_2D,startupTexture);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f + xDiv, 0.0f + yDiv);
				glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f + xDiv, 1.0f - yDiv);
				glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f - xDiv, 1.0f - yDiv);
				glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f - xDiv, 0.0f + yDiv);
			glEnd();
		}

		if (showMessages) {
			font->Begin();
				font->SetTextColor(0.5f,0.5f,0.5f,0.9f);
				font->glPrint(0.1f,0.9f,   globalRendering->viewSizeY / 35.0f, FONT_NORM,
					oldLoadMessages);

				font->SetTextColor(0.9f,0.9f,0.9f,0.9f);
				float posy = font->GetTextNumLines(oldLoadMessages) * font->GetLineHeight() * globalRendering->viewSizeY / 35.0f;
				font->glPrint(0.1f,0.9f - posy * globalRendering->pixelY,   globalRendering->viewSizeY / 35.0f, FONT_NORM,
					curLoadMessage);
			font->End();
		}
	}

	if (!mtLoading)
		globalRendering->SwapBuffers(true, false);

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLoadScreen::SetLoadMessage(const std::string& text, bool replace_lastline)
{
	Watchdog::ClearTimer(WDT_LOAD);

	std::lock_guard<spring::recursive_mutex> lck(mutex);

	if (!replace_lastline) {
		if (oldLoadMessages.empty()) {
			oldLoadMessages = curLoadMessage;
		} else {
			oldLoadMessages += "\n" + curLoadMessage;
		}
	}
	curLoadMessage = text;

	LOG("%s", text.c_str());
	LOG_CLEANUP();

	if (luaIntro != nullptr)
		luaIntro->LoadProgress(text, replace_lastline);

	// Check the FPU state (needed for synced computations),
	// some external libraries which get linked during loading might reset those.
	// Here it is done for the loading thread, for the mainthread it is done in CLoadScreen::Update()
	good_fpu_control_registers(curLoadMessage.c_str());

	if (!mtLoading) {
		Update();
		Draw();
	}
}


/******************************************************************************/

static string SelectPicture(const std::string& dir, const std::string& prefix)
{
	std::vector<std::string> prefPics = std::move(CFileHandler::FindFiles(dir, prefix + "*"));
	std::vector<std::string> sidePics;

	// add 'allside_' pictures if we don't have a prefix
	if (!prefix.empty()) {
		sidePics = std::move(CFileHandler::FindFiles(dir, "allside_*"));
		prefPics.insert(prefPics.end(), sidePics.begin(), sidePics.end());
	}

	if (prefPics.empty())
		return "";

	return prefPics[guRNG.NextInt(prefPics.size())];
}


void CLoadScreen::RandomStartPicture(const std::string& sidePref)
{
	if (startupTexture)
		return;

	const std::string picDir = "bitmaps/loadpictures/";

	std::string name;

	if (!sidePref.empty())
		name = SelectPicture(picDir, sidePref + "_");

	if (name.empty())
		name = SelectPicture(picDir, "");

	if (name.empty() || (name.rfind(".db") == name.size() - 3))
		return; // no valid pictures

	LoadStartPicture(name);
}


void CLoadScreen::LoadStartPicture(const std::string& name)
{
	CBitmap bm;

	if (!bm.Load(name)) {
		LOG_L(L_WARNING, "[%s] could not load \"%s\" (wrong format?)", __func__, name.c_str());
		return;
	}

	aspectRatio = (float)bm.xsize / bm.ysize;

	if ((bm.xsize > globalRendering->viewSizeX) || (bm.ysize > globalRendering->viewSizeY)) {
		float newX = globalRendering->viewSizeX;
		float newY = globalRendering->viewSizeY;

		// Make smaller but preserve aspect ratio.
		// The resulting resolution will make it fill one axis of the
		// screen, and be smaller or equal to the screen on the other axis.
		const float ratioComp = globalRendering->aspectRatio / aspectRatio;

		if (ratioComp > 1.0f) {
			newX /= ratioComp;
		} else {
			newY *= ratioComp;
		}

		bm = bm.CreateRescaled((int) newX, (int) newY);
	}

	startupTexture = bm.CreateTexture();
}


void CLoadScreen::UnloadStartPicture()
{
	if (startupTexture)
		glDeleteTextures(1, &startupTexture);

	startupTexture = 0;
}


/******************************************************************************/
