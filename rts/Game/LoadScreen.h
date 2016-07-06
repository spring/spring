/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOAD_SCREEN_H
#define _LOAD_SCREEN_H

#include <string>
#include <boost/thread/recursive_mutex.hpp>

#include "GameController.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/OffscreenGLContext.h"
#include "System/Misc/SpringTime.h"

namespace boost {
	class thread;
}

class CLoadScreen : public CGameController
{
public:
	void SetLoadMessage(const std::string& text, bool replace_lastline = false);

	CLoadScreen(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CLoadScreen();

	/// Splitt off from the ctor, casue this already uses GetInstance().
	void Init();

public:
	static CLoadScreen* GetInstance() {
		assert(singleton);
		return singleton;
	}
	static void CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	static void DeleteInstance();

	bool Draw();
	bool Update();

	void ResizeEvent();

	/// Called when a key is released by the user
	int KeyReleased(int k);
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(int k,bool isRepeat);


private:
	void RandomStartPicture(const std::string& sidePref);
	void LoadStartPicture(const std::string& picture);
	void UnloadStartPicture();
	void Stop();

private:
	static CLoadScreen* singleton;

	std::string oldLoadMessages;
	std::string curLoadMessage;

	std::string mapName;
	std::string modName;
	ILoadSaveHandler* saveFile;

	boost::recursive_mutex mutex;
	boost::thread* netHeartbeatThread;
	COffscreenGLThread* gameLoadThread;

	bool mtLoading;
	bool showMessages;

	unsigned int startupTexture;
	float aspectRatio;
	spring_time last_draw;
};


#define loadscreen CLoadScreen::GetInstance()


#endif // _LOAD_SCREEN_H
