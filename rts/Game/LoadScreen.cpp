/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL.h>
#include <boost/thread.hpp>

#include "Rendering/GL/myGL.h"
#include "LoadScreen.h"
#include "Game.h"
#include "GameVersion.h"
#include "GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/InputReceiver.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Lua/LuaIntro.h"
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

CONFIG(int, LoadingMT).defaultValue(-1).safemodeValue(0);
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

void CLoadScreen::Init()
{
	activeController = this;

	//! hide the cursor until we are ingame
	SDL_ShowCursor(SDL_DISABLE);

	//! When calling this function, mod archives have to be loaded
	//! and gu->myPlayerNum has to be set.
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

	//! Create a thread during the loading that pings the host/server, so it knows that this client is still alive/loading
	clientNet->KeepUpdating(true);

	netHeartbeatThread = new boost::thread();
	*netHeartbeatThread = Threading::CreateNewThread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, clientNet));

	game = new CGame(mapName, modName, saveFile);

	// new stuff
	CLuaIntro::LoadFreeHandler();

	// old stuff
	if (LuaIntro == nullptr) {
		const CTeam* team = teamHandler->Team(gu->myTeam);

		const std::string mapStartPic(mapInfo->GetStringValue("Startpic"));
		const std::string mapStartMusic(mapInfo->GetStringValue("Startmusic"));

		assert(team != nullptr);

		if (mapStartPic.empty()) {
			RandomStartPicture(team->GetSide());
		} else {
			LoadStartPicture(mapStartPic);
		}

		if (!mapStartMusic.empty())
			Channels::BGMusic->StreamPlay(mapStartMusic);
	}

	try {
		//! Create the Game Loading Thread
		if (mtLoading) {
			CglFont::threadSafety = true;
			gameLoadThread = new COffscreenGLThread(boost::bind(&CGame::LoadGame, game, mapName, true));
		}

	} catch (const opengl_error& gle) {
		LOG_L(L_WARNING, "Offscreen GL Context creation failed, "
				"falling back to single-threaded loading. The problem was: %s",
				gle.what());
		mtLoading = false;
	}

	if (!mtLoading) {
		LOG("LoadingScreen: single-threaded");
		game->LoadGame(mapName, false);
	}
}


CLoadScreen::~CLoadScreen()
{
	assert(!gameLoadThread); // ensure we stopped

	if (clientNet != nullptr)
		clientNet->KeepUpdating(false);
	if (netHeartbeatThread != nullptr)
		netHeartbeatThread->join();

	SafeDelete(netHeartbeatThread);

	if (!gu->globalQuit)
		activeController = game;

	if (activeController == this)
		activeController = nullptr;

	if (LuaIntro != nullptr) {
		Draw(); // one last frame
		LuaIntro->Shutdown();
	}
	CLuaIntro::FreeHandler();

	if (!gu->globalQuit) {
		//! sending your playername to the server indicates that you are finished loading
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
			*(efx->sfxProperties) = *(mapInfo->efxprops);
			efx->CommitEffects();
		}
#endif
	}

	UnloadStartPicture();

	singleton = nullptr;
}


/******************************************************************************/

void CLoadScreen::CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile)
{
	assert(singleton == nullptr);
	singleton = new CLoadScreen(mapName, modName, saveFile);

	// Init() already requires GetInstance() to work.
	singleton->Init();

	if (!singleton->mtLoading) {
		CLoadScreen::DeleteInstance();
	}
}


void CLoadScreen::DeleteInstance()
{
	if (singleton) {
		singleton->Stop();
	}

	SafeDelete(singleton);
}


/******************************************************************************/

void CLoadScreen::ResizeEvent()
{
	if (LuaIntro != nullptr)
		LuaIntro->ViewResize();
}


int CLoadScreen::KeyPressed(int k, bool isRepeat)
{
	//FIXME add mouse events
	if (LuaIntro != nullptr)
		LuaIntro->KeyPress(k, isRepeat);

	return 0;
}


int CLoadScreen::KeyReleased(int k)
{
	if (LuaIntro != nullptr)
		LuaIntro->KeyRelease(k);

	return 0;
}


bool CLoadScreen::Update()
{
	{
		//! cause of `curLoadMessage`
		boost::recursive_mutex::scoped_lock lck(mutex);
		//! Stuff that needs to be done regularly while loading.
		good_fpu_control_registers(curLoadMessage.c_str());
	}

	if (game->IsFinishedLoading()) {
		CLoadScreen::DeleteInstance();
		return true;
	}

	if (!mtLoading) {
		// without this call the window manager would think the window is unresponsive and thus asks for hard kill
		SDL_PollEvent(nullptr);
	}

	CNamedTextures::Update();
	return true;
}


bool CLoadScreen::Draw()
{
	//! Limit the Frames Per Second to not lock a singlethreaded CPU from loading the game
	if (mtLoading) {
		spring_time now = spring_gettime();
		unsigned diff_ms = spring_tomsecs(now - last_draw);
		static const unsigned wantedFPS = 50;
		static const unsigned min_frame_time = 1000 / wantedFPS;
		if (diff_ms < min_frame_time) {
			spring_time nap = spring_msecs(min_frame_time - diff_ms);
			spring_sleep(nap);
		}
		last_draw = now;
	}

	//! cause of `curLoadMessage`
	boost::recursive_mutex::scoped_lock lck(mutex);

	if (LuaIntro != nullptr) {
		LuaIntro->Update();
		LuaIntro->DrawGenesis();
		ClearScreen();
		LuaIntro->DrawLoadScreen();
	} else {
		ClearScreen();

		float xDiv = 0.0f;
		float yDiv = 0.0f;
		const float ratioComp = globalRendering->aspectRatio / aspectRatio;
		if (math::fabs(ratioComp - 1.0f) < 0.01f) { //! ~= 1
			//! show Load-Screen full screen
			//! nothing to do
		} else if (ratioComp > 1.0f) {
			//! show Load-Screen on part of the screens X-Axis only
			xDiv = (1.0f - (1.0f / ratioComp)) * 0.5f;
		} else {
			//! show Load-Screen on part of the screens Y-Axis only
			yDiv = (1.0f - ratioComp) * 0.5f;
		}

		//! Draw loading screen & print load msg.
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

	// Always render Spring's license notice
	font->Begin();
		font->SetOutlineColor(0.0f,0.0f,0.0f,0.65f);
		font->SetTextColor(1.0f,1.0f,1.0f,1.0f);
		font->glFormat(0.5f,0.06f, globalRendering->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
				"Spring %s", SpringVersion::GetFull().c_str());
		font->glFormat(0.5f,0.02f, globalRendering->viewSizeY / 50.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"This program is distributed under the GNU General Public License, see license.html for more info");
	font->End();

	if (!mtLoading)
		SDL_GL_SwapWindow(globalRendering->window);

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLoadScreen::SetLoadMessage(const std::string& text, bool replace_lastline)
{
	Watchdog::ClearTimer(WDT_LOAD);

	boost::recursive_mutex::scoped_lock lck(mutex);

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

	if (LuaIntro)
		LuaIntro->LoadProgress(text, replace_lastline);

	//! Check the FPU state (needed for synced computations),
	//! some external libraries which get linked during loading might reset those.
	//! Here it is done for the loading thread, for the mainthread it is done in CLoadScreen::Update()
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

	//! add 'allside_' pictures if we don't have a prefix
	if (!prefix.empty()) {
		sidePics = std::move(CFileHandler::FindFiles(dir, "allside_*"));
		prefPics.insert(prefPics.end(), sidePics.begin(), sidePics.end());
	}

	if (prefPics.empty())
		return "";

	return prefPics[gu->RandInt() % prefPics.size()];
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
		LOG_L(L_WARNING, "[%s] could not load \"%s\" (wrong format?)", __FUNCTION__, name.c_str());
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


void CLoadScreen::Stop()
{
	if (gameLoadThread)
	{
		// at this point, the thread running CGame::LoadGame
		// has finished and deregistered itself from WatchDog
		if (mtLoading && gameLoadThread) {
			gameLoadThread->Join();
			CglFont::threadSafety = false;
		}

		SafeDelete(gameLoadThread);
	}
}


/******************************************************************************/
