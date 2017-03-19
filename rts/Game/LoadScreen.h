/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOAD_SCREEN_H
#define _LOAD_SCREEN_H

#include <string>

#include "GameController.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/OffscreenGLContext.h"
#include "System/Misc/SpringTime.h"
#include "System/Threading/SpringThreading.h"

class CLoadScreen : public CGameController
{
public:
	void SetLoadMessage(const std::string& text, bool replace_lastline = false);

	CLoadScreen(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	~CLoadScreen();

	bool Init(); /// split from ctor; uses GetInstance()
	void Kill(); /// split from dtor

public:
	// singleton can potentially be null, but only when
	// accessed from Game::KillLua where we do not care
	static CLoadScreen* GetInstance() { return singleton; }

	static void CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	static void DeleteInstance();

	bool Draw();
	bool Update();

	void ResizeEvent();

	int KeyReleased(int k) override;
	int KeyPressed(int k, bool isRepeat) override;


private:
	void RandomStartPicture(const std::string& sidePref);
	void LoadStartPicture(const std::string& picture);
	void UnloadStartPicture();

private:
	static CLoadScreen* singleton;

	std::string oldLoadMessages;
	std::string curLoadMessage;

	std::string mapName;
	std::string modName;
	ILoadSaveHandler* saveFile;

	spring::recursive_mutex mutex;
	spring::thread* netHeartbeatThread;
	COffscreenGLThread* gameLoadThread;

	bool mtLoading;
	bool showMessages;

	unsigned int startupTexture;
	float aspectRatio;
	spring_time last_draw;
};


#define loadscreen CLoadScreen::GetInstance()


#endif // _LOAD_SCREEN_H
