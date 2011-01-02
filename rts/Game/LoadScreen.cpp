/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include <vector>
#include <SDL.h>

#include "Rendering/GL/myGL.h"
#include "LoadScreen.h"
#include "Game.h"
#include "GameVersion.h"
#include "PlayerHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/InputReceiver.h"
#include "Map/MapInfo.h"
#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FPUCheck.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/NetProtocol.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Sound/ISound.h"
#include "System/Sound/IMusicChannel.h"


CLoadScreen* CLoadScreen::singleton = NULL;

/******************************************************************************/

CLoadScreen::CLoadScreen(const std::string& _mapName, const std::string& _modName, ILoadSaveHandler* _saveFile) :
	mapName(_mapName),
	modName(_modName),
	saveFile(_saveFile),
	netHeartbeatThread(NULL),
	gameLoadThread(NULL),
	mt_loading(true),
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

	//! Increase hang detection trigger threshold, to prevent false positives during load
	CrashHandler::GameLoading(true);

	mt_loading = configHandler->Get("LoadingMT", true);

	//! Create a Thread that pings the host/server, so it knows that this client is still alive
	netHeartbeatThread = new boost::thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

	game = new CGame(mapName, modName, saveFile);

	//FIXME: remove when LuaLoadScreen was added
	{
		const CTeam* team = teamHandler->Team(gu->myTeam);
		assert(team);
		const std::string mapStartPic(mapInfo->GetStringValue("Startpic"));

		if (mapStartPic.empty())
			RandomStartPicture(team->side);
		else
			LoadStartPicture(mapStartPic);

		const std::string mapStartMusic(mapInfo->GetStringValue("Startmusic"));
		if (!mapStartMusic.empty())
			Channels::BGMusic.Play(mapStartMusic);
	}

	try {
		//! Create the Game Loading Thread
		if (mt_loading)
			gameLoadThread = new COffscreenGLThread( boost::bind(&CGame::LoadGame, game, mapName) );

	} catch (opengl_error& gle) {
		//! Offscreen GL Context creation failed,
		//! fallback to singlethreaded loading.
		logOutput.Print(std::string(gle.what()));
		logOutput.Print("Fallback to singlethreaded loading.");
		mt_loading = false;
	}

	if (!mt_loading) {
		Draw();
		game->LoadGame(mapName);
	}
}


CLoadScreen::~CLoadScreen()
{
	delete gameLoadThread; gameLoadThread = NULL;

	net->loading = false;
	netHeartbeatThread->join();
	delete netHeartbeatThread; netHeartbeatThread = NULL;

	UnloadStartPicture();

	CrashHandler::ClearDrawWDT();
	//! Set hang detection trigger threshold back to normal
	CrashHandler::GameLoading(false);

	if (!gu->globalQuit) {
		//! sending your playername to the server indicates that you are finished loading
		const CPlayer* p = playerHandler->Player(gu->myPlayerNum);
		net->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, p->name));
#ifdef SYNCCHECK
		net->Send(CBaseNetProtocol::Get().SendPathCheckSum(gu->myPlayerNum, pathManager->GetPathCheckSum()));
#endif
		mouse->ShowMouse();

		activeController = game;
	}

	if (activeController == this)
		activeController = NULL;

	singleton = NULL;
}


/******************************************************************************/

void CLoadScreen::CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile)
{
	assert(singleton == NULL);

	singleton = new CLoadScreen(mapName, modName, saveFile);
	// Init() already requires GetInstance() to work.
	singleton->Init();
	if (!singleton->mt_loading) {
		CLoadScreen::DeleteInstance();
	}
}


void CLoadScreen::DeleteInstance()
{
	delete singleton;
	singleton = NULL;
}


/******************************************************************************/

void CLoadScreen::ResizeEvent()
{
	//eventHandler.ViewResize();
}


int CLoadScreen::KeyPressed(unsigned short k, bool isRepeat)
{
	//! try the input receivers
	/*std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv = *ri;
		if (recv && recv->KeyPressed(k, isRepeat)) {
			return 0;
		}
	}*/

	return 0;
}


int CLoadScreen::KeyReleased(unsigned short k)
{
	//! try the input receivers
	/*std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv = *ri;
		if (recv && recv->KeyReleased(k)) {
			return 0;
		}
	}*/

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

	if (game->finishedLoading) {
		CLoadScreen::DeleteInstance();
	}

	return true;
}


bool CLoadScreen::Draw()
{
	//! Limit the Frames Per Second to not lock a singlethreaded CPU from loading the game
	if (mt_loading) {
		spring_time now = spring_gettime();
		unsigned diff_ms = spring_tomsecs(now - last_draw);
		static unsigned min_frame_time = 40; //! 40ms = 25FPS
		if (diff_ms < min_frame_time) {
			spring_time nap = spring_msecs(min_frame_time - diff_ms);
			spring_sleep(nap);
		}
		last_draw = now;
	}

	//! cause of `curLoadMessage`
	boost::recursive_mutex::scoped_lock lck(mutex);

	ClearScreen();

	float xDiv = 0.0f;
	float yDiv = 0.0f;
	const float ratioComp = globalRendering->aspectRatio / aspectRatio;
	if (fabs(ratioComp - 1.0f) < 0.01f) { //! ~= 1
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

	font->Begin();
		font->SetTextColor(0.5f,0.5f,0.5f,0.9f);
		font->glPrint(0.1f,0.9f,   globalRendering->viewSizeY / 35.0f, FONT_NORM,
			oldLoadMessages);

		font->SetTextColor(0.9f,0.9f,0.9f,0.9f);
		float posy = font->GetTextNumLines(oldLoadMessages) * font->GetLineHeight() * globalRendering->viewSizeY / 35.0f;
		font->glPrint(0.1f,0.9f - posy * globalRendering->pixelY,   globalRendering->viewSizeY / 35.0f, FONT_NORM,
			curLoadMessage);


		font->SetOutlineColor(0.0f,0.0f,0.0f,0.65f);
		font->SetTextColor(1.0f,1.0f,1.0f,1.0f);
#ifdef USE_GML
		font->glFormat(0.5f,0.06f, globalRendering->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"Spring %s (%d threads)", SpringVersion::GetFull().c_str(), gmlThreadCount);
#else
		font->glFormat(0.5f,0.06f, globalRendering->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"Spring %s", SpringVersion::GetFull().c_str());
#endif
		font->glFormat(0.5f,0.02f, globalRendering->viewSizeY / 50.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"This program is distributed under the GNU General Public License, see license.html for more info");
	font->End();

	if (!mt_loading)
		SDL_GL_SwapBuffers();

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLoadScreen::SetLoadMessage(const std::string& text, bool replace_lastline)
{
	boost::recursive_mutex::scoped_lock lck(mutex);

	if (!replace_lastline) {
		if (oldLoadMessages.empty()) {
			oldLoadMessages = curLoadMessage;
		} else {
			oldLoadMessages += "\n" + curLoadMessage;
		}
	}
	curLoadMessage = text;

	logOutput.Print(text);
	logOutput.Flush();

	//! Check the FPU state (needed for synced computations),
	//! some external libraries which get linked during loading might reset those.
	//! Here it is done for the loading thread, for the mainthread it is done in CLoadScreen::Update()
	good_fpu_control_registers(curLoadMessage.c_str());

	if (!mt_loading)
		Draw();
}


/******************************************************************************/

static void AppendStringVec(vector<string>& dst, const vector<string>& src)
{
	for (int i = 0; i < (int)src.size(); i++) {
		dst.push_back(src[i]);
	}
}


static string SelectPicture(const std::string& dir, const std::string& prefix)
{
	std::vector<string> pics;

	AppendStringVec(pics, CFileHandler::FindFiles(dir, prefix + "*"));

	//! add 'allside_' pictures if we don't have a prefix
	if (!prefix.empty()) {
		AppendStringVec(pics, CFileHandler::FindFiles(dir, "allside_*"));
	}

	if (pics.empty()) {
		return "";
	}

	return pics[gu->usRandInt() % pics.size()];
}


void CLoadScreen::RandomStartPicture(const std::string& sidePref)
{
	if (startupTexture)
		return;

	const std::string picDir = "bitmaps/loadpictures/";

	std::string name = "";
	if (!sidePref.empty()) {
		name = SelectPicture(picDir, sidePref + "_");
	}
	if (name.empty()) {
		name = SelectPicture(picDir, "");
	}
	if (name.empty() || (name.rfind(".db") == name.size() - 3)) {
		return; // no valid pictures
	}
	LoadStartPicture(name);
}


void CLoadScreen::LoadStartPicture(const std::string& name)
{
	CBitmap bm;
	if (!bm.Load(name)) {
		throw content_error("Could not load startpicture from file " + name);
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
			newX = newX / ratioComp;
		} else {
			newY = newY * ratioComp;
		}

		bm = bm.CreateRescaled((int) newX, (int) newY);
	}

	startupTexture = bm.CreateTexture(false);
}


void CLoadScreen::UnloadStartPicture()
{
	if (startupTexture) {
		glDeleteTextures(1, &startupTexture);
	}
	startupTexture = 0;
}


/******************************************************************************/
